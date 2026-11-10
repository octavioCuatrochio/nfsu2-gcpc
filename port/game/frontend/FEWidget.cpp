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

void FEButtonWidget::Position()
{
    using L = long double;
    FEObject *o = mpObject;
    if (o && (reinterpret_cast<const FEString *>(o)->_unknown6C & 1)) {   // centred text: centre on the box
        float cx, cy;
        FEngGetCenter(o, &cx, &cy);
        FEngSetCenter(o, static_cast<float>(L(mWidth) * 0.5f + base.mPosX), cy);
        float tlx, tly;
        FEngGetTopLeft(mpObject, &tlx, &tly);
        FEngSetTopLeft(mpObject, tlx, base.mPosY);
    } else {
        FEngSetTopLeft(o, base.mPosX, base.mPosY);
    }
    if (base.mpLabel)
        FEngSetTopLeft(base.mpLabel, static_cast<float>(L(base.mPosX) - base.mLabelOffsetX), static_cast<float>(L(base.mPosY) - base.mLabelOffsetY));
}

void FEButtonWidget::CheckMouse(const char *pkg, float x, float y)
{
    if (!bridge::FEngTestForIntersection(x, y, mpObject)) return;
    const uint32_t trigger = bridge::FEWidget_DetermineSoundTrigger(this);   // vtable +0x3C
    bridge::PlayFrontendSound(trigger);                                     // table 0x8380B8 -> PlayUISoundFX
    bridge::FEWidget_Act(this, pkg, 0x0C407210u);                           // vtable +0x04
}
