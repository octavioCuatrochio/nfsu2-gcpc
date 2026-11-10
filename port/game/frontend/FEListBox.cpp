#include "FEListBox.h"
#include <cmath>
#include <cstring>
#include "FEMath.h"
#include "bridge/Bridge.h"

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
        CompleteScroll();   // inlined on PC
        return;
    }
    const long double inv = static_cast<long double>(1.0f) / len;
    mfDirX = static_cast<float>(inv * ddx);
    mfDirY = static_cast<float>(ddy * inv);
}

void FEListBox::CompleteScroll()
{
    Raw(mfScrollX, mfTargetX);
    Raw(mfScrollY, mfTargetY);
    mu32Flags &= ~0x62u;
    if (mu32SelColumn == 0) { mu32Flags &= ~8u; mfScrollX = 0.0f; }
    if (mu32SelRow == 0) { mu32Flags &= ~0x10u; mfScrollY = 0.0f; }
}

void FEListBox::Update(float dt)
{
    const long double pulse = static_cast<long double>(dt) * mfPulseRate + mfPulse;
    mfPulse = static_cast<float>(pulse);
    bool flip = false;
    if (pulse < 0.0f) { mfPulse = 0.0f; flip = true; }
    else if (pulse > 1.0f) { mfPulse = 1.0f; flip = true; }   // NaN: neither
    if (flip) mfPulseRate = -mfPulseRate;
    if (!(mu32Flags & 2)) return;
    const long double x = mfDirX, y = mfDirY;
    long double step = static_cast<long double>(y * mfSpeedY + x * mfSpeedX);
    step = __builtin_fabsl(step) * dt;   // x87 fabs
    const long double px = x * step + mfScrollX;
    const float stepY = static_cast<float>(y * step);
    mfScrollX = static_cast<float>(px);
    const long double py = static_cast<long double>(stepY) + mfScrollY;
    mfScrollY = static_cast<float>(py);
    const long double toTarget = y * mfTargetY + x * mfTargetX;
    const long double current = py * y + px * x;
    if (toTarget - current < 0.0f) CompleteScroll();
}

FEListBox *FEListBox::Construct(void *mem)
{
    auto *lb = static_cast<FEListBox *>(mem);
    FEObject::Construct(mem);
    lb->mu32NumColumns = lb->mu32NumRows = 0;
    lb->obj.node.vtable = bridge::FEListBoxVTable();
    lb->mu32Flags = 4;
    lb->mfViewWidth = lb->mfViewHeight = lb->mfScrollX = lb->mfScrollY = 0.0f;
    lb->mpColumns = lb->mpRows = nullptr;
    lb->mfSpeedX = lb->mfSpeedY = 1.0f;
    lb->mu32SelColumn = lb->mu32SelRow = 0;
    lb->mpCells = nullptr;
    lb->mfTargetX = lb->mfTargetY = lb->mfDirX = lb->mfDirY = 0.0f;
    lb->mfPulse = 1.0f;
    const uint32_t rate = 0xBAB60B61u;   // -0.00138888 (1/720)
    std::memcpy(&lb->mfPulseRate, &rate, 4);
    lb->obj.mu32Type = 4;
    return lb;
}

void FEListBox::Destruct()
{
    obj.node.vtable = bridge::FEListBoxVTable();
    Terminate();
    obj.Destruct();
}

FEListBox *FEListBox::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

void FEListBox::Terminate()
{
    if (!(mu32Flags & 1)) return;
    mu32Flags &= ~1u;
    if (mu32NumColumns) {
        bridge::FreeArray(mpColumns);
        mpColumns = nullptr;
        mu32NumColumns = 0;
    }
    if (mu32NumRows) {
        bridge::FreeArray(mpRows);
        mpRows = nullptr;
        mu32NumRows = 0;
    }
    CleanupCells();
}

void FEListBox::CleanupCells()
{
    const uint32_t n = mu32NumRows * mu32NumColumns;
    if (!n) return;
    for (uint32_t i = 0; i < n; ++i) {
        FEListBoxCell &c = mpCells[i];
        if (c.mu32Type == 2 && c.mpText) {
            bridge::FreeArray(c.mpText);
            c.mpText = nullptr;
        }
    }
    bridge::FreeArray(mpCells);
    mpCells = nullptr;
}

void FEListBox::InitializeCells(FEListBoxCell *cells, uint32_t count)
{
    const float one = 1.0f;
    uint32_t oneBits;
    std::memcpy(&oneBits, &one, 4);
    for (uint32_t i = 0; i < count; ++i) {
        FEListBoxCell &c = cells[i];
        c.mu32Color = 0xFFFFFFFFu;
        c.mfScaleX = c.mfScaleY = 1.0f;
        c.mJustification[0] = c.mJustification[1] = c.mJustification[2] = 0;
        c.mu32Type = 0;
        c._unknown1C = 0;
        c.mData[0] = c.mData[1] = 0;
        c.mData[2] = c.mData[3] = oneBits;
    }
}

namespace {
// new FEListBoxCell[n] (PC: eh vector ctor with 504150; no cookie)
FEListBoxCell *NewCells(uint32_t n)
{
    auto *cells = static_cast<FEListBoxCell *>(bridge::Alloc(static_cast<uint32_t>(n * sizeof(FEListBoxCell))));
    if (cells)
        for (uint32_t i = 0; i < n; ++i) {
            FEListBoxCell &c = cells[i];
            c.mu32Color = 0;
            c.mfScaleX = c.mfScaleY = 1.0f;
            c.mJustification[0] = c.mJustification[1] = c.mJustification[2] = 0;
            c.mu32Type = 0;
            c.mData[1] = 0xFFFFFFFFu;   // +0x24 (before mpText: on 64-bit the pointer overlaps it)
            c.mpText = nullptr;          // +0x20 = 0
        }
    return cells;
}

// resize a slot array (columns / rows): keeps min(old, new), zeroes the rest
FEListBoxSlot *ResizeSlots(FEListBoxSlot *old, uint32_t oldCount, uint32_t n, uint32_t &keep)
{
    auto *slots = static_cast<FEListBoxSlot *>(bridge::Alloc(static_cast<uint32_t>(n * sizeof(FEListBoxSlot))));
    keep = 0;
    if (oldCount) {
        keep = n < oldCount ? n : oldCount;
        std::memcpy(slots, old, keep * sizeof(FEListBoxSlot));
        bridge::FreeArray(old);
    }
    std::memset(slots + keep, 0, (n - keep) * sizeof(FEListBoxSlot));
    return slots;
}
}  // namespace

void FEListBox::SetNumColumns(uint32_t n)
{
    if (n == 0) {
        if (mu32NumColumns) {
            bridge::FreeArray(mpColumns);
            mpColumns = nullptr;
            mu32NumColumns = 0;
            CleanupCells();
        }
        return;
    }
    uint32_t keep;
    FEListBoxSlot *cols = ResizeSlots(mpColumns, mu32NumColumns, n, keep);
    const uint32_t count = mu32NumRows * n;
    FEListBoxCell *cells = nullptr;
    if (count) {
        cells = NewCells(count);
        if (mpCells) {
            for (uint32_t r = 0; r < mu32NumRows; ++r) {
                // PC/GC: copies `keep` bytes, not cells
                std::memcpy(cells + r * n, mpCells + r * mu32NumColumns, keep);
                InitializeCells(cells + r * n + keep, n - keep);
            }
            bridge::FreeArray(mpCells);
        } else {
            InitializeCells(cells, count);
        }
    }
    mu32NumColumns = n;
    mpCells = cells;
    mpColumns = cols;
}

void FEListBox::SetNumRows(uint32_t n)
{
    if (n == 0) {
        if (mu32NumRows) {
            bridge::FreeArray(mpRows);
            mpRows = nullptr;
            mu32NumRows = 0;
            CleanupCells();
        }
        return;
    }
    uint32_t keep;
    FEListBoxSlot *rows = ResizeSlots(mpRows, mu32NumRows, n, keep);
    const uint32_t count = mu32NumColumns * n;
    FEListBoxCell *cells = nullptr;
    if (count) {
        cells = NewCells(count);
        if (mpCells) {
            std::memcpy(cells, mpCells, mu32NumColumns * keep);   // PC/GC: bytes, not cells
            InitializeCells(cells + mu32NumColumns * keep, mu32NumColumns * (n - keep));
            bridge::FreeArray(mpCells);
        } else {
            InitializeCells(cells, count);
        }
    }
    mu32NumRows = n;
    mpRows = rows;
    mpCells = cells;
}

void FEListBox::SetSelColumn(uint32_t c)
{
    if (!mu32NumColumns) mu32SelColumn = 0;
    else mu32SelColumn = c < mu32NumColumns ? c : mu32NumColumns - 1;
}

void FEListBox::SetSelRow(uint32_t r)
{
    if (!mu32NumRows) mu32SelRow = 0;
    else mu32SelRow = r < mu32NumRows ? r : mu32NumRows - 1;
}

void FEListBox::SetCellType(uint32_t type)
{
    FEListBoxCell *c = CurrentCell();
    if (c->mu32Type == type) return;
    if (c->mu32Type == 2 && c->mpText) {
        bridge::FreeArray(c->mpText);
        c->mpText = nullptr;
    }
    c->mu32Type = type;
}

void FEListBox::IncrementCellByColumn()
{
    if (++mu32SelColumn < mu32NumColumns) return;
    mu32SelColumn = 0;
    if (++mu32SelRow < mu32NumRows) return;
    mu32SelRow = 0;
}

void FEListBox::SetAutoWrap(bool wrap)
{
    if (wrap) mu32Flags &= ~4u;
    else mu32Flags |= 4u;
}

void FEListBox::RecalculateCummulative()
{
    long double acc = 0.0L;   // x87: positions are stored rounded, the sum stays in ST0
    for (uint32_t i = 0; i < mu32NumColumns; ++i) {
        mpColumns[i].mfPosition = static_cast<float>(acc);
        acc += mpColumns[i].mfSize;
    }
    acc = 0.0L;
    for (uint32_t i = 0; i < mu32NumRows; ++i) {
        mpRows[i].mfPosition = static_cast<float>(acc);
        acc += mpRows[i].mfSize;
    }
}

void FEListBox::SetCellColor(const FEColor &c)
{
    auto clamp = [](int32_t v) -> uint32_t { return v < 0 ? 0u : v > 0xFF ? 0xFFu : static_cast<uint32_t>(v); };
    CurrentCell()->mu32Color = clamp(c.c[3]) << 24 | clamp(c.c[2]) << 16 | clamp(c.c[1]) << 8 | clamp(c.c[0]);
}

void FEListBox::SetCellScale(const float *xy)
{
    FEListBoxCell *c = CurrentCell();
    std::memcpy(&c->mfScaleX, xy, 8);
}

void FEListBox::SetCellJustification(uint32_t a, uint32_t b, uint32_t c)
{
    FEListBoxCell *cell = CurrentCell();
    cell->mJustification[0] = a;
    cell->mJustification[1] = b;
    cell->mJustification[2] = c;
}

void FEListBox::SetCellData(const uint32_t *d4)
{
    std::memcpy(CurrentCell()->mData, d4, 16);
}

void FEListBox::SetCellString(const uint16_t *text)
{
    FEListBoxCell *c = CurrentCell();
    if (c->mpText) {
        bridge::FreeArray(c->mpText);
        c->mpText = nullptr;
    }
    if (!text) return;
    uint32_t n = 0;
    while (text[n]) ++n;
    const uint32_t bytes = n * 2 + 2;
    auto *copy = static_cast<uint16_t *>(bridge::Alloc(bytes));
    std::memcpy(copy, text, bytes);
    c->mpText = copy;
}
