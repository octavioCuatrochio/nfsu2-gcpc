#include "HighScores.h"
#include "bridge/Bridge.h"
#include "FEMath.h"
#include "QRTrackSelect.h"
#include <cstring>

namespace {
template <typename T> T Read(const uint8_t *p, size_t off) { T v; std::memcpy(&v, p + off, sizeof v); return v; }
bool ValidLap(int32_t v) { return v != 0 && v != 0x7FFFFFFF; }
using L = long double;
}  // namespace

long double FinishedRaceScore(const uint8_t *stats)
{
    const float f = Read<float>(stats, 0x108);
    L x = static_cast<L>(FEFtol(f));
    if (!(x <= f)) x -= 1.0L;   // floor (PC 0x784250)
    return x * static_cast<L>(FEFtol(Read<float>(stats, 0x104)));
}

void HighScoresDatabase::CommitHighScoresPreRace(int32_t type, bool splitScreen)
{
    ++mRacesStarted;
    if (type != -1) ++mTypes[type].mStarted;
    if (splitScreen) ++mSplitScreenRaces;
}

TrackHighScore *HighScoresDatabase::FindHighScore(int32_t track, int32_t dir, int32_t key2)
{
    for (TrackHighScore &h : mScores)
        if (h.mTrack == track && h.mDirection == dir && h.mKey2 == key2) return &h;
    return nullptr;
}

int32_t HighScoresDatabase::FindBestLapTime(int32_t track, int32_t dir) const
{
    bool found = false;
    int32_t best = 0x7FFFFFFF;
    for (const TrackHighScore &h : mScores)
        if (h.mTrack == track && h.mDirection == dir && (h.mValue < best || !found)) {
            best = h.mValue;
            found = true;
        }
    if (found) return best;
    const auto *ti = reinterpret_cast<const uint8_t *>(bridge::TrackInfo_GetTrackInfo(static_cast<uint32_t>(track)));
    if (!ti) return best;
    const float secs = Read<float>(ti, dir == 0 ? 0x9C : 0xA0);
    return FEFtol(static_cast<L>(secs) * static_cast<L>(0.001f) * 4000.0L + 0.5L);
}

int32_t HighScoresDatabase::FindBestDriftScore(int32_t track, int32_t dir) const
{
    int32_t best = 0;
    for (const TrackHighScore &h : mScores)
        if (h.mTrack == track && h.mDirection == dir && h.mValue > best) best = h.mValue;
    if (best != 0) return best;
    const auto *ti = reinterpret_cast<const uint8_t *>(bridge::TrackInfo_GetTrackInfo(static_cast<uint32_t>(track)));
    if (!ti) return 0;
    return Read<int32_t>(ti, dir == 0 ? 0xA4 : 0xA8);
}

TrackHighScore *HighScoresDatabase::GetEmptyHighScore()
{
    for (TrackHighScore &h : mScores)
        if (h.mTrack == 0) return &h;
    return nullptr;
}

bool HighScoresDatabase::DidPlayerBreakRecord(int32_t type, const TrackHighScore *rec, const uint8_t *stats)
{
    if (type == 2) return static_cast<L>(rec->mValue) < static_cast<L>(Read<float>(stats, 0x104));
    const int32_t lap = Read<int32_t>(stats, 0x10);
    return ValidLap(lap) && lap < rec->mValue;
}

bool HighScoresDatabase::MaybeAddTrackHighScore(int32_t type, int32_t track, int32_t dir, int32_t key2, const uint8_t *stats)
{
    TrackHighScore *rec = FindHighScore(track, dir, key2);
    if (rec) {
        if (!DidPlayerBreakRecord(type, rec, stats)) return false;
    } else {
        rec = GetEmptyHighScore();
        if (!rec) return false;
        if (type == 2) {
            if (Read<float>(stats, 0x104) < 0.0f) return false;
        } else {
            const int32_t lap = Read<int32_t>(stats, 0x10);
            if (!ValidLap(lap) || static_cast<L>(lap) * static_cast<L>(0.00025f) == 0.0L) return false;
        }
        rec->mTrack = static_cast<int16_t>(track);
        rec->mDirection = static_cast<int8_t>(dir);
        rec->mKey2 = static_cast<int8_t>(key2);
    }
    if (type == 2) {
        rec->mValue = Read<int32_t>(stats, 0x140);
    } else {
        const int32_t lap = Read<int32_t>(stats, 0x10);
        if (ValidLap(lap)) rec->mValue = lap;
    }
    return true;
}

void HighScoresDatabase::CommitHighScoresPostRace(int32_t type, int32_t track, int32_t dir, int32_t key2,
                                                  int32_t storeLast, uint8_t *stats)
{
    const int32_t won = Read<int32_t>(stats, 0) == 1 ? 1 : 0;
    if (type != -1) {
        PerType &t = mTypes[type];
        t.mScore += FEFtol(FinishedRaceScore(stats));
        t.mWon += won;
        if (type == 1 && Read<int32_t>(stats, 8) == 9) ++mType1Kind9;
    }
    if (storeLast) {
        mLastScore = FEFtol(FinishedRaceScore(stats));
        mLastWon = won;
    }
    mTotal = static_cast<float>(static_cast<L>(Read<float>(stats, 0xF8)) + mTotal);
    mRacesWon += won;
    mRacesLost += 1 - won;
    const int32_t added = MaybeAddTrackHighScore(type, track, dir, key2, stats) ? 1 : 0;
    std::memcpy(stats + 0x220, &added, 4);
    bridge::TrackInfo_GetTrackInfo(static_cast<uint32_t>(track));   // result unused (PC too)
    DebugPrint();
}
