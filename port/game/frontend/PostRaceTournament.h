// GC cPostRaceTournamentResults (zFe.cpp): split-screen outrun / career multi-race / online getaway standings
// (positions, names, cars, times, points). Online parts (lobby callbacks, countdown, dialogs) stay the game's.
#pragma once
#include <cstddef>
#include <cstdint>
#include "MenuScreen.h"
#include "bWare/bList.h"

// PC 0x8C, vtable 0x797B80: 0 scalar dtor 4F4350, 2 NotificationMessage 4F4370, 3 sounds 4A4750. Base of the
// standings screen (ctor 4F4610, +0x8C..).
struct cPostRaceTournamentResults {
    MenuScreen base;                 // +0x00
    bList mResults;                  // +0x4C nodes 0x18 {links, entry +8, driver +0xC, points +0x10, total +0x14}
    int32_t mDialog1, mDialog2;      // +0x54 online dialogs
    uint8_t mb5C, _pad5D[3];         // +0x5C
    int32_t _unknown60;              // +0x60
    uint8_t mb64, _pad65[3];         // +0x64
    const char *mpPkg;               // +0x68
    int32_t _unknown6C;              // +0x6C
    // +0x70 online countdown / lobby helper (init 4CFD80, messages 4EFE40): +0x70 dialog handle
    int32_t mOnlineDialog;           // +0x70
    uint8_t mb74, _pad75[3];         // +0x74
    int32_t _unknown78;              // +0x78
    uint8_t mb7C, mb7D, mb7E, mb7F;  // +0x7C
    uint8_t mb80, _pad81[3];         // +0x80
    int32_t _unknown84, _unknown88;  // +0x84

    cPostRaceTournamentResults *Construct(const ScreenConstructorData *data);   // GC 800F6C08 / PC 4F41F0 (ret 4)
    void Destruct();                                                           // GC 800F6C64 / PC 4EA360
    cPostRaceTournamentResults *ScalarDeletingDestructor(uint32_t f);          // PC 4F4350 (ret 4)
    void NotificationMessage(uint32_t msg, void *obj, uint32_t arg, uint32_t pkg);   // GC 800F6CF0 / PC 4F4370
    // GC 800F6DDC / PC 4A4770 (ret 8): rows 1..used shown, used+1..max+1 hidden (PC: one past max)
    void SetUnusedEntriesInvisible(int32_t max, int32_t used);
    void Setup();                                      // GC 800F6FD0 / PC 4EA490
    void OrganizeRaceResults();                        // GC 800F7118 / PC 4C0110
    void PrintRaceResults();                           // GC 800F7270 / PC 4D7400
    void SetRaceSpecificHeader();                      // GC 800F745C / PC 4C0270
    void PrintRacePosition(int32_t row, const uint8_t *entry);   // GC 800F74E8 / PC 4C0300 (ret 8)
    void PrintDriverName(int32_t row, const uint8_t *entry);     // GC 800F7614 / PC 4C03A0 (ret 8)
    void PrintCarName(int32_t row, const uint8_t *entry);        // GC 800F76E0 (PC: inlined)
    void PrintRaceTime(int32_t row, int32_t ticks);              // GC 800F7764 (PC: inlined)
    void PrintOutrunLeaderTime(int32_t row, const uint8_t *entry);   // GC 800F77DC / PC 4A48D0 (ret 8): entry +0x14
    void PrintRacePoints(int32_t row, int32_t points);           // GC 800F7854 (PC: inlined)
};
int32_t SortTournamentResults(bNode *a, bNode *b);   // PC 4A4710: b's position >= a's
int32_t SortTournamentStandings(bNode *a, bNode *b);  // PC 4A4730: b's total <= a's

// GC cPostRaceTournamentStandings (PC 0x94, vtable 0x798390: 0 scalar dtor 4F4670, 2 NotificationMessage 4EA6A0).
struct cPostRaceTournamentStandings {
    cPostRaceTournamentResults base;  // +0x00
    bList mStandings;                 // +0x8C nodes as the results (+0x10 rank, +0x14 points total)
    cPostRaceTournamentStandings *Construct(const ScreenConstructorData *data);   // GC 800F78E8 / PC 4F4610 (ret 4)
    void Destruct();                                                             // GC 80118634 / PC 4F4690
    cPostRaceTournamentStandings *ScalarDeletingDestructor(uint32_t f);          // PC 4F4670 (ret 4)
    void NotificationMessage(uint32_t msg, void *obj, uint32_t arg, uint32_t pkg);   // GC 800F80D8 / PC 4EA6A0
    void Setup();                                    // GC 800F7938 / PC 4EA570
    void OrganizeStandings();                        // GC 800F7BA8 / PC 4C07A0
    void PrintRaceStandings();                       // GC 800F7D80 / PC 4D77A0
    // GC 800F7F10 / PC 4C0A00 (ret 0xC): highlight + the 1-based rank (rank < 0: the row number)
    void PrintOrdinalPlacing(int32_t row, const uint8_t *entry, int32_t rank);
    void PrintPointsTotal(int32_t row, int32_t points);   // GC 800F807C (PC: inlined)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(cPostRaceTournamentResults) == 0x8C && offsetof(cPostRaceTournamentResults, mpPkg) == 0x68 &&
              offsetof(cPostRaceTournamentResults, mOnlineDialog) == 0x70);
static_assert(sizeof(cPostRaceTournamentStandings) == 0x94);
#endif
