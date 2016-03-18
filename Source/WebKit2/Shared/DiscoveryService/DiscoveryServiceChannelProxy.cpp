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
#include "DiscoveryServiceChannelProxy.h"

#include "Logging.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"

namespace WebKit {

Ref<DiscoveryServiceChannelProxy> DiscoveryServiceChannelProxy::create(WebPage& page)
{
    return adoptRef(*new DiscoveryServiceChannelProxy(page));
}

DiscoveryServiceChannelProxy::DiscoveryServiceChannelProxy(WebPage& page)
    : m_page(page)
{
}

void DiscoveryServiceChannelProxy::open()
{
    LOG(WebDial, "DiscoveryServiceChannelProxy::open");
    m_page.send(Messages::WebPageProxy::DidOpenDialChannel());
    // TODO implement message caching
}

void DiscoveryServiceChannelProxy::reopen()
{
    m_page.send(Messages::WebPageProxy::ReopenDialChannel());
}

size_t DiscoveryServiceChannelProxy::send(const uint8_t* data, size_t size)
{
    // TODO: handle send async so we can detecta an error here
    Vector<uint8_t> buffer;
    buffer.append(data, size);
    m_page.send(Messages::WebPageProxy::SendDialMessage(buffer));
    return size;
}

void DiscoveryServiceChannelProxy::close()
{
    m_page.send(Messages::WebPageProxy::CloseDialChannel());
}

void DiscoveryServiceChannelProxy::messageReceived(const Vector<uint8_t>& data)
{
    if (m_messageCallback) {
        m_messageCallback(data.data(), data.size());
        return;
    }
    LOG(WebDial, "DiscoveryServiceChannelProxy::messageReceived RemoteApplication was not yet connected");
}

void DiscoveryServiceChannelProxy::errorReceived(const String& reason)
{
    if (m_errorCallback) {
        m_errorCallback(reason);
        return;
    }
    LOG(WebDial, "DiscoveryServiceChannelProxy::messageReceived RemoteApplication was not yet connected");
}

void DiscoveryServiceChannelProxy::closedReceived()
{
    if (m_closedCallback) {
        m_closedCallback();
        return;
    }
    LOG(WebDial, "DiscoveryServiceChannelProxy::messageReceived RemoteApplication was not yet connected");
}

} // namespace WebKit

#endif // ENABLE(WEB_DIAL)
