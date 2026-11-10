// Base of every frontend screen (GC MenuScreen, zFe2.cpp). PC 0x4C bytes, vtable 0x79AC00: 0 scalar dtor 51DC60,
// 1 MaybeOverrideActiveButtons (folded 40D080: returns its arg), 2 NotificationMessage (pure), 3 NotifySoundMessage
// (570A70: returns 0, ret 0x14). PC differs from GC: no per-screen EAM flag (global 0x838528 instead), and the
// constructor records the calling screen's package and syncs the PC help bar.
#pragma once
#include <cstddef>
#include <cstdint>

struct FEPackage;

// GC ScreenConstructorData (5 words, copied whole into the screen)
struct ScreenConstructorData {
    const char *mpPackageName;   // +0x00
    FEPackage *mpPackage;        // +0x04 (GC BaseNotify reads its control mask +0x28)
    uint32_t mHelpHash;          // +0x08
    uint32_t mHelpBarMask;       // +0x0C PC help bar button mask (0: help bar untouched)
    uint32_t mArg;               // +0x10
};

struct MenuScreen {
    void *vtable;                    // +0x00
    const char *mpPackageName;       // +0x04 (GC +0x00)
    uint8_t _unknown08;              // +0x08 (GC +0x04 word)
    uint8_t _pad09[3];
    uint32_t _unknown0C;             // +0x0C (GC +0x08 FEngTextInputObject*)
    uint8_t _unknown10, _unknown11;  // +0x10 (GC +0x0C / +0x0D: BaseNotify input gate)
    uint8_t _pad12[2];
    ScreenConstructorData mData;     // +0x14 (GC +0x10)
    uint32_t mHelpHash;              // +0x28 copy of mData.mHelpHash
    uint8_t _unknown2C;              // +0x2C = 1
    uint8_t _pad2D[3];
    uint32_t _unknown30;             // +0x30
    void *_unknown34, *_unknown38, *_unknown3C;   // +0x34 (GC: FEString*, FEString*, FEObject*: alternating text)
    float _unknown40, _unknown44;    // +0x40
    uint32_t _unknown48;             // +0x48

    MenuScreen *Construct(const ScreenConstructorData *data);   // GC 801515A0 / PC 551DE0 (ret 4)
    MenuScreen *ScalarDeletingDestructor(uint32_t flags);       // GC 80151734 / PC 51DC60 (ret 4)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(MenuScreen) == 0x4C && offsetof(MenuScreen, mData) == 0x14 && offsetof(MenuScreen, mHelpHash) == 0x28);
#endif

// PC 49FFA0 (__cdecl, PC only): overlay packages that never become the calling screen (ScreenPrintf, Chyron_FE,
// GarageMain, UI_PC_Help_Bar). bStrCmp (case-sensitive).
bool IsOverlayPackage(const char *pkg);
// PC 512170 (__cdecl, PC only): the seven GenericDialog_*.fng packages (FEHashUpper compare).
bool IsGenericDialogPackage(const char *pkg);
