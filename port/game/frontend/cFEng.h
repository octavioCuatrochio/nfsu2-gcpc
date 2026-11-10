// Game-side frontend manager (cFEng). Original: zFe2.cpp. Only the fields the rewritten members use (PC offsets).
#pragma once
#include <cstddef>
#include <cstdint>

// Level sorter entry (GC: bMalloc'd from LevelSorterSlots; linked in a circular bTList).
struct cFEngLevelSorter {
    cFEngLevelSorter *next;          // +0x00
    cFEngLevelSorter *prev;          // +0x04
    const char *mpcPackageName;      // +0x08
    int32_t mi32Level;               // +0x0C position in the list (ResyncSortingLevels)
    int32_t mi32State;               // +0x10 0/1: pop directly (Popper), else queue the pop
    int32_t mi32Arg;                 // +0x14 (Switcher's third argument)
    uint32_t mu32Flags;              // +0x18 (GC: 8 checked by Pusher)
};

struct FEngine;
struct FEObject;
struct FEPackage;

struct cFEng {
    uint8_t _unknown00[8];
    FEngine *mpEngine;               // +0x08 (GC +0x10)
    void *mpRender;                  // +0x0C cFEngRender (GC +0x14)
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
    // GC 8014ECD8 / PC 5376B0: number the entries in order and copy the number to each loaded package's priority.
    void ResyncSortingLevels();
    // GC 8014EF3C / PC 5376F0 (ret 4): remove the entry (pop now when state is 0/1, else queue) and renumber.
    void Popper(const char *name);
    // GC 8014F044 / PC 5377A0 (ret 0xC): switch packages and replace the entry for `from` with a new one for `to`
    // (no null checks, like the original).
    void Switcher(const char *from, const char *to, int32_t arg);
    // GC 8014E8FC / PC 537580 (ret 4): loaded package: message 0xD9CA113D, disconnect every object from the
    // renderer (RenderObjectDisconnect callback, game vtable), then FEngine::UnloadPackage (bridge).
    void PopPackage(const char *name);
    // GC 8014EFD0 / PC 537750 (ret 4): PopPackage and drop the sorter entry (no renumbering).
    void ForceUnloadPackage(const char *name);
    static void SetButtonState(const char *pkgName, uint32_t buttonHash, bool enable);
    // GC 8014E798 / 8014E7F0, PC 51CB20 / 51CB60 (ret 0xC): parent looked up by name (null name -> no parent).
    void QueuePushPackage(const char *name, const char *parent, uint32_t mask);
    void QueuePushModalPackage(const char *name, const char *parent, uint32_t mask);
    cFEngLevelSorter *SortSentinel() { return reinterpret_cast<cFEngLevelSorter *>(&mpSortHead); }
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(cFEng, mpEngine) == 8 && offsetof(cFEng, mpSortHead) == 0x10 && offsetof(cFEngLevelSorter, mpcPackageName) == 8);
#endif
