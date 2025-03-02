/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(AVFOUNDATION)
// FIXME: Should be USE(COREMEDIA), but this isn't currently defined on Windows.

#include "CoreMediaSPI.h"
#include "SoftLinking.h"

#if PLATFORM(COCOA)
#include <CoreVideo/CoreVideo.h>
#endif

SOFT_LINK_FRAMEWORK_FOR_SOURCE(WebCore, CoreMedia)

SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBlockBufferCopyDataBytes, OSStatus, (CMBlockBufferRef theSourceBuffer, size_t offsetToData, size_t dataLength, void* destination), (theSourceBuffer, offsetToData, dataLength, destination))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBlockBufferGetDataLength, size_t, (CMBlockBufferRef theBuffer), (theBuffer))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMFormatDescriptionGetExtensions, CFDictionaryRef, (CMFormatDescriptionRef desc), (desc))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetTypeID, CFTypeID, (void), ())
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetDataBuffer, CMBlockBufferRef, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetFormatDescription, CMFormatDescriptionRef, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetSampleTimingInfo, OSStatus, (CMSampleBufferRef sbuf, CMItemIndex sampleIndex, CMSampleTimingInfo* timingInfoOut), (sbuf, sampleIndex, timingInfoOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeCompare, int32_t, (CMTime time1, CMTime time2), (time1, time2))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeAdd, CMTime, (CMTime time1, CMTime time2), (time1, time2))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeGetSeconds, Float64, (CMTime time), (time))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeMake, CMTime, (int64_t value, int32_t timescale), (value, timescale))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeMakeWithSeconds, CMTime, (Float64 seconds, int32_t preferredTimeScale), (seconds, preferredTimeScale))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeRangeGetEnd, CMTime, (CMTimeRange range), (range))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeRangeMake, CMTimeRange, (CMTime start, CMTime duration), (start, duration))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeRangeEqual, Boolean, (CMTimeRange range1, CMTimeRange range2), (range1, range2))

SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMFormatDescriptionExtension_SampleDescriptionExtensionAtoms, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAlignmentType_End, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAlignmentType_Middle, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAlignmentType_Start, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_Alignment, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_BackgroundColorARGB, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_BaseFontSizePercentageRelativeToVideoHeight, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_BoldStyle, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_CharacterBackgroundColorARGB, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_FontFamilyName, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_ForegroundColorARGB, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_ItalicStyle, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_OrthogonalLinePositionPercentageRelativeToWritingDirection, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_RelativeFontSize, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_TextPositionPercentageRelativeToWritingDirection, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_UnderlineStyle, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_VerticalLayout, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextMarkupAttribute_WritingDirectionSizePercentage, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextVerticalLayout_LeftToRight, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTextVerticalLayout_RightToLeft, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTimeInvalid, CMTime)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTimeZero, CMTime)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTimePositiveInfinity, CMTime)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTimeRangeInvalid, CMTimeRange);

#if PLATFORM(COCOA)
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMFormatDescriptionGetMediaSubType, FourCharCode, (CMFormatDescriptionRef desc), (desc))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMFormatDescriptionGetMediaType, CMMediaType, (CMFormatDescriptionRef desc), (desc))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMNotificationCenterGetDefaultLocalCenter, CMNotificationCenterRef, (void), ())
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMNotificationCenterAddListener, OSStatus, (CMNotificationCenterRef center, const void* listener, CMNotificationCallback callback, CFStringRef notification, const void* object, UInt32 flags), (center, listener, callback, notification, object, flags))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMNotificationCenterRemoveListener, OSStatus, (CMNotificationCenterRef center, const void* listener, CMNotificationCallback callback, CFStringRef notification, const void* object), (center, listener, callback, notification, object))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferCallForEachSample, OSStatus, (CMSampleBufferRef sbuf, OSStatus (*callback)( CMSampleBufferRef sampleBuffer, CMItemCount index, void *refcon), void *refcon), (sbuf, callback, refcon))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferCreate, OSStatus, (CFAllocatorRef allocator, CMBlockBufferRef dataBuffer, Boolean dataReady, CMSampleBufferMakeDataReadyCallback makeDataReadyCallback, void *makeDataReadyRefcon, CMFormatDescriptionRef formatDescription, CMItemCount numSamples, CMItemCount numSampleTimingEntries, const CMSampleTimingInfo *sampleTimingArray, CMItemCount numSampleSizeEntries, const size_t *sampleSizeArray, CMSampleBufferRef *sBufOut), (allocator, dataBuffer, dataReady, makeDataReadyCallback, makeDataReadyRefcon, formatDescription, numSamples, numSampleTimingEntries, sampleTimingArray, numSampleSizeEntries, sampleSizeArray, sBufOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferCreateCopy, OSStatus, (CFAllocatorRef allocator, CMSampleBufferRef sbuf, CMSampleBufferRef *sbufCopyOut), (allocator, sbuf, sbufCopyOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferCreateCopyWithNewTiming, OSStatus, (CFAllocatorRef allocator, CMSampleBufferRef originalSBuf, CMItemCount numSampleTimingEntries, const CMSampleTimingInfo *sampleTimingArray, CMSampleBufferRef *sBufCopyOut), (allocator, originalSBuf, numSampleTimingEntries, sampleTimingArray, sBufCopyOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferCreateReadyWithImageBuffer, OSStatus, (CFAllocatorRef allocator, CVImageBufferRef imageBuffer, CMVideoFormatDescriptionRef formatDescription, const CMSampleTimingInfo* sampleTiming, CMSampleBufferRef* sBufOut), (allocator, imageBuffer, formatDescription, sampleTiming, sBufOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetDecodeTimeStamp, CMTime, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetDuration, CMTime, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetImageBuffer, CVImageBufferRef, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetPresentationTimeStamp, CMTime, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetOutputDuration, CMTime, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetOutputPresentationTimeStamp, CMTime, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetSampleAttachmentsArray, CFArrayRef, (CMSampleBufferRef sbuf, Boolean createIfNecessary), (sbuf, createIfNecessary))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetSampleTimingInfoArray, OSStatus, (CMSampleBufferRef sbuf, CMItemCount timingArrayEntries, CMSampleTimingInfo *timingArrayOut, CMItemCount *timingArrayEntriesNeededOut), (sbuf, timingArrayEntries, timingArrayOut, timingArrayEntriesNeededOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeConvertScale, CMTime, (CMTime time, int32_t newTimescale, CMTimeRoundingMethod method), (time, newTimescale, method))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetTotalSampleSize, size_t, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSetAttachment, void, (CMAttachmentBearerRef target, CFStringRef key, CFTypeRef value, CMAttachmentMode attachmentMode), (target, key, value, attachmentMode))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseCreateWithMasterClock, OSStatus, (CFAllocatorRef allocator, CMClockRef masterClock, CMTimebaseRef *timebaseOut), (allocator, masterClock, timebaseOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseGetTime, CMTime, (CMTimebaseRef timebase), (timebase))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseSetRate, OSStatus, (CMTimebaseRef timebase, Float64 rate), (timebase, rate))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseSetTime, OSStatus, (CMTimebaseRef timebase, CMTime time), (timebase, time))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseGetEffectiveRate, Float64, (CMTimebaseRef timebase), (timebase))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseAddTimerDispatchSource, OSStatus, (CMTimebaseRef timebase, dispatch_source_t timerSource), (timebase, timerSource))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseRemoveTimerDispatchSource, OSStatus, (CMTimebaseRef timebase, dispatch_source_t timerSource), (timebase, timerSource))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseSetTimerDispatchSourceNextFireTime, OSStatus, (CMTimebaseRef timebase, dispatch_source_t timerSource, CMTime fireTime, uint32_t flags), (timebase, timerSource, fireTime, flags))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimebaseSetTimerDispatchSourceToFireImmediately, OSStatus, (CMTimebaseRef timebase, dispatch_source_t timerSource), (timebase, timerSource))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeCopyAsDictionary, CFDictionaryRef, (CMTime time, CFAllocatorRef allocator), (time, allocator))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMVideoFormatDescriptionCreateForImageBuffer, OSStatus, (CFAllocatorRef allocator, CVImageBufferRef imageBuffer, CMVideoFormatDescriptionRef* outDesc), (allocator, imageBuffer, outDesc))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMVideoFormatDescriptionGetDimensions, CMVideoDimensions, (CMVideoFormatDescriptionRef videoDesc), (videoDesc))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMVideoFormatDescriptionGetPresentationDimensions, CGSize, (CMVideoFormatDescriptionRef videoDesc, Boolean usePixelAspectRatio, Boolean useCleanAperture), (videoDesc, usePixelAspectRatio, useCleanAperture))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueReset, OSStatus, (CMBufferQueueRef queue), (queue))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueCreate, OSStatus, (CFAllocatorRef allocator, CMItemCount capacity, const CMBufferCallbacks* callbacks, CMBufferQueueRef* queueOut), (allocator, capacity, callbacks, queueOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueGetHead, CMBufferRef, (CMBufferQueueRef queue), (queue))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueDequeueAndRetain, CMBufferRef, (CMBufferQueueRef queue), (queue))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueEnqueue, OSStatus, (CMBufferQueueRef queue, CMBufferRef buffer), (queue, buffer))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueIsEmpty, Boolean, (CMBufferQueueRef queue), (queue))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueGetBufferCount, CMItemCount, (CMBufferQueueRef queue), (queue))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueGetFirstPresentationTimeStamp, CMTime, (CMBufferQueueRef queue), (queue))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueGetEndPresentationTimeStamp, CMTime, (CMBufferQueueRef queue), (queue))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMBufferQueueInstallTriggerWithIntegerThreshold, OSStatus, (CMBufferQueueRef queue, CMBufferQueueTriggerCallback triggerCallback, void* triggerRefcon, CMBufferQueueTriggerCondition triggerCondition, CMItemCount triggerThreshold, CMBufferQueueTriggerToken* triggerTokenOut), (queue, triggerCallback, triggerRefcon, triggerCondition, triggerThreshold, triggerTokenOut))

SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleAttachmentKey_DoNotDisplay, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleAttachmentKey_NotSync, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleBufferAttachmentKey_DisplayEmptyMediaImmediately, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleBufferAttachmentKey_DrainAfterDecoding, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleBufferAttachmentKey_EmptyMedia, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleBufferAttachmentKey_PostNotificationWhenConsumed, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleBufferAttachmentKey_ResetDecoderBeforeDecoding, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleAttachmentKey_DisplayImmediately, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMSampleBufferConsumerNotification_BufferConsumed, CFStringRef)

SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTimebaseNotification_EffectiveRateChanged, CFStringRef)
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTimebaseNotification_TimeJumped, CFStringRef)
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMAudioFormatDescriptionGetStreamBasicDescription, const AudioStreamBasicDescription *, (CMAudioFormatDescriptionRef desc), (desc))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer, OSStatus, (CMSampleBufferRef sbuf, size_t *bufferListSizeNeededOut, AudioBufferList *bufferListOut, size_t bufferListSize, CFAllocatorRef bbufStructAllocator, CFAllocatorRef bbufMemoryAllocator, uint32_t flags, CMBlockBufferRef *blockBufferOut), (sbuf, bufferListSizeNeededOut, bufferListOut, bufferListSize, bbufStructAllocator, bbufMemoryAllocator, flags, blockBufferOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetNumSamples, CMItemCount, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferCallBlockForEachSample, OSStatus, (CMSampleBufferRef sbuf, OSStatus (^handler)(CMSampleBufferRef, CMItemCount)), (sbuf, handler))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferCopySampleBufferForRange, OSStatus, (CFAllocatorRef allocator, CMSampleBufferRef sbuf, CFRange sampleRange, CMSampleBufferRef* sBufOut), (allocator, sbuf, sampleRange, sBufOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferGetSampleSizeArray, OSStatus, (CMSampleBufferRef sbuf, CMItemCount sizeArrayEntries, size_t* sizeArrayOut, CMItemCount* sizeArrayEntriesNeededOut), (sbuf, sizeArrayEntries, sizeArrayOut, sizeArrayEntriesNeededOut))

SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMAudioSampleBufferCreateWithPacketDescriptions, OSStatus, (CFAllocatorRef allocator, CMBlockBufferRef dataBuffer, Boolean dataReady, CMSampleBufferMakeDataReadyCallback makeDataReadyCallback, void *makeDataReadyRefcon, CMFormatDescriptionRef formatDescription, CMItemCount numSamples, CMTime sbufPTS, const AudioStreamPacketDescription *packetDescriptions, CMSampleBufferRef *sBufOut), (allocator, dataBuffer, dataReady, makeDataReadyCallback, makeDataReadyRefcon, formatDescription, numSamples, sbufPTS, packetDescriptions, sBufOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferSetDataBufferFromAudioBufferList, OSStatus, (CMSampleBufferRef sbuf, CFAllocatorRef bbufStructAllocator, CFAllocatorRef bbufMemoryAllocator, uint32_t flags, const AudioBufferList *bufferList), (sbuf, bbufStructAllocator, bbufMemoryAllocator, flags, bufferList))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMSampleBufferSetDataReady, OSStatus, (CMSampleBufferRef sbuf), (sbuf))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMAudioFormatDescriptionCreate, OSStatus, (CFAllocatorRef allocator, const AudioStreamBasicDescription* asbd, size_t layoutSize, const AudioChannelLayout* layout, size_t magicCookieSize, const void* magicCookie, CFDictionaryRef extensions, CMAudioFormatDescriptionRef* outDesc), (allocator, asbd, layoutSize, layout, magicCookieSize, magicCookie, extensions, outDesc))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMClockGetHostTimeClock, CMClockRef, (void), ())
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMClockGetTime, CMTime, (CMClockRef clock), (clock))
#endif // PLATFORM(COCOA)

#if PLATFORM(IOS)
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMAudioClockCreate, OSStatus, (CFAllocatorRef allocator, CMClockRef *clockOut), (allocator, clockOut))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeMaximum, CMTime, (CMTime time1, CMTime time2), (time1, time2))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeMinimum, CMTime, (CMTime time1, CMTime time2), (time1, time2))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeRangeContainsTime, Boolean, (CMTimeRange range, CMTime time), (range, time))
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeSubtract, CMTime, (CMTime minuend, CMTime subtrahend), (minuend, subtrahend))

SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, CoreMedia, kCMTimeIndefinite, CMTime)
#endif // PLATFORM(IOS)

#if PLATFORM(MAC)
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMAudioDeviceClockCreate, OSStatus, (CFAllocatorRef allocator, CFStringRef deviceUID, CMClockRef *clockOut), (allocator, deviceUID, clockOut))
#endif // PLATFORM(MAC)

#if PLATFORM(WIN)
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, CoreMedia, CMTimeMakeFromDictionary, CMTime, (CFDictionaryRef dict), (dict))
#endif // PLATFORM(WIN)

#endif // USE(AVFOUNDATION)
