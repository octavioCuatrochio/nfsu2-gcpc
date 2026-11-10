#include "PostRaceMenu.h"
#include "bridge/Bridge.h"
#include "cFEng.h"
#include "FEHash.h"
#include "FEngUtil.h"
#include "TuningSlidersMenu.h"   // FEngMapPortToControlMask

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC GetHelpBarFNGName 50CEA0
const char *Pkg(const PostRaceMenu &m) { return m.base.base.mpPackageName; }

void Confirm(const char *pkg, uint32_t yes, uint32_t text)
{
    bridge::DialogInterface_ShowTwoButtons(pkg, "UI_InGameDialog.fng", 0x639u, 0x541389E5u, yes, 0xB4623F67u, 0xB4623F67u, 1, text);
    bridge::FEngSendMessageToPackage(0x96DDA9E6u, pkg);
}

void AddPR(PostRaceMenu &m, PRKind k, uint32_t name, uint32_t header, bool direct)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, 0);
        o->vtable = bridge::GameVTable(PRVTable(k));
        if (direct) o->mbActsDirectly = 1;
    }
    bridge::IconScrollerMenu_AddOption(&m.base, o);   // vtable +0x18
}

// career with autosave on and the career accepting the result: autosave first, then `next`
bool AutoSaveBefore()
{
    return bridge::GameModeFlags() == 1 && *bridge::GameByte(0x844AD8) && bridge::CareerState_5287A0();
}
}  // namespace

uint32_t PRVTable(PRKind k)
{
    static const uint32_t kVT[] = {0x795014, 0x794FF8, 0x79503C, 0x79501C, 0x795024, 0x795034, 0x79502C};
    return kVT[static_cast<int>(k)];
}

void PRReact(PRKind k, const char *pkg, uint32_t msg)
{
    if (msg != 0x0C407210u) return;
    switch (k) {
    case PRKind::NextEvent:
        bridge::PlayerCareerState_54CAE0();
        bridge::RaceCoordinator_PostMessage(0xA, 0);
        return;
    case PRKind::QuitToFreeRoam:
        if (bridge::FrontendDatabase_503080()) Confirm(pkg, 0x451E768Eu, 0x6C328734u);
        else bridge::FEngSendMessageToPackage(0x3411395Cu, pkg);
        return;
    case PRKind::WorldMap: Confirm(pkg, 0x33C42E03u, 0x33C42E03u); return;
    case PRKind::RestartRace: Confirm(pkg, 0xE1A57D51u, 0xAA50AA02u); return;
    case PRKind::Stats:
        *bridge::GameByte(0x7F45A1) = 1;
        FEngSwitchPackages(pkg, "UI_PostRaceStats.fng", 0);
        return;
    case PRKind::Replay:
        FEngSwitchPackages(pkg, "UI_ReplayControl.fng", 0);
        bridge::SetReplayReturnScreen(pkg);   // SetReplayBackScreen 4A4B50
        return;
    case PRKind::QuitToFrontEnd: Confirm(pkg, 0x30ED2368u, 0xE15B558Au); return;
    }
}

void PostRace_SetControlMask(const char *pkg)
{
    const int32_t players = bridge::GameModeFlags() == 4 ? 2 : 1;
    uint32_t mask = 0;
    for (int32_t i = 0; i < players; ++i) mask |= FEngMapPortToControlMask(*bridge::GameInt(0x83A9D8 + 4u * static_cast<uint32_t>(i)));
    FEngSetControlMask(pkg, mask);
}

// ---- PostRaceMenu ----

PostRaceMenu *PostRaceMenu::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::GameVTable(0x797A70);
    mbToFreeRoam = 0;
    Setup();
    return this;
}

void PostRaceMenu::Destruct()
{
    base.base.vtable = bridge::GameVTable(0x797A70);
    bridge::EAXSound_481210(3);
    base.Destruct();
}

PostRaceMenu *PostRaceMenu::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void PostRaceMenu::Setup()
{
    PostRace_SetControlMask(Pkg(*this));
    FEngSetLanguageHash(FEngFindString(Pkg(*this), 0x001335F0u), 0x001335F0u);
    FEngSetLanguageHash(FEngFindString(Pkg(*this), 0xB5074AB4u), 0x79AD657Fu);
    FEngSetInvisible(FEngFindObject(Pkg(*this), FEHashUpper("OL_Countdown_Timer")));
    SetupRace();
    bridge::IconScroller_SetInitialPos(&base.mScroller, FEngGetLastButton(Pkg(*this)));   // scroller +0x20
    bridge::IconScrollerMenu_RefreshHeader(&base);                                         // +0x10
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, Pkg(*this), 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, Pkg(*this));
}

void PostRaceMenu::SetupRace()
{
    if (bridge::GameModeFlags() == 1) {
        if (bridge::CareerState_513FA0()) AddPR(*this, PRKind::NextEvent, 0x12BB5EA2u, 0x23F359B8u, false);
        AddPR(*this, PRKind::QuitToFreeRoam, 0x62AA0DF6u, 0xC3E5BCDCu, true);
        if (NeedToSeeReturnToWorldMap()) AddPR(*this, PRKind::WorldMap, 0x3109913Cu, 0x5E670A9Eu, true);
    }
    AddPR(*this, PRKind::RestartRace, 0xB295A6B6u, 0x80833FC2u, true);
    AddPR(*this, PRKind::Stats, 0x70BB9626u, 0xF82DF37Eu, false);
    if (bridge::GameModeFlags() != 4 && bridge::PostRaceReplayAvailable()) AddPR(*this, PRKind::Replay, 0x84D1D864u, 0x7840428Au, false);
    AddPR(*this, PRKind::QuitToFrontEnd, 0x4C9E34E6u, 0xC7E1420Cu, true);
}

bool PostRaceMenu::NeedToSeeReturnToWorldMap() { return bridge::GameModeFlags() == 1 && *bridge::GameByte(0x85BA67); }
void PostRaceMenu::RestartRace() { bridge::RaceCoordinator_PostMessage(9, 0); }

void PostRaceMenu::QuitToWorldMap()
{
    bridge::PlayerCareerState_NotifyEnterCribFromPostRace();
    bridge::FEngSetFirstFrontendScreen("UI_CareerCrib.fng");
    bridge::RaceCoordinator_PostMessage(0x15, 0);
}

void PostRaceMenu::QuitRaceToFreeRoam()
{
    bridge::PlayerCareerState_513950();
    bridge::RaceCoordinator_PostMessage(0xB, 0);
    if (FEngIsPackagePushed(kHelpBar)) FEngPopPackage(kHelpBar);
}

void PostRaceMenu::QuitRaceToFreeRoamRaceDone()
{
    char movie[64];
    if (bridge::PlayerCareerState_NotifyEnterFreeRoamFromPostRace(movie)) bridge::LaunchPostRaceMovie(Pkg(*this), movie);
    else bridge::RaceCoordinator_PostMessage(0xB, 0);
}

void PostRaceMenu::QuitGame()
{
    if (bridge::GameModeFlags() == 1) {
        bridge::PlayerCareerState_NotifyEnterCribFromPostRace();
        bridge::PlayerCareerState_NotifyExitToMainMenu();
    }
    FEngSetLastButton(Pkg(*this), 0);
    bridge::FEngSetFirstFrontendScreen(bridge::FirstScreenAfterQuitRace());
    bridge::RaceCoordinator_PostMessage(0x15, 0);
}

void PostRaceMenu::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = Pkg(*this);
    switch (msg) {
    case 0x30ED2368u: bridge::FEngSendMessageToPackage(0x24DD40CFu, pkg); return;   // quit confirmed
    case 0x24DD40CFu:
        if (AutoSaveBefore()) {
            bridge::PlayerCareerState_MarkCurrentEventComplete();
            bridge::EnterMemcard(pkg, pkg, 0x441, 0, 0, 0xD78D7BF1u, 0xD78D7BF1u);
        } else {
            bridge::FEngSendMessageToPackage(0xD78D7BF1u, pkg);
        }
        return;
    case 0x33C42E03u: QuitToWorldMap(); return;
    case 0x3411395Cu:
        if (AutoSaveBefore()) {
            mbToFreeRoam = 1;
            bridge::PlayerCareerState_MarkCurrentEventComplete();
            bridge::EnterMemcard(pkg, pkg, 0x441, 0, 0, 0x35E413DDu, 0x35E413DDu);
        } else {
            bridge::FEngSendMessageToPackage(0x35E413DDu, pkg);
        }
        return;
    case 0x35E413DDu: QuitRaceToFreeRoamRaceDone(); return;
    case 0x451E768Eu: QuitRaceToFreeRoam(); return;
    case 0xB4623F67u: bridge::FEngSendMessageToPackage(0x251BF83Eu, pkg); return;   // dialog "no"
    case 0x911AB364u: bridge::PostRaceManager_Back(); return;
    case 0xD78D7BF1u: QuitGame(); return;
    case 0xE1A57D51u: RestartRace(); return;   // restart
    case 0xE1FDE1D1u:
        if (base.mPrevMsg == 0x30ED2368u) bridge::FEngSendMessageToPackage(0x24DD40CFu, pkg);
        else if (base.mPrevMsg == 0xB5AF2461u) bridge::RaceCoordinator_RequestUnPause(1);
        else if (base.mPrevMsg == 0xE1A57D51u) RestartRace();
        return;
    default: return;
    }
}
