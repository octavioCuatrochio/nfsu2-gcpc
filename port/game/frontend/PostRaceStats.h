// GC PostRaceStatsScreen (zFe.cpp): per-racer stats sheet (UI_PostRaceStats.fng): name + car, stat rows (PR*
// FEStatWidget subclasses reading a FinishedRaceStatsEntry), lap list (PRLapResult rows), racer left / right.
#pragma once
#include <cstddef>
#include <cstdint>
#include "UIWidgetMenu.h"
#include "FEWidget.h"

// GC PR* stat rows (PC 0x50: FEStatWidget + entry). PC vtables (slot 3 Draw, slot 4 Position 4A4300):
// PRFinishTime 0x795130 (4BF9E0), PRTimeBehind 0x795170 (4A4380), PRTopSpeed 0x7951B0 (4BFA30), PRAverageSpeed
// 0x795200 (4BFAA0), PRPerfectShifts 0x795240 (4BFB10), PRBestLapTime 0x795280 (ctor 4BFB50, 4BFB90), PRLapsLed
// 0x7952C0 (ctor 4BFBE0, 4BFC20), PRBestDriftScore 0x795308 (4BFC60), PRLongestDrift 0x795348 (4BFC90), PRScore
// 0x795388 (4BFD00). Slot 0 = the generic widget scalar dtor 4AEA10.
struct PRStatWidget {
    FEStatWidget base;               // +0x00
    const uint8_t *mpEntry;          // +0x4C FinishedRaceStatsEntry

    void Position();                 // PC 4A4300: FEStatWidget::Position, then the value box right-aligned at valueSize + valuePos
    void DrawFinishTime();           // GC PRFinishTime::Draw / PC 4BF9E0
    void DrawTimeBehind();           // PC 4A4380: status text, or the time behind the first entry (split format)
    void DrawTopSpeed();             // PC 4BFA30
    void DrawAverageSpeed();         // PC 4BFAA0
    void DrawPerfectShifts();        // PC 4BFB10
    void DrawBestLapTime();          // PC 4BFB90
    void DrawLapsLed();              // PC 4BFC20
    void DrawBestDriftScore();       // PC 4BFC60
    void DrawLongestDrift();         // PC 4BFC90
    void DrawScore();                // PC 4BFD00
    // PC 4BFB50 / 4BFBE0 (ret 0x18): the two out-of-line ctors (PRBestLapTime / PRLapsLed)
    PRStatWidget *Construct(uint32_t vtable, const uint8_t *entry, const float *pos, const float *size,
                            const float *valueSize, const float *valuePos, bool enabled);
};

// GC PRLapResult (PC 0x58, vtable 0x7950E8): "lap   time" and position.
struct PRLapResult {
    FEStatWidget base;               // +0x00
    int32_t mLap;                    // +0x4C
    int32_t mTime;                   // +0x50 Timer ticks
    int32_t mPosition;               // +0x54
    void Draw();                     // PC 4BF990
};

// PC 0x190 (factory 4FD6D0), vtable 0x799108: 0 scalar dtor 4FBDA0, 2 NotificationMessage 4FBE20, 3 sounds 4A4750.
struct PostRaceStatsScreen {
    UIWidgetMenu base;               // +0x000
    int32_t _unknown14C;             // +0x14C
    float mTimeX, mTimeY, mTimeW, mTimeH;    // +0x150 TIME_MASTER box
    float mPosX, mPosY, mPosW, mPosH;        // +0x160 POSITION_MASTER box
    float mLapX, mLapY;              // +0x170 next lap row position
    WidgetListNode mLaps;            // +0x178 PRLapResult rows (node at widget +4)
    int32_t mLapRow;                 // +0x180 1-based (lap row object names)
    int32_t mRacer;                  // +0x184 shown finisher index
    uint8_t mbLaterLaps, _pad189[3]; // +0x188 laps 11.. instead of 1..10
    const uint8_t *mpEntry;          // +0x18C

    PostRaceStatsScreen *Construct(const ScreenConstructorData *data);   // GC 800F50D4 / PC 4FBBC0 (ret 4)
    void Destruct();                                                    // GC 801183A0 / PC 4FBDC0
    PostRaceStatsScreen *ScalarDeletingDestructor(uint32_t f);          // PC 4FBDA0 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800F520C / PC 4FBE20
    void Init();                                                        // GC 800F52FC / PC 4F3CA0
    void SetLapList(int32_t first, int32_t last);                       // GC 800F549C / PC 4D73B0 (ret 8)
    void AddLapResult(int32_t lap, int32_t time, int32_t position);     // GC 800F551C / PC 4BFD40 (ret 0xC)
    void ClearLapResultWidgets();                                       // GC 800F56DC / PC 4BFF00
    void Setup(int32_t racer);                                          // GC 800F57B4 / PC 4E9C00 (ret 4)
    void SetNavigationArrows(int32_t racer, uint32_t msg);              // GC 800F5D2C / PC 4A4480 (ret 8)
    void Scroll(uint32_t msg);                                          // GC 800F5F04 / PC 4F3DE0 (ret 4)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(PRStatWidget) == 0x50 && sizeof(PRLapResult) == 0x58);
static_assert(sizeof(PostRaceStatsScreen) == 0x190 && offsetof(PostRaceStatsScreen, mLaps) == 0x178 &&
              offsetof(PostRaceStatsScreen, mRacer) == 0x184 && offsetof(PostRaceStatsScreen, mpEntry) == 0x18C);
#endif
