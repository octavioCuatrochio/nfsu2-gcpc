#include "FEWidget.h"
#include "bridge/Bridge.h"
#include "FEngUtil.h"
#include "FEObject.h"
#include "FEString.h"
#include "cFEng.h"
#include "FEHash.h"
#include "FEMath.h"
#include "FEScrollBar.h"
#include <cstring>
#include <initializer_list>

void FEWidget::Init(const float *pos, const float *size, bool enabled)
{
    vtable = bridge::FEWidgetVTable();
    mPosX = pos[0];
    mPosY = pos[1];
    mSizeX = size[0];
    mSizeY = size[1];
    mLabelOffsetX = -276.0f;   // 0xC38A0000
    mLabelOffsetY = -10.0f;    // 0xC1200000
    mpLabel = nullptr;
    mbEnabled = enabled;
    mbHidden = 0;
}

FEButtonWidget *FEButtonWidget::Construct(const float *pos, const float *size, bool enabled)
{
    base.Init(pos, size, enabled);
    base.vtable = bridge::FEButtonWidgetVTable();
    mpObject = nullptr;
    mWidth = size[0];
    mHeight = size[1];   // PC: fld/fstp (a signalling NaN would be quieted)
    return this;
}

void FEButtonWidget::Show()
{
    FEngSetVisible(mpObject);
    if (base.mpLabel) FEngSetVisible(base.mpLabel);
}

void FEButtonWidget::Hide()
{
    FEngSetInvisible(mpObject);
    if (base.mpLabel) FEngSetInvisible(base.mpLabel);
}

void FEButtonWidget::SetFocus(const char *pkg)
{
    if (cFEng *fe = bridge::cFEngInstance())
        if (mpObject) fe->SetCurrentButton(pkg, mpObject->mu32NameHash);
    FEngSetScript(mpObject, "Highlight", true);   // PC: inlined
    if (base.mpLabel) FEngSetScript(base.mpLabel, "Highlight", true);
}

void FEButtonWidget::UnsetFocus()
{
    FEngSetScript(mpObject, "UnHighlight", true);   // PC: inlined
    if (base.mpLabel) FEngSetScript(base.mpLabel, "UnHighlight", true);
}

namespace {
using L = long double;
// object at the widget's top-left; centred text is centred on the box (x) and keeps the top edge (y)
void PlaceObject(FEObject *o, float x, float y, float width)
{
    if (o && (reinterpret_cast<const FEString *>(o)->_unknown6C & 1)) {
        float cx, cy;
        FEngGetCenter(o, &cx, &cy);
        FEngSetCenter(o, static_cast<float>(L(width) * 0.5f + x), cy);
        float tlx, tly;
        FEngGetTopLeft(o, &tlx, &tly);
        FEngSetTopLeft(o, tlx, y);
    } else {
        FEngSetTopLeft(o, x, y);
    }
}
void PlaceLabel(const FEWidget &w)
{
    if (w.mpLabel)
        FEngSetTopLeft(w.mpLabel, static_cast<float>(L(w.mPosX) - w.mLabelOffsetX), static_cast<float>(L(w.mPosY) - w.mLabelOffsetY));
}
}  // namespace

void FEButtonWidget::Position()
{
    PlaceObject(mpObject, base.mPosX, base.mPosY, mWidth);
    PlaceLabel(base);
}

void FEButtonWidget::CheckMouse(const char *pkg, float x, float y)
{
    if (!FEngTestForIntersection(x, y, mpObject)) return;
    const uint32_t trigger = bridge::FEWidget_DetermineSoundTrigger(this);   // vtable +0x3C
    bridge::PlayFrontendSound(trigger);                                     // table 0x8380B8 -> PlayUISoundFX
    bridge::FEWidget_Act(this, pkg, 0x0C407210u);                           // vtable +0x04
}

FELabelWidget *FELabelWidget::Construct(const float *pos, const float *size)
{
    base.Init(pos, size, true);
    base.vtable = bridge::FELabelWidgetVTable();
    mpObject = nullptr;
    mWidth = size[0];
    mHeight = size[1];
    return this;
}

void FELabelWidget::Position()
{
    FEngSetTopLeft(mpObject, base.mPosX, base.mPosY);
    PlaceLabel(base);
}

void FELabelWidget::SetFocus(const char *)
{
    FEngSetScript(mpObject, "Highlight", true);   // PC: inlined
    if (base.mpLabel) FEngSetScript(base.mpLabel, "Highlight", true);
}

FEStatWidget *FEStatWidget::Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled)
{
    base.Init(pos, size, enabled);
    base.vtable = bridge::FEStatWidgetVTable();
    mpObject = nullptr;
    mpValue = nullptr;
    mWidth = size[0];
    mHeight = size[1];
    mValueSizeX = valueSize[0];
    mValueSizeY = valueSize[1];
    mValuePosX = valuePos[0];
    mValuePosY = valuePos[1];
    return this;
}

void FEStatWidget::Show()
{
    FEngSetVisible(mpObject);
    FEngSetVisible(mpValue);
    if (base.mpLabel) FEngSetVisible(base.mpLabel);
}

void FEStatWidget::Hide()
{
    FEngSetInvisible(mpObject);
    FEngSetInvisible(mpValue);
    if (base.mpLabel) FEngSetInvisible(base.mpLabel);
}

void FEStatWidget::SetPosX(float x)
{
    mValuePosX = static_cast<float>((L(mValuePosX) - base.mPosX) + x);
    base.mPosX = x;
    if (!base.mpLabel) return;
    float tlx, tly;
    FEngGetTopLeft(base.mpLabel, &tlx, &tly);
    FEngSetTopLeft(base.mpLabel, static_cast<float>(L(x) - base.mLabelOffsetX), tly);
}

void FEStatWidget::SetPosY(float y)
{
    mValuePosY = static_cast<float>((L(mValuePosY) - base.mPosY) + y);
    base.mPosY = y;
    if (!base.mpLabel) return;
    float tlx, tly;
    FEngGetTopLeft(base.mpLabel, &tlx, &tly);
    FEngSetTopLeft(base.mpLabel, tlx, static_cast<float>(L(y) - base.mLabelOffsetY));
}

void FEStatWidget::Position()
{
    PlaceObject(mpObject, base.mPosX, base.mPosY, mWidth);
    // the value object: centred in its box horizontally, top at the box top
    float cx, cy;
    FEngGetCenter(mpValue, &cx, &cy);
    FEngSetCenter(mpValue, static_cast<float>(L(mValueSizeX) * 0.5f + mValuePosX), cy);
    float tlx, tly;
    FEngGetTopLeft(mpValue, &tlx, &tly);
    FEngSetTopLeft(mpValue, tlx, mValuePosY);
    PlaceLabel(base);
}

FESliderWidget *FESliderWidget::Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled)
{
    base.Construct(pos, size, valueSize, valuePos, enabled);
    mpLeftArrow = mpRightArrow = nullptr;
    base.base.vtable = bridge::FESliderWidgetVTable();
    mSlider.Construct();   // PC: inlined
    return this;
}

uint32_t FESliderWidget::DetermineSpecialSound(uint32_t, uint32_t event) const
{
    if (event == 4) return mSlider.mPrev != mSlider.mMin ? 4 : 0;   // PC: NaN counts as different
    if (event == 5) return mSlider.mPrev != mSlider.mMax ? 5 : 0;
    return event;
}

void FESliderWidget::Position()
{
    FEngSetTopLeft(base.mpObject, base.base.mPosX, base.base.mPosY);
    const float y = base.mValuePosY;
    const float cx = static_cast<float>(L(base.mValueSizeX) * 0.5L + base.mValuePosX);
    float w, h;
    FEngGetSize(mSlider.mpBacking, &w, &h);
    FEngSetTopLeft(mSlider.mpBacking, static_cast<float>(cx - L(w) * 0.5L), y);
    bridge::cSlider_Draw(&mSlider);
    FEngGetSize(mpRightArrow, &w, &h);
    FEngSetTopLeft(mpLeftArrow, static_cast<float>(cx - (L(base.mValueSizeX) * 0.5L + w)), base.mValuePosY);
    FEngSetTopLeft(mpRightArrow, static_cast<float>(L(base.mValueSizeX) * 0.5L + cx), base.mValuePosY);
    if (base.base.mpLabel)
        FEngSetTopLeft(base.base.mpLabel, base.base.mPosX - base.base.mLabelOffsetX, base.base.mPosY - base.base.mLabelOffsetY);
}

void FESliderWidget::Show()
{
    FEngSetVisible(base.mpObject);
    FEngSetVisible(mpLeftArrow);
    FEngSetVisible(mpRightArrow);
    bridge::cSlider_Toggle(&mSlider, true);
    if (base.base.mpLabel) FEngSetVisible(base.base.mpLabel);
}

void FESliderWidget::Hide()
{
    FEngSetInvisible(base.mpObject);
    FEngSetInvisible(mpLeftArrow);
    FEngSetInvisible(mpRightArrow);
    bridge::cSlider_Toggle(&mSlider, false);
    if (base.base.mpLabel) FEngSetInvisible(base.base.mpLabel);
}

void FESliderWidget::SetFocus(const char *pkg)
{
    if (cFEng *fe = bridge::cFEngInstance())
        if (base.mpObject) fe->SetCurrentButton(pkg, base.mpObject->mu32NameHash);
    FEngSetScript(base.mpObject, "Highlight", true);   // PC: inlined (51CF70)
    FEngSetScript(mpLeftArrow, "Highlight", true);
    FEngSetScript(mpRightArrow, "Highlight", true);
    bridge::cSlider_Highlight(&mSlider);
    if (base.base.mpLabel) FEngSetScript(base.base.mpLabel, "Highlight", true);
}

void FESliderWidget::UnsetFocus()
{
    FEngSetScript(base.mpObject, "UnHighlight", true);   // PC: inlined
    FEngSetScript(mpLeftArrow, "UnHighlight", true);
    FEngSetScript(mpRightArrow, "UnHighlight", true);
    bridge::cSlider_UnHighlight(&mSlider);
    if (base.base.mpLabel) FEngSetScript(base.base.mpLabel, "UnHighlight", true);
}

cSlider *cSlider::Construct()
{
    vtable = bridge::cSliderVTable();
    mpPackage = bridge::GameEmptyString();
    mpBacking = mpHandle = mpValue = mpCurrent = mpEndCap = mpEnd = nullptr;
    mMax = mMin = mValue = mPrev = mInitial = mStep = mLength = 0;
    mMargin = 46.0f;
    mbVertical = 0;
    return this;
}

bool cSlider::Update(uint32_t msg)
{
    bool moved = false;
    if (msg == 0x9120409Eu || msg == 0xB5971BF1u) {
        // x87: the new value stays extended for the limit test (rounded only in memory)
        L v;
        float limit;
        mPrev = mValue;
        if (msg == 0x9120409Eu) {
            v = L(mValue) - mStep;
            limit = mMin;
            if (L(limit) > v) v = limit;   // PC: test 0x41 (NaN keeps v)
        } else {
            v = L(mStep) + mValue;
            limit = mMax;
            if (L(limit) < v) v = limit;   // PC: test 0x05 (NaN keeps v)
        }
        mValue = static_cast<float>(v);
        moved = !(v == L(limit) && mPrev == limit);
    }
    bridge::cSlider_Draw(this);
    return moved;
}

void cSlider::Init(const SliderInitParams *p, bool vertical)
{
    if (p->mpPackage) mpPackage = p->mpPackage;
    if (p->mpHandle) mpHandle = FEngFindImage(mpPackage, FEHashUpper(p->mpHandle));
    if (p->mpBacking) mpBacking = FEngFindImage(mpPackage, FEHashUpper(p->mpBacking));
    if (p->mpValue) mpValue = FEngFindString(mpPackage, FEHashUpper(p->mpValue));
    if (p->mpCurrent) mpCurrent = FEngFindImage(mpPackage, FEHashUpper(p->mpCurrent));
    if (p->mpEndCap) mpEndCap = FEngFindImage(mpPackage, FEHashUpper(p->mpEndCap));
    if (p->mpEnd) mpEnd = FEngFindImage(mpPackage, FEHashUpper(p->mpEnd));
    mMax = p->mMax;
    mMin = p->mMin;
    mValue = mInitial = p->mValue;
    mStep = p->mStep;
    mMargin = p->mMargin;
    // PC: the width lands in the `params` argument slot, so without a backing object it is the pointer's bits
    float w, h;
    const uint32_t bits = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p));
    std::memcpy(&w, &bits, 4);
    FEngGetSize(mpBacking, &w, &h);
    mbVertical = vertical;
    mLength = static_cast<float>(L(w) - (L(mMargin) + mMargin));
}

void cSlider::Draw()
{
    if (mbVertical) bridge::cSlider_DrawVertical(this);
    else bridge::cSlider_DrawHorizontal(this);
}

// PC also measures the end cap / handle / centre / fill bar here into unused locals (pure, left out).
void cSlider::DrawHorizontal()
{
    float hw = 0, hh = 0, x = 0, y = 0, bw = 0, bh = 0;
    FEngGetSize(mpHandle, &hw, &hh);
    FEngGetTopLeft(mpBacking, &x, &y);
    if (mbVertical) { const float t = x; x = y; y = t; }
    const L ratio = (L(mValue) - mMin) / (L(mMax) - mMin);
    const float ratioF = static_cast<float>(ratio);
    const float hx = static_cast<float>(((ratio * mLength - L(hw) * 0.5L) + mMargin) + x);
    bh = hw;   // PC: same stack slot (kept when there is no backing)
    FEngGetSize(mpBacking, &bw, &bh);
    const L hy = ((L(bh) * 0.5L + y) - L(hh) * 0.5L) + 1.0L;
    const float hyF = static_cast<float>(hy);
    FEngSetTopLeft(mpHandle, hx, static_cast<float>(hy - 2.0L));
    FEngSetWidth(mpCurrent, static_cast<float>(L(ratioF) * mLength));
    FEngSetTopLeft(mpCurrent, static_cast<float>(L(x) + mMargin), hyF);
    if (mpValue) bridge::FEPrintfInt(mpValue, "%d", FEFtol(mValue));
}

void cSlider::DrawVertical()
{
    float w = 0, h = 0, cy = 0;
    FEngGetSize(mpHandle, &w, &h);    // PC: only feed the centre's x slot when there is no backing
    FEngGetSize(mpBacking, &w, &h);
    float cx = w;
    FEngGetCenter(mpBacking, &cx, &cy);
    const float x = static_cast<float>(L(cx) + 4.0L);
    const float base = static_cast<float>(L(mLength) * 0.5L + cy);
    const float ratio = static_cast<float>((L(mValue) - mMin) / (L(mMax) - mMin));
    const float hx = static_cast<float>(L(x) - 3.0L);
    L hy = L(base) - L(ratio) * mLength * L(0.95f);
    if (ratio > 0.0f) hy -= 7.0L;
    FEngSetCenter(mpHandle, hx, static_cast<float>(hy));
    const float size = static_cast<float>(L(ratio) * mLength * L(0.95f));
    FEngSetInvisible(mpEndCap);
    FEngSetInvisible(mpEnd);
    FEngSetWidth(mpCurrent, size);   // PC: width, also for the vertical bar
    FEngSetCenter(mpCurrent, x, static_cast<float>((L(base) - L(size) * 0.5L) - 7.0L));
    if (mpValue) bridge::FEPrintfInt(mpValue, "%d", FEFtol(mValue));
}

void cSlider::Toggle(bool on)
{
    if (on) {
        FEngSetVisible(mpBacking);
        FEngSetVisible(mpHandle);
        FEngSetVisible(mpValue);
        FEngSetVisible(mpCurrent);
    } else {
        FEngSetInvisible(mpValue);
        FEngSetInvisible(mpBacking);
        FEngSetInvisible(mpHandle);
        FEngSetInvisible(mpCurrent);
        FEngSetInvisible(mpEndCap);
        FEngSetInvisible(mpEnd);
    }
}

void cSlider::SetCurValue(float v)
{
    mPrev = mValue;
    mValue = v;
    if (v > mMax) mValue = mMax;          // PC: test 0x41 (NaN kept)
    if (mValue < mMin) mValue = mMin;     // PC: test 0x05
}

void cSlider::Highlight()
{
    for (FEObject *o : {mpBacking, mpHandle, mpValue, mpCurrent, mpEndCap, mpEnd}) FEngSetScript(o, "Highlight", true);
}

void cSlider::UnHighlight()
{
    for (FEObject *o : {mpBacking, mpHandle, mpValue, mpCurrent, mpEndCap, mpEnd}) FEngSetScript(o, "UnHighlight", true);
}
