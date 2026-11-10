#include "FEWidget.h"
#include "bridge/Bridge.h"
#include "FEngUtil.h"
#include "FEObject.h"
#include "FEString.h"
#include "cFEng.h"

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
    cSlider &s = mSlider;   // PC: cSlider ctor (4FF430) inlined
    s.mpThumb = nullptr;
    for (uint32_t &u : s._unknown10) u = 0;
    s._unknown24 = s._unknown28 = s._unknown2C = 0;
    for (uint32_t &u : s._unknown30) u = 0;
    s._unknown04[0] = 0;
    s.vtable = bridge::cSliderVTable();
    s._unknown08 = bridge::GameEmptyString();
    s._unknown40 = 46.0f;
    return this;
}

uint32_t FESliderWidget::DetermineSpecialSound(uint32_t, uint32_t event) const
{
    if (event == 4) return mSlider._unknown2C != mSlider._unknown28 ? 4 : 0;   // PC: NaN counts as different
    if (event == 5) return mSlider._unknown2C != mSlider._unknown24 ? 5 : 0;
    return event;
}

void FESliderWidget::Position()
{
    FEngSetTopLeft(base.mpObject, base.base.mPosX, base.base.mPosY);
    const float y = base.mValuePosY;
    const float cx = static_cast<float>(L(base.mValueSizeX) * 0.5L + base.mValuePosX);
    float w, h;
    FEngGetSize(mSlider.mpThumb, &w, &h);
    FEngSetTopLeft(mSlider.mpThumb, static_cast<float>(cx - L(w) * 0.5L), y);
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
