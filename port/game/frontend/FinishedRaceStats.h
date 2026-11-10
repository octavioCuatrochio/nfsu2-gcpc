// GC cFinishedRaceStats (zFe2.cpp): per-driver race results (PC 0xCF4; the FE database copy at 0x85036C =
// cFrontendDatabase +0x1599C). FinishedRaceStatsEntry (0x224) itself is still the game's (Construct 515DB0).
#pragma once
#include <cstddef>
#include <cstdint>
#include "bWare/bList.h"

// Entry fields used here (PC): +0x04 driver number, +0x1C + lap * 4 lap time (ticks), +0x70 + lap * 4 total time (lower = ahead),
// +0xC8 + lap * 4 position on that lap, +0xF4 laps led, +0x144 + lap * 4 drift lap score, +0x198 + lap * 4 drift
// total, +0x1E8 + lap * 4 drift position.
struct cFinishedRaceStats {
    uint8_t mEntries[6][0x224];          // +0x000
    uint8_t *mFinishers[6];              // +0xCD8 entries in finishing order
    int32_t mCount;                      // +0xCF0

    // GC 8013303C / PC 54CCA0 (ret 4): an entry per racer (race +0x24 count, cars at +0xBD0), lap positions, then
    // the finishing order of the FE database's entries (sorted by drift points in drift races, re-sorted after
    // every insert like the original).
    void Construct(void *race);
    uint8_t *GetStatsForDriver(int32_t driver);   // GC 80133214 / PC 503320 (ret 4)
    uint8_t *GetFinisher(int32_t place) const;    // GC 80133254 (PC: inlined): 1-based, null past the count
    // GC 8013327C / PC 541B40 (static; works on the FE database's entries): per lap (1..min(laps, 21)) sort by time
    // and store positions; drift races also by drift score.
    static void CalculatePositionsForEachLap();
};

// bList comparators (PC __cdecl on bPNode, data at +8; nonzero = a first). Lap = PC 0x83846C (CurLapForSorting...).
int32_t SortRacingCarByTime(bNode *a, bNode *b);        // GC 80133530 / PC 503360
int32_t SortRacingCarByDrift(bNode *a, bNode *b);       // PC 5033F0 (GC: SortRacingCarByScore 801335F4)
int32_t SortResultsByDriftPoints(bNode *a, bNode *b);   // GC 80133004 / PC 5032F0: by total at the race's lap count

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(cFinishedRaceStats, mFinishers) == 0xCD8 && offsetof(cFinishedRaceStats, mCount) == 0xCF0);
#endif
