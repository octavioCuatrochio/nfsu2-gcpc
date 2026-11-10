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
