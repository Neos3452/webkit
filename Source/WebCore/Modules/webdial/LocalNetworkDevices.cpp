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
#include "LocalNetworkDevices.h"

#include "Document.h"
#include "Frame.h"
#include "Logging.h"
#include "LocalNetworkEnumerationRequest.h"
#include "NotImplemented.h"
#include "Page.h"

namespace WebCore {

Ref<LocalNetworkDevices> LocalNetworkDevices::create(ScriptExecutionContext* context)
{
    auto devices = adoptRef(*new LocalNetworkDevices(context));
    devices.get().suspendIfNeeded();
    return devices;
}

LocalNetworkDevices::LocalNetworkDevices(ScriptExecutionContext* context)
    : ActiveDOMObject(context)
{
}

ExceptionOr<void> LocalNetworkDevices::enumerateDevices(EnumeratePromise&& promise)
{
    LOG(WebDial, "LocalNetworkDevices::enumerateDevices()");
    return LocalNetworkEnumerationRequest::start(downcast<Document>(scriptExecutionContext()), WTFMove(promise));
}

void LocalNetworkDevices::LocalNetworkDevices::stop()
{
    // FIXME: stop permission request?
}

bool LocalNetworkDevices::canSuspendForDocumentSuspension() const
{
    return true;
}

void LocalNetworkDevices::suspend(ReasonForSuspension)
{

}

void LocalNetworkDevices::resume()
{

}

const char* LocalNetworkDevices::activeDOMObjectName() const
{
    return "LocalNetworkDevices";
}
    
} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
