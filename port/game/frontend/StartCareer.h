// GC UIStartCareer (zFe.cpp): career start icon menu (UI_StartCareer.fng): resume / new / load.
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// icons (IconOption 0x48): SCNewCareerMemcard 790CB8 / 4AF090 (EnterMemcard(pkg, pkg, 0x21961)), SCStart 790CC0 /
// 4AF0C0 (StartCareerHelper(a3)), SCLoad 790CC8 / 4AF0E0 (EnterMemcard(pkg, pkg, 0xC11, 0, 0, 0x461A18EE, 0xF35D144E)).
enum class SCKind { NewCareerMemcard, Start, Load };
void SCReact(SCKind k, const char *pkg, uint32_t msg, uint32_t a3);

// PC 0x154, vtable 0x79752C: 0 4DF370, 1 40D080, 2 NotificationMessage 4AF2C0, 3 49CB40, 4 50DA50, 5 Setup 4AF110,
// 6 50DAB0. PC inlines the ctor into the screen factory 4ED710 (new 0x154, IconScrollerMenu ctor, vtable, Setup).
struct UIStartCareer {
    IconScrollerMenu base;
    IconOption *mpStartIcon;         // +0x150 the 790CC0 icon (locked by 0x251BF83E when the profile has no name)

    static UIStartCareer *Create(const ScreenConstructorData *data);   // PC 4ED710 (__cdecl)
    void Destruct();                                                   // GC 80117774 / PC 4DF390
    UIStartCareer *ScalarDeletingDestructor(uint32_t f);               // PC 4DF370 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800D2150 / PC 4AF2C0
    void Setup();                                                      // GC 800D1F7C / PC 4AF110
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(UIStartCareer) == 0x154);
#endif
