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
#include "LocalNetworkEnumerationRequest.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "JSLocalNetworkDevice.h"
#include "LocalNetworkDiscoveryController.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

ExceptionOr<void> LocalNetworkEnumerationRequest::start(Document* document, LocalNetworkDevices::EnumeratePromise&& promise)
{
    LocalNetworkDiscoveryController* discoveryController = LocalNetworkDiscoveryController::from(document ? document->page() : nullptr);
    if (!discoveryController) {
        return Exception{NOT_SUPPORTED_ERR};
    }

    Ref<LocalNetworkEnumerationRequest> request = adoptRef(*new LocalNetworkEnumerationRequest(document, discoveryController, WTFMove(promise)));
    request->start();
    return { };
}

LocalNetworkEnumerationRequest::LocalNetworkEnumerationRequest(ScriptExecutionContext* context, LocalNetworkDiscoveryController* controller, LocalNetworkDevices::EnumeratePromise&& promise)
    : ContextDestructionObserver(context)
    , m_controller(controller)
    , m_promise(WTFMove(promise))
{
}

LocalNetworkEnumerationRequest::~LocalNetworkEnumerationRequest()
{
}

SecurityOrigin* LocalNetworkEnumerationRequest::localNetworkDevicesDocumentOrigin() const
{
    if (!m_scriptExecutionContext)
        return nullptr;

    return m_scriptExecutionContext->securityOrigin();
}

SecurityOrigin& LocalNetworkEnumerationRequest::topLevelDocumentOrigin() const
{
    ASSERT(m_scriptExecutionContext);
    return m_scriptExecutionContext->topOrigin();
}

void LocalNetworkEnumerationRequest::start()
{
    m_controller->requestPermission(*this);
}

void LocalNetworkEnumerationRequest::permissionGranted()
{
    if (!m_scriptExecutionContext)
        return;

    LOG(WebDial, "LocalNetworkEnumerationRequest::permissionGranted()");
    m_provider = LocalNetworkDiscoveryProvider::create(*this);
    m_provider->startDiscovery();
}

void LocalNetworkEnumerationRequest::permissionDenied()
{
    if (!m_scriptExecutionContext)
        return;

    LOG(WebDial, "LocalNetworkEnumerationRequest::permissionDenied()");
    m_promise.reject(SECURITY_ERR);
}

void LocalNetworkEnumerationRequest::discoveryHasFinished(LocalNetworkDeviceVector&& devices)
{
    m_promise.resolve(WTFMove(devices));
}

void LocalNetworkEnumerationRequest::failedToPerformDiscovery(const ExceptionCode& ec)
{
    m_promise.reject(ec);
}

void LocalNetworkEnumerationRequest::contextDestroyed()
{
    Ref<LocalNetworkEnumerationRequest> protect(*this);
    
    if (m_controller) {
        m_controller->cancelPermissionRequest(*this);
        m_controller = nullptr;
    }
    if (m_provider) {
        m_provider->cancelDiscovery();
        m_provider = nullptr;
    }
    
    ContextDestructionObserver::contextDestroyed();
}
    
} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
