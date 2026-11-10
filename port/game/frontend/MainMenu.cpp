#include "MainMenu.h"
#include "bridge/Bridge.h"
#include "FEPackageManager.h"
#include "FEngUtil.h"
#include "cFEng.h"

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";
const char *Pkg(const UIMain &m) { return m.base.base.mpPackageName; }

void AddMM(UIMain &m, MMKind k, uint32_t name, uint32_t header, bool direct)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, 0);
        o->vtable = bridge::GameVTable(MMVTable(k));
        if (direct) o->mbActsDirectly = 1;
    }
    bridge::IconScrollerMenu_AddOption(&m.base, o);
}
}  // namespace

uint32_t MMVTable(MMKind k) { return 0x790C30u + 8u * static_cast<uint32_t>(k); }

int32_t FEngMapJoyParamToJoyport(uint32_t p)
{
    if (p & 1) return 0;
    if (p & 2) return 1;
    if (p & 4) return 2;
    return (p & 8) ? 3 : -1;
}

void FEngSetJoyPortActive(int32_t port, bool on)
{
    uint32_t *mask = reinterpret_cast<uint32_t *>(bridge::GameByte(0x838524));
    const uint32_t bit = 1u << (port & 31);
    if (on) *mask |= bit;
    else *mask &= bit ^ 0xFFu;   // PC: xor 0xFF (clears the high bytes too)
}

void MMReact(MMKind k, const char *pkg, uint32_t msg, uint32_t a3)
{
    if (msg != 0x0C407210u) return;
    switch (k) {
    case MMKind::QuickRace:
        bridge::SetGameModeFlags(2);
        bridge::QuickRace_Begin(pkg);
        return;
    case MMKind::Online:
        bridge::SetMenuJoyPort(FEngMapJoyParamToJoyport(a3));
        bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0xD05FC3A3u);
        return;
    case MMKind::Options:
        bridge::SetGameModeFlags(0x10);
        bridge::SetOptionsReturnScreen(pkg);
        bridge::OptionsDB_AllocBackup();
        FEngSwitchPackages(pkg, "UI_OptionsMain.fng", 0);
        *bridge::GameByte(0x836360) = *bridge::GameByte(0x844AD8);
        return;
    case MMKind::CarSelect:
        bridge::SetGameModeFlags(0x20);
        *bridge::GameByte(0x838540) = 0;   // ClearHasBeenModified 50E2D0
        FEngSwitchPackages(pkg, "UI_QRCarSelect.fng", 0);   // PC: 404800 (always false) would skip this
        return;
    case MMKind::Career: FEngSwitchPackages(bridge::CareerReturnScreen(), "UI_StartCareer.fng", 0); return;
    case MMKind::SaveLoad: FEngSwitchPackages(pkg, "UI_ProfileManager.fng", 0); return;
    case MMKind::SplitScreen:
        FEngSetJoyPortActive(3, false);
        FEngSetJoyPortActive(0, false);
        FEngSetJoyPortActive(1, false);
        FEngSetJoyPortActive(2, false);
        bridge::SetGameModeFlags(0x200);
        FEngSwitchPackages(pkg, "UI_PC_LAN_ServerSelect.fng", 0);
        return;
    }
}

UIMain *UIMain::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::UIMainVTable();
    bridge::SetGameModeFlags(2);
    void **pending = bridge::PendingOnlineScreen();
    if (*pending) bridge::VirtualScalarDelete(*pending);
    *pending = nullptr;
    Setup();
    base.RefreshHeader();
    bridge::GarageShowPlayerCar();
    if (!FEngIsPackagePushed("GarageMain.fng")) FEngPushPackageSpecial(0, "GarageMain.fng", nullptr, 0, 0);
    bridge::QuickRaceSettings_SetNumPlayers(1);
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, Pkg(*this), 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, Pkg(*this));
    return this;
}

void UIMain::Destruct()
{
    base.base.vtable = bridge::UIMainVTable();
    base.Destruct();
}

UIMain *UIMain::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIMain::Setup()
{
    AddMM(*this, MMKind::Career, 0x03704F3Du, 0x5E7E1192u, false);
    AddMM(*this, MMKind::QuickRace, 0x4E6FBB02u, 0x24909FD7u, false);
    AddMM(*this, MMKind::Online, 0x771849A5u, 0x7B63FF85u, false);   // PC 570A60 (online available) is always true
    AddMM(*this, MMKind::SplitScreen, 0xD13A2E66u, 0x7294434Cu, true);
    AddMM(*this, MMKind::CarSelect, 0xB0C46023u, 0xA53039A3u, false);
    AddMM(*this, MMKind::SaveLoad, 0x6B303856u, 0xBCB18F38u, true);
    AddMM(*this, MMKind::Options, 0x3058FE37u, 0xED1F0B2Cu, false);
    if (FEngGetLastButton(Pkg(*this)) == 0) FEngSetLastButton(Pkg(*this), 2);
    bridge::IconScroller_SetInitialPos(&base.mScroller, FEngGetLastButton(Pkg(*this)));
}

void UIMain::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = Pkg(*this);
    if (msg == 0x34DC1BCFu) {
        bridge::TheFEPackageManager()->SendMessageToPackage(pkg, 0x251BF83Eu);
        FEngSetScript(pkg, 0x47FF4E7Cu, "CALL", true);
    } else if (msg == 0xD05FC3A3u) {   // online chosen
        bridge::SetGameModeFlags(8);
        bridge::CreateOnlineLoginScreen(pkg);
    }
}
