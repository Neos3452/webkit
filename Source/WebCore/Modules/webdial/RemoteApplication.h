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

#ifndef RemoteApplication_h
#define RemoteApplication_h

#if ENABLE(WEB_DIAL)

#include "ActiveDOMObject.h"
#include "Blob.h"
#include "DiscoveryServiceChannel.h"
#include "EventTarget.h"
#include "ExceptionOr.h"
#include "GenericEventQueue.h"
#include <runtime/ArrayBufferView.h>
#include <wtf/Ref.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class WebDialMessageMessage;

class RemoteApplication final : public RefCounted<RemoteApplication>, public EventTargetWithInlineData, public ActiveDOMObject {
public:
    using ParameterType = RemoteApplication;
    enum class ConnectionState { Connecting, Connected, Opened, Disconnected };

    static Ref<RemoteApplication> create(ScriptExecutionContext*, Ref<DiscoveryServiceChannel>&&);
    static RefPtr<RemoteApplication> createAndLaunch(ScriptExecutionContext*, Ref<DiscoveryServiceChannel>&&, const String& url);

    // EventTarget interface
    ScriptExecutionContext* scriptExecutionContext() const override { return ActiveDOMObject::scriptExecutionContext(); }
    EventTargetInterface eventTargetInterface() const override { return RemoteApplicationEventTargetInterfaceType; }

    using RefCounted<RemoteApplication>::ref;
    using RefCounted<RemoteApplication>::deref;

    // ActiveDOMObject API.
    bool hasPendingActivity() const override;

    // RemoteApplication.idl
    ConnectionState connectionState() const { return m_connectionState; }
    ExceptionOr<void> reconnect();
    void disconnect();
    ExceptionOr<void> send(const String&);
    ExceptionOr<void> send(Blob&);
    ExceptionOr<void> send(ArrayBuffer&);
    ExceptionOr<void> send(Ref<ArrayBufferView>&&);

protected:
    // EventTarget interface
    void refEventTarget() override { ref(); }
    void derefEventTarget() override { deref(); }

private:
    RemoteApplication(ScriptExecutionContext*, Ref<DiscoveryServiceChannel>&&);
    
    // ActiveDOMObject API.
    const char* activeDOMObjectName() const override;
    bool canSuspendForDocumentSuspension() const override;

    // EventTarget helper
    void scheduleEvent(Ref<Event> &&event);

    void changeState(ConnectionState newState, const String& disconnectReason = emptyString());
    ExceptionOr<void> canSendMessage() const;

    bool launch(const String& url);

    void messageReceived(const uint8_t* data, size_t size);
    void handleMessageMessage(const WebDialMessageMessage&);

    // EventTarget helper
    GenericEventQueue m_asyncEventQueue;

    ConnectionState m_connectionState;
    String m_disconnectReason;

    Ref<DiscoveryServiceChannel> m_channel;
};

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)

#endif // RemoteApplication_h
