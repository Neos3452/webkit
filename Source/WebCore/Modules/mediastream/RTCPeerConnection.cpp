/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2015, 2016 Ericsson AB. All rights reserved.
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RTCPeerConnection.h"

#if ENABLE(WEB_RTC)

#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "Frame.h"
#include "Logging.h"
#include "MediaEndpointConfiguration.h"
#include "MediaStream.h"
#include "MediaStreamTrack.h"
#include "Page.h"
#include "RTCConfiguration.h"
#include "RTCController.h"
#include "RTCDataChannel.h"
#include "RTCIceCandidate.h"
#include "RTCPeerConnectionIceEvent.h"
#include "RTCSessionDescription.h"
#include "RTCTrackEvent.h"
#include <wtf/MainThread.h>
#include <wtf/UUID.h>
#include <wtf/text/Base64.h>

namespace WebCore {

using namespace PeerConnection;

Ref<RTCPeerConnection> RTCPeerConnection::create(ScriptExecutionContext& context)
{
    Ref<RTCPeerConnection> peerConnection = adoptRef(*new RTCPeerConnection(context));
    peerConnection->suspendIfNeeded();
    // RTCPeerConnection may send events at about any time during its lifetime.
    // Let's make it uncollectable until the pc is closed by JS or the page stops it.
    if (!peerConnection->isClosed()) {
        peerConnection->setPendingActivity(peerConnection.ptr());

        auto* page = downcast<Document>(context).page();
        peerConnection->registerToController(page->rtcController());
    }
    return peerConnection;
}

RTCPeerConnection::RTCPeerConnection(ScriptExecutionContext& context)
    : ActiveDOMObject(&context)
    , m_backend(PeerConnectionBackend::create(*this))
{
    if (!m_backend)
        m_connectionState = RTCPeerConnectionState::Closed;
}

RTCPeerConnection::~RTCPeerConnection()
{
    unregisterFromController();
    stop();
}

ExceptionOr<void> RTCPeerConnection::initializeWith(Document& document, RTCConfiguration&& configuration)
{
    if (!document.frame())
        return Exception { NOT_SUPPORTED_ERR };

    if (!m_backend)
        return Exception { NOT_SUPPORTED_ERR };

    return initializeConfiguration(WTFMove(configuration));
}

ExceptionOr<Ref<RTCRtpSender>> RTCPeerConnection::addTrack(Ref<MediaStreamTrack>&& track, const Vector<std::reference_wrapper<MediaStream>>& streams)
{
    if (isClosed())
        return Exception { INVALID_STATE_ERR };

    for (RTCRtpSender& sender : m_transceiverSet->senders()) {
        if (sender.trackId() == track->id())
            return Exception { INVALID_ACCESS_ERR };
    }

    Vector<String> mediaStreamIds;
    for (auto stream : streams)
        mediaStreamIds.append(stream.get().id());

    RTCRtpSender* sender = nullptr;

    // Reuse an existing sender with the same track kind if it has never been used to send before.
    for (auto& transceiver : m_transceiverSet->list()) {
        auto& existingSender = transceiver->sender();
        if (existingSender.trackKind() == track->kind() && existingSender.trackId().isNull() && !transceiver->hasSendingDirection()) {
            existingSender.setTrack(WTFMove(track));
            existingSender.setMediaStreamIds(WTFMove(mediaStreamIds));
            transceiver->enableSendingDirection();
            sender = &existingSender;
            
            break;
        }
    }

    if (!sender) {
        String transceiverMid = RTCRtpTransceiver::getNextMid();
        const String& trackKind = track->kind();
        String trackId = createCanonicalUUIDString();

        auto newSender = RTCRtpSender::create(WTFMove(track), WTFMove(mediaStreamIds), *this);
        auto receiver = m_backend->createReceiver(transceiverMid, trackKind, trackId);
        auto transceiver = RTCRtpTransceiver::create(WTFMove(newSender), WTFMove(receiver));

        // This transceiver is not yet associated with an m-line (null mid), but we need a
        // provisional mid if the transceiver is used to create an offer.
        transceiver->setProvisionalMid(transceiverMid);

        sender = &transceiver->sender();
        m_transceiverSet->append(WTFMove(transceiver));
    }

#if !USE(LIBWEBRTC)
    m_backend->markAsNeedingNegotiation();
#endif

    m_backend->notifyAddedTrack(*sender);
    return Ref<RTCRtpSender> { *sender };
}

ExceptionOr<void> RTCPeerConnection::removeTrack(RTCRtpSender& sender)
{
    if (isClosed())
        return Exception { INVALID_STATE_ERR };

    bool shouldAbort = true;
    for (RTCRtpSender& senderInSet : m_transceiverSet->senders()) {
        if (&senderInSet == &sender) {
            shouldAbort = sender.isStopped();
            break;
        }
    }
    if (shouldAbort)
        return { };

    sender.stop();

    m_backend->notifyRemovedTrack(sender);
#if !USE(LIBWEBRTC)
    m_backend->markAsNeedingNegotiation();
#endif
    return { };
}

ExceptionOr<Ref<RTCRtpTransceiver>> RTCPeerConnection::addTransceiver(AddTransceiverTrackOrKind&& withTrack, const RTCRtpTransceiverInit& init)
{
    if (WTF::holds_alternative<String>(withTrack)) {
        const String& kind = WTF::get<String>(withTrack);
        if (kind != "audio" && kind != "video")
            return Exception { TypeError };

        auto sender = RTCRtpSender::create(String(kind), Vector<String>(), *this);
        return completeAddTransceiver(WTFMove(sender), init, createCanonicalUUIDString(), kind);
    }

    Ref<MediaStreamTrack> track = WTF::get<RefPtr<MediaStreamTrack>>(withTrack).releaseNonNull();
    const String& trackId = track->id();
    const String& trackKind = track->kind();

    auto sender = RTCRtpSender::create(WTFMove(track), Vector<String>(), *this);
    return completeAddTransceiver(WTFMove(sender), init, trackId, trackKind);
}

Ref<RTCRtpTransceiver> RTCPeerConnection::completeAddTransceiver(Ref<RTCRtpSender>&& sender, const RTCRtpTransceiverInit& init, const String& trackId, const String& trackKind)
{
    String transceiverMid = RTCRtpTransceiver::getNextMid();
    auto transceiver = RTCRtpTransceiver::create(WTFMove(sender), m_backend->createReceiver(transceiverMid, trackKind, trackId));

    transceiver->setProvisionalMid(transceiverMid);
    transceiver->setDirection(init.direction);

    m_transceiverSet->append(transceiver.copyRef());
#if !USE(LIBWEBRTC)
    m_backend->markAsNeedingNegotiation();
#endif
    return transceiver;
}

void RTCPeerConnection::queuedCreateOffer(RTCOfferOptions&& options, SessionDescriptionPromise&& promise)
{
    RELEASE_LOG(WebRTC, "Creating offer\n");
    if (isClosed()) {
        promise.reject(INVALID_STATE_ERR);
        return;
    }

    m_backend->createOffer(WTFMove(options), WTFMove(promise));
}

void RTCPeerConnection::queuedCreateAnswer(RTCAnswerOptions&& options, SessionDescriptionPromise&& promise)
{
    RELEASE_LOG(WebRTC, "Creating answer\n");
    if (isClosed()) {
        promise.reject(INVALID_STATE_ERR);
        return;
    }

    m_backend->createAnswer(WTFMove(options), WTFMove(promise));
}

void RTCPeerConnection::queuedSetLocalDescription(RTCSessionDescription& description, DOMPromiseDeferred<void>&& promise)
{
    RELEASE_LOG(WebRTC, "Setting local description:\n%{public}s\n", description.sdp().utf8().data());
    if (isClosed()) {
        promise.reject(INVALID_STATE_ERR);
        return;
    }

    m_backend->setLocalDescription(description, WTFMove(promise));
}

RefPtr<RTCSessionDescription> RTCPeerConnection::localDescription() const
{
    return m_backend->localDescription();
}

RefPtr<RTCSessionDescription> RTCPeerConnection::currentLocalDescription() const
{
    return m_backend->currentLocalDescription();
}

RefPtr<RTCSessionDescription> RTCPeerConnection::pendingLocalDescription() const
{
    return m_backend->pendingLocalDescription();
}

void RTCPeerConnection::queuedSetRemoteDescription(RTCSessionDescription& description, DOMPromiseDeferred<void>&& promise)
{
    RELEASE_LOG(WebRTC, "Setting remote description:\n%{public}s\n", description.sdp().utf8().data());

    if (isClosed()) {
        promise.reject(INVALID_STATE_ERR);
        return;
    }
    m_backend->setRemoteDescription(description, WTFMove(promise));
}

RefPtr<RTCSessionDescription> RTCPeerConnection::remoteDescription() const
{
    return m_backend->remoteDescription();
}

RefPtr<RTCSessionDescription> RTCPeerConnection::currentRemoteDescription() const
{
    return m_backend->currentRemoteDescription();
}

RefPtr<RTCSessionDescription> RTCPeerConnection::pendingRemoteDescription() const
{
    return m_backend->pendingRemoteDescription();
}

void RTCPeerConnection::queuedAddIceCandidate(RTCIceCandidate* rtcCandidate, DOMPromiseDeferred<void>&& promise)
{
    RELEASE_LOG(WebRTC, "Received ice candidate:\n%{public}s\n", rtcCandidate ? rtcCandidate->candidate().utf8().data() : "null");

    if (isClosed()) {
        promise.reject(INVALID_STATE_ERR);
        return;
    }

    m_backend->addIceCandidate(rtcCandidate, WTFMove(promise));
}

static inline std::optional<Vector<MediaEndpointConfiguration::IceServerInfo>> iceServersFromConfiguration(RTCConfiguration& configuration)
{
    Vector<MediaEndpointConfiguration::IceServerInfo> servers;
    if (configuration.iceServers) {
        servers.reserveInitialCapacity(configuration.iceServers->size());
        for (auto& server : configuration.iceServers.value()) {
            Vector<URL> serverURLs;
            WTF::switchOn(server.urls, [&serverURLs] (const String& string) {
                serverURLs.reserveInitialCapacity(1);
                serverURLs.uncheckedAppend(URL { URL { }, string });
            }, [&serverURLs] (const Vector<String>& vector) {
                serverURLs.reserveInitialCapacity(vector.size());
                for (auto& string : vector)
                    serverURLs.uncheckedAppend(URL { URL { }, string });
            });
            for (auto& serverURL : serverURLs) {
                if (!(serverURL.protocolIs("turn") || serverURL.protocolIs("turns") || serverURL.protocolIs("stun")))
                    return std::nullopt;
            }
            servers.uncheckedAppend({ WTFMove(serverURLs), server.credential, server.username });
        }
    }
    return servers;
}

ExceptionOr<void> RTCPeerConnection::initializeConfiguration(RTCConfiguration&& configuration)
{
    auto servers = iceServersFromConfiguration(configuration);
    if (!servers)
        return Exception { INVALID_ACCESS_ERR };

    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=173938
    // Also decide whether to report an exception or output a message in the console log if setting configuration fails.
    m_backend->setConfiguration({ WTFMove(servers.value()), configuration.iceTransportPolicy, configuration.bundlePolicy, configuration.iceCandidatePoolSize });

    m_configuration = WTFMove(configuration);
    return { };
}

ExceptionOr<void> RTCPeerConnection::setConfiguration(RTCConfiguration&& configuration)
{
    if (isClosed())
        return Exception { INVALID_STATE_ERR };

    auto servers = iceServersFromConfiguration(configuration);
    if (!servers)
        return Exception { INVALID_ACCESS_ERR };

    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=173938
    // Also decide whether to report an exception or output a message in the console log if setting configuration fails.
    m_backend->setConfiguration({ WTFMove(servers.value()), configuration.iceTransportPolicy, configuration.bundlePolicy, configuration.iceCandidatePoolSize });
    m_configuration = WTFMove(configuration);
    return { };
}

void RTCPeerConnection::getStats(MediaStreamTrack* selector, Ref<DeferredPromise>&& promise)
{
    m_backend->getStats(selector, WTFMove(promise));
}

ExceptionOr<Ref<RTCDataChannel>> RTCPeerConnection::createDataChannel(ScriptExecutionContext& context, String&& label, RTCDataChannelInit&& options)
{
    if (isClosed())
        return Exception { INVALID_STATE_ERR };

    if (options.negotiated && !options.negotiated.value() && (label.length() > 65535 || options.protocol.length() > 65535))
        return Exception { TypeError };

    if (options.maxPacketLifeTime && options.maxRetransmits)
        return Exception { TypeError };

    if (options.id && options.id.value() > 65534)
        return Exception { TypeError };
    
    auto channelHandler = m_backend->createDataChannelHandler(label, options);
    if (!channelHandler)
        return Exception { NOT_SUPPORTED_ERR };

    return RTCDataChannel::create(context, WTFMove(channelHandler), WTFMove(label), WTFMove(options));
}

bool RTCPeerConnection::doClose()
{
    if (isClosed())
        return false;

    m_connectionState = RTCPeerConnectionState::Closed;
    m_iceConnectionState = RTCIceConnectionState::Closed;

    for (RTCRtpReceiver& receiver : m_transceiverSet->receivers())
        receiver.stop();

    for (RTCRtpSender& sender : m_transceiverSet->senders())
        sender.stop();

    return true;
}

void RTCPeerConnection::close()
{
    if (!doClose())
        return;

    updateConnectionState();
    scriptExecutionContext()->postTask([protectedThis = makeRef(*this)](ScriptExecutionContext&) {
        protectedThis->doStop();
    });
}

void RTCPeerConnection::emulatePlatformEvent(const String& action)
{
    m_backend->emulatePlatformEvent(action);
}

void RTCPeerConnection::stop()
{
    if (!doClose())
        return;

    doStop();
}

void RTCPeerConnection::doStop()
{
    if (m_isStopped)
        return;

    m_isStopped = true;

    m_backend->stop();

    unsetPendingActivity(this);
}

void RTCPeerConnection::registerToController(RTCController& controller)
{
    m_controller = &controller;
    m_controller->add(*this);
}

void RTCPeerConnection::unregisterFromController()
{
    if (m_controller)
        m_controller->remove(*this);
}

const char* RTCPeerConnection::activeDOMObjectName() const
{
    return "RTCPeerConnection";
}

bool RTCPeerConnection::canSuspendForDocumentSuspension() const
{
    return !hasPendingActivity();
}

bool RTCPeerConnection::hasPendingActivity() const
{
    return !m_isStopped;
}

void RTCPeerConnection::addTransceiver(Ref<RTCRtpTransceiver>&& transceiver)
{
    m_transceiverSet->append(WTFMove(transceiver));
}

void RTCPeerConnection::setSignalingState(RTCSignalingState newState)
{
    m_signalingState = newState;
}

#if !RELEASE_LOG_DISABLED
static inline const char* rtcIceGatheringStateToString(RTCIceGatheringState newState)
{
    switch (newState) {
    case RTCIceGatheringState::New:
        return "new";
    case RTCIceGatheringState::Gathering:
        return "gathering";
    case RTCIceGatheringState::Complete:
        return "complete";
    }
}
#endif

void RTCPeerConnection::updateIceGatheringState(RTCIceGatheringState newState)
{
    RELEASE_LOG(WebRTC, "New ICE gathering state: %{public}s\n", rtcIceGatheringStateToString(newState));

    scriptExecutionContext()->postTask([protectedThis = makeRef(*this), newState](ScriptExecutionContext&) {
        if (protectedThis->isClosed() || protectedThis->m_iceGatheringState == newState)
            return;

        protectedThis->m_iceGatheringState = newState;
        protectedThis->dispatchEvent(Event::create(eventNames().icegatheringstatechangeEvent, false, false));
        protectedThis->updateConnectionState();
    });
}

#if !RELEASE_LOG_DISABLED
static inline const char* rtcIceConnectionStateToString(RTCIceConnectionState newState)
{
    switch (newState) {
    case RTCIceConnectionState::New:
        return "new";
    case RTCIceConnectionState::Checking:
        return "checking";
    case RTCIceConnectionState::Connected:
        return "connected";
    case RTCIceConnectionState::Completed:
        return "completed";
    case RTCIceConnectionState::Failed:
        return "failed";
    case RTCIceConnectionState::Disconnected:
        return "disconnected";
    case RTCIceConnectionState::Closed:
        return "closed";
    }
}
#endif

void RTCPeerConnection::updateIceConnectionState(RTCIceConnectionState newState)
{
    RELEASE_LOG(WebRTC, "New ICE connection state: %{public}s\n", rtcIceConnectionStateToString(newState));

    scriptExecutionContext()->postTask([protectedThis = makeRef(*this), newState](ScriptExecutionContext&) {
        if (protectedThis->isClosed() || protectedThis->m_iceConnectionState == newState)
            return;

        protectedThis->m_iceConnectionState = newState;
        protectedThis->dispatchEvent(Event::create(eventNames().iceconnectionstatechangeEvent, false, false));
        protectedThis->updateConnectionState();
    });
}

void RTCPeerConnection::updateConnectionState()
{
    RTCPeerConnectionState state;

    // FIXME: In case m_iceGatheringState is RTCIceGatheringState::Gathering, and m_iceConnectionState is Closed, we should have the connection state be Closed.
    if (m_iceConnectionState == RTCIceConnectionState::New && m_iceGatheringState == RTCIceGatheringState::New)
        state = RTCPeerConnectionState::New;
    else if (m_iceConnectionState == RTCIceConnectionState::Checking || m_iceGatheringState == RTCIceGatheringState::Gathering)
        state = RTCPeerConnectionState::Connecting;
    else if ((m_iceConnectionState == RTCIceConnectionState::Completed || m_iceConnectionState == RTCIceConnectionState::Connected) && m_iceGatheringState == RTCIceGatheringState::Complete)
        state = RTCPeerConnectionState::Connected;
    else if (m_iceConnectionState == RTCIceConnectionState::Disconnected)
        state = RTCPeerConnectionState::Disconnected;
    else if (m_iceConnectionState == RTCIceConnectionState::Failed)
        state = RTCPeerConnectionState::Failed;
    else if (m_iceConnectionState == RTCIceConnectionState::Closed)
        state = RTCPeerConnectionState::Closed;
    else
        return;

    if (state == m_connectionState)
        return;

    m_connectionState = state;
    dispatchEvent(Event::create(eventNames().connectionstatechangeEvent, false, false));
}

void RTCPeerConnection::scheduleNegotiationNeededEvent()
{
    scriptExecutionContext()->postTask([protectedThis = makeRef(*this)](ScriptExecutionContext&) {
        if (protectedThis->isClosed())
            return;
        if (!protectedThis->m_backend->isNegotiationNeeded())
            return;
        protectedThis->m_backend->clearNegotiationNeededState();
        protectedThis->dispatchEvent(Event::create(eventNames().negotiationneededEvent, false, false));
    });
}

void RTCPeerConnection::fireEvent(Event& event)
{
    dispatchEvent(event);
}

void RTCPeerConnection::enqueueReplaceTrackTask(RTCRtpSender& sender, Ref<MediaStreamTrack>&& withTrack, DOMPromiseDeferred<void>&& promise)
{
    scriptExecutionContext()->postTask([protectedThis = makeRef(*this), protectedSender = makeRef(sender), promise = WTFMove(promise), withTrack = WTFMove(withTrack)](ScriptExecutionContext&) mutable {
        if (protectedThis->isClosed())
            return;
        protectedSender->setTrack(WTFMove(withTrack));
        protectedThis->m_backend->notifyAddedTrack(protectedSender.get());
        promise.resolve();
    });
}

void RTCPeerConnection::replaceTrack(RTCRtpSender& sender, RefPtr<MediaStreamTrack>&& withTrack, DOMPromiseDeferred<void>&& promise)
{
    if (!withTrack) {
        scriptExecutionContext()->postTask([protectedSender = makeRef(sender), promise = WTFMove(promise)](ScriptExecutionContext&) mutable {
            protectedSender->setTrackToNull();
            promise.resolve();
        });
        return;
    }
    
    if (!sender.track()) {
        enqueueReplaceTrackTask(sender, withTrack.releaseNonNull(), WTFMove(promise));
        return;
    }

    m_backend->replaceTrack(sender, withTrack.releaseNonNull(), WTFMove(promise));
}

RTCRtpParameters RTCPeerConnection::getParameters(RTCRtpSender& sender) const
{
    return m_backend->getParameters(sender);
}

} // namespace WebCore

#endif // ENABLE(WEB_RTC)
