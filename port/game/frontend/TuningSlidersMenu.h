// GC TuningSlidersMenu (zFe2.cpp; nfsu2-re PerformanceTuningSlidersFNGObject): the performance tuning slider
// screen "UI_PerformanceTuning_Sliders.fng" (category global PC 0x83A648: 0 suspension, 1 drivetrain, 2 aero,
// 3 wheels/brakes). PC 0x154 bytes (factory 55DB26), vtable 0x79D360 (5 slots): 0 scalar dtor 55AD00,
// 1 40D080 (UIWidgetMenu's), 2 NotificationMessage 5650C0, 3 570A70, 4 Setup 557780.
#pragma once
#include <cstddef>
#include <cstdint>
#include "UIWidgetMenu.h"

struct ScreenConstructorData;
struct FEObject;

struct TuningSlidersMenu {
    UIWidgetMenu base;               // +0x000
    uint8_t mbLeaving;               // +0x14C (GC +0x16C) set by accept / 0xE1FDE1D1; 0xC98356BA then pops the screen
    uint8_t _pad14D[3];
    int32_t mStartTime;              // +0x150 (GC +0x170) RealTimer at construction (accept ignored in that tick)

    TuningSlidersMenu *Construct(const ScreenConstructorData *data);   // GC 8017F2F0 / PC 55AC10 (ret 4)
    void Destruct();                                                   // GC 8018D4B8 / PC 55AD20 (= UIWidgetMenu body)
    TuningSlidersMenu *ScalarDeletingDestructor(uint32_t flags);       // PC 55AD00 (ret 4)
    // GC 8017F370 / PC 5650C0 (ret 0x10): UIWidgetMenu's first, then sounds / reset / tips / accept / back.
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);
    // GC 8017FA14 / PC 510120 (__cdecl): tip text hash for the tuning type (0 for 16, 17 and > 0x16)
    static uint32_t GetTipHash(int32_t type);
    bool IsOkToResume() const;       // GC 8017FB18 (PC inlined): not in the construction tick
    // GC 8017FB44 / PC 549850 (ret 0x14): -1 if the type is unavailable; else a new TuningSlider row (the
    // AddToggleOption sequence inlined: arrows style 1 if `plainArrows` else 2), returns its 0-based index.
    int32_t AddTuningSlider(int32_t type, uint32_t minHash, uint32_t maxHash, bool plainArrows, float yOffset);
    void Setup();                    // GC 8017FF18 / PC 557780
    void SetupSuspension();          // GC 80180010 / PC 549B40
    void SetupDrivetrain();          // GC 8018017C / PC 549C20 (+ speed labels, metric 60..420 / else 35..245)
    void SetupAero();                // GC 801803C8 / PC 549DC0
    void SetupWheelBrakes();         // GC 80180438 / PC 549E00
};

// GC FEngMapPortToControlMask / PC 50D2D0 (__cdecl): port 0..3 -> 1 << port, else 0.
uint32_t FEngMapPortToControlMask(int32_t port);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(TuningSlidersMenu) == 0x154 && offsetof(TuningSlidersMenu, mStartTime) == 0x150);
#endif
