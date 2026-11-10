#include "OptionsMain.h"
#include "bridge/Bridge.h"
#include "FEHash.h"
#include "FEngUtil.h"
#include "cFEng.h"

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC GetHelpBarFNGName 50CEA0
constexpr uint32_t kOutObject = 0x47FF4E7Cu;
constexpr uint32_t kOutScript = 0xDE6EFF34u;
const char *Pkg(const IconScrollerMenu &m) { return m.base.mpPackageName; }

// new OM* icon: operator new 0x48, IconOption(name, header, 0), class vtable (PC: a failed allocation adds null)
void AddOM(IconScrollerMenu &m, OMKind k, uint32_t name, uint32_t header)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, 0);
        o->vtable = bridge::GameVTable(OMVTable(k));
    }
    bridge::IconScrollerMenu_AddOption(&m, o);
}

// Header_text / Back_button / Select_button relabelled (PC only)
void SetPauseHeaders(const IconScrollerMenu &m)
{
    const char *pkg = Pkg(m);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Header_text")), 0x1D7BB6C9u);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Back_button")), 0x933DD695u);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Select_button")), 0x8202C364u);
}

void PopHelpBarIfPushed()
{
    if (FEngIsPackagePushed(kHelpBar)) FEngPopPackage(kHelpBar);
}

// both menus end Setup the same way: scroller at the remembered icon, header from it
void FinishSetup(IconScrollerMenu &m)
{
    bridge::IconScroller_SetInitialPos(&m.mScroller, FEngGetLastButton(Pkg(m)));
    bridge::IconScrollerMenu_RefreshHeader(&m);
}
}  // namespace

int32_t OMPage(OMKind k)
{
    static constexpr int32_t kPages[] = {0, 1, 2, 3, 4, 5, 7, 8};
    return kPages[static_cast<int>(k)];
}

uint32_t OMVTable(OMKind k) { return 0x7918B8u + 8u * static_cast<uint32_t>(k); }

void OMReact(OMKind k, uint32_t msg)
{
    if (msg == 0x0C407210u) bridge::SetOptionsPage(OMPage(k));
}

uint32_t FilterSoundForHorizontalMenu(uint32_t, uint32_t event, bool back)
{
    if (static_cast<int32_t>(event) < 6) return event;
    if (event <= 7) return 0;
    if (event != 0xA) return event;
    return back ? 0xAu : 0u;
}

void ExitFEOptionsSystem(const char *pkg)
{
    const uint8_t autosave = *bridge::GameByte(0x844AD8);
    if (autosave && (bridge::OptionsDB_IsDirty() || autosave != *bridge::GameByte(0x836360)))
        bridge::EnterMemcard(pkg, bridge::OptionsReturnScreen(), 0x2042, 0, 0, 0, 0);
    else
        FEngSwitchPackages(pkg, bridge::OptionsReturnScreen(), 0);
    bridge::OptionsDB_DeallocBackup();
}

// ---- UIOptionsMain ----

UIOptionsMain *UIOptionsMain::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    MenuScreen &ms = base.base;
    ms.vtable = bridge::UIOptionsMainVTable();
    const char *pkg = ms.mpPackageName;
    if (bridge::GameModeFlags() == 8) {
        ms.mHelpHash = 0xC5AC0F31u;
        FEngSetInvisible(FEngFindObject(pkg, 0xEB406FECu));
        FEngSetScript(pkg, 0x2BAC0CEEu, "Show", true);
        FEngSetLanguageHash(FEngFindString(ms.mpPackageName, 0xC90DC4ABu), 0xB0FCF98Eu);
        FEngSetScript(ms.mpPackageName, "EA_MESSENGER_GROUP", "Show", true);
    } else {
        FEngSetInvisible(FEngFindObject(pkg, 0x010CBA23u));
        FEngSetInvisible(FEngFindObject(ms.mpPackageName, FEHashUpper("EA_MESSENGER_GROUP")));
    }
    bridge::SetPlayerToEditForOptions(0);
    Setup();
    if (bridge::GameModeFlags() == 8) bridge::HelpBar_SyncByMask(ms.mData.mHelpBarMask | 0x4000u, ms.mpPackageName);
    return this;
}

void UIOptionsMain::Destruct()
{
    base.base.vtable = bridge::UIOptionsMainVTable();
    base.Destruct();
}

UIOptionsMain *UIOptionsMain::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

uint32_t UIOptionsMain::NotifySoundMessage(uint32_t msg, uint32_t event) { return FilterSoundForHorizontalMenu(msg, event, true); }

void UIOptionsMain::Setup()
{
    IconScrollerMenu &m = base;
    AddOM(m, OMKind::Audio, 0xF37AF144u, 0xE76CD783u);
    AddOM(m, OMKind::Video, 0x8A006328u, 0x4204D527u);
    AddOM(m, OMKind::Gameplay, 0x4DF98FB2u, 0xD0CF6EE1u);
    AddOM(m, OMKind::Player, 0xD708EFEFu, 0xF760EABEu);
    AddOM(m, OMKind::Controller, 0xA04A7B26u, 0x04DC6DB5u);
    if (bridge::ProfileState() < 0xC) {
        AddOM(m, OMKind::EATrax, 0xC52CCBF6u, 0xDCFB6B36u);
        AddOM(m, OMKind::Credits, 0x51009E20u, 0x0905101Fu);
    } else {
        AddOM(m, OMKind::Inline, 0x83070A37u, bridge::GameModeFlags() == 0x200 ? 0x19889765u : 0x7D347E05u);
    }
    FinishSetup(m);
}

void UIOptionsMain::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = Pkg(base);
    switch (msg) {
    case 0x1FAB5998u:   // EA Trax shortcut
        bridge::SetOptionsPage(5);
        base.StopDim();
        return;
    case 0x0C407210u:
        if (!FEngIsScriptRunning(pkg, kOutObject, kOutScript) && bridge::OptionsPage() != 6)
            FEngSetScript(pkg, kOutObject, kOutScript, true);
        return;
    case 0x911AB364u:
        if (!FEngIsScriptRunning(pkg, kOutObject, kOutScript)) FEngSetScript(pkg, kOutObject, kOutScript, true);
        return;
    case 0xE1FDE1D1u:
        if (base.mPrevMsg == 0x0C407210u) {
            switch (static_cast<uint32_t>(bridge::OptionsPage())) {
            case 0: case 1: case 2: case 3: case 7: FEngSwitchPackages(pkg, "UI_Options.fng", 0); return;
            case 4: FEngSwitchPackages(pkg, "UI_Options_PC_Controller.fng", 0); return;
            case 5: FEngSwitchPackages(pkg, "EA_Trax_Jukebox.fng", 0); return;
            case 8: FEngSwitchPackages(pkg, "Credits.fng", 0); return;
            default: return;
            }
        }
        if (base.mPrevMsg == 0x911AB364u) {
            FEngSetLastButton(pkg, 0);
            ExitFEOptionsSystem(pkg);
        }
        return;
    default: return;
    }
}

// ---- UIPauseOptionsMain ----

UIPauseOptionsMain *UIPauseOptionsMain::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::UIPauseOptionsMainVTable();
    mbExitToRace = 0;
    Setup();
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, Pkg(base), 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, Pkg(base));
    return this;
}

void UIPauseOptionsMain::Destruct()
{
    base.base.vtable = bridge::UIPauseOptionsMainVTable();
    base.Destruct();
}

UIPauseOptionsMain *UIPauseOptionsMain::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIPauseOptionsMain::Setup()
{
    IconScrollerMenu &m = base;
    SetPauseHeaders(m);
    AddOM(m, OMKind::Audio, 0xF37AF144u, 0xE76CD783u);
    AddOM(m, OMKind::Video, 0x8A006328u, 0x4204D527u);
    AddOM(m, OMKind::Gameplay, 0x4DF98FB2u, 0xD0CF6EE1u);
    AddOM(m, OMKind::Player, 0xD708EFEFu, 0xF760EABEu);
    FinishSetup(m);
}

void UIPauseOptionsMain::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = Pkg(base);
    switch (msg) {
    case 0xB5AF2461u:   // resume race (only with the pausing player's pad plugged in)
        if (!bridge::IsJoystickPluggedIn(bridge::RaceCoordinatorPausePlayer())) return;
        if (FEngIsScriptRunning(pkg, kOutObject, "FORWARD")) return;
        PopHelpBarIfPushed();
        base.StopInput();
        base.StorePrevNotification(0xB5AF2461u, obj, arg, pkgArg);
        FEngSetScript(pkg, kOutObject, "FORWARD", true);
        FEngSetLastButton(pkg, 0);
        FEngSetLastButton("UI_Pause.fng", 0);
        return;
    case 0x911AB364u:
        if (FEngIsScriptRunning(pkg, kOutObject, "FORWARD")) return;
        FEngSetLastButton(pkg, 0);
        FEngSetScript(pkg, kOutObject, "FORWARD", true);
        PopHelpBarIfPushed();
        return;
    case 0xC98356BAu:
        if (!mbExitToRace) return;
        FEngSetLastButton(pkg, 0);
        FEngPopPackage(pkg);
        bridge::RaceCoordinator_RequestUnPause(1);
        return;
    case 0xE1FDE1D1u:
        switch (base.mPrevMsg) {
        case 0x0C407210u: FEngSwitchPackages(pkg, "UI_PauseOptions.fng", 0); return;
        case 0x911AB364u:
            FEngSwitchPackages(pkg, bridge::IsInPerformanceTuning() ? "UI_PerformanceDyno_MAIN.fng" : "UI_Pause.fng", 0);
            return;
        case 0xB5AF2461u: mbExitToRace = 1; return;
        default: return;
        }
    default: return;
    }
}
