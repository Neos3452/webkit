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
#include "LocalNetworkDiscoveryPermissionRequestManagerProxy.h"

#include "Logging.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"

namespace WebKit {

LocalNetworkDiscoveryPermissionRequestManagerProxy::LocalNetworkDiscoveryPermissionRequestManagerProxy(WebPageProxy& page)
    : m_page(page)
{
}

void LocalNetworkDiscoveryPermissionRequestManagerProxy::invalidateRequests()
{
    for (auto& request : m_pendingRequests.values())
        request->invalidate();

    m_pendingRequests.clear();
}

Ref<LocalNetworkDiscoveryPermissionRequestProxy> LocalNetworkDiscoveryPermissionRequestManagerProxy::createRequest(uint64_t discoveryID)
{
    Ref<LocalNetworkDiscoveryPermissionRequestProxy> request = LocalNetworkDiscoveryPermissionRequestProxy::create(this, discoveryID);
    m_pendingRequests.add(discoveryID, request.ptr());
    return request;
}

void LocalNetworkDiscoveryPermissionRequestManagerProxy::didReceiveLocalNetworkDiscoveryPermissionDecision(uint64_t discoveryID, bool allowed)
{
    if (!m_page.isValid())
        return;

    auto it = m_pendingRequests.find(discoveryID);
    if (it == m_pendingRequests.end())
        return;

    LOG(WebDial, "LocalNetworkDiscoveryPermissionRequestManagerProxy::didReceiveLocalNetworkDiscoveryPermissionDecision() id:%llu, allowed:%d", discoveryID, allowed);

#if ENABLE(WEB_DIAL)
    m_page.process().send(Messages::WebPage::DidReceiveLocalNetworkDiscoveryPermissionDecision(discoveryID, allowed), m_page.pageID());
#else
    UNUSED_PARAM(allowed);
#endif
    
    m_pendingRequests.remove(it);
}
    
} // namespace WebKit
