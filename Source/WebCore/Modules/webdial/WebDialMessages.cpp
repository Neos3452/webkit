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
#include "WebDialMessages.h"

#include "Logging.h"
#include "NotImplemented.h"
#include <arpa/inet.h>
#include <type_traits>

namespace {

template<class T>
T readPOD(const uint8_t*& buffer, size_t& size, typename std::enable_if<std::is_pod<T>::value, int>::type = 0)
{
    ASSERT(size >= sizeof(T));
    size -= sizeof(T);
    auto oldBuffer = buffer;
    buffer += sizeof(T);
    return *reinterpret_cast<const T*>(oldBuffer);
}

}

namespace WebCore {

RefPtr<WebDialMessage> WebDialMessage::deserialize(const uint8_t* buffer, size_t size)
{
    if (size < 8) {
        LOG(WebDial, "WebDialMessage::deserialize() buffer is too small");
        return nullptr;
    }

    const uint32_t version = ntohl(readPOD<decltype(version)>(buffer, size));
    if (version != kWebDialMessageDraftVersion) {
        LOG(WebDial, "WebDialMessage::deserialize() incorrect version %u", version);
        return nullptr;
    }

    RefPtr<WebDialMessage> deserializedMessage;
    
    const WebDialMessageType type = static_cast<WebDialMessageType>(ntohl(readPOD<uint32_t>(buffer, size)));
    switch(type) {
        case WebDialMessageType::Error:
        {
            String reason;
            if (size >= 4) {
                const uint32_t bodySize = ntohl(readPOD<decltype(bodySize)>(buffer, size));
                if (size < bodySize) {
                    LOG(WebDial, "WebDialMessage::deserialize() Not enough data");
                    break;
                }
                reason = String(buffer, bodySize);
                buffer += bodySize;
                size -= bodySize;
            }
            deserializedMessage = WebDialMessageError::create(reason);
            break;
        }
        case WebDialMessageType::Request:
        {
            if (size < 4) {
                LOG(WebDial, "WebDialMessage::deserialize() Not enough data");
                break;
            }
            const uint32_t bodySize = ntohl(readPOD<decltype(bodySize)>(buffer, size));
            if (size < bodySize) {
                LOG(WebDial, "WebDialMessage::deserialize() Not enough data");
                break;
            }
            String url(buffer, bodySize);
            buffer += bodySize;
            size -= bodySize;
            deserializedMessage = WebDialMessageRequest::create(url);
            break;
        }
        case WebDialMessageType::Launched:
        {
            deserializedMessage = WebDialMessageLaunched::create();
            break;
        }
        case WebDialMessageType::Opened:
        {
            deserializedMessage = WebDialMessageOpened::create();
            break;
        }
        case WebDialMessageType::Disconnect:
        {
            String reason;
            if (size >= 4) {
                const uint32_t bodySize = ntohl(readPOD<decltype(bodySize)>(buffer, size));
                if (size < bodySize) {
                    LOG(WebDial, "WebDialMessage::deserialize() Not enough data");
                    break;
                }
                reason = String(buffer, bodySize);
                buffer += bodySize;
                size -= bodySize;
            }
            deserializedMessage = WebDialMessageDisconnect::create(reason);
            break;
        }
        case WebDialMessageType::Message:
        {
            if (size < 5) {
                LOG(WebDial, "WebDialMessage::deserialize() Not enough data");
                break;
            }
            const uint8_t messageType = ntohl(readPOD<uint8_t>(buffer, size));
            if (messageType != 0) {
                // TODO support
                LOG(WebDial, "WebDialMessage::deserialize() Binary type not supported");
                notImplemented();
                break;
            }
            const uint32_t bodySize = ntohl(readPOD<decltype(bodySize)>(buffer, size));
            if (size < bodySize) {
                LOG(WebDial, "WebDialMessage::deserialize() Not enough data");
                break;
            }
            String data(buffer, bodySize);
            buffer += bodySize;
            size -= bodySize;
            deserializedMessage = WebDialMessageTextMessage::create(data);
            break;
        }
    }
    if (size > 0) {
        LOG(WebDial, "WebDialMessage::deserialize() there is some data left");
    }
    return deserializedMessage;
}

Vector<uint8_t> WebDialMessage::serialize()
{
    Vector<uint8_t> result;
    const uint32_t vers = htonl(version());
    const uint32_t typ = htonl(static_cast<uint32_t>(type()));
    result.reserveCapacity(sizeof(vers) + sizeof(typ));
    result.append(reinterpret_cast<const uint8_t*>(&vers), sizeof(vers));
    result.append(reinterpret_cast<const uint8_t*>(&typ), sizeof(typ));
    return result;
}

Vector<uint8_t> WebDialMessageError::serialize()
{
    Vector<uint8_t> result = WebDialMessage::serialize();
    if (!m_errorMessage.isEmpty()) {
        CString urlInUtf8 = m_errorMessage.utf8();
        result.reserveCapacity(result.capacity() + sizeof(uint32_t) + urlInUtf8.length());
        const uint32_t strSize = htonl(urlInUtf8.length());
        result.append(reinterpret_cast<const uint8_t*>(&strSize), sizeof(strSize));
        result.append(urlInUtf8.data(), urlInUtf8.length());
    }
    return result;
}

Vector<uint8_t> WebDialMessageRequest::serialize()
{
    Vector<uint8_t> result = WebDialMessage::serialize();
    CString urlInUtf8 = m_url.utf8();
    result.reserveCapacity(result.capacity() + sizeof(uint32_t) + urlInUtf8.length());
    const uint32_t strSize = htonl(urlInUtf8.length());
    result.append(reinterpret_cast<const uint8_t*>(&strSize), sizeof(strSize));
    result.append(urlInUtf8.data(), urlInUtf8.length());
    return result;
}

Vector<uint8_t> WebDialMessageDisconnect::serialize()
{
    Vector<uint8_t> result = WebDialMessage::serialize();
    if (!m_reason.isEmpty()) {
        CString urlInUtf8 = m_reason.utf8();
        result.reserveCapacity(result.capacity() + sizeof(uint32_t) + urlInUtf8.length());
        const uint32_t strSize = htonl(urlInUtf8.length());
        result.append(reinterpret_cast<const uint8_t*>(&strSize), sizeof(strSize));
        result.append(urlInUtf8.data(), urlInUtf8.length());
    }
    return result;
}

Vector<uint8_t> WebDialMessageMessage::serialize()
{
    Vector<uint8_t> result = WebDialMessage::serialize();
    static_assert(sizeof(std::underlying_type<decltype(dataType())>::type) == 1, "Need hton when using more than one byte");
    const uint8_t dType = static_cast<uint8_t>(dataType());
    result.append(reinterpret_cast<const uint8_t*>(&dType), sizeof(dType));
    return result;
}

Vector<uint8_t> WebDialMessageTextMessage::serialize()
{
    Vector<uint8_t> result = WebDialMessageMessage::serialize();
    CString urlInUtf8 = m_data.utf8();
    result.reserveCapacity(result.capacity() + sizeof(uint32_t) + urlInUtf8.length());
    const uint32_t strSize = htonl(urlInUtf8.length());
    result.append(reinterpret_cast<const uint8_t*>(&strSize), sizeof(strSize));    result.append(urlInUtf8.data(), urlInUtf8.length());
    return result;
}

Vector<uint8_t> WebDialMessageArrayBufferMessage::serialize()
{
    Vector<uint8_t> result = WebDialMessage::serialize();
    ASSERT_NOT_REACHED();
    return result;
}

Vector<uint8_t> WebDialMessageBlobMessage::serialize()
{
    Vector<uint8_t> result = WebDialMessage::serialize();
    ASSERT_NOT_REACHED();
    return result;
}

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
