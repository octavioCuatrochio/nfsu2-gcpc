#include "OptionWidgets.h"
#include "bridge/Bridge.h"
#include "FEngUtil.h"

const AOVolumeInfo kAOVolumes[7] = {
    {0x7918F8, 0x49A720, 0x49A770, 0x49A790, 0x00, 0x26782C3Eu, "AOMasterVol"},
    {0x791948, 0x49A7C0, 0x49A810, 0x49A830, 0x10, 0xFD487543u, "AOSFXMasterVol"},
    {0x791998, 0x49A860, 0x49A8B0, 0x49A8D0, 0x18, 0x69B745C6u, "AOTireVol"},
    {0x7919E8, 0x49A900, 0x49A950, 0x49A970, 0x14, 0xA2B1F888u, "AOEngineVol"},
    {0x791A38, 0x49A9A0, 0x49A9F0, 0x49AA10, 0x04, 0x9E5FB82Au, "AOSpeechVol"},
    {0x791A88, 0x49AA40, 0x49AA90, 0x49AAB0, 0x08, 0x418E681Du, "AOFEMusicVol"},
    {0x791AD8, 0x49AAE0, 0x49AB30, 0x49AB50, 0x0C, 0xDF21EAC2u, "AOIGMusicVol"},
};

namespace {
float &AudioField(uint32_t off) { return *reinterpret_cast<float *>(bridge::AudioSettings() + off); }
}

void AOVolumeSlider::Act(uint32_t msg, uint32_t field)
{
    if (bridge::cSlider_Update(&base.mSlider, msg)) bridge::FEWidget_BlinkArrows(this, msg);   // +0x40
    AudioField(field) = bridge::FEWidget_GetValue(this);                                      // +0x48
    bridge::EAXSound_UpdateAudioSettings();
    bridge::FEWidget_Draw(this);                                                               // +0x0C
}

void AOVolumeSlider::Draw(uint32_t nameHash) const { FEngSetLanguageHash(base.base.mpObject, nameHash); }

void AOVolumeSlider::SetInitialValues(uint32_t field)
{
    const float v = AudioField(field);
    cSlider &s = base.mSlider;
    s.mMin = 0.0f;
    s.mMax = 1.0f;
    s.mStep = 0.05f;   // 0x3D4CCCCD
    s.SetCurValue(v);
}

const OptionToggleInfo kOptionToggles[10] = {
    {0x791B70, 0x49AC20, 0x49AC60, ToggleFlag::Global, 0x8500C9, 0x0B660729u, "OOShowPlayers"},
    {0x791BB8, 0x49ACB0, 0x49ACF0, ToggleFlag::Global, 0x8500CA, 0xD1009B34u, "OOBadConnectionWarning"},
    {0x791C48, 0x49AE00, 0x49AE40, ToggleFlag::Global, 0x83AA18, 0x85A6CE05u, "GORearview"},
    {0x7941C8, 0x4B7860, 0x4B78B0, ToggleFlag::EditedPlayer, 0x00, 0xAC148579u, "POGauges"},
    {0x794210, 0x4B78F0, 0x4B7940, ToggleFlag::EditedPlayer, 0x01, 0x82CD8F92u, "POPosition"},
    {0x794258, 0x4B7980, 0x4B79D0, ToggleFlag::EditedPlayer, 0x02, 0x969F96A5u, "POLapInfo"},
    {0x7942A0, 0x4B7A10, 0x4B7A60, ToggleFlag::EditedPlayer, 0x03, 0xC03F9F19u, "POScore"},
    {0x794330, 0x4B7BB0, 0x4B7C00, ToggleFlag::EditedPlayer, 0x05, 0xC93FBFB5u, "POLeaderBoard"},
    {0x794378, 0x4B7C40, 0x4B7C90, ToggleFlag::EditedPlayer, 0x06, 0xBFB34120u, "POTrannyPrompt"},
    {0x791DB0, 0x49B260, 0x49B2B0, ToggleFlag::OwnPlayer, 0x04, 0x358CDCFDu, "PC player toggle 791DB0"},
};

uint8_t &OptionToggleFlag(const OptionToggle &t, const OptionToggleInfo &info)
{
    switch (info.kind) {
    case ToggleFlag::Global: return *bridge::GameByte(info.addr);
    case ToggleFlag::EditedPlayer: return bridge::PlayerControllerOptions(bridge::PlayerToEditForOptions())[info.addr];
    default: return bridge::PlayerControllerOptions(t.mPlayer)[info.addr];
    }
}

void OptionToggleAct(OptionToggle &t, const OptionToggleInfo &info, uint32_t msg)
{
    if (msg == 0x9120409Eu || msg == 0xB5971BF1u) {
        uint8_t &f = OptionToggleFlag(t, info);
        f = f == 0;
    }
    bridge::FEWidget_BlinkArrows(&t, msg);   // +0x40
    bridge::FEWidget_Draw(&t);               // +0x0C
}

void OptionToggleDraw(const OptionToggle &t, const OptionToggleInfo &info)
{
    FEngSetLanguageHash(t.base.base.mpObject, info.nameHash);
    FEngSetLanguageHash(t.base.base.mpValue, OptionToggleFlag(t, info) ? 0x63Cu : 0xCCFAu);
}

void OptionToggleEnable(OptionToggle &t)
{
    t.base.base.base.mbEnabled = 1;
    FEngSetScript(t.base.base.mpObject, "UnHighlight", true);
    FEngSetScript(t.base.base.mpValue, "UnHighlight", true);
}

void OptionToggleDisable(OptionToggle &t)
{
    t.base.base.base.mbEnabled = 0;
    FEngSetScript(t.base.base.mpObject, "Disabled", true);
    FEngSetScript(t.base.base.mpValue, "Disabled", true);
}
