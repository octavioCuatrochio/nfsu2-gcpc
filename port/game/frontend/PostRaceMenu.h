// GC PostRaceMenu (zFe.cpp): the icon menu after a race (UI_PostRace.fng) and its pr_* icons.
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// pr_* icons: IconOption (0x48), one vtable each (slot 0 SponsorOption dtor 4AEB70), React = slot 1 (ret 0x14),
// all react to 0x0C407210 only. Confirm dialogs: UI_InGameDialog.fng (like PauseMenu's) + 0x96DDA9E6 to the menu.
enum class PRKind {
    NextEvent,        // 795014: React 4BEC90  PlayerCareerState 54CAE0, RaceCoordinator message 0xA
    QuitToFreeRoam,   // 794FF8: React 4BEC20  503080 (cFrontendDatabase) ? confirm (yes 0x451E768E) : 0x3411395C
    WorldMap,         // 79503C: React 4BEDC0  confirm yes 0x33C42E03, text 0x33C42E03
    RestartRace,      // 79501C: React 4BECC0  confirm yes 0xE1A57D51, text 0xAA50AA02
    Stats,            // 795024: React 4BED10  byte 0x7F45A1 = 1, UI_PostRaceStats.fng
    Replay,           // 795034: React 4BED90  UI_ReplayControl.fng, SetReplayBackScreen(pkg)
    QuitToFrontEnd,   // 79502C: React 4BED40  confirm yes 0x30ED2368, text 0xE15B558A
};
uint32_t PRVTable(PRKind k);
void PRReact(PRKind k, const char *pkg, uint32_t msg);

// PC 0x154, vtable 0x797A70: 0 scalar dtor 4E9520, 1 40D080, 2 NotificationMessage 4F39A0, 3 49DFC0, 4 RefreshHeader
// 50DA50, 5 Setup 4D7160, 6 AddOption 50DAB0.
struct PostRaceMenu {
    IconScrollerMenu base;           // +0x000
    uint8_t mbToFreeRoam;            // +0x150 set when 0x3411395C starts the autosave before free roam
    uint8_t _pad151[3];

    PostRaceMenu *Construct(const ScreenConstructorData *data);   // GC 800F2CF0 / PC 4E94C0 (ret 4)
    void Destruct();                                              // GC 801184C0 / PC 4E9540
    PostRaceMenu *ScalarDeletingDestructor(uint32_t f);           // PC 4E9520 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800F2D3C / PC 4F39A0
    void Setup();                                                 // GC 800F3190 / PC 4D7160
    void SetupRace();                                             // GC 800F3250 / PC 4BEE10
    static void QuitToWorldMap();                                 // GC 800F3080 / PC 4A3DC0 (__cdecl-like, no this)
    void QuitRaceToFreeRoam();                                    // GC 800F30C4 / PC 4A3DF0
    void QuitRaceToFreeRoamRaceDone();                            // GC 800F30FC / PC 4A3E30
    void QuitGame();                                              // GC 800F3590 / PC 4A3E80
};
// PC 4A3D50 (__cdecl, PC only): control mask = the ports of the 1 (2 in split screen) menu players.
void PostRace_SetControlMask(const char *pkg);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(PostRaceMenu) == 0x154 && offsetof(PostRaceMenu, mbToFreeRoam) == 0x150);
#endif
