// Base of every frontend screen (GC MenuScreen, zFe2.cpp). PC 0x4C bytes, vtable 0x79AC00: 0 scalar dtor 51DC60,
// 1 MaybeOverrideActiveButtons (folded 40D080: returns its arg), 2 NotificationMessage (pure), 3 NotifySoundMessage
// (570A70: returns 0, ret 0x14). PC differs from GC: no per-screen EAM flag (global 0x838528 instead), and the
// constructor records the calling screen's package and syncs the PC help bar.
#pragma once
#include <cstddef>
#include <cstdint>

struct FEPackage;
struct FEObject;

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
    uint8_t mbMessengerOpen;         // +0x08 (GC +0x04 word) set: the EAM button does not open the messenger
    uint8_t _pad09[3];
    void *mpTextInput;               // +0x0C (GC +0x08 FEngTextInputObject*) PC: 0xC-byte input object (see BeginTextInput)
    uint8_t mTextInputState;         // +0x10 (GC +0x0C) 1 started, 2/3 by message 0xC98356BA
    uint8_t mbIgnoreInputGate;       // +0x11 (GC +0x0D) set: input messages pass even with control mask 0... (see BaseNotify)
    uint8_t _pad12[2];
    ScreenConstructorData mData;     // +0x14 (GC +0x10)
    uint32_t mHelpHash;              // +0x28 copy of mData.mHelpHash
    uint8_t mbSounds;                // +0x2C = 1 (GC +0x2C int): BaseNotifySound enabled
    uint8_t _pad2D[3];
    uint32_t mSoundTrigger;          // +0x30 trigger for BaseNotifySound / UIWidgetMenu sounds
    FEObject *mpAltText1, *mpAltText2, *mpAltCurrent;   // +0x34 alternating texts (profile name / cash) + the shown one
    float mAltTime1, mAltTime2;      // +0x40 seconds each text stays
    int32_t mAltTimer;               // +0x48 Timer::CountDown ticks (seconds * 4000)

    MenuScreen *Construct(const ScreenConstructorData *data);   // GC 801515A0 / PC 551DE0 (ret 4)
    MenuScreen *ScalarDeletingDestructor(uint32_t flags);       // GC 80151734 / PC 51DC60 (ret 4)

    // GC 80151768 / PC 55B5C0 (ret 0x10): global input handling, then NotificationMessage (slot 2) with arg 1 (PC).
    void BaseNotify(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);
    // GC 80151AB8 / PC 54E5B0 (ret 4): true = the text input took the message
    bool CheckKeyboard(uint32_t msg);
    void EndTextInput();                                        // PC 51DC80 (PC only): free the input object, clear buffers
    // GC 80151968 / PC 555F80 (ret 0x18): PC creates a 0xC input object {string, screen, 0} on the manager 0x83A7B0;
    // buffer2 and flags are unused on PC.
    void FEngBeginTextInput(uint32_t objHash, uint32_t maxLen, char *buffer, char *buffer2, uint32_t mode, uint32_t flags);
    // GC 80151B4C / PC 51DCD0 (ret 0x10): message -> sound event, NotifySoundMessage (slot 3) may change it
    void BaseNotifySound(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);
    void SetAlternatingPlayerCash();                            // GC 80151D20 / PC 537E60
    void SetAlternatingText(FEObject *a, FEObject *b, float timeA, float timeB);   // GC 80151DF0 / PC 51DD90 (ret 0x10)
    void AlternateTheText();                                    // GC 80151ED8 / PC 537F50
    void UpdateEAMIcons();                                      // GC 80151FC8 / PC 542720: "PC_EAM_Status_1..4"
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(MenuScreen) == 0x4C && offsetof(MenuScreen, mData) == 0x14 && offsetof(MenuScreen, mHelpHash) == 0x28);
#endif

// PC 49FFA0 (__cdecl, PC only): overlay packages that never become the calling screen (ScreenPrintf, Chyron_FE,
// GarageMain, UI_PC_Help_Bar). bStrCmp (case-sensitive).
bool IsOverlayPackage(const char *pkg);
// PC 512170 (__cdecl, PC only): the seven GenericDialog_*.fng packages (FEHashUpper compare).
bool IsGenericDialogPackage(const char *pkg);
