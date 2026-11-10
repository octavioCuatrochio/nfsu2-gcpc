#include "CarTunePause.h"
#include "bridge/Bridge.h"
#include "cFEng.h"
#include "FEHash.h"
#include "FEngUtil.h"
#include "FEPackage.h"
#include "FEPackageManager.h"

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC 0x790000
constexpr uint32_t kOutObject = 0x47FF4E7Cu, kOutScript = 0xDE6EFF34u;
const char *Pkg(const CarTunePauseScreen &m) { return m.base.base.mpPackageName; }
int32_t &PauseMode() { return *bridge::GameInt(0x83A644); }        // 0 dyno, 1 tuning
int32_t &Category() { return *bridge::GameInt(0x83A63C); }         // tuning category, 0xFF none
uint8_t &FromCrib() { return *bridge::GameByte(0x83A7A4); }

void PopHelpBarIfPushed()
{
    if (FEngIsPackagePushed(kHelpBar)) FEngPopPackage(kHelpBar);
}
void ShowHelpBarIfNeeded(const char *pkg)
{
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, pkg, 0xFF, 0);
}
// screen create data of `pkg` (FindScreenCreateData 50B840 by FEHashUpper): +0x10 = remembered button
void ResetCreateDataButton(const char *pkg)
{
    if (int32_t *slot = bridge::ScreenLastButtonSlot(FEHashUpper(pkg))) *slot = 0;
}
// confirm dialog (DialogInterface 558790) then 0x96DDA9E6 (dim) to the screen of `pkg` (FEngSendMessageToPackage inlined)
void Confirm(const char *pkg, const char *dlg, uint32_t a, uint32_t b, uint32_t yes, uint32_t no1, uint32_t no2, uint32_t text)
{
    bridge::DialogInterface_ShowTwoButtons(pkg, dlg, a, b, yes, no1, no2, 1, text);
    bridge::FEngSendMessageToPackage(0x96DDA9E6u, pkg);
}
const char *GlobalName(uint32_t addr) { return bridge::GameStringPointer(addr); }
// PC 522710 / 55A580: CarTuneManager (*0x839C40) and the stored specs (*0x83A540) exist, a category is open and
// 59AE90(stored + 0x14, 0x83A544 + category * 0x29) returned 0 (`neg; sbb; inc`)
bool SettingUnchanged()
{
    return bridge::CarTuneManagerActive() && Category() != 0xFF && !bridge::CarTune_59AE90(Category());
}

void AddCT(CarTunePauseScreen &m, CTKind k, uint32_t name, uint32_t header, uint32_t desc, bool direct)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, desc);   // PC: the IconOption ctor inlined (same fields)
        o->vtable = bridge::GameVTable(CTVTable(k));
        o->mbActsDirectly = direct ? 1 : 0;
    }
    bridge::IconScrollerMenu_AddOption(&m.base, o);   // vtable +0x18
}
}  // namespace

uint32_t CTVTable(CTKind k) { return 0x79B720u + 8u * static_cast<uint32_t>(k); }

void CTReact(CTKind k, const char *pkg, uint32_t msg, uint32_t arg4)
{
    if (msg != 0x0C407210u) return;
    switch (k) {
    case CTKind::Resume:
        PopHelpBarIfPushed();
        bridge::RaceCoordinator_RequestUnPause(1);
        ResetCreateDataButton(pkg);
        return;
    case CTKind::Restart:
        Confirm(pkg, "UI_InGameDialog.fng", 0x639u, 0x541389E5u, 0xE1A57D51u, 0x34DC1BCFu, 0x34DC1BCFu, 0x0972A44Bu);
        return;
    case CTKind::DynoChart: FEngSwitchPackages(pkg, GlobalName(0x7FA230), 0); return;
    case CTKind::QuitToTuning:
        Confirm(pkg, "", 0xF770u, 0x61Cu, 0x526ADEFCu, 0x34DC1BCFu, 0x34DC1BCFu, 0x455C3F80u);
        return;
    case CTKind::Options: FEngSwitchPackages(pkg, "UI_PauseOptionsMain.fng", 0); return;
    case CTKind::TuningMaster: FEngSwitchPackages(pkg, GlobalName(0x7FA214), 0); return;
    case CTKind::BackToDyno: {
        PopHelpBarIfPushed();
        bridge::PresetCarSlot_DefaultCustomTuningSpecs(-1);   // 59AFE0 on 0x83A611
        bridge::CarTune_CopyStoredSpecs();                    // 0x83A611 = *(0x83A540) + 0x14 (0x29 bytes) if set
        bridge::CarTune_5B7ED0(0);
        FromCrib() = 0;
        PauseMode() = 0;
        ResetCreateDataButton(pkg);
        if (bridge::RaceCoordinatorState() == 6) bridge::RaceCoordinator_RequestUnPause(1);
        bridge::Race_SimulateRaceFinish();
        // the arg (pad mask) picks the menu port of the tuning player
        int32_t port;
        if (arg4 & 1) port = 0;
        else if (arg4 & 2) port = 1;
        else if (arg4 & 4) port = 2;
        else port = (arg4 & 8) ? 3 : -1;
        *bridge::GameInt(0x83A9D8 + 4u * static_cast<uint32_t>(*bridge::GameInt(0x83A64C))) = port;
        return;
    }
    case CTKind::DiscardChanges:
        Confirm(pkg, "", 0xF770u, 0x61Cu, 0x51123592u, 0x34DC1BCFu, 0x34DC1BCFu, 0x0D7DE8ABu);
        return;
    case CTKind::QuitTuning: {
        *bridge::GameStringSlot(0x83A650) = GlobalName(0x7FA21C);   // UI_PerformanceDyno_MAIN.fng
        bridge::RaceCoordinator_RequestPause(bridge::TuningPlayerId());
        const int32_t mode = bridge::GameModeFlags();
        uint32_t text = 0x40813F63u;
        if (mode == 0x100) text = 0xFC820867u;
        else if (mode == 1) text = *bridge::GameInt(0x83A640) ? 0xE5A0D45Eu + 0xCE5A854Bu : 0xE5A0D45Eu;
        Confirm(pkg, "", 0xF770u, 0x61Cu, 0x30ED2368u, 0x34DC1BCFu, 0x34DC1BCFu, text);
        return;
    }
    case CTKind::GoToSetting:
        if (SettingUnchanged()) {
            bridge::DialogInterface_ShowTwoButtons(pkg, "", 0xF770u, 0x61Cu, 0x2B5DFEF6u, 0xB4EDEB6Du, 0x34DC1BCFu, 1, 0xA2323032u);
            return;
        }
        FEngSwitchPackages(pkg, GlobalName(0x7FA218), 0);
        return;
    }
}

// ---- CarTunePauseScreen ----

CarTunePauseScreen *CarTunePauseScreen::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::GameVTable(0x79D30C);
    mOpenTime = 0;
    mbSaving = 0;
    ShowHelpBarIfNeeded(Pkg(*this));
    bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0 inlined
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, Pkg(*this));
    *bridge::GameByte(0x838528) = 0;
    *bridge::GameByte(0x83A655) = 1;
    bridge::SetCarTuningSpec();
    *bridge::GameStringSlot(0x83A650) = Pkg(*this);
    mOpenTime = bridge::RealTimer();
    Setup();
    return this;
}

void CarTunePauseScreen::Destruct()
{
    base.base.vtable = bridge::GameVTable(0x79D30C);
    base.Destruct();
}

CarTunePauseScreen *CarTunePauseScreen::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

uint32_t CarTunePauseScreen::NotifySoundMessage(uint32_t event) { return event == 4 || event == 5 ? 0 : event; }

bool CarTunePauseScreen::IsOkToResume() const
{
    if (mOpenTime == bridge::RealTimer()) return false;
    if (PauseMode() != 0) return true;
    return bridge::IsJoystickPluggedIn(bridge::RaceCoordinatorPausePlayer());
}

void CarTunePauseScreen::Setup()
{
    if (PauseMode() == 0) SetupDynoPause();
    else if (PauseMode() == 1) SetupTuningPause();
    const int32_t *slot = bridge::ScreenLastButtonSlot(FEHashUpper(Pkg(*this)));
    bridge::IconScroller_SetInitialPos(&base.mScroller, slot ? *slot : 0);   // scroller +0x20
    bridge::IconScrollerMenu_RefreshHeader(&base);                                                             // +0x10
    ShowHelpBarIfNeeded(Pkg(*this));
    bridge::HelpBar_SyncByMask(FromCrib() ? 4u : 0x404u, Pkg(*this));
}

void CarTunePauseScreen::SetupDynoPause()
{
    AddCT(*this, CTKind::Resume, 0x12BB5EA2u, 0x105149CBu, 0, false);
    AddCT(*this, CTKind::Restart, 0xB295A6B6u, 0x80833FC2u, 0, false);
    AddCT(*this, CTKind::TuningMaster, 0xD4060FC4u, 0xA34E13BDu, 0, false);
    AddCT(*this, CTKind::QuitToTuning, 0xFE1E723Eu, 0x18D5A136u, 0, false);
    AddCT(*this, CTKind::Options, 0x520DE4E3u, 0xB34993C9u, 0, false);
    AddCT(*this, CTKind::QuitTuning, 0x4C9E34E6u, 0xDE5CDFB2u, 0, true);
    // the pausing player's port -> all four control masks of the package
    const int32_t port = bridge::RaceCoordinatorPausePlayer();
    *bridge::GameInt(0x83A7A0) = port;
    uint32_t mask;
    if (port == 0) mask = 1;
    else if (port == 1) mask = 2;
    else if (port == 2) mask = 4;
    else mask = port == 3 ? 8 : 0;
    if (FEPackage *p = FEngFindPackage(Pkg(*this))) {
        p->mu32ControlMask = p->mu32SavedControlMask = p->mu32InputControlMask = p->mu32SavedInputControlMask = mask;
    }
}

void CarTunePauseScreen::SetupTuningPause()
{
    AddCT(*this, CTKind::TuningMaster, 0xD4060FC4u, 0xA34E13BDu, 0, false);
    AddCT(*this, CTKind::BackToDyno, 0xF188098Cu, 0xF62234EFu, 0, false);
    AddCT(*this, CTKind::DynoChart, 0xFE1E723Eu, 0x436C91D4u, 0, false);
    AddCT(*this, CTKind::GoToSetting, 0xC2B2829Cu, 0xFFE08459u, 0, SettingUnchanged());   // ctor 522710
    if (bridge::CarTune_50FD10()) AddCT(*this, CTKind::DiscardChanges, 0x73FE99B7u, 0x410258C3u, 0, true);   // ctor 5226C0
    AddCT(*this, CTKind::QuitTuning, 0x4C9E34E6u, 0xDE5CDFB2u, 0, true);
}

void CarTunePauseScreen::RefreshHeader()
{
    base.RefreshHeader();
    static const uint32_t kNames[5] = {0x5AE1D9E5u, 0x5B4967F0u, 0xC476882Bu, 0xCA169C60u, 0xCC76DEE5u};
    const uint32_t c = static_cast<uint32_t>(Category());
    if (c <= 4) FEngSetLanguageHash(Pkg(*this), 0x242217C1u, kNames[c]);
    else bridge::FEngPrintfString(Pkg(*this), 0x242217C1u, "");
}

void CarTunePauseScreen::DoQuitTuning()
{
    PopHelpBarIfPushed();
    const char *next = nullptr;
    switch (*bridge::GameInt(0x83A640)) {
    case 0:
        next = "UI_CareerCrib.fng";
        *bridge::GameByte(0x8635B9) = 1;
        break;
    case 1: next = "UI_ChoosePerformanceCategory.fng"; break;
    case 2: next = "UI_ChooseCustomizeCategory.fng"; break;
    default: break;
    }
    *bridge::GameStringSlot(0x8384C8) = next;
    ResetCreateDataButton(Pkg(*this));
    *bridge::GameInt(0x839C40) = 0;   // CarTuneManager
    *bridge::GameInt(0x83A64C) = 0;
    *bridge::GameInt(0x83A540) = 0;
    Category() = 0xFF;
    bridge::CarTune_5B7ED0(0);
    FromCrib() = 0;
    *bridge::GameByte(0x83A655) = 0;
    *bridge::GameByte(0x83A654) = 0;
    bridge::RaceCoordinator_RequestUnPause(6);
}

void CarTunePauseScreen::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    if (FromCrib() && msg == 0x911AB364u) return;
    if (mOpenTime != bridge::RealTimer()) base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = Pkg(*this);
    auto resume = [&] {
        if (!IsOkToResume() || FEngIsScriptRunning(pkg, kOutObject, kOutScript)) return;
        FEngSetScript(pkg, kOutObject, kOutScript, true);
        PopHelpBarIfPushed();
    };
    switch (msg) {
    case 0x159C894Du:
        bridge::CarTune_RestoreCustomTunings(Category());
        base.StopDim();
        return;
    case 0x1FAB5998u:
        bridge::CarTune_50FAE0(Category());
        DoQuitTuning();
        return;
    case 0x24DD40CFu:   // save first
        bridge::CarTune_50FAE0(Category());
        mbSaving = 1;
        bridge::EnterMemcard(pkg, pkg, 0x151, 0, 0, 0, 0);
        return;
    case 0x2B5DFEF6u:
        bridge::CarTune_50FC20(Category());
        [[fallthrough]];
    case 0xB4EDEB6Du: FEngSwitchPackages(pkg, GlobalName(0x7FA218), 0); return;
    case 0x34DC1BCFu: base.StopDim(); return;
    case 0x30ED2368u:   // quit tuning confirmed
        if (bridge::CarTune_50FD10() && *bridge::GameByte(0x844AD8)) {
            bridge::DialogInterface_ShowTwoButtons(pkg, "", 0xF770u, 0x61Cu, 0x24DD40CFu, 0x1FAB5998u, 0x34DC1BCFu, 1, 0x71851654u);
            base.StartDim();
            return;
        }
        DoQuitTuning();
        return;
    case 0x35F8620Bu: bridge::cFEng_RegisterJoy(); return;
    case 0x461A18EEu:
    case 0x8867412Du: DoQuitTuning(); return;
    case 0x51123592u: bridge::EnterMemcard(pkg, pkg, 0x51, 0, 0, 0, 0); return;
    case 0x526ADEFCu: {   // back to tuning from the dyno
        PopHelpBarIfPushed();
        bridge::CarTune_50FDF0();
        *bridge::GameByte(0x83A655) = 0;
        bridge::CarTune_50F7E0(1);
        PauseMode() = 1;
        bridge::RaceCoordinator_RequestUnPause(0xA);
        const bool kph = bridge::Player_IsHudKPH(*bridge::GameInt(0x83A64C));
        *reinterpret_cast<int32_t *>(bridge::CarTuneStoredSpecs() + 4) = kph ? 1 : 0;   // *(0x83A540) + 4
        return;
    }
    case 0x8FB8A616u:
        bridge::CarTune_50FC20(Category());
        base.StopDim();
        return;
    case 0xB4623F67u:   // dialog "no"
        bridge::FEngSendMessageToPackage(0x251BF83Eu, pkg);
        ShowHelpBarIfNeeded(pkg);
        bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, pkg);
        return;
    case 0xB5AF2461u:
        if (FromCrib()) return;
        base.StorePrevNotification(msg, obj, arg, pkgArg);
        resume();
        return;
    case 0x0C407210u:
    case 0x911AB364u: resume(); return;
    case 0xE1A57D51u:   // restart
        PopHelpBarIfPushed();
        bridge::CarTune_50FDF0();
        bridge::RaceCoordinator_RequestUnPause(3);
        FEngSetLastButton(pkg, 0);
        return;
    case 0xE1FDE1D1u:
        if (base.mPrevMsg != 0x911AB364u && base.mPrevMsg != 0xB5AF2461u) return;
        PopHelpBarIfPushed();
        bridge::RaceCoordinator_RequestUnPause(1);
        FEngSetLastButton(pkg, 0);
        PopHelpBarIfPushed();
        return;
    default: return;
    }
}
