#include "FECodeListBox.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "FEHash.h"

int32_t FEPositiveMod(int32_t v, int32_t n)
{
    if (v >= 0) return v % n;
    if (n <= 1) return 0;
    return n - (-v) % n;
}

FECodeListBox *FECodeListBox::Construct(void *mem)
{
    auto *lb = static_cast<FECodeListBox *>(mem);
    FEObject::Construct(mem);
    lb->mpGameInterface = nullptr;
    lb->mu32NumColumns = lb->mu32NumRows = 0;
    lb->mu32TotalColumns = lb->mu32TotalRows = 0;
    lb->mi32ColumnOffset = lb->mi32RowOffset = 0;
    lb->mu32SelColumn = lb->mu32SelRow = 0;
    lb->obj.node.vtable = bridge::FECodeListBoxVTable();
    lb->mu32Flags = 2;
    lb->mfViewWidth = lb->mfViewHeight = 0.0f;
    lb->mpCells = nullptr;
    lb->mu32NumStrings = lb->mu32StringLength = lb->mu32StringsUsed = 0;
    lb->mppStrings = nullptr;
    lb->mpStringBuffer = nullptr;
    lb->mfPulse = 1.0f;
    const uint32_t rate = 0xBAB60B61u;
    std::memcpy(&lb->mfPulseRate, &rate, 4);
    lb->mColor = FEColor{{0xFF, 0xFF, 0xFF, 0xFF}};
    lb->mpFillCallback = nullptr;
    lb->mpFillCallbackData = nullptr;
    lb->mpSelectCallback = reinterpret_cast<void (*)(FECodeListBox *)>(bridge::FECodeListBoxDefaultCallback());
    lb->obj.mu32Type = 6;
    return lb;
}

void FECodeListBox::Destruct()
{
    obj.node.vtable = bridge::FECodeListBoxVTable();
    if (mpCells) bridge::FreeArray(mpCells);
    if (mpStringBuffer) bridge::FreeArray(mpStringBuffer);
    if (mppStrings) bridge::FreeArray(mppStrings);
    obj.Destruct();
}

FECodeListBox *FECodeListBox::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

namespace {
void ReleaseText(FECodeListBox &lb, const FEListBoxCell &c)   // type-2 text back to the pool (no bounds check)
{
    if (c.mu32Type == 2) lb.mppStrings[--lb.mu32StringsUsed] = c.mpText;
}
}  // namespace

void FECodeListBox::Initialize(uint32_t columns, uint32_t rows)
{
    FEListBoxCell *old = mpCells;
    const uint32_t oldColumns = mu32NumColumns, oldRows = mu32NumRows;
    mu32NumColumns = columns;
    mu32NumRows = rows;
    const uint32_t n = columns * rows;
    auto *cells = static_cast<FEListBoxCell *>(bridge::Alloc(static_cast<uint32_t>(n * sizeof(FEListBoxCell))));
    if (cells)
        for (uint32_t i = 0; i < n; ++i) {   // cell ctor (inlined)
            FEListBoxCell &c = cells[i];
            c.mu32Color = 0;
            c.mfScaleX = c.mfScaleY = 1.0f;
            c.mJustification[0] = c.mJustification[1] = c.mJustification[2] = 0;
            c.mu32Type = 0;
            c.mData[1] = 0xFFFFFFFFu;   // +0x24 (before the pointer: it overlaps on 64-bit)
            c.mpText = nullptr;
        }
    mpCells = cells;
    FEListBox::InitializeCells(cells, mu32NumRows * mu32NumColumns);
    SetTotalNumColumns(mu32NumColumns);
    SetTotalNumRows(mu32NumRows);
    if (mu32Flags & 1) {
        const uint32_t keepColumns = columns < oldColumns ? columns : oldColumns;
        const uint32_t keepRows = rows < oldRows ? rows : oldRows;
        if (old) {
            for (uint32_t r = 0; r < keepRows; ++r) {
                std::memcpy(mpCells + r * mu32NumColumns, old + r * oldColumns, mu32NumColumns * sizeof(FEListBoxCell));
                for (uint32_t c = keepColumns; c < oldColumns; ++c) ReleaseText(*this, old[r * oldColumns + c]);
            }
            for (uint32_t r = keepRows; r < oldRows; ++r)
                for (uint32_t c = 0; c < oldColumns; ++c) ReleaseText(*this, old[r * oldColumns + c]);
            bridge::FreeArray(old);
        }
    }
    mu32Flags |= 1;
}

namespace {
// shared body of SetTotalNumColumns / SetTotalNumRows
int32_t CenteredOffset(uint32_t total, uint32_t sel, uint32_t visible, bool centered)
{
    if (sel >= total) sel = total ? total - 1 : 0;
    if (!centered) return static_cast<int32_t>(sel);   // PC: jumps past the xor (offset = clamped selection)
    const int32_t x = static_cast<int32_t>(sel) - static_cast<int32_t>(visible >> 1);
    if (x >= 0) return x % static_cast<int32_t>(total);   // PC: total 0 divides by zero
    if (static_cast<int32_t>(total) <= 1) return 0;
    return static_cast<int32_t>(total) - (-x) % static_cast<int32_t>(total);
}
}  // namespace

void FECodeListBox::SetTotalNumColumns(uint32_t n)
{
    mu32TotalColumns = n;
    mi32ColumnOffset = CenteredOffset(n, mu32SelColumn, mu32NumColumns, (mu32Flags & 8) != 0);
}

void FECodeListBox::SetTotalNumRows(uint32_t n)
{
    mu32TotalRows = n;
    mi32RowOffset = CenteredOffset(n, mu32SelRow, mu32NumRows, (mu32Flags & 8) != 0);
}

void FECodeListBox::AllocateStrings(uint32_t count, uint32_t length)
{
    uint16_t *oldBuffer = mpStringBuffer;
    uint16_t **oldTable = mppStrings;
    mu32NumStrings = 0;
    mu32StringsUsed = 0;
    mu32StringLength = 0;
    mpStringBuffer = nullptr;
    mppStrings = nullptr;
    if (count && length) {
        const uint32_t bytes = count * length * 2;
        mpStringBuffer = static_cast<uint16_t *>(bridge::Alloc(bytes));
        mppStrings = static_cast<uint16_t **>(bridge::Alloc(static_cast<uint32_t>(count * sizeof(uint16_t *))));
        std::memset(mpStringBuffer, 0, bytes);
        for (uint32_t i = 0; i < count; ++i) mppStrings[i] = mpStringBuffer + i * length;
        mu32NumStrings = count;
        mu32StringLength = length;
        if (oldBuffer && oldTable)
            for (uint32_t r = 0; r < mu32NumRows; ++r)
                for (uint32_t c = 0; c < mu32NumColumns; ++c) {
                    FEListBoxCell &cell = mpCells[r * mu32NumColumns + c];
                    if (cell.mu32Type != 2) continue;
                    uint16_t *s = mppStrings[mu32StringsUsed++];
                    const uint16_t *src = cell.mpText;
                    s[0] = 0;
                    cell.mpText = s;
                    uint16_t *d = s;
                    if (src)
                        for (uint32_t k = 0; *src && k != length - 1; ++k) *d++ = *src++;
                    *d = 0;
                }
    }
    if (oldBuffer) bridge::FreeArray(oldBuffer);
    if (oldTable) bridge::FreeArray(oldTable);
}

namespace {
int32_t RealIndex(int32_t v, uint32_t total, int32_t offset, uint32_t visible)
{
    const int32_t t = static_cast<int32_t>(total);
    if (t == 0) return -1;
    int32_t x = v >= t ? v % t : v;
    x -= offset;
    if (x < 0) x += t;
    return FEPositiveMod(x, static_cast<int32_t>(visible));
}
}  // namespace

int32_t FECodeListBox::GetRealColumn(int32_t column) const { return RealIndex(column, mu32TotalColumns, mi32ColumnOffset, mu32NumColumns); }
int32_t FECodeListBox::GetRealRow(int32_t row) const { return RealIndex(row, mu32TotalRows, mi32RowOffset, mu32NumRows); }

namespace {
template <typename F>
void ForCells(FECodeListBox &lb, uint32_t column0, uint32_t row0, uint32_t columns, uint32_t rows, F apply)
{
    for (uint32_t r = row0; r < row0 + rows; ++r)
        for (uint32_t c = column0; c < column0 + columns; ++c) {
            const int32_t rc = lb.GetRealColumn(static_cast<int32_t>(c)), rr = lb.GetRealRow(static_cast<int32_t>(r));
            apply(lb.mpCells[static_cast<int32_t>(lb.mu32NumColumns) * rr + rc]);   // PC: -1 indexes out of bounds
        }
}
}  // namespace

void FECodeListBox::SetCellColor(uint32_t column0, uint32_t row0, uint32_t color, uint32_t columns, uint32_t rows)
{
    ForCells(*this, column0, row0, columns, rows, [&](FEListBoxCell &c) { c.mu32Color = color; });
}

void FECodeListBox::SetCellScale(uint32_t column0, uint32_t row0, const float *xy, uint32_t columns, uint32_t rows)
{
    ForCells(*this, column0, row0, columns, rows, [&](FEListBoxCell &c) { std::memcpy(&c.mfScaleX, xy, 8); });
}

void FECodeListBox::SetCellJustification(uint32_t column0, uint32_t row0, uint32_t value, uint32_t columns, uint32_t rows)
{
    ForCells(*this, column0, row0, columns, rows, [&](FEListBoxCell &c) { c._unknown1C = value; });
}

void FECodeListBox::FillAllCells()
{
    if (!mu32TotalColumns || !mu32TotalRows || !mu32NumRows || !mu32NumColumns) return;
    const uint32_t rows = mu32NumRows > mu32TotalRows ? mu32TotalRows : mu32NumRows;
    const uint32_t columns = mu32NumColumns > mu32TotalColumns ? mu32TotalColumns : mu32NumColumns;
    int32_t row = mi32RowOffset;
    const int32_t column0 = mi32ColumnOffset;
    if (!mpFillCallback && !mpGameInterface) return;
    for (uint32_t r = 0; r < rows; ++r) {
        int32_t column = column0;
        for (uint32_t c = 0; c < columns; ++c) {
            if (mpFillCallback) mpFillCallback(mpFillCallbackData, this, column, row);
            else bridge::FEGameInterface_FillCodeListBoxCell(mpGameInterface, this, column, row);
            column = FEPositiveMod(column + 1, static_cast<int32_t>(mu32TotalColumns));   // re-read: the callback may resize
        }
        row = FEPositiveMod(row + 1, static_cast<int32_t>(mu32TotalRows));
    }
}

void FECodeListBox::Update(float dt)
{
    if (mpSelectCallback) mpSelectCallback(this);
    const long double v = static_cast<long double>(dt) * mfPulseRate + mfPulse;
    mfPulse = static_cast<float>(v);
    if (v < 0.0L) {
        mfPulse = 0.0f;
        mfPulseRate = -mfPulseRate;
    } else if (v > 1.0L) {   // NaN: no change (both tests false, like fcom + jp / jne)
        mfPulse = 1.0f;
        mfPulseRate = -mfPulseRate;
    }
}

namespace {
void NotifyGame(FECodeListBox &lb, const char *what)
{
    bridge::FEGameInterface_Message(lb.mpGameInterface, FEHashUpper(what), &lb, 0xFF, 0);
}
}  // namespace

bool FECodeListBox::CheckMovement(int32_t dir, int32_t offset, uint32_t sel, int32_t total, int32_t visible)
{
    const bool clamp = (mu32Flags & 4) != 0;
    if (clamp && total <= visible) {
        NotifyGame(*this, "CLB AT MIN");
        NotifyGame(*this, "CLB AT MAX");   // the original falls through to the MAX message too
        return false;
    }
    if (!(mu32Flags & 2)) return true;
    int32_t v, limit;
    if (clamp) { v = offset + dir; limit = total - visible; }
    else { v = static_cast<int32_t>(sel) + dir; limit = total; }
    if (v < 0) { NotifyGame(*this, "CLB AT MIN"); return false; }
    if (v >= limit) { NotifyGame(*this, "CLB AT MAX"); return false; }
    return true;
}

bool FECodeListBox::MakeMove(int32_t dir, int32_t *offset, uint32_t *sel, int32_t total, int32_t visible)
{
    if (mu32Flags & 8) {
        *offset = FEPositiveMod(*offset + dir, total);
        *sel = static_cast<uint32_t>(FEPositiveMod(static_cast<int32_t>(*sel) + dir, total));
        return true;
    }
    if ((mu32Flags & 4) && (mu32Flags & 2)) {
        const int32_t v = FEPositiveMod(*offset + dir, total);
        *offset = v;
        *sel = static_cast<uint32_t>(v);
        return true;
    }
    const uint32_t old = *sel;
    const uint32_t v = static_cast<uint32_t>(FEPositiveMod(static_cast<int32_t>(old) + dir, total));
    *sel = v;
    if (dir < 0) {
        if (static_cast<uint32_t>(*offset) != old) return false;
        *offset = static_cast<int32_t>(v);
        return true;
    }
    const uint32_t off = static_cast<uint32_t>(*offset);
    if (off == old) return false;
    const uint32_t distance = off < v ? v - off : v - off + static_cast<uint32_t>(total);
    if (distance < static_cast<uint32_t>(visible)) return false;
    *offset = FEPositiveMod(*offset + dir, total);
    return true;
}

bool FECodeListBox::ScrollSelection(int32_t dir, int32_t *offset, uint32_t *sel, uint32_t total, uint32_t visible, bool columns)
{
    if (dir == 0) return false;
    if (!CheckMovement(dir, *offset, *sel, static_cast<int32_t>(total), static_cast<int32_t>(visible))) return false;
    if (!MakeMove(dir, offset, sel, static_cast<int32_t>(total), static_cast<int32_t>(visible))) return false;
    if (total == 0) return true;
    const uint32_t C = mu32NumColumns, R = mu32NumRows;
    auto cell = [&](uint32_t r, uint32_t c) -> FEListBoxCell & { return mpCells[mu32NumColumns * r + c]; };
    auto fill = [&](int32_t column, int32_t row) {
        if (mpFillCallback) mpFillCallback(mpFillCallbackData, this, column, row);
        else bridge::FEGameInterface_FillCodeListBoxCell(mpGameInterface, this, column, row);
    };
    if (!mpFillCallback && !mpGameInterface) return true;
    auto rowOf = [&](uint32_t r) { return FEPositiveMod(mi32RowOffset + static_cast<int32_t>(r), static_cast<int32_t>(mu32TotalRows)); };
    auto colOf = [&](uint32_t c) { return FEPositiveMod(mi32ColumnOffset + static_cast<int32_t>(c), static_cast<int32_t>(mu32TotalColumns)); };
    if (columns) {
        if (dir > 0) {   // shift left, refill the last column
            const int32_t vc = FEPositiveMod(mi32ColumnOffset + static_cast<int32_t>(C) - 1, static_cast<int32_t>(mu32TotalColumns));
            for (uint32_t r = 0; r < mu32NumRows; ++r) {
                uint16_t *text = cell(r, 0).mpText;
                uint32_t k = 0;
                for (; k + 1 < C; ++k) std::memcpy(&cell(r, k), &cell(r, k + 1), sizeof(FEListBoxCell));
                cell(r, k).mpText = text;
                fill(vc, rowOf(r));
            }
        } else {         // shift right, refill the first column
            for (uint32_t r = 0; r < mu32NumRows; ++r) {
                uint16_t *text = cell(r, mu32NumColumns - 1).mpText;
                uint32_t k = mu32NumColumns - 1;
                for (; k != 0; --k) std::memcpy(&cell(r, k), &cell(r, k - 1), sizeof(FEListBoxCell));
                cell(r, 0).mpText = text;
                fill(mi32ColumnOffset, rowOf(r));
            }
        }
    } else {
        if (dir > 0) {   // shift up, refill the last row
            const int32_t vr = FEPositiveMod(mi32RowOffset + static_cast<int32_t>(R) - 1, static_cast<int32_t>(mu32TotalRows));
            for (uint32_t c = 0; c < mu32NumColumns; ++c) {
                uint16_t *text = cell(0, c).mpText;
                uint32_t k = 0;
                for (; k + 1 < R; ++k) std::memcpy(&cell(k, c), &cell(k + 1, c), sizeof(FEListBoxCell));
                cell(k, c).mpText = text;
                fill(colOf(c), vr);
            }
        } else {         // shift down, refill the first row
            for (uint32_t c = 0; c < mu32NumColumns; ++c) {
                uint32_t k = mu32NumRows - 1;
                uint16_t *text = cell(k, c).mpText;
                for (; k != 0; --k) std::memcpy(&cell(k, c), &cell(k - 1, c), sizeof(FEListBoxCell));
                cell(0, c).mpText = text;
                fill(mi32ColumnOffset, rowOf(0));   // sic: column offset for every column
            }
        }
    }
    return true;
}

bool FECodeListBox::ScrollSelection(int32_t dx, int32_t dy)
{
    ScrollSelection(dx, &mi32ColumnOffset, &mu32SelColumn, mu32TotalColumns, mu32NumColumns, true);
    return ScrollSelection(dy, &mi32RowOffset, &mu32SelRow, mu32TotalRows, mu32NumRows, false);
}

void FECodeListBox::DefaultSelectCallback(FECodeListBox *lb)
{
    const long double a = static_cast<long double>(lb->mfPulse) * 255.0L;   // x87, then _ftol (truncate)
    lb->mColor.c[3] = static_cast<int32_t>(a);
}
