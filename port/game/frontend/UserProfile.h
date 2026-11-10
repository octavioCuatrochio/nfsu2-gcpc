// GC UserProfile (zFe2.cpp): one player profile (name, options, quick race settings, EA Trax, car stable, high
// scores). PC 0xAB4C; two of them at cFrontendDatabase +0x10 (profile 0 = PC 0x83A9E0). Most members are inlined into
// cFrontendDatabase on PC.
#pragma once
#include <cstddef>
#include <cstdint>
#include "QuickRaceSettings.h"
#include "HighScores.h"
#include "FEPlayerCarDB.h"

struct EATraxEntry {             // 8
    int32_t mIndex;              // +0 track index (table PC 0x7ECD8C, 0x10 per entry)
    uint8_t mPlayIn;             // +4 1 "FE", 2 "IG", 3 "AL", else 0
    uint8_t _pad5[3];
};

struct UserProfile {
    char mName[0x20];                    // +0x000
    uint8_t mbNamed;                     // +0x020
    uint8_t _unknown021[7];
    uint8_t mVideo[0x10];                // +0x028 (GC VideoSettings)
    uint8_t mGameplay[0x10];             // +0x038 (GC GameplaySettings)
    uint8_t mAudio[0x28];                // +0x048 (GC AudioSettings, 0x3C incl. the mode)
    int32_t mAudioMode;                  // +0x070 EAXSound platform mode
    uint8_t mAudioRest[0x10];            // +0x074
    uint8_t mPlayers[0x50];              // +0x084 (GC PlayerSettings x2)
    QuickRaceSettings mQuickRace;        // +0x0D4
    int32_t mCareerSettings;             // +0x250 GC CareerSettings::Default = 1
    int32_t _unknown254;                 // +0x254
    uint8_t _unknown258[0x80];           // +0x258
    EATraxEntry mEATrax[27];             // +0x2D8
    FEPlayerCarDB mCarDB;                // +0x3B0
    int32_t _unknownA0F4;                // +0xA0F4
    uint8_t mbFlagA0F8, mbFlagA0F9, mbFlagA0FA, _padA0FB;   // +0xA0F8 (GC +0x8890 / +0x8894 / +0x8898)
    HighScoresDatabase mHighScores;      // +0xA0FC

    // GC 80131A80 / PC 502F40 (ret 8): a non-empty name is copied (0x20); else the default name (language
    // 0xCF4F80C2 when `primary`, else 0xD87133BD; PC: unbounded copy) and not named.
    void SetProfileName(const char *name, bool primary);
    const char *GetProfileName() const { return mName; }   // GC 80131B48 / PC 502FD0 (folded `mov eax, ecx`)
    bool IsProfileNamed() const { return mbNamed != 0; }    // GC 80131B4C / PC 502FE0
    // GC 80131B54 / PC 515AA0 (ret 8; second arg unused): default name (secondary when `secondary`), flags, quick
    // race / career settings, audio mode, EA Trax play-in table, InitEATRAX, high scores.
    void Default(bool secondary);
    // GC 80131DB4 / PC 52A280 (ret 8): car DB, career state, then the profile blocks (a block that doesn't fit is
    // skipped without advancing), then the online settings (bridge). Returns the last bridge result (PC eax).
    uint8_t *SaveToBuffer(uint8_t *buf, int32_t size);
    // GC 80131F28 / PC 5344D0 (ret 0xC): career Default(true), car DB, career, blocks (always advancing; copied only
    // when they fit), online settings; named. `apply`: audio mode / volumes / EA Trax / joystick configs (PC reads
    // the joystick choices of profile 0, 0x83AA6C, whatever `this` is).
    void LoadFromBuffer(const uint8_t *buf, int32_t size, bool apply);
    // GC 80131CB8 / 80131CF0 / 80131D18 (PC: inlined into cFrontendDatabase)
    void CommitHighScoresPreRace(int32_t type, bool split) { mHighScores.CommitHighScoresPreRace(type, split); }
    void CommitHighScoresPauseQuit() { mHighScores.CommitHighScoresPauseQuit(); }
    void CommitHighScoresPostRace(int32_t type, int32_t track, int32_t dir, int32_t key2, int32_t last, uint8_t *stats)
    {
        mHighScores.CommitHighScoresPostRace(type, track, dir, key2, last, stats);   // GC 80131CF0 (PC: inlined)
    }
    // GC 80131EE4 (PC: inlined into cFrontendDatabase::LoadUserProfileFromBuffer 534770, size 0x993E): car stable only.
    void ReloadCarStable(const uint8_t *buf);
    // GC 80132130 (PC: inlined everywhere as 5B9680() + 0xD254).
    static int32_t GetSaveBufferSize();
};
// Profile blocks after the car DB / career state (offset, size), in save order.
struct UserProfileBlock { uint32_t mOffset, mSize; };
extern const UserProfileBlock kUserProfileBlocks[14];

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(UserProfile, mQuickRace) == 0xD4 && offsetof(UserProfile, mCareerSettings) == 0x250 &&
              offsetof(UserProfile, mEATrax) == 0x2D8 && offsetof(UserProfile, mCarDB) == 0x3B0 &&
              offsetof(UserProfile, mHighScores) == 0xA0FC && sizeof(UserProfile) == 0xAB4C);
#endif
