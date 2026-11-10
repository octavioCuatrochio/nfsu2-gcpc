// GC UIProfileManager / UIDeleteProfile (zFe.cpp): save / load / delete / create profile icon menus.
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// PM* icons (IconOption 0x48): PMSave 7959D4 / 4C57F0 (memcard op 0x4A51), PMLoad 7959DC / 4C5820 (0xC11, yes 0x3A2BE557,
// no 0x8867412D), PMDelete 7959E4 / 4C5850 (0x831), PMCreateNew 7959EC / 4C5880 (0x861). All EnterMemcard(pkg, pkg, op..).
enum class PMIcon { Save, Load, Delete, CreateNew };
void PMIconReact(PMIcon k, const char *pkg, uint32_t msg);

// PC 0x154, vtable 0x797D14: 0 4EB770, 2 NotificationMessage 4D9800, 3 49CB40, 5 Setup 4C58B0.
struct UIProfileManager {
    IconScrollerMenu base;
    IconOption *mpSave;              // +0x150 PMSave (locked while the profile has no name)

    UIProfileManager *Construct(const ScreenConstructorData *data);   // GC 80104D44 / PC 4EB700 (ret 4)
    void Destruct();                                                  // GC 80118E2C / PC 4EB790
    UIProfileManager *ScalarDeletingDestructor(uint32_t f);           // PC 4EB770 (ret 4)
    void Refresh();                                                   // GC 80104DB0 / PC 4A9360
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 80104E5C / PC 4D9800
    void Setup();                                                     // GC 80104F64 / PC 4C58B0
};

// PC 0x150, vtable 0x797D30: 0 4EB810, 2 NotificationMessage 4A93E0, 3 49CB40, 5 Setup 4C5A40.
struct UIDeleteProfile {
    IconScrollerMenu base;
    UIDeleteProfile *Construct(const ScreenConstructorData *data);    // GC 80105138 / PC 4EB7A0 (ret 4)
    void Destruct();                                                  // GC 80118EFC / PC 4EB830
    UIDeleteProfile *ScalarDeletingDestructor(uint32_t f);            // PC 4EB810 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 801052A4 / PC 4A93E0
    void Setup();                                                     // GC 801051A4 / PC 4C5A40
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(UIProfileManager) == 0x154 && sizeof(UIDeleteProfile) == 0x150);
#endif
