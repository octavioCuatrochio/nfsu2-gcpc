#include "UIWidgetMenu.h"
#include "bridge/Bridge.h"
#include "FEWidget.h"
#include "FEngUtil.h"
#include "FEHash.h"
#include <cmath>

namespace {
using L = long double;
constexpr size_t kNodeOffset = offsetof(FEWidget, _unknown04);   // PC 4
// node -> owning widget (PC: lea r,[node-4] when the node is non-null, else null); the head maps to this - 4 too
FEWidget *FromNode(const WidgetListNode *n)
{
    return n ? reinterpret_cast<FEWidget *>(reinterpret_cast<uintptr_t>(n) - kNodeOffset) : nullptr;
}
WidgetListNode *NextOf(FEWidget *w) { return *reinterpret_cast<WidgetListNode **>(reinterpret_cast<uint8_t *>(w) + kNodeOffset); }
}  // namespace

void DeleteAllNodeOwners(WidgetListNode *head)
{
    while (head->next != head) {
        WidgetListNode *n = head->next;
        n->prev->next = n->next;
        n->next->prev = n->prev;
        if (FEWidget *w = FromNode(n)) bridge::VirtualScalarDelete(w);
    }
}

WidgetListNode *ListGetNode(WidgetListNode *head, uint32_t index)
{
    WidgetListNode *n = head->next;
    for (uint32_t i = 0; n != head && i != index; ++i) n = n->next;
    return n;
}

UIWidgetMenu *UIWidgetMenu::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::UIWidgetMenuVTable();
    mWidgets.next = mWidgets.prev = &mWidgets;
    mpCurrent = mpViewTop = nullptr;
    mpTitleMaster = mpDataMaster = mpPrevObject = mpDoneButton = nullptr;
    mScrollBar.Construct(base.mpPackageName, "scrollbar", true, false, false);
    mOptionName = "OPTION_NAME_";
    mOptionData = "OPTION_DATA_";
    mOptionImage = "OPTION_IMAGE_";
    mOptionBacking = "OPTION_BACKING_";
    mLeftArrow = "LEFT_ARROW_";
    mRightArrow = "RIGHT_ARROW_";
    mSliderBase = "SLIDER_BASE_";
    mSliderKnob = "SLIDER_KNOB_";
    mSliderValue = "SLIDER_VALUE_";
    mSliderHandle = "SLIDER_HANDLE_";
    mSliderBeg = "SLIDER_BEG_";
    mSliderEnd = "SLIDER_END_";
    mTitleX = mTitleY = mStartX = mStartY = mDataOffsetX = mRowHeight = 0.0f;
    mTitleW = 175.0f; mTitleH = 24.0f;
    mDataW = 175.0f; mDataH = 24.0f;
    mDataX = mDataY = 0.0f;
    _unknown124 = 4.0f;
    mSpacing = 0.0f;
    mCurrentIndex = 1;
    _unknown130 = 1;
    mVisibleCount = 7;
    mPrevMsg = mPrevArg = mPrevPkg = 0;
    _unknown144 = 0; _unknown145 = 0; _unknown146 = 1;
    mbViewDirty = 0;
    _unknown148 = 1; _unknown149 = 1;
    mpTitleMaster = FEngFindObject(base.mpPackageName, FEHashUpper("TITLE_MASTER"));
    mpDataMaster = FEngFindObject(base.mpPackageName, FEHashUpper("DATA_MASTER"));
    mpDoneButton = FEngFindObject(base.mpPackageName, FEHashUpper("DONE_BUTTON"));
    if (mpTitleMaster && mpDataMaster) {
        // PC: hide the scroll bar parts (GC: FEScrollBar::Update(0, 0, 0))
        FEngSetInvisible(mScrollBar.mpHandle);
        FEngSetInvisible(mScrollBar.mpArrow1);
        FEngSetInvisible(mScrollBar.mpArrow2);
        SetInitialPositions();
        base._unknown2C = 0;
    }
    return this;
}

void UIWidgetMenu::DestructBody()
{
    base.vtable = bridge::UIWidgetMenuVTable();
    DeleteAllNodeOwners(&mWidgets);
    base.vtable = bridge::MenuScreenVTable();   // MenuScreen dtor body (PC 510D40)
}

UIWidgetMenu *UIWidgetMenu::ScalarDeletingDestructor(uint32_t flags)
{
    DestructBody();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

void UIWidgetMenu::RefreshWidgets()
{
    for (FEWidget *w = FromNode(mWidgets.next); w != FromNode(&mWidgets); w = FromNode(NextOf(w)))
        bridge::FEWidget_Draw(w);   // slot 3
}

void UIWidgetMenu::SetInitialPositions()
{
    FEngGetTopLeft(mpTitleMaster, &mTitleX, &mTitleY);
    mStartY = mTitleY;
    mStartX = mTitleX;
    FEngGetTopLeft(mpDataMaster, &mDataX, &mDataY);
    FEngGetSize(mpTitleMaster, &mTitleW, &mTitleH);
    FEngGetSize(mpDataMaster, &mDataW, &mDataH);
    mRowHeight = mTitleH;
    mDataOffsetX = static_cast<float>(std::fabs(L(mTitleX) - (L(mDataW) + mDataX)));
}

uint32_t UIWidgetMenu::GetWidgetIndex(const FEWidget *target) const
{
    uint32_t n = 1;
    for (FEWidget *w = FromNode(mWidgets.next); w != FromNode(&mWidgets); w = FromNode(NextOf(w)), ++n)
        if (target == w) return n;
    return 0;
}

void UIWidgetMenu::ClearWidgets()
{
    for (FEWidget *w = FromNode(mWidgets.next); w != FromNode(&mWidgets); w = FromNode(NextOf(w))) {
        bridge::FEWidget_Hide(w);         // slot 6
        bridge::FEWidget_UnsetFocus(w);   // slot 10
    }
    DeleteAllNodeOwners(&mWidgets);
    SetInitialPositions();
}

void UIWidgetMenu::Reposition()
{
    const uint32_t first = GetWidgetIndex(mpViewTop);
    float y = mTitleY;
    uint32_t n = 1;
    for (FEWidget *w = FromNode(mWidgets.next); w != FromNode(&mWidgets); w = FromNode(NextOf(w)), ++n) {
        if (n >= first && n < first + mVisibleCount) {
            bridge::FEWidget_Show(w);         // slot 5
            bridge::FEWidget_SetPosY(w, y);   // slot 13
            bridge::FEWidget_Position(w);     // slot 4
            y = static_cast<float>(L(y) + mRowHeight);
        } else {
            bridge::FEWidget_SetPosY(w, 6969.0f);   // parked off screen
            bridge::FEWidget_Hide(w);
        }
    }
}

void UIWidgetMenu::Reset()
{
    if (mWidgets.next == &mWidgets) return;
    FEWidget *w = FromNode(mWidgets.next);
    mpCurrent = w;
    mpViewTop = w;
    mbViewDirty = 0;
    bridge::FEWidget_SetFocus(w, base.mpPackageName);   // slot 9
    Reposition();
}

void UIWidgetMenu::SyncViewToSelection()
{
    if (mWidgets.next == &mWidgets) return;
    if (!mpCurrent && !mpDoneButton) {
        Reset();
        return;
    }
    const uint32_t idx = mCurrentIndex, count = mVisibleCount;
    if (idx - 1 > count && GetWidgetIndex(mpCurrent) <= idx - count) {
        mpViewTop = mpCurrent;
    } else {
        const int32_t top = static_cast<int32_t>(idx - count - 1);
        mpViewTop = FromNode(ListGetNode(&mWidgets, static_cast<uint32_t>(top < 0 ? 0 : top)));
    }
    Reposition();
    mbViewDirty = 0;
}

FEWidget *UIWidgetMenu::GetWidget(uint32_t index1)
{
    return index1 == 0 ? nullptr : FromNode(ListGetNode(&mWidgets, index1 - 1));
}

void UIWidgetMenu::IncrementStartPos()
{
    const float step = static_cast<float>(L(mRowHeight) + mSpacing);
    mStartY = static_cast<float>(L(mStartY) + step);
    mDataY = static_cast<float>(L(mDataY) + step);
}
