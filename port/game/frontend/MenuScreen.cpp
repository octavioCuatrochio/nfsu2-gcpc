#include "MenuScreen.h"
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "FEHash.h"
#include "cFEng.h"
#include "FEngUtil.h"
#include "FEMath.h"
#include "FEngine.h"
#include "FEPackage.h"
#include "FEPackageManager.h"
#include <cmath>
#include <cstdio>
#include <cstring>

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
    mpAltText1 = mpAltText2 = mpAltCurrent = nullptr;
    mAltTime1 = mAltTime2 = 0.0f;
    mAltTimer = 0;
    mData = *data;   // rep movsd x5
    mpPackageName = data->mpPackageName;
    mbMessengerOpen = 0;
    mHelpHash = data->mHelpHash;
    mpTextInput = 0;
    mTextInputState = mbIgnoreInputGate = 0;
    mbSounds = 1;
    mSoundTrigger = 0;
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

bool MenuScreen::CheckKeyboard(uint32_t msg)
{
    if (msg == 0xC98356BAu && mpTextInput) {
        if (mTextInputState == 1) {
            mTextInputState = 2;
            return true;
        }
        if (mTextInputState == 2) {
            mTextInputState = 3;
            *bridge::TextInputFlag() = 1;
        }
    }
    if (!mpTextInput) return false;
    if (msg == 0xD15C04ECu) {   // input finished: copy, then wipe both buffers and close
        bStrNCpy(bridge::EditedStringBuffer(), bridge::TextInputBuffer(), 0x100);
        *bridge::TextInputFlag() = 0;
        *bridge::TextInputHandle() = 0;
        std::memset(bridge::EditedStringBuffer(), 0, 0x100);
        std::memset(bridge::TextInputBuffer(), 0, 0x100);
        bridge::TextInput_Close(mpTextInput, 1);
        EndTextInput();
        return false;
    }
    if (*bridge::TextInputFlag() && *bridge::TextInputHandle()) {
        if (msg == 0x911AB364u) bridge::TextInput_Accept(mpTextInput);
        else if (msg == 0x406415E3u) bridge::TextInput_Cancel(mpTextInput);
        else bridge::TextInput_Notify(mpTextInput, msg);
    }
    return true;
}

void MenuScreen::EndTextInput()
{
    if (mpTextInput) {
        *bridge::TextInputFlag() = 0;
        *bridge::TextInputHandle() = 0;
        std::memset(bridge::EditedStringBuffer(), 0, 0x100);
        std::memset(bridge::TextInputBuffer(), 0, 0x100);
        bridge::FreeArray(mpTextInput);
    }
    mpTextInput = nullptr;
    mTextInputState = 0;
}

void MenuScreen::FEngBeginTextInput(uint32_t objHash, uint32_t maxLen, char *buffer, char *, uint32_t mode, uint32_t)
{
    auto *obj = static_cast<void **>(bridge::Alloc(0xC));
    if (obj) {
        obj[0] = FEngFindString(mpPackageName, objHash);
        obj[1] = this;
        obj[2] = nullptr;
        bridge::TextInput_Start(obj, maxLen, buffer, mode);
    }
    mpTextInput = obj;
    *bridge::TextInputFlag() = 0;
    mTextInputState = 1;
}

void MenuScreen::BaseNotifySound(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg)
{
    if (!mbSounds) return;
    uint32_t event = 0;
    switch (msg) {
    case 0x0C407210u: case 0x406415E3u: event = 8; break;
    case 0x72619778u: event = 6; break;
    case 0x911AB364u: event = 10; break;
    case 0x911C0A4Bu: event = 7; break;
    case 0x9120409Eu: event = 4; break;
    case 0xB5971BF1u: event = 5; break;
    case 0xB5AF2461u: event = 9; break;
    default: break;
    }
    event = bridge::MenuScreen_NotifySoundMessage(this, msg, obj, arg, pkg, event);   // vtable +0x0C
    bridge::MakeFrontendSound(event, mSoundTrigger);
}

void MenuScreen::SetAlternatingText(FEObject *a, FEObject *b, float timeA, float timeB)
{
    if (mpAltCurrent) return;
    mpAltText1 = a;
    mpAltText2 = b;
    mpAltCurrent = a && b ? a : nullptr;
    mAltTime1 = timeA;
    mAltTime2 = std::fabs(static_cast<long double>(timeB)) > 0.0001f || timeB != timeB ? timeB : timeA;
    mAltTimer = FEFtol(static_cast<long double>(timeA) * 4000.0f + 0.5f);
    FEngSetScript(a, "Fade_In", true);
    FEngSetScript(mpAltText2, "Fade_Out", true);
}

void MenuScreen::SetAlternatingPlayerCash()
{
    FEObject *b = FEngFindString(mpPackageName, 0xEB3A688Au);
    FEObject *a = FEngFindString(mpPackageName, 0xEB406FECu);
    SetAlternatingText(a, b, 0.75f, 5.0f);
    if (mpAltCurrent) {
        bridge::FEPrintfString(mpAltText1, "%s", bridge::ProfileNameBuffer());
        bridge::FEPrintfInt(mpAltText2, "%$d", bridge::PlayerCash());
    }
}

void MenuScreen::AlternateTheText()
{
    if (!mpAltCurrent) return;
    if (bridge::Timer_CountDown(&mAltTimer, bridge::RealTimeElapsed())) return;
    FEngSetScript(mpAltCurrent, "Fade_Out", true);
    float t;
    if (mpAltCurrent == mpAltText1) {
        mpAltCurrent = mpAltText2;
        t = mAltTime2;
    } else {
        mpAltCurrent = mpAltText1;
        t = mAltTime1;
    }
    FEngSetScript(mpAltCurrent, "Fade_In", true);
    mAltTimer = FEFtol(static_cast<long double>(t) * 4000.0f + 0.5f);
    bridge::FEPrintfInt(mpAltText2, "%$d", bridge::PlayerCash());
}

void MenuScreen::UpdateEAMIcons()
{
    const uint8_t state = *bridge::EAMIconState();
    for (int i = 0; i < 4; ++i) {
        char name[32];
        std::snprintf(name, sizeof name, "PC_EAM_Status_%d", i + 1);   // PC: FEngHashStringUpper (bVSPrintf + hash)
        const uint32_t h = FEHashUpper(name);
        const char *pkg = mpPackageName;
        switch (i) {
        case 0:
            if (state & 1) {
                FEngSetTextureHash(pkg, h, FEHashUpper("U2_OLX_Mail"));
                FEngSetVisible(pkg, h);
                FEngSetScript(pkg, h, "active", true);
            } else {
                FEngSetInvisible(pkg, h);
                FEngSetScript(pkg, h, "hide", true);
            }
            break;
        case 1:
        case 3:
            if (state & (i == 1 ? 2 : 8)) {
                FEngSetTextureHash(pkg, h, FEHashUpper(i == 1 ? "U2_OLX_Challenge_Request" : "U2_OLX_FRIEND"));
                FEngSetVisible(pkg, h);
                FEngSetScript(pkg, h, "active", true);
            } else if (i == 1) {
                FEngSetScript(pkg, h, "hide", true);   // PC: script first for icon 2
                FEngSetInvisible(pkg, h);
            } else {
                FEngSetInvisible(pkg, h);
                FEngSetScript(pkg, h, "hide", true);
            }
            break;
        default:   // 2: no visibility change
            if (state & 4) {
                FEngSetTextureHash(pkg, h, FEHashUpper("U2_OLX_Voice_Request"));
                FEngSetScript(pkg, h, "active", true);
            } else {
                FEngSetScript(pkg, h, "hide", true);
            }
            break;
        }
    }
}

namespace {
void SetPackageMasks(FEPackage *p, uint32_t v)   // FEPackage +0x1C..+0x28 control masks
{
    auto *m = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(p) + 0x1C);
    m[0] = m[1] = m[2] = m[3] = v;
}
uint32_t PackageMask(const FEPackage *p) { uint32_t v; std::memcpy(&v, reinterpret_cast<const uint8_t *>(p) + 0x1C, 4); return v; }

// PC 55B969..: EA online login (servers are gone; kept for behaviour). Returns nothing; on failure a dialog.
void OnlineLogin(MenuScreen &s)
{
    char bufB[0xD] = {};
    char bufA[0x24] = {};
    bool found = false;
    uint32_t id = 0;
    for (int32_t i = 0; i < 5; ++i) {
        const uint8_t *e = static_cast<const uint8_t *>(bridge::Online_GetEntry(bridge::Online_Manager(), i));
        if (!e) break;
        if (bStrCmp(reinterpret_cast<const char *>(e + 0xC), bridge::OnlineUserName()) == 0) {
            std::memcpy(&id, e + 8, 4);
            found = true;
        }
    }
    bool ok = false;
    if (found) {
        const uint8_t *acct = static_cast<const uint8_t *>(bridge::Online_Find(bridge::Online_Manager(), bridge::OnlineUserName(), id));
        void *dict;
        std::memcpy(&dict, acct + 0x20, sizeof dict);
        bridge::Online_CopyOrDefault(bridge::Online_DictGet(dict, "GAMENAME"), bufA, 0x24, "unknown game");
        std::memcpy(&dict, acct + 0x20, sizeof dict);
        bridge::Online_CopyOrDefault(bridge::Online_DictGet(dict, "PASSWORD"), bufB, 0xD, bridge::GameEmptyString());
        uint32_t aid;
        std::memcpy(&aid, acct + 8, 4);
        bridge::Online_Select(bridge::Online_Manager(), reinterpret_cast<const char *>(acct + 0xC), aid);
        bStrCpy(bridge::OnlineField85009C(), bufB[0] ? bufB : bridge::GameEmptyString());
        bStrCpy(bridge::OnlineField8500A9(), bufB[0] ? bufB : bridge::GameEmptyString());   // PC copies bufB twice
        *bridge::OnlineFlag85009B() = bufB[0] != 0;
        const int32_t r = bridge::Online_Login(bufA, bufB, &s);
        if (r >= 0) {
            *bridge::OnlineLoginHandle() = bridge::Online_558330(s.mpPackageName, "animating", 0x91070FF2u, bridge::OnlineUserName());
            ok = true;
        }
    }
    if (!ok) {
        FEngSetScript(bridge::CallingScreenPackageName(), 0x47FF4E7Cu, "UNDIM", true);
        bridge::ShowOneButtonDialog(s.mpPackageName, bridge::GameEmptyString(), 0x639, -1, -1, bridge::LocalizedString(0x331F4F58u));
    }
}
}  // namespace

void MenuScreen::BaseNotify(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg)
{
    if (*bridge::ResetMasksFlag())
        if (FEPackage *p = FEngFindPackage(mpPackageName)) SetPackageMasks(p, 0);
    if (CheckKeyboard(msg)) return;
    FEPackage *p = FEngFindPackage(mpPackageName);
    if ((mbIgnoreInputGate || !p || !PackageMask(p)) && FEngine::IsInputMessage(msg)) return;   // control mask 0: drop
    bool helpBarCheck = false;
    switch (msg) {
    case 0x2C0E485Au:
        *bridge::ResetMasksFlag() = 0;
        if (!bridge::OnlineBusy()) {   // back to the main menu: pop every non-overlay package
            bridge::Call4A0140(0);
            for (FEPackage *pk = bridge::cFEngInstance()->mpEngine->mpPackagesTail; pk;) {
                FEPackage *prev;
                const char *name;
                std::memcpy(&prev, reinterpret_cast<uint8_t *>(pk) + 8, sizeof prev);
                std::memcpy(&name, reinterpret_cast<uint8_t *>(pk) + 0xC, sizeof name);
                if (!IsOverlayPackage(name)) FEngPopPackage(name);
                pk = prev;
            }
            bridge::FEngSwitchPackages("UI_Main.fng", "GarageMain.fng", 0xFF, 0);
            return;
        }
        break;
    case 0x35F8620Bu:
    case 0x9757A0C0u:
        UpdateEAMIcons();
        break;
    case 0x9803F6E2u: {
        uint32_t m;
        std::memcpy(&m, reinterpret_cast<const uint8_t *>(mData.mpPackage) + 0x1C, 4);
        if (!m) return;
        break;
    }
    case 0xC519BFC0u:   // EA Messenger button
        if (!FEngIsPackagePushed("GenericDialog_MED.fng") &&
            static_cast<long double>(bridge::RealTimer() - bridge::MessengerButtonTimer()) * 0.00025f > bridge::KeyboardDelaySeconds()) {
            if (PackageMask(FEngFindPackage(mpPackageName)) && bridge::ProfileState() >= 0xC && !mbMessengerOpen &&
                bridge::ShowEAMIcons()) {
                static const char *const kBlocking[] = {"GenericDialog.fng", "GenericDialog_LARGE.fng", "GenericDialog_MED.fng",
                    "GenericDialog_SMALL.fng", "GenericDialog_Animate_LARGE.fng", "GenericDialog_Animate_MED.fng",
                    "GenericDialog_Animate_SMALL.fng", "GenericDialog_ThreeButton.fng", "HelpDialog_LARGE.fng", "HelpDialog_MED.fng",
                    "HelpDialog_SMALL.fng"};
                bool blocked = false;
                for (int k = 0; k < 11 && !blocked; ++k) blocked = FEngIsPackagePushed(kBlocking[k]);
                if (!blocked && !bridge::CallingScreenFrozen()) {
                    bridge::PlayUISound(0x4A);
                    bStrCpy(bridge::CallingScreenPackageName(), mpPackageName);
                    bridge::FEngPushPackageOver("UI_OLEAMessenger.fng", mpPackageName, 0xFF, 0);
                    bridge::SetCallingScreenFrozen(true);
                }
            }
        }
        helpBarCheck = true;
        break;
    case 0xC519BFC3u: {   // help
        FEPackage *pp = FEngFindPackage(mpPackageName);
        if ((arg & PackageMask(pp)) && mHelpHash) {
            bridge::MakeFrontendSound(2, 0);   // MakeFrontendSound(pkg, 2)
            bridge::ShowHelpMessage(mpPackageName, bridge::GameEmptyString(), mHelpHash);
        }
        break;
    }
    case 0xC98356BAu:
        AlternateTheText();
        break;
    case 0xF764CBE7u:
        if (bridge::ProfileState() >= 0xC) OnlineLogin(*this);
        *bridge::ResetMasksFlag() = 0;
        if (FEPackage *hb = FEngFindPackage("UI_PC_Help_Bar.fng")) SetPackageMasks(hb, 0xFF);
        break;
    case 0x911AB364u:
    case 0x0C407210u:
    case 0x406415E3u:
        helpBarCheck = true;
        break;
    default:
        break;
    }
    if (helpBarCheck && arg == 3 && bridge::HelpBar_HandlesSelect()) return;   // PC help bar takes the click
    bridge::MenuScreen_NotificationMessage(this, msg, obj, 1, pkg);              // vtable +0x08 (PC passes 1)
}
