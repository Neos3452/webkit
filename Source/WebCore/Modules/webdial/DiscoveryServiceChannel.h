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

#ifndef DiscoveryServiceChannel_h
#define DiscoveryServiceChannel_h

#if ENABLE(WEB_DIAL)

#include <functional>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>
#include <wtf/Vector.h>

namespace WebCore {

class DiscoveryServiceChannel : public RefCounted<DiscoveryServiceChannel> {
public:
    using MessageCallback = std::function<void(const uint8_t* data, size_t size)>;
    using ErrorCallback = std::function<void(const String&)>;
    using ClosedCallback = std::function<void()>;

    virtual ~DiscoveryServiceChannel() = default;

    WEBCORE_EXPORT virtual void open() = 0;
    WEBCORE_EXPORT virtual void reopen() = 0;
    WEBCORE_EXPORT virtual size_t send(const uint8_t* data, size_t size) = 0;
    WEBCORE_EXPORT virtual void close() = 0;

    WEBCORE_EXPORT void setMessageCallback(MessageCallback&& cb);
    WEBCORE_EXPORT void setErrorCallback(ErrorCallback&& cb);
    WEBCORE_EXPORT void setClosedCallback(ClosedCallback&& cb);

protected:
    MessageCallback m_messageCallback;
    ErrorCallback m_errorCallback;
    ClosedCallback m_closedCallback;
};

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
#endif // DiscoveryServiceChannel_h
