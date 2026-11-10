// GC cPostRaceScreenManager (zFe.cpp): the chain of end-of-race screens (results, tournament results / standings,
// rewards, sponsor popups, post race menu), built once per race end. Instance PC *0x8363DC.
#pragma once
#include <cstddef>
#include <cstdint>

struct cPostRaceScreenManager {   // PC 0x3C, vtable 0x797B7C (one slot: scalar dtor 4F4170)
    void *vtable;                 // +0x00
    // +0x04 online countdown / message center (dtor 4E9180, network: game's)
    float mCountdownTime;         // +0x04 2.0
    int32_t mCountdownSteps;      // +0x08 5
    int32_t _unknown0C, _unknown10;    // +0x0C
    void *mMsgHead, *mMsgTail;    // +0x14 bPNode list (self when empty)
    int32_t _unknown1C_unused;    // +0x1C current results package (PostRaceResultsScreen::Setup / dtor)
    const char *mpTimerName;      // +0x20 "OL_Countdown_Timer"
    uint8_t mb24, _pad25[3];      // +0x24
    int32_t _unknown28;           // +0x28
    int32_t _unknown2C;           // +0x2C = 1
    struct Node { Node *next, *prev; const char *mpName; } *mHead, *mTail;   // +0x30 screens (bPNode, 0xC on PC)
    Node *mpCurrent;              // +0x38

    static cPostRaceScreenManager *&Instance();                 // PC *0x8363DC
    static void DestroyInstance();                               // PC 4A4690 (GC Suspend-like): virtual delete, clear
    cPostRaceScreenManager *Construct();                         // GC 800F676C / PC 4F3EA0: screens, packages kept in memory
    void Destruct();                                             // GC 800F6974 / PC 4EA290
    cPostRaceScreenManager *ScalarDeletingDestructor(uint32_t f);   // PC 4F4170 (ret 4)
    // GC 800F6040 / PC 4BFF80: next screen; at the end: split screen race 8 / online -> race coordinator 9 (more
    // races), else back to the frontend (first screen after the race, coordinator 0x15).
    void ChangeToNextScreen();
    void ChangeToPrevScreen();                                   // GC 800F6124 / PC 4C0020
    static bool NeedToSeePostRaceResults();                      // GC 800F6268 (PC: inlined in the ctor)
    static bool NeedToSeePostRaceTournamentResults();            // GC 800F6304 / PC 4C0050
    static bool NeedToSeePostRaceTournamentStandings();          // GC 800F6420 / PC 4C00C0
    static bool NeedToSeePostRaceRewards();                      // GC 800F6618 (PC: inlined)
    static bool NeedToAskForSponsor();                           // GC 800F652C / PC 4A4610
    static bool NeedToSeePostRaceMenu();                         // GC 800F66B4 (PC: inlined)
    static bool NeedToRemoveTheSponsorScreens();                 // GC 800F6174 / PC 4A45E0
private:
    void Add(const char *name);
};

// GC cPostRaceScreenMgrInterface: owns the manager instance (PC object only carries the ctor / dtor).
namespace cPostRaceScreenMgrInterface {
void *Construct(void *self);   // GC 800F6A90 / PC 4F4190: Instance = new cPostRaceScreenManager
void Destruct();               // GC 800F6ACC / PC 4A4690 (= cPostRaceScreenManager::DestroyInstance)
void Suspend();                // GC 800F6B38 / PC 4A46B0: help bar off (54E6B0), stop the current screen's interaction
void Resume();                 // GC 800F6B64 / PC 4A46E0: restart it (FEngStartInteraction(name, 0)), help bar on
}  // namespace cPostRaceScreenMgrInterface

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(cPostRaceScreenManager, mMsgHead) == 0x14 && offsetof(cPostRaceScreenManager, mpTimerName) == 0x20 &&
              offsetof(cPostRaceScreenManager, mHead) == 0x30 && offsetof(cPostRaceScreenManager, mpCurrent) == 0x38 &&
              sizeof(cPostRaceScreenManager) == 0x3C);
#endif
