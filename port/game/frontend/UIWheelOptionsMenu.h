// GC UIWheelOptionsMenu (zFe2.cpp): the steering wheel options screen ("UI_OptionsController"-style widget menu)
// and its WCO* slider options. PC 0x154 bytes (factory 4E37A0), vtable 0x795E4C: 0 scalar dtor 4CFC80,
// 1 40D080, 2 NotificationMessage 49CBB0, 3 NotifySoundMessage 49DFC0, 4 Setup 4B8A10.
// PC differs from GC: no WCOTurning / WCOLinearity toggles, only the three sliders.
#pragma once
#include <cstddef>
#include <cstdint>
#include "UIWidgetMenu.h"
#include "FEWidget.h"

struct ScreenConstructorData;
struct FEObject;

// GC WCO* (PC 0x9C): FESliderWidget + the player whose options it edits (GetPlayerToEditForOptions, PC 0x836364).
// Options record: PC 0x83AA64 + player * 0x28 (floats +0x1C wheel range, +0x20 strength of force, +0x24 overall).
// Slider widget vtable slots used: 1 Act, 3 Draw, 16 BlinkArrows, 17 SetInitialValues, 18 GetValue.
struct WCOSlider {
    FESliderWidget base;             // +0x00
    int32_t mPlayer;                 // +0x98

    // shared Act body (PC 49B350 / 49B400 / 49B550, ret 8): slider Update (virtual); moved -> BlinkArrows; then
    // the option field = GetValue (virtual, rounded to float); Draw (virtual).
    void ActField(uint32_t msg, uint32_t fieldOffset);
};

// PC vtable 0x791E40: Act 49B350 (+0x1C), Draw 49B3A0 (hash 0x56FEB0D2), SetInitialValues 49B3C0 (min 0.1, max 1,
// step 0.1), GetValue = FESliderWidget's 45D300.
struct WCOWheelRange {
    WCOSlider base;
    void Act(const char *pkg, uint32_t msg) { base.ActField(msg, 0x1C); }   // GC 800EC864
    void Draw();                     // GC 800EC90C
    void SetInitialValues();         // GC 800EC938
};
// PC vtable 0x791E90: Act 49B400 (+0x20), GetValue 49B450 (20 - clamp(v, 0, 1) * 18), Draw 49B4B0 (0x1CC4B396),
// SetInitialValues 49B4D0 (inverse: 1 - (clamp(x, 2, 20) - 2) / 18, min 0, max 1, step 0.1).
struct WCOStrengthOfForce {
    WCOSlider base;
    void Act(const char *pkg, uint32_t msg) { base.ActField(msg, 0x20); }   // GC 800EC998
    long double GetValue() const;    // GC 800ECA40 (x87 result)
    void Draw();                     // GC 800ECA84
    void SetInitialValues();         // GC 800ECAB0
};
// PC vtable 0x791EE0: Act 49B550 (+0x24), Draw 49B5A0 (0x28F42AE8), SetInitialValues 49B5C0 (min 0, max 1, step 0.1).
struct WCOOverallForceStrength {
    WCOSlider base;
    void Act(const char *pkg, uint32_t msg) { base.ActField(msg, 0x24); }   // GC 800ECB4C
    void Draw();                     // GC 800ECBF4
    void SetInitialValues();         // GC 800ECC20
};

struct UIWheelOptionsMenu {
    UIWidgetMenu base;               // +0x000
    int32_t mTurningIndex;           // +0x14C (GC +0x16C toggle row index; PC: only cleared)
    int32_t mLinearityIndex;         // +0x150 (GC +0x170)

    UIWheelOptionsMenu *Construct(const ScreenConstructorData *data);   // GC 800EF990 / PC 4CFBA0 (ret 4)
    void Destruct();                                                    // GC 801182B0 / PC 4CFCA0
    UIWheelOptionsMenu *ScalarDeletingDestructor(uint32_t flags);       // PC 4CFC80 (ret 4)
    // GC 800EFA48 / PC 49CBB0 (ret 0x10): back starts the out-script once; its end switches to UI_Options.fng.
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);
    // GC 80118348 / PC 49DFC0 (ret 0x14): FilterSoundForMenu(msg, event)
    static uint32_t NotifySoundMessage(uint32_t msg, uint32_t event);
    void Setup();                    // GC 800EFAB0 / PC 4B8A10
};

// GC FilterSoundForMenu / PC 50D1D0 (__cdecl): events 4 / 5 (left / right) -> 0, else the event (message unused).
uint32_t FilterSoundForMenu(uint32_t msg, uint32_t event);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(WCOSlider) == 0x9C && sizeof(UIWheelOptionsMenu) == 0x154);
#endif
