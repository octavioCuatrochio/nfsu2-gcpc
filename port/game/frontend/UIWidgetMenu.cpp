#include "UIWidgetMenu.h"
#include "bridge/Bridge.h"
#include "FEWidget.h"
#include "FEngUtil.h"
#include "FEHash.h"
#include "cFEng.h"
#include "FEObject.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include "bWare/bString.h"
#include "FEScrollBar.h"

namespace {
using L = long double;
constexpr size_t kNodeOffset = offsetof(FEWidget, _unknown04);   // PC 4
// node -> owning widget (PC: lea r,[node-4] when the node is non-null, else null); the head maps to this - 4 too
FEWidget *FromNode(const WidgetListNode *n)
{
    return n ? reinterpret_cast<FEWidget *>(reinterpret_cast<uintptr_t>(n) - kNodeOffset) : nullptr;
}
WidgetListNode *NextOf(FEWidget *w) { return *reinterpret_cast<WidgetListNode **>(reinterpret_cast<uint8_t *>(w) + kNodeOffset); }
WidgetListNode *PrevOf(FEWidget *w) { return reinterpret_cast<WidgetListNode **>(reinterpret_cast<uint8_t *>(w) + kNodeOffset)[1]; }
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
    mScrollPos = 1;
    mVisibleCount = 7;
    mPrevMsg = mPrevArg = mPrevPkg = 0;
    mbWrapScroll = 0; mbInitialOptionSet = 0; mbScrollBarEnabled = 1;
    mbViewDirty = 0;
    mbInputEnabled = 1; mbSoundsEnabled = 1;
    mpTitleMaster = FEngFindObject(base.mpPackageName, FEHashUpper("TITLE_MASTER"));
    mpDataMaster = FEngFindObject(base.mpPackageName, FEHashUpper("DATA_MASTER"));
    mpDoneButton = FEngFindObject(base.mpPackageName, FEHashUpper("DONE_BUTTON"));
    if (mpTitleMaster && mpDataMaster) {
        // PC: hide the scroll bar parts (GC: FEScrollBar::Update(0, 0, 0))
        FEngSetInvisible(mScrollBar.mpHandle);
        FEngSetInvisible(mScrollBar.mpArrow1);
        FEngSetInvisible(mScrollBar.mpArrow2);
        SetInitialPositions();
        base.mbSounds = 0;
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

namespace {
// selection change shared by Scroll / ScrollWrapped (PC 5392E4 / 5395B9)
void ApplySelection(UIWidgetMenu &m, FEWidget *w)
{
    if (m.mpCurrent != w) {
        if (m.mpCurrent) bridge::FEWidget_UnsetFocus(m.mpCurrent);
        if (w) bridge::FEWidget_SetFocus(w, m.base.mpPackageName);
        m.mpCurrent = w;
        if (!m.mbScrollBarEnabled) return;
        if (w) m.mScrollBar.Update(1, static_cast<int32_t>(m.mCurrentIndex - 1), static_cast<int32_t>(m.GetWidgetIndex(w)));
    }
    if (m.mbScrollBarEnabled && m.mScrollBar.mpHandle) {
        if (w) FEngSetVisible(m.mScrollBar.mpHandle);
        else FEngSetInvisible(m.mScrollBar.mpHandle);
    }
}
}  // namespace

void UIWidgetMenu::Scroll(int32_t dir)
{
    if (mWidgets.next == &mWidgets) return;
    if (mbViewDirty) {
        SyncViewToSelection();
        return;
    }
    FEWidget *w = mpCurrent;
    if (dir == 1) {
        if (w && w == FromNode(mWidgets.prev) && mpDoneButton) {
            FEngSetCurrentButton(base.mpPackageName, mpDoneButton);   // past the last widget: the done button
            ApplySelection(*this, nullptr);
            return;
        }
        if (w) {
            if (w != FromNode(mWidgets.prev)) {
                const int32_t limit = static_cast<int32_t>(mCurrentIndex - 1);
                do {
                    w = FromNode(NextOf(w));
                    int32_t p = static_cast<int32_t>(mScrollPos + 1);
                    if (limit < p) p = limit;
                    mScrollPos = static_cast<uint32_t>(p);
                } while (w && !w->mbEnabled && w != FromNode(mWidgets.prev));
            }
            while (GetWidgetIndex(w) >= GetWidgetIndex(mpViewTop) + mVisibleCount) {
                mpViewTop = FromNode(NextOf(mpViewTop));
                Reposition();
            }
        }
        if (mScrollBar.mpHandle) FEngSetVisible(mScrollBar.mpHandle);
    } else if (!w) {
        w = FromNode(mWidgets.prev);   // nothing selected: up selects the last widget
    } else {
        if (w != FromNode(mWidgets.next)) {
            do {
                w = FromNode(PrevOf(w));
                int32_t p = static_cast<int32_t>(mScrollPos - 1);
                if (p < 1) p = 1;
                mScrollPos = static_cast<uint32_t>(p);
            } while (w && !w->mbEnabled && w != FromNode(mWidgets.next));
        }
        while (GetWidgetIndex(w) < GetWidgetIndex(mpViewTop)) {
            mpViewTop = FromNode(PrevOf(mpViewTop));
            Reposition();
        }
    }
    ApplySelection(*this, w);
}

void UIWidgetMenu::ScrollWrapped(int32_t dir)
{
    if (mWidgets.next == &mWidgets) return;
    if (mbViewDirty) {
        SyncViewToSelection();
        return;
    }
    FEWidget *top = mpViewTop;
    FEWidget *w = mpCurrent;
    const uint32_t curIdx = GetWidgetIndex(w);
    const uint32_t topIdx = GetWidgetIndex(top);
    if (dir == 1) {
        const uint32_t slack = mVisibleCount - curIdx + topIdx - 1;
        bool done = false;
        for (;;) {
            if (w && !(w == FromNode(mWidgets.prev) && !mpDoneButton)) {
                if (w == FromNode(mWidgets.prev)) {   // last widget with a done button: select it
                    w = nullptr;
                    if (cFEng *fe = bridge::cFEngInstance())
                        if (mpDoneButton) fe->SetCurrentButton(base.mpPackageName, mpDoneButton->mu32NameHash);
                    done = true;
                    break;
                }
                w = FromNode(NextOf(w));
                if (++mScrollPos > mCurrentIndex - 1) mScrollPos = 1;
            } else {
                w = FromNode(mWidgets.next);   // wrap to the first widget, view from the top
                top = w;
            }
            if (!w || w->mbEnabled) break;
        }
        (void)done;
        const uint32_t idx = GetWidgetIndex(w);
        if (idx >= mVisibleCount + topIdx) {
            const int32_t steps = static_cast<int32_t>(idx - curIdx - slack);
            for (int32_t i = steps; i > 0; --i) {
                WidgetListNode *n = NextOf(top);
                top = n ? FromNode(n) : nullptr;
            }
        }
    } else {
        bool done = false;
        for (;;) {
            if (w && !(w == FromNode(mWidgets.next) && !mpDoneButton)) {
                if (w == FromNode(mWidgets.next)) {   // first widget with a done button
                    w = nullptr;
                    FEngSetCurrentButton(base.mpPackageName, mpDoneButton);
                    done = true;
                    break;
                }
                w = FromNode(PrevOf(w));
                if (--mScrollPos < 1) mScrollPos = mCurrentIndex - 1;
            } else {
                w = FromNode(mWidgets.prev);   // wrap to the last widget, view at the end
                WidgetListNode *n = ListGetNode(&mWidgets, mCurrentIndex - mVisibleCount - 1);
                top = n ? FromNode(n) : nullptr;
            }
            if (!w || w->mbEnabled) break;
        }
        (void)done;
        if (GetWidgetIndex(w) < topIdx) top = w;
    }
    if (mpViewTop != top) {
        mpViewTop = top;
        Reposition();
    }
    ApplySelection(*this, w);
}

namespace {
// "<prefix>%d" formatted with the current row index (PC: bStrCat + FEngSNPrintf, 0x20-byte buffers)
void OptionName(char (&out)[64], const char *prefix, uint32_t index)
{
    char fmt[64];
    bStrCat(fmt, prefix, "%d");
    std::snprintf(out, sizeof out, fmt, static_cast<int>(index));
}
void Append(UIWidgetMenu &m, FEWidget *w)
{
    auto *n = reinterpret_cast<WidgetListNode *>(reinterpret_cast<uint8_t *>(w) + kNodeOffset);
    WidgetListNode *last = m.mWidgets.prev;
    last->next = n;
    m.mWidgets.prev = n;
    n->next = &m.mWidgets;
    n->prev = last;
}
// the inlined IncrementStartPos: two evaluation orders on PC
void AdvanceRowStepFirst(UIWidgetMenu &m)   // AddButton / AddStat / AddToggleImage / AddSlider
{
    ++m.mCurrentIndex;
    m.mStartY = static_cast<float>((L(m.mRowHeight) + m.mSpacing) + m.mStartY);
    m.mDataY = static_cast<float>((L(m.mRowHeight) + m.mSpacing) + m.mDataY);
}
void AdvanceRowPosFirst(UIWidgetMenu &m)    // AddToggleOption
{
    ++m.mCurrentIndex;
    m.mStartY = static_cast<float>((L(m.mStartY) + m.mRowHeight) + m.mSpacing);
    m.mDataY = static_cast<float>((L(m.mDataY) + m.mRowHeight) + m.mSpacing);
}
void ShowNewRow(FEWidget *w)
{
    if (!w->mbEnabled) bridge::FEWidget_Disable(w);   // slot 8
    bridge::FEWidget_Show(w);
    bridge::FEWidget_Position(w);
    bridge::FEWidget_Draw(w);
}
// width = |left edge - right edge of obj| (PC: x87 w + x)
float RowWidth(const FEWidget *w, const FEObject *obj)
{
    float x, y, ow, oh;
    FEngGetTopLeft(obj, &x, &y);
    FEngGetSize(obj, &ow, &oh);
    return static_cast<float>(std::fabs(L(w->mPosX) - (L(ow) + x)));
}
void SetArrowTextures(FEObject *left, FEObject *right, int32_t style)
{
    uint32_t l = 0x367B3E04u, r = 0x367B3E04u;
    if (style == 1) l = r = 0x9943D584u;
    else if (style == 2 || style == 3) { l = 0x374F6465u; r = 0x760BDC9Du; }
    if (left) FEngSetTextureHash(left, l);
    if (right) FEngSetTextureHash(right, r);
}
}  // namespace

FEObject *UIWidgetMenu::GetCurrentFEObject(const char *prefix)
{
    char name[64];
    OptionName(name, prefix, mCurrentIndex);
    return FEngFindObject(base.mpPackageName, FEHashUpper(name));
}

FEObject *UIWidgetMenu::GetCurrentFEString(const char *prefix)
{
    char name[64];
    OptionName(name, prefix, mCurrentIndex);
    return FEngFindString(base.mpPackageName, FEHashUpper(name));
}

FEObject *UIWidgetMenu::GetCurrentFEImage(const char *prefix)
{
    char name[64];
    OptionName(name, prefix, mCurrentIndex);
    return FEngFindImage(base.mpPackageName, name);
}

uint32_t UIWidgetMenu::AddButtonOption(FEButtonWidget *w)
{
    w->mpObject = GetCurrentFEString(mOptionName);
    w->base.mpLabel = GetCurrentFEObject(mOptionBacking);
    Append(*this, &w->base);
    AdvanceRowStepFirst(*this);
    ShowNewRow(&w->base);
    w->base.mSizeX = mDataOffsetX;
    return mCurrentIndex - 1;
}

uint32_t UIWidgetMenu::AddStatOption(FEStatWidget *w)
{
    w->mpObject = GetCurrentFEString(mOptionName);
    w->mpValue = GetCurrentFEString(mOptionData);
    w->base.mpLabel = GetCurrentFEObject(mOptionBacking);
    Append(*this, &w->base);
    AdvanceRowStepFirst(*this);
    ShowNewRow(&w->base);
    w->base.mSizeX = RowWidth(&w->base, w->mpValue);
    return mCurrentIndex - 1;
}

uint32_t UIWidgetMenu::AddToggleOption(FEToggleWidget *w, int32_t style)
{
    FEStatWidget &s = w->base;
    s.mpObject = GetCurrentFEString(mOptionName);
    s.mpValue = GetCurrentFEString(mOptionData);
    s.base.mpLabel = GetCurrentFEObject(mOptionBacking);
    w->mpLeftArrow = GetCurrentFEImage(mLeftArrow);
    w->mpRightArrow = GetCurrentFEImage(mRightArrow);
    SetArrowTextures(w->mpLeftArrow, w->mpRightArrow, style);
    Append(*this, &s.base);
    AdvanceRowPosFirst(*this);
    ShowNewRow(&s.base);
    s.base.mSizeX = RowWidth(&s.base, w->mpRightArrow);
    return mCurrentIndex - 1;
}

uint32_t UIWidgetMenu::AddToggleOption(FEToggleWidget *w, bool plain) { return AddToggleOption(w, plain ? 0 : 2); }

uint32_t UIWidgetMenu::AddToggleImageOption(FEToggleImageWidget *w, int32_t style)
{
    FEToggleWidget &t = w->base;
    FEStatWidget &s = t.base;
    s.mpObject = GetCurrentFEString(mOptionName);
    s.mpValue = GetCurrentFEString(mOptionData);
    s.base.mpLabel = GetCurrentFEObject(mOptionBacking);
    w->mpImage = GetCurrentFEImage(mOptionImage);
    t.mpLeftArrow = GetCurrentFEImage(mLeftArrow);
    t.mpRightArrow = GetCurrentFEImage(mRightArrow);
    SetArrowTextures(t.mpLeftArrow, t.mpRightArrow, style);
    Append(*this, &s.base);
    AdvanceRowStepFirst(*this);
    ShowNewRow(&s.base);
    s.base.mSizeX = RowWidth(&s.base, t.mpRightArrow);
    return mCurrentIndex - 1;
}

uint32_t UIWidgetMenu::AddSliderOption(FESliderWidget *w, int32_t style)
{
    FEStatWidget &s = w->base;
    s.mpObject = GetCurrentFEString(mOptionName);
    FEObject *end = GetCurrentFEImage(mSliderEnd);
    FEObject *beg = GetCurrentFEImage(mSliderBeg);
    FEObject *handle = GetCurrentFEImage(mSliderHandle);
    FEObject *value = GetCurrentFEString(mSliderValue);
    FEObject *knob = GetCurrentFEImage(mSliderKnob);
    FEObject *backing = GetCurrentFEImage(mSliderBase);
    w->SetSliderObjects(backing, knob, value, handle, beg, end);
    s.base.mpLabel = GetCurrentFEObject(mOptionBacking);
    bridge::FEWidget_Slot17(w);   // vtable +0x44 (slider widgets)
    w->mpLeftArrow = GetCurrentFEImage(mLeftArrow);
    w->mpRightArrow = GetCurrentFEImage(mRightArrow);
    SetArrowTextures(w->mpLeftArrow, w->mpRightArrow, style);
    Append(*this, &s.base);
    AdvanceRowStepFirst(*this);
    ShowNewRow(&s.base);
    s.base.mSizeX = RowWidth(&s.base, w->mpRightArrow);
    return mCurrentIndex - 1;
}

uint32_t UIWidgetMenu::AddSliderOption(FESliderWidget *w, bool plain) { return AddSliderOption(w, plain ? 0 : 2); }

void UIWidgetMenu::StorePrevNotification(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg)
{
    mPrevMsg = msg;
    mpPrevObject = obj;
    mPrevArg = arg;
    mPrevPkg = pkg;
}

void UIWidgetMenu::SetInitialOption(int32_t index1)
{
    if (mbInitialOptionSet) return;
    WidgetListNode *firstNode = mWidgets.next;
    FEWidget *w = FromNode(firstNode);
    bool fallback = false;
    if (index1 != 0) {
        uint32_t n = 1;
        for (;;) {
            if (w == FromNode(&mWidgets)) goto sync;   // index past the end: nothing selected
            if (n == static_cast<uint32_t>(index1)) break;
            w = FromNode(NextOf(w));
            ++n;
        }
        if (w && w->mbEnabled) {
            mpCurrent = w;
            bridge::FEWidget_SetFocus(w, base.mpPackageName);
            mbInitialOptionSet = 1;
            mScrollPos = n;
            goto sync;
        }
        fallback = true;
    } else if (!mpDoneButton) {
        fallback = true;
    } else {
        if (mpCurrent) {
            bridge::FEWidget_UnsetFocus(mpCurrent);
            mpCurrent = nullptr;
        }
        if (cFEng *fe = bridge::cFEngInstance())
            if (mpDoneButton) fe->SetCurrentButton(base.mpPackageName, mpDoneButton->mu32NameHash);
    }
    if (fallback) {
        // first enabled widget (counting mScrollPos from 1)
        mScrollPos = 1;
        w = FromNode(firstNode);
        while (w && !w->mbEnabled && w != FromNode(&mWidgets)) {
            w = FromNode(NextOf(w));
            ++mScrollPos;
        }
        if (w != FromNode(&mWidgets)) {
            mpCurrent = w;
            bridge::FEWidget_SetFocus(w, base.mpPackageName);
            mbInitialOptionSet = 1;
        } else {
            mpCurrent = FromNode(firstNode);
        }
    }
sync:
    SyncViewToSelection();
    if (mbScrollBarEnabled)
        mScrollBar.Update(1, static_cast<int32_t>(mCurrentIndex - 1), static_cast<int32_t>(GetWidgetIndex(mpCurrent)));
}

FEWidget *UIWidgetMenu::WidgetAtPoint(float x, float y)
{
    for (FEWidget *w = FromNode(mWidgets.next); w != FromNode(&mWidgets); w = FromNode(NextOf(w))) {
        if (w->mbHidden) continue;
        const float pos[2] = {w->mPosX, w->mPosY}, size[2] = {w->mSizeX, w->mSizeY};
        if (FEngTestForIntersection(x, y, pos, size)) return w;
    }
    return nullptr;
}

void UIWidgetMenu::SelectWidget(FEWidget *w)
{
    if (mpCurrent == w) return;
    if (mpCurrent) bridge::FEWidget_UnsetFocus(mpCurrent);
    mpCurrent = w;
    bridge::FEWidget_SetFocus(w, base.mpPackageName);
}

void UIWidgetMenu::HandleMouse()
{
    const uint8_t *ms = bridge::MouseGlobals();
    int32_t mxi, myi, moved1, moved2, held;
    std::memcpy(&mxi, ms + 0x00, 4);
    std::memcpy(&myi, ms + 0x04, 4);
    std::memcpy(&moved1, ms + 0x10, 4);
    std::memcpy(&moved2, ms + 0x14, 4);
    std::memcpy(&held, ms + 0x18, 4);
    const float x = static_cast<float>(L(mxi) - 320.0f);   // screen -> FEng coordinates
    const float y = static_cast<float>(L(myi) - 240.0f);
    const uint8_t clicked = ms[0x1C] ? ms[0x21] : ms[0x1E];   // left-handed swaps the buttons
    if (clicked) {
        if (mpCurrent && mpCurrent->mbEnabled) bridge::FEWidget_CheckMouse(mpCurrent, base.mpPackageName, x, y);
        return;
    }
    if (held) return;
    if (!moved1 && !moved2) return;
    if (FEWidget *w = WidgetAtPoint(x, y)) {
        if (w != mpCurrent && w->mbEnabled) SelectWidget(w);
        return;
    }
    if (FEngTestForIntersection(x, y, mpDoneButton)) {
        if (mpCurrent) {
            bridge::FEWidget_UnsetFocus(mpCurrent);
            mpCurrent = nullptr;
        }
        FEngSetCurrentButton(base.mpPackageName, mpDoneButton);
    }
}

void UIWidgetMenu::HandleScrollBarMouse()
{
    mScrollBar.HandleMouse();   // sets mHit (-1 up arrow, 1 down arrow, ...)
    const int32_t hit = mScrollBar.mHit;
    if (hit == -1) {
        if (mbWrapScroll) {
            ScrollWrapped(-1);
        } else {
            Scroll(-1);
            if (mpCurrent == FromNode(mWidgets.next)) return;   // already at the top: no blink
        }
        mScrollBar.BlinkArrow1();
    } else if (hit == 1) {
        if (mbWrapScroll) {
            ScrollWrapped(1);
        } else {
            Scroll(1);
            if (mpCurrent == FromNode(mWidgets.prev)) return;
        }
        mScrollBar.BlinkArrow2();
    } else {
        mScrollBar.Update(1, static_cast<int32_t>(mCurrentIndex - 1), static_cast<int32_t>(GetWidgetIndex(mpCurrent)));
    }
}

void UIWidgetMenu::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg)
{
    uint32_t event = 0;
    uint32_t trigger = base.mSoundTrigger;
    const char *name = base.mpPackageName;
    auto enabledCurrent = [this] { return mpCurrent && mpCurrent->mbEnabled; };
    switch (msg) {
    case 0x0C407210u:
    case 0x911AB364u:
        if (enabledCurrent()) {
            bridge::FEWidget_Act(mpCurrent, name, msg);
            StorePrevNotification(msg, obj, arg, pkg);
        }
        if (mpCurrent && !mpCurrent->mbEnabled) break;
        event = msg == 0x911AB364u ? 10 : 8;
        break;
    case 0x0153E234u:
        bridge::FEWidget_SetFocus(mpCurrent, name);
        break;
    case 0x251BF83Eu:
    case 0xC9D30688u:
        FEngSetScript(name, 0x47FF4E7Cu, "UNDIM", true);
        if (msg == 0xC9D30688u) mbInputEnabled = 1;
        break;
    case 0x96DDA9E6u:
        FEngSetScript(name, 0x47FF4E7Cu, "DIM", true);
        break;
    case 0x35F8620Bu:
        if (enabledCurrent()) {
            bridge::FEWidget_SetFocus(mpCurrent, name);
            event = 11;
            trigger = bridge::FEWidget_DetermineSoundTrigger(mpCurrent);
        }
        break;
    case 0x406415E3u:
        if (enabledCurrent()) event = 8;
        break;
    case 0x72619778u:
    case 0x911C0A4Bu:
        if (mbInputEnabled) {
            const int32_t dir = msg == 0x911C0A4Bu ? 1 : -1;
            if (mbWrapScroll) ScrollWrapped(dir);
            else Scroll(dir);
            event = dir == 1 ? 7 : 6;
            trigger = 7;
        }
        break;
    case 0x84378BEFu:
        if (enabledCurrent()) {
            bridge::FEWidget_UnsetFocus(mpCurrent);
            event = 12;
            trigger = bridge::FEWidget_DetermineSoundTrigger(mpCurrent);
        }
        break;
    case 0x81017864u:   // start text input once the keyboard delay has passed
        // PC: fild (now - created) * 0.00025f compared unrounded with g_KBDelaySeconds
        if (!(L(bridge::RealTimer() - bridge::KeyboardCreationTimer()) * 0.00025f < bridge::KeyboardDelaySeconds())) {
            FEWidget *iw = mpCurrent;
            bridge::FEInputWidget_SetText(iw, bridge::GameEmptyString());
            const uint8_t *p = reinterpret_cast<const uint8_t *>(iw);
            FEObject *field;
            uint32_t a250, a24C;
            std::memcpy(&field, p + 0x30, sizeof field);
            std::memcpy(&a250, p + 0x250, 4);
            std::memcpy(&a24C, p + 0x24C, 4);
            bridge::MenuScreen_BeginTextInput(this, field->mu32NameHash, a250, const_cast<uint8_t *>(p) + 0x4C,
                                              const_cast<uint8_t *>(p) + 0x14C, a24C, 0);
            mbInputEnabled = 0;
            bridge::FEInputWidget_SetInputFocus(iw);
        }
        break;
    case 0x9120409Eu:
    case 0xB5971BF1u:
        if (enabledCurrent()) {
            bridge::FEWidget_Act(mpCurrent, name, msg);
            event = mpCurrent->DetermineSound(msg);
            trigger = bridge::FEWidget_DetermineSoundTrigger(mpCurrent);
        }
        break;
    case 0x92B703B5u:
        RefreshWidgets();
        break;
    case 0x9803F6E2u:
        HandleMouse();
        break;
    case 0xAF0BBD92u:   // rebuild: clear and run the screen's Setup (vtable slot 4)
        mbInitialOptionSet = 0;
        mCurrentIndex = 1;
        ClearWidgets();
        bridge::MenuScreen_Setup(this);
        break;
    case 0xC98356BAu:
        HandleScrollBarMouse();
        break;
    case 0xDA52AF6Bu: {
        FEWidget *w = mpCurrent;
        bridge::FEDateWidget_SetDate(w);
        bridge::FEWidget_Draw(w);
        break;
    }
    case 0xDA5B8712u: {   // text input finished: copy the edited string into the input widget
        FEngSetScript(name, 0x47FF4E7Cu, "UNDIM", true);
        FEWidget *w = mpCurrent;
        bStrNCpy(reinterpret_cast<char *>(w) + 0x4C, base.mpTextInput ? bridge::EditedStringBuffer() : nullptr, 0xFF);
        bridge::FEWidget_Draw(w);
        event = w->DetermineSound(0xDA5B8712u);
        trigger = bridge::FEWidget_DetermineSoundTrigger(w);
        mbInputEnabled = 1;
        break;
    }
    default:
        break;
    }
    if (mbSoundsEnabled && event) bridge::MakeFrontendSound(event, trigger);
}
