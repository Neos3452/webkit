/*
 * Copyright (C) 2012-2016 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NetworkProcessProxy.h"

#include "AuthenticationChallengeProxy.h"
#include "DatabaseProcessMessages.h"
#include "DownloadProxyMessages.h"
#include "LegacyCustomProtocolManagerProxyMessages.h"
#include "Logging.h"
#include "NetworkProcessCreationParameters.h"
#include "NetworkProcessMessages.h"
#include "SandboxExtension.h"
#include "WebPageProxy.h"
#include "WebProcessMessages.h"
#include "WebProcessPool.h"
#include "WebsiteData.h"

#if ENABLE(SEC_ITEM_SHIM)
#include "SecItemShimProxy.h"
#endif

#if PLATFORM(IOS)
#include <wtf/spi/darwin/XPCSPI.h>
#endif

#define MESSAGE_CHECK(assertion) MESSAGE_CHECK_BASE(assertion, connection())

using namespace WebCore;

namespace WebKit {

static uint64_t generateCallbackID()
{
    static uint64_t callbackID;

    return ++callbackID;
}

Ref<NetworkProcessProxy> NetworkProcessProxy::create(WebProcessPool& processPool)
{
    return adoptRef(*new NetworkProcessProxy(processPool));
}

NetworkProcessProxy::NetworkProcessProxy(WebProcessPool& processPool)
    : ChildProcessProxy(processPool.alwaysRunsAtBackgroundPriority())
    , m_processPool(processPool)
    , m_numPendingConnectionRequests(0)
    , m_customProtocolManagerProxy(this, processPool)
    , m_throttler(*this, processPool.shouldTakeUIBackgroundAssertion())
{
    connect();
}

NetworkProcessProxy::~NetworkProcessProxy()
{
    ASSERT(m_pendingFetchWebsiteDataCallbacks.isEmpty());
    ASSERT(m_pendingDeleteWebsiteDataCallbacks.isEmpty());
    ASSERT(m_pendingDeleteWebsiteDataForOriginsCallbacks.isEmpty());
}

void NetworkProcessProxy::getLaunchOptions(ProcessLauncher::LaunchOptions& launchOptions)
{
    launchOptions.processType = ProcessLauncher::ProcessType::Network;
    ChildProcessProxy::getLaunchOptions(launchOptions);
}

void NetworkProcessProxy::connectionWillOpen(IPC::Connection& connection)
{
#if ENABLE(SEC_ITEM_SHIM)
    SecItemShimProxy::singleton().initializeConnection(connection);
#else
    UNUSED_PARAM(connection);
#endif
}

void NetworkProcessProxy::processWillShutDown(IPC::Connection& connection)
{
    ASSERT_UNUSED(connection, this->connection() == &connection);
}

void NetworkProcessProxy::getNetworkProcessConnection(Ref<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>&& reply)
{
    m_pendingConnectionReplies.append(WTFMove(reply));

    if (state() == State::Launching) {
        m_numPendingConnectionRequests++;
        return;
    }

    connection()->send(Messages::NetworkProcess::CreateNetworkConnectionToWebProcess(), 0, IPC::SendOption::DispatchMessageEvenWhenWaitingForSyncReply);
}

DownloadProxy* NetworkProcessProxy::createDownloadProxy(const ResourceRequest& resourceRequest)
{
    if (!m_downloadProxyMap)
        m_downloadProxyMap = std::make_unique<DownloadProxyMap>(this);

    return m_downloadProxyMap->createDownloadProxy(m_processPool, resourceRequest);
}

void NetworkProcessProxy::fetchWebsiteData(SessionID sessionID, OptionSet<WebsiteDataType> dataTypes, OptionSet<WebsiteDataFetchOption> fetchOptions, WTF::Function<void (WebsiteData)>&& completionHandler)
{
    ASSERT(canSendMessage());

    uint64_t callbackID = generateCallbackID();
    RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is taking a background assertion because the Network process is fetching Website data", this);

    m_pendingFetchWebsiteDataCallbacks.add(callbackID, [this, token = throttler().backgroundActivityToken(), completionHandler = WTFMove(completionHandler), sessionID](WebsiteData websiteData) {
        completionHandler(WTFMove(websiteData));
        RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is releasing a background assertion because the Network process is done fetching Website data", this);
    });

    send(Messages::NetworkProcess::FetchWebsiteData(sessionID, dataTypes, fetchOptions, callbackID), 0);
}

void NetworkProcessProxy::deleteWebsiteData(WebCore::SessionID sessionID, OptionSet<WebsiteDataType> dataTypes, std::chrono::system_clock::time_point modifiedSince, WTF::Function<void ()>&& completionHandler)
{
    auto callbackID = generateCallbackID();
    RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is taking a background assertion because the Network process is deleting Website data", this);

    m_pendingDeleteWebsiteDataCallbacks.add(callbackID, [this, token = throttler().backgroundActivityToken(), completionHandler = WTFMove(completionHandler), sessionID] {
        completionHandler();
        RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is releasing a background assertion because the Network process is done deleting Website data", this);
    });
    send(Messages::NetworkProcess::DeleteWebsiteData(sessionID, dataTypes, modifiedSince, callbackID), 0);
}

void NetworkProcessProxy::deleteWebsiteDataForOrigins(SessionID sessionID, OptionSet<WebsiteDataType> dataTypes, const Vector<WebCore::SecurityOriginData>& origins, const Vector<String>& cookieHostNames, WTF::Function<void()>&& completionHandler)
{
    ASSERT(canSendMessage());

    uint64_t callbackID = generateCallbackID();
    RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is taking a background assertion because the Network process is deleting Website data for several origins", this);

    m_pendingDeleteWebsiteDataForOriginsCallbacks.add(callbackID, [this, token = throttler().backgroundActivityToken(), completionHandler = WTFMove(completionHandler), sessionID] {
        completionHandler();
        RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is releasing a background assertion because the Network process is done deleting Website data for several origins", this);
    });

    send(Messages::NetworkProcess::DeleteWebsiteDataForOrigins(sessionID, dataTypes, origins, cookieHostNames, callbackID), 0);
}

void NetworkProcessProxy::networkProcessCrashed()
{
    clearCallbackStates();

    Vector<Ref<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>> pendingReplies;
    pendingReplies.reserveInitialCapacity(m_pendingConnectionReplies.size());
    for (auto& reply : m_pendingConnectionReplies)
        pendingReplies.append(WTFMove(reply));

    // Tell the network process manager to forget about this network process proxy. This may cause us to be deleted.
    m_processPool.networkProcessCrashed(*this, WTFMove(pendingReplies));
}

void NetworkProcessProxy::networkProcessFailedToLaunch()
{
    // The network process must have crashed or exited, send any pending sync replies we might have.
    while (!m_pendingConnectionReplies.isEmpty()) {
        Ref<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply> reply = m_pendingConnectionReplies.takeFirst();

#if USE(UNIX_DOMAIN_SOCKETS)
        reply->send(IPC::Attachment());
#elif OS(DARWIN)
        reply->send(IPC::Attachment(0, MACH_MSG_TYPE_MOVE_SEND));
#else
        notImplemented();
#endif
    }
    clearCallbackStates();
    // Tell the network process manager to forget about this network process proxy. This may cause us to be deleted.
    m_processPool.networkProcessFailedToLaunch(*this);
}

void NetworkProcessProxy::clearCallbackStates()
{
    for (const auto& callback : m_pendingFetchWebsiteDataCallbacks.values())
        callback(WebsiteData());
    m_pendingFetchWebsiteDataCallbacks.clear();

    for (const auto& callback : m_pendingDeleteWebsiteDataCallbacks.values())
        callback();
    m_pendingDeleteWebsiteDataCallbacks.clear();

    for (const auto& callback : m_pendingDeleteWebsiteDataForOriginsCallbacks.values())
        callback();
    m_pendingDeleteWebsiteDataForOriginsCallbacks.clear();
}

void NetworkProcessProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (dispatchMessage(connection, decoder))
        return;

    if (m_processPool.dispatchMessage(connection, decoder))
        return;

    didReceiveNetworkProcessProxyMessage(connection, decoder);
}

void NetworkProcessProxy::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (dispatchSyncMessage(connection, decoder, replyEncoder))
        return;

    ASSERT_NOT_REACHED();
}

void NetworkProcessProxy::didClose(IPC::Connection&)
{
    if (m_downloadProxyMap)
        m_downloadProxyMap->processDidClose();
    m_customProtocolManagerProxy.processDidClose();

    m_tokenForHoldingLockedFiles = nullptr;

    // This may cause us to be deleted.
    networkProcessCrashed();
}

void NetworkProcessProxy::didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference, IPC::StringReference)
{
}

void NetworkProcessProxy::didCreateNetworkConnectionToWebProcess(const IPC::Attachment& connectionIdentifier)
{
    ASSERT(!m_pendingConnectionReplies.isEmpty());

    // Grab the first pending connection reply.
    RefPtr<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply> reply = m_pendingConnectionReplies.takeFirst();

#if USE(UNIX_DOMAIN_SOCKETS)
    reply->send(connectionIdentifier);
#elif OS(DARWIN)
    reply->send(IPC::Attachment(connectionIdentifier.port(), MACH_MSG_TYPE_MOVE_SEND));
#else
    notImplemented();
#endif
}

void NetworkProcessProxy::didReceiveAuthenticationChallenge(uint64_t pageID, uint64_t frameID, const WebCore::AuthenticationChallenge& coreChallenge, uint64_t challengeID)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    MESSAGE_CHECK(page);

    auto authenticationChallenge = AuthenticationChallengeProxy::create(coreChallenge, challengeID, connection());
    page->didReceiveAuthenticationChallengeProxy(frameID, WTFMove(authenticationChallenge));
}

void NetworkProcessProxy::didFetchWebsiteData(uint64_t callbackID, const WebsiteData& websiteData)
{
    auto callback = m_pendingFetchWebsiteDataCallbacks.take(callbackID);
    callback(websiteData);
}

void NetworkProcessProxy::didDeleteWebsiteData(uint64_t callbackID)
{
    auto callback = m_pendingDeleteWebsiteDataCallbacks.take(callbackID);
    callback();
}

void NetworkProcessProxy::didDeleteWebsiteDataForOrigins(uint64_t callbackID)
{
    auto callback = m_pendingDeleteWebsiteDataForOriginsCallbacks.take(callbackID);
    callback();
}

void NetworkProcessProxy::grantSandboxExtensionsToDatabaseProcessForBlobs(uint64_t requestID, const Vector<String>& paths)
{
#if ENABLE(DATABASE_PROCESS)
#if ENABLE(SANDBOX_EXTENSIONS)
    SandboxExtension::HandleArray extensions;
    extensions.allocate(paths.size());
    for (size_t i = 0; i < paths.size(); ++i) {
        // ReadWrite is required for creating hard links as well as deleting the temporary file, which the DatabaseProcess will do.
        SandboxExtension::createHandle(paths[i], SandboxExtension::ReadWrite, extensions[i]);
    }

    m_processPool.sendToDatabaseProcessRelaunchingIfNecessary(Messages::DatabaseProcess::GrantSandboxExtensionsForBlobs(paths, extensions));
#endif
    connection()->send(Messages::NetworkProcess::DidGrantSandboxExtensionsToDatabaseProcessForBlobs(requestID), 0);
#endif
}

#if HAVE(CFNETWORK_STORAGE_PARTITIONING)
void NetworkProcessProxy::shouldPartitionCookiesForTopPrivatelyOwnedDomains(const Vector<String>& domainsToRemove, const Vector<String>& domainsToAdd, bool clearFirst)
{
    connection()->send(Messages::NetworkProcess::ShouldPartitionCookiesForTopPrivatelyOwnedDomains(domainsToRemove, domainsToAdd, clearFirst), 0);
}
#endif

void NetworkProcessProxy::didFinishLaunching(ProcessLauncher* launcher, IPC::Connection::Identifier connectionIdentifier)
{
    ChildProcessProxy::didFinishLaunching(launcher, connectionIdentifier);

    if (IPC::Connection::identifierIsNull(connectionIdentifier)) {
        networkProcessFailedToLaunch();
        return;
    }

    for (unsigned i = 0; i < m_numPendingConnectionRequests; ++i)
        connection()->send(Messages::NetworkProcess::CreateNetworkConnectionToWebProcess(), 0);
    
    m_numPendingConnectionRequests = 0;

#if PLATFORM(COCOA)
    if (m_processPool.processSuppressionEnabled())
        setProcessSuppressionEnabled(true);
#endif
    
#if PLATFORM(IOS)
    if (xpc_connection_t connection = this->connection()->xpcConnection())
        m_throttler.didConnectToProcess(xpc_connection_get_pid(connection));
#endif
}

void NetworkProcessProxy::logDiagnosticMessage(uint64_t pageID, const String& message, const String& description, WebCore::ShouldSample shouldSample)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    // FIXME: We do this null-check because by the time the decision to log is made, the page may be gone. We should refactor to avoid this,
    // but for now we simply drop the message in the rare case this happens.
    if (!page)
        return;

    page->logDiagnosticMessage(message, description, shouldSample);
}

void NetworkProcessProxy::logDiagnosticMessageWithResult(uint64_t pageID, const String& message, const String& description, uint32_t result, WebCore::ShouldSample shouldSample)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    // FIXME: We do this null-check because by the time the decision to log is made, the page may be gone. We should refactor to avoid this,
    // but for now we simply drop the message in the rare case this happens.
    if (!page)
        return;

    page->logDiagnosticMessageWithResult(message, description, result, shouldSample);
}

void NetworkProcessProxy::logDiagnosticMessageWithValue(uint64_t pageID, const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample shouldSample)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    // FIXME: We do this null-check because by the time the decision to log is made, the page may be gone. We should refactor to avoid this,
    // but for now we simply drop the message in the rare case this happens.
    if (!page)
        return;

    page->logDiagnosticMessageWithValue(message, description, value, significantFigures, shouldSample);
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
void NetworkProcessProxy::canAuthenticateAgainstProtectionSpace(uint64_t loaderID, uint64_t pageID, uint64_t frameID, const WebCore::ProtectionSpace& protectionSpace)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    if (!page)
        return;
    
    page->canAuthenticateAgainstProtectionSpace(loaderID, frameID, protectionSpace);
}
#endif

void NetworkProcessProxy::sendProcessWillSuspendImminently()
{
    if (!canSendMessage())
        return;

    bool handled = false;
    sendSync(Messages::NetworkProcess::ProcessWillSuspendImminently(), Messages::NetworkProcess::ProcessWillSuspendImminently::Reply(handled), 0, 1_s);
}
    
void NetworkProcessProxy::sendPrepareToSuspend()
{
    if (canSendMessage())
        send(Messages::NetworkProcess::PrepareToSuspend(), 0);
}

void NetworkProcessProxy::sendCancelPrepareToSuspend()
{
    if (canSendMessage())
        send(Messages::NetworkProcess::CancelPrepareToSuspend(), 0);
}

void NetworkProcessProxy::sendProcessDidResume()
{
    if (canSendMessage())
        send(Messages::NetworkProcess::ProcessDidResume(), 0);
}

void NetworkProcessProxy::processReadyToSuspend()
{
    m_throttler.processReadyToSuspend();
}

void NetworkProcessProxy::didSetAssertionState(AssertionState)
{
}
    
void NetworkProcessProxy::setIsHoldingLockedFiles(bool isHoldingLockedFiles)
{
    if (!isHoldingLockedFiles) {
        RELEASE_LOG(ProcessSuspension, "UIProcess is releasing a background assertion because the Network process is no longer holding locked files");
        m_tokenForHoldingLockedFiles = nullptr;
        return;
    }
    if (!m_tokenForHoldingLockedFiles) {
        RELEASE_LOG(ProcessSuspension, "UIProcess is taking a background assertion because the Network process is holding locked files");
        m_tokenForHoldingLockedFiles = m_throttler.backgroundActivityToken();
    }
}

} // namespace WebKit
