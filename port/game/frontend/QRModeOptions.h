// GC UIQRModeOptions (zFe.cpp): quick race options (laps, direction, lap KO, traffic, opponents, skill, catch-up,
// outrun points / time) built from MO* / TO* toggle widgets that edit QuickRaceSettings (PC 0x83AAB4).
// Network code is not ported (AGENTS.md): BoilerPlateOnline 4B41C0, HandlePasswordEntryRestrictions 4982C0 and the
// online helpers are reached through bridge stubs; the screen keeps the online branches around them.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEWidget.h"
#include "UIWidgetMenu.h"

// The quick race toggles (FEToggleWidget layout, 0x54; TO_OL_NumLaps 0x58). Act(pkg, msg) (ret 8): left / right
// change the setting, then BlinkArrows (slot 0x40) and Draw (slot 0x0C). Draw (slot 3): name hash on +0x2C, value
// on +0x30. Ctors (ret 0x14) = FEToggleWidget ctor 51E230 + vtable; MOCatchUp / MOPointsToWin / MOTimeLimit have
// no ctor function on PC (inlined into the Setup* functions).
enum class QRToggle : uint8_t {
    NumOpponents,    // 791380: ctor 4B3750, Act 4B37F0, Draw 4B38C0
    AISkill,         // 7913C8: ctor 4B3930, Act 4B3960, Draw 4B39C0
    CatchUp,         // 791410:              Act 4B3A10, Draw 4B3A60
    TrafficLevel,    // 791458: ctor 4B3AA0, Act 4B3AD0, Draw 4B3B30
    PointsToWin,     // 7914A0:              Act 4B3BA0, Draw 4B3C00
    TimeLimit,       // 7914E8:              Act 4B3C60, Draw 4B3CC0
    NumLaps,         // 791530: ctor 4B3D30, Act 4B3D60, Draw 4B3DD0
    OnlineNumLaps,   // 791578: ctor 4B3E40, Act 4B3F10, Draw 4B3F70 (TO_OL_NumLaps, laps at 0x8500BA)
    LapKO,           // 7915C0: ctor 4B3FA0, Act 4B4020, Draw 4B3FD0
    TrackDirection,  // 791608: ctor 4B40B0, Act 4B40E0, Draw 4B4150
};
uint32_t QRToggleVTable(QRToggle k);

struct QRToggleWidget {
    FEToggleWidget base;             // +0x00
    uint16_t mMinLaps;               // +0x54 TO_OL_NumLaps only (1, 2 or 4 by the online race mode)
    uint8_t _pad56[2];

    QRToggleWidget *Construct(QRToggle k, const float *pos, const float *size, const float *valueSize,
                              const float *valuePos, bool enabled);
    void Act(QRToggle k, const char *pkg, uint32_t msg);
    void Draw(QRToggle k);
};

// PC 0x164, vtable 0x798014: 0 4EF5F0, 1 40D080, 2 NotificationMessage 4EF620, 3 570A70, 4 Setup 4E2C70.
struct UIQRModeOptions {
    UIWidgetMenu base;               // +0x000
    uint32_t mPasswordWidget;        // +0x14C widget index (online password entry)
    uint32_t mConfirmWidget;         // +0x150 widget index (online password confirm)
    uint32_t _unknown154;
    uint8_t mbAccepted;              // +0x158 0x0C407210 / 0x59337816 accepted: out script -> next screen
    uint8_t _pad159[3];
    void *mpDialog;                  // +0x15C online error dialog (DismissDialog)
    uint32_t mOnlineLapsWidget;      // +0x160 widget index of TO_OL_NumLaps (online)

    UIQRModeOptions *Construct(const ScreenConstructorData *data);   // GC 800E1CCC / PC 4EF3F0 (ret 4)
    void Destruct();                                                // GC 80117D74 / PC 4EF610
    UIQRModeOptions *ScalarDeletingDestructor(uint32_t f);          // PC 4EF5F0 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800E213C / PC 4EF620
    void Setup();                    // GC 800E2460 / PC 4E2C70 (vtable slot 4)
    void SetupCircuit();             // GC 800E25EC / PC 4CCF60
    void SetupSprint();              // GC 800E2930 / PC 4CD2D0
    void SetupDrag();                // GC 800E2B70 / PC 4CD530
    void SetupDrift();               // GC 800E2DC4 / PC 4CD7A0
    void SetupOutrun();              // GC 800E3278 / PC 4CDA20
    void SetupGT();                  // GC 800E335C / PC 4CDB40
    void SetupShortTrack();          // GC 800E3B20 (and SetupURL 800E3E7C, folded) / PC 4B47B0
    void SetupFreeRun();             // GC 800E3D60 / PC 4B4A10
    // GC 800E3038 (GC-only: PC's Setup has no street-cross case): direction (reversible + unlocked), laps, opponents /
    // AI skill unless splitscreen, catch-up.
    void SetupStreetCross();
    // GC 800E20DC (PC: inlined into the lap-KO toggle's Act 4B4020): lap KO on -> 3 laps, 3 opponents.
    static void SetupForLapKO();
    // GC 800E1EA0 HandlePasswordEntryRestrictions / 800E355C BoilerPlateOnline: network, not ported (bridged).
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(QRToggleWidget) == 0x58 && offsetof(QRToggleWidget, mMinLaps) == 0x54);
static_assert(sizeof(UIQRModeOptions) == 0x164 && offsetof(UIQRModeOptions, mbAccepted) == 0x158 &&
              offsetof(UIQRModeOptions, mOnlineLapsWidget) == 0x160);
#endif
