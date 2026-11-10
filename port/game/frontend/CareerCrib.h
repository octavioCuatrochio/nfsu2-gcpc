// GC uiCareerCrib (zFe.cpp): the career crib icon menu (UI_CareerCrib.fng) and its CC* icons.
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// CC* icons (IconOption 0x48), vtable / React: EnterGameplay 790CF4 / 4AF630, ChangeParts 790CFC / 4AF680, CarSel 790D04 /
// 4AF6B0, ViewSponsors 790D24 / 4AF6E0, ViewWorldMap 790D44 / 4AF710, SaveMemcard 790D64 / 4AF740, TestTrack 790D6C /
// 4AF780, BackToMain 790D8C / 4AF7D0.
enum class CCKind { EnterGameplay, ChangeParts, CarSel, ViewSponsors, ViewWorldMap, SaveMemcard, TestTrack, BackToMain };
uint32_t CCVTable(CCKind k);
void CCReact(CCKind k, const char *pkg, uint32_t msg);

// PC 0x150, vtable 0x797548: 0 4DF540, 1 40D080, 2 NotificationMessage 4AF3A0, 3 49DFC0, 4 50DA50, 5 Setup 4AF820, 6 50DAB0.
struct uiCareerCrib {
    IconScrollerMenu base;
    uiCareerCrib *Construct(const ScreenConstructorData *data);   // GC 800D2268 / PC 4DF3A0 (ret 4)
    void Destruct();                                              // GC 80117844 / PC 4DF560
    uiCareerCrib *ScalarDeletingDestructor(uint32_t f);           // PC 4DF540 (ret 4)
    void CheckMemory();                                           // GC 800D2308 / PC 4AF360
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800D2378 / PC 4AF3A0
    void Setup();                                                 // GC 800D26AC / PC 4AF820
    void DoStartTuning();                                         // GC 800D2A10 / PC 495AF0
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(uiCareerCrib) == 0x150);
#endif
