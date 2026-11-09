// bWare slot pool (fixed-size block allocator, chained when expanded). Original: zBWare.cpp (GC "SlotPool").
// Layout identical on GC and PC for the fields used here; slots start at +0x30.
#pragma once
#include <cstddef>
#include <cstdint>

struct SlotPool {
    SlotPool *mpListNext;            // +0x00 SlotPoolManager list (bTList node; sentinel = manager +4)
    SlotPool *mpListPrev;            // +0x04
    SlotPool *mpNext;                // +0x08 expansion pool (same slot size)
    const char *mpcName;             // +0x0C debug name
    void *mpFreeList;                // +0x10 singly linked through the first word of each free slot
    uint32_t mu32Flags;              // +0x14 1: expand when empty (Malloc), 2: zero slots on Malloc; NewSlotPool sets 0xB
    int32_t mi32NumAllocated;        // +0x18
    int32_t mi32MostAllocated;       // +0x1C high-water mark
    uint32_t mu32MemFlags;           // +0x20 bWareMalloc allocation flags
    int32_t mi32NumSlots;            // +0x24 slots in this pool
    uint32_t mu32SlotSize;           // +0x28
    int32_t mi32TotalSlots;          // +0x2C over the chain (bCountTotalSlots)
    // +0x30 slot data

    // slots follow the header (+0x30 on PC; larger header on 64-bit builds)
    uint8_t *SlotData() { return reinterpret_cast<uint8_t *>(this) + sizeof(SlotPool); }
    // GC 8005C9E8 / PC 43CD40 (__thiscall; null pool is fine): rebuild every chained pool's free list in order.
    // Static with an explicit pool because the original accepts this == null.
    static void FlushSlotPool(SlotPool *pool);
    // GC 8005CAEC / PC 43CD80 (ret 4): index over the chain, -1 if not in any pool. Uses the FIRST pool's slot size
    // for every pool and an unsigned division (pointers below a pool give huge quotients -> not found there).
    static int32_t GetSlotNumber(SlotPool *pool, const void *slot);   // null pool -> -1
    void *FastMalloc();              // GC 8005CE20 / PC 43CDD0: pop the free list (null if empty), update counts
    void Free(void *slot);           // GC 8005CE58 / PC 43CDF0 (ret 4): push onto the free list
    // GC 8005CD80 / PC 440D40: FastMalloc, expanding by numSlots/4 + 1 first when empty and flag 1; flag 2 zeroes
    // (slotSize >> 2) words of the slot.
    void *Malloc();
    // GC 8005CB3C (PC: inlined): slot `index` over the chain (first pool's slot size), null if out of range.
    void *GetSlot(int32_t index);
    // GC 8005CB84 / PC 440C30 (ret 4): the n-th allocated slot (in slot order), null if fewer. Builds a byte map
    // of size mi32TotalSlots with bWareMalloc; a free-list entry outside every pool marks map[-1] (as the original).
    void *GetAllocatedSlot(int32_t n);
    // GC 8005CA44 / PC 440BB0 (ret 4): append a pool of n slots (same size/name/flags) to the chain and splice its
    // free list in front of ours (the new pool's last slot points at our old head). No allocation-failure check.
    void ExpandSlotPool(int32_t n);
    // GC 8005C938 / PC 440B40 (__cdecl): one block (0x30 + size * n) from bWareMalloc; null if that fails.
    static SlotPool *NewSlotPool(uint32_t slotSize, int32_t numSlots, const char *name, uint32_t memFlags);
};

// Global pool registry (PC TheSlotPoolManager @ 0x828530, game data via bridge).
struct SlotPoolManager {
    int32_t mi32Enabled;             // +0x0 pools are linked only when == 1 (unlinked when != 0)
    SlotPool *mpHead;                // +0x4 list sentinel {head, tail}
    SlotPool *mpTail;                // +0x8
    SlotPool *Sentinel() { return reinterpret_cast<SlotPool *>(&mpHead); }
    // GC 8005CF8C / PC 440DA0 (ret 4): unlink, (flag 8 + allocations: GetAllocatedSlot(0), a leftover leak report),
    // then free every pool of the chain with bWareFree. Null is fine.
    void DeleteSlotPool(SlotPool *pool);
};

// GC 8005C83C / PC 440DF0: slot size rounded up to 4; registered with the manager when enabled.
SlotPool *bNewSlotPool(int32_t slotSize, int32_t numSlots, const char *name, uint32_t memFlags);
void bDeleteSlotPool(SlotPool *pool);           // GC 8005C87C / PC 440E40
void *bMalloc(SlotPool *pool);                  // GC 8005C8A8 (and bOMalloc 8005C8C8, folded) / PC 440E50
int32_t bIsSlotPoolFull(const SlotPool *p);     // GC 8005C908 / PC 43CD10: allocated == total
int32_t bCountFreeSlots(const SlotPool *p);     // GC 8005C920 / PC 43CD20: total - allocated
int32_t bCountTotalSlots(const SlotPool *p);    // GC 8005C930 / PC 43CD30

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(SlotPool, mpFreeList) == 0x10 && offsetof(SlotPool, mi32NumSlots) == 0x24 && sizeof(SlotPool) == 0x30);
#endif
