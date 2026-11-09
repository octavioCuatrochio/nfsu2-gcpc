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

struct FEngine;
struct FEObject;
struct FEPackage;

struct cFEng {
    uint8_t _unknown00[8];
    FEngine *mpEngine;               // +0x08 (GC +0x10)
    uint8_t _unknown0C[4];
    cFEngLevelSorter *mpSortHead;    // +0x10 (GC +0x18) list sentinel: {head, tail}; empty -> points at itself
    cFEngLevelSorter *mpSortTail;    // +0x14

    // GC 8014EC70 / PC 51CCD0 (__thiscall, ret 4): first entry whose name matches (bStrICmp); null if none.
    cFEngLevelSorter *FindInLevelSortingList(const char *name);
    // GC 8014D75C / PC 55DD50 (ret 0x10): 0x05922615 disables / 0x7E4D1288 enables FEng input (FEngine +1),
    // anything else goes to FEPackageManager::NotificationMessage on the global manager.
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, FEPackage *pkg);
    // GC 8014E848 / PC 51CBA0 (ret 8): loaded `from` gets message 0x5ECA113D, then a queued switch to `to`.
    void SwitchFEngPackages(const char *from, const char *to);
    // GC 8014E9A4 / PC 51CC00 (ret 4): loaded -> message 0x90CA113D + queued pop; else unqueue a pending push.
    void QueuePopPackage(const char *name);
    // GC 8014EAE8 / PC 51CC60 (ret 0xC; this unused): last button with that name hash; enable (== 1) clears
    // FEObject flag 0x04000000, anything else sets it.
    static void SetButtonState(const char *pkgName, uint32_t buttonHash, bool enable);
    cFEngLevelSorter *SortSentinel() { return reinterpret_cast<cFEngLevelSorter *>(&mpSortHead); }
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(cFEng, mpEngine) == 8 && offsetof(cFEng, mpSortHead) == 0x10 && offsetof(cFEngLevelSorter, mpcPackageName) == 8);
#endif
