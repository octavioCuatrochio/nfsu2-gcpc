#include "OptionWidgets.h"
#include "bridge/Bridge.h"
#include "FEngUtil.h"
#include <initializer_list>

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

namespace {
constexpr uint32_t kLeft = 0x9120409Eu, kRight = 0xB5971BF1u;
int32_t Cycle(int32_t v, uint32_t msg, int32_t max)
{
    if (msg == kLeft) { if (--v < 0) v = max; }
    else if (msg == kRight) { if (++v > max) v = 0; }
    return v;
}
void BlinkDraw(void *w, uint32_t msg)
{
    bridge::FEWidget_BlinkArrows(w, msg);   // +0x40
    bridge::FEWidget_Draw(w);               // +0x0C
}
void SetNameValue(const FEToggleWidget &t, uint32_t name, uint32_t value)
{
    FEngSetLanguageHash(t.base.mpObject, name);
    FEngSetLanguageHash(t.base.mpValue, value);
}
uint32_t Pick(int32_t v, std::initializer_list<uint32_t> values)
{
    return v >= 0 && static_cast<size_t>(v) < values.size() ? values.begin()[v] : 0u;
}
int32_t &GameInt(uint32_t a) { return *reinterpret_cast<int32_t *>(bridge::GameByte(a)); }
}  // namespace

void OptionCycle::AudioModeAct(uint32_t msg)
{
    const int32_t v = Cycle(GameInt(0x83AA50), msg, 1);
    bridge::FEWidget_BlinkArrows(this, msg);
    GameInt(0x83AA50) = v;
    bridge::FEWidget_Draw(this);
}
void OptionCycle::AudioModeDraw() const { SetNameValue(base, 0x2881AB87u, Pick(GameInt(0x83AA50), {0xC50FA35Fu, 0x55DA8BF8u})); }

void OptionCycle::SpeedoUnitsAct(uint32_t msg)
{
    if (msg == kLeft || msg == kRight) {
        uint8_t &f = *bridge::GameByte(0x83AA1A);
        if (f == 0) f = 1;
        else if (f == 1) f = 0;
    }
    BlinkDraw(this, msg);
}
void OptionCycle::SpeedoUnitsDraw() const { SetNameValue(base, 0xF86A6272u, *bridge::GameByte(0x83AA1A) ? 0x8569A25Fu : 0x8569AB44u); }

void OptionCycle::RacingMiniMapAct(uint32_t msg)
{
    const int32_t max = bridge::GameModeFlags() != 4 ? 2 : 1;
    uint8_t &f = *bridge::GameByte(0x83AA1B);
    f = static_cast<uint8_t>(Cycle(f, msg, max));
    BlinkDraw(this, msg);
}
void OptionCycle::RacingMiniMapDraw() const { SetNameValue(base, 0x9FA5EC9Eu, Pick(*bridge::GameByte(0x83AA1B), {0xCCFAu, 0xF75595F2u, 0xF4B00E99u})); }

void OptionCycle::DifficultyAct(uint32_t msg)
{
    GameInt(0x83AC30) = Cycle(GameInt(0x83AC30), msg, 2);
    BlinkDraw(this, msg);
}
void OptionCycle::DifficultyDraw() const { SetNameValue(base, 0xBC0E6C55u, Pick(GameInt(0x83AC30), {0xDBBB4E8Fu, 0xCAB262DEu, 0xDBBCF37Cu})); }

void OptionCycle::ExploringMiniMapAct(uint32_t msg)
{
    uint8_t &f = *bridge::GameByte(0x83AA1C);
    f = static_cast<uint8_t>(Cycle(f, msg, 2));
    BlinkDraw(this, msg);
}
void OptionCycle::ExploringMiniMapDraw() const { SetNameValue(base, 0xC6269082u, Pick(*bridge::GameByte(0x83AA1C), {0xCCFAu, 0xF75595F2u, 0xF4B00E99u})); }
