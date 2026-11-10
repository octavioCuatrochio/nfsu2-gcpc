// GC option widgets of UIOptionsScreen (zFe2.cpp: AO* audio, GO* gameplay, VO* video, PO* player, OO* online).
// Each is a stock widget (FESliderWidget / FEToggleWidget / FEButtonWidget layout) with its own vtable whose
// Act (slot 1) / Draw (slot 3) / SetInitialValues (slot 17, sliders) edit one game setting.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEWidget.h"

// Audio settings (PC 0x83AA28, GC 0x803BC274): floats +0x00 master, +0x04 speech, +0x08 FE music, +0x0C in-game
// music, +0x10 SFX master, +0x14 engine, +0x18 tires. After a change PC calls EAXSound (*0x82B884) 469050(&settings).
// AO*Vol (PC 0x98): Act = slider Update (virtual) -> BlinkArrows; field = GetValue (virtual); refresh; Draw (virtual).
// Draw = FEngSetLanguageHash(name object, hash). SetInitialValues = min 0, max 1, step 0.05, SetCurValue(field).
struct AOVolumeSlider {
    FESliderWidget base;
    void Act(uint32_t msg, uint32_t field);
    void Draw(uint32_t nameHash) const;
    void SetInitialValues(uint32_t field);
};
// PC vtables / functions (Act, Draw, SetInitialValues), field, name hash:
//   AOMasterVol    7918F8:   49A720 49A770 49A790  +0x00 0x26782C3E   (GC 800E... AOMasterVol)
//   AOSFXMasterVol 791948:   49A7C0 49A810 49A830  +0x10 0xFD487543
//   AOTireVol      791998:   49A860 49A8B0 49A8D0  +0x18 0x69B745C6
//   AOEngineVol    7919E8:   49A900 49A950 49A970  +0x14 0xA2B1F888
//   AOSpeechVol    791A38:   49A9A0 49A9F0 49AA10  +0x04 0x9E5FB82A
//   AOFEMusicVol   791A88:   49AA40 49AA90 49AAB0  +0x08 0x418E681D
//   AOIGMusicVol   791AD8:   49AAE0 49AB30 49AB50  +0x0C 0xDF21EAC2
struct AOVolumeInfo { uint32_t vtable, act, draw, init, field, hash; const char *name; };
extern const AOVolumeInfo kAOVolumes[7];

// On/off toggles (FEToggleWidget layout). Act: left/right flip the byte flag, then (any message) BlinkArrows and
// Draw (virtual). Draw: name hash, value 0x63C (on) / 0xCCFA (off). Flag kinds: a global byte, a byte in the
// player options record of the player being edited (*0x836364), or (PC-only 791DB0) of the widget's own player +0x54.
struct OptionToggle {
    FEToggleWidget base;             // +0x00
    int32_t mPlayer;                 // +0x54 (791DB0 only, 0x58 bytes; the others are 0x54)
};
enum class ToggleFlag : uint8_t { Global, EditedPlayer, OwnPlayer };
struct OptionToggleInfo { uint32_t vtable, act, draw; ToggleFlag kind; uint32_t addr; uint32_t nameHash; const char *name; };
// addr: the byte (Global) or the offset in the 0x28-byte player record (player kinds)
extern const OptionToggleInfo kOptionToggles[10];
uint8_t &OptionToggleFlag(const OptionToggle &t, const OptionToggleInfo &info);
void OptionToggleAct(OptionToggle &t, const OptionToggleInfo &info, uint32_t msg);
void OptionToggleDraw(const OptionToggle &t, const OptionToggleInfo &info);
// PC-only 791DB0 Enable / Disable (4B5F70 / 4B5FA0): enabled flag, then "UnHighlight" / "Disabled" on object + value.
void OptionToggleEnable(OptionToggle &t);
void OptionToggleDisable(OptionToggle &t);

// Multi-value toggles (FEToggleWidget layout, 0x54). Act: left (0x9120409E) = previous, right (0xB5971BF1) = next,
// wrapping in [0, max]; then BlinkArrows and Draw (virtual). Draw: name hash, value hash by the value (else 0).
//   AOAudioMode        791B28: Act 49AB80 / Draw 49ABD0  int 0x83AA50, max 1 (stored after BlinkArrows), name 0x2881AB87,
//                      values 0xC50FA35F 0x55DA8BF8
//   GOSpeedoUnits      791C90: 49AE80 / 49AEE0  byte 0x83AA1A: 0 <-> 1 (other values kept), name 0xF86A6272,
//                      value 0x8569A25F if set else 0x8569AB44
//   GORacingMiniMap    791CD8: 49AFE0 / 49B040  byte 0x83AA1B, max 1 in game mode 4 else 2, name 0x9FA5EC9E,
//                      values 0xCCFA 0xF75595F2 0xF4B00E99
//   GOCareerDifficulty 791D20: 49AF20 / 49AF70  int 0x83AC30, max 2, name 0xBC0E6C55, values 0xDBBB4E8F 0xCAB262DE
//                      0xDBBCF37C
//   GOExploringMiniMap 791D68: 49B0B0 / 49B100  byte 0x83AA1C, max 2, name 0xC6269082, values as GORacingMiniMap
struct OptionCycle {
    FEToggleWidget base;
    void AudioModeAct(uint32_t msg);       void AudioModeDraw() const;
    void SpeedoUnitsAct(uint32_t msg);     void SpeedoUnitsDraw() const;
    void RacingMiniMapAct(uint32_t msg);   void RacingMiniMapDraw() const;
    void DifficultyAct(uint32_t msg);      void DifficultyDraw() const;
    void ExploringMiniMapAct(uint32_t msg); void ExploringMiniMapDraw() const;
    // edited player's record (0x83AA64 + *0x836364 * 0x28):
    //   POTransmission 7940F0: 4B75D0 / 4B7630  byte +0x15: 0 <-> 1, name 0xD31407E7, values 0x61CB0720 0x1B3A2485
    //   POHandling     794138: 4B7690 / 4B76F0  byte +0x16: 0 <-> 1, name 0xDF06ED22, values 0x63C 0xCCFA
    //   PODriveCam     794180: 4B7750 / 4B77B0  int +0x10, max 3, name 0xF6CCDC5F, values 0xC3E9AE58 0x414F19D7
    //                  0x5AE3441F 0x1EA4CEC2 0x916039B4 (jump table, 5 cases)
    //   POSplitTime    7942E8: 4B7AA0 / 4B7B00  byte +0x14, max 4, name 0x084BC378, values 0xD93F8F55 0xC44D3943
    //                  0x17FAFC32 0x1EA459F8 0xCCFA
    void TransmissionAct(uint32_t msg);    void TransmissionDraw() const;
    void HandlingAct(uint32_t msg);        void HandlingDraw() const;
    void DriveCamAct(uint32_t msg);        void DriveCamDraw() const;
    void SplitTimeAct(uint32_t msg);       void SplitTimeDraw() const;
    // GOJumpCams 791C00: Act 49AD40 / Draw 49ADC0. Float 0x83AA20: left/right -> 127 if it was 0 else 0 (NaN -> 0);
    // when *0x8650B8 exists: its 578EF0(3, 0, _ftol(value), 1). Draw only (no BlinkArrows). Draw: name 0xF26A5CBF,
    // 0xCCFA if 0 else 0x63C.
    void JumpCamsAct(uint32_t msg);        void JumpCamsDraw() const;
    // GOAutoSave 793D68: Act 49B170 / Draw 49B210. Bytes 0x844AD8 (on) / 0x844AD9 (second state, needs 502FE0 on
    // 0x83A9E0): right: AD9 -> on only; on -> off; off -> on (+AD9 if the check passes). Left: AD9 -> off; off -> on;
    // on -> on+AD9 if the check passes else off. Then BlinkArrows + Draw. Draw: name 0x996463E6, AD9 0x1B7ABF8E,
    // on 0x1B7ABF8F, off 0x1B7ABF90.
    void AutoSaveAct(uint32_t msg);        void AutoSaveDraw() const;
    // PC-only resolution toggle 791F30: Act 49B600 / Draw 49B690. Index *0x870D1C (0..5) over the mode table
    // 0x86E880[6] (nonzero = available): right/left move to the next / previous available index (none: the
    // last / first); an unavailable result restores the old index; a change sets the dirty byte 0x8709D1.
    // Draw only (no BlinkArrows). Draw: name 0xA9A76BBA, value printed "640x480" .. "1600x1200" (0..5).
    void ResolutionAct(uint32_t msg);      void ResolutionDraw() const;
};

// PC-only detail-level slider 791F78 (0x9C): Act 49B740 / Draw 49B7A0 / SetInitialValues 49B7C0. Value = int
// *0x87099C in [1, 5] (step 1). A moved slider: BlinkArrows, value = _ftol(GetValue), apply (5BE9F0), dirty byte
// 0x8709D1, Draw of the linked widget (+0x98, virtual), own Draw. Draw: name 0xB55E7665.
struct DetailSlider {
    FESliderWidget base;             // +0x00
    FEWidget *mpLinked;              // +0x98 (redrawn after a change)
    void Act(uint32_t msg);
    void Draw() const;
    void SetInitialValues();
};

// Option buttons (FEButtonWidget layout, 0x38). "Forward" = FEngSetScript(pkg, 0x47FF4E7C, "FORWARD", true).
//   COWheelOptions   791E00: Act 49B2F0 (0x406415E3 or 0x0C407210 -> forward; then Draw), Draw 49B330 (0x4DEAB931),
//                    Enable 4B5FD0 / Disable 4B5FF0 (flag + "UnHighlight" / "Disabled" on the object)
//   PC button 792060: Act 49B930 (0x0C407210 -> forward), Draw 49B960 (0x381FE1E9)
//   PC defaults btn  7920A0: Act 49B980 (0x0C407210 or 0x406415E3 -> MakeFrontendSound(1, 0), DialogInterface::
//                    ShowTwoButtons(pkg, "GenericDialog_SMALL.fng", 0xF770, 0x61C, 0xD05FC3A3, 0x34DC1BCF, 0x34DC1BCF,
//                    0, 0x2BBCCA6B)), Draw 49B9E0 (0x13F0EDEB)
//   TOSSX / TONFLStreet / PC NASCAR trailers 794480 / 7944C8 / 794518: shared Act 4B8300 (0x0C407210 -> trailer
//                    name *0x7F4480 = "SSX" for all three (folded PC code), forward); Draw 4B82C0 / 4B82E0 / 4B8330
//                    prints "SSX" / "NFL Street" / "NASCAR Thunder" into the object.
struct OptionButton {
    FEButtonWidget base;
    void WheelOptionsAct(const char *pkg, uint32_t msg);  void WheelOptionsDraw() const;
    void WheelOptionsEnable();                            void WheelOptionsDisable();
    static void ForwardAct(const char *pkg, uint32_t msg);   void Button792060Draw() const;
    static void DefaultsAct(const char *pkg, uint32_t msg);  void DefaultsDraw() const;
    static void TrailerAct(const char *pkg, uint32_t msg);
    void TrailerDraw(const char *name) const;
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(OptionButton) == 0x38);
static_assert(sizeof(OptionCycle) == 0x54 && sizeof(DetailSlider) == 0x9C);
static_assert(sizeof(AOVolumeSlider) == 0x98 && offsetof(OptionToggle, mPlayer) == 0x54);
#endif
