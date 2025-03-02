/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "WEBPImageDecoder.h"

#if USE(WEBP)

// Backward emulation for earlier versions than 0.1.99.
#if (WEBP_DECODER_ABI_VERSION < 0x0163)
#define MODE_rgbA MODE_RGBA
#define MODE_bgrA MODE_BGRA
#endif

#if CPU(BIG_ENDIAN) || CPU(MIDDLE_ENDIAN)
inline WEBP_CSP_MODE outputMode(bool hasAlpha) { return hasAlpha ? MODE_rgbA : MODE_RGBA; }
#else // LITTLE_ENDIAN, output BGRA pixels.
inline WEBP_CSP_MODE outputMode(bool hasAlpha) { return hasAlpha ? MODE_bgrA : MODE_BGRA; }
#endif

namespace WebCore {

WEBPImageDecoder::WEBPImageDecoder(AlphaOption alphaOption, GammaAndColorProfileOption gammaAndColorProfileOption)
    : ImageDecoder(alphaOption, gammaAndColorProfileOption)
    , m_decoder(0)
    , m_hasAlpha(false)
{
}

WEBPImageDecoder::~WEBPImageDecoder()
{
    clear();
}

void WEBPImageDecoder::clear()
{
    if (m_decoder)
        WebPIDelete(m_decoder);
    m_decoder = 0;
}

ImageFrame* WEBPImageDecoder::frameBufferAtIndex(size_t index)
{
    if (index)
        return 0;

    if (m_frameBufferCache.isEmpty())
        m_frameBufferCache.resize(1);

    ImageFrame& frame = m_frameBufferCache[0];
    if (!frame.isComplete())
        decode(false, isAllDataReceived());
    return &frame;
}

bool WEBPImageDecoder::decode(bool onlySize, bool)
{
    if (failed())
        return false;

    const uint8_t* dataBytes = reinterpret_cast<const uint8_t*>(m_data->data());
    const size_t dataSize = m_data->size();

    if (ImageDecoder::encodedDataStatus() < EncodedDataStatus::SizeAvailable) {
        static const size_t imageHeaderSize = 30;
        if (dataSize < imageHeaderSize)
            return false;
        int width, height;
#if (WEBP_DECODER_ABI_VERSION >= 0x0163)
        WebPBitstreamFeatures features;
        if (WebPGetFeatures(dataBytes, dataSize, &features) != VP8_STATUS_OK)
            return setFailed();
        width = features.width;
        height = features.height;
        m_hasAlpha = features.has_alpha;
#else
        // Earlier version won't be able to display WebP files with alpha.
        if (!WebPGetInfo(dataBytes, dataSize, &width, &height))
            return setFailed();
        m_hasAlpha = false;
#endif
        if (!setSize(IntSize(width, height)))
            return setFailed();
    }

    ASSERT(ImageDecoder::encodedDataStatus() >= EncodedDataStatus::SizeAvailable);
    if (onlySize)
        return true;

    ASSERT(!m_frameBufferCache.isEmpty());
    ImageFrame& buffer = m_frameBufferCache[0];
    ASSERT(!buffer.isComplete());

    if (buffer.isInvalid()) {
        if (!buffer.initialize(size(), m_premultiplyAlpha))
            return setFailed();
        buffer.setDecodingStatus(ImageFrame::DecodingStatus::Partial);
        buffer.setHasAlpha(m_hasAlpha);
    }

    if (!m_decoder) {
        WEBP_CSP_MODE mode = outputMode(m_hasAlpha);
        if (!m_premultiplyAlpha)
            mode = outputMode(false);
        int rowStride = size().width() * sizeof(RGBA32);
        uint8_t* output = reinterpret_cast<uint8_t*>(buffer.backingStore()->pixelAt(0, 0));
        int outputSize = size().height() * rowStride;
        m_decoder = WebPINewRGB(mode, output, outputSize, rowStride);
        if (!m_decoder)
            return setFailed();
    }

    switch (WebPIUpdate(m_decoder, dataBytes, dataSize)) {
    case VP8_STATUS_OK:
        buffer.setDecodingStatus(ImageFrame::DecodingStatus::Complete);
        clear();
        return true;
    case VP8_STATUS_SUSPENDED:
        return false;
    default:
        clear();
        return setFailed();
    }
}

} // namespace WebCore

#endif
