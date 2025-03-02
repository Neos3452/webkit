/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#pragma once

#include "AXTextStateChangeIntent.h"
#include "EditingStyle.h"
#include "Element.h"
#include "IntRect.h"
#include "LayoutRect.h"
#include "Range.h"
#include "ScrollAlignment.h"
#include "Timer.h"
#include "VisibleSelection.h"
#include <wtf/Noncopyable.h>

#if PLATFORM(IOS)
#include "Color.h"
#endif

namespace WebCore {

class CharacterData;
class Frame;
class GraphicsContext;
class HTMLFormElement;
class MutableStyleProperties;
class RenderBlock;
class RenderObject;
class RenderView;
class VisiblePosition;

enum EUserTriggered { NotUserTriggered = 0, UserTriggered = 1 };

enum RevealExtentOption {
    RevealExtent,
    DoNotRevealExtent
};

class CaretBase {
    WTF_MAKE_NONCOPYABLE(CaretBase);
    WTF_MAKE_FAST_ALLOCATED;
protected:
    enum CaretVisibility { Visible, Hidden };
    explicit CaretBase(CaretVisibility = Hidden);

    void invalidateCaretRect(Node*, bool caretRectChanged = false);
    void clearCaretRect();
    bool updateCaretRect(Document*, const VisiblePosition& caretPosition);
    bool shouldRepaintCaret(const RenderView*, bool isContentEditable) const;
    void paintCaret(Node*, GraphicsContext&, const LayoutPoint&, const LayoutRect& clipRect) const;

    const LayoutRect& localCaretRectWithoutUpdate() const { return m_caretLocalRect; }

    bool shouldUpdateCaretRect() const { return m_caretRectNeedsUpdate; }
    void setCaretRectNeedsUpdate() { m_caretRectNeedsUpdate = true; }

    void setCaretVisibility(CaretVisibility visibility) { m_caretVisibility = visibility; }
    bool caretIsVisible() const { return m_caretVisibility == Visible; }
    CaretVisibility caretVisibility() const { return m_caretVisibility; }

private:
    LayoutRect m_caretLocalRect; // caret rect in coords local to the renderer responsible for painting the caret
    bool m_caretRectNeedsUpdate; // true if m_caretRect (and m_absCaretBounds in FrameSelection) need to be calculated
    CaretVisibility m_caretVisibility;
};

class DragCaretController : private CaretBase {
    WTF_MAKE_NONCOPYABLE(DragCaretController);
    WTF_MAKE_FAST_ALLOCATED;
public:
    DragCaretController();

    RenderBlock* caretRenderer() const;
    void paintDragCaret(Frame*, GraphicsContext&, const LayoutPoint&, const LayoutRect& clipRect) const;

    bool isContentEditable() const { return m_position.rootEditableElement(); }
    WEBCORE_EXPORT bool isContentRichlyEditable() const;

    bool hasCaret() const { return m_position.isNotNull(); }
    const VisiblePosition& caretPosition() { return m_position; }
    void setCaretPosition(const VisiblePosition&);
    void clear() { setCaretPosition(VisiblePosition()); }
    WEBCORE_EXPORT IntRect caretRectInRootViewCoordinates() const;

    void nodeWillBeRemoved(Node&);

private:
    VisiblePosition m_position;
};

class FrameSelection : private CaretBase {
    WTF_MAKE_NONCOPYABLE(FrameSelection);
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum EAlteration { AlterationMove, AlterationExtend };
    enum CursorAlignOnScroll { AlignCursorOnScrollIfNeeded,
                               AlignCursorOnScrollAlways };
    enum SetSelectionOption {
        FireSelectEvent = 1 << 0,
        CloseTyping = 1 << 1,
        ClearTypingStyle = 1 << 2,
        SpellCorrectionTriggered = 1 << 3,
        DoNotSetFocus = 1 << 4,
        DictationTriggered = 1 << 5,
        RevealSelection = 1 << 6,
    };
    typedef unsigned SetSelectionOptions; // Union of values in SetSelectionOption and EUserTriggered
    static inline SetSelectionOptions defaultSetSelectionOptions(EUserTriggered userTriggered = NotUserTriggered)
    {
        return CloseTyping | ClearTypingStyle | (userTriggered ? (RevealSelection | FireSelectEvent) : 0);
    }

    WEBCORE_EXPORT explicit FrameSelection(Frame* = nullptr);

    WEBCORE_EXPORT Element* rootEditableElementOrDocumentElement() const;
     
    WEBCORE_EXPORT void moveTo(const Range*);
    WEBCORE_EXPORT void moveTo(const VisiblePosition&, EUserTriggered = NotUserTriggered, CursorAlignOnScroll = AlignCursorOnScrollIfNeeded);
    WEBCORE_EXPORT void moveTo(const VisiblePosition&, const VisiblePosition&, EUserTriggered = NotUserTriggered);
    void moveTo(const Position&, EAffinity, EUserTriggered = NotUserTriggered);
    void moveTo(const Position&, const Position&, EAffinity, EUserTriggered = NotUserTriggered);
    void moveWithoutValidationTo(const Position&, const Position&, bool selectionHasDirection, bool shouldSetFocus, const AXTextStateChangeIntent& = AXTextStateChangeIntent());

    const VisibleSelection& selection() const { return m_selection; }
    WEBCORE_EXPORT void setSelection(const VisibleSelection&, SetSelectionOptions = defaultSetSelectionOptions(), AXTextStateChangeIntent = AXTextStateChangeIntent(), CursorAlignOnScroll = AlignCursorOnScrollIfNeeded, TextGranularity = CharacterGranularity);
    WEBCORE_EXPORT bool setSelectedRange(Range*, EAffinity, bool closeTyping, EUserTriggered = NotUserTriggered);
    WEBCORE_EXPORT void selectAll();
    WEBCORE_EXPORT void clear();
    void prepareForDestruction();

    void updateAppearanceAfterLayout();
    void scheduleAppearanceUpdateAfterStyleChange();
    void setNeedsSelectionUpdate();

    bool contains(const LayoutPoint&) const;

    WEBCORE_EXPORT bool modify(EAlteration, SelectionDirection, TextGranularity, EUserTriggered = NotUserTriggered);
    enum VerticalDirection { DirectionUp, DirectionDown };
    bool modify(EAlteration, unsigned verticalDistance, VerticalDirection, EUserTriggered = NotUserTriggered, CursorAlignOnScroll = AlignCursorOnScrollIfNeeded);

    TextGranularity granularity() const { return m_granularity; }

    void setStart(const VisiblePosition &, EUserTriggered = NotUserTriggered);
    void setEnd(const VisiblePosition &, EUserTriggered = NotUserTriggered);
    
    void setBase(const VisiblePosition&, EUserTriggered = NotUserTriggered);
    void setBase(const Position&, EAffinity, EUserTriggered = NotUserTriggered);
    void setExtent(const VisiblePosition&, EUserTriggered = NotUserTriggered);
    void setExtent(const Position&, EAffinity, EUserTriggered = NotUserTriggered);

    // Return the renderer that is responsible for painting the caret (in the selection start node)
    RenderBlock* caretRendererWithoutUpdatingLayout() const;

    // Bounds of (possibly transformed) caret in absolute coords
    WEBCORE_EXPORT IntRect absoluteCaretBounds(bool* insideFixed = nullptr);
    void setCaretRectNeedsUpdate() { CaretBase::setCaretRectNeedsUpdate(); }

    void willBeModified(EAlteration, SelectionDirection);

    bool isNone() const { return m_selection.isNone(); }
    bool isCaret() const { return m_selection.isCaret(); }
    bool isRange() const { return m_selection.isRange(); }
    bool isCaretOrRange() const { return m_selection.isCaretOrRange(); }
    bool isAll(EditingBoundaryCrossingRule rule = CannotCrossEditingBoundary) const { return m_selection.isAll(rule); }
    
    RefPtr<Range> toNormalizedRange() const { return m_selection.toNormalizedRange(); }

    void debugRenderer(RenderObject*, bool selected) const;

    void nodeWillBeRemoved(Node&);
    void textWasReplaced(CharacterData*, unsigned offset, unsigned oldLength, unsigned newLength);

    void setCaretVisible(bool caretIsVisible) { setCaretVisibility(caretIsVisible ? Visible : Hidden); }
    void paintCaret(GraphicsContext&, const LayoutPoint&, const LayoutRect& clipRect);

    // Used to suspend caret blinking while the mouse is down.
    void setCaretBlinkingSuspended(bool suspended) { m_isCaretBlinkingSuspended = suspended; }
    bool isCaretBlinkingSuspended() const { return m_isCaretBlinkingSuspended; }

    // Focus
    void setFocused(bool);
    bool isFocused() const { return m_focused; }
    WEBCORE_EXPORT bool isFocusedAndActive() const;
    void pageActivationChanged();

    // Painting.
    WEBCORE_EXPORT void updateAppearance();

#if ENABLE(TREE_DEBUGGING)
    void formatForDebugger(char* buffer, unsigned length) const;
    void showTreeForThis() const;
#endif

#if PLATFORM(IOS)
public:
    WEBCORE_EXPORT void expandSelectionToElementContainingCaretSelection();
    WEBCORE_EXPORT RefPtr<Range> elementRangeContainingCaretSelection() const;
    WEBCORE_EXPORT void expandSelectionToWordContainingCaretSelection();
    WEBCORE_EXPORT RefPtr<Range> wordRangeContainingCaretSelection();
    WEBCORE_EXPORT void expandSelectionToStartOfWordContainingCaretSelection();
    WEBCORE_EXPORT UChar characterInRelationToCaretSelection(int amount) const;
    WEBCORE_EXPORT UChar characterBeforeCaretSelection() const;
    WEBCORE_EXPORT UChar characterAfterCaretSelection() const;
    WEBCORE_EXPORT int wordOffsetInRange(const Range*) const;
    WEBCORE_EXPORT bool spaceFollowsWordInRange(const Range*) const;
    WEBCORE_EXPORT bool selectionAtDocumentStart() const;
    WEBCORE_EXPORT bool selectionAtSentenceStart() const;
    WEBCORE_EXPORT bool selectionAtWordStart() const;
    WEBCORE_EXPORT RefPtr<Range> rangeByMovingCurrentSelection(int amount) const;
    WEBCORE_EXPORT RefPtr<Range> rangeByExtendingCurrentSelection(int amount) const;
    WEBCORE_EXPORT void selectRangeOnElement(unsigned location, unsigned length, Node&);
    WEBCORE_EXPORT void clearCurrentSelection();
    void setCaretBlinks(bool caretBlinks = true);
    WEBCORE_EXPORT void setCaretColor(const Color&);
    WEBCORE_EXPORT static VisibleSelection wordSelectionContainingCaretSelection(const VisibleSelection&);
    void setUpdateAppearanceEnabled(bool enabled) { m_updateAppearanceEnabled = enabled; }
    void suppressScrolling() { ++m_scrollingSuppressCount; }
    void restoreScrolling()
    {
        ASSERT(m_scrollingSuppressCount);
        --m_scrollingSuppressCount;
    }
private:
    bool actualSelectionAtSentenceStart(const VisibleSelection&) const;
    RefPtr<Range> rangeByAlteringCurrentSelection(EAlteration, int amount) const;
public:
#endif

    bool shouldChangeSelection(const VisibleSelection&) const;
    bool shouldDeleteSelection(const VisibleSelection&) const;
    enum EndPointsAdjustmentMode { AdjustEndpointsAtBidiBoundary, DoNotAdjsutEndpoints };
    void setSelectionByMouseIfDifferent(const VisibleSelection&, TextGranularity, EndPointsAdjustmentMode = DoNotAdjsutEndpoints);

    EditingStyle* typingStyle() const;
    WEBCORE_EXPORT RefPtr<MutableStyleProperties> copyTypingStyle() const;
    void setTypingStyle(RefPtr<EditingStyle>&& style) { m_typingStyle = WTFMove(style); }
    void clearTypingStyle();

    WEBCORE_EXPORT FloatRect selectionBounds(bool clipToVisibleContent = true) const;

    enum class TextRectangleHeight { TextHeight, SelectionHeight };
    WEBCORE_EXPORT void getClippedVisibleTextRectangles(Vector<FloatRect>&, TextRectangleHeight = TextRectangleHeight::SelectionHeight) const;
    WEBCORE_EXPORT void getTextRectangles(Vector<FloatRect>&, TextRectangleHeight = TextRectangleHeight::SelectionHeight) const;

    WEBCORE_EXPORT HTMLFormElement* currentForm() const;

    WEBCORE_EXPORT void revealSelection(SelectionRevealMode = SelectionRevealMode::Reveal, const ScrollAlignment& = ScrollAlignment::alignCenterIfNeeded, RevealExtentOption = DoNotRevealExtent);
    WEBCORE_EXPORT void setSelectionFromNone();

    bool shouldShowBlockCursor() const { return m_shouldShowBlockCursor; }
    void setShouldShowBlockCursor(bool);

private:
    enum EPositionType { START, END, BASE, EXTENT };

    void updateAndRevealSelection(const AXTextStateChangeIntent&);
    void updateDataDetectorsForSelection();

    bool setSelectionWithoutUpdatingAppearance(const VisibleSelection&, SetSelectionOptions, CursorAlignOnScroll, TextGranularity);

    void respondToNodeModification(Node&, bool baseRemoved, bool extentRemoved, bool startRemoved, bool endRemoved);
    TextDirection directionOfEnclosingBlock();
    TextDirection directionOfSelection();

    VisiblePosition positionForPlatform(bool isGetStart) const;
    VisiblePosition startForPlatform() const;
    VisiblePosition endForPlatform() const;
    VisiblePosition nextWordPositionForPlatform(const VisiblePosition&);

    VisiblePosition modifyExtendingRight(TextGranularity);
    VisiblePosition modifyExtendingForward(TextGranularity);
    VisiblePosition modifyMovingRight(TextGranularity, bool* reachedBoundary = nullptr);
    VisiblePosition modifyMovingForward(TextGranularity, bool* reachedBoundary = nullptr);
    VisiblePosition modifyExtendingLeft(TextGranularity);
    VisiblePosition modifyExtendingBackward(TextGranularity);
    VisiblePosition modifyMovingLeft(TextGranularity, bool* reachedBoundary = nullptr);
    VisiblePosition modifyMovingBackward(TextGranularity, bool* reachedBoundary = nullptr);

    LayoutUnit lineDirectionPointForBlockDirectionNavigation(EPositionType);

    AXTextStateChangeIntent textSelectionIntent(EAlteration, SelectionDirection, TextGranularity);
#if HAVE(ACCESSIBILITY)
    void notifyAccessibilityForSelectionChange(const AXTextStateChangeIntent&);
#else
    void notifyAccessibilityForSelectionChange(const AXTextStateChangeIntent&) { }
#endif

    void updateSelectionCachesIfSelectionIsInsideTextFormControl(EUserTriggered);

    void selectFrameElementInParentIfFullySelected();

    void setFocusedElementIfNeeded();
    void focusedOrActiveStateChanged();

    void caretBlinkTimerFired();

    void updateAppearanceAfterLayoutOrStyleChange();
    void appearanceUpdateTimerFired();

    void setCaretVisibility(CaretVisibility);
    bool recomputeCaretRect();
    void invalidateCaretRect();

    bool dispatchSelectStart();

    Frame* m_frame;

    LayoutUnit m_xPosForVerticalArrowNavigation;

    VisibleSelection m_selection;
    VisiblePosition m_originalBase; // Used to store base before the adjustment at bidi boundary
    TextGranularity m_granularity;

    RefPtr<Node> m_previousCaretNode; // The last node which painted the caret. Retained for clearing the old caret when it moves.

    RefPtr<EditingStyle> m_typingStyle;

    Timer m_caretBlinkTimer;
    Timer m_appearanceUpdateTimer;
    // The painted bounds of the caret in absolute coordinates
    IntRect m_absCaretBounds;
    bool m_caretInsidePositionFixed : 1;
    bool m_absCaretBoundsDirty : 1;
    bool m_caretPaint : 1;
    bool m_isCaretBlinkingSuspended : 1;
    bool m_focused : 1;
    bool m_shouldShowBlockCursor : 1;
    bool m_pendingSelectionUpdate : 1;
    bool m_shouldRevealSelection : 1;
    bool m_alwaysAlignCursorOnScrollWhenRevealingSelection : 1;

#if PLATFORM(IOS)
    bool m_updateAppearanceEnabled : 1;
    bool m_caretBlinks : 1;
    Color m_caretColor;
    int m_scrollingSuppressCount { 0 };
#endif
};

inline EditingStyle* FrameSelection::typingStyle() const
{
    return m_typingStyle.get();
}

inline void FrameSelection::clearTypingStyle()
{
    m_typingStyle = nullptr;
}

#if !(PLATFORM(COCOA) || PLATFORM(GTK))
#if HAVE(ACCESSIBILITY)
inline void FrameSelection::notifyAccessibilityForSelectionChange(const AXTextStateChangeIntent&)
{
}
#endif
#endif

} // namespace WebCore

#if ENABLE(TREE_DEBUGGING)
// Outside the WebCore namespace for ease of invocation from the debugger.
void showTree(const WebCore::FrameSelection&);
void showTree(const WebCore::FrameSelection*);
#endif
