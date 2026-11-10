// GC UIOptionsScreen (zFe2.cpp): one options page (UI_Options.fng), page index PC *0x83AA04: 0 audio, 1 video
// (display), 2 gameplay, 3 player, 6 trailers, 7 online (4, 5, 8: title / help only). PC 0x164 bytes (factory
// 4E3740), vtable 0x795E28: 0 scalar dtor 4CFAF0, 1 40D080, 2 NotificationMessage 49C690, 3 NotifySoundMessage
// 4CFAC0, 4 Setup 4B8470. PC differs from GC: video page = resolution / detail / two buttons, wheel button and the
// force-feedback toggle on the player page, mode-8 (GameModeFlags) help objects.
#pragma once
#include <cstddef>
#include <cstdint>
#include "UIWidgetMenu.h"

struct ScreenConstructorData;
struct FEObject;

struct UIOptionsScreen {
    UIWidgetMenu base;               // +0x000
    uint8_t mbColorCal;              // +0x14C cleared by Setup (colour calibration objects hidden)
    uint8_t mbBusy;                  // +0x14D set while the trailers page leaves: base notifications skipped
    uint8_t mbPlayerPage;            // +0x14E set by SetupPlayer: NotifySoundMessage plays sound 0xB (not in mode 8)
    uint8_t mbAutoSaveAsked;         // +0x14F (read only here) autosave-off warning gate
    uint8_t mbAutoSave2Asked;        // +0x150 (read only here)
    uint8_t _pad151[3];
    int32_t mWheelButtonIndex;       // +0x154 COWheelOptions row (1-based)
    int32_t mForceFeedbackIndex;     // +0x158 PC toggle 791DB0 row
    uint8_t _unknown15C[8];

    UIOptionsScreen *Construct(const ScreenConstructorData *data);   // GC 800ED3D4 / PC 4CF750 (ret 4)
    void Destruct();                                                 // GC 801180C0 / PC 4CFB10 (UIWidgetMenu body)
    UIOptionsScreen *ScalarDeletingDestructor(uint32_t flags);       // PC 4CFAF0 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800ED964 / PC 49C690
    uint32_t NotifySoundMessage() const;                              // GC 80118158 / PC 4CFAC0 (ret 0x14): 0
    void CheckForWheels();           // GC 800ED79C / PC 49C600 (message 0xC98356BA): wheel button follows the wheel
    void Setup();                    // GC 800EE78C / PC 4B8470
    void SetupAudio();               // GC 800EDCA0 / PC 4B6E60
    void SetupVideo();               // GC 800EDEFC / PC 4B7180 (PC page)
    void SetupGameplay();            // GC 800EE01C / PC 4B7310
    void SetupPlayer();              // GC 800EE22C / PC 4B7CD0
    void SetupOnline();              // GC 800EE614 / PC 4B81A0
    void SetupTrailers();            // GC 800EE6F4 / PC 4B8350
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(UIOptionsScreen) == 0x164 && offsetof(UIOptionsScreen, mWheelButtonIndex) == 0x154);
#endif
