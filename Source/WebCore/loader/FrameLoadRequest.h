/*
 * Copyright (C) 2003-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include "FrameLoaderTypes.h"
#include "ResourceRequest.h"
#include "SubstituteData.h"
#include <wtf/Forward.h>

namespace WebCore {

class Document;
class Frame;
class SecurityOrigin;

class FrameLoadRequest {
public:
    WEBCORE_EXPORT FrameLoadRequest(Document&, SecurityOrigin&, const ResourceRequest&, const String& frameName, LockHistory, LockBackForwardList, ShouldSendReferrer, AllowNavigationToInvalidURL, NewFrameOpenerPolicy, ShouldOpenExternalURLsPolicy, ShouldReplaceDocumentIfJavaScriptURL = ReplaceDocumentIfJavaScriptURL, const AtomicString& downloadAttribute = { });
    WEBCORE_EXPORT FrameLoadRequest(Frame&, const ResourceRequest&, ShouldOpenExternalURLsPolicy, const SubstituteData& = SubstituteData());

    WEBCORE_EXPORT ~FrameLoadRequest();

    WEBCORE_EXPORT FrameLoadRequest(FrameLoadRequest&&);
    WEBCORE_EXPORT FrameLoadRequest& operator=(FrameLoadRequest&&);

    bool isEmpty() const { return m_resourceRequest.isEmpty(); }

    Document& requester();
    const SecurityOrigin& requesterSecurityOrigin() const;

    ResourceRequest& resourceRequest() { return m_resourceRequest; }
    const ResourceRequest& resourceRequest() const { return m_resourceRequest; }

    const String& frameName() const { return m_frameName; }
    void setFrameName(const String& frameName) { m_frameName = frameName; }

    void setShouldCheckNewWindowPolicy(bool checkPolicy) { m_shouldCheckNewWindowPolicy = checkPolicy; }
    bool shouldCheckNewWindowPolicy() const { return m_shouldCheckNewWindowPolicy; }

    const SubstituteData& substituteData() const { return m_substituteData; }
    void setSubstituteData(const SubstituteData& data) { m_substituteData = data; }
    bool hasSubstituteData() { return m_substituteData.isValid(); }

    LockHistory lockHistory() const { return m_lockHistory; }
    LockBackForwardList lockBackForwardList() const { return m_lockBackForwardList; }
    ShouldSendReferrer shouldSendReferrer() const { return m_shouldSendReferrer; }
    AllowNavigationToInvalidURL allowNavigationToInvalidURL() const { return m_allowNavigationToInvalidURL; }
    NewFrameOpenerPolicy newFrameOpenerPolicy() const { return m_newFrameOpenerPolicy; }

    // The shouldReplaceDocumentIfJavaScriptURL parameter will go away when the FIXME to eliminate the
    // corresponding parameter from ScriptController::executeIfJavaScriptURL() is addressed.
    ShouldReplaceDocumentIfJavaScriptURL shouldReplaceDocumentIfJavaScriptURL() const { return m_shouldReplaceDocumentIfJavaScriptURL; }

    void setShouldOpenExternalURLsPolicy(ShouldOpenExternalURLsPolicy policy) { m_shouldOpenExternalURLsPolicy = policy; }
    ShouldOpenExternalURLsPolicy shouldOpenExternalURLsPolicy() const { return m_shouldOpenExternalURLsPolicy; }

    const AtomicString& downloadAttribute() const { return m_downloadAttribute; }

private:
    Ref<Document> m_requester;
    Ref<SecurityOrigin> m_requesterSecurityOrigin;
    ResourceRequest m_resourceRequest;
    String m_frameName;
    SubstituteData m_substituteData;

    bool m_shouldCheckNewWindowPolicy { false };
    LockHistory m_lockHistory;
    LockBackForwardList m_lockBackForwardList;
    ShouldSendReferrer m_shouldSendReferrer;
    AllowNavigationToInvalidURL m_allowNavigationToInvalidURL;
    NewFrameOpenerPolicy m_newFrameOpenerPolicy;
    ShouldReplaceDocumentIfJavaScriptURL m_shouldReplaceDocumentIfJavaScriptURL;
    ShouldOpenExternalURLsPolicy m_shouldOpenExternalURLsPolicy { ShouldOpenExternalURLsPolicy::ShouldNotAllow };
    AtomicString m_downloadAttribute;
};

} // namespace WebCore
