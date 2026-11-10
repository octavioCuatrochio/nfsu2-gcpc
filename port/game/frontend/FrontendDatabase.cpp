#include "FrontendDatabase.h"
#include <cstring>
#include "bridge/Bridge.h"

void cFrontendDatabase::Default()
{
    mNumPlayers = 1;
    mbFlag04 = 0;
    mbFlag05 = 1;
    mProfiles[0].mCarDB.Default(static_cast<int32_t>(bridge::DefaultPlayerCarType(0)));
    mProfiles[1].mCarDB.Default(static_cast<int32_t>(bridge::DefaultPlayerCarType(1)));
    mProfiles[0].Default(false);
    mProfiles[1].Default(true);
    _unknown15994 = 0;
    _unknown15998 = 0;
    mGameMode = 2;
    _unknown08 = 0;
    _unknown0C = 1;
    bridge::PlayerCareer_Default(true);
}

const char *cFrontendDatabase::CalculateFirstFrontendScreenToSeeAfterQuitRace(int32_t quitType) const
{
    if (quitType != 0) return "UI_Main.fng";
    switch (mGameMode) {
    case 4: return "UI_QRModeSelect.fng";
    case 8:
    case 0x200: return bridge::OnlineLobbyScreenName();   // PC 0x78F320 (network screen, name kept)
    default: return "UI_Main.fng";
    }
}

int32_t cFrontendDatabase::CalcRaceTypeForHighScores()
{
    const auto *ti = reinterpret_cast<const uint8_t *>(bridge::TrackInfo_GetTrackInfo(static_cast<uint32_t>(*bridge::GameInt(0x89E7A0))));
    if (!ti) return -1;
    uint32_t f;
    std::memcpy(&f, ti + 0x94, 4);
    if (f & 1) return 3;
    if (f & 2) return 1;
    if (f & 4) return 0;
    if (f & 8) return 2;
    return -1;
}

void cFrontendDatabase::NotifyRestartRace()
{
    mExitRaceState = 2;
    if (IsCurrentRaceEventInCareerMode()) bridge::PlayerCareer_NotifyRestartCurrentRace();
}

void cFrontendDatabase::NotifyStartNewRace()
{
    const bool split = mNumPlayers == 2;
    const int32_t type = CalcRaceTypeForHighScores();
    mProfiles[0].CommitHighScoresPreRace(type, split);
    if (split) mProfiles[1].CommitHighScoresPreRace(type, split);
    if (mGameMode == 1) {
        for (uint32_t a = 0x863444; a <= 0x863450; a += 4) *bridge::GameInt(a) = 0;
        bridge::PlayerCareer_NotifyBeginNewRace();
    }
}

void cFrontendDatabase::NotifyExitRaceToFrontend(int32_t place)
{
    mExitRaceState = 1;
    if (place != 0) return;
    const bool split = mGameMode == 4;
    mProfiles[0].CommitHighScoresPauseQuit();
    if (split) mProfiles[1].CommitHighScoresPauseQuit();
}

bool cFrontendDatabase::IsThereANextRace() const { return mGameMode == 1 && bridge::EventInProgress_IsThereANextRace(); }

bool cFrontendDatabase::IsCurrentRaceEventInCareerMode() const { return mGameMode == 1 && *bridge::GameByte(0x85BA65) != 0; }

int32_t cFrontendDatabase::GetUserProfileSaveSize() { return UserProfile::GetSaveBufferSize(); }

void cFrontendDatabase::LoadUserProfileFromBuffer(const uint8_t *buf, int32_t size)
{
    if (mGameMode == 4) {
        mProfiles[mReloadPlayer].ReloadCarStable(buf);
        return;
    }
    mProfiles[0].LoadFromBuffer(buf, size, true);
}

void cFrontendDatabase::AllocBackupDB(bool force)
{
    if (mpBackup || !(force || mProfiles[0].mbFlagA0F8)) return;
    mpBackup = static_cast<uint8_t *>(bridge::bMalloc(static_cast<uint32_t>(GetUserProfileSaveSize()), 8));
    mProfiles[0].SaveToBuffer(mpBackup, GetUserProfileSaveSize());
}

void cFrontendDatabase::DeallocBackupDB()
{
    if (!mpBackup) return;
    bridge::bWareFree(mpBackup);
    mpBackup = nullptr;
}

bool cFrontendDatabase::IsDirty()
{
    if (!mpBackup) return false;
    auto *buf = static_cast<uint8_t *>(bridge::bMalloc(static_cast<uint32_t>(GetUserProfileSaveSize()), 8));
    bool dirty = true;
    mProfiles[0].SaveToBuffer(buf, GetUserProfileSaveSize());
    if (*bridge::GameByte(0x838467)) {   // UseSlowMemCmpDebug: 8-byte chunks
        for (int32_t i = 0; i < GetUserProfileSaveSize(); i += 8) {
            int32_t n = GetUserProfileSaveSize() - i;
            if (n > 8) n = 8;
            if (std::memcmp(buf + i, mpBackup + i, static_cast<size_t>(n)) != 0) {
                dirty = true;
                break;
            }
            dirty = false;
        }
    } else if (std::memcmp(buf, mpBackup, static_cast<size_t>(GetUserProfileSaveSize())) == 0) {
        dirty = false;
    }
    bridge::bWareFree(buf);
    DeallocBackupDB();
    return dirty;
}

void cFrontendDatabase::RestoreFromBackupDB()
{
    if (!mpBackup) return;
    LoadUserProfileFromBuffer(mpBackup, GetUserProfileSaveSize());
    DeallocBackupDB();
}

void cFrontendDatabase::NotifyTheRaceIsOver(void *race)
{
    mFinishedRaceStats.Construct(race);
    const bool split = mGameMode == 4;
    const int32_t type = CalcRaceTypeForHighScores();
    const int32_t laps = *bridge::GameInt(0x89E7BC), track = *bridge::GameInt(0x89E7A0), dir = *bridge::GameInt(0x89E7A4);
    uint8_t *st = mFinishedRaceStats.GetStatsForDriver(bridge::RaceParameters_GetDriverNumber(0));
    mProfiles[0].CommitHighScoresPostRace(type, track, dir, laps, split, st);
    if (split) {
        st = mFinishedRaceStats.GetStatsForDriver(bridge::RaceParameters_GetDriverNumber(1));
        mProfiles[1].CommitHighScoresPostRace(type, track, dir, laps, split, st);
    }
    if (mGameMode == 1) bridge::PlayerCareer_NotifyCurrentRaceFinished(race);
}

void cFrontendDatabase::BuildCurrentRideForPlayer(int32_t player, void *ride, int32_t tuning)
{
    FEPlayerCarDB *db = player >= 0 && player < 2 ? &mProfiles[player].mCarDB : nullptr;
    const int32_t id = mGameMode == 1 ? *bridge::GameInt(0x863480) : mProfiles[0].mQuickRace.mSelectedCar[player];
    void *car = db->GetCarRecordByHandle(static_cast<uint32_t>(id));   // PC: no null check either
    if (!car) return;
    bridge::CarRecord_GetType(car);   // vslot 1, result unused
    bridge::CarRecord_BuildRide(car, player, ride, tuning);
}

void cFrontendDatabase::SaveCurrentStable()
{
    std::memcpy(mStableBackup, &mProfiles[0].mCarDB, sizeof mStableBackup);
    mStableBackupCar = mProfiles[0].mQuickRace.mSelectedCar[0];
}

void cFrontendDatabase::RestorePreviousStable()
{
    std::memcpy(&mProfiles[0].mCarDB, mStableBackup, sizeof mStableBackup);
    mProfiles[0].mCarDB.ResetPointers();
    mProfiles[0].mQuickRace.mSelectedCar[0] = mStableBackupCar;
}
