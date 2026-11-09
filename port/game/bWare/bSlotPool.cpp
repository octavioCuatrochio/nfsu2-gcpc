#include "bSlotPool.h"
#include "bridge/Bridge.h"

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
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(slot) - reinterpret_cast<uintptr_t>(p) - sizeof(SlotPool)) / size);
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

void *SlotPool::Malloc()
{
    if (!mpFreeList && (mu32Flags & 1)) ExpandSlotPool(mi32NumSlots / 4 + 1);
    void *slot = FastMalloc();
    if (slot && (mu32Flags & 2)) {
        uint32_t *w = static_cast<uint32_t *>(slot);
        for (int32_t i = static_cast<int32_t>(mu32SlotSize) >> 2; i > 0; --i) *w++ = 0;
    }
    return slot;
}

void SlotPool::ExpandSlotPool(int32_t n)
{
    SlotPool *np = NewSlotPool(mu32SlotSize, n, mpcName, mu32MemFlags);
    np->mu32Flags = mu32Flags;
    SlotPool *last = this;
    while (last->mpNext) last = last->mpNext;
    last->mpNext = np;
    // the new pool's last slot (word index (n-1)*size/4, signed) links to our current free list
    const int32_t word = static_cast<int32_t>((n - 1) * static_cast<int32_t>(mu32SlotSize)) / 4;
    reinterpret_cast<void **>(np->SlotData())[word] = mpFreeList;
    mpFreeList = np->mpFreeList;
    np->mpFreeList = nullptr;
    mi32TotalSlots += n;
}

SlotPool *SlotPool::NewSlotPool(uint32_t slotSize, int32_t numSlots, const char *name, uint32_t memFlags)
{
    auto *p = static_cast<SlotPool *>(bridge::bWareMalloc(slotSize * static_cast<uint32_t>(numSlots) + static_cast<uint32_t>(sizeof(SlotPool)), nullptr, 0, memFlags));
    if (!p) return nullptr;
    p->mpcName = name;
    p->mi32NumSlots = numSlots;
    p->mu32SlotSize = slotSize;
    p->mu32Flags = 0xB;
    p->mpFreeList = nullptr;
    p->mpNext = nullptr;
    p->mu32MemFlags = memFlags;
    p->mi32TotalSlots = numSlots;
    p->mi32NumAllocated = 0;
    p->mi32MostAllocated = 0;
    FlushSlotPool(p);
    return p;
}
