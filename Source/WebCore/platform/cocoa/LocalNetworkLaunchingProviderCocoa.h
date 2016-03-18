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

#ifndef LocalNetworkLaunchingProviderCocoa_h
#define LocalNetworkLaunchingProviderCocoa_h

#if ENABLE(WEB_DIAL)

#include "DiscoveryServiceChannel.h"
#include "LocalNetworkLaunchingProvider.h"
#include <memory>
#include <wtf/Optional.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

OBJC_CLASS NSNetService;
OBJC_CLASS WebNetServiceDelegate;

namespace WebCore {

class LocalNetworkLaunchingProviderCocoa final : public LocalNetworkLaunchingProvider {
public:
    static Ref<LocalNetworkLaunchingProviderCocoa> create(RetainPtr<NSNetService>&&);
    ~LocalNetworkLaunchingProviderCocoa();

    void launch(ScriptExecutionContext&, const String& url, LocalNetworkLaunchingProvider::Promise&&) override;
    void cancelLaunch() override;

    void connected(Ref<DiscoveryServiceChannel>&&);
    void launchSuccessful(Ref<RemoteApplication>&&);
    void launchFailed();

    const String& launchUrl() const { return m_launchUrl; }

    // RefCounted
    using LocalNetworkLaunchingProvider::ref;
    using LocalNetworkLaunchingProvider::deref;

private:
    LocalNetworkLaunchingProviderCocoa(RetainPtr<NSNetService>&&);

    bool m_currentlyLaunching;
    ScriptExecutionContext* m_context;
    RetainPtr<NSNetService> m_service;
    RetainPtr<WebNetServiceDelegate> m_resolvingDelegate;
    std::optional<LocalNetworkLaunchingProvider::Promise> m_currentPromise;
    String m_launchUrl;
    RefPtr<LocalNetworkLaunchingProviderCocoa> m_protect;
};

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
#endif // LocalNetworkLaunchingProviderCocoa_h
