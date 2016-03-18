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
#include "DiscoveryService.h"

#include "DiscoveryServiceClient.h"
#include "Logging.h"
#include <WebCore/ResourceRequest.h>
#include <WebCore/URL.h>
#include <WebCore/WebDialMessages.h>

namespace WebKit {

DiscoveryService::DiscoveryService()
    : m_client(nullptr)
{
    m_private = DiscoveryServicePrivate::create(this);
}

DiscoveryService::~DiscoveryService()
{
    stop();
}

void DiscoveryService::start()
{
    LOG(WebDial, "DiscoveryService::start()");
    m_private->publishService("_webdial._tcp");
}

void DiscoveryService::stop()
{
    LOG(WebDial, "DiscoveryService::stop()");
    m_private->stopService();
}

void DiscoveryService::servicePublishedSuccesfully()
{
    LOG(WebDial, "DiscoveryService::servicePublishedSuccesfully()");
    ASSERT(m_client);
}

void DiscoveryService::serviceStopped()
{
    LOG(WebDial, "DiscoveryService::serviceStopped()");
    ASSERT(m_client);
}

void DiscoveryService::channelEstablished(Ref<WebCore::DiscoveryServiceChannel>&& channel)
{
    channel->setMessageCallback(std::bind(&DiscoveryService::messageReceived, this, channel.ptr(), std::placeholders::_1, std::placeholders::_2));
    channel->setErrorCallback(std::bind(&DiscoveryService::channelClosed, this, channel.ptr(), std::placeholders::_1));
    channel->setClosedCallback(std::bind(&DiscoveryService::channelClosed, this, channel.ptr(), "closed"));
    channel->open();
    m_channels.add(WTFMove(channel));
}

void DiscoveryService::messageReceived(WebCore::DiscoveryServiceChannel* channel, const uint8_t* data, size_t size)
{
    LOG(WebDial, "DiscoveryService::messageReceived() Got message of size %zu", size);
    Ref<WebCore::DiscoveryServiceChannel> channelRef = *m_channels.take(channel);

    RefPtr<WebCore::WebDialMessage> message = WebCore::WebDialMessage::deserialize(data, size);
    if (!message) {
        LOG(WebDial, "DiscoveryService::messageReceived() Could not deserialize message");
        return;
    }
    LOG(WebDial, "DiscoveryService::messageReceived() message of type %s", toString(message->type()));
    switch (message->type()) {
        case WebCore::WebDialMessageType::Request:
        {
            WebCore::WebDialMessageRequest& request = static_cast<WebCore::WebDialMessageRequest&>(*message);
            LOG(WebDial, "DiscoveryService::messageReceived() url:%s", request.url().ascii().data());
            ASSERT(m_client);

            RefPtr<WebPageProxy> webPageProxy = m_client->launchNewPage(this);
            if (webPageProxy) {
                Ref<DiscoveryServiceInvoker> invoker = DiscoveryServiceInvoker::create(*webPageProxy, channelRef.copyRef());
                webPageProxy->setDiscoveryServiceInvoker(WTFMove(invoker));
                webPageProxy->loadRequest(WebCore::ResourceRequest(WebCore::URL(WebCore::URL(), request.url())));

                const auto launchedMessage = WebCore::WebDialMessageLaunched::create()->serialize();
                channelRef->send(launchedMessage.data(), launchedMessage.size());
            } else {
                const auto disconnectMessage = WebCore::WebDialMessageDisconnect::create("Permission denied")->serialize();
                channelRef->send(disconnectMessage.data(), disconnectMessage.size());
                channelRef->close();
            }
            break;
        }
        default:
            LOG(WebDial, "DiscoveryService::messageReceived() First message must be a launch request");
    }
}

void DiscoveryService::channelClosed(WebCore::DiscoveryServiceChannel* channel, const String& reason)
{
    LOG(WebDial, "DiscoveryService::messageReceived() Channel closed because: %s", reason.ascii().data());
    m_channels.remove(channel);
}

} // namespace WebKit

#endif // ENABLE(WEB_DIAL)
