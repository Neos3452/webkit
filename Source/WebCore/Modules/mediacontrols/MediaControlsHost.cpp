/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(MEDIA_CONTROLS_SCRIPT)

#include "MediaControlsHost.h"

#include "CaptionUserPreferences.h"
#include "Element.h"
#include "HTMLMediaElement.h"
#include "Logging.h"
#include "MediaControlElements.h"
#include "Page.h"
#include "PageGroup.h"
#include "RenderTheme.h"
#include "TextTrack.h"
#include "TextTrackList.h"
#include <runtime/JSCJSValueInlines.h>
#include <wtf/UUID.h>

namespace WebCore {

const AtomicString& MediaControlsHost::automaticKeyword()
{
    static NeverDestroyed<const AtomicString> automatic("automatic", AtomicString::ConstructFromLiteral);
    return automatic;
}

const AtomicString& MediaControlsHost::forcedOnlyKeyword()
{
    static NeverDestroyed<const AtomicString> forcedOn("forced-only", AtomicString::ConstructFromLiteral);
    return forcedOn;
}

const AtomicString& MediaControlsHost::alwaysOnKeyword()
{
    static NeverDestroyed<const AtomicString> alwaysOn("always-on", AtomicString::ConstructFromLiteral);
    return alwaysOn;
}

const AtomicString& MediaControlsHost::manualKeyword()
{
    static NeverDestroyed<const AtomicString> alwaysOn("manual", AtomicString::ConstructFromLiteral);
    return alwaysOn;
}


Ref<MediaControlsHost> MediaControlsHost::create(HTMLMediaElement* mediaElement)
{
    return adoptRef(*new MediaControlsHost(mediaElement));
}

MediaControlsHost::MediaControlsHost(HTMLMediaElement* mediaElement)
    : m_mediaElement(mediaElement)
{
    ASSERT(mediaElement);
}

MediaControlsHost::~MediaControlsHost()
{
}

Vector<RefPtr<TextTrack>> MediaControlsHost::sortedTrackListForMenu(TextTrackList& trackList)
{
    Page* page = m_mediaElement->document().page();
    if (!page)
        return { };

    return page->group().captionPreferences().sortedTrackListForMenu(&trackList);
}

Vector<RefPtr<AudioTrack>> MediaControlsHost::sortedTrackListForMenu(AudioTrackList& trackList)
{
    Page* page = m_mediaElement->document().page();
    if (!page)
        return { };

    return page->group().captionPreferences().sortedTrackListForMenu(&trackList);
}

String MediaControlsHost::displayNameForTrack(const std::optional<TextOrAudioTrack>& track)
{
    if (!track)
        return emptyString();

    Page* page = m_mediaElement->document().page();
    if (!page)
        return emptyString();

    return WTF::visit([&page](auto& track) {
        return page->group().captionPreferences().displayNameForTrack(track.get());
    }, track.value());
}

TextTrack* MediaControlsHost::captionMenuOffItem()
{
    return TextTrack::captionMenuOffItem();
}

TextTrack* MediaControlsHost::captionMenuAutomaticItem()
{
    return TextTrack::captionMenuAutomaticItem();
}

AtomicString MediaControlsHost::captionDisplayMode() const
{
    Page* page = m_mediaElement->document().page();
    if (!page)
        return emptyAtom;

    switch (page->group().captionPreferences().captionDisplayMode()) {
    case CaptionUserPreferences::Automatic:
        return automaticKeyword();
    case CaptionUserPreferences::ForcedOnly:
        return forcedOnlyKeyword();
    case CaptionUserPreferences::AlwaysOn:
        return alwaysOnKeyword();
    case CaptionUserPreferences::Manual:
        return manualKeyword();
    default:
        ASSERT_NOT_REACHED();
        return emptyAtom;
    }
}

void MediaControlsHost::setSelectedTextTrack(TextTrack* track)
{
    m_mediaElement->setSelectedTextTrack(track);
}

Element* MediaControlsHost::textTrackContainer()
{
    if (!m_textTrackContainer) {
        m_textTrackContainer = MediaControlTextTrackContainerElement::create(m_mediaElement->document());
        m_textTrackContainer->setMediaController(m_mediaElement);
    }
    return m_textTrackContainer.get();
}

void MediaControlsHost::updateTextTrackContainer()
{
    if (m_textTrackContainer)
        m_textTrackContainer->updateDisplay();
}

void MediaControlsHost::enteredFullscreen()
{
    if (m_textTrackContainer)
        m_textTrackContainer->enteredFullscreen();
}

void MediaControlsHost::exitedFullscreen()
{
    if (m_textTrackContainer)
        m_textTrackContainer->exitedFullscreen();
}

void MediaControlsHost::updateCaptionDisplaySizes()
{
    if (m_textTrackContainer)
        m_textTrackContainer->updateSizes(true);
}
    
bool MediaControlsHost::allowsInlineMediaPlayback() const
{
    return !m_mediaElement->mediaSession().requiresFullscreenForVideoPlayback(*m_mediaElement);
}

bool MediaControlsHost::supportsFullscreen() const
{
    return m_mediaElement->supportsFullscreen(HTMLMediaElementEnums::VideoFullscreenModeStandard);
}

bool MediaControlsHost::isVideoLayerInline() const
{
    return m_mediaElement->isVideoLayerInline();
}

bool MediaControlsHost::isInMediaDocument() const
{
    return m_mediaElement->document().isMediaDocument();
}

void MediaControlsHost::setPreparedToReturnVideoLayerToInline(bool value)
{
    m_mediaElement->setPreparedToReturnVideoLayerToInline(value);
}

bool MediaControlsHost::userGestureRequired() const
{
    return !m_mediaElement->mediaSession().playbackPermitted(*m_mediaElement);
}

bool MediaControlsHost::shouldForceControlsDisplay() const
{
    return m_mediaElement->shouldForceControlsDisplay();
}

String MediaControlsHost::externalDeviceDisplayName() const
{
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    MediaPlayer* player = m_mediaElement->player();
    if (!player) {
        LOG(Media, "MediaControlsHost::externalDeviceDisplayName - returning \"\" because player is NULL");
        return emptyString();
    }
    
    String name = player->wirelessPlaybackTargetName();
    LOG(Media, "MediaControlsHost::externalDeviceDisplayName - returning \"%s\"", name.utf8().data());
    
    return name;
#else
    return emptyString();
#endif
}

auto MediaControlsHost::externalDeviceType() const -> DeviceType
{
#if !ENABLE(WIRELESS_PLAYBACK_TARGET)
    return DeviceType::None;
#else
    MediaPlayer* player = m_mediaElement->player();
    if (!player) {
        LOG(Media, "MediaControlsHost::externalDeviceType - returning \"none\" because player is NULL");
        return DeviceType::None;
    }
    
    switch (player->wirelessPlaybackTargetType()) {
    case MediaPlayer::TargetTypeNone:
        return DeviceType::None;
    case MediaPlayer::TargetTypeAirPlay:
        return DeviceType::Airplay;
    case MediaPlayer::TargetTypeTVOut:
        return DeviceType::Tvout;
    }

    ASSERT_NOT_REACHED();
    return DeviceType::None;
#endif
}

bool MediaControlsHost::controlsDependOnPageScaleFactor() const
{
    return m_mediaElement->mediaControlsDependOnPageScaleFactor();
}

void MediaControlsHost::setControlsDependOnPageScaleFactor(bool value)
{
    m_mediaElement->setMediaControlsDependOnPageScaleFactor(value);
}

String MediaControlsHost::generateUUID() const
{
    return createCanonicalUUIDString();
}

String MediaControlsHost::shadowRootCSSText() const
{
    return RenderTheme::singleton().modernMediaControlsStyleSheet();
}

String MediaControlsHost::base64StringForIconNameAndType(const String& iconName, const String& iconType) const
{

    return RenderTheme::singleton().mediaControlsBase64StringForIconNameAndType(iconName, iconType);
}

String MediaControlsHost::formattedStringForDuration(double durationInSeconds) const
{
    return RenderTheme::singleton().mediaControlsFormattedStringForDuration(durationInSeconds);
}

}

#endif
