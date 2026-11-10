#include "QuickRaceSettings.h"

namespace {
constexpr int32_t kMode12 = 12;
const int8_t kPointsToWin[] = {10, 20, 30, 40, 50, 100, -1, 0};   // PC 0x7992CC (255 read as signed by the tourney test)
const int8_t kTimeLimits[] = {0, 2, 5, 8, 10, 0, 0, 0};           // PC 0x7992D4

// PC 5027B0 / 502840 shared scan: highest score and how many players have it (player 0 always counts).
void ScanScores(const QuickRaceSettings &s, int32_t &best, int32_t &count)
{
    best = s.mOutrunScores[0];
    count = 1;
    for (int i = 1; i < 4; ++i) {
        const int32_t v = s.mOutrunScores[i];
        if (v > best) {
            best = v;
            count = 1;
        } else if (v == best) {
            ++count;
        }
    }
}
}  // namespace

QRModeSettings *QuickRaceSettings::Mode()
{
    return reinterpret_cast<QRModeSettings *>(reinterpret_cast<uint8_t *>(this) + 0x10 + mRaceMode * 0x14);
}
const QRModeSettings *QuickRaceSettings::Mode() const
{
    return reinterpret_cast<const QRModeSettings *>(reinterpret_cast<const uint8_t *>(this) + 0x10 + mRaceMode * 0x14);
}

int32_t QuickRaceSettings::GetNumPlayers() const { return mRaceMode == kMode12 ? mNumPlayers12 : Mode()->mNumPlayers; }
int32_t QuickRaceSettings::GetNumOpponents() const { return mRaceMode == kMode12 ? mNumOpponents12 : Mode()->mNumOpponents; }
int32_t QuickRaceSettings::GetAISkill() const { return mRaceMode == kMode12 ? mAISkill12 : Mode()->mAISkill; }
int32_t QuickRaceSettings::GetTrafficLevel() const { return mRaceMode == kMode12 ? mTrafficLevel12 : Mode()->mTrafficLevel; }
int8_t QuickRaceSettings::GetNumPointsToWinOutrunValue() const { return kPointsToWin[mPointsToWin]; }
int8_t QuickRaceSettings::GetOutrunTimeLimitValue() const { return kTimeLimits[mTimeLimit]; }

bool QuickRaceSettings::HasAPlayerWonTheOutrunTourney() const
{
    if (mPointsToWin == 6) return false;
    int32_t best, count;
    ScanScores(*this, best, count);
    return best >= kPointsToWin[mPointsToWin] && count == 1;
}

bool QuickRaceSettings::IsOutrunOvertime() const
{
    if (mPointsToWin == 6) return false;
    int32_t best, count;
    ScanScores(*this, best, count);
    return best >= kPointsToWin[mPointsToWin] && count > 1;
}

void QuickRaceSettings::ResetOutrunScores()
{
    *reinterpret_cast<uint32_t *>(mOutrunScores) = 0;
    mOutrunLeader = -1;
}

uint8_t QuickRaceSettings::IsCatchUpOn() const { return mRaceMode == kMode12 ? mbCatchUp12 : Mode()->mbCatchUp; }

QRTrackSettings *QuickRaceSettings::GetCurrentTrackSettings() { return mRaceMode == kMode12 ? &mTrack12 : &Mode()->mTrack; }

void QuickRaceSettings::SetNumPlayers(int32_t n) { (mRaceMode == kMode12 ? mNumPlayers12 : Mode()->mNumPlayers) = static_cast<uint8_t>(n); }
void QuickRaceSettings::SetNumOpponents(int32_t n) { (mRaceMode == kMode12 ? mNumOpponents12 : Mode()->mNumOpponents) = static_cast<uint8_t>(n); }
void QuickRaceSettings::SetAISkill(int32_t n) { (mRaceMode == kMode12 ? mAISkill12 : Mode()->mAISkill) = static_cast<uint8_t>(n); }
void QuickRaceSettings::SetTrafficLevel(int32_t n) { (mRaceMode == kMode12 ? mTrafficLevel12 : Mode()->mTrafficLevel) = static_cast<uint8_t>(n); }
void QuickRaceSettings::SetCatchUp(uint8_t on) { (mRaceMode == kMode12 ? mbCatchUp12 : Mode()->mbCatchUp) = on; }
