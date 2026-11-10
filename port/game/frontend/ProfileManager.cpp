#include "ProfileManager.h"
#include "bridge/Bridge.h"
#include "FELocalize.h"
#include "FEngUtil.h"
#include "cFEng.h"

namespace {
IconOption *NewPM(PMIcon k, uint32_t name, uint32_t header)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, 0);
        o->vtable = bridge::GameVTable(0x7959D4u + 8u * static_cast<uint32_t>(k));
    }
    return o;
}
void PrintPlayerName(const char *pkg) { bridge::FEngPrintfByName(pkg, "Player_Name", bridge::ProfileDBName()); }
}  // namespace

void PMIconReact(PMIcon k, const char *pkg, uint32_t msg)
{
    if (msg != 0x0C407210u) return;
    switch (k) {
    case PMIcon::Save: bridge::EnterMemcard(pkg, pkg, 0x4A51, 0, 0, 0, 0); return;
    case PMIcon::Load: bridge::EnterMemcard(pkg, pkg, 0xC11, 0, 0, 0x3A2BE557u, 0x8867412Du); return;
    case PMIcon::Delete: bridge::EnterMemcard(pkg, pkg, 0x831, 0, 0, 0, 0); return;
    case PMIcon::CreateNew: bridge::EnterMemcard(pkg, pkg, 0x861, 0, 0, 0, 0); return;
    }
}

// ---- UIProfileManager ----

UIProfileManager *UIProfileManager::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::UIProfileManagerVTable();
    Setup();
    bridge::FEngPrintfString(base.base.mpPackageName, 0x42ADB44Cu, GetLocalizedString(0xBCB18F38u));
    return this;
}

void UIProfileManager::Destruct()
{
    base.base.vtable = bridge::UIProfileManagerVTable();
    base.Destruct();
}

UIProfileManager *UIProfileManager::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIProfileManager::Refresh()
{
    mpSave->mbLocked = bridge::AutoSaveSecondStateAllowed() ? 0 : 1;   // PC 502FE0 IsProfileNamed
    PrintPlayerName(base.base.mpPackageName);
    FEngSetColor(mpSave->mpObject, mpSave->mColor);
    bridge::IconScrollerMenu_RefreshHeader(&base);
    bridge::HelpBar_SetFlag6C(1);
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, base.base.mpPackageName);
}

void UIProfileManager::Setup()
{
    mpSave = NewPM(PMIcon::Save, 0x228B7E32u, 0x001C8ACEu);
    bridge::IconScrollerMenu_AddOption(&base, NewPM(PMIcon::CreateNew, 0x56026748u, 0x55423473u));
    bridge::IconScrollerMenu_AddOption(&base, NewPM(PMIcon::Load, 0x2287E063u, 0x0018ECFFu));
    bridge::IconScrollerMenu_AddOption(&base, mpSave);
    bridge::IconScrollerMenu_AddOption(&base, NewPM(PMIcon::Delete, 0xB9929709u, 0x56B00632u));
    Refresh();
    bridge::IconScroller_SetInitialPos(&base.mScroller, FEngGetLastButton(base.base.mpPackageName));
    if (uint8_t *music = bridge::MenuMusicObject()) {
        bridge::MenuMusic_Command(0);
        music[0xB] = 1;
    }
}

void UIProfileManager::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    switch (msg) {
    case 0x911AB364u:
        if (uint8_t *music = bridge::MenuMusicObject()) {
            music[0xB] = 0;
            if (*bridge::GameByte(0x844AD9)) bridge::MenuMusic_Command(1);
        }
        FEngSwitchPackages(base.base.mpPackageName, bridge::CareerReturnScreen(), 0);
        return;
    case 0x251BF83Eu:
        Refresh();
        bridge::GarageShowSelectedCar();   // PC falls into the 0x3A2BE557 handler
        return;
    case 0x3A2BE557u: bridge::GarageShowSelectedCar(); return;   // profile loaded
    default: return;
    }
}

// ---- UIDeleteProfile ----

UIDeleteProfile *UIDeleteProfile::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::UIDeleteProfileVTable();
    Setup();
    bridge::FEngPrintfString(base.base.mpPackageName, 0x42ADB44Cu, GetLocalizedString(0x4AF2E79Eu));
    return this;
}

void UIDeleteProfile::Destruct()
{
    base.base.vtable = bridge::UIDeleteProfileVTable();
    base.Destruct();
}

UIDeleteProfile *UIDeleteProfile::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIDeleteProfile::Setup()
{
    bridge::IconScrollerMenu_AddOption(&base, NewPM(PMIcon::CreateNew, 0x56026748u, 0x55423473u));
    bridge::IconScrollerMenu_AddOption(&base, NewPM(PMIcon::Delete, 0xB9929709u, 0x56B00632u));
    bridge::IconScrollerMenu_RefreshHeader(&base);
    bridge::IconScroller_SetInitialPos(&base.mScroller, FEngGetLastButton(base.base.mpPackageName));
}

void UIDeleteProfile::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    if (msg == 0x911AB364u) FEngSwitchPackages(base.base.mpPackageName, "UI_ProfileManager.fng", 0);
    else if (msg == 0x251BF83Eu) PrintPlayerName(base.base.mpPackageName);
}
