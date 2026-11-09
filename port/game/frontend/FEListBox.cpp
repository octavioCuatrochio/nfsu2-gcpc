#include "FEListBox.h"
#include <cmath>
#include <cstring>

namespace {
// The original moves these floats through integer registers (bit copy, NaN payloads kept).
inline void Raw(float &dst, const float &src) { std::memcpy(&dst, &src, 4); }

// One axis of ScrollSelection (PC 516EE2.. for X, 516F8B.. for Y). `savedFlags` is the flags copy the original
// keeps on the stack (wrap paths OR 8 into it, also for Y).
void ScrollAxis(FEListBox *lb, int32_t delta, uint32_t savedFlags, uint32_t &sel, uint32_t count, FEListBoxSlot *slots,
                float &scroll, float &target, float view, uint32_t shortFlag)
{
    int32_t idx = static_cast<int32_t>(sel) + delta;
    const int32_t n = static_cast<int32_t>(count);
    bool set = true;
    if (lb->mu32Flags & 4) {
        if (idx >= n) idx = n - 1;
        if (idx < 0) idx = 0;
    } else if (idx < 0) {
        // wrap backwards: start from the wrapped slot's position and add sizes up to the old selection
        uint32_t i = static_cast<uint32_t>(n + idx);
        target = scroll = slots[i].mfPosition;
        do {
            scroll = static_cast<float>(static_cast<long double>(slots[i].mfSize) + scroll);
            i = (i + 1) % count;
        } while (i != sel);
        set = false;
    } else if (idx >= n) {
        Raw(target, scroll);
        uint32_t i = sel;
        do {
            const uint32_t k = i % count;
            ++i;
            target = static_cast<float>(static_cast<long double>(slots[k].mfSize) + target);
        } while (i != static_cast<uint32_t>(idx));
        set = false;
    }
    if (set) {
        sel = static_cast<uint32_t>(idx);
        Raw(target, slots[idx].mfPosition);
    } else {
        lb->mu32Flags = savedFlags | 8;
        sel = static_cast<uint32_t>(idx) % count;
    }
    long double rest = 0.0f;   // sizes from the selection to the end
    for (uint32_t i = sel; i < count; ++i) rest = rest + slots[i].mfSize;
    if (lb->mu32Flags & 4) {
        if (rest < view) target = static_cast<float>(target - (view - rest));
    } else if (rest < view) {
        lb->mu32Flags |= shortFlag;
    }
}
}  // namespace

void FEListBox::ScrollSelection(int32_t dx, int32_t dy)
{
    uint32_t saved = mu32Flags;
    if ((saved & 0x60) == 0x60) return;
    if (saved & 0x20) dx = 0;
    if (saved & 0x40) dy = 0;
    if (dx == 0 && dy == 0) return;
    if (dx != 0) {
        ScrollAxis(this, dx, saved, mu32SelColumn, mu32NumColumns, mpColumns, mfScrollX, mfTargetX, mfViewWidth, 8);
        mu32Flags |= 0x20;
    }
    if (dy != 0) {
        saved = mu32Flags;
        if (!(saved & 0x40)) {
            ScrollAxis(this, dy, saved, mu32SelRow, mu32NumRows, mpRows, mfScrollY, mfTargetY, mfViewHeight, 0x10);
            mu32Flags |= 0x40;
        }
    }
    mu32Flags |= 2;
    const long double ddx = static_cast<long double>(mfTargetX) - mfScrollX;
    const long double ddy = static_cast<long double>(mfTargetY) - mfScrollY;
    mfDirX = static_cast<float>(ddx);
    mfDirY = static_cast<float>(ddy);
    const long double len = __builtin_sqrtl(ddy * ddy + ddx * ddx);   // x87 fsqrt (current precision)
    if (len < static_cast<long double>(0.1f)) {
        // CompleteScroll (inlined): jump to the target, clear scrolling/pending; selection 0 resets the axis
        Raw(mfScrollX, mfTargetX);
        Raw(mfScrollY, mfTargetY);
        mu32Flags &= ~0x62u;
        if (mu32SelColumn == 0) { mu32Flags &= ~8u; mfScrollX = 0.0f; }
        if (mu32SelRow == 0) { mu32Flags &= ~0x10u; mfScrollY = 0.0f; }
        return;
    }
    const long double inv = static_cast<long double>(1.0f) / len;
    mfDirX = static_cast<float>(inv * ddx);
    mfDirY = static_cast<float>(ddy * inv);
}
