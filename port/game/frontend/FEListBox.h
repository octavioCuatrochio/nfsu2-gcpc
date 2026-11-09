// FEng list box (FEObject type 4, PC 0xAC bytes). Original: zFe2.cpp. Field roles as used by ScrollSelection.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEObject.h"

// One column / row of a list box (12 bytes).
struct FEListBoxSlot {
    float mfSize;                    // +0x0 width (column) / height (row)
    float mfPosition;                // +0x4 scroll position that shows this slot
    uint32_t _unknown08;
};

struct FEListBox {
    FEObject obj;                    // +0x00
    uint32_t mu32Flags;              // +0x5C 2 scrolling, 4 clamp (no wrap), 8 / 0x10 X / Y wrapped or short,
                                     //       0x20 / 0x40 X / Y scroll pending (blocks further input on that axis)
    uint32_t mu32NumColumns;         // +0x60
    uint32_t mu32NumRows;            // +0x64
    float mfViewWidth;               // +0x68
    float mfViewHeight;              // +0x6C
    float mfScrollX;                 // +0x70 current
    float mfScrollY;                 // +0x74
    FEListBoxSlot *mpColumns;        // +0x78
    FEListBoxSlot *mpRows;           // +0x7C
    float mfSpeedX;                  // +0x80 scroll speed factors (Update: |dir . speed| * dt per step)
    float mfSpeedY;                  // +0x84
    uint32_t mu32SelColumn;          // +0x88
    uint32_t mu32SelRow;             // +0x8C
    uint8_t _unknown90[4];
    float mfTargetX;                 // +0x94
    float mfTargetY;                 // +0x98
    float mfDirX;                    // +0x9C unit scroll direction
    float mfDirY;                    // +0xA0
    float mfPulse;                   // +0xA4 0..1 ping-pong value (Update)
    float mfPulseRate;               // +0xA8 per-second rate; sign flips at the ends

    // GC 801362FC / PC 516E90 (__thiscall, ret 8): move the selection by (dx, dy) and start scrolling towards it.
    // PC: sums and the direction length stay in x87 (unrounded); CompleteScroll (GC 80136770) is inlined.
    void ScrollSelection(int32_t dx, int32_t dy);
    // GC 80136B4C / PC 5043B0: jump to the target, clear scrolling/pending; selection 0 resets that axis to 0.
    void CompleteScroll();
    // GC 80136770 / PC 517230 (__thiscall, ret 4): advance the pulse and the scroll by dt seconds.
    // PC: the pulse bounds are tested on the unrounded x87 value; the X step stays unrounded but the Y step is
    // rounded to float before it is added; the overshoot test uses the unrounded new positions.
    void Update(float dt);
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEListBox) == 0xAC && offsetof(FEListBox, mu32Flags) == 0x5C && offsetof(FEListBox, mpRows) == 0x7C);
static_assert(offsetof(FEListBox, mu32SelColumn) == 0x88 && offsetof(FEListBox, mfTargetX) == 0x94 && offsetof(FEListBox, mfDirY) == 0xA0);
#endif
