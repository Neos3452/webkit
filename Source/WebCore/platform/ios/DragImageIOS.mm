/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#import "config.h"
#import "DragImage.h"

#if PLATFORM(IOS)

#import "Document.h"
#import "Element.h"
#import "FloatRoundedRect.h"
#import "FontCascade.h"
#import "FontPlatformData.h"
#import "Frame.h"
#import "GraphicsContext.h"
#import "Image.h"
#import "NotImplemented.h"
#import "Page.h"
#import "Range.h"
#import "SoftLinking.h"
#import "StringTruncator.h"
#import "TextIndicator.h"
#import "TextRun.h"
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#import <UIKit/UIColor.h>
#import <UIKit/UIFont.h>
#import <UIKit/UIGraphicsImageRenderer.h>
#import <UIKit/UIImage.h>
#import <wtf/NeverDestroyed.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"

SOFT_LINK_FRAMEWORK(UIKit)
SOFT_LINK_CLASS(UIKit, UIFont)
SOFT_LINK_CLASS(UIKit, UIGraphicsImageRenderer)
SOFT_LINK(UIKit, UIGraphicsBeginImageContextWithOptions, void, (CGSize size, BOOL opaque, CGFloat scale), (size, opaque, scale))
SOFT_LINK(UIKit, UIGraphicsGetCurrentContext, CGContextRef, (void), ())
SOFT_LINK(UIKit, UIGraphicsGetImageFromCurrentImageContext, UIImage *, (void), ())
SOFT_LINK(UIKit, UIGraphicsEndImageContext, void, (void), ())

#pragma clang diagnostic pop

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)

IntSize dragImageSize(DragImageRef image)
{
    return IntSize(CGImageGetWidth(image.get()), CGImageGetHeight(image.get()));
}

DragImageRef scaleDragImage(DragImageRef image, FloatSize scale)
{
    CGSize imageSize = CGSizeMake(scale.width() * CGImageGetWidth(image.get()), scale.height() * CGImageGetHeight(image.get()));
    CGRect imageRect = { CGPointZero, imageSize };

    RetainPtr<UIGraphicsImageRenderer> render = adoptNS([allocUIGraphicsImageRendererInstance() initWithSize:imageSize]);
    UIImage *imageCopy = [render.get() imageWithActions:^(UIGraphicsImageRendererContext *rendererContext) {
        CGContextRef context = rendererContext.CGContext;
        CGContextTranslateCTM(context, 0, imageSize.height);
        CGContextScaleCTM(context, 1, -1);
        CGContextDrawImage(context, imageRect, image.get());
    }];
    return imageCopy.CGImage;
}

DragImageRef createDragImageFromImage(Image * _Nullable image, ImageOrientationDescription orientation)
{
    if (!image)
        return nil;

    CGSize imageSize(image->size());

    RetainPtr<UIGraphicsImageRenderer> render = adoptNS([allocUIGraphicsImageRendererInstance() initWithSize:imageSize]);
    UIImage *imageCopy = [render.get() imageWithActions:^(UIGraphicsImageRendererContext *rendererContext) {
        GraphicsContext context(rendererContext.CGContext);
        context.translate(0, imageSize.height);
        context.scale({ 1, -1 });
        ImagePaintingOptions paintingOptions;
        paintingOptions.m_orientationDescription = orientation;
        context.drawImage(*image, FloatPoint(), paintingOptions);
    }];
    return imageCopy.CGImage;
}

void deleteDragImage(DragImageRef)
{
}

static TextIndicatorOptions defaultLinkIndicatorOptions = TextIndicatorOptionTightlyFitContent | TextIndicatorOptionRespectTextColor | TextIndicatorOptionUseBoundingRectAndPaintAllContentForComplexRanges | TextIndicatorOptionExpandClipBeyondVisibleRect | TextIndicatorOptionComputeEstimatedBackgroundColor;

DragImageRef createDragImageForLink(Element& linkElement, URL& url, const String& title, TextIndicatorData& indicatorData, FontRenderingMode, float)
{
    // FIXME: Most of this can go away once we can use UIURLDragPreviewView unconditionally.
    static CGFloat dragImagePadding = 10;
    static LazyNeverDestroyed<FontCascade> titleFontCascade;
    static LazyNeverDestroyed<FontCascade> urlFontCascade;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^ {
        UIFont *titleFont = [getUIFontClass() systemFontOfSize:16];
        UIFont *urlFont = [getUIFontClass() systemFontOfSize:14];
        titleFontCascade.construct(FontPlatformData(CTFontCreateWithName((CFStringRef)titleFont.fontName, titleFont.pointSize, nil), titleFont.pointSize), AutoSmoothing);
        urlFontCascade.construct(FontPlatformData(CTFontCreateWithName((CFStringRef)urlFont.fontName, urlFont.pointSize, nil), urlFont.pointSize), AutoSmoothing);
    });

    String topString(title.stripWhiteSpace());
    String bottomString([(NSURL *)url absoluteString]);
    if (topString.isEmpty()) {
        topString = bottomString;
        bottomString = emptyString();
    }

    static CGFloat maxTextWidth = 320;
    auto truncatedTopString = StringTruncator::rightTruncate(topString, maxTextWidth, titleFontCascade);
    auto truncatedBottomString = StringTruncator::centerTruncate(bottomString, maxTextWidth, urlFontCascade);
    CGFloat textWidth = std::max(StringTruncator::width(truncatedTopString, titleFontCascade), StringTruncator::width(truncatedBottomString, urlFontCascade));
    CGFloat textHeight = truncatedBottomString.isEmpty() ? 22 : 44;

    CGRect imageRect = CGRectMake(0, 0, textWidth + 2 * dragImagePadding, textHeight + 2 * dragImagePadding);

    RetainPtr<UIGraphicsImageRenderer> render = adoptNS([allocUIGraphicsImageRendererInstance() initWithSize:imageRect.size]);
    UIImage *image = [render.get() imageWithActions:^(UIGraphicsImageRendererContext *rendererContext) {
        GraphicsContext context(rendererContext.CGContext);
        context.translate(0, CGRectGetHeight(imageRect));
        context.scale({ 1, -1 });
        context.fillRoundedRect(FloatRoundedRect(imageRect, FloatRoundedRect::Radii(4)), { 255, 255, 255 });
        titleFontCascade->drawText(context, TextRun(truncatedTopString), FloatPoint(dragImagePadding, 18 + dragImagePadding));
        if (!truncatedBottomString.isEmpty())
            urlFontCascade->drawText(context, TextRun(truncatedBottomString), FloatPoint(dragImagePadding, 40 + dragImagePadding));
    }];

    auto linkRange = rangeOfContents(linkElement);
    if (auto textIndicator = TextIndicator::createWithRange(linkRange, defaultLinkIndicatorOptions, TextIndicatorPresentationTransition::None, FloatSize()))
        indicatorData = textIndicator->data();

    return image.CGImage;
}

DragImageRef createDragImageIconForCachedImageFilename(const String&)
{
    notImplemented();
    return nullptr;
}

DragImageRef platformAdjustDragImageForDeviceScaleFactor(DragImageRef image, float)
{
    // On iOS, we just create the drag image at the right device scale factor, so we don't need to scale it by 1 / deviceScaleFactor later.
    return image;
}

static TextIndicatorOptions defaultSelectionDragImageTextIndicatorOptions = TextIndicatorOptionExpandClipBeyondVisibleRect | TextIndicatorOptionPaintAllContent | TextIndicatorOptionUseSelectionRectForSizing | TextIndicatorOptionComputeEstimatedBackgroundColor;

DragImageRef createDragImageForSelection(Frame& frame, TextIndicatorData& indicatorData, bool forceBlackText)
{
    if (auto document = frame.document())
        document->updateLayout();

    TextIndicatorOptions options = defaultSelectionDragImageTextIndicatorOptions;
    if (!forceBlackText)
        options |= TextIndicatorOptionRespectTextColor;

    auto textIndicator = TextIndicator::createWithSelectionInFrame(frame, options, TextIndicatorPresentationTransition::None, FloatSize());
    if (!textIndicator)
        return nullptr;

    auto image = textIndicator->contentImage();
    if (image)
        indicatorData = textIndicator->data();
    else
        return nullptr;

    FloatRect imageRect(0, 0, image->width(), image->height());
    if (auto page = frame.page())
        imageRect.scale(1 / page->deviceScaleFactor());


    RetainPtr<UIGraphicsImageRenderer> render = adoptNS([allocUIGraphicsImageRendererInstance() initWithSize:imageRect.size()]);
    UIImage *finalImage = [render.get() imageWithActions:^(UIGraphicsImageRendererContext *rendererContext) {
        GraphicsContext context(rendererContext.CGContext);
        context.translate(0, imageRect.height());
        context.scale({ 1, -1 });
        context.drawImage(*image, imageRect);
    }];

    return finalImage.CGImage;
}

DragImageRef dissolveDragImageToFraction(DragImageRef image, float)
{
    notImplemented();
    return image;
}

#else

void deleteDragImage(RetainPtr<CGImageRef>)
{
    // Since this is a RetainPtr, there's nothing additional we need to do to
    // delete it. It will be released when it falls out of scope.
}

// FIXME: fix signature of dragImageSize() to avoid copying the argument.
IntSize dragImageSize(RetainPtr<CGImageRef> image)
{
    return IntSize(CGImageGetWidth(image.get()), CGImageGetHeight(image.get()));
}

RetainPtr<CGImageRef> scaleDragImage(RetainPtr<CGImageRef>, FloatSize)
{
    return nullptr;
}

RetainPtr<CGImageRef> createDragImageFromImage(Image*, ImageOrientationDescription)
{
    return nullptr;
}

#endif

} // namespace WebCore

#endif // PLATFORM(IOS)
