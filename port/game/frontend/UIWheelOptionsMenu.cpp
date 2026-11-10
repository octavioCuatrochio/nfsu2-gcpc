#include "UIWheelOptionsMenu.h"
#include "bridge/Bridge.h"
#include "FEngUtil.h"
#include "FEHash.h"
#include "FEObject.h"
#include "MenuScreen.h"
#include "cFEng.h"

namespace {
using L = long double;
float &OptionField(int32_t player, uint32_t offset)
{
    return *reinterpret_cast<float *>(bridge::PlayerControllerOptions(player) + offset);
}
// slider range used by every WCO slider (PC writes min, max, step, then SetCurValue)
void InitSlider(cSlider &s, float min, float value)
{
    s.mMin = min;
    s.mMax = 1.0f;
    s.mStep = 0.1f;   // 0x3DCCCCCD
    s.SetCurValue(value);
}
}  // namespace

uint32_t FilterSoundForMenu(uint32_t, uint32_t event)
{
    return event == 4 || event == 5 ? 0 : event;   // the message is not read on PC
}

void WCOSlider::ActField(uint32_t msg, uint32_t fieldOffset)
{
    if (bridge::cSlider_Update(&base.mSlider, msg)) bridge::FEWidget_BlinkArrows(this, msg);   // +0x40
    OptionField(mPlayer, fieldOffset) = bridge::FEWidget_GetValue(this);                     // +0x48
    bridge::FEWidget_Draw(this);                                                              // +0x0C
}

void WCOWheelRange::Draw() { FEngSetLanguageHash(base.base.base.mpObject, 0x56FEB0D2u); }
void WCOWheelRange::SetInitialValues() { InitSlider(base.base.mSlider, 0.1f, OptionField(base.mPlayer, 0x1C)); }

long double WCOStrengthOfForce::GetValue() const
{
    const float v = base.base.mSlider.mValue;
    L c;
    if (!(v > 0.0f)) c = 0.0L;           // <= 0 or NaN
    else c = 1.0f < v ? 1.0L : L(v);
    return 20.0L - c * 18.0L;
}
void WCOStrengthOfForce::Draw() { FEngSetLanguageHash(base.base.base.mpObject, 0x1CC4B396u); }
void WCOStrengthOfForce::SetInitialValues()
{
    const float x = OptionField(base.mPlayer, 0x20);
    L c;
    if (!(x > 2.0f)) c = 2.0L;           // <= 2 or NaN
    else c = 20.0f < x ? 20.0L : L(x);
    cSlider &s = base.base.mSlider;
    s.mMin = 0.0f;
    s.mMax = 1.0f;
    s.mStep = 0.1f;
    // 1 - (x - 2) * (float)(1/18), rounded to float for the call (x87)
    s.SetCurValue(static_cast<float>(1.0L - (c - 2.0L) * L(0.0555555559694767f)));
}

void WCOOverallForceStrength::Draw() { FEngSetLanguageHash(base.base.base.mpObject, 0x28F42AE8u); }
void WCOOverallForceStrength::SetInitialValues() { InitSlider(base.base.mSlider, 0.0f, OptionField(base.mPlayer, 0x24)); }

UIWheelOptionsMenu *UIWheelOptionsMenu::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    const char *pkg = base.base.mpPackageName;
    base.base.vtable = bridge::UIWheelOptionsMenuVTable();
    mTurningIndex = 0;
    mLinearityIndex = 0;
    const uint32_t lang = 0xD87133BCu + (bridge::PlayerToEditForOptions() != 0 ? 1u : 0u);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("PLAYER")), lang);
    base.mVisibleCount = 8;
    Setup();
    base.mScrollBar.Update(8, 4, 1);
    base.SetInitialOption(0);
    if (bridge::GameModeFlags() == 8) bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask | 0x4000u, base.base.mpPackageName);
    return this;
}

void UIWheelOptionsMenu::Destruct() { base.DestructBody(); }

UIWheelOptionsMenu *UIWheelOptionsMenu::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

void UIWheelOptionsMenu::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg)
{
    base.NotificationMessage(msg, obj, arg, pkg);
    const char *name = base.base.mpPackageName;
    if (msg == 0x911AB364u) {   // back: start the out-script unless it already runs
        if (!FEngIsScriptRunning(name, 0x47FF4E7Cu, 0xDE6EFF34u)) {
            base.mPrevMsg = 0x911AB364u;
            FEngSetScript(name, 0x47FF4E7Cu, 0xDE6EFF34u, true);
        }
    } else if (msg == 0xE1FDE1D1u && base.mPrevMsg == 0x911AB364u) {
        FEngSwitchPackages(name, "UI_Options.fng", 0);
    }
}

uint32_t UIWheelOptionsMenu::NotifySoundMessage(uint32_t msg, uint32_t event) { return FilterSoundForMenu(msg, event); }

void UIWheelOptionsMenu::Setup()
{
    UIWidgetMenu &m = base;
    void *const vtables[3] = {bridge::WCOWheelRangeVTable(), bridge::WCOStrengthOfForceVTable(), bridge::WCOOverallForceStrengthVTable()};
    for (void *vt : vtables) {
        auto *w = static_cast<WCOSlider *>(bridge::Alloc(sizeof(WCOSlider)));
        const int32_t player = bridge::PlayerToEditForOptions();
        w->base.Construct(&m.mStartX, &m.mTitleW, &m.mDataW, &m.mDataX, true);
        w->base.base.base.vtable = vt;
        w->mPlayer = player;
        m.AddSliderOption(&w->base, 1);
    }
}
