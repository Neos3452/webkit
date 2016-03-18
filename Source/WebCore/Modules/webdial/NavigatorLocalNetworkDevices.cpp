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
#include "NavigatorLocalNetworkDevices.h"

#include "Document.h"
#include "Frame.h"
#include "LocalNetworkDevices.h"
#include "LocalNetworkDiscoveryController.h"
#include "Navigator.h"
#include "NotImplemented.h"
#include "RemoteApplication.h"
#include "WebDialMessages.h"

namespace WebCore {

NavigatorLocalNetworkDevices::NavigatorLocalNetworkDevices(Frame* frame)
    : DOMWindowProperty(frame)
{
}

NavigatorLocalNetworkDevices* NavigatorLocalNetworkDevices::from(Navigator* navigator)
{
    NavigatorLocalNetworkDevices* supplement
        = static_cast<NavigatorLocalNetworkDevices*>(Supplement<Navigator>::from(navigator, supplementName()));

    if (!supplement) {
        auto newSupplement = std::make_unique<NavigatorLocalNetworkDevices>(navigator->frame());
        supplement = newSupplement.get();
        provideTo(navigator, supplementName(), WTFMove(newSupplement));
    }
    
    return supplement;
}

LocalNetworkDevices* NavigatorLocalNetworkDevices::localNetworkDevices(Navigator& navigator)
{
    return NavigatorLocalNetworkDevices::from(&navigator)->localNetworkDevices();
}

LocalNetworkDevices* NavigatorLocalNetworkDevices::localNetworkDevices() const
{
    if (!m_localNetworkDevices)
        m_localNetworkDevices = LocalNetworkDevices::create(frame()->document());

    return m_localNetworkDevices.get();
}

RemoteApplication* NavigatorLocalNetworkDevices::getRemoteInvoker(Navigator& navigator, ScriptExecutionContext& context)
{
    return NavigatorLocalNetworkDevices::from(&navigator)->getRemoteInvoker(context);
}

RemoteApplication* NavigatorLocalNetworkDevices::getRemoteInvoker(ScriptExecutionContext& context)
{
    if (m_remoteApplication) {
        return m_remoteApplication.get();
    }

    if (frame()->isMainFrame()) {
        if (LocalNetworkDiscoveryController* discoveryController = LocalNetworkDiscoveryController::from(frame()->document()->page())) {
            if (RefPtr<DiscoveryServiceChannel> channel = discoveryController->client().getInvokerChannel()) {
                // channel should be opened because launched message was send
                const auto openedMessage = WebCore::WebDialMessageOpened::create()->serialize();
                channel->send(openedMessage.data(), openedMessage.size());
                m_remoteApplication = RemoteApplication::create(&context, channel.releaseNonNull());
                return m_remoteApplication.get();
            }
        }
    }
    return nullptr;
}

const char* NavigatorLocalNetworkDevices::supplementName()
{
    return "NavigatorLocalNetworkDevices";
}

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
