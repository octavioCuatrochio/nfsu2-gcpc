// GC HighScoresDatabase (zFe2.cpp): per-profile race statistics and track records. PC: 0xA50 bytes at
// UserProfile +0xA0FC (GC +0x889C); the UserProfile forwarders and Default are inlined into cFrontendDatabase.
#pragma once
#include <cstddef>
#include <cstdint>

struct TrackHighScore {          // 8
    int16_t mTrack;              // +0 0 = empty slot
    int8_t mKey2;                // +2 FindHighScore third key
    int8_t mDirection;           // +3 FindHighScore second key / FindBest* direction
    int32_t mValue;              // +4 lap time (ticks) or drift score
};

// FinishedRaceStatsEntry fields used here (PC offsets): +0x00 place (1 = won), +0x08 race subtype, +0x10 best lap
// (ticks; 0 / 0x7FFFFFFF = none), +0xF8 float distance?, +0x104 float, +0x108 float, +0x140 drift score, +0x220 out.
struct HighScoresDatabase {
    TrackHighScore mScores[0x140];   // +0x000
    float mTotal;                    // +0xA00 += stats +0xF8 every race
    int32_t mRacesStarted;           // +0xA04
    int32_t mRacesWon;               // +0xA08
    int32_t mRacesLost;              // +0xA0C also += 1 per pause-quit
    struct PerType { int32_t mStarted, mWon, mScore; } mTypes[3];   // +0xA10 (type -1 = none)
    uint8_t _unknownA34[0xC];
    int32_t mSplitScreenRaces;       // +0xA40 CommitHighScoresPreRace(.., true)
    int32_t mLastWon;                // +0xA44
    int32_t mLastScore;              // +0xA48
    int32_t mType1Kind9;             // +0xA4C races of type 1 with stats +8 == 9

    void Default() { *this = HighScoresDatabase{}; }   // GC 801318D4 (PC: inlined memset 0xA50)
    // GC 8013122C (PC: inlined into cFrontendDatabase::NotifyStartNewRace 541A50)
    void CommitHighScoresPreRace(int32_t type, bool splitScreen);
    void CommitHighScoresPauseQuit() { ++mRacesLost; }  // GC 8013126C (PC: inlined)
    // GC 8013127C / PC 502A50 (ret 0xC): slot with this track / direction / key2, else null.
    TrackHighScore *FindHighScore(int32_t track, int32_t dir, int32_t key2);
    // GC 801312C0 / PC 502AA0 (ret 8): lowest stored lap time; none: TrackInfo default (+0x9C fwd / +0xA0 rev) in
    // seconds * 0.001 * 4000 + 0.5 (_ftol), or 0x7FFFFFFF without TrackInfo.
    int32_t FindBestLapTime(int32_t track, int32_t dir) const;
    // GC 8013139C / PC 502C50 (ret 8): highest drift score > 0; none: TrackInfo +0xA4 / +0xA8.
    int32_t FindBestDriftScore(int32_t track, int32_t dir) const;
    TrackHighScore *GetEmptyHighScore();               // GC 80131434 (PC: inlined into MaybeAdd)
    // GC 80131464 / PC 502DA0 (ret 0xC, `this` unused): type 2: record < stats +0x104 (float); else the valid best
    // lap is below the record.
    static bool DidPlayerBreakRecord(int32_t type, const TrackHighScore *rec, const uint8_t *stats);
    // GC 801314FC / PC 502DF0 (ret 0x14): update or add the record; true if it changed.
    bool MaybeAddTrackHighScore(int32_t type, int32_t track, int32_t dir, int32_t key2, const uint8_t *stats);
    // GC 80131698 / PC 515990 (ret 0x18): per-type / last-race counters, totals, record; stats +0x220 = result.
    void CommitHighScoresPostRace(int32_t type, int32_t track, int32_t dir, int32_t key2, int32_t storeLast,
                                  uint8_t *stats);
    // GC 801318FC / PC 502F00: formats every record's time into a local buffer (Timer 577240) and discards it.
    void DebugPrint() const {}
};

// PC 4FE170 (__thiscall on FinishedRaceStatsEntry): floor(+0x108) * _ftol(+0x104), in x87 precision.
long double FinishedRaceScore(const uint8_t *stats);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(HighScoresDatabase) == 0xA50 && offsetof(HighScoresDatabase, mTypes) == 0xA10 &&
              offsetof(HighScoresDatabase, mSplitScreenRaces) == 0xA40 && offsetof(HighScoresDatabase, mType1Kind9) == 0xA4C);
#endif
