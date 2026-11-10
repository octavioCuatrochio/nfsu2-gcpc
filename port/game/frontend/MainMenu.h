// GC UIMain (zFe.cpp): the main menu icon scroller (UI_Main.fng) and its MM* icons.
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// MM* icons: IconOption (0x48), vtables 790C30.. (slot 1 React). PC: MMQuickRace 790C30 / 4FC050, MMOnline 790C38 /
// 4AECB0 (PC: picks the pad and sends 0xD05FC3A3; GC empty), MMOptions 790C40 / 4AECE0, MMCarSelect 790C48 / 4AED30,
// MMCareer 790C50 / 4AED80, MMSaveLoad 790C58 / 4AEDB0, MMSplitScreen 790C60 / 4AEDE0 (PC: LAN server select).
enum class MMKind { QuickRace, Online, Options, CarSelect, Career, SaveLoad, SplitScreen };
uint32_t MMVTable(MMKind k);
void MMReact(MMKind k, const char *pkg, uint32_t msg, uint32_t a3);
// GC 80152D8C / PC 50D2A0 (__cdecl): pad bit mask -> port (bit 0..3 -> 0..3, else -1)
int32_t FEngMapJoyParamToJoyport(uint32_t param);
// PC 50CEE0 (__cdecl): bit `port` of the active-pad mask 0x838524 set / cleared
void FEngSetJoyPortActive(int32_t port, bool on);

// PC 0x150, vtable 0x797510: 0 scalar dtor 4DF340, 1 40D080, 2 NotificationMessage 4F98B0, 3 49CB40
// (FilterSoundForHorizontalMenu(.., true)), 4 50DA50, 5 Setup 4AEE30, 6 50DAB0.
struct UIMain {
    IconScrollerMenu base;

    UIMain *Construct(const ScreenConstructorData *data);   // GC 800D1A14 / PC 4DF140 (ret 4)
    void Destruct();                                        // GC 801176A4 / PC 4DF360
    UIMain *ScalarDeletingDestructor(uint32_t f);           // PC 4DF340 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800D1B10 / PC 4F98B0
    void Setup();                                           // GC 800D1C2C / PC 4AEE30
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(UIMain) == 0x150);
#endif
