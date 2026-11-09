// Game-side frontend manager (cFEng). Original: zFe2.cpp. Only the fields the rewritten members use (PC offsets).
#pragma once
#include <cstddef>
#include <cstdint>

// Level sorter entry (GC: bMalloc'd from LevelSorterSlots; linked in a circular bTList).
struct cFEngLevelSorter {
    cFEngLevelSorter *next;          // +0x00
    cFEngLevelSorter *prev;          // +0x04
    const char *mpcPackageName;      // +0x08
    // +0x0C.. more fields (GC: +0x10 state, +0x18 flags) not needed yet
};

struct cFEng {
    uint8_t _unknown00[0x10];
    cFEngLevelSorter *mpSortHead;    // +0x10 (GC +0x18) list sentinel: {head, tail}; empty -> points at itself
    cFEngLevelSorter *mpSortTail;    // +0x14

    // GC 8014EC70 / PC 51CCD0 (__thiscall, ret 4): first entry whose name matches (bStrICmp); null if none.
    cFEngLevelSorter *FindInLevelSortingList(const char *name);
    cFEngLevelSorter *SortSentinel() { return reinterpret_cast<cFEngLevelSorter *>(&mpSortHead); }
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(cFEng, mpSortHead) == 0x10 && offsetof(cFEngLevelSorter, mpcPackageName) == 8);
#endif
