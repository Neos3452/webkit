/*
 * Copyright (C) 2016 Michal Debski <mi.zd.debski@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"

#if ENABLE(WEB_DIAL)
#import "DiscoveryServicePrivateCocoa.h"

#import "Logging.h"
#include <arpa/inet.h>
#import <Foundation/Foundation.h>
#include <netinet/in.h>
#include <sys/socket.h>
#import <wtf/text/WTFString.h>
#import <wtf/Vector.h>
#import <WebCore/DiscoveryServiceChannelCocoa.h>
#import <WebCore/NotImplemented.h>

using namespace WebKit;

@interface WKNetServiceDelegate : NSObject <NSNetServiceDelegate>
@end

@implementation WKNetServiceDelegate {
    DiscoveryServicePrivateClient *_service;
}

- (id)initWithDiscoveryService:(DiscoveryServicePrivateClient *)service
{
    if ( self = [super init]) {
        _service = service;
        return self;
    }
    return nil;
}

- (void)netServiceDidPublish:(NSNetService *)sender
{
    _service->servicePublishedSuccesfully();
}

- (void)netServiceDidStop:(NSNetService *)sender
{
    _service->serviceStopped();
}

- (void)netService:(NSNetService *)sender didNotPublish:(NSDictionary<NSString *, NSNumber *> *)errorDict
{
    LOG(WebDial, "netService didNotPublish");
    notImplemented();
}

- (void)netService:(NSNetService *)sender
didAcceptConnectionWithInputStream:(NSInputStream *)inputStream outputStream:(NSOutputStream *)outputStream
{
    LOG(WebDial, "netService didAcceptConnectionWithInputStream");

    _service->channelEstablished(WebCore::DiscoveryServiceChannelCocoa::create(adoptNS(inputStream), adoptNS(outputStream)));
}

@end

extern "C" {

void DiscoveryServicePrivateCocoa::serverAcceptCallBack(CFSocketRef socket, CFSocketCallBackType type, CFDataRef address, const void *data, void *info)
{
    ASSERT(type == kCFSocketAcceptCallBack);
    UNUSED_PARAM(type);

    LOG(WebDial, "DiscoveryServicePrivateCocoa::serverAcceptCallBack() Started");

    DiscoveryServicePrivateCocoa *server = static_cast<DiscoveryServicePrivateCocoa*>(info);

    char ipStr[INET6_ADDRSTRLEN];

    NSData* addrData = static_cast<NSData*>(address);
    if (socket == server->m_ipv4socket.get()) {
        inet_ntop(AF_INET, &(static_cast<const sockaddr_in*>([addrData bytes])->sin_addr), ipStr, INET_ADDRSTRLEN);
        LOG(WebDial, "DiscoveryServicePrivateCocoa::serverAcceptCallBack() Connection IPv4 from: %s", ipStr);
    } else if (socket == server->m_ipv6socket.get()) {
        inet_ntop(AF_INET6, &(static_cast<const sockaddr_in6*>([addrData bytes])->sin6_addr), ipStr, INET6_ADDRSTRLEN);
        LOG(WebDial, "DiscoveryServicePrivateCocoa::serverAcceptCallBack() Connection IPv6 from: %s", ipStr);
    } else {
        ASSERT_NOT_REACHED();
    }

    CFReadStreamRef readStream = nullptr;
    CFWriteStreamRef writeStream = nullptr;
    CFStreamCreatePairWithSocket(kCFAllocatorDefault, *static_cast<const CFSocketNativeHandle*>(data), &readStream, &writeStream);
    if (readStream && writeStream) {
        CFReadStreamSetProperty(readStream, kCFStreamPropertyShouldCloseNativeSocket, kCFBooleanTrue);
        CFWriteStreamSetProperty(writeStream, kCFStreamPropertyShouldCloseNativeSocket, kCFBooleanTrue);

        [server->m_delegate netService: server->m_service.get() didAcceptConnectionWithInputStream: static_cast<NSInputStream*>(CFRetain(readStream)) outputStream: static_cast<NSOutputStream*>(CFRetain(writeStream))];
    } else {
        LOG(WebDial, "DiscoveryServicePrivateCocoa::serverAcceptCallBack() Error creating streams");
        // On any failure, we need to destroy the CFSocketNativeHandle
        // since we are not going to use it any more.
        close(*static_cast<const CFSocketNativeHandle*>(data));

        // FIXME: error message
        RetainPtr<NSDictionary> error = adoptNS([NSDictionary dictionary]);
        [server->m_delegate netService: server->m_service.get() didNotPublish:error.get()];
    }
    if (readStream) CFRelease(readStream);
    if (writeStream) CFRelease(writeStream);
}

}

namespace WebKit {

DiscoveryServicePrivateCocoa::DiscoveryServicePrivateCocoa(DiscoveryServicePrivateClient* client)
    : m_client(client)
{
}

DiscoveryServicePrivateCocoa::~DiscoveryServicePrivateCocoa()
{
}

void DiscoveryServicePrivateCocoa::publishService(const String& serviceTypeName)
{
    LOG(WebDial, "DiscoveryServicePrivateCocoa::publishService(%s)", serviceTypeName.ascii().data());

    ASSERT(!m_ipv4socket && !m_ipv6socket);       // don't call -start twice!

    CFSocketContext socketCtxt = {0, static_cast<void*>(this), nullptr, nullptr, nullptr};
    m_ipv4socket = adoptCF(CFSocketCreate(kCFAllocatorDefault, AF_INET,  SOCK_STREAM, IPPROTO_TCP, kCFSocketAcceptCallBack, &DiscoveryServicePrivateCocoa::serverAcceptCallBack, &socketCtxt));
    m_ipv6socket = adoptCF(CFSocketCreate(kCFAllocatorDefault, AF_INET6, SOCK_STREAM, IPPROTO_TCP, kCFSocketAcceptCallBack, &DiscoveryServicePrivateCocoa::serverAcceptCallBack, &socketCtxt));

    if (!m_ipv4socket || !m_ipv6socket) {
        LOG(WebDial, "DiscoveryServicePrivateCocoa::publishService() Socket creation failed");
        return;
    }

    static const int yes = 1;
    setsockopt(CFSocketGetNative(m_ipv4socket.get()), SOL_SOCKET, SO_REUSEADDR, (const void *) &yes, sizeof(yes));
    setsockopt(CFSocketGetNative(m_ipv6socket.get()), SOL_SOCKET, SO_REUSEADDR, (const void *) &yes, sizeof(yes));

    int flags;
    flags = fcntl(CFSocketGetNative(m_ipv4socket.get()),F_GETFL);
    if (-1 == fcntl(CFSocketGetNative(m_ipv4socket.get()), F_SETFL, flags | O_NONBLOCK)) {
        LOG(WebDial, "DiscoveryServicePrivateCocoa::publishService() Couldn't set socket4 to non-blocking");
        return;
    }

    flags = fcntl(CFSocketGetNative(m_ipv6socket.get()),F_GETFL);
    if (-1 == fcntl(CFSocketGetNative(m_ipv6socket.get()), F_SETFL, flags | O_NONBLOCK)) {
        LOG(WebDial, "DiscoveryServicePrivateCocoa::publishService() Couldn't set socket4 to non-blocking");
        return;
    }

    // Set up the IPv4 listening socket; port is 0, which will cause the kernel to choose a port for us.
    sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_len = sizeof(addr4);
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(0);
    addr4.sin_addr.s_addr = htonl(INADDR_ANY);
    
    NSData* data = [NSData dataWithBytes:&addr4 length:sizeof(addr4)];
    if (kCFSocketSuccess != CFSocketSetAddress(m_ipv4socket.get(), static_cast<CFDataRef>(data))) {
        LOG(WebDial, "DiscoveryServicePrivateCocoa::publishService() Address4 failed");
        return;
    }

    // Now that the IPv4 binding was successful, we get the port number
    // -- we will need it for the IPv6 listening socket and for the NSNetService.
    RetainPtr<NSData> addr = adoptNS(static_cast<NSData*>(CFSocketCopyAddress(m_ipv4socket.get())));
    assert([addr length] == sizeof(struct sockaddr_in));
    const auto port = ntohs(((const sockaddr_in*)[addr bytes])->sin_port);

    // Set up the IPv6 listening socket.
    sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_len = sizeof(addr6);
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(port);
    memcpy(&(addr6.sin6_addr), &in6addr_any, sizeof(addr6.sin6_addr));
    data = [NSData dataWithBytes:&addr6 length:sizeof(addr6)];
    if (kCFSocketSuccess != CFSocketSetAddress(m_ipv6socket.get(), static_cast<CFDataRef>(data))) {
        LOG(WebDial, "DiscoveryServicePrivateCocoa::publishService() Address6 failed");
        return;
    }

    // Set up the run loop sources for the sockets.
    m_source4 = adoptCF(CFSocketCreateRunLoopSource(kCFAllocatorDefault, m_ipv4socket.get(), 0));
    CFRunLoopAddSource(CFRunLoopGetCurrent(), m_source4.get(), kCFRunLoopCommonModes);

    m_source6 = adoptCF(CFSocketCreateRunLoopSource(kCFAllocatorDefault, m_ipv6socket.get(), 0));
    CFRunLoopAddSource(CFRunLoopGetCurrent(), m_source6.get(), kCFRunLoopCommonModes);

    ASSERT(port > 0);

    m_service = adoptNS([[NSNetService alloc] initWithDomain:@"" type:serviceTypeName name:@"" port:port]);
    if (m_service) {
         m_delegate = adoptNS([[WKNetServiceDelegate alloc] initWithDiscoveryService:m_client]);
        [m_service setDelegate:m_delegate.get()];
        [m_service publish];
    } else {
        LOG(WebDial, "DiscoveryServicePrivateCocoa::publishService() Error creating service");
    }
}

void DiscoveryServicePrivateCocoa::stopService()
{
    LOG(WebDial, "DiscoveryServicePrivateCocoa::stopService()");
    [m_service stop];

    if (m_source6) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_source6.get(), kCFRunLoopCommonModes);
        m_source6.clear();
    }
    if (m_source4) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_source4.get(), kCFRunLoopCommonModes);
        m_source4.clear();
    }
    if (m_ipv6socket) {
        CFSocketInvalidate(m_ipv6socket.get());
        m_ipv6socket.clear();
    }
    if (m_ipv4socket) {
        CFSocketInvalidate(m_ipv4socket.get());
        m_ipv4socket.clear();
    }
}

} // namespace WebKit

#endif // ENABLE(WEB_DIAL)
