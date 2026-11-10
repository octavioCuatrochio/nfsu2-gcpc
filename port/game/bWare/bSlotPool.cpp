#include "bSlotPool.h"
#include <cstring>
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
    *reinterpret_cast<void **>(np->SlotData() + word * 4) = mpFreeList;   // PC: dword index word
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

void *SlotPool::GetSlot(int32_t index)
{
    for (SlotPool *p = this; p; p = p->mpNext) {
        if (index >= 0 && index < p->mi32NumSlots) return p->SlotData() + static_cast<uint32_t>(index) * mu32SlotSize;
        index -= p->mi32NumSlots;
    }
    return nullptr;
}

void *SlotPool::GetAllocatedSlot(int32_t n)
{
    auto *map = static_cast<int8_t *>(bridge::bWareMalloc(static_cast<uint32_t>(mi32TotalSlots), nullptr, 0, 0));
    std::memset(map, 1, static_cast<uint32_t>(mi32TotalSlots));
    for (void *f = mpFreeList; f; f = *static_cast<void **>(f)) map[GetSlotNumber(this, f)] = 0;
    void *result = nullptr;
    const int32_t total = mi32TotalSlots;
    for (int32_t i = 0; i < total; ++i) {
        if (!map[i]) continue;
        if (n == 0) { result = GetSlot(i); break; }
        --n;
    }
    bridge::bWareFree(map);
    return result;
}

void SlotPoolManager::DeleteSlotPool(SlotPool *pool)
{
    if (!pool) return;
    if (mi32Enabled != 0) {
        SlotPool *next = pool->mpListNext, *prev = pool->mpListPrev;
        prev->mpListNext = next;
        next->mpListPrev = prev;
    }
    if ((pool->mu32Flags & 8) && pool->mi32NumAllocated > 0) pool->GetAllocatedSlot(0);   // result unused (PC too)
    while (pool) {
        SlotPool *next = pool->mpNext;
        bridge::bWareFree(pool);
        pool = next;
    }
}

SlotPool *SlotPoolManager::NewSlotPool(int32_t slotSize, int32_t numSlots, const char *name, uint32_t memFlags)
{
    SlotPool *p = SlotPool::NewSlotPool(static_cast<uint32_t>(slotSize + 3) & ~3u, numSlots, name, memFlags);
    if (!p) return nullptr;
    if (mi32Enabled == 1) {   // AddTail
        SlotPool *tail = mpTail;
        tail->mpListNext = p;
        mpTail = p;
        p->mpListPrev = tail;
        p->mpListNext = Sentinel();
    }
    return p;
}

SlotPool *bNewSlotPool(int32_t slotSize, int32_t numSlots, const char *name, uint32_t memFlags)
{
    return bridge::TheSlotPoolManager()->NewSlotPool(slotSize, numSlots, name, memFlags);
}

void SlotPoolManager::Construct()
{
    mpHead = mpTail = Sentinel();
    mi32Enabled = 1;
}

void SlotPoolManager::Destruct()
{
    mi32Enabled = 0;
    mpHead = mpTail = Sentinel();
    while (mpHead != Sentinel()) {   // never runs (list just emptied), like the original
        SlotPool *p = mpHead;
        p->mpListPrev->mpListNext = p->mpListNext;
        p->mpListNext->mpListPrev = p->mpListPrev;
        bridge::FreeArray(p);
    }
}

void SlotPoolManager::CleanupExpandedSlotPools()
{
    for (SlotPool *p = mpHead; p != Sentinel(); p = p->mpListNext) p->CleanupExpandedSlotPools();
}

void SlotPool::DeleteSlotPool(SlotPool *pool)
{
    if (pool) bridge::bWareFree(pool);
}

void SlotPool::CleanupExpandedSlotPools()
{
    if (!mpNext) return;
    for (SlotPool *p = mpNext; p; p = p->mpNext) p->mi32NumAllocated = p->mi32NumSlots;
    for (void *f = mpFreeList; f; f = *static_cast<void **>(f)) {
        for (SlotPool *p = mpNext; p; p = p->mpNext) {
            const int32_t i = GetSlotNumber(p, f);
            if (i >= 0 && i < p->mi32NumSlots) {
                --p->mi32NumAllocated;
                break;
            }
        }
    }
    SlotPool *prev = this;
    for (SlotPool *p = mpNext; p;) {
        SlotPool *next = p->mpNext;
        if (p->mi32NumAllocated == 0) {
            void **link = &mpFreeList;
            for (void *f = *link; f;) {
                void *fn = *static_cast<void **>(f);
                const int32_t i = GetSlotNumber(p, f);
                if (i >= 0 && i < p->mi32NumSlots) *link = fn;
                else link = static_cast<void **>(f);
                f = fn;
            }
            mi32TotalSlots -= p->mi32NumSlots;
            prev->mpNext = p->mpNext;
            DeleteSlotPool(p);
        } else {
            prev = p;
        }
        p = next;
    }
}

void bDeleteSlotPool(SlotPool *pool) { bridge::TheSlotPoolManager()->DeleteSlotPool(pool); }
void *bMalloc(SlotPool *pool) { return pool->Malloc(); }
void *bOMalloc(SlotPool *pool) { return pool->FastMalloc(); }
void bFree(SlotPool *pool, void *slot) { pool->Free(slot); }

void bPListInit(int32_t numSlots)
{
    SlotPool **pool = bridge::BPNodeSlotPool();
    if (*pool) return;
    *pool = bNewSlotPool(static_cast<int32_t>(3 * sizeof(void *)), numSlots, "bPNode SlotPool", 0);   // PC 0xC (next, prev, data)
    *bridge::BPListWantToClose() = 0;
}

void bPListClose()
{
    SlotPool **pool = bridge::BPNodeSlotPool();
    if (!*pool) return;
    if ((*pool)->mi32NumAllocated == 0) {
        bridge::TheSlotPoolManager()->DeleteSlotPool(*pool);
        *pool = nullptr;
    } else {
        *bridge::BPListWantToClose() = 1;
    }
}

void *bPNode_Malloc()
{
    if (!*bridge::BPNodeSlotPool()) bPListInit(0x100);
    return (*bridge::BPNodeSlotPool())->Malloc();
}

void bPNode_Free(void *node)
{
    (*bridge::BPNodeSlotPool())->Free(node);
    if (*bridge::BPListWantToClose()) bPListClose();
}
