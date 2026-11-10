#include "UserProfile.h"
#include <cstring>
#include "bWare/bString.h"
#include "bridge/Bridge.h"
#include "FELocalize.h"

namespace {
void DefaultName(UserProfile &p, bool primary)
{
    std::memset(p.mName, 0, sizeof p.mName);
    const char *s = GetLocalizedString(primary ? 0xCF4F80C2u : 0xD87133BDu);
    std::memset(p.mName, 0, sizeof p.mName);
    std::strcpy(p.mName, s);   // bStrCpy 43DBD0 (no bound, like the original)
    p.mbNamed = 0;
}
}  // namespace

void UserProfile::SetProfileName(const char *name, bool primary)
{
    const bool has = name && bStrLen(name) > 0;
    std::memset(mName, 0, sizeof mName);
    if (has) {
        bStrNCpy(mName, name, 0x20);
        mbNamed = 1;
        return;
    }
    DefaultName(*this, primary);
}

void UserProfile::Default(bool secondary)
{
    DefaultName(*this, !secondary);
    mbFlagA0F9 = 0;
    mbFlagA0F8 = 1;
    mbFlagA0FA = 1;
    mQuickRace.Default();
    mCareerSettings = 1;
    if (bridge::EAXSoundInstance()) mAudioMode = bridge::EAXSound_GetDefaultPlatformAudioMode();
    _unknown254 = 0;
    std::memset(_unknown258, 0, sizeof _unknown258);
    for (int32_t i = 0; i < 27; ++i) {
        mEATrax[i].mIndex = i;
        const char *kind = bridge::EATraxPlayIn(i);
        uint8_t v;
        if (bStrCmp(kind, "FE") == 0) v = 1;
        else if (bStrCmp(kind, "IG") == 0) v = 2;
        else v = bStrCmp(kind, "AL") == 0 ? 3 : 0;
        mEATrax[i].mPlayIn = v;
    }
    bridge::EAXSound_InitEATRAX();   // PC: called even without a sound object
    mHighScores.Default();
    _unknownA0F4 = 0;
}

const UserProfileBlock kUserProfileBlocks[14] = {
    {offsetof(UserProfile, mEATrax), 0xD8}, {offsetof(UserProfile, mAudio), 0x3C}, {offsetof(UserProfile, mGameplay), 0x10},
    {offsetof(UserProfile, mPlayers), 0x50}, {offsetof(UserProfile, mVideo), 0x10}, {offsetof(UserProfile, mQuickRace), 0x17C},
    {offsetof(UserProfile, mCareerSettings), 4}, {offsetof(UserProfile, _unknown254), 4}, {offsetof(UserProfile, mbFlagA0F9), 1},
    {offsetof(UserProfile, mbFlagA0F8), 1}, {offsetof(UserProfile, _unknownA0F4), 4}, {offsetof(UserProfile, mHighScores), 0xA50},
    {offsetof(UserProfile, _unknown258), 0x80}, {offsetof(UserProfile, mName), 0x20},
};

uint8_t *UserProfile::SaveToBuffer(uint8_t *buf, int32_t size)
{
    uint8_t *const end = buf + size;
    uint8_t *p = mCarDB.SaveToBuffer(buf, static_cast<int32_t>(end - buf));
    p = bridge::PlayerCareer_SaveToBuffer(p, static_cast<int32_t>(end - p));
    const auto *self = reinterpret_cast<const uint8_t *>(this);
    for (const UserProfileBlock &b : kUserProfileBlocks) {
        if (p + b.mSize > end) continue;
        std::memcpy(p, self + b.mOffset, b.mSize);
        p += b.mSize;
    }
    return bridge::OnlineSettings_Save(p);
}

void UserProfile::LoadFromBuffer(const uint8_t *buf, int32_t size, bool apply)
{
    const uint8_t *const end = buf + size;
    bridge::PlayerCareer_Default(true);
    const uint8_t *p = mCarDB.LoadFromBuffer(buf, size);
    p = bridge::PlayerCareer_LoadFromBuffer(p, static_cast<int32_t>(end - p));
    auto *self = reinterpret_cast<uint8_t *>(this);
    for (const UserProfileBlock &b : kUserProfileBlocks) {
        if (p + b.mSize <= end) std::memcpy(self + b.mOffset, p, b.mSize);
        p += b.mSize;
    }
    bridge::OnlineSettings_Load(p);
    mbNamed = 1;
    if (!apply) return;
    mAudioMode = bridge::EAXSound_SetAudioModeFromMemoryCard(mAudioMode);
    bridge::EAXSound_UpdateVolumes(mAudio);
    bridge::EAXSound_InitEATRAX();
    bridge::Sound_5BE690();
    for (int32_t port = 0; port < 2; ++port) bridge::ApplyJoystickConfig(port);
}

void UserProfile::ReloadCarStable(const uint8_t *buf) { mCarDB.LoadFromBuffer(buf, 0x993E); }

int32_t UserProfile::GetSaveBufferSize() { return bridge::OnlineSettingsSaveSize() + 0xD254; }
