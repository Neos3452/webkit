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

#ifndef DiscoveryServicePrivateCocoa_h
#define DiscoveryServicePrivateCocoa_h

#if ENABLE(WEB_DIAL)

#import <CoreFoundation/CoreFoundation.h>
#include "DiscoveryService.h"
#include "DiscoveryServicePrivate.h"
#include <memory>
#include <wtf/Noncopyable.h>
#include <wtf/RetainPtr.h>

OBJC_CLASS NSNetService;
OBJC_CLASS WKNetServiceDelegate;

namespace WebKit {

class DiscoveryServicePrivateCocoa : public DiscoveryServicePrivate {
    WTF_MAKE_NONCOPYABLE(DiscoveryServicePrivateCocoa);
public:
    DiscoveryServicePrivateCocoa(DiscoveryServicePrivateClient*);
    ~DiscoveryServicePrivateCocoa();

    void publishService(const String& serviceTypeName) override;
    void stopService() override;

private:
    static void serverAcceptCallBack(CFSocketRef socket, CFSocketCallBackType type, CFDataRef address, const void *data, void *info);

    DiscoveryServicePrivateClient* m_client;
    RetainPtr<NSNetService> m_service;
    RetainPtr<WKNetServiceDelegate> m_delegate;
    RetainPtr<CFSocketRef> m_ipv4socket;
    RetainPtr<CFRunLoopSourceRef> m_source4;
    RetainPtr<CFSocketRef> m_ipv6socket;
    RetainPtr<CFRunLoopSourceRef> m_source6;
};

std::unique_ptr<DiscoveryServicePrivate> DiscoveryServicePrivate::create(DiscoveryServicePrivateClient* client)
{
    return std::make_unique<DiscoveryServicePrivateCocoa>(client);
}

} // namespace WebKit

#endif // ENABLE(WEB_DIAL)
#endif // DiscoveryServicePrivateCocoa_h
