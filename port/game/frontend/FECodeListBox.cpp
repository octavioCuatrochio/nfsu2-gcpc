#include "FECodeListBox.h"
#include <cstring>
#include "bridge/Bridge.h"

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
    lb->_unknownC0 = lb->_unknownC4 = 0;
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
