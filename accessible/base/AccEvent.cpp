/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AccEvent.h"

#include "nsAccUtils.h"
#include "DocAccessible.h"
#include "xpcAccEvents.h"
#include "States.h"
#include "xpcAccessibleDocument.h"

#include "mozilla/EventStateManager.h"
#include "mozilla/dom/Selection.h"

using namespace mozilla;
using namespace mozilla::a11y;

static_assert(static_cast<bool>(eNoUserInput) == false &&
              static_cast<bool>(eFromUserInput) == true,
              "EIsFromUserInput cannot be casted to bool");

////////////////////////////////////////////////////////////////////////////////
// AccEvent
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// AccEvent constructors

AccEvent::AccEvent(uint32_t aEventType, Accessible* aAccessible,
                   EIsFromUserInput aIsFromUserInput, EEventRule aEventRule) :
  mEventType(aEventType), mEventRule(aEventRule), mAccessible(aAccessible)
{
  if (aIsFromUserInput == eAutoDetect)
    mIsFromUserInput = EventStateManager::IsHandlingUserInput();
  else
    mIsFromUserInput = aIsFromUserInput == eFromUserInput ? true : false;
}

////////////////////////////////////////////////////////////////////////////////
// AccEvent cycle collection

NS_IMPL_CYCLE_COLLECTION(AccEvent, mAccessible)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AccEvent, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AccEvent, Release)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// AccTextChangeEvent
////////////////////////////////////////////////////////////////////////////////

// Note: we pass in eAllowDupes to the base class because we don't support text
// events coalescence. We fire delayed text change events in DocAccessible but
// we continue to base the event off the accessible object rather than just the
// node. This means we won't try to create an accessible based on the node when
// we are ready to fire the event and so we will no longer assert at that point
// if the node was removed from the document. Either way, the AT won't work with
// a defunct accessible so the behaviour should be equivalent.
AccTextChangeEvent::
  AccTextChangeEvent(Accessible* aAccessible, int32_t aStart,
                     const nsAString& aModifiedText, bool aIsInserted,
                     EIsFromUserInput aIsFromUserInput)
  : AccEvent(aIsInserted ?
             static_cast<uint32_t>(nsIAccessibleEvent::EVENT_TEXT_INSERTED) :
             static_cast<uint32_t>(nsIAccessibleEvent::EVENT_TEXT_REMOVED),
             aAccessible, aIsFromUserInput, eAllowDupes)
  , mStart(aStart)
  , mIsInserted(aIsInserted)
  , mModifiedText(aModifiedText)
{
  // XXX We should use IsFromUserInput here, but that isn't always correct
  // when the text change isn't related to content insertion or removal.
   mIsFromUserInput = mAccessible->State() &
    (states::FOCUSED | states::EDITABLE);
}


////////////////////////////////////////////////////////////////////////////////
// AccReorderEvent
////////////////////////////////////////////////////////////////////////////////

uint32_t
AccReorderEvent::IsShowHideEventTarget(const Accessible* aTarget) const
{
  uint32_t count = mDependentEvents.Length();
  for (uint32_t index = count - 1; index < count; index--) {
    if (mDependentEvents[index]->mAccessible == aTarget) {
      uint32_t eventType = mDependentEvents[index]->mEventType;
      if (eventType == nsIAccessibleEvent::EVENT_SHOW ||
          eventType == nsIAccessibleEvent::EVENT_HIDE) {
        return mDependentEvents[index]->mEventType;
      }
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// AccHideEvent
////////////////////////////////////////////////////////////////////////////////

AccHideEvent::
  AccHideEvent(Accessible* aTarget, bool aNeedsShutdown) :
  AccMutationEvent(::nsIAccessibleEvent::EVENT_HIDE, aTarget),
  mNeedsShutdown(aNeedsShutdown)
{
  mNextSibling = mAccessible->NextSibling();
  mPrevSibling = mAccessible->PrevSibling();
}


////////////////////////////////////////////////////////////////////////////////
// AccShowEvent
////////////////////////////////////////////////////////////////////////////////

AccShowEvent::
  AccShowEvent(Accessible* aTarget) :
  AccMutationEvent(::nsIAccessibleEvent::EVENT_SHOW, aTarget)
{
  int32_t idx = aTarget->IndexInParent();
  MOZ_ASSERT(idx >= 0);
  mInsertionIndex = idx;
}


////////////////////////////////////////////////////////////////////////////////
// AccTextSelChangeEvent
////////////////////////////////////////////////////////////////////////////////

AccTextSelChangeEvent::AccTextSelChangeEvent(HyperTextAccessible* aTarget,
                                             dom::Selection* aSelection,
                                             int32_t aReason) :
  AccEvent(nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED, aTarget,
           eAutoDetect, eCoalesceTextSelChange),
  mSel(aSelection), mReason(aReason) {}

AccTextSelChangeEvent::~AccTextSelChangeEvent() { }

bool
AccTextSelChangeEvent::IsCaretMoveOnly() const
{
  return mSel->RangeCount() == 1 && mSel->IsCollapsed() &&
    ((mReason & (nsISelectionListener::COLLAPSETOSTART_REASON |
                 nsISelectionListener::COLLAPSETOEND_REASON)) == 0);
}

////////////////////////////////////////////////////////////////////////////////
// AccSelChangeEvent
////////////////////////////////////////////////////////////////////////////////

AccSelChangeEvent::
  AccSelChangeEvent(Accessible* aWidget, Accessible* aItem,
                    SelChangeType aSelChangeType) :
    AccEvent(0, aItem, eAutoDetect, eCoalesceSelectionChange),
    mWidget(aWidget), mItem(aItem), mSelChangeType(aSelChangeType),
    mPreceedingCount(0), mPackedEvent(nullptr)
{
  if (aSelChangeType == eSelectionAdd) {
    if (mWidget->GetSelectedItem(1))
      mEventType = nsIAccessibleEvent::EVENT_SELECTION_ADD;
    else
      mEventType = nsIAccessibleEvent::EVENT_SELECTION;
  } else {
    mEventType = nsIAccessibleEvent::EVENT_SELECTION_REMOVE;
  }
}


////////////////////////////////////////////////////////////////////////////////
// AccTableChangeEvent
////////////////////////////////////////////////////////////////////////////////

AccTableChangeEvent::
  AccTableChangeEvent(Accessible* aAccessible, uint32_t aEventType,
                      int32_t aRowOrColIndex, int32_t aNumRowsOrCols) :
  AccEvent(aEventType, aAccessible),
  mRowOrColIndex(aRowOrColIndex), mNumRowsOrCols(aNumRowsOrCols)
{
}


////////////////////////////////////////////////////////////////////////////////
// AccVCChangeEvent
////////////////////////////////////////////////////////////////////////////////

AccVCChangeEvent::
  AccVCChangeEvent(Accessible* aAccessible,
                   Accessible* aOldAccessible,
                   int32_t aOldStart, int32_t aOldEnd,
                   int16_t aReason, EIsFromUserInput aIsFromUserInput) :
    AccEvent(::nsIAccessibleEvent::EVENT_VIRTUALCURSOR_CHANGED, aAccessible,
             aIsFromUserInput),
    mOldAccessible(aOldAccessible), mOldStart(aOldStart), mOldEnd(aOldEnd),
    mReason(aReason)
{
}

already_AddRefed<nsIAccessibleEvent>
a11y::MakeXPCEvent(AccEvent* aEvent)
{
  DocAccessible* doc = aEvent->GetDocAccessible();
  Accessible* acc = aEvent->GetAccessible();
  nsINode* node = acc->GetNode();
  nsIDOMNode* domNode = node ? node->AsDOMNode() : nullptr;
  bool fromUser = aEvent->IsFromUserInput();
  uint32_t type = aEvent->GetEventType();
  uint32_t eventGroup = aEvent->GetEventGroups();
  nsCOMPtr<nsIAccessibleEvent> xpEvent;

  if (eventGroup & (1 << AccEvent::eStateChangeEvent)) {
    AccStateChangeEvent* sc = downcast_accEvent(aEvent);
    bool extra = false;
    uint32_t state = nsAccUtils::To32States(sc->GetState(), &extra);
    xpEvent = new xpcAccStateChangeEvent(type, ToXPC(acc), ToXPCDocument(doc),
                                         domNode, fromUser,
                                         state, extra, sc->IsStateEnabled());
    return xpEvent.forget();
  }

  if (eventGroup & (1 << AccEvent::eTextChangeEvent)) {
    AccTextChangeEvent* tc = downcast_accEvent(aEvent);
    nsString text;
    tc->GetModifiedText(text);
    xpEvent = new xpcAccTextChangeEvent(type, ToXPC(acc), ToXPCDocument(doc),
                                        domNode, fromUser,
                                        tc->GetStartOffset(), tc->GetLength(),
                                        tc->IsTextInserted(), text);
    return xpEvent.forget();
  }

  if (eventGroup & (1 << AccEvent::eHideEvent)) {
    AccHideEvent* hideEvent = downcast_accEvent(aEvent);
    xpEvent = new xpcAccHideEvent(type, ToXPC(acc), ToXPCDocument(doc),
                                  domNode, fromUser,
                                  ToXPC(hideEvent->TargetParent()),
                                  ToXPC(hideEvent->TargetNextSibling()),
                                  ToXPC(hideEvent->TargetPrevSibling()));
    return xpEvent.forget();
  }

  if (eventGroup & (1 << AccEvent::eCaretMoveEvent)) {
    AccCaretMoveEvent* cm = downcast_accEvent(aEvent);
    xpEvent = new xpcAccCaretMoveEvent(type, ToXPC(acc), ToXPCDocument(doc),
                                       domNode, fromUser,
                                       cm->GetCaretOffset());
    return xpEvent.forget();
  }

  if (eventGroup & (1 << AccEvent::eVirtualCursorChangeEvent)) {
    AccVCChangeEvent* vcc = downcast_accEvent(aEvent);
    xpEvent = new xpcAccVirtualCursorChangeEvent(type,
                                                 ToXPC(acc), ToXPCDocument(doc),
                                                 domNode, fromUser,
                                                 ToXPC(vcc->OldAccessible()),
                                                 vcc->OldStartOffset(),
                                                 vcc->OldEndOffset(),
                                                 vcc->Reason());
    return xpEvent.forget();
  }

  if (eventGroup & (1 << AccEvent::eObjectAttrChangedEvent)) {
    AccObjectAttrChangedEvent* oac = downcast_accEvent(aEvent);
    xpEvent = new xpcAccObjectAttributeChangedEvent(type,
                                                    ToXPC(acc),
                                                    ToXPCDocument(doc), domNode,
                                                    fromUser,
                                                    oac->GetAttribute());
    return xpEvent.forget();
  }

  xpEvent = new xpcAccEvent(type, ToXPC(acc), ToXPCDocument(doc), domNode, fromUser);
  return xpEvent.forget();
  }
