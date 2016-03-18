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

#include "config.h"

#if ENABLE(WEB_DIAL)
#include "LocalNetworkDiscoveryProviderCocoa.h"

#include "ExceptionCode.h"
#import <Foundation/Foundation.h>
#include "LocalNetworkDeviceDescriptor.h"
#include "LocalNetworkEnumerationRequest.h"
#import "LocalNetworkLaunchingProviderCocoa.h"
#include "Logging.h"

using namespace WebCore;

@interface WebNetServiceBrowserDelegate : NSObject <NSNetServiceBrowserDelegate>
@end

@implementation WebNetServiceBrowserDelegate {
    LocalNetworkDiscoveryProviderCocoa *_provider;
}

- (id)initWithProvider:(LocalNetworkDiscoveryProviderCocoa *)provider
{
    if ( self = [super init]) {
        _provider = provider;
        return self;
    }
    return nil;
}

- (void)netServiceBrowser:(NSNetServiceBrowser *) browser
             didNotSearch:(NSDictionary *)errorDict
{
    UNUSED_PARAM(browser);
    UNUSED_PARAM(errorDict);
    _provider->browseFailed();
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)browser
           didFindService:(NSNetService *)aNetService
               moreComing:(BOOL)moreComing
{
    UNUSED_PARAM(browser);
    _provider->deviceDiscovered(LocalNetworkDevice::create(LocalNetworkDeviceDescriptor::create([aNetService name]), LocalNetworkLaunchingProviderCocoa::create(RetainPtr<NSNetService>(aNetService))));
    if(!moreComing)
    {
        _provider->discoveryFinished();
    }
}

- (void)netServiceBrowserWillSearch:(NSNetServiceBrowser *) browser
{
    UNUSED_PARAM(browser);
    LOG(WebDial, "Local network discovery has started");
}

@end

namespace WebCore {

std::unique_ptr<LocalNetworkDiscoveryProvider> LocalNetworkDiscoveryProvider::create(LocalNetworkEnumerationRequest& request)
{
    return std::make_unique<LocalNetworkDiscoveryProviderCocoa>(request);
}

LocalNetworkDiscoveryProviderCocoa::LocalNetworkDiscoveryProviderCocoa(LocalNetworkEnumerationRequest& request)
    : m_discoveryTimeoutTimer(*this, &LocalNetworkDiscoveryProviderCocoa::discoveryFinished, WTF::Seconds(5))
    , m_request(&request)
{
    m_browsingDelegate = adoptNS([[WebNetServiceBrowserDelegate alloc] initWithProvider:this]);
}

void LocalNetworkDiscoveryProviderCocoa::startDiscovery()
{
    ASSERT(m_request);

    m_browser = adoptNS([[NSNetServiceBrowser alloc] init]);
    [m_browser setDelegate:m_browsingDelegate.get()];
    [m_browser searchForServicesOfType:@"_webdial._tcp." inDomain:@"local."];
    m_discoveryTimeoutTimer.restart();
}

void LocalNetworkDiscoveryProviderCocoa::cancelDiscovery()
{
    [m_browser stop];
    m_discoveryTimeoutTimer.stop();
}

LocalNetworkDiscoveryProviderCocoa::~LocalNetworkDiscoveryProviderCocoa()
{
    cancelDiscovery();
}

void LocalNetworkDiscoveryProviderCocoa::browseFailed()
{
    cancelDiscovery();
    m_request->failedToPerformDiscovery(NETWORK_ERR);
    m_request = nullptr;
}

void LocalNetworkDiscoveryProviderCocoa::deviceDiscovered(Ref<LocalNetworkDevice>&& device)
{
    LOG(WebDial, "Discovered service at %s", device->descriptor().userAgent().ascii().data());
    m_devices.append(WTFMove(device));
}

void LocalNetworkDiscoveryProviderCocoa::discoveryFinished()
{
    cancelDiscovery();
    LOG(WebDial, "Number of discovered services %zu", m_devices.size());
    m_request->discoveryHasFinished(WTFMove(m_devices));
    m_request = nullptr;
}

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
