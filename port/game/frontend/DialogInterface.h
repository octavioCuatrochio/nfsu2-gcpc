// GC DialogInterface (zFe2.cpp): the generic FEng dialogs (GenericDialog_*.fng / HelpDialog_*.fng). One dialog at a
// time: its config is copied to SecretDialogInfo (PC 0x838640) and pushed true-modal over its parent package.
// PC: every Show* variant is a thin varargs wrapper (format, or language hash -> 0x200-byte copy) around a core
// taking a va_list; the text goes through bVSPrintf (PC 43FBC0).
#pragma once
#include <cstdarg>
#include <cstddef>
#include <cstdint>

// GC feDialogConfig; PC 0x338 (ctor 4FF250 zeroes everything).
struct feDialogConfig {
    char mText[0x300];               // +0x000 formatted text
    uint32_t mParams[8];             // +0x300 button text / message hashes; slots differ per variant (see the .cpp)
    const char *mpParentPkg;         // +0x320
    const char *mpDialogPkg;         // +0x324 explicit package, "3button" / "animating" or null (see CalcFEngDialogPackageName)
    int32_t mNumButtons;             // +0x328
    uint8_t mbFlag32C;               // +0x32C
    uint8_t mbHelp;                  // +0x32D HelpDialog_*.fng
    uint8_t _unknown32E;
    uint8_t mbFlag32F;               // +0x32F cleared by ShowDialog
    uint32_t mDismissedByOtherMsg;   // +0x330 sent to the parent when another dialog replaces this one
    int32_t mHandle;                 // +0x334 SecretDialogInfo: current dialog handle (0 = none)

    void Construct();                // PC 4FF250 (GC feDialogConfig::feDialogConfig): all zero
};

namespace DialogInterface {
// GC 8011EDEC / PC 526C40: explicit name (unless "animating" / "3button"); help dialogs: > 4 lines ('\n' / '^') MED
// else SMALL; else width = strlen * font CONDUITMDITC_TT21I +0x24: "3button" ThreeButton, < 2561 SMALL, < 5122 MED,
// else LARGE ("animating": the Animate_ variants).
const char *CalcFEngDialogPackageName(const feDialogConfig *cfg);
// GC 8011E2CC / PC 558020: 0 if dialogs are blocked (PC byte 0x8383B4). Replaces the current dialog (its dismissed
// message to its parent; a parent equal to the old dialog becomes the old parent), control mask from the parent (or
// the old dialog), else the top package's, else 0xFF; copies the config, new handle (PC 0x7F6DE0++), create
// callback 5542A0, cFEng::Pusher(0x16, dialog, parent, mask, &SecretDialogInfo). Returns the handle.
int32_t ShowDialog(feDialogConfig *cfg);
void DismissDialog(int32_t handle);   // GC 8011E258 / PC 540A60: current one -> cFEng::Popper, handle 0
void SetDismissedByAnotherDialogMsg(uint32_t msg);   // GC 8011EFF8 (PC: inlined, store to 0x838970)

// va_list cores (PC __cdecl)
int32_t ShowMessageV(const char *parent, const char *dlg, const char *fmt, va_list va);   // PC 5582A0: 0 buttons
int32_t ShowOkV(const char *parent, const char *dlg, const char *fmt, va_list va);        // PC 558390: OK (0x639)
// PC 558490: 1 button, +0x310 a, +0x314 b, +0x318 c, +0x32C flag
int32_t ShowOneButtonV(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t c, uint8_t flag,
                       const char *fmt, va_list va);
// PC 558680: 2 buttons, +0x300 a, +0x308 b, +0x304 c, +0x30C d, +0x318 e, +0x32C flag, +0x31C f
int32_t ShowTwoButtonsV(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                        uint32_t e, uint8_t flag, uint32_t f, const char *fmt, va_list va);
// PC 5588F0 (PC only): 3 buttons, +0x300 a, +0x308 b, +0x310 c, +0x304 d, +0x30C e, +0x314 f, +0x318 g, +0x31C h
int32_t ShowThreeButtonsV(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                          uint32_t e, uint32_t f, uint32_t g, uint32_t h, const char *fmt, va_list va);
// GC 8011ED5C / PC 558AC0: help text (language hash, 0x300 bytes), OK button, help package.
int32_t ShowHelpMessage(const char *parent, const char *dlg, uint32_t hash);
// Language hash -> the string (missing: 0xC01A6F63) copied into a 0x200 buffer (the wrappers' format).
void LocalizedFormat(char *buf, uint32_t hash);
// PC 558790 (two buttons, flag 1, language-hash text): a, b, c, d, e as ShowTwoButtonsV, then f (+0x31C).
int32_t ShowTwoButtonsHash(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                           uint32_t e, uint32_t f, uint32_t hash, ...);
// PC 558570 (one button, flag 1, language-hash text): a, b, c as ShowOneButtonV.
int32_t ShowOneButtonHash(const char *parent, const char *dlg, uint32_t a, uint32_t b, uint32_t c, uint32_t hash, ...);
}  // namespace DialogInterface

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(feDialogConfig) == 0x338 && offsetof(feDialogConfig, mpParentPkg) == 0x320 &&
              offsetof(feDialogConfig, mNumButtons) == 0x328 && offsetof(feDialogConfig, mHandle) == 0x334);
#endif
