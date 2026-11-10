#include "QuickRaceSettings.h"
#include "bridge/Bridge.h"
#include <cstring>

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

void QuickRaceSettings::Default()
{
    mRaceMode = 0;
    auto *rec = reinterpret_cast<uint8_t *>(mModes);   // 13 records: modes 0..11 + the one at +0x100
    for (int i = 0; i < 13; ++i, rec += sizeof(QRModeSettings)) {   // SingleRaceSettings::Default (inlined)
        rec[0] = 1;   // players
        rec[1] = 3;   // opponents
        rec[2] = 1;   // AI skill
        rec[3] = 1;
        rec[4] = 1;   // traffic
        rec[5] = 1;   // catch-up
        rec[6] = 0;
    }
    mModes[1].mTrack.mLaps = 1;
    mModes[2].mTrack.mLaps = 1;
    mModes[5].mNumOpponents = 1;
    mModes[0].mTrack.mTrack = 0xFB5;
    mModes[3].mTrack.mLaps = 3;
    mModes[3].mTrafficLevel = 2;
    mModes[9].mTrack.mLaps = 0;
    mModes[9].mNumOpponents = 0;
    mModes[5].mAISkill = 0;
    mModes[10].mAISkill = 0;
    mModes[10].mTrack.mTrack = 0x11F9;
    mModes[10].mTrafficLevel = 0;
    mModes[10].mTrack.mLaps = 10;
    mModes[4].mNumOpponents = 5;
    mPointsToWin = 2;
    mTimeLimit = 0;
    for (int32_t p = 0; p < 2; ++p)   // player 2 uses its own default type (PC also does the player-1 search first)
        mSelectedCar[p] = static_cast<int32_t>(bridge::StockCarIdByType(p, bridge::DefaultPlayerCarType(p)));
    TourneyDefault();
}

void QuickRaceSettings::TourneyDefault() { TournamentSettingsDefault(reinterpret_cast<uint8_t *>(this) + 0x100); }

void TournamentSettingsDefault(void *ts)
{
    auto *t = static_cast<uint8_t *>(ts);
    t[1] = 3;
    t[5] = 3;
    t[0] = 1;
    t[2] = 1;
    t[3] = 1;
    t[4] = 1;
    t[0x19] = 1;
    t[0x1A] = 0;
    std::memset(t + 0x14, 0, 4);
    for (auto *tr = reinterpret_cast<QRTrackSettings *>(t + 0x1C); tr != reinterpret_cast<QRTrackSettings *>(t + 0x7C); ++tr) {
        tr->mTrack = 0;
        tr->mDirection = 0;
        tr->mLaps = 3;
        tr->mbLapKO = 0;
        tr->_pad09[0] = 0;
    }
}
