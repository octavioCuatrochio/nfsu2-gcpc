// GC cFrontendDatabase (zFe2.cpp): the global FE database (PC FEDatabase at 0x83A9D0): two user profiles, game mode
// (PC 0x850078 = +0x156A8), finished race stats, memcard backup. PC inlines most UserProfile members into it.
#pragma once
#include <cstddef>
#include <cstdint>
#include "UserProfile.h"
#include "FinishedRaceStats.h"

struct cFrontendDatabase {
    int32_t mNumPlayers;                 // +0x00000 2 = split screen (GC +0x0)
    uint8_t mbFlag04, mbFlag05;          // +0x00004 (GC +0x4 / +0x8 ints)
    uint8_t _pad06[2];
    int32_t _unknown08;                  // +0x00008 (GC +0x10)
    int32_t _unknown0C;                  // +0x0000C (GC +0x14)
    UserProfile mProfiles[2];            // +0x00010
    int32_t mGameMode;                   // +0x156A8 1 career, 2 quick race, 4 split screen, 8 online, 0x200 LAN
    uint8_t _unknown156AC[4];
    uint8_t mOnlineSettings[0x15994 - 0x156B0];   // +0x156B0 (network, GetOnlineSettings)
    int32_t _unknown15994, _unknown15998;          // +0x15994
    cFinishedRaceStats mFinishedRaceStats;         // +0x1599C
    uint8_t _unknown16690[0x16774 - 0x16690];
    int32_t mExitRaceState;              // +0x16774 1 exit to FE, 2 restart
    uint8_t *mpBackup;                   // +0x16778 AllocBackupDB copy
    uint8_t mStableBackup[0x9BD8];       // +0x1677C FEPlayerCarDBBackup: copy of profile 0's car DB
    int32_t mStableBackupCar;            // +0x20354 and its selected car
    int32_t mReloadPlayer;               // +0x20358 split screen: whose car stable LoadUserProfileFromBuffer reloads

    void Default();                                              // GC 80132174 / PC 5346F0
    const char *CalculateFirstFrontendScreenToSeeAfterQuitRace(int32_t quitType) const;   // GC 801322A0 / PC 502FF0
    static int32_t CalcRaceTypeForHighScores();                  // GC 801322FC (PC: inlined): TrackInfo +0x94 bits
    void NotifyRestartRace();                                    // GC 80132470 / PC 541A10
    void NotifyStartNewRace();                                   // GC 801324B0 / PC 541A50
    void NotifyExitRaceToFrontend(int32_t place);                // GC 80132558 / PC 503040 (ret 4)
    bool IsThereANextRace() const;                               // GC 801325B8 / PC 503080
    bool IsCurrentRaceEventInCareerMode() const;                 // GC 80132604 / PC 5030C0
    static int32_t GetUserProfileSaveSize();                     // GC 801326E4 (PC: inlined, 5B9680() + 0xD254)
    uint8_t *SaveUserProfileToBuffer(uint8_t *buf, int32_t size) { return mProfiles[0].SaveToBuffer(buf, size); }   // GC 80132708
    // GC 8013272C / PC 534770 (ret 8): split screen: only the car stable of mReloadPlayer (PC: size 0x993E).
    void LoadUserProfileFromBuffer(const uint8_t *buf, int32_t size);
    void AllocBackupDB(bool force);                              // GC 8013278C / PC 52A460 (ret 4)
    void DeallocBackupDB();                                      // GC 80132808 / PC 5032C0
    bool IsDirty();                                              // GC 80132848 / PC 52A4C0 (frees the backup)
    // GC 80132370 / PC 551430 (ret 4): results, high scores for player 1 (and 2 in split screen) with the race's
    // track / direction / lap count, career notified in career mode.
    void NotifyTheRaceIsOver(void *race);
    // GC 80132214 / PC 515D00 (ret 0xC): car = player's car DB record (career: *0x863480, else the quick race
    // selection) -> record vslot 1, then vslot 3 (player, ride, tuning) (FEPlayerCarDB::BuildRideForPlayer inlined).
    void BuildCurrentRideForPlayer(int32_t player, void *ride, int32_t tuning);
    void SaveCurrentStable();       // GC 801329AC (PC: no code; its only GC caller is the split-screen menu entry)
    void RestorePreviousStable();   // GC 801329D8 / PC 515D70: car DB back from the copy, re-indexed, selection
    void RestoreFromBackupDB();                                  // GC 80132958 (PC: not found / inlined)
    bool IsOnlineMode() const { return mGameMode == 8; }         // GC 803036E8 (PC: inlined)
    bool IsLANMode() const { return mGameMode == 0x200; }        // GC 80303700 (PC: inlined)
    uint8_t *GetOnlineSettings() { return mOnlineSettings; }     // GC 80303718 (PC: inlined)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(cFrontendDatabase, mProfiles) == 0x10 && offsetof(cFrontendDatabase, mGameMode) == 0x156A8 &&
              offsetof(cFrontendDatabase, mFinishedRaceStats) == 0x1599C && offsetof(cFrontendDatabase, mpBackup) == 0x16778 && offsetof(cFrontendDatabase, mStableBackupCar) == 0x20354 &&
              offsetof(cFrontendDatabase, mReloadPlayer) == 0x20358);
#endif
