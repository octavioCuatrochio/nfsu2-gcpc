#include "UIOptionsScreen.h"
#include "bridge/Bridge.h"
#include "FEWidget.h"
#include "FEngUtil.h"
#include "FEHash.h"
#include "FEObject.h"
#include "FEPackageManager.h"
#include "MenuScreen.h"
#include "cFEng.h"
#include <initializer_list>

namespace {
constexpr uint32_t kTitle = 0x42ADB44Cu;   // title string object
const char *Pkg(const UIOptionsScreen &s) { return s.base.base.mpPackageName; }

// new widget of the option class: operator new, stock ctor at the menu's row geometry, the class vtable
// (PC: a failed allocation passes null to Add*Option).
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
FEToggleWidget *NewToggle(UIWidgetMenu &m, uint32_t vtable, uint32_t size = sizeof(FEToggleWidget), bool enabled = true)
{
    auto *w = Alloc<FEToggleWidget>(size);
    if (w) {
        w->Construct(&m.mStartX, &m.mTitleW, &m.mDataW, &m.mDataX, enabled);
        w->base.base.vtable = bridge::GameVTable(vtable);
    }
    return w;
}
FEButtonWidget *NewButton(UIWidgetMenu &m, uint32_t vtable, uint32_t size = sizeof(FEButtonWidget), bool enabled = true)
{
    auto *w = Alloc<FEButtonWidget>(size);
    if (w) {
        w->Construct(&m.mStartX, &m.mTitleW, enabled);
        w->base.vtable = bridge::GameVTable(vtable);
    }
    return w;
}
void AddSlider(UIWidgetMenu &m, uint32_t vt) { m.AddSliderOption(NewSlider(m, vt), int32_t(1)); }
void AddToggle(UIWidgetMenu &m, uint32_t vt, uint32_t size = sizeof(FEToggleWidget)) { m.AddToggleOption(NewToggle(m, vt, size), int32_t(1)); }
void AddButton(UIWidgetMenu &m, uint32_t vt) { m.AddButtonOption(NewButton(m, vt)); }
void SetTitle(const UIOptionsScreen &s, uint32_t lang) { FEngSetLanguageHash(FEngFindString(Pkg(s), kTitle), lang); }
}  // namespace

UIOptionsScreen *UIOptionsScreen::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    MenuScreen &ms = base.base;
    ms.vtable = bridge::UIOptionsScreenVTable();
    mbColorCal = 0;
    mbBusy = 0;
    mbPlayerPage = 0;
    mWheelButtonIndex = 0;
    mForceFeedbackIndex = 0;
    ms.mbSounds = 1;
    base.mVisibleCount = 8;
    const char *pkg = ms.mpPackageName;
    const int32_t mode = bridge::GameModeFlags();
    if (mode == 8) {
        ms.mHelpHash = 0xDA850671u;
        FEngSetInvisible(FEngFindObject(pkg, 0xEB406FECu));
        FEngSetScript(pkg, 0x2BAC0CEEu, "Show", true);
        FEngSetLanguageHash(FEngFindString(pkg, 0xC90DC4ABu), 0xB0FCF98Eu);
    } else {
        ms.mHelpHash = 0xED586157u;
    }
    FEObject *player = FEngFindString(pkg, FEHashUpper("PLAYER"));
    switch (static_cast<uint32_t>(bridge::OptionsPage())) {
    case 0: SetTitle(*this, 0xE76CD783u); break;
    case 1: SetTitle(*this, 0x4204D527u); break;
    case 2: SetTitle(*this, 0xD0CF6EE1u); FEngSetInvisible(player); break;
    case 3:
        SetTitle(*this, 0xF760EABEu);
        FEngSetLanguageHash(player, 0xD87133BCu + (bridge::PlayerToEditForOptions() != 0 ? 1u : 0u));
        ms.mHelpHash = 0xF35B6D7Fu;
        if (bridge::GameModeFlags() == 8 || bridge::GameModeFlags() == 0x200) bridge::FEngPrintfByName(pkg, "PLAYER", " ");
        if (bridge::GameModeFlags() == 8) FEngSetLanguageHash(pkg, 0xC90DC4ABu, 0xB0FCF98Eu);
        break;
    case 4:
        if (bridge::GameModeFlags() == 8 || bridge::GameModeFlags() == 0x200) {
            ms.mHelpHash = 0xD5F3F0E0u;
            bridge::FEngPrintfByName(pkg, "PLAYER", " ");
        }
        break;
    case 5:
        SetTitle(*this, 0xDCFB6B36u);
        ms.mHelpHash = bridge::GameModeFlags() == 8 ? 0xCE497C51u : 0xD8F5EDF7u;
        break;
    case 6: SetTitle(*this, 0xD10D7477u); ms.mHelpHash = 0xB5B3AB78u; break;
    case 8: SetTitle(*this, 0x0905101Fu); ms.mHelpHash = 0x844FEB00u; break;
    default: break;
    }
    Setup();
    if (bridge::GameModeFlags() == 8) {
        FEngSetLanguageHash(FEngFindString(pkg, 0xC90DC4ABu), 0xB0FCF98Eu);
        FEngSetVisible(FEngFindObject(pkg, 0xC90DC4ABu));
        FEngSetVisible(FEngFindObject(pkg, FEHashUpper("JOY_EVENT_BUTTON1")));
        if (bridge::GameModeFlags() == 8) bridge::HelpBar_SyncByMask(ms.mData.mHelpBarMask | 0x4000u, pkg);
    }
    return this;
}

void UIOptionsScreen::Destruct() { base.DestructBody(); }

UIOptionsScreen *UIOptionsScreen::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

uint32_t UIOptionsScreen::NotifySoundMessage() const
{
    if (mbPlayerPage && bridge::GameModeFlags() != 8) bridge::PlayUISound(0xB);   // PC 50D180 (-1: nothing)
    return 0;
}

void UIOptionsScreen::Setup()
{
    const char *pkg = Pkg(*this);
    for (uint32_t h : {0x010CBA23u, 0x97B71DDEu, 0x7326E44Cu, 0xC90DC4ABu}) FEngSetInvisible(pkg, h);
    mbColorCal = 0;
    FEngSetInvisible(pkg, "ColorCal_Logo");
    FEngSetInvisible(pkg, "ColorCal_Blurb");
    FEngSetInvisible(pkg, "Colour_Calibration_Backing");
    FEngSetInvisible(pkg, 0x444969FDu);
    FEngSetInvisible(pkg, 0x444969FEu);
    switch (static_cast<uint32_t>(bridge::OptionsPage())) {
    case 0: SetupAudio(); break;
    case 1: SetupVideo(); break;
    case 2: SetupGameplay(); break;
    case 3:
        SetupPlayer();
        FEngSetVisible(pkg, 0x444969FDu);
        FEngSetVisible(pkg, 0x444969FEu);
        break;
    case 6: SetupTrailers(); break;
    case 7: SetupOnline(); break;
    default: break;
    }
    base.SetInitialOption(0);
}

void UIOptionsScreen::SetupAudio()
{
    UIWidgetMenu &m = base;
    // AOMasterVol, AOFEMusicVol, AOIGMusicVol, AOSpeechVol, AOSFXMasterVol, AOTireVol, AOEngineVol, AOAudioMode
    for (uint32_t vt : {0x7918F8u, 0x791A88u, 0x791AD8u, 0x791A38u, 0x791948u, 0x791998u, 0x7919E8u}) AddSlider(m, vt);
    AddToggle(m, 0x791B28u);
    m.base.mHelpHash = 0xF6D18624u;
}

void UIOptionsScreen::SetupVideo()
{
    UIWidgetMenu &m = base;
    FEToggleWidget *resolution = NewToggle(m, 0x791F30u);
    m.AddToggleOption(resolution, int32_t(1));
    auto *detail = reinterpret_cast<uint8_t *>(NewSlider(m, 0x791F78u, sizeof(FESliderWidget) + sizeof(void *)));
    if (detail) *reinterpret_cast<FEWidget **>(detail + sizeof(FESliderWidget)) = nullptr;
    m.AddSliderOption(reinterpret_cast<FESliderWidget *>(detail), int32_t(1));
    // PC writes the link after the add, without a null check
    *reinterpret_cast<FEWidget **>(detail + sizeof(FESliderWidget)) = resolution ? &resolution->base.base : nullptr;
    AddButton(m, 0x792060u);
    AddButton(m, 0x7920A0u);
}

void UIOptionsScreen::SetupGameplay()
{
    UIWidgetMenu &m = base;
    // GOJumpCams, GORearview, GOSpeedoUnits, GOCareerDifficulty, GOExploringMiniMap, GORacingMiniMap, GOAutoSave (0x58)
    for (uint32_t vt : {0x791C00u, 0x791C48u, 0x791C90u, 0x791D20u, 0x791D68u, 0x791CD8u}) AddToggle(m, vt);
    AddToggle(m, 0x793D68u, sizeof(FEToggleWidget) + 4);   // PC 0x58
    m.base.mHelpHash = 0xB575A5E2u;
}

void UIOptionsScreen::SetupPlayer()
{
    UIWidgetMenu &m = base;
    // POTransmission, POHandling, PODriveCam, POGauges, POPosition, POLapInfo, POSplitTime, POScore, POLeaderBoard
    for (uint32_t vt : {0x7940F0u, 0x794138u, 0x794180u, 0x7941C8u, 0x794210u, 0x794258u, 0x7942E8u, 0x7942A0u, 0x794330u})
        AddToggle(m, vt);
    auto *wheel = reinterpret_cast<uint8_t *>(Alloc<uint8_t>(sizeof(FEButtonWidget) + 4));   // COWheelOptions (PC 0x3C): button + player +0x38
    if (wheel) {
        const int32_t player = bridge::PlayerToEditForOptions();
        reinterpret_cast<FEButtonWidget *>(wheel)->Construct(&m.mStartX, &m.mTitleW, false);
        reinterpret_cast<FEWidget *>(wheel)->vtable = bridge::GameVTable(0x791E00u);
        *reinterpret_cast<int32_t *>(wheel + sizeof(FEButtonWidget)) = player;
    }
    mWheelButtonIndex = static_cast<int32_t>(m.AddButtonOption(reinterpret_cast<FEButtonWidget *>(wheel)));
    auto *ff = reinterpret_cast<uint8_t *>(Alloc<uint8_t>(sizeof(FEToggleWidget) + 4));   // PC toggle 791DB0 (0x58): toggle + player +0x54
    if (ff) {
        const int32_t player = bridge::PlayerToEditForOptions();
        const bool enabled = bridge::ForceFeedbackAvailable();
        reinterpret_cast<FEToggleWidget *>(ff)->Construct(&m.mStartX, &m.mTitleW, &m.mDataW, &m.mDataX, enabled);
        reinterpret_cast<FEWidget *>(ff)->vtable = bridge::GameVTable(0x791DB0u);
        *reinterpret_cast<int32_t *>(ff + sizeof(FEToggleWidget)) = player;
    }
    mForceFeedbackIndex = static_cast<int32_t>(m.AddToggleOption(reinterpret_cast<FEToggleWidget *>(ff), true));
    AddToggle(m, 0x794378u);   // POTrannyPrompt
    const char *pkg = Pkg(*this);
    FEngSetVisible(FEngFindObject(pkg, 0xC90DC4ABu));
    if (bridge::GameModeFlags() == 8) FEngSetLanguageHash(FEngFindString(pkg, 0xC90DC4ABu), 0xB0FCF98Eu);
    mbPlayerPage = 1;
    m.base.mHelpHash = 0xF35B6D7Fu;
}

void UIOptionsScreen::SetupOnline()
{
    UIWidgetMenu &m = base;
    AddToggle(m, 0x791B70u);   // OOShowPlayers
    AddToggle(m, 0x791BB8u);   // OOBadConnectionWarning
    SetTitle(*this, bridge::GameModeFlags() == 0x200 ? 0x19889765u : 0x22B1C2E6u);
    m.base.mHelpHash = 0xE683C971u;
}

void UIOptionsScreen::SetupTrailers()
{
    UIWidgetMenu &m = base;
    for (uint32_t vt : {0x794480u, 0x7944C8u, 0x794518u}) AddButton(m, vt);   // TOSSX, TONFLStreet, PC NASCAR
}

void UIOptionsScreen::CheckForWheels()
{
    if (bridge::OptionsPage() != 3) return;
    FEWidget *w = base.GetWidget(static_cast<uint32_t>(mWheelButtonIndex));
    const char *pkg = Pkg(*this);
    if (bridge::WheelPluggedIn(bridge::PlayerToEditForOptions())) {
        if (w->mbEnabled) return;
        bridge::FEWidget_Enable(w);                                  // +0x1C
        if (w == base.mpCurrent) bridge::FEWidget_SetFocus(w, pkg);  // +0x24
        bridge::FEWidget_Draw(w);
    } else {
        if (!w->mbEnabled) return;
        if (base.mpCurrent == w) bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0x911C0A4Bu);
        bridge::FEWidget_Disable(w);                                 // +0x20
        bridge::FEWidget_Draw(w);
    }
}

void UIOptionsScreen::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    if (!mbBusy) base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = Pkg(*this);
    const uint32_t page = static_cast<uint32_t>(bridge::OptionsPage());
    auto leave = [&] {   // start the out-script once
        mbBusy = 1;
        if (!FEngIsScriptRunning(pkg, 0x47FF4E7Cu, 0xDE6EFF34u)) {
            base.mPrevMsg = 0x911AB364u;
            FEngSetScript(pkg, 0x47FF4E7Cu, 0xDE6EFF34u, true);
        }
    };
    switch (msg) {
    case 0x11111111u: FEngSwitchPackages(pkg, "UI_OptionsMain.fng", 0); return;
    case 0x0C407210u: if (page == 6) leave(); return;
    case 0x911AB364u: leave(); return;
    case 0x22222222u:
        mbBusy = 0;
        FEngSetScript(pkg, 0x47FF4E7Cu, "CALL", true);
        return;
    case 0x35F8620Bu: {
        FEWidget *wheel = base.GetWidget(static_cast<uint32_t>(mWheelButtonIndex));
        FEWidget *ff = base.GetWidget(static_cast<uint32_t>(mForceFeedbackIndex));
        if (wheel && !wheel->mbEnabled) bridge::FEWidget_Disable(wheel);
        if (ff && !ff->mbEnabled) bridge::FEWidget_Disable(ff);
        return;
    }
    case 0x5D1A570Fu:
        bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0x92B703B5u);
        bridge::FEWidget_SetFocus(base.mpCurrent, pkg);
        return;
    case 0xC98356BAu: CheckForWheels(); return;
    case 0xD05FC3A3u:   // restore video defaults confirmed
        if (page != 1) return;
        bridge::ResetVideoDefaults();
        *bridge::GameByte(0x8709D1) = 1;
        bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0xAF0BBD92u);
        return;
    case 0xE1FDE1D1u:   // out-script finished
        if (base.mPrevMsg == 0x911AB364u) {
            if (page == 1) bridge::ApplyVideoSettings();
            if (page == 2) {
                if (!*bridge::GameByte(0x844AD8) && mbAutoSaveAsked == 1) {
                    bridge::DialogInterface_ShowTwoButtons(pkg, "", 0xF770u, 0x61Cu, 0x11111111u, 0x22222222u, 0x22222222u, 1, 0x22BC94AFu);
                    return;
                }
                if (*bridge::GameByte(0x844AD9) == 1 && !mbAutoSave2Asked) {
                    bridge::DialogInterface_ShowTwoButtons(pkg, "", 0xF770u, 0x61Cu, 0x11111111u, 0x22222222u, 0x22222222u, 1, 0xF3568B86u);
                    return;
                }
            }
            FEngSwitchPackages(pkg, "UI_OptionsMain.fng", 0);
        } else if (base.mPrevMsg == 0x0C407210u) {
            if (page == 6) FEngSwitchPackages(pkg, "UI_Trailers.fng", 0);
            else if (page == 1) FEngSwitchPackages(pkg, "UI_PC_Customize_Options.fng", 0);
            else if (page == 3) FEngSwitchPackages(pkg, "UI_Wheel_Options.fng", 0);
        }
        return;
    default: return;
    }
}
