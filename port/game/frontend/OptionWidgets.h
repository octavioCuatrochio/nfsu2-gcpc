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

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(AOVolumeSlider) == 0x98);
#endif
