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
#include "LocalNetworkDiscoveryPermissionRequestManager.h"

#if ENABLE(WEB_DIAL)

#include "Logging.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityOriginData.h>

using namespace WebCore;

namespace WebKit {

static uint64_t generateDiscoveryID()
{
    static uint64_t uniqueDiscoveryID = 1;
    return uniqueDiscoveryID++;
}

LocalNetworkDiscoveryPermissionRequestManager::LocalNetworkDiscoveryPermissionRequestManager(WebPage& page)
: m_page(page)
{
}

void LocalNetworkDiscoveryPermissionRequestManager::startRequestForLocalNetworkDiscovery(LocalNetworkEnumerationRequest& requester)
{
    auto origin = requester.localNetworkDevicesDocumentOrigin();
    if (!origin) {
        requester.permissionDenied();
        return;
    }

    Document* document = downcast<Document>(requester.scriptExecutionContext());
    Frame* frame = document ? document->frame() : nullptr;

    if (!frame) {
        requester.permissionDenied();
        return;
    }

    uint64_t discoveryID = generateDiscoveryID();

    m_localNetworkDevicesToIDMap.set(&requester, discoveryID);
    m_idToLocalNetworkDevicesMap.set(discoveryID, &requester);

    WebFrame* webFrame = WebFrame::fromCoreFrame(*frame);
    ASSERT(webFrame);

    m_page.send(Messages::WebPageProxy::RequestLocalNetworkDiscoveryPermissionForFrame(discoveryID, webFrame->frameID(), SecurityOriginData::fromSecurityOrigin(*origin).databaseIdentifier()));
}

void LocalNetworkDiscoveryPermissionRequestManager::cancelRequestForLocalNetworkDiscovery(LocalNetworkEnumerationRequest& requester)
{
    if (const auto requestID = m_localNetworkDevicesToIDMap.take(&requester))
        m_idToLocalNetworkDevicesMap.remove(requestID);
}

void LocalNetworkDiscoveryPermissionRequestManager::didReceiveLocalNetworkDiscoveryPermissionDecision(uint64_t discoveryID, bool allowed)
{
    RefPtr<LocalNetworkEnumerationRequest> requester = m_idToLocalNetworkDevicesMap.take(discoveryID);
    LOG(WebDial, "LocalNetworkDiscoveryPermissionRequestManager::didReceiveLocalNetworkDiscoveryPermissionDecision() requester:%p allowed:%d", requester.get(), allowed);
    if (requester) {
        m_localNetworkDevicesToIDMap.remove(requester.get());

        requester->permissionGranted();
    }
}
    
} // namespace WebKit

#endif // ENABLE(WEB_DIAL)
