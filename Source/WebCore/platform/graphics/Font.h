/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006, 2008, 2010, 2015-2016 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2008 Torch Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef Font_h
#define Font_h

#include "FloatRect.h"
#include "FontMetrics.h"
#include "FontPlatformData.h"
#include "GlyphBuffer.h"
#include "GlyphMetricsMap.h"
#include "GlyphPage.h"
#include "OpenTypeMathData.h"
#if ENABLE(OPENTYPE_VERTICAL)
#include "OpenTypeVerticalData.h"
#endif
#include <wtf/BitVector.h>
#include <wtf/Optional.h>
#include <wtf/text/StringHash.h>

#if PLATFORM(COCOA)
#include "WebCoreSystemInterface.h"
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(WIN)
#include <usp10.h>
#endif

#if USE(CAIRO)
#include <cairo.h>
#endif

#if USE(CG)
#include "CoreGraphicsSPI.h"
#endif

#if USE(DIRECT2D)
interface IDWriteFactory;
interface IDWriteGdiInterop;
#endif

namespace WebCore {

class GlyphPage;
class FontDescription;
class SharedBuffer;
struct GlyphData;
struct WidthIterator;

enum FontVariant { AutoVariant, NormalVariant, SmallCapsVariant, EmphasisMarkVariant, BrokenIdeographVariant };
enum Pitch { UnknownPitch, FixedPitch, VariablePitch };

class Font : public RefCounted<Font> {
public:
    // Used to create platform fonts.
    enum class Origin {
        Remote,
        Local
    };
    enum class Interstitial {
        Yes,
        No
    };
    enum class Visibility {
        Visible,
        Invisible
    };
    enum class OrientationFallback {
        Yes,
        No
    };
    static Ref<Font> create(const FontPlatformData& platformData, Origin origin = Origin::Local, Interstitial interstitial = Interstitial::No, Visibility visibility = Visibility::Visible, OrientationFallback orientationFallback = OrientationFallback::No)
    {
        return adoptRef(*new Font(platformData, origin, interstitial, visibility, orientationFallback));
    }

    WEBCORE_EXPORT ~Font();

    static const Font* systemFallback() { return reinterpret_cast<const Font*>(-1); }

    const FontPlatformData& platformData() const { return m_platformData; }
    const OpenTypeMathData* mathData() const;
#if ENABLE(OPENTYPE_VERTICAL)
    const OpenTypeVerticalData* verticalData() const { return m_verticalData.get(); }
#endif

    const Font* smallCapsFont(const FontDescription&) const;
    const Font& noSynthesizableFeaturesFont() const;
    const Font* emphasisMarkFont(const FontDescription&) const;
    const Font& brokenIdeographFont() const;

    const Font* variantFont(const FontDescription& description, FontVariant variant) const
    {
#if PLATFORM(COCOA)
        ASSERT(variant != SmallCapsVariant);
#endif
        switch (variant) {
        case SmallCapsVariant:
            return smallCapsFont(description);
        case EmphasisMarkVariant:
            return emphasisMarkFont(description);
        case BrokenIdeographVariant:
            return &brokenIdeographFont();
        case AutoVariant:
        case NormalVariant:
            break;
        }
        ASSERT_NOT_REACHED();
        return const_cast<Font*>(this);
    }

    bool variantCapsSupportsCharacterForSynthesis(FontVariantCaps, UChar32) const;

    const Font& verticalRightOrientationFont() const;
    const Font& uprightOrientationFont() const;

    bool hasVerticalGlyphs() const { return m_hasVerticalGlyphs; }
    bool isTextOrientationFallback() const { return m_isTextOrientationFallback; }

    FontMetrics& fontMetrics() { return m_fontMetrics; }
    const FontMetrics& fontMetrics() const { return m_fontMetrics; }
    float sizePerUnit() const { return platformData().size() / (fontMetrics().unitsPerEm() ? fontMetrics().unitsPerEm() : 1); }

    float maxCharWidth() const { return m_maxCharWidth; }
    void setMaxCharWidth(float maxCharWidth) { m_maxCharWidth = maxCharWidth; }

    float avgCharWidth() const { return m_avgCharWidth; }
    void setAvgCharWidth(float avgCharWidth) { m_avgCharWidth = avgCharWidth; }

    FloatRect boundsForGlyph(Glyph) const;
    float widthForGlyph(Glyph) const;
    FloatRect platformBoundsForGlyph(Glyph) const;
    float platformWidthForGlyph(Glyph) const;

    float spaceWidth() const { return m_spaceWidth; }
    float adjustedSpaceWidth() const { return m_adjustedSpaceWidth; }
    void setSpaceWidths(float spaceWidth)
    {
        m_spaceWidth = spaceWidth;
        m_adjustedSpaceWidth = spaceWidth;
    }

#if USE(CG) || USE(DIRECT2D) || USE(CAIRO)
    float syntheticBoldOffset() const { return m_syntheticBoldOffset; }
#endif

    Glyph spaceGlyph() const { return m_spaceGlyph; }
    void setSpaceGlyph(Glyph spaceGlyph) { m_spaceGlyph = spaceGlyph; }
    Glyph zeroWidthSpaceGlyph() const { return m_zeroWidthSpaceGlyph; }
    void setZeroWidthSpaceGlyph(Glyph spaceGlyph) { m_zeroWidthSpaceGlyph = spaceGlyph; }
    bool isZeroWidthSpaceGlyph(Glyph glyph) const { return glyph == m_zeroWidthSpaceGlyph && glyph; }
    Glyph zeroGlyph() const { return m_zeroGlyph; }
    void setZeroGlyph(Glyph zeroGlyph) { m_zeroGlyph = zeroGlyph; }

    GlyphData glyphDataForCharacter(UChar32) const;
    Glyph glyphForCharacter(UChar32) const;

    RefPtr<Font> systemFallbackFontForCharacter(UChar32, const FontDescription&, bool isForPlatformFont) const;

    const GlyphPage* glyphPage(unsigned pageNumber) const;

    void determinePitch();
    Pitch pitch() const { return m_treatAsFixedPitch ? FixedPitch : VariablePitch; }

    Origin origin() const { return m_origin; }
    bool isInterstitial() const { return m_isInterstitial; }
    Visibility visibility() const { return m_visibility; }

#ifndef NDEBUG
    String description() const;
#endif

#if PLATFORM(IOS)
    bool shouldNotBeUsedForArabic() const { return m_shouldNotBeUsedForArabic; };
#endif
#if PLATFORM(COCOA)
    CTFontRef getCTFont() const { return m_platformData.font(); }
    CFDictionaryRef getCFStringAttributes(bool enableKerning, FontOrientation) const;
    const BitVector& glyphsSupportedBySmallCaps() const;
    const BitVector& glyphsSupportedByAllSmallCaps() const;
    const BitVector& glyphsSupportedByPetiteCaps() const;
    const BitVector& glyphsSupportedByAllPetiteCaps() const;
#endif

#if PLATFORM(COCOA) || USE(HARFBUZZ)
    bool canRenderCombiningCharacterSequence(const UChar*, size_t) const;
#endif

    bool applyTransforms(GlyphBufferGlyph*, GlyphBufferAdvance*, size_t glyphCount, bool enableKerning, bool requiresShaping) const;

#if PLATFORM(WIN)
    SCRIPT_FONTPROPERTIES* scriptFontProperties() const;
    SCRIPT_CACHE* scriptCache() const { return &m_scriptCache; }
    static void setShouldApplyMacAscentHack(bool);
    static bool shouldApplyMacAscentHack();
    static float ascentConsideringMacAscentHack(const WCHAR*, float ascent, float descent);
#endif

#if USE(DIRECT2D)
    WEBCORE_EXPORT static IDWriteFactory* systemDWriteFactory();
    WEBCORE_EXPORT static IDWriteGdiInterop* systemDWriteGdiInterop();
#endif

private:
    Font(const FontPlatformData&, Origin, Interstitial, Visibility, OrientationFallback);

    void platformInit();
    void platformGlyphInit();
    void platformCharWidthInit();
    void platformDestroy();

    void initCharWidths();

    RefPtr<Font> createFontWithoutSynthesizableFeatures() const;
    RefPtr<Font> createScaledFont(const FontDescription&, float scaleFactor) const;
    RefPtr<Font> platformCreateScaledFont(const FontDescription&, float scaleFactor) const;

    void removeFromSystemFallbackCache();

#if PLATFORM(WIN)
    void initGDIFont();
    void platformCommonDestroy();
    FloatRect boundsForGDIGlyph(Glyph) const;
    float widthForGDIGlyph(Glyph) const;
#endif

    FontMetrics m_fontMetrics;
    float m_maxCharWidth;
    float m_avgCharWidth;

    const FontPlatformData m_platformData;

    mutable RefPtr<GlyphPage> m_glyphPageZero;
    mutable HashMap<unsigned, RefPtr<GlyphPage>> m_glyphPages;
    mutable std::unique_ptr<GlyphMetricsMap<FloatRect>> m_glyphToBoundsMap;
    mutable GlyphMetricsMap<float> m_glyphToWidthMap;

    mutable RefPtr<OpenTypeMathData> m_mathData;
#if ENABLE(OPENTYPE_VERTICAL)
    RefPtr<OpenTypeVerticalData> m_verticalData;
#endif

    Glyph m_spaceGlyph { 0 };
    float m_spaceWidth { 0 };
    Glyph m_zeroGlyph { 0 };
    float m_adjustedSpaceWidth { 0 };

    Glyph m_zeroWidthSpaceGlyph { 0 };

    struct DerivedFonts {
#if !COMPILER(MSVC)
        WTF_MAKE_FAST_ALLOCATED;
#endif
    public:

        RefPtr<Font> smallCaps;
        RefPtr<Font> noSynthesizableFeatures;
        RefPtr<Font> emphasisMark;
        RefPtr<Font> brokenIdeograph;
        RefPtr<Font> verticalRightOrientation;
        RefPtr<Font> uprightOrientation;
    };

    mutable std::unique_ptr<DerivedFonts> m_derivedFontData;

#if USE(CG) || USE(DIRECT2D) || USE(CAIRO)
    float m_syntheticBoldOffset;
#endif

#if PLATFORM(COCOA)
    mutable RetainPtr<CFDictionaryRef> m_nonKernedCFStringAttributes;
    mutable RetainPtr<CFDictionaryRef> m_kernedCFStringAttributes;
    mutable std::optional<BitVector> m_glyphsSupportedBySmallCaps;
    mutable std::optional<BitVector> m_glyphsSupportedByAllSmallCaps;
    mutable std::optional<BitVector> m_glyphsSupportedByPetiteCaps;
    mutable std::optional<BitVector> m_glyphsSupportedByAllPetiteCaps;
#endif

#if PLATFORM(COCOA) || USE(HARFBUZZ)
    mutable std::unique_ptr<HashMap<String, bool>> m_combiningCharacterSequenceSupport;
#endif

#if PLATFORM(WIN)
    mutable SCRIPT_CACHE m_scriptCache;
    mutable SCRIPT_FONTPROPERTIES* m_scriptFontProperties;
#endif

    Origin m_origin; // Whether or not we are custom font loaded via @font-face
    Visibility m_visibility; // @font-face's internal timer can cause us to show fonts even when a font is being downloaded.

    unsigned m_treatAsFixedPitch : 1;
    unsigned m_isInterstitial : 1; // Whether or not this custom font is the last resort placeholder for a loading font

    unsigned m_isTextOrientationFallback : 1;
    unsigned m_isBrokenIdeographFallback : 1;
    unsigned m_hasVerticalGlyphs : 1;

    unsigned m_isUsedInSystemFallbackCache : 1;

#if PLATFORM(IOS)
    unsigned m_shouldNotBeUsedForArabic : 1;
#endif
};

#if PLATFORM(IOS)
bool fontFamilyShouldNotBeUsedForArabic(CFStringRef);
#endif

ALWAYS_INLINE FloatRect Font::boundsForGlyph(Glyph glyph) const
{
    if (isZeroWidthSpaceGlyph(glyph))
        return FloatRect();

    FloatRect bounds;
    if (m_glyphToBoundsMap) {
        bounds = m_glyphToBoundsMap->metricsForGlyph(glyph);
        if (bounds.width() != cGlyphSizeUnknown)
            return bounds;
    }

    bounds = platformBoundsForGlyph(glyph);
    if (!m_glyphToBoundsMap)
        m_glyphToBoundsMap = std::make_unique<GlyphMetricsMap<FloatRect>>();
    m_glyphToBoundsMap->setMetricsForGlyph(glyph, bounds);
    return bounds;
}

ALWAYS_INLINE float Font::widthForGlyph(Glyph glyph) const
{
    // The optimization of returning 0 for the zero-width-space glyph is incorrect for the LastResort font,
    // used in place of the actual font when isLoading() is true on both macOS and iOS.
    // The zero-width-space glyph in that font does not have a width of zero and, further, that glyph is used
    // for many other characters and must not be zero width when used for them.
    if (isZeroWidthSpaceGlyph(glyph) && !isInterstitial())
        return 0;

    float width = m_glyphToWidthMap.metricsForGlyph(glyph);
    if (width != cGlyphSizeUnknown)
        return width;

#if ENABLE(OPENTYPE_VERTICAL)
    if (m_verticalData) {
#if USE(CG) || USE(DIRECT2D) || USE(CAIRO)
        width = m_verticalData->advanceHeight(this, glyph) + m_syntheticBoldOffset;
#else
        width = m_verticalData->advanceHeight(this, glyph);
#endif
    } else
#endif
        width = platformWidthForGlyph(glyph);

    m_glyphToWidthMap.setMetricsForGlyph(glyph, width);
    return width;
}

} // namespace WebCore

#endif // Font_h
