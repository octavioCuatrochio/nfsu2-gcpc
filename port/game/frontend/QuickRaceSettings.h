// GC QuickRaceSettings (the global at PC 0x83AAB4): per race mode players / opponents / AI skill / traffic / catch-up
// and track settings, plus the outrun tourney settings. Mode 12 (PC: the LAN / online slot) has its own fields at
// +0x100.. instead of the 0x14-byte record (+0x10 + mode * 0x14); every accessor checks for it. Other modes index
// the record array without a bounds check (as on PC: done with byte offsets here).
#pragma once
#include <cstddef>
#include <cstdint>

struct QRTrackSettings {         // 0xC
    uint16_t mTrack;             // +0x00 track number
    uint16_t mLaps;              // +0x02
    int32_t mDirection;          // +0x04 0 forward, 1 reverse
    uint8_t mbLapKO;             // +0x08 lap knockout
    uint8_t _pad09[3];
};

struct QRModeSettings {          // 0x14
    uint8_t mNumPlayers;         // +0x00
    uint8_t mNumOpponents;       // +0x01
    uint8_t mAISkill;            // +0x02
    uint8_t _unknown03;
    uint8_t mTrafficLevel;       // +0x04
    uint8_t mbCatchUp;           // +0x05
    uint8_t _unknown06[2];
    QRTrackSettings mTrack;      // +0x08
};

struct QuickRaceSettings {       // PC 0x128 at 0x83AAB4
    int32_t mRaceMode;                   // +0x000
    int32_t mSelectedCar[3];             // +0x004 SetSelectedCar(car, player)
    QRModeSettings mModes[12];           // +0x010
    uint8_t mNumPlayers12, mNumOpponents12, mAISkill12, _unknown103;   // +0x100 mode 12
    uint8_t mTrafficLevel12;             // +0x104
    uint8_t _unknown105[3];
    int32_t mPointsToWin;                // +0x108 index into {10, 20, 30, 40, 50, 100, 255} (6: no limit)
    int32_t mTimeLimit;                  // +0x10C index into {0, 2, 5, 8, 10} minutes (0: none)
    int32_t _unknown110;
    uint8_t mOutrunScores[4];            // +0x114
    int8_t mOutrunLeader;                // +0x118 ResetOutrunScores: -1
    uint8_t mbCatchUp12;                 // +0x119
    uint8_t _pad11A[2];
    QRTrackSettings mTrack12;            // +0x11C

    QRModeSettings *Mode();              // +0x10 + mRaceMode * 0x14 (no bounds check)
    const QRModeSettings *Mode() const;

    int32_t GetNumPlayers() const;                   // GC 80130EFC / PC 5026F0
    int32_t GetNumOpponents() const;                 // GC 80130F20 / PC 502710
    int32_t GetAISkill() const;                      // GC 80130F44 / PC 502730
    int32_t GetTrafficLevel() const;                 // GC 80130F68 / PC 502750
    int32_t GetNumPointsToWinOutrun() const { return mPointsToWin; }   // GC 80130F8C / PC 502770
    int32_t GetOutrunTimeLimit() const { return mTimeLimit; }          // GC 80130F94 / PC 502780
    int8_t GetNumPointsToWinOutrunValue() const;     // GC 80130F9C / PC 502790 (byte 0x7992CC[index])
    int8_t GetOutrunTimeLimitValue() const;          // GC 80130FB0 / PC 5027A0 (byte 0x7992D4[index])
    bool HasAPlayerWonTheOutrunTourney() const;      // GC 80130FC4 / PC 5027B0: one leader reached the points
    bool IsOutrunOvertime() const;                   // GC 80131064 / PC 502840: tied leaders reached the points
    void ResetOutrunScores();                        // PC 5028D0 (PC only)
    uint8_t IsCatchUpOn() const;                     // GC 80131108 / PC 5028F0 (the raw byte)
    QRTrackSettings *GetCurrentTrackSettings();      // GC 8013112C / PC 502910
    void SetNumPlayers(int32_t n);                   // GC 80131158 / PC 502930 (ret 4; byte)
    void SetNumOpponents(int32_t n);                 // GC 8013117C / PC 502960
    void SetAISkill(int32_t n);                      // GC 801311A0 / PC 502990
    void SetTrafficLevel(int32_t n);                 // GC 801311C4 / PC 5029C0
    void SetCatchUp(uint8_t on);                     // GC 801311E8 / PC 5029F0 (stores the byte as passed)
    void SetNumPointsToWinOutrun(int32_t i) { mPointsToWin = i; }      // GC 8013120C / PC 502A20
    void SetOutrunTimeLimit(int32_t i) { mTimeLimit = i; }             // GC 80131214 / PC 502A30
    void SetSelectedCar(int32_t car, int32_t player) { mSelectedCar[player] = car; }   // GC 8013121C / PC 502A40 (ret 8)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(QRModeSettings) == 0x14 && offsetof(QRModeSettings, mTrack) == 8);
static_assert(sizeof(QuickRaceSettings) == 0x128 && offsetof(QuickRaceSettings, mModes) == 0x10 &&
              offsetof(QuickRaceSettings, mNumPlayers12) == 0x100 && offsetof(QuickRaceSettings, mPointsToWin) == 0x108 &&
              offsetof(QuickRaceSettings, mOutrunScores) == 0x114 && offsetof(QuickRaceSettings, mbCatchUp12) == 0x119 &&
              offsetof(QuickRaceSettings, mTrack12) == 0x11C);
#endif
