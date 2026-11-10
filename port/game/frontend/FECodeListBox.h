// FEng code list box (FEObject type 6, PC 0xC8 bytes): a window of visible cells over a larger virtual grid that
// game code fills through a callback. Original: zFe2.cpp (FECodeListBox). PC layout.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEListBox.h"
#include "FEMath.h"

struct FECodeListBox {
    FEObject obj;                    // +0x00 vtable PC 0x79B904
    void *mpGameInterface;           // +0x5C FEPackageReader::CreateObject: the engine's game interface
    uint32_t mu32NumColumns;         // +0x60 visible columns
    uint32_t mu32NumRows;            // +0x64 visible rows
    uint32_t mu32Flags;              // +0x68 1 initialized (Initialize keeps contents), 2 (ctor), 8 centered selection
    uint32_t mu32TotalColumns;       // +0x6C virtual grid size (0: none)
    uint32_t mu32TotalRows;          // +0x70
    int32_t mi32ColumnOffset;        // +0x74 virtual column shown in visible column 0
    int32_t mi32RowOffset;           // +0x78
    uint32_t mu32SelColumn;          // +0x7C virtual selection
    uint32_t mu32SelRow;             // +0x80
    float mfViewWidth, mfViewHeight; // +0x84 ("Lv")
    FEListBoxCell *mpCells;          // +0x8C visible cells, row-major
    uint32_t mu32NumStrings;         // +0x90 string pool (AllocateStrings)
    uint32_t mu32StringLength;       // +0x94 characters per string, terminator included
    uint32_t mu32StringsUsed;        // +0x98
    uint16_t **mppStrings;           // +0x9C pool slots
    uint16_t *mpStringBuffer;        // +0xA0
    float mfPulse;                   // +0xA4 1.0
    float mfPulseRate;               // +0xA8 -1/720
    FEColor mColor;                  // +0xAC 0xFF x4
    void (*mpSelectCallback)(FECodeListBox *);   // +0xBC __cdecl, ctor: FECodeListBox::mpDefaultCallback (PC *0x7F7CC4)
    void (*mpFillCallback)(void *data, FECodeListBox *lb, int32_t column, int32_t row);   // +0xC0 __cdecl
    void *mpFillCallbackData;        // +0xC4

    static FECodeListBox *Construct(void *mem);   // GC 8013E120 / PC 52B9E0
    void Destruct();                               // GC 8013E4FC / PC 52BAA0
    FECodeListBox *ScalarDeletingDestructor(uint32_t flags);   // PC 5352B0 (vtable slot 0)
    // GC 8013E570 / PC 52BB30 (ret 8): new visible grid; when already initialized (flag 1) the old rows are copied
    // (each new row takes `columns` cells from the old row start, even past its end) and the text of dropped cells
    // goes back to the string pool.
    void Initialize(uint32_t columns, uint32_t rows);
    // GC 8013EA24 / 8013EA68, PC 518BB0 / 518C00 (ret 4): set the virtual size; the offset is the (clamped) selection, or with
    // flag 8 the offset that centers it.
    void SetTotalNumColumns(uint32_t n);
    void SetTotalNumRows(uint32_t n);
    // GC 8013EAAC / PC 518C60 (ret 8): new pool of count strings of `length` chars; existing type-2 cells move
    // their text into new slots (truncated to length - 1).
    void AllocateStrings(uint32_t count, uint32_t length);
    // GC 8013EE28 / 8013EEA0 (PC: inlined): virtual -> visible index; -1 without a virtual size.
    int32_t GetRealColumn(int32_t column) const;
    int32_t GetRealRow(int32_t row) const;
    // PC 5196B0 / 5197D0 / 5198F0 (ret 0x14; GC SetCellColor/Scale/Justification 8013FBD0..): set a field in
    // every cell of the virtual rectangle (column0, row0, columns, rows).
    void SetCellColor(uint32_t column0, uint32_t row0, uint32_t color, uint32_t columns, uint32_t rows);
    void SetCellScale(uint32_t column0, uint32_t row0, const float *xy, uint32_t columns, uint32_t rows);
    void SetCellJustification(uint32_t column0, uint32_t row0, uint32_t value, uint32_t columns, uint32_t rows);   // cell +0x1C
    // GC 8013E7DC / PC 504C40: for each visible cell (min(visible, total) per axis, starting at the offsets, wrapping
    // with FEPositiveMod) ask the fill callback, else the game interface (vtable +0x4C), with the virtual column/row.
    void FillAllCells();
    // GC 8013ECF0 / PC 504DC0 (ret 4): select callback, then the pulse (x87: tested unrounded, stored rounded;
    // below 0 -> 0, above 1 -> 1, the rate flips at both ends).
    void Update(float dt);
    // GC 8013EF18 / PC 518E70 (ret 0x14): may the selection move by dir? Flag 4 (clamp) with everything visible:
    // "CLB AT MIN" and "CLB AT MAX" to the game, false. Flag 2: past either end -> that message, false.
    bool CheckMovement(int32_t dir, int32_t offset, uint32_t sel, int32_t total, int32_t visible);
    // GC 8013F03C / PC 504E40 (ret 0x14): move selection/offset; true when the visible window shifted.
    bool MakeMove(int32_t dir, int32_t *offset, uint32_t *sel, int32_t total, int32_t visible);
    // GC 8013F1EC / PC 518F20 (ret 0x18): one axis; on a window shift the visible cells move one step (the text
    // pointers rotate) and the new edge line is refilled through the fill callback or the game interface. Like the
    // original, a backward row shift asks every new cell with the column offset itself (not offset + column).
    bool ScrollSelection(int32_t dir, int32_t *offset, uint32_t *sel, uint32_t total, uint32_t visible, bool columns);
    // GC 8013EC8C / PC 52BD70 (ret 8): columns by dx, then rows by dy; returns the row result.
    bool ScrollSelection(int32_t dx, int32_t dy);
    // GC 8013ED74 / PC 518E20 (__cdecl, mpDefaultCallback): color alpha = _ftol(pulse * 255).
    static void DefaultSelectCallback(FECodeListBox *lb);
};

// PC 504790 (__cdecl): modulo that is positive for negative values (n - (-v % n), so a multiple gives n; n <= 1 -> 0).
int32_t FEPositiveMod(int32_t v, int32_t n);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FECodeListBox, mu32Flags) == 0x68 && offsetof(FECodeListBox, mpCells) == 0x8C);
static_assert(offsetof(FECodeListBox, mpStringBuffer) == 0xA0 && offsetof(FECodeListBox, mColor) == 0xAC);
static_assert(offsetof(FECodeListBox, mpSelectCallback) == 0xBC && sizeof(FECodeListBox) == 0xC8);
#endif
