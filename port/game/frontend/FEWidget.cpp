#include "FEWidget.h"
#include "bridge/Bridge.h"
#include "FEngUtil.h"
#include "FEObject.h"
#include "FEString.h"
#include "cFEng.h"
#include "FEHash.h"
#include "FEMath.h"
#include "FEScrollBar.h"
#include "FELocalize.h"
#include "TuningUtil.h"
#include "FEPackageManager.h"
#include <cstring>
#include <initializer_list>
#include <cmath>

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

FEToggleWidget *FEToggleWidget::Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled)
{
    base.Construct(pos, size, valueSize, valuePos, enabled);
    mpLeftArrow = mpRightArrow = nullptr;
    base.base.vtable = bridge::FEToggleWidgetVTable();
    return this;
}

void FEToggleWidget::CheckMouse(const char *pkg, float x, float y)
{
    uint32_t msg, event;
    if (FEngTestForIntersection(x, y, mpLeftArrow)) {
        msg = 0x9120409Eu;   // left
        event = 4;
    } else if (FEngTestForIntersection(x, y, mpRightArrow) || FEngTestForIntersection(x, y, base.mpValue)) {
        msg = 0xB5971BF1u;   // right
        event = 5;
    } else {
        return;
    }
    bridge::FEWidget_Act(this, pkg, msg);                                      // vtable +0x04
    bridge::FEWidget_BlinkArrows(this, msg);                                   // vtable +0x40
    const uint32_t trigger = bridge::FEWidget_DetermineSoundTrigger(this);     // vtable +0x3C (PC: before +0x38)
    const uint32_t sound = bridge::FEWidget_DetermineSpecialSound(this, msg, event);   // vtable +0x38
    bridge::MakeFrontendSound(sound, trigger);
}

namespace {
// arrows left and right of the value box at height y (GC FEToggleWidget::Position tail)
void PlaceArrows(const FEStatWidget &s, FEObject *left, FEObject *right, float y)
{
    const float cx = static_cast<float>(L(s.mValueSizeX) * 0.5f + s.mValuePosX);
    float w, h;
    FEngGetSize(right, &w, &h);
    FEngSetTopLeft(left, static_cast<float>(cx - (L(s.mValueSizeX) * 0.5f + w)), y);
    FEngSetTopLeft(right, static_cast<float>(L(s.mValueSizeX) * 0.5f + cx), y);
}
// the four toggle objects + optional label (PC: FEngSetScript inlined into each member)
void ToggleScript(const FEToggleWidget &t, const char *script)
{
    FEngSetScript(t.base.mpObject, script, true);
    FEngSetScript(t.base.mpValue, script, true);
    FEngSetScript(t.mpLeftArrow, script, true);
    FEngSetScript(t.mpRightArrow, script, true);
    if (t.base.base.mpLabel) FEngSetScript(t.base.base.mpLabel, script, true);
}
void ToggleVisible(const FEToggleWidget &t, bool visible)
{
    for (FEObject *o : {t.base.mpObject, t.base.mpValue, t.mpLeftArrow, t.mpRightArrow})
        visible ? FEngSetVisible(o) : FEngSetInvisible(o);
    if (FEObject *l = t.base.base.mpLabel) visible ? FEngSetVisible(l) : FEngSetInvisible(l);
}
}  // namespace

void FEToggleWidget::Position()
{
    base.Position();
    PlaceArrows(base, mpLeftArrow, mpRightArrow, base.mValuePosY);
}

void FEToggleWidget::Enable()
{
    base.base.mbEnabled = 1;
    ToggleScript(*this, "UnHighlight");
}

void FEToggleWidget::Disable()
{
    base.base.mbEnabled = 0;
    ToggleScript(*this, "Disable");
}

void FEToggleWidget::Show() { ToggleVisible(*this, true); }
void FEToggleWidget::Hide() { ToggleVisible(*this, false); }

void FEToggleWidget::SetFocus(const char *pkg)
{
    if (cFEng *fe = bridge::cFEngInstance())
        if (base.mpObject) fe->SetCurrentButton(pkg, base.mpObject->mu32NameHash);
    ToggleScript(*this, "Highlight");
}

void FEToggleWidget::UnsetFocus() { ToggleScript(*this, "UnHighlight"); }

void FEToggleWidget::BlinkArrows(uint32_t msg)
{
    if (msg == 0x9120409Eu) FEngSetScript(mpLeftArrow, "Blink", true);
    else if (msg == 0xB5971BF1u) FEngSetScript(mpRightArrow, "Blink", true);
}

FEToggleImageWidget *FEToggleImageWidget::Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled)
{
    base.Construct(pos, size, valueSize, valuePos, enabled);   // GC then installs FEToggleImageWidget's vtable (none on PC)
    mpImage = nullptr;
    return this;
}

void FEToggleImageWidget::Position()
{
    const FEStatWidget &s = base.base;
    float iw, ih;
    FEngGetSize(mpImage, &iw, &ih);
    const float y = static_cast<float>(L(ih) * 0.5f + s.base.mPosY);   // object and arrows: centred on the image
    FEngSetTopLeft(s.mpObject, s.base.mPosX, y);
    float cx, cy, tlx, tly;
    // image: centred in the value box, top at the box top
    FEngGetCenter(mpImage, &cx, &cy);
    FEngSetCenter(mpImage, static_cast<float>(L(s.mValueSizeX) * 0.5f + s.mValuePosX), cy);
    FEngGetTopLeft(mpImage, &tlx, &tly);
    FEngSetTopLeft(mpImage, tlx, s.mValuePosY);
    // value text: centred, one pixel below the image
    FEngGetCenter(s.mpValue, &cx, &cy);
    FEngSetCenter(s.mpValue, static_cast<float>(L(s.mValueSizeX) * 0.5f + s.mValuePosX), cy);
    FEngGetTopLeft(s.mpValue, &tlx, &tly);
    FEngSetTopLeft(s.mpValue, tlx, static_cast<float>((L(s.mValuePosY) + ih) + 1.0f));
    PlaceArrows(s, base.mpLeftArrow, base.mpRightArrow, y);
    PlaceLabel(s.base);
}

void FEToggleImageWidget::Enable()
{
    base.Enable();
    FEngSetScript(mpImage, "UnHighlight", true);
}

void FEToggleImageWidget::Disable()
{
    base.Disable();
    FEngSetScript(mpImage, "Disable", true);
}

void FEToggleImageWidget::Show()
{
    base.Show();
    FEngSetVisible(mpImage);
}

void FEToggleImageWidget::Hide()
{
    base.Hide();
    FEngSetInvisible(mpImage);
}

void FEToggleImageWidget::SetFocus(const char *pkg)
{
    base.SetFocus(pkg);
    FEngSetScript(mpImage, "Highlight", true);
}

void FEToggleImageWidget::UnsetFocus()
{
    base.UnsetFocus();
    FEngSetScript(mpImage, "UnHighlight", true);
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

void PreviewSlider::Toggle(bool on)
{
    cSlider &b = base;
    for (FEObject *o : {b.mpBacking, b.mpHandle, b.mpValue, b.mpCurrent, b.mpEndCap, b.mpEnd, b.mpHandle, mpPreviewBar}) {
        if (on) FEngSetVisible(o);
        else FEngSetInvisible(o);
    }
}

void PreviewSlider::Draw()
{
    cSlider &b = base;
    float x = 0, y = 0;
    FEngGetTopLeft(b.mpBacking, &x, &y);
    FEngSetTopLeft(b.mpEndCap, static_cast<float>((L(x) + b.mMargin) - 4.0L), static_cast<float>(L(y) + 4.0L));
    L avail = L(b.mLength) - 8.0L;
    const float width = 0.0L > avail ? 0.0f : static_cast<float>(avail);   // PC: test 0x41 (NaN kept)
    const float diff = static_cast<float>(L(b.mValue) - mPreview);
    if (diff == 0.0f) {
        FEngSetInvisible(mpPreviewBar);
    } else if (diff > 0.0f) {
        FEngSetVisible(mpPreviewBar);
        FEngSetScript(mpPreviewBar, "ToLesser", true);
    } else if (diff < 0.0f) {
        FEngSetVisible(mpPreviewBar);
        FEngSetScript(mpPreviewBar, "ToGreater", true);
    }
    L v = L(b.mValue) - diff;   // the smaller of value and value - diff
    if (b.mValue < v) v = b.mValue;
    const float t0 = static_cast<float>(v / b.mMax);
    L t;
    if (0.0f > t0) t = 0.0L;
    else if (t0 < 1.0f) t = t0;
    else t = 1.0L;   // also NaN
    FEngSetWidth(b.mpCurrent, static_cast<float>(t * width));
    const float barY = static_cast<float>(L(y) + 9.0L);
    FEngSetTopLeft(b.mpCurrent, static_cast<float>((L(x) + b.mMargin) + 8.0L), barY);
    float brx = barY, bry = x;   // PC: stack slots kept when there is no fill bar
    FEngGetBottomRight(b.mpCurrent, &brx, &bry);
    const float w2 = static_cast<float>(fabsl(L(diff) / b.mMax) * width);
    FEngSetWidth(mpPreviewBar, w2);
    FEngSetTopLeft(mpPreviewBar, brx, barY);
    const float hy = static_cast<float>(L(y) + 7.0L);
    if (diff < 0.0f) FEngSetTopLeft(b.mpHandle, static_cast<float>(L(brx) - 9.0L), hy);
    else FEngSetTopLeft(b.mpHandle, static_cast<float>((L(brx) + w2) - 9.0L), hy);
}

bool ThreeStageSlider::Update(uint32_t msg)
{
    cSlider &b = base.base;
    bool moved = false;
    if (msg == 0x9120409Eu || msg == 0xB5971BF1u) {
        float nv, limit;
        b.mPrev = b.mValue;
        if (msg == 0x9120409Eu) {
            limit = mStageLow;
            const L v = L(b.mValue) - b.mStep;
            nv = v > L(limit) ? static_cast<float>(v) : limit;   // PC: rounded value only when inside; NaN -> limit
        } else {
            limit = base.mPreview;
            const L v = L(b.mStep) + b.mValue;
            nv = v < L(limit) ? static_cast<float>(v) : limit;
        }
        b.mValue = nv;
        moved = !(nv == limit && b.mPrev == limit);
    }
    bridge::cSlider_Draw(this);
    return moved;
}

void ThreeStageSlider::Draw()
{
    cSlider &b = base.base;
    L range = L(b.mMax) - b.mMin;
    if (range == 0.0L) range = 1.0L;
    const L inv = 1.0L / range;
    const float len = b.mLength;
    const float a = static_cast<float>((L(b.mValue) - mStageLow) * inv);
    const float c = static_cast<float>((L(mStageLow) - b.mMin) * inv * len + b.mMargin);
    const float bw = static_cast<float>((L(base.mPreview) - b.mValue) * inv);
    float x = 0, y = 0;
    FEngGetTopLeft(b.mpBacking, &x, &y);
    const float left = static_cast<float>(L(x) + c);
    const float curW = static_cast<float>(L(len) * a);
    FEngSetWidth(b.mpCurrent, curW);
    const float barY = static_cast<float>(L(y) + 5.0L);
    FEngSetTopLeft(b.mpCurrent, left, barY);
    if (base.mpPreviewBar) FEngSetScript(base.mpPreviewBar, "ToLesser", true);   // PC: inlined
    const float right = static_cast<float>(L(left) + curW);
    FEngSetWidth(base.mpPreviewBar, static_cast<float>(L(len) * bw));
    FEngSetTopLeft(base.mpPreviewBar, right, barY);
    FEngSetTopLeft(b.mpHandle, static_cast<float>(L(right) - 4.0L), static_cast<float>(L(y) + 4.0L));
}

TuningSlider *TuningSlider::Construct(int32_t type, const float *pos, const float *size, const float *valueSize,
                                      const float *valuePos, uint32_t minHash, uint32_t maxHash, bool enabled, float yOffset)
{
    base.base.Construct(pos, size, valueSize, valuePos, enabled);
    mMinHash = minHash;
    base.mpLeftArrow = base.mpRightArrow = nullptr;
    mMaxHash = maxHash;
    mpExtra = nullptr;
    base.base.base.vtable = bridge::TuningSliderVTable();
    mTuningType = type;
    // ThreeStageSlider ctor (inlined): cSlider fields, then the subclass vtable; preview / stage limits untouched
    cSlider &s = mSlider.base.base;
    s.mpBacking = s.mpHandle = s.mpValue = s.mpCurrent = s.mpEndCap = s.mpEnd = nullptr;
    s.mMax = s.mMin = 0.0f;
    s.mValue = s.mPrev = s.mInitial = s.mStep = s.mLength = 0.0f;
    s.mbVertical = 0;
    mSlider.base.mpPreviewBar = nullptr;
    s.mpPackage = bridge::GameEmptyString();
    s.mMargin = 46.0f;
    s.vtable = bridge::ThreeStageSliderVTable();
    base.base.base.mLabelOffsetX = 0.0f;
    mYOffset = yOffset;
    return this;
}

void TuningSlider::SetSliderObjects(FEObject *backing, FEObject *handle, FEObject *value, FEObject *current,
                                    FEObject *previewBar, FEObject *endCap, FEObject *end)
{
    cSlider &s = mSlider.base.base;
    s.mpHandle = handle;
    s.mpBacking = backing;
    s.mpValue = value;
    mSlider.base.mpPreviewBar = previewBar;
    s.mpCurrent = current;
    s.mpEndCap = endCap;
    s.mpEnd = end;
    float w, h;
    FEngGetSize(s.mpBacking, &w, &h);
    s.mLength = static_cast<float>(L(w) - (L(s.mMargin) + s.mMargin));
}

void TuningSlider::Position()
{
    FEStatWidget &st = base.base;
    FEWidget &wd = st.base;
    const bool ranges = mMinHash && mMaxHash;
    float aw, ah;
    FEngGetSize(base.mpRightArrow, &aw, &ah);
    const float y = static_cast<float>(L(st.mValuePosY) + mYOffset);
    const float x = st.mValuePosX, sx = st.mValueSizeX;
    const float d = ranges ? -6.0f : 0.0f;   // arrows / bar raised when the range labels go below
    FEngSetTopLeft(st.mpObject, wd.mPosX, static_cast<float>((L(wd.mPosY) + mYOffset) - 3.0f));
    const float ay = static_cast<float>(L(d) + y);
    FEngSetTopLeft(base.mpLeftArrow, x, ay);
    FEngSetTopLeft(base.mpRightArrow, static_cast<float>((L(sx) + x) - aw), ay);
    // the bar fills the space between the arrows (9 px gap each side)
    cSlider &s = mSlider.base.base;
    const float bx = static_cast<float>((L(aw) + x) + 9.0f);
    const float by = static_cast<float>(L(d) + y);
    const L gap = L(bx) - x;
    FEngSetWidth(s.mpBacking, static_cast<float>(sx - (gap + gap)));
    FEngSetTopLeft(s.mpBacking, bx, by);
    float ltx, lty, rbx, rby, bw, bh;
    FEngGetTopLeft(base.mpLeftArrow, &ltx, &lty);
    FEngGetBottomRight(base.mpRightArrow, &rbx, &rby);
    FEngGetSize(s.mpBacking, &bw, &bh);
    const float ly = static_cast<float>(L(bh) + by);   // just below the bar
    if (ranges) {
        FEngSetTopLeft(mpMinLabel, ltx, ly);            // min under the left arrow, max right-aligned to the right arrow
        float px, py;
        FEngGetBottomRight(mpMaxLabel, &px, &py);
        FEngSetBottomRight(mpMaxLabel, rbx, py);
        FEngGetTopLeft(mpMaxLabel, &px, &py);
        FEngSetTopLeft(mpMaxLabel, px, ly);
    }
    if (mpExtra) {
        float px, py, bcx, bcy, ecx, ecy;
        FEngGetTopLeft(mpExtra, &px, &py);
        FEngSetTopLeft(mpExtra, px, ly);
        FEngGetCenter(s.mpBacking, &bcx, &bcy);
        FEngGetCenter(mpExtra, &ecx, &ecy);
        FEngSetCenter(mpExtra, bcx, ecy);
    }
    FEngSetTopLeft(s.mpValue, static_cast<float>(L(x) - 50.0f), static_cast<float>(L(y) - 3.0f));
    wd.mLabelOffsetY = static_cast<float>(L(wd.mLabelOffsetY) - mYOffset);   // PC: shifts again on every call
    bridge::cSlider_Draw(&mSlider);                                          // vtable +0x18: ThreeStageSlider::Draw
}

void TuningSlider::Hide()
{
    base.Hide();
    FEngSetInvisible(mpMinLabel);
    FEngSetInvisible(mpMaxLabel);
    if (mpExtra) FEngSetInvisible(mpExtra);
    bridge::cSlider_Toggle(&mSlider, false);   // vtable +0x0C: PreviewSlider::Toggle
}

void TuningSlider::Show()
{
    base.Show();
    FEngSetVisible(mpMinLabel);
    FEngSetVisible(mpMaxLabel);
    if (mpExtra) FEngSetVisible(mpExtra);
    bridge::cSlider_Toggle(&mSlider, true);
}

void TuningSlider::SetFocus(const char *pkg)
{
    base.SetFocus(pkg);
    bridge::cSlider_Highlight(&mSlider);   // vtable +0x10
    FEngSetScript(mpMinLabel, "Highlight", true);
    FEngSetScript(mpMaxLabel, "Highlight", true);
    if (mpExtra) FEngSetScript(mpExtra, "Highlight", true);
}

void TuningSlider::UnsetFocus()
{
    base.UnsetFocus();
    bridge::cSlider_UnHighlight(&mSlider);   // vtable +0x14
    FEngSetScript(mpMinLabel, "UnHighlight", true);
    FEngSetScript(mpMaxLabel, "UnHighlight", true);
    if (mpExtra) FEngSetScript(mpExtra, "UnHighlight", true);
}

namespace {
// value = v (previous kept), clamped to [min, max] like cSlider::SetCurValue (PC: inlined, v compared unrounded)
void SetSliderValue(cSlider &s, long double v)
{
    s.mPrev = s.mValue;
    s.mValue = static_cast<float>(v);
    if (v > s.mMax) s.mValue = s.mMax;
    if (s.mValue < s.mMin) s.mValue = s.mMin;
}
}  // namespace

void TuningSlider::InitValues()
{
    cSlider &s = mSlider.base.base;
    const int32_t t = mTuningType;
    s.mMin = bridge::Tuning_IsAvailable(t) ? static_cast<float>(bridge::Tuning_GetMin(t)) : 0.0f;
    long double mx = 1.0f;
    if (bridge::Tuning_IsAvailable(t)) {
        mx = bridge::Tuning_GetMax(t);
        if (mx == 0.0f) mx = 1.0f;
    }
    s.mMax = static_cast<float>(mx);
    s.mStep = static_cast<float>(bridge::CarTuneManager_GetStep(t));
    SetSliderValue(s, bridge::Tuning_IsAvailable(t) ? bridge::Tuning_GetValue(t) : 0.0L);
    long double up = 1.0f;
    if (bridge::Tuning_IsAvailable(t)) {
        up = bridge::Tuning_GetUpper(t);
        if (up == 0.0f) up = 1.0f;
    }
    mSlider.base.mPreview = static_cast<float>(up);
    mSlider.mStageLow = bridge::Tuning_IsAvailable(t) ? static_cast<float>(bridge::Tuning_GetLower(t)) : 0.0f;
    float w, h;
    FEngGetSize(s.mpBacking, &w, &h);
    s.mLength = static_cast<float>(L(w) - (L(s.mMargin) + s.mMargin));
}

void TuningSlider::Draw()
{
    bridge::cSlider_Draw(&mSlider);
    const uint32_t name = GetNameHashForTuningType(mTuningType);
    const uint32_t units = GetUnitsHashForTuningType(mTuningType, bridge::TuningUnitType());
    if (units == 0) {
        FEngSetLanguageHash(base.base.mpObject, name);
    } else {
        const char *u = GetLocalizedString(units);   // PC: units looked up first
        const char *n = GetLocalizedString(name);
        bridge::FEPrintfStrings(base.base.mpObject, "%s (%s)", n, u);
    }
    cSlider &s = mSlider.base.base;
    if (mTuningType >= 10 && mTuningType <= 15) {
        bridge::FEPrintfDouble(s.mpValue, "%$2.1f", s.mValue);
    } else {
        if (mpExtra) FEngSetLanguageHash(mpExtra, 0xCBBCB1C5u);
        bridge::FEPrintfDouble(s.mpValue, "%$2.2f", s.mValue);
    }
    if (mMinHash && mMaxHash) {
        FEngSetLanguageHash(mpMinLabel, mMinHash);
        FEngSetLanguageHash(mpMaxLabel, mMaxHash);
    }
}

void TuningSlider::Act(const char *pkg, uint32_t msg)
{
    if (msg == 0x9120409Eu || msg == 0xB5971BF1u) {
        cSlider &s = mSlider.base.base;
        bridge::cSlider_Update(&mSlider, msg);   // vtable +0x04: ThreeStageSlider::Update
        const float v = s.mValue;
        if (bridge::Tuning_IsAvailable(mTuningType)) bridge::Tuning_SetValue(v, mTuningType);
        SetSliderValue(s, bridge::Tuning_IsAvailable(mTuningType) ? bridge::Tuning_GetValue(mTuningType) : 0.0L);
    }
    if (mTuningType >= 9 && mTuningType <= 15) bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0x5FE4BAE4u);
    bridge::TheFEPackageManager()->SendMessageToPackage(bridge::TuningMasterScreenName(), 0xE15F62F8u);
    bridge::FEWidget_BlinkArrows(this, msg);   // vtable +0x40
    bridge::FEWidget_Draw(this);               // vtable +0x0C
}

void TuningSlider::CheckMouse(const char *pkg, float x, float y)
{
    base.CheckMouse(pkg, x, y);
    if (FEngTestForIntersection(x, y, mpExtra)) bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0xC519BFC3u);
}

void FESliderWidget::SetSliderObjects(FEObject *backing, FEObject *handle, FEObject *value, FEObject *current,
                                      FEObject *endCap, FEObject *end)
{
    mSlider.mpBacking = backing;
    mSlider.mpHandle = handle;
    mSlider.mpCurrent = current;
    mSlider.mpValue = value;
    mSlider.mpEndCap = endCap;
    mSlider.mpEnd = end;
    float w, h;
    FEngGetSize(mSlider.mpBacking, &w, &h);
    mSlider.mLength = static_cast<float>(L(w) - (L(mSlider.mMargin) + mSlider.mMargin));
}

uint32_t FEWidget::DetermineSound(uint32_t msg)
{
    uint32_t event;
    switch (msg) {
    case 0x911C0A4Bu: event = 7; break;
    case 0x911AB364u: event = 10; break;
    case 0x72619778u: event = 6; break;
    case 0x9120409Eu: event = 4; break;
    case 0xB5971BF1u: event = 5; break;
    case 0xB5AF2461u: event = 9; break;
    default: return 0;
    }
    return bridge::FEWidget_DetermineSpecialSound(this, msg, event);
}
