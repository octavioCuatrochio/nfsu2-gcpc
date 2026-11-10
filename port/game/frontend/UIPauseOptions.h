// GC UIPauseOptions (zFe.cpp): one in-race options page (pause menu -> Options), page index PC *0x83AA04: 0 audio,
// 1 video, 2 gameplay, 3 player. PC 0x15C bytes (factory 4EB650 area: new 0x15C), vtable 0x796504: 0 scalar dtor
// 4EB6B0, 1 40D080, 2 NotificationMessage 4A8010, 3 MenuScreen NotifySoundMessage 570A70, 4 Setup 4C4BD0.
// PC differs from GC: header strings per page, video page (resolution / detail / two buttons, like UIOptionsScreen),
// the help bar is pushed by the ctor, PC "exit to race" flag +0x158 (messages 0x33333333 / 0xB5AF2461).
#pragma once
#include <cstddef>
#include <cstdint>
#include "UIWidgetMenu.h"

struct ScreenConstructorData;
struct FEObject;
struct FEToggleWidget;

struct UIPauseOptions {
    UIWidgetMenu base;               // +0x000
    FEToggleWidget *mpHandling;      // +0x14C PPOHandling row (player page), written only
    uint8_t mbColorCal;              // +0x150 cleared by ctor and Setup (as UIOptionsScreen +0x14C), never read
    uint8_t _pad151[3];
    int32_t mVibrationIndex;         // +0x154 791DB0 vibration toggle row (1-based)
    uint8_t mbExitToRace;            // +0x158 next back (0xC98356BA) pops the screen + help bar and unpauses
    uint8_t mbAutoSave;              // +0x159 copy of byte 0x844AD8 taken by SetupGameplay
    uint8_t mbAutoSave2;             // +0x15A copy of byte 0x844AD9
    uint8_t mbBusy;                  // +0x15B a confirm dialog / out-script is pending

    UIPauseOptions *Construct(const ScreenConstructorData *data);    // GC 80100F98 / PC 4D93C0 (ret 4)
    void Destruct();                                                 // GC 80100FFC / PC 4D9470 (vtable, then UIWidgetMenu 4CBC30)
    UIPauseOptions *ScalarDeletingDestructor(uint32_t flags);        // PC 4EB6B0 (ret 4)
    // GC 80101094 / PC 4A7F70 (ret 4): false (and a two-button dialog whose first button sends `msg`) when an
    // autosave setting changed and needs confirming; true otherwise.
    bool ReadyToExit(uint32_t msg);
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 80101164 / PC 4A8010
    void Setup();                    // GC 80101EDC / PC 4C4BD0
    void SetupAudio();               // GC 80101580 / PC 4C3370
    void SetupVideo();               // PC 4C36C0 (PC page)
    void SetupGameplay();            // GC 80101818 / PC 4C38F0
    void SetupPlayer();              // GC 80101AF4 / PC 4C4740
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(UIPauseOptions) == 0x15C && offsetof(UIPauseOptions, mVibrationIndex) == 0x154 &&
              offsetof(UIPauseOptions, mbBusy) == 0x15B);
#endif
