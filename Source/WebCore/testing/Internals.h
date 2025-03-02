/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
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

#pragma once

#include "CSSComputedStyleDeclaration.h"
#include "ContextDestructionObserver.h"
#include "ExceptionOr.h"
#include "JSDOMPromiseDeferred.h"
#include "OrientationNotifier.h"
#include "PageConsoleClient.h"
#include "RealtimeMediaSource.h"
#include <runtime/Float32Array.h>
#include <wtf/Optional.h>

#if ENABLE(MEDIA_SESSION)
#include "MediaSessionInterruptionProvider.h"
#endif

namespace WebCore {

class AudioContext;
class DOMRect;
class DOMRectList;
class DOMURL;
class DOMWindow;
class Document;
class Element;
class File;
class Frame;
class GCObservation;
class HTMLImageElement;
class HTMLInputElement;
class HTMLLinkElement;
class HTMLMediaElement;
class HTMLSelectElement;
class ImageData;
class InspectorStubFrontend;
class InternalSettings;
class MallocStatistics;
class MediaSession;
class MediaStream;
class MediaStreamTrack;
class MemoryInfo;
class MockCDMFactory;
class MockContentFilterSettings;
class MockPageOverlay;
class NodeList;
class Page;
class Range;
class RenderedDocumentMarker;
class RTCPeerConnection;
class SVGSVGElement;
class SerializedScriptValue;
class SourceBuffer;
class StyleSheet;
class TimeRanges;
class TypeConversions;
class WebGLRenderingContext;
class XMLHttpRequest;

class Internals final : public RefCounted<Internals>,  private ContextDestructionObserver
#if ENABLE(MEDIA_STREAM)
    , private RealtimeMediaSource::Observer
#endif
    {
public:
    static Ref<Internals> create(Document&);
    virtual ~Internals();

    static void resetToConsistentState(Page&);

    ExceptionOr<String> elementRenderTreeAsText(Element&);
    bool hasPausedImageAnimations(Element&);

    String address(Node&);
    bool nodeNeedsStyleRecalc(Node&);
    String styleChangeType(Node&);
    String description(JSC::JSValue);

    bool isPreloaded(const String& url);
    bool isLoadingFromMemoryCache(const String& url);
    String xhrResponseSource(XMLHttpRequest&);
    Vector<String> mediaResponseSources(HTMLMediaElement&);
    Vector<String> mediaResponseContentRanges(HTMLMediaElement&);
    bool isSharingStyleSheetContents(HTMLLinkElement&, HTMLLinkElement&);
    bool isStyleSheetLoadingSubresources(HTMLLinkElement&);
    enum class CachePolicy { UseProtocolCachePolicy, ReloadIgnoringCacheData, ReturnCacheDataElseLoad, ReturnCacheDataDontLoad };
    void setOverrideCachePolicy(CachePolicy);
    ExceptionOr<void> setCanShowModalDialogOverride(bool allow);
    enum class ResourceLoadPriority { ResourceLoadPriorityVeryLow, ResourceLoadPriorityLow, ResourceLoadPriorityMedium, ResourceLoadPriorityHigh, ResourceLoadPriorityVeryHigh };
    void setOverrideResourceLoadPriority(ResourceLoadPriority);
    void setStrictRawResourceValidationPolicyDisabled(bool);

    void clearMemoryCache();
    void pruneMemoryCacheToSize(unsigned size);
    void destroyDecodedDataForAllImages();
    unsigned memoryCacheSize() const;

    unsigned imageFrameIndex(HTMLImageElement&);
    void setImageFrameDecodingDuration(HTMLImageElement&, float duration);
    void resetImageAnimation(HTMLImageElement&);
    bool isImageAnimating(HTMLImageElement&);
    void setClearDecoderAfterAsyncFrameRequestForTesting(HTMLImageElement&, bool);
    unsigned imageDecodeCount(HTMLImageElement&);

    void setGridMaxTracksLimit(unsigned);

    void clearPageCache();
    unsigned pageCacheSize() const;

    void disableTileSizeUpdateDelay();

    void setSpeculativeTilingDelayDisabledForTesting(bool);

    Ref<CSSComputedStyleDeclaration> computedStyleIncludingVisitedInfo(Element&) const;

    Node* ensureUserAgentShadowRoot(Element& host);
    Node* shadowRoot(Element& host);
    ExceptionOr<String> shadowRootType(const Node&) const;
    String shadowPseudoId(Element&);
    void setShadowPseudoId(Element&, const String&);

    // CSS Deferred Parsing Testing
    unsigned deferredStyleRulesCount(StyleSheet&);
    unsigned deferredGroupRulesCount(StyleSheet&);
    unsigned deferredKeyframesRulesCount(StyleSheet&);

    // DOMTimers throttling testing.
    ExceptionOr<bool> isTimerThrottled(int timeoutId);
    bool isRequestAnimationFrameThrottled() const;
    double requestAnimationFrameInterval() const;
    bool scriptedAnimationsAreSuspended() const;
    bool areTimersThrottled() const;

    enum EventThrottlingBehavior { Responsive, Unresponsive };
    void setEventThrottlingBehaviorOverride(std::optional<EventThrottlingBehavior>);
    std::optional<EventThrottlingBehavior> eventThrottlingBehaviorOverride() const;

    // Spatial Navigation testing.
    ExceptionOr<unsigned> lastSpatialNavigationCandidateCount() const;

    // CSS Animation testing.
    unsigned numberOfActiveAnimations() const;
    ExceptionOr<bool> animationsAreSuspended() const;
    ExceptionOr<void> suspendAnimations() const;
    ExceptionOr<void> resumeAnimations() const;
    ExceptionOr<bool> pauseAnimationAtTimeOnElement(const String& animationName, double pauseTime, Element&);
    ExceptionOr<bool> pauseAnimationAtTimeOnPseudoElement(const String& animationName, double pauseTime, Element&, const String& pseudoId);
    double animationsInterval() const;

    // CSS Transition testing.
    ExceptionOr<bool> pauseTransitionAtTimeOnElement(const String& propertyName, double pauseTime, Element&);
    ExceptionOr<bool> pauseTransitionAtTimeOnPseudoElement(const String& property, double pauseTime, Element&, const String& pseudoId);

    Node* treeScopeRootNode(Node&);
    Node* parentTreeScope(Node&);

    String visiblePlaceholder(Element&);
    void selectColorInColorChooser(HTMLInputElement&, const String& colorValue);
    ExceptionOr<Vector<String>> formControlStateOfPreviousHistoryItem();
    ExceptionOr<void> setFormControlStateOfPreviousHistoryItem(const Vector<String>&);

    ExceptionOr<Ref<DOMRect>> absoluteCaretBounds();

    Ref<DOMRect> boundingBox(Element&);

    ExceptionOr<Ref<DOMRectList>> inspectorHighlightRects();
    ExceptionOr<String> inspectorHighlightObject();

    ExceptionOr<unsigned> markerCountForNode(Node&, const String&);
    ExceptionOr<RefPtr<Range>> markerRangeForNode(Node&, const String& markerType, unsigned index);
    ExceptionOr<String> markerDescriptionForNode(Node&, const String& markerType, unsigned index);
    ExceptionOr<String> dumpMarkerRects(const String& markerType);
    void addTextMatchMarker(const Range&, bool isActive);
    ExceptionOr<void> setMarkedTextMatchesAreHighlighted(bool);

    void invalidateFontCache();
    void setFontSmoothingEnabled(bool);

    ExceptionOr<void> setLowPowerModeEnabled(bool);

    ExceptionOr<void> setScrollViewPosition(int x, int y);

    ExceptionOr<Ref<DOMRect>> layoutViewportRect();
    ExceptionOr<Ref<DOMRect>> visualViewportRect();

    ExceptionOr<void> setViewBaseBackgroundColor(const String& colorValue);

    ExceptionOr<void> setPagination(const String& mode, int gap, int pageLength);
    ExceptionOr<void> setPaginationLineGridEnabled(bool);
    ExceptionOr<String> configurationForViewport(float devicePixelRatio, int deviceWidth, int deviceHeight, int availableWidth, int availableHeight);

    ExceptionOr<bool> wasLastChangeUserEdit(Element& textField);
    bool elementShouldAutoComplete(HTMLInputElement&);
    void setEditingValue(HTMLInputElement&, const String&);
    void setAutofilled(HTMLInputElement&, bool enabled);
    enum class AutoFillButtonType { AutoFillButtonTypeNone, AutoFillButtonTypeContacts, AutoFillButtonTypeCredentials };
    void setShowAutoFillButton(HTMLInputElement&, AutoFillButtonType);
    ExceptionOr<void> scrollElementToRect(Element&, int x, int y, int w, int h);

    ExceptionOr<String> autofillFieldName(Element&);

    ExceptionOr<void> paintControlTints();

    RefPtr<Range> rangeFromLocationAndLength(Element& scope, int rangeLocation, int rangeLength);
    unsigned locationFromRange(Element& scope, const Range&);
    unsigned lengthFromRange(Element& scope, const Range&);
    String rangeAsText(const Range&);
    Ref<Range> subrange(Range&, int rangeLocation, int rangeLength);
    ExceptionOr<RefPtr<Range>> rangeForDictionaryLookupAtLocation(int x, int y);
    RefPtr<Range> rangeOfStringNearLocation(const Range&, const String&, unsigned);

    ExceptionOr<void> setDelegatesScrolling(bool enabled);

    ExceptionOr<int> lastSpellCheckRequestSequence();
    ExceptionOr<int> lastSpellCheckProcessedSequence();

    Vector<String> userPreferredLanguages() const;
    void setUserPreferredLanguages(const Vector<String>&);

    Vector<String> userPreferredAudioCharacteristics() const;
    void setUserPreferredAudioCharacteristic(const String&);

    ExceptionOr<unsigned> wheelEventHandlerCount();
    ExceptionOr<unsigned> touchEventHandlerCount();

    ExceptionOr<Ref<DOMRectList>> touchEventRectsForEvent(const String&);
    ExceptionOr<Ref<DOMRectList>> passiveTouchEventListenerRects();

    ExceptionOr<RefPtr<NodeList>> nodesFromRect(Document&, int x, int y, unsigned topPadding, unsigned rightPadding, unsigned bottomPadding, unsigned leftPadding, bool ignoreClipping, bool allowShadowContent, bool allowChildFrameContent) const;

    String parserMetaData(JSC::JSValue = JSC::JSValue::JSUndefined);

    void updateEditorUINowIfScheduled();

    bool hasSpellingMarker(int from, int length);
    bool hasGrammarMarker(int from, int length);
    bool hasAutocorrectedMarker(int from, int length);
    void setContinuousSpellCheckingEnabled(bool);
    void setAutomaticQuoteSubstitutionEnabled(bool);
    void setAutomaticLinkDetectionEnabled(bool);
    void setAutomaticDashSubstitutionEnabled(bool);
    void setAutomaticTextReplacementEnabled(bool);
    void setAutomaticSpellingCorrectionEnabled(bool);

    void handleAcceptedCandidate(const String& candidate, unsigned location, unsigned length);

    bool isOverwriteModeEnabled();
    void toggleOverwriteModeEnabled();

    ExceptionOr<RefPtr<Range>> rangeOfString(const String&, RefPtr<Range>&&, const Vector<String>& findOptions);
    ExceptionOr<unsigned> countMatchesForText(const String&, const Vector<String>& findOptions, const String& markMatches);
    ExceptionOr<unsigned> countFindMatches(const String&, const Vector<String>& findOptions);

    unsigned numberOfScrollableAreas();

    ExceptionOr<bool> isPageBoxVisible(int pageNumber);

    static const char* internalsId;

    InternalSettings* settings() const;
    unsigned workerThreadCount() const;
    ExceptionOr<bool> areSVGAnimationsPaused() const;
    ExceptionOr<double> svgAnimationsInterval(SVGSVGElement&) const;

    ExceptionOr<void> setDeviceProximity(const String& eventType, double value, double min, double max);

    enum {
        // Values need to be kept in sync with Internals.idl.
        LAYER_TREE_INCLUDES_VISIBLE_RECTS = 1,
        LAYER_TREE_INCLUDES_TILE_CACHES = 2,
        LAYER_TREE_INCLUDES_REPAINT_RECTS = 4,
        LAYER_TREE_INCLUDES_PAINTING_PHASES = 8,
        LAYER_TREE_INCLUDES_CONTENT_LAYERS = 16,
        LAYER_TREE_INCLUDES_ACCELERATES_DRAWING = 32,
        LAYER_TREE_INCLUDES_BACKING_STORE_ATTACHED = 64,
    };
    ExceptionOr<String> layerTreeAsText(Document&, unsigned short flags) const;
    ExceptionOr<uint64_t> layerIDForElement(Element&);
    ExceptionOr<String> repaintRectsAsText() const;
    ExceptionOr<String> scrollingStateTreeAsText() const;
    ExceptionOr<String> mainThreadScrollingReasons() const;
    ExceptionOr<Ref<DOMRectList>> nonFastScrollableRects() const;

    ExceptionOr<void> setElementUsesDisplayListDrawing(Element&, bool usesDisplayListDrawing);
    ExceptionOr<void> setElementTracksDisplayListReplay(Element&, bool isTrackingReplay);

    enum {
        // Values need to be kept in sync with Internals.idl.
        DISPLAY_LIST_INCLUDES_PLATFORM_OPERATIONS = 1,
    };
    ExceptionOr<String> displayListForElement(Element&, unsigned short flags);
    ExceptionOr<String> replayDisplayListForElement(Element&, unsigned short flags);

    ExceptionOr<void> garbageCollectDocumentResources() const;

    void beginSimulatedMemoryPressure();
    void endSimulatedMemoryPressure();
    bool isUnderMemoryPressure();

    ExceptionOr<void> insertAuthorCSS(const String&) const;
    ExceptionOr<void> insertUserCSS(const String&) const;

    unsigned numberOfLiveNodes() const;
    unsigned numberOfLiveDocuments() const;
    unsigned referencingNodeCount(const Document&) const;

    RefPtr<DOMWindow> openDummyInspectorFrontend(const String& url);
    void closeDummyInspectorFrontend();
    ExceptionOr<void> setInspectorIsUnderTest(bool);

    String counterValue(Element&);

    int pageNumber(Element&, float pageWidth = 800, float pageHeight = 600);
    Vector<String> shortcutIconURLs() const;

    int numberOfPages(float pageWidthInPixels = 800, float pageHeightInPixels = 600);
    ExceptionOr<String> pageProperty(const String& propertyName, int pageNumber) const;
    ExceptionOr<String> pageSizeAndMarginsInPixels(int pageNumber, int width, int height, int marginTop, int marginRight, int marginBottom, int marginLeft) const;

    ExceptionOr<float> pageScaleFactor() const;

    ExceptionOr<void> setPageScaleFactor(float scaleFactor, int x, int y);
    ExceptionOr<void> setPageZoomFactor(float);
    ExceptionOr<void> setTextZoomFactor(float);

    ExceptionOr<void> setUseFixedLayout(bool);
    ExceptionOr<void> setFixedLayoutSize(int width, int height);
    ExceptionOr<void> setViewExposedRect(float left, float top, float width, float height);
    void setPrinting(int width, int height);

    void setHeaderHeight(float);
    void setFooterHeight(float);

    void setTopContentInset(float);

#if ENABLE(FULLSCREEN_API)
    void webkitWillEnterFullScreenForElement(Element&);
    void webkitDidEnterFullScreenForElement(Element&);
    void webkitWillExitFullScreenForElement(Element&);
    void webkitDidExitFullScreenForElement(Element&);
#endif

    WEBCORE_TESTSUPPORT_EXPORT void setApplicationCacheOriginQuota(unsigned long long);

    void registerURLSchemeAsBypassingContentSecurityPolicy(const String& scheme);
    void removeURLSchemeRegisteredAsBypassingContentSecurityPolicy(const String& scheme);

    void registerDefaultPortForProtocol(unsigned short port, const String& protocol);

    Ref<MallocStatistics> mallocStatistics() const;
    Ref<TypeConversions> typeConversions() const;
    Ref<MemoryInfo> memoryInfo() const;

    Vector<String> getReferencedFilePaths() const;

    ExceptionOr<void> startTrackingRepaints();
    ExceptionOr<void> stopTrackingRepaints();

    ExceptionOr<void> startTrackingLayerFlushes();
    ExceptionOr<unsigned> layerFlushCount();

    ExceptionOr<void> startTrackingStyleRecalcs();
    ExceptionOr<unsigned> styleRecalcCount();
    unsigned lastStyleUpdateSize() const;

    ExceptionOr<void> startTrackingCompositingUpdates();
    ExceptionOr<unsigned> compositingUpdateCount();

    ExceptionOr<void> updateLayoutIgnorePendingStylesheetsAndRunPostLayoutTasks(Node*);
    unsigned layoutCount() const;

    Ref<ArrayBuffer> serializeObject(const RefPtr<SerializedScriptValue>&) const;
    Ref<SerializedScriptValue> deserializeBuffer(ArrayBuffer&) const;

    bool isFromCurrentWorld(JSC::JSValue) const;

    void setUsesOverlayScrollbars(bool);
    void setUsesMockScrollAnimator(bool);

    ExceptionOr<String> getCurrentCursorInfo();

    String markerTextForListItem(Element&);

    String toolTipFromElement(Element&) const;

    void forceReload(bool endToEnd);
    void reloadExpiredOnly();

    void enableAutoSizeMode(bool enabled, int minimumWidth, int minimumHeight, int maximumWidth, int maximumHeight);

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    void initializeMockCDM();
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    Ref<MockCDMFactory> registerMockCDM();
#endif

#if ENABLE(SPEECH_SYNTHESIS)
    void enableMockSpeechSynthesizer();
#endif

#if ENABLE(MEDIA_STREAM)
    void setMockMediaCaptureDevicesEnabled(bool);
#endif

#if ENABLE(WEB_RTC)
#if USE(OPENWEBRTC)
    void enableMockMediaEndpoint();
#endif
    void emulateRTCPeerConnectionPlatformEvent(RTCPeerConnection&, const String& action);
    void useMockRTCPeerConnectionFactory(const String&);
    void setICECandidateFiltering(bool);
    void setEnumeratingAllNetworkInterfacesEnabled(bool);
    void stopPeerConnection(RTCPeerConnection&);
    void applyRotationForOutgoingVideoSources(RTCPeerConnection&);
#endif

    String getImageSourceURL(Element&);

#if ENABLE(VIDEO)
    void simulateAudioInterruption(HTMLMediaElement&);
    ExceptionOr<bool> mediaElementHasCharacteristic(HTMLMediaElement&, const String&);
#endif

    bool isSelectPopupVisible(HTMLSelectElement&);

    ExceptionOr<String> captionsStyleSheetOverride();
    ExceptionOr<void> setCaptionsStyleSheetOverride(const String&);
    ExceptionOr<void> setPrimaryAudioTrackLanguageOverride(const String&);
    ExceptionOr<void> setCaptionDisplayMode(const String&);

#if ENABLE(VIDEO)
    Ref<TimeRanges> createTimeRanges(Float32Array& startTimes, Float32Array& endTimes);
    double closestTimeToTimeRanges(double time, TimeRanges&);
#endif

    ExceptionOr<Ref<DOMRect>> selectionBounds();

    ExceptionOr<bool> isPluginUnavailabilityIndicatorObscured(Element&);
    bool isPluginSnapshotted(Element&);

#if ENABLE(MEDIA_SOURCE)
    WEBCORE_TESTSUPPORT_EXPORT void initializeMockMediaSource();
    Vector<String> bufferedSamplesForTrackID(SourceBuffer&, const AtomicString&);
    Vector<String> enqueuedSamplesForTrackID(SourceBuffer&, const AtomicString&);
    void setShouldGenerateTimestamps(SourceBuffer&, bool);
#endif

#if ENABLE(VIDEO)
    ExceptionOr<void> beginMediaSessionInterruption(const String&);
    void endMediaSessionInterruption(const String&);
    void applicationWillBecomeInactive();
    void applicationDidBecomeActive();
    void applicationWillEnterForeground(bool suspendedUnderLock) const;
    void applicationDidEnterBackground(bool suspendedUnderLock) const;
    ExceptionOr<void> setMediaSessionRestrictions(const String& mediaType, StringView restrictionsString);
    ExceptionOr<String> mediaSessionRestrictions(const String& mediaType) const;
    void setMediaElementRestrictions(HTMLMediaElement&, StringView restrictionsString);
    ExceptionOr<void> postRemoteControlCommand(const String&, float argument);
    bool elementIsBlockingDisplaySleep(HTMLMediaElement&) const;
#endif

#if ENABLE(MEDIA_SESSION)
    void sendMediaSessionStartOfInterruptionNotification(MediaSessionInterruptingCategory);
    void sendMediaSessionEndOfInterruptionNotification(MediaSessionInterruptingCategory);
    String mediaSessionCurrentState(MediaSession&) const;
    double mediaElementPlayerVolume(HTMLMediaElement&) const;
    enum class MediaControlEvent { PlayPause, NextTrack, PreviousTrack };
    void sendMediaControlEvent(MediaControlEvent);
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    void setMockMediaPlaybackTargetPickerEnabled(bool);
    ExceptionOr<void> setMockMediaPlaybackTargetPickerState(const String& deviceName, const String& deviceState);
#endif

#if ENABLE(WEB_AUDIO)
    void setAudioContextRestrictions(AudioContext&, StringView restrictionsString);
#endif

    void simulateSystemSleep() const;
    void simulateSystemWake() const;

    enum class PageOverlayType { View, Document };
    ExceptionOr<Ref<MockPageOverlay>> installMockPageOverlay(PageOverlayType);
    ExceptionOr<String> pageOverlayLayerTreeAsText(unsigned short flags) const;

    void setPageMuted(StringView);
    String pageMediaState();

    void setPageDefersLoading(bool);

    RefPtr<File> createFile(const String&);
    void queueMicroTask(int);
    bool testPreloaderSettingViewport();

#if ENABLE(CONTENT_FILTERING)
    MockContentFilterSettings& mockContentFilterSettings();
#endif

#if ENABLE(CSS_SCROLL_SNAP)
    ExceptionOr<String> scrollSnapOffsets(Element&);
    void setPlatformMomentumScrollingPredictionEnabled(bool);
#endif

    ExceptionOr<String> pathStringWithShrinkWrappedRects(const Vector<double>& rectComponents, double radius);

    String getCurrentMediaControlsStatusForElement(HTMLMediaElement&);

    String userVisibleString(const DOMURL&);
    void setShowAllPlugins(bool);

    String resourceLoadStatisticsForOrigin(const String& origin);
    void setResourceLoadStatisticsEnabled(bool);

#if ENABLE(STREAMS_API)
    bool isReadableStreamDisturbed(JSC::ExecState&, JSC::JSValue);
    JSC::JSValue cloneArrayBuffer(JSC::ExecState&, JSC::JSValue, JSC::JSValue, JSC::JSValue);
#endif

    String composedTreeAsText(Node&);

    bool isProcessingUserGesture();
    double lastHandledUserGestureTimestamp();

    RefPtr<GCObservation> observeGC(JSC::JSValue);

    enum class UserInterfaceLayoutDirection { LTR, RTL };
    void setUserInterfaceLayoutDirection(UserInterfaceLayoutDirection);

    bool userPrefersReducedMotion() const;

    void reportBacktrace();

    enum class BaseWritingDirection { Natural, Ltr, Rtl };
    void setBaseWritingDirection(BaseWritingDirection);

#if ENABLE(POINTER_LOCK)
    bool pageHasPendingPointerLock() const;
    bool pageHasPointerLock() const;
#endif

    Vector<String> accessKeyModifiers() const;

    void setQuickLookPassword(const String&);

    void setAsRunningUserScripts(Document&);

#if ENABLE(WEBGL)
    void simulateWebGLContextChanged(WebGLRenderingContext&);
    void failNextGPUStatusCheck(WebGLRenderingContext&);
#endif

    void setPageVisibility(bool isVisible);

#if ENABLE(WEB_RTC)
    void setH264HardwareEncoderAllowed(bool allowed);
#endif

#if ENABLE(MEDIA_STREAM)
    void setCameraMediaStreamTrackOrientation(MediaStreamTrack&, int orientation);
    ExceptionOr<void> setMediaDeviceState(const String& id, const String& property, bool value);
    unsigned long trackAudioSampleCount() const { return m_trackAudioSampleCount; }
    unsigned long trackVideoSampleCount() const { return m_trackVideoSampleCount; }
    void observeMediaStreamTrack(MediaStreamTrack&);
    using TrackFramePromise = DOMPromiseDeferred<IDLInterface<ImageData>>;
    void grabNextMediaStreamTrackFrame(TrackFramePromise&&);
    void delayMediaStreamTrackSamples(MediaStreamTrack&, float);
    void setMediaStreamTrackMuted(MediaStreamTrack&, bool);
    void removeMediaStreamTrack(MediaStream&, MediaStreamTrack&);
#endif

    String audioSessionCategory() const;

private:
    explicit Internals(Document&);
    Document* contextDocument() const;
    Frame* frame() const;

    ExceptionOr<RenderedDocumentMarker*> markerAt(Node&, const String& markerType, unsigned index);

    // RealtimeMediaSource::Observer API
#if ENABLE(MEDIA_STREAM)
    void videoSampleAvailable(MediaSample&) final;
    void audioSamplesAvailable(const MediaTime&, const PlatformAudioData&, const AudioStreamDescription&, size_t) final { m_trackAudioSampleCount++; }

    OrientationNotifier m_orientationNotifier;
    unsigned long m_trackVideoSampleCount { 0 };
    unsigned long m_trackAudioSampleCount { 0 };
    RefPtr<MediaStreamTrack> m_track;
    std::optional<TrackFramePromise> m_nextTrackFramePromise;
#endif

    std::unique_ptr<InspectorStubFrontend> m_inspectorFrontend;
};

} // namespace WebCore
