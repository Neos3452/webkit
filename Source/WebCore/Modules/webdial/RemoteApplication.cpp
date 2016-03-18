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
#include "RemoteApplication.h"

#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "Logging.h"
#include "MessageEvent.h"
#include "NotImplemented.h"
#include "StateChangedEvent.h"
#include "WebDialMessages.h"

namespace {

const char* stateAsString(WebCore::RemoteApplication::ConnectionState state) {
    switch(state) {
        case WebCore::RemoteApplication::ConnectionState::Connecting:
            return "ConnectionState::Connecting";
        case WebCore::RemoteApplication::ConnectionState::Connected:
            return "ConnectionState::Connected";
        case WebCore::RemoteApplication::ConnectionState::Opened:
            return "ConnectionState::Opened";
        case WebCore::RemoteApplication::ConnectionState::Disconnected:
            return "ConnectionState::Disconnected";
    }
    ASSERT_NOT_REACHED();
}

} // anonymouse namespace

namespace WebCore {

Ref<RemoteApplication> RemoteApplication::create(ScriptExecutionContext* context, Ref<DiscoveryServiceChannel>&& channel)
{
    auto application = adoptRef(*new RemoteApplication(context, WTFMove(channel)));
    application->suspendIfNeeded();
    // right now RemoteApplication should always receive opened channel from remote invoker
    application->m_connectionState = ConnectionState::Opened;
    return application;
}

RefPtr<RemoteApplication> RemoteApplication::createAndLaunch(ScriptExecutionContext* context, Ref<DiscoveryServiceChannel>&& channel, const String& url)
{
    auto application = RemoteApplication::create(context, WTFMove(channel));

    if (!application->launch(url))
        return nullptr;
    // if launching succeded we are waiting for the connection to establish
    application->m_connectionState = ConnectionState::Connecting;
    return WTFMove(application);
}

RemoteApplication::RemoteApplication(ScriptExecutionContext* context, Ref<DiscoveryServiceChannel>&& channel)
    : ActiveDOMObject(context)
    , m_asyncEventQueue(*this)
    , m_connectionState(ConnectionState::Disconnected)
    , m_channel(WTFMove(channel))
{
    LOG(WebDial, "RemoteApplication::RemoteApplication creating for channel %p", m_channel.ptr());
    m_channel->setMessageCallback([this](const uint8_t* const data, const size_t size){
        messageReceived(data, size);
    });
    m_channel->setErrorCallback([this](const String& reason){
        changeState(ConnectionState::Disconnected, reason);
    });
    m_channel->setClosedCallback([this](){
        changeState(ConnectionState::Disconnected, "closed");
    });
}

bool RemoteApplication::hasPendingActivity() const
{
    return m_asyncEventQueue.hasPendingEvents();
}

ExceptionOr<void> RemoteApplication::reconnect()
{
    if (m_connectionState != ConnectionState::Disconnected) {
        return Exception{INVALID_STATE_ERR};
    }
    notImplemented();
    return {};
}

void RemoteApplication::disconnect()
{
    const auto disconnectMessage = WebCore::WebDialMessageDisconnect::create(emptyString())->serialize();
    m_channel->send(disconnectMessage.data(), disconnectMessage.size());
}

ExceptionOr<void> RemoteApplication::canSendMessage() const
{
    if (m_connectionState != ConnectionState::Opened)
        return Exception{INVALID_STATE_ERR};
    return { };
}

ExceptionOr<void> RemoteApplication::send(const String& msg)
{
    auto exception = canSendMessage();
    if (!exception.hasException()) {
        const auto message = WebCore::WebDialMessageTextMessage::create(msg)->serialize();
        m_channel->send(message.data(), message.size());
    }
    return exception;
}

ExceptionOr<void> RemoteApplication::send(Blob& msg)
{
    auto exception = canSendMessage();
    if (!exception.hasException()) {
        const auto message = WebCore::WebDialMessageBlobMessage::create(Ref<Blob>(msg))->serialize();
        m_channel->send(message.data(), message.size());
    }
    return exception;
}

ExceptionOr<void> RemoteApplication::send(ArrayBuffer& msg)
{
    auto exception = canSendMessage();
    if (!exception.hasException()) {
        const auto message = WebCore::WebDialMessageArrayBufferMessage::create(Ref<ArrayBuffer>(msg))->serialize();
        m_channel->send(message.data(), message.size());
    }
    return exception;
}

ExceptionOr<void> RemoteApplication::send(Ref<ArrayBufferView>&& msg)
{
    if (msg->isNeutered())
        return Exception{TypeError};
    return send(*msg->possiblySharedBuffer());
}

bool RemoteApplication::canSuspendForDocumentSuspension() const
{
    return !hasPendingActivity();
}

const char* RemoteApplication::activeDOMObjectName() const
{
    return "RemoteApplication";
}

void RemoteApplication::scheduleEvent(Ref<Event> &&event)
{
    m_asyncEventQueue.enqueueEvent(WTFMove(event));
}

void RemoteApplication::changeState(const ConnectionState newState, const String& disconnectReason)
{
    LOG(WebDial, "RemoteApplication::changeState() changing from %s to %s, reason: %s", stateAsString(m_connectionState), stateAsString(newState), disconnectReason.utf8().data());
    m_connectionState = newState;
    if (m_connectionState == ConnectionState::Disconnected)
        m_disconnectReason = disconnectReason;

    scheduleEvent(StateChangedEvent::create(
        eventNames().statechangeEvent,
//        m_connectionState,
        m_disconnectReason));
}

bool RemoteApplication::launch(const String& url)
{
    m_channel->open();

    const auto request = WebDialMessageRequest::create(url)->serialize();
    const auto bytes = m_channel->send(request.data(), request.size());

    return bytes == request.size();
}

void RemoteApplication::messageReceived(const uint8_t* data, size_t size)
{
    LOG(WebDial, "RemoteApplication::messageReceived() Got message of size %zu", size);

    RefPtr<WebDialMessage> message = WebCore::WebDialMessage::deserialize(data, size);
    if (!message) {
        LOG(WebDial, "RemoteApplication::messageReceived() Could not deserialize message");
        return;
    }
    LOG(WebDial, "RemoteApplication::messageReceived() message of type %s", toString(message->type()));
    switch (message->type()) {
        case WebDialMessageType::Message:
            if (m_connectionState == ConnectionState::Opened) {
                handleMessageMessage(static_cast<const WebDialMessageMessage&>(*message));
            } else
                LOG(WebDial, "RemoteApplication::messageReceived()  received message not in opened state");
            break;
        case WebDialMessageType::Error:
            changeState(ConnectionState::Disconnected, static_cast<const WebDialMessageError&>(*message).errorMessage());
            m_channel->close();
            break;
        case WebDialMessageType::Disconnect: {
            const auto& reason = static_cast<const WebDialMessageDisconnect&>(*message).reason();
            changeState(ConnectionState::Disconnected, reason.isEmpty() ? "closed" : reason);
            m_channel->close();
            break;
        }
        case WebDialMessageType::Launched:
            if (ConnectionState::Connecting == m_connectionState)
                changeState(ConnectionState::Connected);
            else
                LOG(WebDial, "RemoteApplication::messageReceived() Launch confirmation in illegal state %s", stateAsString(m_connectionState));

            break;
        case WebDialMessageType::Opened:
            if (ConnectionState::Connected == m_connectionState)
                changeState(ConnectionState::Opened);
            else
                LOG(WebDial, "RemoteApplication::messageReceived() Open confirmation in illegal state %s", stateAsString(m_connectionState));

            break;
        case WebDialMessageType::Request:
            LOG(WebDial, "RemoteApplication::messageReceived() Illegal message");
    }
}

void RemoteApplication::handleMessageMessage(const WebDialMessageMessage& message)
{
    switch (message.dataType()) {
        case WebDialDataType::Text:
            scheduleEvent(MessageEvent::create(static_cast<const WebDialMessageTextMessage&>(message).data()));
            break;
        case WebDialDataType::Binary:
            switch (static_cast<const WebDialMessageBinaryMessage&>(message).binaryType()) {
                case WebDialBinaryType::ArrayBuffer:
                    scheduleEvent(MessageEvent::create(static_cast<const WebDialMessageArrayBufferMessage&>(message).data().copyRef()));
                    break;
                case WebDialBinaryType::Blob:
                    scheduleEvent(MessageEvent::create(static_cast<const WebDialMessageBlobMessage&>(message).data().copyRef(), {}));
                    break;
            }
            break;
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
