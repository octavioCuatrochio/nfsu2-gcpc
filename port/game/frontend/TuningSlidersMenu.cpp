#include "TuningSlidersMenu.h"
#include "bridge/Bridge.h"
#include "FEWidget.h"
#include "FEngUtil.h"
#include "FEHash.h"
#include "FEObject.h"
#include "FEPackage.h"
#include "FEPackageManager.h"
#include "MenuScreen.h"
#include "cFEng.h"
#include <cmath>

namespace {
using L = long double;
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC 0x790000

// PC inlines this in every PC-help-bar screen constructor (55AC10, 55AD70, ...): push the help bar over the screen
// if needed, then show it and sync its buttons to the screen's mask.
void PushHelpBar(const MenuScreen &s)
{
    if (!FEngIsPackagePushed(kHelpBar))
        if (cFEng *fe = bridge::cFEngInstance()) {
            if (s.mpPackageName) fe->Pusher(4, kHelpBar, s.mpPackageName, 0xFF, 0);
            else fe->Pusher(0, kHelpBar, nullptr, 0xFF, 0);
        }
    bridge::HelpBar_SetFlag6C(1);
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(s.mData.mHelpBarMask, s.mpPackageName);
}

// FEngSetControlMask(name, FEngMapPortToControlMask(port)) unless every port drives the frontend (PC inlines the
// mapping and the package lookup in Setup).
void SetMaskForActivePort(const char *pkg)
{
    if (bridge::FEngAllPortsFlag() == 0) FEngSetControlMask(pkg, FEngMapPortToControlMask(bridge::FEngActivePort()));
}

void ClearLastButton(const char *pkg)
{
    if (int32_t *slot = bridge::ScreenLastButtonSlot(FEHashUpper(pkg))) *slot = 0;
}

// every row: InitValues, then Draw (virtual). PC walks the list from the head (rows are TuningSliders).
void ReinitAll(TuningSlidersMenu &m)
{
    FEWidget *end = uiwm::FromNode(&m.base.mWidgets);
    for (FEWidget *w = uiwm::FromNode(m.base.mWidgets.next); w != end; w = uiwm::FromNode(uiwm::NextOf(w))) {
        reinterpret_cast<TuningSlider *>(w)->InitValues();
        bridge::FEWidget_Draw(w);
    }
}
}  // namespace

uint32_t FEngMapPortToControlMask(int32_t port)
{
    if (port == 0) return 1;
    if (port == 1) return 2;
    if (port == 2) return 4;
    return port == 3 ? 8u : 0u;
}

TuningSlidersMenu *TuningSlidersMenu::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::TuningSlidersMenuVTable();
    mbLeaving = 0;
    mStartTime = 0;
    PushHelpBar(base.base);
    bridge::SetShowEAMIcons(false);
    base.mVisibleCount = bridge::TuningCategory() == 1 ? 7 : 6;
    mStartTime = bridge::RealTimer();
    base.mpCurrent = nullptr;
    base.mbSoundsEnabled = 0;
    Setup();
    return this;
}

void TuningSlidersMenu::Destruct() { base.DestructBody(); }

TuningSlidersMenu *TuningSlidersMenu::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

bool TuningSlidersMenu::IsOkToResume() const { return mStartTime != bridge::RealTimer(); }

uint32_t TuningSlidersMenu::GetTipHash(int32_t type)
{
    switch (type) {
    case 0: case 4: return 0xD7C2416Au;
    case 1: case 5: return 0x6AD0772Fu;
    case 2: case 6: return 0xC4553F70u;
    case 3: case 7: return 0xF51BD160u;
    case 8: return 0x184AB1A3u;
    case 9: return 0x2EA43025u;
    case 10: case 11: case 12: case 13: case 14: case 15: return 0xE2E98554u;
    case 0x12: return 0x76E721C7u;
    case 0x13: case 0x14: return 0xC7E6E649u;
    case 0x15: case 0x16: return 0x9DCC6E8Bu;
    default: return 0;
    }
}

int32_t TuningSlidersMenu::AddTuningSlider(int32_t type, uint32_t minHash, uint32_t maxHash, bool plainArrows, float yOffset)
{
    if (!bridge::Tuning_IsAvailable(type)) return -1;
    auto *t = static_cast<TuningSlider *>(bridge::Alloc(sizeof(TuningSlider)));
    UIWidgetMenu &m = base;
    // PC: a failed allocation would go on with a null widget (crash); the availability is asked again.
    t->Construct(type, &m.mStartX, &m.mTitleW, &m.mDataW, &m.mDataX, minHash, maxHash, bridge::Tuning_IsAvailable(type), yOffset);
    FEStatWidget &s = t->base.base;
    s.mpObject = m.GetCurrentFEString(m.mOptionName);
    // evaluation order of the original (last argument first)
    FEObject *end = m.GetCurrentFEImage(m.mSliderEnd);
    FEObject *beg = m.GetCurrentFEImage(m.mSliderBeg);
    FEObject *preview = m.GetCurrentFEImage("SLIDER_B_HANDLE_");
    FEObject *handle = m.GetCurrentFEImage(m.mSliderHandle);
    FEObject *value = m.GetCurrentFEString(m.mOptionData);
    FEObject *knob = m.GetCurrentFEImage(m.mSliderKnob);
    FEObject *backing = m.GetCurrentFEImage(m.mSliderBase);
    t->SetSliderObjects(backing, knob, value, handle, preview, beg, end);
    s.base.mpLabel = m.GetCurrentFEObject(m.mOptionBacking);
    t->InitValues();
    t->mpMinLabel = m.GetCurrentFEString("OPTION_DATA_B_");
    t->mpMaxLabel = m.GetCurrentFEString("OPTION_DATA_C_");
    if (type < 9 || type > 15) t->mpExtra = m.GetCurrentFEString("TIP_TEXT_");
    t->base.mpLeftArrow = m.GetCurrentFEImage(m.mLeftArrow);
    t->base.mpRightArrow = m.GetCurrentFEImage(m.mRightArrow);
    uiwm::SetArrowTextures(t->base.mpLeftArrow, t->base.mpRightArrow, plainArrows ? 1 : 2);
    FEWidget *w = &s.base;
    uiwm::Append(m, w);
    ++m.mCurrentIndex;
    m.mStartY = static_cast<float>((L(m.mSpacing) + m.mStartY) + m.mRowHeight);
    m.mDataY = static_cast<float>((L(m.mSpacing) + m.mDataY) + m.mRowHeight);
    t->mSlider.base.base.mMargin = 4.0f;
    bridge::FEWidget_Position(w);                // slot 4
    float bw, bh;
    FEngGetSize(t->mSlider.base.base.mpBacking, &bw, &bh);
    t->mSlider.base.base.mLength = static_cast<float>(L(bw) - (L(t->mSlider.base.base.mMargin) + t->mSlider.base.base.mMargin));
    bridge::FEWidget_Show(w);                    // slot 5
    bridge::FEWidget_Draw(w);                    // slot 3
    s.base.mSizeX = uiwm::RowWidth(w, t->base.mpRightArrow);
    return static_cast<int32_t>(m.mCurrentIndex - 1);
}

void TuningSlidersMenu::SetupSuspension()
{
    AddTuningSlider(3, 0x26033125u, 0xE66708F3u, false, 0.0f);
    AddTuningSlider(7, 0x26033125u, 0xE66708F3u, false, 0.0f);
    AddTuningSlider(0, 0xE66D2A8Fu, 0xE666E852u, false, 0.0f);
    AddTuningSlider(4, 0xE66D2A8Fu, 0xE666E852u, false, 0.0f);
    AddTuningSlider(1, 0xE66D2A8Fu, 0xE666E852u, false, 0.0f);
    AddTuningSlider(5, 0xE66D2A8Fu, 0xE666E852u, false, 0.0f);
    AddTuningSlider(2, 0xE66D2A8Fu, 0xE666E852u, false, 0.0f);
    AddTuningSlider(6, 0xE66D2A8Fu, 0xE666E852u, false, 0.0f);
    AddTuningSlider(8, 0xE6692B0Au, 0xE669E1C6u, false, 0.0f);
}

void TuningSlidersMenu::SetupDrivetrain()
{
    for (int32_t type = 10; type <= 15; ++type) AddTuningSlider(type, 0, 0, false, 0.0f);
    // speed axis labels 0xAEA3C019.. (PC: byte 0x83AA1A == 1 -> 60-step km/h, else 35-step mph)
    const bool metric = bridge::SpeedUnitsMetric();
    const char *pkg = base.base.mpPackageName;
    bridge::FEngPrintfInt(pkg, 0xAEA3C019u, "%d", 0);
    for (int32_t i = 1; i <= 7; ++i) bridge::FEngPrintfInt(pkg, 0xAEA3C019u + static_cast<uint32_t>(i), "%d", (metric ? 60 : 35) * i);
    AddTuningSlider(9, 0xCEBFAA36u, 0x95A7073Du, false, 10.0f);
}

void TuningSlidersMenu::SetupAero()
{
    AddTuningSlider(0x15, 0xE66F30DBu, 0x36C30E70u, false, 0.0f);
    AddTuningSlider(0x16, 0xE66F30DBu, 0x36C30E70u, false, 0.0f);
}

void TuningSlidersMenu::SetupWheelBrakes()
{
    AddTuningSlider(0x13, 0xE66F30DBu, 0x36C30E70u, false, 0.0f);
    AddTuningSlider(0x14, 0xE66F30DBu, 0x36C30E70u, false, 0.0f);
    AddTuningSlider(0x12, 0x001C0CC9u, 0x02CBDC28u, true, 0.0f);
}

void TuningSlidersMenu::Setup()
{
    switch (bridge::TuningCategory()) {
    case 0: SetupSuspension(); break;
    case 1: SetupDrivetrain(); break;
    case 2: SetupAero(); break;
    case 3: SetupWheelBrakes(); break;
    default: break;
    }
    const int32_t *last = bridge::ScreenLastButtonSlot(FEHashUpper(base.base.mpPackageName));   // FEngGetLastButton
    base.SetInitialOption(last ? *last : 0);
    FEWidget *cur = base.mpCurrent;
    if (cur && cur != uiwm::FromNode(&base.mWidgets)) bridge::FEWidget_UnsetFocus(cur);   // slot 10
    SetMaskForActivePort(bridge::TuningSlidersScreenName());
    if (!FEngIsPackagePushed(kHelpBar))
        if (cFEng *fe = bridge::cFEngInstance()) {
            if (base.base.mpPackageName) fe->Pusher(4, kHelpBar, base.base.mpPackageName, 0xFF, 0);
            else fe->Pusher(0, kHelpBar, nullptr, 0xFF, 0);
        }
}

void TuningSlidersMenu::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg)
{
    base.NotificationMessage(msg, obj, arg, pkg);
    const char *name = base.base.mpPackageName;
    auto *cur = reinterpret_cast<TuningSlider *>(base.mpCurrent);
    // first switch: sounds (some cases go on below)
    bool reset = false, tip = false, back = false;
    switch (msg) {
    case 0x9120409Eu: bridge::PlayUISound(9); return;
    case 0x72619778u: bridge::PlayUISound(0); return;
    case 0x911AB364u: bridge::MakeFrontendSound(10, 0); back = true; break;   // PC 51DE60(pkg, 10): table row 10
    case 0x911C0A4Bu: bridge::PlayUISound(1); return;
    case 0xB5971BF1u: bridge::PlayUISound(8); return;
    case 0xC519BFC0u: bridge::PlayUISound(0xB); reset = true; break;
    case 0xC519BFC3u: bridge::MakeFrontendSound(2, 0); tip = true; break;
    default: break;
    }
    if (reset) {   // "reset to default" on the selected row, then every row re-reads its value
        if (!cur) return;
        bridge::TuneManager_ResetToDefault(cur->mTuningType);
        if (cur->mTuningType >= 9 && cur->mTuningType <= 15) {
            bridge::TuneManager_RebuildTheRideInfo();
            bridge::TuneManager_RebuildPlayersRideInfo();
        }
        ReinitAll(*this);
        return;
    }
    if (tip) {
        bridge::DialogInterface_ShowOk(name, "", GetTipHash(cur->mTuningType));
        return;
    }
    if (back) {
        if (cur) bridge::FEWidget_UnsetFocus(cur);
        ClearLastButton(name);
        FEngSetScript(name, 0x47FF4E7Cu, 0xDE6EFF34u, true);
        FEngSetScript(bridge::TuningMasterScreenName(), "BUTTON_MASTER", "FadeIn", true);
        bridge::TheFEPackageManager()->SendMessageToPackage(bridge::TuningMasterScreenName(), 0x731BF236u);   // 55DDA0
        return;
    }
    switch (msg) {
    case 0xB5AF2461u:   // accept: rebuild the car physics, drop the help bar, unpause
        if (mStartTime == bridge::RealTimer() || bridge::TuningAcceptBlocked()) return;
        mbLeaving = 1;
        bridge::BuildPlayerCarPhysicsInfo();
        if (FEngIsPackagePushed(kHelpBar)) FEngPopPackage(kHelpBar);
        bridge::RaceCoordinator_RequestUnPause(1);
        return;
    case 0x5FE4BAE4u:   // values changed: PC re-reads every row (both branches of the GC type-9 test are identical)
        ReinitAll(*this);
        return;
    case 0x0C407210u:
        bridge::FEWidget_SetFocus(cur, name);   // slot 9 (no null check)
        return;
    case 0x35F8620Bu:
        base.RefreshWidgets();
        return;
    case 0xC98356BAu:
        if (!mbLeaving) return;
        if (cFEng *fe = bridge::cFEngInstance()) fe->Popper(name);
        if (bridge::FEngAllPortsFlag() == 0)
            FEngSetControlMask(bridge::TuningMasterScreenName(), FEngMapPortToControlMask(bridge::FEngActivePort()));
        else
            FEngSetControlMask(bridge::TuningMasterScreenName(), 0xFF);
        return;
    case 0xE1FDE1D1u:
        mbLeaving = 1;
        ClearLastButton(name);
        return;
    default:
        return;
    }
}
