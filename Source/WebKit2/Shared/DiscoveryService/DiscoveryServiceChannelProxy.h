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

#ifndef DiscoveryServiceChannelProxy_h
#define DiscoveryServiceChannelProxy_h

#if ENABLE(WEB_DIAL)
#include <WebCore/DiscoveryServiceChannel.h>
#include <wtf/RefCounted.h>

namespace WebKit {

class WebPage;

class DiscoveryServiceChannelProxy : public WebCore::DiscoveryServiceChannel {
public:
    static Ref<DiscoveryServiceChannelProxy> create(WebPage&);

    void open() override;
    void reopen() override;
    size_t send(const uint8_t* data, size_t size) override;
    void close() override;

    void messageReceived(const Vector<uint8_t>&);
    void errorReceived(const String& reason);
    void closedReceived();

private:
    DiscoveryServiceChannelProxy(WebPage&);

    WebPage& m_page;
};

} // namespace WebKit

#endif // ENABLE(WEB_DIAL)
#endif // DiscoveryServiceChannelProxy_h
