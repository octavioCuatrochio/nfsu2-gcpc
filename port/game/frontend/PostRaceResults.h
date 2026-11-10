// GC PostRaceResultsScreen (zFe.cpp; nfsu2-re PostRaceResultsFNGObject): the race results table (UI_PostRace
// results package): positions, names, cars, times (flipping between finish and split times), drift / downhill
// scores, drag / dyno sheet. Online paths (PostRaceMessageCenter updates, online manager 0x89CF48) stay the game's.
#pragma once
#include <cstddef>
#include <cstdint>
#include "MenuScreen.h"
#include "bWare/bList.h"

uint32_t FilterSoundForBrowser(uint32_t msg, uint32_t event);   // GC 80152514 / PC 50D230: events 4..7 -> 0

// PC 0x78 (factory 4FBB60), vtable 0x797A8C: 0 scalar dtor 4F3C80, 1 40D080, 2 NotificationMessage 4E9A20,
// 3 NotifySoundMessage 4A3F00.
struct PostRaceResultsScreen {
    MenuScreen base;                 // +0x00
    bList mResults;                  // +0x4C nodes {next, prev, FinishedRaceStatsEntry *} (0xC, operator new)
    uint8_t mb54, mb55, mb56, _pad57;   // +0x54
    int32_t mDialog;                 // +0x58 dialog handle (dismissDialog)
    uint8_t mb5C, _pad5D[3];         // +0x5C PostRaceMessageCenter (GC +0x58): flag, package, value
    const char *mpMessagePkg;        // +0x60
    int32_t _unknown64;              // +0x64
    uint8_t mbSplits, _pad69[3];     // +0x68 time column: split to the winner instead of the finish time
    uint8_t *mpLeader;               // +0x6C first finisher (FEDatabase finishers[0], PC 0x851044)
    float mFlipTimer;                // +0x70 seconds since the last flip
    float mFlipPeriod;               // +0x74 4.5 (finish times) / 1.75 (splits)

    PostRaceResultsScreen *Construct(const ScreenConstructorData *data);   // GC 800F3648 / PC 4F3BE0 (ret 4)
    void Destruct();                                                      // GC 800F36F8 / PC 4E95A0
    PostRaceResultsScreen *ScalarDeletingDestructor(uint32_t f);          // PC 4F3C80 (ret 4)
    void dismissDialog();                                                 // GC 800F3784 / PC 4A3EE0
    // GC 800F37C4 / PC 4A3F00 (ret 0x14): back / customize silent, else FilterSoundForBrowser
    uint32_t NotifySoundMessage(uint32_t msg, uint32_t event) const;
    void Setup();                                                         // GC 800F381C / PC 4E9640
    void ResultsUpdate();                                                 // GC 800F3A10 / PC 4BF0A0
    void SetRaceSpecificHeader();                                         // GC 800F3B04 / PC 4A3F30
    void OrganizeResults();                                               // GC 800F3C98 / PC 4D7240
    void PrintDownhillResults();                                          // GC 800F3D7C / PC 4BF140
    void PrintResults();                                                  // GC 800F3F2C / PC 4BF290
    void PrintFinishTime(const uint8_t *entry, int32_t row);              // GC 800F4260 / PC 4A4020 (ret 8)
    void PrintSplitTime(const uint8_t *entry, int32_t row);               // GC 800F42D8 / PC 4A4110 (ret 8)
    void PrintDragDynoResults();                                          // GC 800F4390 / PC 4BF4D0
    void SetUnusedEntriesInvisible(int32_t max, int32_t used);            // GC 800F4978 / PC 4A41A0 (ret 8)
    void NotificationMessage(uint32_t msg, void *obj, uint32_t arg, uint32_t pkg);   // GC 800F4AF0 / PC 4E9A20
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(PostRaceResultsScreen) == 0x78 && offsetof(PostRaceResultsScreen, mResults) == 0x4C &&
              offsetof(PostRaceResultsScreen, mDialog) == 0x58 && offsetof(PostRaceResultsScreen, mbSplits) == 0x68 &&
              offsetof(PostRaceResultsScreen, mFlipPeriod) == 0x74);
#endif

// GC PostRaceRewardsScreen (UI_PostRaceReward.fng): career cash / reputation / driver name. PC 0x50, vtable
// 0x7950D8: 0 scalar dtor 4BF950, 2 NotificationMessage 4D7350, 3 sounds 4A4750.
struct PostRaceRewardsScreen {
    MenuScreen base;                 // +0x00
    int32_t _unknown4C;              // +0x4C
    PostRaceRewardsScreen *Construct(const ScreenConstructorData *data);   // GC 800F4D40 / PC 4BF8E0 (ret 4)
    void Destruct();                                                      // GC 801185A4 / PC 4BF970
    PostRaceRewardsScreen *ScalarDeletingDestructor(uint32_t f);          // PC 4BF950 (ret 4)
    void Setup();                                                         // GC 800F4D98 / PC 4A4240
    void NotificationMessage(uint32_t msg, void *obj, uint32_t arg, uint32_t pkg);   // GC 800F4E5C / PC 4D7350
};
