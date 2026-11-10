#include "DialogInterface.h"
#include <cstring>
#include "bWare/bString.h"
#include "bridge/Bridge.h"
#include "cFEng.h"
#include "FEPackage.h"
#include "FEPackageManager.h"
#include "FELocalize.h"
#include "FEngFont.h"

void feDialogConfig::Construct() { std::memset(this, 0, sizeof *this); }

namespace DialogInterface {
namespace {
feDialogConfig &Secret() { return *bridge::SecretDialogInfo(); }
// PC loads the width as float(strlen) * float, stored to memory
float TextWidth(const feDialogConfig *cfg)
{
    const FEngFont *f = FEngFindFontNative(bStringHash("CONDUITMDITC_TT21I"));
    const float h = f->mfLineHeight;
    return static_cast<float>(static_cast<long double>(static_cast<int32_t>(std::strlen(cfg->mText))) * h);
}
}  // namespace

const char *CalcFEngDialogPackageName(const feDialogConfig *cfg)
{
    const char *s = cfg->mpDialogPkg;
    if (s && *s && bStrCmp(s, "animating") != 0 && bStrCmp(s, "3button") != 0) return s;
    if (cfg->mbHelp) {
        int32_t lines = 1;
        for (const char *p = cfg->mText; *p; ++p)
            if (*p == '\n' || *p == '^') ++lines;
        return lines > 4 ? "HelpDialog_MED.fng" : "HelpDialog_SMALL.fng";
    }
    const float w = TextWidth(cfg);
    if (bStrCmp(s, "3button") == 0) return "GenericDialog_ThreeButton.fng";
    const bool anim = bStrCmp(s, "animating") == 0;
    if (w < 2561.0f) return anim ? "GenericDialog_Animate_SMALL.fng" : "GenericDialog_SMALL.fng";   // PC 0x79B894
    if (w < 5122.0f) return anim ? "GenericDialog_Animate_MED.fng" : "GenericDialog_MED.fng";       // PC 0x79B890
    return anim ? "GenericDialog_Animate_LARGE.fng" : "GenericDialog_LARGE.fng";
}

void SetDismissedByAnotherDialogMsg(uint32_t msg) { Secret().mDismissedByOtherMsg = msg; }

void DismissDialog(int32_t handle)
{
    feDialogConfig &s = Secret();
    if (s.mHandle != handle) return;
    if (cFEng *fe = bridge::cFEngInstance()) fe->Popper(s.mpDialogPkg);
    s.mHandle = 0;
}

int32_t ShowDialog(feDialogConfig *cfg)
{
    if (*bridge::GameByte(0x8383B4)) return 0;
    cfg->mpDialogPkg = CalcFEngDialogPackageName(cfg);
    cfg->mbFlag32F = 0;
    cfg->mDismissedByOtherMsg = 0;
    feDialogConfig &s = Secret();
    uint32_t mask = 0xFF;
    bool checkMask = true;
    if (s.mHandle == 0) {
        FEPackage *p = FEngFindPackage(cfg->mpParentPkg);
        if (p) mask = p->mu32ControlMask;
        else checkMask = false;
    } else {
        if (s.mDismissedByOtherMsg) bridge::TheFEPackageManager()->SendMessageToPackage(s.mpParentPkg, s.mDismissedByOtherMsg);
        if (std::strcmp(cfg->mpParentPkg, s.mpDialogPkg) == 0) cfg->mpParentPkg = s.mpParentPkg;   // not under itself
        if (FEPackage *p = FEngFindPackage(s.mpDialogPkg)) mask = p->mu32ControlMask;
        DismissDialog(s.mHandle);
    }
    if (checkMask && mask == 0) {
        uint32_t skip[4];
        skip[0] = bridge::FEngHashStringUpper("ScreenPrintf.fng");
        skip[1] = bridge::FEngHashStringUpper("Chyron_FE.fng");
        skip[2] = bridge::FEngHashStringUpper("GarageMain.fng");
        skip[3] = bridge::FEngHashStringUpper("UI_PC_Help_Bar.fng");
        const char *top = bridge::FEngGetTopPackage(skip, 4);
        FEPackage *p = FEngFindPackage(top);
        mask = p ? p->mu32ControlMask : 0;
        if (mask == 0) mask = 0xFF;
    }
    s = *cfg;
    int32_t *handle = bridge::GameInt(0x7F6DE0);   // gDialogHandle
    s.mHandle = (*handle)++;
    bridge::FEngSetCreateCallback(s.mpDialogPkg, bridge::DialogCreaterCallback());
    if (cFEng *fe = bridge::cFEngInstance())
        fe->Pusher(0x16, s.mpDialogPkg, cfg->mpParentPkg, mask, static_cast<int32_t>(reinterpret_cast<uintptr_t>(&s)));
    return s.mHandle;
}

namespace {
int32_t Show(feDialogConfig &c, const char *parent, const char *dlg)
{
    c.mpParentPkg = parent;
    c.mpDialogPkg = dlg;
    return ShowDialog(&c);
}
}  // namespace

int32_t ShowMessageV(const char *parent, const char *dlg, const char *fmt, va_list va)
{
    feDialogConfig c;
    c.Construct();
    bridge::bVSPrintf(c.mText, fmt, va);
    c.mNumButtons = 0;
    c.mbFlag32C = 0;
    return Show(c, parent, dlg);
}

int32_t ShowOkV(const char *parent, const char *dlg, const char *fmt, va_list va)
{
    feDialogConfig c;
    c.Construct();
    bridge::bVSPrintf(c.mText, fmt, va);
    c.mNumButtons = 1;
    c.mbFlag32C = 1;
    c.mParams[4] = 0x639;           // "OK"
    c.mParams[5] = 0x34DC1BECu;
    c.mParams[6] = 0xB4EDEB6Du;
    c.mParams[7] = 0;
    return Show(c, parent, dlg);
}

int32_t ShowOneButtonV(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t cc, uint8_t flag,
                       const char *fmt, va_list va)
{
    feDialogConfig c;
    c.Construct();
    bridge::bVSPrintf(c.mText, fmt, va);
    c.mParams[4] = a;
    c.mParams[5] = b;
    c.mParams[6] = cc;
    c.mNumButtons = 1;
    c.mParams[7] = 0;
    c.mbFlag32C = flag;
    return Show(c, parent, dlg);
}

int32_t ShowTwoButtonsV(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t cc, uint32_t d,
                        uint32_t e, uint8_t flag, uint32_t f, const char *fmt, va_list va)
{
    feDialogConfig c;
    c.Construct();
    bridge::bVSPrintf(c.mText, fmt, va);
    c.mParams[0] = a;
    c.mParams[2] = b;
    c.mParams[1] = cc;
    c.mParams[3] = d;
    c.mParams[6] = e;
    c.mParams[7] = f;
    c.mNumButtons = 2;
    c.mbFlag32C = flag;
    return Show(c, parent, dlg);
}

int32_t ShowThreeButtonsV(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t cc, uint32_t d,
                          uint32_t e, uint32_t f, uint32_t g, uint32_t h, const char *fmt, va_list va)
{
    feDialogConfig c;
    c.Construct();
    bridge::bVSPrintf(c.mText, fmt, va);
    c.mParams[0] = a;
    c.mParams[2] = b;
    c.mParams[4] = cc;
    c.mParams[1] = d;
    c.mParams[3] = e;
    c.mParams[5] = f;
    c.mParams[6] = g;
    c.mParams[7] = h;
    c.mNumButtons = 3;
    c.mbFlag32C = 1;
    return Show(c, parent, dlg);
}

int32_t ShowHelpMessage(const char *parent, const char *dlg, uint32_t hash)
{
    feDialogConfig c;
    c.Construct();
    bStrNCpy(c.mText, GetLocalizedString(hash), 0x300);
    c.mNumButtons = 1;
    c.mbFlag32C = 1;
    c.mbHelp = 1;
    c.mParams[0] = 0x639;
    c.mParams[1] = 0x34DC1BECu;
    c.mParams[6] = 0xB4EDEB6Du;
    c.mParams[7] = 0;
    return Show(c, parent, dlg);
}

void LocalizedFormat(char *buf, uint32_t hash) { bStrNCpy(buf, GetLocalizedString(hash), 0x200); }

int32_t ShowTwoButtonsHash(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                           uint32_t e, uint32_t f, uint32_t hash, ...)
{
    char text[0x200];
    LocalizedFormat(text, hash);
    va_list va;
    va_start(va, hash);
    const int32_t r = ShowTwoButtonsV(parent, dlg, a, b, c, d, e, 1, f, text, va);
    va_end(va);
    return r;
}

int32_t ShowOneButtonHash(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t c, uint32_t hash, ...)
{
    char text[0x200];
    LocalizedFormat(text, hash);
    va_list va;
    va_start(va, hash);
    const int32_t r = ShowOneButtonV(parent, dlg, a, b, c, 1, text, va);
    va_end(va);
    return r;
}
}  // namespace DialogInterface
