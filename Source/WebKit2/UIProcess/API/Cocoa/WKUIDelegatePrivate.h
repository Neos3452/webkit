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

#import <WebKit/WKUIDelegate.h>

#if WK_API_ENABLED

#import <WebKit/WKDragDestinationAction.h>
#import <WebKit/WKSecurityOrigin.h>
#import <WebKit/WKWebViewPrivate.h>
#import <WebKit/_WKActivatedElementInfo.h>

@class UIScrollView;
@class UIViewController;
@class WKFrameInfo;
@class _WKContextMenuElementInfo;
@class _WKActivatedElementInfo;
@class _WKElementAction;
@class _WKFrameHandle;

#if TARGET_OS_IOS
@class UIDragItem;
@class UITargetedDragPreview;
@protocol UIDragSession;
@protocol UIDropSession;
#endif

@protocol WKUIDelegatePrivate <WKUIDelegate>

struct UIEdgeInsets;

@optional

// FIXME: This should be handled by the WKWebsiteDataStore delegate.
- (void)_webView:(WKWebView *)webView decideDatabaseQuotaForSecurityOrigin:(WKSecurityOrigin *)securityOrigin currentQuota:(unsigned long long)currentQuota currentOriginUsage:(unsigned long long)currentOriginUsage currentDatabaseUsage:(unsigned long long)currentUsage expectedUsage:(unsigned long long)expectedUsage decisionHandler:(void (^)(unsigned long long newQuota))decisionHandler;

// FIXME: This should be handled by the WKWebsiteDataStore delegate.
- (void)_webView:(WKWebView *)webView decideWebApplicationCacheQuotaForSecurityOrigin:(WKSecurityOrigin *)securityOrigin currentQuota:(unsigned long long)currentQuota totalBytesNeeded:(unsigned long long)totalBytesNeeded decisionHandler:(void (^)(unsigned long long newQuota))decisionHandler;

- (void)_webView:(WKWebView *)webView printFrame:(_WKFrameHandle *)frame;

- (void)_webViewClose:(WKWebView *)webView;
- (void)_webViewFullscreenMayReturnToInline:(WKWebView *)webView;
- (void)_webViewDidEnterFullscreen:(WKWebView *)webView WK_API_AVAILABLE(macosx(10.11), ios(8.3));
- (void)_webViewDidExitFullscreen:(WKWebView *)webView WK_API_AVAILABLE(macosx(10.11), ios(8.3));
- (void)_webViewRequestPointerLock:(WKWebView *)webView WK_API_AVAILABLE(macosx(10.12.3));
- (void)_webViewDidLosePointerLock:(WKWebView *)webView WK_API_AVAILABLE(macosx(10.12.3));
- (void)_webView:(WKWebView *)webView hasVideoInPictureInPictureDidChange:(BOOL)hasVideoInPictureInPicture WK_API_AVAILABLE(macosx(WK_MAC_TBA), ios(WK_IOS_TBA));

- (void)_webView:(WKWebView *)webView imageOrMediaDocumentSizeChanged:(CGSize)size WK_API_AVAILABLE(macosx(10.12), ios(10.0));
- (NSDictionary *)_dataDetectionContextForWebView:(WKWebView *)webView WK_API_AVAILABLE(macosx(10.12), ios(10.0));
- (void)_webView:(WKWebView *)webView requestUserMediaAuthorizationForDevices:(_WKCaptureDevices)devices url:(NSURL *)url mainFrameURL:(NSURL *)mainFrameURL decisionHandler:(void (^)(BOOL authorized))decisionHandler WK_API_AVAILABLE(macosx(WK_MAC_TBA), ios(WK_IOS_TBA));
- (void)_webView:(WKWebView *)webView checkUserMediaPermissionForURL:(NSURL *)url mainFrameURL:(NSURL *)mainFrameURL frameIdentifier:(NSUInteger)frameIdentifier decisionHandler:(void (^)(NSString *salt, BOOL authorized))decisionHandler WK_API_AVAILABLE(macosx(10.12.3), ios(10.3));
- (void)_webView:(WKWebView *)webView mediaCaptureStateDidChange:(_WKMediaCaptureState)state WK_API_AVAILABLE(macosx(WK_MAC_TBA), ios(WK_IOS_TBA));
- (WKDragDestinationAction)_webView:(WKWebView *)webView dragDestinationActionMaskForDraggingInfo:(id)draggingInfo WK_API_AVAILABLE(macosx(WK_MAC_TBA), ios(WK_IOS_TBA));
- (void)_webView:(WKWebView *)webView createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration forNavigationAction:(WKNavigationAction *)navigationAction windowFeatures:(WKWindowFeatures *)windowFeatures completionHandler:(void (^)(WKWebView *webView))completionHandler WK_API_AVAILABLE(macosx(WK_MAC_TBA), ios(WK_IOS_TBA));

- (void)_webView:(WKWebView *)webView runBeforeUnloadConfirmPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(BOOL result))completionHandler WK_API_AVAILABLE(macosx(WK_MAC_TBA), ios(WK_IOS_TBA));

#if TARGET_OS_IPHONE
- (BOOL)_webView:(WKWebView *)webView shouldIncludeAppLinkActionsForElement:(_WKActivatedElementInfo *)element WK_API_AVAILABLE(ios(9.0));
- (NSArray *)_webView:(WKWebView *)webView actionsForElement:(_WKActivatedElementInfo *)element defaultActions:(NSArray<_WKElementAction *> *)defaultActions;
- (void)_webView:(WKWebView *)webView didNotHandleTapAsClickAtPoint:(CGPoint)point;
- (BOOL)_webView:(WKWebView *)webView shouldRequestGeolocationAuthorizationForURL:(NSURL *)url isMainFrame:(BOOL)isMainFrame mainFrameURL:(NSURL *)mainFrameURL;
- (void)_webView:(WKWebView *)webView requestGeolocationAuthorizationForURL:(NSURL *)url frame:(WKFrameInfo *)frame decisionHandler:(void (^)(BOOL authorized))decisionHandler WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (UIViewController *)_webView:(WKWebView *)webView previewViewControllerForURL:(NSURL *)url WK_API_AVAILABLE(ios(9.0));
- (void)_webView:(WKWebView *)webView commitPreviewedViewController:(UIViewController *)previewedViewController WK_API_AVAILABLE(ios(9.0));
- (void)_webView:(WKWebView *)webView willPreviewImageWithURL:(NSURL *)imageURL WK_API_AVAILABLE(ios(9.0));
- (void)_webView:(WKWebView *)webView commitPreviewedImageWithURL:(NSURL *)imageURL WK_API_AVAILABLE(ios(9.0));
- (void)_webView:(WKWebView *)webView didDismissPreviewViewController:(UIViewController *)previewedViewController committing:(BOOL)committing WK_API_AVAILABLE(ios(9.0));
- (void)_webView:(WKWebView *)webView didDismissPreviewViewController:(UIViewController *)previewedViewController WK_API_AVAILABLE(ios(9.0));
- (BOOL)_webView:(WKWebView *)webView showCustomSheetForElement:(_WKActivatedElementInfo *)element WK_API_AVAILABLE(ios(10.0));
- (void)_webView:(WKWebView *)webView alternateActionForURL:(NSURL *)url WK_API_AVAILABLE(ios(10.0));
- (NSArray *)_attachmentListForWebView:(WKWebView *)webView WK_API_AVAILABLE(ios(10.0));
- (NSArray *)_attachmentListForWebView:(WKWebView *)webView sourceIsManaged:(BOOL*)sourceIsManaged WK_API_AVAILABLE(ios(10.3));
- (NSUInteger)_webView:(WKWebView *)webView indexIntoAttachmentListForElement:(_WKActivatedElementInfo *)element WK_API_AVAILABLE(ios(10.3));
- (UIEdgeInsets)_webView:(WKWebView *)webView finalObscuredInsetsForScrollView:(UIScrollView *)scrollView withVelocity:(CGPoint)velocity targetContentOffset:(inout CGPoint *)targetContentOffset WK_API_AVAILABLE(ios(9.0));
- (UIViewController *)_webView:(WKWebView *)webView previewViewControllerForURL:(NSURL *)url defaultActions:(NSArray<_WKElementAction *> *)actions elementInfo:(_WKActivatedElementInfo *)elementInfo WK_API_AVAILABLE(ios(9.0));
- (UIViewController *)_webView:(WKWebView *)webView previewViewControllerForAnimatedImageAtURL:(NSURL *)url defaultActions:(NSArray<_WKElementAction *> *)actions elementInfo:(_WKActivatedElementInfo *)elementInfo imageSize:(CGSize)imageSize WK_API_AVAILABLE(ios(9.0));
- (UIViewController *)_presentingViewControllerForWebView:(WKWebView *)webView WK_API_AVAILABLE(ios(10.0));
- (void)_webView:(WKWebView *)webView getAlternateURLFromImage:(UIImage *)image completionHandler:(void (^)(NSURL *alternateURL, NSDictionary *userInfo))completionHandler WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (NSURL *)_webView:(WKWebView *)webView alternateURLFromImage:(UIImage *)image userInfo:(NSDictionary **)userInfo WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (UIViewController *)_webView:(WKWebView *)webView previewViewControllerForImage:(UIImage *)image alternateURL:(NSURL *)url defaultActions:(NSArray<_WKElementAction *> *)actions elementInfo:(_WKActivatedElementInfo *)elementInfo WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (NSArray *)_webView:(WKWebView *)webView adjustedDataInteractionItemProviders:(NSArray *)originalItemProviders WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (NSArray *)_webView:(WKWebView *)webView adjustedDataInteractionItemProvidersForItemProvider:(id)itemProvider representingObjects:(NSArray *)representingObjects additionalData:(NSDictionary *)additionalData WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (BOOL)_webView:(WKWebView *)webView performDataInteractionOperationWithItemProviders:(NSArray *)itemProviders WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (void)_webView:(WKWebView *)webView dataInteraction:(id)interaction sessionWillBegin:(id)session WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (void)_webView:(WKWebView *)webView dataInteraction:(id)interaction session:(id)session didEndWithOperation:(NSUInteger)operation WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (void)_webView:(WKWebView *)webView dataInteractionOperationWasHandled:(BOOL)handled forSession:(id)session itemProviders:(NSArray *)itemProviders WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (NSUInteger)_webView:(WKWebView *)webView willUpdateDataInteractionOperationToOperation:(NSUInteger)operation forSession:(id)session WK_API_AVAILABLE(ios(WK_IOS_TBA));
#if TARGET_OS_IOS
- (UITargetedDragPreview *)_webView:(WKWebView *)webView previewForLiftingItem:(UIDragItem *)item session:(id <UIDragSession>)session WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (UITargetedDragPreview *)_webView:(WKWebView *)webView previewForCancellingItem:(UIDragItem *)item withDefault:(UITargetedDragPreview *)defaultPreview WK_API_AVAILABLE(ios(WK_IOS_TBA));
- (NSArray<UIDragItem *> *)_webView:(WKWebView *)webView willPerformDropWithSession:(id <UIDropSession>)session WK_API_AVAILABLE(ios(WK_IOS_TBA));
#endif
- (void)_webView:(WKWebView *)webView didChangeSafeAreaShouldAffectObscuredInsets:(BOOL)safeAreaShouldAffectObscuredInsets WK_API_AVAILABLE(ios(WK_IOS_TBA));
#else
- (NSMenu *)_webView:(WKWebView *)webView contextMenu:(NSMenu *)menu forElement:(_WKContextMenuElementInfo *)element WK_API_AVAILABLE(macosx(10.12));
- (NSMenu *)_webView:(WKWebView *)webView contextMenu:(NSMenu *)menu forElement:(_WKContextMenuElementInfo *)element userInfo:(id <NSSecureCoding>)userInfo WK_API_AVAILABLE(macosx(10.12));
#endif

@end

#endif // WK_API_ENABLED
