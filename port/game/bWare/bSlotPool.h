// bWare slot pool (fixed-size block allocator, chained when expanded). Original: zBWare.cpp (GC "SlotPool").
// Layout identical on GC and PC for the fields used here; slots start at +0x30.
#pragma once
#include <cstddef>
#include <cstdint>

struct SlotPool {
    uint8_t _unknown00[8];
    SlotPool *mpNext;                // +0x08 expansion pool (same slot size)
    uint8_t _unknown0C[4];
    void *mpFreeList;                // +0x10 singly linked through the first word of each free slot
    uint8_t _unknown14[4];
    int32_t mi32NumAllocated;        // +0x18
    int32_t mi32MostAllocated;       // +0x1C high-water mark
    uint8_t _unknown20[4];
    int32_t mi32NumSlots;            // +0x24 slots in this pool
    uint32_t mu32SlotSize;           // +0x28
    int32_t mi32TotalSlots;          // +0x2C over the chain (bCountTotalSlots)
    // +0x30 slot data

    uint8_t *SlotData() { return reinterpret_cast<uint8_t *>(this) + 0x30; }
    // GC 8005C9E8 / PC 43CD40 (__thiscall; null pool is fine): rebuild every chained pool's free list in order.
    // Static with an explicit pool because the original accepts this == null.
    static void FlushSlotPool(SlotPool *pool);
    // GC 8005CAEC / PC 43CD80 (ret 4): index over the chain, -1 if not in any pool. Uses the FIRST pool's slot size
    // for every pool and an unsigned division (pointers below a pool give huge quotients -> not found there).
    static int32_t GetSlotNumber(SlotPool *pool, const void *slot);   // null pool -> -1
    void *FastMalloc();              // GC 8005CE20 / PC 43CDD0: pop the free list (null if empty), update counts
    void Free(void *slot);           // GC 8005CE58 / PC 43CDF0 (ret 4): push onto the free list
};

int32_t bIsSlotPoolFull(const SlotPool *p);     // GC 8005C908 / PC 43CD10: allocated == total
int32_t bCountFreeSlots(const SlotPool *p);     // GC 8005C920 / PC 43CD20: total - allocated
int32_t bCountTotalSlots(const SlotPool *p);    // GC 8005C930 / PC 43CD30

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(SlotPool, mpFreeList) == 0x10 && offsetof(SlotPool, mi32NumSlots) == 0x24 && sizeof(SlotPool) == 0x30);
#endif
