#include "PauseMenu.h"
#include "bridge/Bridge.h"
#include "FEHash.h"
#include "FEngUtil.h"
#include "FEPackageManager.h"
#include "TuningSlidersMenu.h"   // FEngMapPortToControlMask
#include "cFEng.h"

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC GetHelpBarFNGName 50CEA0
constexpr uint32_t kOutObject = 0x47FF4E7Cu;
const char *Pkg(const PauseMenu &m) { return m.base.base.mpPackageName; }

void PopHelpBarIfPushed()
{
    if (FEngIsPackagePushed(kHelpBar)) FEngPopPackage(kHelpBar);
}

// new pm_* icon (operator new 0x48 + IconOption ctor + class vtable, optional mbActsDirectly), added through the
// menu's AddOption slot (PC: a failed allocation adds null)
void AddPM(PauseMenu &m, PMKind k, uint32_t name, uint32_t header, bool direct)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, 0);
        o->vtable = bridge::GameVTable(PMVTable(k));
        if (direct) o->mbActsDirectly = 1;
    }
    bridge::IconScrollerMenu_AddOption(&m.base, o);
}
void AddReplay(PauseMenu &m)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    bridge::IconScrollerMenu_AddOption(&m.base, o ? PMReplayConstruct(o, 0x84D1D864u, 0x7840428Au, 0) : nullptr);
}
// the quit-to-front-end icon closes every Setup; PC uses its ctor 4C27F0 except where the ctor was inlined (mode 8)
void AddQuitToFrontEnd(PauseMenu &m, bool inlinedCtor)
{
    const int32_t mode = bridge::GameModeFlags();
    if (mode == 8 && inlinedCtor) { AddPM(m, PMKind::QuitToFrontEnd, 0x4C9E34E6u, 0xFF956AF2u, true); return; }
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) PMQuitToFrontEndConstruct(o, 0x4C9E34E6u, mode == 0x200 ? 0xF39723A1u : 0xC7E1420Cu, 0);
    bridge::IconScrollerMenu_AddOption(&m.base, o);
}

// "are you sure" dialog of the quit / restart icons; `yes` comes back to the pause menu
void AskConfirm(const char *pkg, uint32_t yes, uint32_t text)
{
    bridge::DialogInterface_ShowTwoButtons(pkg, "UI_InGameDialog.fng", 0x639u, 0x541389E5u, yes, 0xB4623F67u, 0xB4623F67u, 1, text);
    bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0x96DDA9E6u);
}
// PC picks dialog texts with `neg; sbb; and K; add base`: cond ? base + K : base (32-bit wrap)
uint32_t Pick(bool cond, uint32_t k, uint32_t base) { return cond ? base + k : base; }
}  // namespace

uint32_t PMVTable(PMKind k)
{
    static constexpr uint32_t kVt[] = {0x7954D0u, 0x7954D8u, 0x7954E0u, 0x7954E8u, 0x7954F0u, 0x795510u, 0x795518u, 0x795520u, 0x795528u};
    return kVt[static_cast<int>(k)];
}

IconOption *PMReplayConstruct(IconOption *o, uint32_t name, uint32_t header, uint32_t desc)
{
    o->Construct(name, header, desc);
    o->vtable = bridge::GameVTable(0x7954F0u);
    o->mbLocked = !bridge::ReplayAvailable();
    return o;
}

IconOption *PMQuitToFrontEndConstruct(IconOption *o, uint32_t name, uint32_t header, uint32_t desc)
{
    o->Construct(name, header, desc);
    o->vtable = bridge::GameVTable(0x795528u);
    o->mbActsDirectly = 1;
    return o;
}

void PMReact(PMKind k, const char *pkg, uint32_t msg)
{
    if (msg != 0x0C407210u) return;
    const bool alt = k == PMKind::ReturnToWorldMap || k == PMKind::RestartRace || k == PMKind::QuitRaceToFreeRoam ? bridge::RaceCoordinatorAltQuitText() : false;
    switch (k) {
    case PMKind::ResumeRace:
    case PMKind::ResumeFreeRoam:
        bridge::RaceCoordinator_RequestUnPause(k == PMKind::ResumeRace ? 1 : 2);
        FEngSetLastButton(pkg, 0);
        PopHelpBarIfPushed();
        return;
    case PMKind::ReturnToWorldMap: AskConfirm(pkg, 0x33C42E03u, Pick(alt, 0xB39D9AB5u, 0x33C42E03u)); return;
    case PMKind::RestartRace: AskConfirm(pkg, 0xE1A57D51u, Pick(alt, 0xA0DE05B7u, 0x0972A44Bu)); return;
    case PMKind::Replay:
        if (!bridge::ReplayAvailable()) return;
        FEngSwitchPackages(pkg, "ui_ReplayControl.fng", 0);
        bridge::SetReplayReturnScreen(pkg);
        return;
    case PMKind::SwitchToOptions: FEngSwitchPackages(pkg, "UI_PauseOptionsMain.fng", 0); return;
    case PMKind::QuitToCrib: AskConfirm(pkg, 0x43DA9FD0u, 0xC9BA9E3Eu); return;
    case PMKind::QuitRaceToFreeRoam: AskConfirm(pkg, 0x451E768Eu, Pick(alt, 0xA718ECB5u, 0x60B42AFBu)); return;
    case PMKind::QuitToFrontEnd: {
        const int32_t mode = bridge::GameModeFlags();
        uint32_t text;
        if (mode == 8) {
            const bool flag = *bridge::GameByte(0x850098) != 0;
            text = bridge::OnlineSessionActive() ? Pick(flag, 0x244C7F5Du, 0x555BD5B6u) : Pick(flag, 0xEDD4EC9Du, 0xCE50DA14u);
        } else if (mode == 0x200) {
            text = Pick(bridge::OnlineSessionActive(), 0x7F206C34u, 0xD63B6982u);
        } else if (bridge::RaceCoordinatorAltQuitText()) {
            text = 0xE15B558Au;
        } else {
            text = Pick(mode != 1, 0xC1A58CC2u, 0x6F4796A6u);
        }
        AskConfirm(pkg, 0x30ED2368u, text);
        return;
    }
    }
}

int32_t GetPauseMenuReplayIndex()
{
    const int32_t t = bridge::RaceType();
    if (t == 0) return 3;
    return (t == 7 ? 1 : 2) + 1;
}

// ---- PauseMenu ----

PauseMenu *PauseMenu::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::PauseMenuVTable();
    mOpenTime = 0;
    mOpenTime = bridge::RealTimer();
    FEngSetControlMask(Pkg(*this), FEngMapPortToControlMask(bridge::RaceCoordinatorPausePlayer()));
    bridge::SetOptionsPage(0);
    Setup();
    if (!FEngIsPackagePushed(kHelpBar)) {
        FEngShowPackage(kHelpBar, Pkg(*this), 0xFF, 0);
        bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0
        bridge::HelpBar_542990(true);
        bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, Pkg(*this));
    }
    return this;
}

void PauseMenu::Destruct()
{
    base.base.vtable = bridge::PauseMenuVTable();
    PopHelpBarIfPushed();
    base.Destruct();
}

PauseMenu *PauseMenu::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void PauseMenu::Setup()
{
    const char *pkg = Pkg(*this);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Header_text")), 0x6C839FBEu);
    FEngSetLanguageHash(FEngFindString(Pkg(*this), FEHashUpper("Back_button")), 0x933DD695u);
    FEngSetLanguageHash(FEngFindString(Pkg(*this), FEHashUpper("Select_button")), 0x8202C364u);
    if (bridge::RaceType() == 0) SetupExplore();
    else SetupRace();
    FEngSetLastButton(Pkg(*this), 0);
    bridge::IconScroller_SetInitialPos(&base.mScroller, FEngGetLastButton(Pkg(*this)));
    bridge::IconPanel_Update(&base.mScroller);
    bridge::IconScrollerMenu_RefreshHeader(&base);
}

void PauseMenu::SetupExplore()
{
    const int32_t mode = bridge::GameModeFlags();
    bool crib;
    if (mode == 1) {
        if (bridge::RaceType() != 7) AddPM(*this, PMKind::ResumeFreeRoam, 0x62AA0DF6u, 0xC3E5BCDCu, false);
        crib = true;   // PC: race type 7 jumps straight to the crib check
    } else {
        AddPM(*this, PMKind::ResumeFreeRoam, 0x62AA0DF6u, 0xC44A6515u, false);
        crib = bridge::GameModeFlags() == 1;
    }
    if (crib && *bridge::GameByte(0x8635A8)) AddPM(*this, PMKind::QuitToCrib, 0xB8038AC0u, 0x453890E1u, true);
    if (NeedToSeeOptions()) AddPM(*this, PMKind::SwitchToOptions, 0x520DE4E3u, 0xB34993C9u, false);
    const int32_t m = bridge::GameModeFlags();
    if (m != 4 && m != 8 && m != 0x200) AddReplay(*this);
    AddQuitToFrontEnd(*this, true);   // SetupExplore inlines every quit icon
}

void PauseMenu::SetupRace()
{
    if (bridge::ProfileState() >= 3) {
        AddQuitToFrontEnd(*this, true);
        return;
    }
    if (bridge::GameModeFlags() == 1 && *bridge::GameByte(0x85BA67))
        AddPM(*this, PMKind::ReturnToWorldMap, 0x3109913Cu, 0x5E670A9Eu, true);
    AddPM(*this, PMKind::ResumeRace, 0x12BB5EA2u, 0xF6601088u, true);
    int32_t mode = bridge::GameModeFlags();
    if (bridge::RaceType() != 7 && mode != 8 && mode != 0x200) {
        AddPM(*this, PMKind::RestartRace, 0xB295A6B6u, 0x80833FC2u, true);
        mode = bridge::GameModeFlags();
    }
    if (mode == 1 && bridge::RaceType() != 7) AddPM(*this, PMKind::QuitRaceToFreeRoam, 0x62AA0DF6u, 0xDB4E3C0Du, true);
    if (NeedToSeeOptions()) AddPM(*this, PMKind::SwitchToOptions, 0x520DE4E3u, 0xB34993C9u, false);
    mode = bridge::GameModeFlags();
    if (mode != 4 && mode != 8 && mode != 0x200) AddReplay(*this);
    AddQuitToFrontEnd(*this, true);   // mode 8 inline; others through ctor 4C27F0 (same result)
}

bool PauseMenu::OkToHandleInput() const
{
    return mOpenTime != bridge::RealTimer() && bridge::IsJoystickPluggedIn(bridge::RaceCoordinatorPausePlayer()) != 0;
}

bool PauseMenu::NeedToSeeOptions() const
{
    const int32_t pauser = bridge::RaceCoordinatorPausePlayer();
    for (int32_t n = 0; n < 2; ++n) {
        if (!bridge::PlayerExists(n) || bridge::PlayerId(n) != pauser) continue;
        return !bridge::PlayerField50(n) || bridge::RaceType() == 0;
    }
    return false;
}

void PauseMenu::QuitToCrib()
{
    bridge::PlayerCareerState_513B30();
    bridge::FEngSetFirstFrontendScreen("UI_CareerCrib.fng");
    bridge::RaceCoordinator_RequestUnPause(6);
    PopHelpBarIfPushed();
}

void PauseMenu::QuitToWorldMap()
{
    bridge::PlayerCareerState_513B30();
    bridge::FEngSetFirstFrontendScreen("UI_CareerCrib.fng");
    bridge::RaceCoordinator_RequestUnPause(6);
}

void PauseMenu::QuitGame()
{
    bridge::FEngSetFirstFrontendScreen(bridge::FirstScreenAfterQuitRace());
    FEngSetLastButton(Pkg(*this), 0);
    bridge::RaceCoordinator_RequestUnPause(6);
    if (bridge::GameModeFlags() == 1) bridge::PlayerCareerState_NotifyExitToMainMenu();
    PopHelpBarIfPushed();
}

void PauseMenu::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    if (OkToHandleInput()) base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = Pkg(*this);
    auto leave = [&] {   // resume / back: out-script, the end message unpauses
        if (!OkToHandleInput() || FEngIsScriptRunning(pkg, kOutObject, "FORWARD")) return;
        PopHelpBarIfPushed();
        base.StopInput();
        base.StorePrevNotification(msg, obj, arg, pkgArg);
        FEngSetScript(pkg, kOutObject, "FORWARD", true);
        FEngSetLastButton(pkg, 0);
    };
    switch (msg) {
    case 0x30ED2368u: QuitGame(); return;   // quit to front end: yes
    case 0x13999FC2u:
    case 0x4A05AE7Fu: bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0x251BF83Eu); return;
    case 0x0C407210u:
        if (!OkToHandleInput() || FEngIsScriptRunning(pkg, kOutObject, "FORWARD")) return;
        FEngSetScript(pkg, kOutObject, "FORWARD", true);
        PopHelpBarIfPushed();
        return;
    case 0x30F32A49u: bridge::RaceCoordinator_RequestUnPause(4); return;
    case 0x33C42E03u: QuitToWorldMap(); return;
    case 0x43DA9FD0u: QuitToCrib(); return;
    case 0x451E768Eu:   // quit race to free roam: yes
        bridge::RaceCoordinator_PostMessage(0xB, 0);
        PopHelpBarIfPushed();
        return;
    case 0xB4623F67u:   // dialog "no": undim, help bar back
        bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0x251BF83Eu);
        if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, pkg, 0xFF, 0);
        bridge::HelpBar_SetFlag6C(1);
        bridge::HelpBar_542990(true);
        bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, Pkg(*this));
        return;
    case 0x911AB364u:
    case 0xB5AF2461u: leave(); return;
    case 0xE1A57D51u: bridge::RaceCoordinator_RequestUnPause(3); return;   // restart: yes
    case 0xE1FDE1D1u:
        switch (base.mPrevMsg) {
        case 0x30ED2368u: QuitGame(); return;
        case 0x30F32A49u: bridge::RaceCoordinator_RequestUnPause(4); return;
        case 0x43DA9FD0u: QuitToCrib(); return;
        case 0x911AB364u:
        case 0xB5AF2461u: bridge::RaceCoordinator_RequestUnPause(1); return;
        case 0xE1A57D51u: bridge::RaceCoordinator_RequestUnPause(3); return;
        default: return;
        }
    default: return;
    }
}
