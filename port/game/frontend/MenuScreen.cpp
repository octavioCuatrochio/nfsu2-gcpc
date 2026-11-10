#include "MenuScreen.h"
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "FEHash.h"
#include "cFEng.h"

bool IsOverlayPackage(const char *pkg)
{
    return bStrCmp(pkg, "ScreenPrintf.fng") == 0 || bStrCmp(pkg, "Chyron_FE.fng") == 0 || bStrCmp(pkg, "GarageMain.fng") == 0 ||
           bStrCmp(pkg, "UI_PC_Help_Bar.fng") == 0;
}

bool IsGenericDialogPackage(const char *pkg)
{
    static const char *const kDialogs[] = {
        "GenericDialog_ThreeButton.fng", "GenericDialog_Animate_SMALL.fng", "GenericDialog_SMALL.fng",
        "GenericDialog_Animate_MED.fng", "GenericDialog_MED.fng", "GenericDialog_Animate_LARGE.fng", "GenericDialog_LARGE.fng"};
    const uint32_t h = FEHashUpper(pkg);
    for (const char *d : kDialogs)
        if (h == FEHashUpper(d)) return true;
    return false;
}

MenuScreen *MenuScreen::Construct(const ScreenConstructorData *data)
{
    vtable = bridge::MenuScreenVTable();
    _unknown34 = _unknown38 = _unknown3C = nullptr;
    _unknown40 = _unknown44 = 0.0f;
    _unknown48 = 0;
    mData = *data;   // rep movsd x5
    mpPackageName = data->mpPackageName;
    _unknown08 = 0;
    mHelpHash = data->mHelpHash;
    _unknown0C = 0;
    _unknown10 = _unknown11 = 0;
    _unknown2C = 1;
    _unknown30 = 0;
    if (bridge::ProfileState() >= 0xC) {
        const int32_t mode = bridge::GameModeFlags();
        bridge::FEngPrintfString(mpPackageName, 0xEB406FECu, "%s", bridge::ProfileName(mode == 0x200));
        const int32_t m = bridge::GameModeFlags();   // reloaded
        bool eam = false;
        if (m != 0x200 && m != 0x20 && m != 0x100)
            eam = bStrCmp(mpPackageName, "UI_OLEAMessenger.fng") != 0 && bStrCmp(mpPackageName, "GarageMain.fng") != 0 &&
                  bStrCmp(mpPackageName, "Keyboard.fng") != 0 && bStrCmp(mpPackageName, "UI_VirtualKeyboard.fng") != 0;
        bridge::SetShowEAMIcons(eam);
        if (!bridge::CallingScreenFrozen()) {
            // gCallingScreenPackageName: the last real screen (not overlays or dialogs); debug prints dropped
            if (!IsOverlayPackage(mpPackageName) && !IsGenericDialogPackage(mpPackageName))
                bStrCpy(bridge::CallingScreenPackageName(), mpPackageName);
        }
    }
    if (mData.mHelpBarMask && FEngIsPackagePushed("UI_PC_Help_Bar.fng"))
        bridge::HelpBar_SyncByMask(mData.mHelpBarMask, mpPackageName);
    return this;
}

MenuScreen *MenuScreen::ScalarDeletingDestructor(uint32_t flags)
{
    vtable = bridge::MenuScreenVTable();
    if (flags & 1) bridge::FreeArray(this);   // operator delete (575630)
    return this;
}
