// GC UIOptionsMain / UIPauseOptionsMain (zFe.cpp): the options icon menus (front end UI_OptionsMain.fng, in race
// UI_PauseOptionsMain.fng) and their icons OMAudio..OMCredits (accept sets the options page *0x83AA04).
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// OM* icons: IconOption (0x48) with one vtable each (PC 7918B8 + 8 * i, slot 1 React 49A620 + 0x20 * i):
// accept (0x0C407210) sets the page. GC OMAudio 0, OMVideo 1, OMGameplay 2, OMPlayer 3, OMController 4, OMEATrax 5,
// OMInline 7, OMCredits 8.
enum class OMKind { Audio, Video, Gameplay, Player, Controller, EATrax, Inline, Credits };
int32_t OMPage(OMKind k);
uint32_t OMVTable(OMKind k);
void OMReact(OMKind k, uint32_t msg);   // PC 49A620.. (ret 0x14)

// PC 0x150, vtable 0x797794: 0 scalar dtor 4E3680, 1 40D080, 2 NotificationMessage 4B6080, 3 NotifySoundMessage
// 49CB40, 4 RefreshHeader 50DA50, 5 Setup 4B6230, 6 AddOption 50DAB0.
struct UIOptionsMain {
    IconScrollerMenu base;           // +0x000

    UIOptionsMain *Construct(const ScreenConstructorData *data);   // GC 800ECDB8 / PC 4E3560 (ret 4)
    void Destruct();                                               // GC 80117FF0 / PC 4E36A0
    UIOptionsMain *ScalarDeletingDestructor(uint32_t f);           // PC 4E3680 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800ECEC4 / PC 4B6080
    // GC 80118094 / PC 49CB40 (ret 0x14): FilterSoundForHorizontalMenu(msg, event, true)
    static uint32_t NotifySoundMessage(uint32_t msg, uint32_t event);
    void Setup();                                                  // GC 800ED078 / PC 4B6230
};

// PC 0x154, vtable 0x797CCC: 0 scalar dtor 4EB620, 1 40D080, 2 NotificationMessage 4A7D60, 3 49DFC0
// (FilterSoundForMenu, UIWheelOptionsMenu's), 4 50DA50, 5 Setup 4C3000, 6 50DAB0.
struct UIPauseOptionsMain {
    IconScrollerMenu base;           // +0x000
    uint8_t mbExitToRace;            // +0x150 next 0xC98356BA pops the screen and unpauses (after "resume race")
    uint8_t _pad151[3];

    UIPauseOptionsMain *Construct(const ScreenConstructorData *data);   // GC 80100B0C / PC 4EB580 (ret 4)
    void Destruct();                                                    // GC 801188D8 / PC 4EB640
    UIPauseOptionsMain *ScalarDeletingDestructor(uint32_t f);           // PC 4EB620 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 80100B58 / PC 4A7D60
    void Setup();                                                       // GC 80100D94 / PC 4C3000
};

// GC 800ECCE8 / PC 4B6010 (__cdecl): leaving the options: with autosave on and changed options (OptionsDB dirty or
// autosave flag 0x844AD8 != 0x836360) -> memory card save (EnterMemcard op 0x2042) back to *0x83635C, else switch
// straight there; then drop the options backup.
void ExitFEOptionsSystem(const char *pkg);
// GC 80152488 / PC 50D1A0 (__cdecl): up / down (6, 7) silenced; back (0xA) only when `back`; else the event.
uint32_t FilterSoundForHorizontalMenu(uint32_t msg, uint32_t event, bool back);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(UIOptionsMain) == 0x150 && sizeof(UIPauseOptionsMain) == 0x154);
#endif
