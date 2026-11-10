#include "UIWidgetMenu.h"
#include "bridge/Bridge.h"
#include "FEWidget.h"
#include "FEngUtil.h"
#include "FEHash.h"
#include "cFEng.h"
#include "FEObject.h"
#include <cmath>
#include <cstdio>
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

namespace {
// selection change shared by Scroll / ScrollWrapped (PC 5392E4 / 5395B9)
void ApplySelection(UIWidgetMenu &m, FEWidget *w)
{
    if (m.mpCurrent != w) {
        if (m.mpCurrent) bridge::FEWidget_UnsetFocus(m.mpCurrent);
        if (w) bridge::FEWidget_SetFocus(w, m.base.mpPackageName);
        m.mpCurrent = w;
        if (!m._unknown146) return;
        if (w) m.mScrollBar.Update(1, static_cast<int32_t>(m.mCurrentIndex - 1), static_cast<int32_t>(m.GetWidgetIndex(w)));
    }
    if (m._unknown146 && m.mScrollBar.mpHandle) {
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
