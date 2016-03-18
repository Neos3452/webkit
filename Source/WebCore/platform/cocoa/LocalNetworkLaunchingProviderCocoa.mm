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
#include "LocalNetworkLaunchingProviderCocoa.h"

#import "DiscoveryServiceChannelCocoa.h"
#include "ExceptionCode.h"
#import <Foundation/Foundation.h>
#include "JSRemoteApplication.h"
#include "Logging.h"
#include "WebDialMessages.h"

using namespace WebCore;

@interface WebNetServiceDelegate : NSObject <NSNetServiceDelegate>
@end

@implementation WebNetServiceDelegate {
    LocalNetworkLaunchingProviderCocoa *_provider;
}

- (id)initWithProvider:(LocalNetworkLaunchingProviderCocoa*)provider
{
    if ( self = [super init]) {
        _provider = provider;
        return self;
    }
    return nil;
}

- (void)netServiceDidResolveAddress:(NSNetService *)sender
{
    UNUSED_PARAM(sender);
    [sender stop];

    NSOutputStream* oStream;
    NSInputStream* iStream;
    [sender getInputStream:&iStream outputStream:&oStream];
    Ref<DiscoveryServiceChannelCocoa> channel = DiscoveryServiceChannelCocoa::create(adoptNS(iStream), adoptNS(oStream));

    _provider->connected(WTFMove(channel));
}

- (void)netService:(NSNetService *)sender didNotResolve:(NSDictionary<NSString *, NSNumber *> *)errorDict
{
    UNUSED_PARAM(sender);
    UNUSED_PARAM(errorDict);
    _provider->launchFailed();
}

@end

namespace WebCore {

Ref<LocalNetworkLaunchingProviderCocoa> LocalNetworkLaunchingProviderCocoa::create(RetainPtr<NSNetService>&& service)
{
    return adoptRef(*new LocalNetworkLaunchingProviderCocoa(WTFMove(service)));
}

LocalNetworkLaunchingProviderCocoa::LocalNetworkLaunchingProviderCocoa(RetainPtr<NSNetService>&& service)
    : m_currentlyLaunching(false)
    , m_context(nullptr)
    , m_service(WTFMove(service))
{
    m_resolvingDelegate = adoptNS([[WebNetServiceDelegate alloc] initWithProvider:this]);
    [m_service setDelegate: m_resolvingDelegate.get()];
}

LocalNetworkLaunchingProviderCocoa::~LocalNetworkLaunchingProviderCocoa()
{
    [m_service stop];
}

void LocalNetworkLaunchingProviderCocoa::launch(ScriptExecutionContext& context, const String& url, LocalNetworkLaunchingProvider::Promise&& promise)
{
    if (m_currentlyLaunching) {
        promise.reject(INVALID_STATE_ERR);
        return;
    }
    m_protect = this;
    m_currentlyLaunching = true;
    m_currentPromise = WTFMove(promise);
    m_context = &context;
    m_launchUrl = url;
    [m_service resolveWithTimeout: 5];
}

void LocalNetworkLaunchingProviderCocoa::cancelLaunch()
{
    [m_service stop];
    m_currentPromise = std::nullopt;
    m_launchUrl.releaseImpl();
    m_currentlyLaunching = false;
    m_protect = nullptr;
}

void LocalNetworkLaunchingProviderCocoa::launchSuccessful(Ref<RemoteApplication>&& application)
{
    ASSERT(m_currentPromise);
    std::exchange(m_currentPromise, std::nullopt)->resolve(WTFMove(application));
    m_launchUrl.releaseImpl();
    m_currentlyLaunching = false;
    m_protect = nullptr;
}

void LocalNetworkLaunchingProviderCocoa::launchFailed()
{
    ASSERT(m_currentPromise);
    std::exchange(m_currentPromise, std::nullopt)->reject(NETWORK_ERR);
    m_launchUrl.releaseImpl();
    m_currentlyLaunching = false;
    m_protect = nullptr;
}

void LocalNetworkLaunchingProviderCocoa::connected(Ref<DiscoveryServiceChannel>&& channel)
{
    ASSERT(m_context);
    auto application = RemoteApplication::createAndLaunch(m_context, WTFMove(channel), launchUrl());
    if (application) {
        launchSuccessful(application.releaseNonNull());
    } else {
        launchFailed();
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
