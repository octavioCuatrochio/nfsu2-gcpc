#include "UIPauseOptions.h"
#include "bridge/Bridge.h"
#include "FEWidget.h"
#include "FEngUtil.h"
#include "FEHash.h"
#include "FEObject.h"
#include "FEPackageManager.h"
#include "MenuScreen.h"
#include "OptionWidgets.h"
#include "cFEng.h"
#include <initializer_list>

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";          // PC GetHelpBarFNGName 50CEA0 -> 0x790000
constexpr const char *kPauseMain = "UI_PauseOptionsMain.fng";   // PC 0x79071C
constexpr uint32_t kOutObject = 0x47FF4E7Cu;                      // object running the page's in/out scripts
constexpr uint32_t kOutScript = 0xDE6EFF34u;
const char *Pkg(const UIPauseOptions &s) { return s.base.base.mpPackageName; }

// PC inlines the widget news: operator new, stock ctor at the menu's row geometry, the class vtable (a failed
// allocation passes null to Add*Option).
template <typename W> W *Alloc(uint32_t size) { return static_cast<W *>(bridge::Alloc(size)); }
FESliderWidget *NewSlider(UIWidgetMenu &m, uint32_t vtable, uint32_t size = sizeof(FESliderWidget))
{
    auto *w = Alloc<FESliderWidget>(size);
    if (w) {
        w->Construct(&m.mStartX, &m.mTitleW, &m.mDataW, &m.mDataX, true);
        w->base.base.vtable = bridge::GameVTable(vtable);
    }
    return w;
}
FEToggleWidget *NewToggle(UIWidgetMenu &m, uint32_t vtable, uint32_t size = sizeof(FEToggleWidget))
{
    auto *w = Alloc<FEToggleWidget>(size);
    if (w) {
        w->Construct(&m.mStartX, &m.mTitleW, &m.mDataW, &m.mDataX, true);
        w->base.base.vtable = bridge::GameVTable(vtable);
    }
    return w;
}
FEButtonWidget *NewButton(UIWidgetMenu &m, uint32_t vtable)
{
    auto *w = Alloc<FEButtonWidget>(sizeof(FEButtonWidget));
    if (w) {
        w->Construct(&m.mStartX, &m.mTitleW, true);
        w->base.vtable = bridge::GameVTable(vtable);
    }
    return w;
}
// AddToggleOption(w, true) (PC 54F170) is the form every pause page uses except the career difficulty row.
uint32_t AddToggle(UIWidgetMenu &m, uint32_t vt, uint32_t size = sizeof(FEToggleWidget)) { return m.AddToggleOption(NewToggle(m, vt, size), true); }

// every page relabels the shared header / button strings (PC only; GC pages keep the package's text)
void SetHeaders(const UIPauseOptions &s, uint32_t page)
{
    const char *pkg = Pkg(s);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Header_text")), 0x1D7BB6C9u);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Header_text2")), page);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Back_button")), 0x933DD695u);
    FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Select_button")), 0x8202C364u);
}

void PopHelpBarIfPushed()
{
    if (FEngIsPackagePushed(kHelpBar)) FEngPopPackage(kHelpBar);
}
}  // namespace

UIPauseOptions *UIPauseOptions::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::UIPauseOptionsVTable();
    mbColorCal = 0;
    mVibrationIndex = 0;
    mbExitToRace = 0;
    mpHandling = nullptr;
    base.mVisibleCount = 8;
    Setup();
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, Pkg(*this), 0xFF, 0);
    mbBusy = 0;
    return this;
}

void UIPauseOptions::Destruct() { base.DestructBody(); }

UIPauseOptions *UIPauseOptions::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

void UIPauseOptions::Setup()
{
    const char *pkg = Pkg(*this);
    mbColorCal = 0;
    FEngSetInvisible(FEngFindObject(pkg, 0x444969FDu));
    FEngSetInvisible(FEngFindObject(pkg, 0x444969FEu));
    switch (static_cast<uint32_t>(bridge::OptionsPage())) {
    case 0: SetupAudio(); break;
    case 1: SetupVideo(); break;
    case 2: SetupGameplay(); break;
    case 3:
        SetupPlayer();
        FEngSetVisible(FEngFindObject(pkg, 0x444969FDu));
        FEngSetVisible(FEngFindObject(pkg, 0x444969FEu));
        break;
    default: break;
    }
    base.SetInitialOption(FEngGetLastButton(pkg));
}

void UIPauseOptions::SetupAudio()
{
    SetHeaders(*this, 0x737D1B97u);
    // AOMasterVol, AOFEMusicVol, AOIGMusicVol, AOSpeechVol, AOSFXMasterVol, AOTireVol, AOEngineVol (no audio mode
    // toggle in the race); PC AddSliderOption(w, true) 54F800
    for (uint32_t vt : {0x7918F8u, 0x791A88u, 0x791AD8u, 0x791A38u, 0x791948u, 0x791998u, 0x7919E8u})
        base.AddSliderOption(NewSlider(base, vt), true);
}

void UIPauseOptions::SetupVideo()
{
    UIWidgetMenu &m = base;
    SetHeaders(*this, 0xEF1BB23Bu);
    FEToggleWidget *resolution = NewToggle(m, 0x791F30u);
    m.AddToggleOption(resolution, int32_t(1));
    auto *detail = reinterpret_cast<uint8_t *>(NewSlider(m, 0x791F78u, sizeof(FESliderWidget) + sizeof(void *)));
    if (detail) *reinterpret_cast<FEWidget **>(detail + sizeof(FESliderWidget)) = nullptr;
    m.AddSliderOption(reinterpret_cast<FESliderWidget *>(detail), int32_t(1));
    // PC writes the link after the add, without a null check
    *reinterpret_cast<FEWidget **>(detail + sizeof(FESliderWidget)) = resolution ? &resolution->base.base : nullptr;
    m.AddButtonOption(NewButton(m, 0x792060u));
    m.AddButtonOption(NewButton(m, 0x7920A0u));
}

void UIPauseOptions::SetupGameplay()
{
    UIWidgetMenu &m = base;
    mbAutoSave = *bridge::GameByte(0x844AD8);
    mbAutoSave2 = *bridge::GameByte(0x844AD9);
    SetHeaders(*this, 0xE8647E75u);
    if (bridge::IsCareerFreeRoam()) m.AddToggleOption(NewToggle(m, 0x791D20u), int32_t(1));   // GOCareerDifficulty (54EF30)
    AddToggle(m, 0x791C00u);                                                // GOJumpCams
    if (bridge::QuickRaceNumPlayers() != 2) AddToggle(m, 0x791C48u);        // GORearview
    AddToggle(m, 0x791C90u);                                                // GOSpeedoUnits
    if (bridge::QuickRaceNumPlayers() != 2) AddToggle(m, 0x791D68u);        // GOExploringMiniMap
    AddToggle(m, 0x791CD8u);                                                // GORacingMiniMap
    AddToggle(m, 0x793D68u, sizeof(FEToggleWidget) + 4);                    // GOAutoSave (PC 0x58)
}

void UIPauseOptions::SetupPlayer()
{
    UIWidgetMenu &m = base;
    SetHeaders(*this, 0xD9DC2F12u);
    if (!bridge::IsDragRace()) AddToggle(m, 0x795660u);   // PPOTransmission
    FEToggleWidget *handling = NewToggle(m, 0x7955F0u);    // PPOHandling
    mpHandling = handling;
    m.AddToggleOption(handling, true);
    AddToggle(m, 0x7956A8u);                               // PPODriveCam
    // vibration: PC toggle 791DB0 (0x58) = toggle + player +0x54, enabled when force feedback exists (fetched
    // before the pausing player, unlike UIOptionsScreen::SetupPlayer)
    auto *vib = reinterpret_cast<uint8_t *>(Alloc<uint8_t>(sizeof(FEToggleWidget) + 4));
    if (vib) {
        const bool enabled = bridge::ForceFeedbackAvailable();
        const int32_t player = PausingPlayerIndex();
        reinterpret_cast<FEToggleWidget *>(vib)->Construct(&m.mStartX, &m.mTitleW, &m.mDataW, &m.mDataX, enabled);
        *reinterpret_cast<int32_t *>(vib + sizeof(FEToggleWidget)) = player;
        reinterpret_cast<FEWidget *>(vib)->vtable = bridge::GameVTable(0x791DB0u);
    }
    mVibrationIndex = static_cast<int32_t>(m.AddToggleOption(reinterpret_cast<FEToggleWidget *>(vib), true));
    AddToggle(m, 0x7956F0u);                               // PPOGauges
    if (!bridge::IsInPerformanceTuning()) {
        AddToggle(m, 0x795738u);                           // PPOPosition
        AddToggle(m, 0x795780u);                           // PPOLapInfo
        AddToggle(m, 0x795810u);                           // PPOSplitTime
        AddToggle(m, 0x795858u);                           // PPOLeaderBoard
    }
    AddToggle(m, 0x7957C8u);                               // PPOScore
}

bool UIPauseOptions::ReadyToExit(uint32_t msg)
{
    const char *pkg = Pkg(*this);
    if (*bridge::GameByte(0x844AD8) == 0 && mbAutoSave == 1) {
        mbBusy = 1;
        bridge::DialogInterface_ShowTwoButtons(pkg, "", 0xF770u, 0x61Cu, msg, 0x22222222u, 0x22222222u, 1, 0x22BC94AFu);
        return false;
    }
    if (*bridge::GameByte(0x844AD9) == 1 && mbAutoSave2 == 0) {
        mbBusy = 1;
        bridge::DialogInterface_ShowTwoButtons(pkg, "", 0xF770u, 0x61Cu, msg, 0x22222222u, 0x22222222u, 1, 0xF3568B86u);
        return false;
    }
    return true;
}

void UIPauseOptions::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    const char *pkg = Pkg(*this);
    if (mbBusy && (msg == 0x911AB364u || msg == 0x0C407210u)) return;
    const uint32_t page = static_cast<uint32_t>(bridge::OptionsPage());
    auto back = [&] {   // 0x911AB364: confirm autosave changes on the gameplay page, then run the out-script
        if (page == 2 && !ReadyToExit(0x11111111u)) return;
        if (!FEngIsScriptRunning(pkg, kOutObject, kOutScript)) FEngSetScript(pkg, kOutObject, kOutScript, true);
        base.NotificationMessage(0x911AB364u, obj, arg, pkgArg);
    };
    if (msg == 0xC98356BAu) {
        if (mbExitToRace) {
            PopHelpBarIfPushed();
            FEngPopPackage(pkg);
            bridge::RaceCoordinator_RequestUnPause(1);
            return;
        }
    } else if (msg == 0x911AB364u) {
        back();   // the base handler only sees it from back()
        return;
    }
    base.NotificationMessage(msg, obj, arg, pkgArg);
    auto finished = [&] {   // 0xE1FDE1D1 (out-script done): act on the message that started it
        switch (base.mPrevMsg) {
        case 0x0C407210u:
            FEngSetLastButton(pkg, 0);
            FEngSwitchPackages(pkg, "UI_PC_Customize_Options.fng", 0);
            return;
        case 0x911AB364u:
            if (mbBusy) return;
            if (page == 1) bridge::ApplyVideoSettings();
            FEngSetLastButton(pkg, 0);
            FEngSwitchPackages(pkg, kPauseMain, 0);
            return;
        case 0xB5AF2461u:
            if (!mbBusy) mbExitToRace = 1;
            return;
        default: return;
        }
    };
    switch (msg) {
    case 0x11111111u:   // dialog: keep the autosave change, leave the page
        mbBusy = 0;
        base.mPrevMsg = 0x911AB364u;
        FEngSetScript(pkg, kOutObject, "FORWARD", true);
        FEngSetLastButton(pkg, 0);
        FEngSwitchPackages(pkg, kPauseMain, 0);
        return;
    case 0x22222222u:   // dialog: cancel
        base.mPrevMsg = 0;
        FEngSetScript(pkg, kOutObject, "CALL", true);
        mbBusy = 0;
        return;
    case 0x33333333u:   // dialog after "resume race": forget the pause menu buttons, back to the race next
        mbBusy = 0;
        mbExitToRace = 1;
        FEngSetLastButton(pkg, 0);
        FEngSetLastButton(kPauseMain, 0);
        FEngSetLastButton("UI_Pause.fng", 0);
        return;
    case 0x35F8620Bu: {
        FEWidget *vib = base.GetWidget(static_cast<uint32_t>(mVibrationIndex));
        if (vib && !vib->mbEnabled) {
            bridge::FEWidget_Disable(vib);
            bridge::FEWidget_Draw(vib);
        }
        finished();   // PC falls into the 0xE1FDE1D1 handler
        return;
    }
    case 0xE1FDE1D1u: finished(); return;
    case 0xD05FC3A3u:   // restore video defaults confirmed
        if (page != 1) return;
        bridge::ResetVideoDefaults();
        *bridge::GameByte(0x8709D1) = 1;
        bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0xAF0BBD92u);
        return;
    case 0xB5AF2461u:   // resume race
        base.StorePrevNotification(0xB5AF2461u, obj, arg, pkgArg);
        FEngSetScript(pkg, kOutObject, "FORWARD", true);
        if (page == 2 && !ReadyToExit(0x33333333u)) return;
        PopHelpBarIfPushed();
        bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0x33333333u);
        return;
    default: return;
    }
}
