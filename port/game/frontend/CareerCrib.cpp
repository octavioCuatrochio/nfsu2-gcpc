#include "CareerCrib.h"
#include "bridge/Bridge.h"
#include "FEPackageManager.h"
#include "FEngUtil.h"
#include "MainMenu.h"   // FEngMapJoyParamToJoyport
#include "cFEng.h"

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";
constexpr uint32_t kOutObject = 0x47FF4E7Cu, kOutScript = 0xDE6EFF34u;
void SendToPkg(const char *pkg, uint32_t msg) { bridge::TheFEPackageManager()->SendMessageToPackage(pkg, msg); }
// "are you sure" dialogs of the crib (GenericDialog_SMALL.fng, no = 0x43DA9FD0) + dim
void Ask(const char *pkg, uint32_t yes, uint32_t text)
{
    bridge::DialogInterface_ShowTwoButtons(pkg, "GenericDialog_SMALL.fng", 0x88579775u, 0xBE503981u, yes, 0x43DA9FD0u, 0x43DA9FD0u, 1, text);
    SendToPkg(pkg, 0x96DDA9E6u);
}
void AddCC(uiCareerCrib &c, CCKind k, uint32_t name, uint32_t header, bool direct)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, 0);
        o->vtable = bridge::GameVTable(CCVTable(k));
        if (direct) o->mbActsDirectly = 1;
    }
    bridge::IconScrollerMenu_AddOption(&c.base, o);
}
}  // namespace

uint32_t CCVTable(CCKind k)
{
    static constexpr uint32_t kVt[] = {0x790CF4u, 0x790CFCu, 0x790D04u, 0x790D24u, 0x790D44u, 0x790D64u, 0x790D6Cu, 0x790D8Cu};
    return kVt[static_cast<int>(k)];
}

void CCReact(CCKind k, const char *pkg, uint32_t msg)
{
    if (msg != 0x0C407210u) return;
    switch (k) {
    case CCKind::EnterGameplay: Ask(pkg, 0x451E768Eu, 0x60B42AFBu); return;
    case CCKind::ChangeParts: bridge::Crib_ChangeParts(pkg); return;
    case CCKind::CarSel: FEngSwitchPackages(pkg, "UI_CareerCarSelect.fng", 0); return;
    case CCKind::ViewSponsors: FEngSwitchPackages(pkg, "UI_CribRewardOptionsMain.fng", 0); return;
    case CCKind::ViewWorldMap: FEngSwitchPackages(pkg, "UI_CareerWorldMap.fng", 0); return;
    case CCKind::SaveMemcard:
        bridge::OptionsDB_DeallocBackup();
        bridge::EnterMemcard(pkg, pkg, 0xC41, 0, 0, 0x28C2C1B8u, 0x7D1E4C8Cu);
        return;
    case CCKind::TestTrack:
        SendToPkg(pkg, 0x96DDA9E6u);
        bridge::DialogInterface_ShowTwoButtons(pkg, "GenericDialog_Small.fng", 0xF770u, 0x61Cu, 0x34DC1BECu, 0x1FAB5998u, 0x1FAB5998u, 1, 0xB10AE56Fu);
        return;
    case CCKind::BackToMain: Ask(pkg, 0x30ED2368u, 0xCB5E3178u); return;
    }
}

uiCareerCrib *uiCareerCrib::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::CareerCribVTable();
    Setup();
    bridge::GarageShowCurrentRide();
    if (!FEngIsPackagePushed("GarageMain.fng")) FEngPushPackageSpecial(0, "GarageMain.fng", nullptr, 0, 0);
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, base.base.mpPackageName, 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, base.base.mpPackageName);
    return this;
}

void uiCareerCrib::Destruct()
{
    base.base.vtable = bridge::CareerCribVTable();
    base.Destruct();
}

uiCareerCrib *uiCareerCrib::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void uiCareerCrib::CheckMemory()
{
    const char *pkg = base.base.mpPackageName;
    if (bridge::OptionsDB_IsDirty()) bridge::EnterMemcard(pkg, pkg, 0x151, 0, 0, 0, 0);
    else SendToPkg(pkg, 0x9A3B571Du);
}

void uiCareerCrib::DoStartTuning()
{
    FEngSetLastButton(base.base.mpPackageName, 0);
    bridge::Crib_StartTuning();
}

void uiCareerCrib::Setup()
{
    AddCC(*this, CCKind::EnterGameplay, 0xA230CD88u, 0x194EFA7Au, true);
    if (bridge::CareerStage() > 1) AddCC(*this, CCKind::ViewWorldMap, 0x22F2CEABu, 0x9C07CA52u, false);
    AddCC(*this, CCKind::CarSel, 0xA964095Bu, 0x451879E2u, false);
    AddCC(*this, CCKind::ChangeParts, 0x511F9D75u, 0xABB24BBCu, false);
    AddCC(*this, CCKind::TestTrack, 0x7A9B397Au, 0x8513F2C0u, true);
    AddCC(*this, CCKind::ViewSponsors, 0x49D2E3DEu, 0xCE62C0BFu, false);
    AddCC(*this, CCKind::SaveMemcard, 0x8ADA6FD4u, 0x03EC7395u, false);
    AddCC(*this, CCKind::BackToMain, 0xFC39853Fu, 0x046E7D84u, true);
    bridge::IconScroller_SetInitialPos(&base.mScroller, FEngGetLastButton(base.base.mpPackageName));
    bridge::IconScrollerMenu_RefreshHeader(&base);
    if (FEngIsPackagePushed("GarageMain.fng") && !bridge::PlatformFileLoaded("FRONTEND\\PLATFORMS\\CRIB01.BIN"))
        bridge::Garage_ChangePlatform(1, 4);
}

void uiCareerCrib::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = base.base.mpPackageName;
    auto toFreeRoam = [&] {
        *bridge::GameByte(0x836324) = 0;
        *bridge::GameByte(0x863484) = 1;
        bridge::StartCareerFreeRoam();
    };
    switch (msg) {
    case 0x0C407210u:
        if (base.mScroller.base.mpCurrent->mbShowLock || FEngIsScriptRunning(pkg, kOutObject, kOutScript)) return;
        FEngSetScript(pkg, kOutObject, kOutScript, true);
        return;
    case 0x30ED2368u:   // back to main menu: yes
        bridge::Garage_ChangePlatform(4, 1);
        bridge::OptionsDB_DeallocBackup();
        *bridge::GameByte(0x836324) = 0;
        FEngSwitchPackages(pkg, "UI_Main.fng", 0);
        FEngSetLastButton(base.base.mpPackageName, 0);
        return;
    case 0x1FAB5998u:
    case 0x43DA9FD0u:   // dialog "no": undim
        bridge::OptionsDB_AllocBackup();
        base.StopDim();
        FEngSetScript(pkg, 0x444969FDu, "UnDim", true);
        FEngSetScript(base.base.mpPackageName, 0x444969FEu, "UnDim", true);
        return;
    case 0x35F8620Bu:
        bridge::FESpeech_HandleEvent(0x11);
        bridge::OptionsDB_AllocBackup();
        return;
    case 0x28C2C1B8u:
    case 0x7D1E4C8Cu: bridge::OptionsDB_AllocBackup(); return;
    case 0x34DC1BECu:   // test track: yes
        bridge::OptionsDB_DeallocBackup();
        DoStartTuning();
        return;
    case 0x451E768Eu:   // enter gameplay: yes
        bridge::SetMenuJoyPort(FEngMapJoyParamToJoyport(arg));
        CheckMemory();
        bridge::OptionsDB_DeallocBackup();
        return;
    case 0x461A18EEu:
    case 0x8867412Du:
        bridge::SetMenuJoyPort(FEngMapJoyParamToJoyport(static_cast<uint32_t>(bridge::CareerSlot())));
        toFreeRoam();
        return;
    case 0x9A3B571Du: toFreeRoam(); return;
    case 0x911AB364u: {   // back: ask, text depends on the "came from main menu" flag
        const bool flag = *bridge::GameByte(0x836324) != 0;
        bridge::DialogInterface_ShowTwoButtons(pkg, "GenericDialog_SMALL.fng", 0x88579775u, 0xBE503981u,
                                               flag ? 0x30ED2368u : 0x451E768Eu, 0x43DA9FD0u, 0x43DA9FD0u, 1, flag ? 0xCB5E3178u : 0x60B42AFBu);
        FEngSetControlMask(base.base.mpPackageName, 0);
        SendToPkg(base.base.mpPackageName, 0x96DDA9E6u);
        return;
    }
    default: return;
    }
}
