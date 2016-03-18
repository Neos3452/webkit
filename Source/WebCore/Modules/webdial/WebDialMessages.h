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

#ifndef WebDialMessages_h
#define WebDialMessages_h

#if ENABLE(WEB_DIAL)

#include "Blob.h"
#include <limits>
#include <runtime/ArrayBufferView.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>
#include <wtf/Vector.h>

namespace WebCore {

enum class WebDialMessageType : uint32_t {
    Error = 0,
    Request = 1,
    Launched = 2,
    Opened = 3,
    Disconnect = 4,
    Message = 5,
};

inline const char* toString(const WebDialMessageType& type) {
    switch (type) {
        case WebDialMessageType::Error:
            return "Error";
        case WebDialMessageType::Request:
            return "Request";
        case WebDialMessageType::Launched:
            return "Launched";
        case WebDialMessageType::Opened:
            return "Opened";
        case WebDialMessageType::Disconnect:
            return "Disconnect";
        case WebDialMessageType::Message:
            return "Message";
        default:
            return "UNKNOWN_MESSAGE_TYPE";
    }
}

enum class WebDialDataType : uint8_t {
    Text = 0,
    Binary = std::numeric_limits<uint8_t>::max(),
};

enum class WebDialBinaryType : uint8_t {
    ArrayBuffer = 0,
    Blob = 1,
};

constexpr const uint32_t kWebDialMessageDraftVersion = 1;

class WEBCORE_EXPORT WebDialMessage : public RefCounted<WebDialMessage> {
public:
    static RefPtr<WebDialMessage> deserialize(const uint8_t* data, size_t size);

    virtual ~WebDialMessage() = default;

    virtual Vector<uint8_t> serialize();

    virtual WebDialMessageType type() const = 0;
    uint32_t version() const { return kWebDialMessageDraftVersion; }

protected:
    WebDialMessage() = default;
};

class WEBCORE_EXPORT WebDialMessageError : public WebDialMessage {
public:
    static Ref<WebDialMessageError> create(const String& errorMessage) { return *adoptRef(new WebDialMessageError(errorMessage)); }
    ~WebDialMessageError() = default;

    Vector<uint8_t> serialize() final;

    WebDialMessageType type() const final { return WebDialMessageType::Error; }
    const String& errorMessage() const { return m_errorMessage; }

private:
    WebDialMessageError(const String& errorMessage) : m_errorMessage(errorMessage) {}

    String m_errorMessage;
};

class WEBCORE_EXPORT WebDialMessageRequest : public WebDialMessage {
public:
    static Ref<WebDialMessageRequest> create(const String& url) { return *adoptRef(new WebDialMessageRequest(url)); }
    ~WebDialMessageRequest() = default;

    Vector<uint8_t> serialize() final;

    WebDialMessageType type() const final { return WebDialMessageType::Request; }
    const String& url() const { return m_url; }
    
private:
    WebDialMessageRequest(const String& url) : m_url(url) {}

    String m_url;
};

template <WebDialMessageType messageType>
class WEBCORE_EXPORT WebDialSimpleMessage : public WebDialMessage {
public:
    static Ref<WebDialSimpleMessage> create() { return *adoptRef(new WebDialSimpleMessage()); }
    ~WebDialSimpleMessage() = default;

    WebDialMessageType type() const final { return messageType; }
};

using WebDialMessageLaunched = WebDialSimpleMessage<WebDialMessageType::Launched>;
using WebDialMessageOpened = WebDialSimpleMessage<WebDialMessageType::Opened>;

class WEBCORE_EXPORT WebDialMessageDisconnect : public WebDialMessage {
public:
    static Ref<WebDialMessageDisconnect> create(const String& reason) { return *adoptRef(new WebDialMessageDisconnect(reason)); }
    ~WebDialMessageDisconnect() = default;

    Vector<uint8_t> serialize() final;

    WebDialMessageType type() const final { return WebDialMessageType::Disconnect; }
    const String& reason() const { return m_reason; }

private:
    WebDialMessageDisconnect(const String& reason) : m_reason(reason) {}

    String m_reason;
};

class WEBCORE_EXPORT WebDialMessageMessage : public WebDialMessage {
public:
    virtual ~WebDialMessageMessage() = default;

    Vector<uint8_t> serialize() override;

    WebDialMessageType type() const final { return WebDialMessageType::Message; }
    virtual WebDialDataType dataType() const = 0;
};

class WEBCORE_EXPORT WebDialMessageTextMessage : public WebDialMessageMessage {
public:
    static Ref<WebDialMessageTextMessage> create(const String& data) { return *adoptRef(new WebDialMessageTextMessage(data)); }
    ~WebDialMessageTextMessage() = default;

    Vector<uint8_t> serialize() final;

    WebDialDataType dataType() const final { return WebDialDataType::Text; }
    const String& data() const { return m_data; }

private:
    WebDialMessageTextMessage(const String& data) : m_data(data) {}

    String m_data;
};

class WEBCORE_EXPORT WebDialMessageBinaryMessage : public WebDialMessageMessage {
public:
    virtual ~WebDialMessageBinaryMessage() = default;

    WebDialDataType dataType() const final { return WebDialDataType::Binary; }
    virtual WebDialBinaryType binaryType() const = 0;
};

class WEBCORE_EXPORT WebDialMessageArrayBufferMessage : public WebDialMessageBinaryMessage {
public:
    static Ref<WebDialMessageArrayBufferMessage> create(Ref<ArrayBuffer>&& data) { return *adoptRef(new WebDialMessageArrayBufferMessage(WTFMove(data))); }
    ~WebDialMessageArrayBufferMessage() = default;

    Vector<uint8_t> serialize() final;

    WebDialBinaryType binaryType() const final { return WebDialBinaryType::ArrayBuffer; }
    const Ref<ArrayBuffer>& data() const { return m_data; }

private:
    WebDialMessageArrayBufferMessage(Ref<ArrayBuffer>&& data) : m_data(WTFMove(data)) {}
    
    Ref<ArrayBuffer> m_data;
};

class WEBCORE_EXPORT WebDialMessageBlobMessage : public WebDialMessageBinaryMessage {
public:
    static Ref<WebDialMessageBlobMessage> create(Ref<Blob>&& data) { return *adoptRef(new WebDialMessageBlobMessage(WTFMove(data))); }
    ~WebDialMessageBlobMessage() = default;

    Vector<uint8_t> serialize() final;

    WebDialBinaryType binaryType() const final { return WebDialBinaryType::Blob; }
    const Ref<Blob>& data() const { return m_data; }

private:
    WebDialMessageBlobMessage(Ref<Blob>&& data) : m_data(WTFMove(data)) {}
    
    Ref<Blob> m_data;
};

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
#endif // WebDialMessages_h
