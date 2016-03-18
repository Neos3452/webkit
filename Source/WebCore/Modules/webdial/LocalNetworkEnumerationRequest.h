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

#ifndef LocalNetworkEnumerationRequest_h
#define LocalNetworkEnumerationRequest_h

#if ENABLE(WEB_DIAL)

#include "ContextDestructionObserver.h"
#include "LocalNetworkDevices.h"
#include "LocalNetworkDiscoveryProvider.h"
#include <memory>
#include <wtf/RefCounted.h>

namespace WebCore {

class LocalNetworkDiscoveryController;

class LocalNetworkEnumerationRequest : public ContextDestructionObserver, public RefCounted<LocalNetworkEnumerationRequest> {
public:
    static ExceptionOr<void> start(Document*, LocalNetworkDevices::EnumeratePromise&&);

    ~LocalNetworkEnumerationRequest();

    WEBCORE_EXPORT SecurityOrigin* localNetworkDevicesDocumentOrigin() const;
    WEBCORE_EXPORT SecurityOrigin& topLevelDocumentOrigin() const;

    void start();
    WEBCORE_EXPORT void permissionGranted();
    WEBCORE_EXPORT void permissionDenied();

    void discoveryHasFinished(LocalNetworkDeviceVector&&);
    void failedToPerformDiscovery(const ExceptionCode&);

private:
    LocalNetworkEnumerationRequest(ScriptExecutionContext*, LocalNetworkDiscoveryController*, LocalNetworkDevices::EnumeratePromise&&);

    // ContextDestructionObserver
    void contextDestroyed() final;
    
    LocalNetworkDiscoveryController* m_controller;
    LocalNetworkDevices::EnumeratePromise m_promise;
    std::unique_ptr<LocalNetworkDiscoveryProvider> m_provider;
};

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)

#endif // LocalNetworkEnumerationRequest_h
