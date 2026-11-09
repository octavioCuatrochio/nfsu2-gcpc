#include "bSlotPool.h"

void SlotPool::FlushSlotPool(SlotPool *pool)
{
    for (SlotPool *p = pool; p; p = p->mpNext) {
        const int32_t n = p->mi32NumSlots;
        const uint32_t size = p->mu32SlotSize;
        p->mpFreeList = nullptr;
        if (n == 0) continue;
        uint8_t *slot = p->SlotData();
        p->mpFreeList = slot;
        for (int32_t i = n - 1; i > 0; --i) {
            uint8_t *next = slot + size;
            *reinterpret_cast<void **>(slot) = next;
            slot = next;
        }
        *reinterpret_cast<void **>(slot) = nullptr;
    }
}

int32_t SlotPool::GetSlotNumber(SlotPool *pool, const void *slot)
{
    if (!pool) return -1;
    const uint32_t size = pool->mu32SlotSize;
    int32_t base = 0;
    for (SlotPool *p = pool; p; p = p->mpNext) {
        const int32_t idx = static_cast<int32_t>(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(slot) - reinterpret_cast<uintptr_t>(p) - 0x30) / size);
        if (idx >= 0 && idx < p->mi32NumSlots) return idx + base;
        base += p->mi32NumSlots;
    }
    return -1;
}

void *SlotPool::FastMalloc()
{
    const int32_t n = mi32NumAllocated + 1;
    void *slot = mpFreeList;
    if (slot) {
        void *next = *static_cast<void **>(slot);
        if (n > mi32MostAllocated) mi32MostAllocated = n;
        mpFreeList = next;
        mi32NumAllocated = n;
    }
    return slot;
}

void SlotPool::Free(void *slot)
{
    *static_cast<void **>(slot) = mpFreeList;
    --mi32NumAllocated;
    mpFreeList = slot;
}

int32_t bIsSlotPoolFull(const SlotPool *p) { return p->mi32NumAllocated == p->mi32TotalSlots; }
int32_t bCountFreeSlots(const SlotPool *p) { return p->mi32TotalSlots - p->mi32NumAllocated; }
int32_t bCountTotalSlots(const SlotPool *p) { return p->mi32TotalSlots; }
