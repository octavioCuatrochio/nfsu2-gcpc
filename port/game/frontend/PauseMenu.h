// GC PauseMenu (zFe.cpp): the in-race / free-roam pause icon menu (UI_Pause.fng) and its pm_* icons.
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// pm_* icons: IconOption (0x48), one vtable each, React = slot 1 (ret 0x14). Dialog ones open UI_InGameDialog.fng
// (DialogInterface 558790) whose "yes" sends a message back to the pause menu, then send 0x96DDA9E6 (dim) to it.
enum class PMKind { ResumeRace, ResumeFreeRoam, ReturnToWorldMap, RestartRace, Replay, SwitchToOptions, QuitToCrib,
                    QuitRaceToFreeRoam, QuitToFrontEnd };
uint32_t PMVTable(PMKind k);                 // PC 7954D0, 7954D8, 7954E0, 7954E8, 7954F0, 795510, 795518, 795520, 795528
void PMReact(PMKind k, const char *pkg, uint32_t msg);   // PC 4C24C0, 4C2510, 4C2560, 4C25D0, 4C26B0, 4C2700, 4C2730, 4C2780, 4C2820
IconOption *PMReplayConstruct(IconOption *o, uint32_t name, uint32_t header, uint32_t desc);   // PC 4C2640 (ret 0xC)
IconOption *PMQuitToFrontEndConstruct(IconOption *o, uint32_t name, uint32_t header, uint32_t desc);   // PC 4C27F0

// PC 0x158, vtable 0x797CB0: 0 scalar dtor 4EB4F0, 1 40D080, 2 NotificationMessage 4C2170, 3 49DFC0 (FilterSoundForMenu),
// 4 RefreshHeader 50DA50, 5 Setup 4D9300, 6 AddOption 50DAB0.
struct PauseMenu {
    IconScrollerMenu base;           // +0x000
    int32_t mOpenTime;               // +0x150 RealTimer at construction: input ignored in that same tick
    int32_t _unknown154;

    PauseMenu *Construct(const ScreenConstructorData *data);   // GC 800FFA7C / PC 4EB410 (ret 4)
    void Destruct();                                           // GC 8011880C / PC 4EB510
    PauseMenu *ScalarDeletingDestructor(uint32_t f);           // PC 4EB4F0 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800FFAF8 / PC 4C2170
    void Setup();                                              // GC 800FFF7C / PC 4D9300
    void SetupExplore();                                       // GC 80100320 / PC 4C2900
    void SetupRace();                                          // GC 801005C4 / PC 4C2BD0
    bool OkToHandleInput() const;                              // PC 4A7BC0 (PC only): not the opening tick, pad plugged
    bool NeedToSeeOptions() const;                             // GC 80100084 / PC 4A7BF0
    void QuitToCrib();                                         // GC 801009DC / PC 4A7C70
    void QuitToWorldMap();                                     // GC 80100A1C / PC 4A7CC0
    void QuitGame();                                           // GC 80100A5C / PC 4A7CF0
    // Inlined on PC (Setup*/NotificationMessage). PC versions: Restart/Replay also hide in online mode 0x200.
    static bool NeedToSeeJumpToGarage();             // GC 801000E8: career (mode 1) && crib reachable (PC byte 0x8635A8)
    static bool NeedToSeeReplay();                   // GC 8010011C: mode not 4 / 8 (PC: / 0x200)
    static bool NeedToSeeReturnToExploreMode();      // GC 8010014C: not (career && race type 7)
    static bool NeedToSeeQuitToMainMenu() { return true; }   // GC 80100180
    static bool NeedToSeeReturnToWorldMap();         // GC 80100188: career && PC byte 0x85BA67
    static bool NeedToSeeResumeRace() { return true; }       // GC 801001BC
    static bool NeedToSeeRestartRace();              // GC 801001C4: race type != 7, mode not 8 (PC: / 0x200)
    static bool NeedToSeeQuitRaceToExploreMode();    // GC 80100200: career && race type != 7 (GC LAN check moot)
    static void RestartRace();                       // GC 80100960: RequestUnPause(3)
    static void QuitRace();                          // GC 80100988: RequestUnPause(4)
    static void QuitRaceToFreeRoam();                // GC 801009B0: RCPostMessage(0xB, 0)
    // GC 80100AD0 EnableVOIP: empty, network (not ported).
};

// GC 801002D0 / PC 4A7C40: free roam 3, race type 7 -> 2, other races 3.
int32_t GetPauseMenuReplayIndex();

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(PauseMenu) == 0x158 && offsetof(PauseMenu, mOpenTime) == 0x150);
#endif
