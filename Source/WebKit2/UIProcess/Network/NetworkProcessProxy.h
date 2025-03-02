/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NetworkProcessProxy_h
#define NetworkProcessProxy_h

#include "ChildProcessProxy.h"
#include "LegacyCustomProtocolManagerProxy.h"
#include "ProcessLauncher.h"
#include "ProcessThrottler.h"
#include "ProcessThrottlerClient.h"
#include "WebProcessProxyMessages.h"
#include <memory>
#include <wtf/Deque.h>

namespace WebCore {
class AuthenticationChallenge;
class ProtectionSpace;
class ResourceRequest;
enum class ShouldSample;
class SecurityOrigin;
struct SecurityOriginData;
class SessionID;
}

namespace WebKit {

class DownloadProxy;
class DownloadProxyMap;
class WebProcessPool;
enum class WebsiteDataFetchOption;
enum class WebsiteDataType;
struct NetworkProcessCreationParameters;
struct WebsiteData;

class NetworkProcessProxy : public ChildProcessProxy, private ProcessThrottlerClient {
public:
    static Ref<NetworkProcessProxy> create(WebProcessPool&);
    ~NetworkProcessProxy();

    void getNetworkProcessConnection(Ref<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>&&);

    DownloadProxy* createDownloadProxy(const WebCore::ResourceRequest&);

    void fetchWebsiteData(WebCore::SessionID, OptionSet<WebsiteDataType>, OptionSet<WebsiteDataFetchOption>, WTF::Function<void(WebsiteData)>&& completionHandler);
    void deleteWebsiteData(WebCore::SessionID, OptionSet<WebsiteDataType>, std::chrono::system_clock::time_point modifiedSince, WTF::Function<void()>&& completionHandler);
    void deleteWebsiteDataForOrigins(WebCore::SessionID, OptionSet<WebKit::WebsiteDataType>, const Vector<WebCore::SecurityOriginData>& origins, const Vector<String>& cookieHostNames, WTF::Function<void()>&& completionHandler);

#if PLATFORM(COCOA)
    void setProcessSuppressionEnabled(bool);
#endif

    void processReadyToSuspend();

    void setIsHoldingLockedFiles(bool);

    ProcessThrottler& throttler() { return m_throttler; }

private:
    NetworkProcessProxy(WebProcessPool&);

    // ChildProcessProxy
    void getLaunchOptions(ProcessLauncher::LaunchOptions&) override;
    void connectionWillOpen(IPC::Connection&) override;
    void processWillShutDown(IPC::Connection&) override;

    void networkProcessFailedToLaunch();
    void networkProcessCrashed();
    void clearCallbackStates();

    // ProcessThrottlerClient
    void sendProcessWillSuspendImminently() override;
    void sendPrepareToSuspend() override;
    void sendCancelPrepareToSuspend() override;
    void sendProcessDidResume() override;
    void didSetAssertionState(AssertionState) override;

    // IPC::Connection::Client
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;
    void didClose(IPC::Connection&) override;
    void didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference messageReceiverName, IPC::StringReference messageName) override;

    // Message handlers
    void didReceiveNetworkProcessProxyMessage(IPC::Connection&, IPC::Decoder&);
    void didCreateNetworkConnectionToWebProcess(const IPC::Attachment&);
    void didReceiveAuthenticationChallenge(uint64_t pageID, uint64_t frameID, const WebCore::AuthenticationChallenge&, uint64_t challengeID);
    void didFetchWebsiteData(uint64_t callbackID, const WebsiteData&);
    void didDeleteWebsiteData(uint64_t callbackID);
    void didDeleteWebsiteDataForOrigins(uint64_t callbackID);
    void grantSandboxExtensionsToDatabaseProcessForBlobs(uint64_t requestID, const Vector<String>& paths);
#if HAVE(CFNETWORK_STORAGE_PARTITIONING)
    void shouldPartitionCookiesForTopPrivatelyOwnedDomains(const Vector<String>& domainsToRemove, const Vector<String>& domainsToAdd, bool clearFirst);
#endif
    void logDiagnosticMessage(uint64_t pageID, const String& message, const String& description, WebCore::ShouldSample);
    void logDiagnosticMessageWithResult(uint64_t pageID, const String& message, const String& description, uint32_t result, WebCore::ShouldSample);
    void logDiagnosticMessageWithValue(uint64_t pageID, const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample);
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    void canAuthenticateAgainstProtectionSpace(uint64_t loaderID, uint64_t pageID, uint64_t frameID, const WebCore::ProtectionSpace&);
#endif

    // ProcessLauncher::Client
    void didFinishLaunching(ProcessLauncher*, IPC::Connection::Identifier) override;

    WebProcessPool& m_processPool;
    
    unsigned m_numPendingConnectionRequests;
    Deque<Ref<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>> m_pendingConnectionReplies;

    HashMap<uint64_t, WTF::Function<void (WebsiteData)>> m_pendingFetchWebsiteDataCallbacks;
    HashMap<uint64_t, WTF::Function<void ()>> m_pendingDeleteWebsiteDataCallbacks;
    HashMap<uint64_t, WTF::Function<void ()>> m_pendingDeleteWebsiteDataForOriginsCallbacks;

    std::unique_ptr<DownloadProxyMap> m_downloadProxyMap;
    LegacyCustomProtocolManagerProxy m_customProtocolManagerProxy;
    ProcessThrottler m_throttler;
    ProcessThrottler::BackgroundActivityToken m_tokenForHoldingLockedFiles;
};

} // namespace WebKit

#endif // NetworkProcessProxy_h
