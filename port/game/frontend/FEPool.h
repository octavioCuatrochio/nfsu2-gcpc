// FEng per-class object pools (FEPoolNode<T, N>). Original: zFe2.cpp. Layouts shared with original game code (PC).
// A pool = FEMinList of blocks; a block = FEMinNode + N entries + free list + used count. Objects come from the
// free list; a block is created when none has a free entry and deleted (virtually) when its last entry returns.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
#include "bridge/Bridge.h"
#include "FEMessageResponse.h"
#include "FEObject.h"

template <typename T, uint32_t Count>
struct FEPoolBlock {
    FEMinNode node;                          // +0x0
    alignas(T) uint8_t entries[sizeof(T) * Count];   // +0xC (32-bit: T is exactly the PC entry size)
    FEMinList freeList;                      // +0xC + EntrySize*Count
    uint32_t used;                           // after the free list

    // Block constructor (PC 53E710/53E910): node guards + own vtable, entries constructed in order (MSVC
    // "eh vector constructor iterator"), empty free list, used = 0, then every entry appended to the free list.
    void Construct(void *blockVTable, void (*entryCtor)(void *))
    {
        node.vtable = blockVTable;
        node.next = node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
        for (uint32_t i = 0; i < Count; ++i) entryCtor(entries + i * sizeof(T));
        freeList = FEMinList{bridge::FEMinListVTable(), 0, nullptr, nullptr};
        used = 0;
        for (uint32_t i = 0; i < Count; ++i) freeList.AddTail(reinterpret_cast<FEMinNode *>(entries + i * sizeof(T)));
    }

    // Block destructor (PC 53EB40/53EC40): drains the free list (each removed head gets the unlinked guards; the
    // count drops even if head is null), ~FEMinList, entries destroyed last to first, vtable -> FEMinNode.
    void Destruct(void *blockVTable, void (*entryDtor)(void *))
    {
        node.vtable = blockVTable;
        auto *unlinked = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
        while (freeList.count != 0) {
            if (FEMinNode *h = freeList.head) {
                freeList.head = h->next;
                if (h == freeList.tail) freeList.tail = h->prev;
                if (h->prev) h->prev->next = h->next;
                if (h->next) h->next->prev = h->prev;
                h->next = h->prev = unlinked;
            }
            --freeList.count;
        }
        freeList.Destruct();
        for (uint32_t i = Count; i-- > 0;) entryDtor(entries + i * sizeof(T));
        node.vtable = bridge::FEMinNodeVTable();
    }

    bool Contains(const void *p) const
    {
        auto *b = static_cast<const uint8_t *>(p);
        return b >= entries && b < entries + sizeof entries;
    }
};

// operator new. `constructBlock` builds a fresh block in place (entries constructed, all on the free list).
// Like the original: out of memory -> null block -> crash.
template <typename T, uint32_t Count>
void *FEPoolNew(FEMinList &blocks, void (*constructBlock)(void *))
{
    using Block = FEPoolBlock<T, Count>;
    Block *blk = nullptr;
    for (FEMinNode *n = blocks.head; n; n = n->next) {
        auto *b = reinterpret_cast<Block *>(n);
        if (b->freeList.count != 0) { blk = b; break; }
    }
    if (!blk) {
        if (void *mem = bridge::Alloc(sizeof(Block))) {
            constructBlock(mem);
            blk = static_cast<Block *>(mem);
            blocks.AddNode(nullptr, &blk->node);
        }
    }
    FEMinNode *e = blk->freeList.head;
    if (e) blk->freeList.RemNode(e);
    ++blk->used;
    return e;
}

// operator delete: the entry goes back to its block's free list (tail); an emptied block is deleted.
template <typename T, uint32_t Count>
void FEPoolDelete(FEMinList &blocks, void *obj)
{
    using Block = FEPoolBlock<T, Count>;
    for (FEMinNode *n = blocks.head; n; n = n->next) {
        auto *b = reinterpret_cast<Block *>(n);
        if (!b->Contains(obj)) continue;
        b->freeList.AddTail(static_cast<FEMinNode *>(obj));
        if (--b->used == 0) {
            blocks.RemNode(&b->node);
            bridge::VirtualScalarDelete(b);
        }
        return;
    }
}

// The two instantiations in the game
using FEMessageResponseBlock = FEPoolBlock<FEMessageResponse, 64>;   // 0x620 bytes, pool list PC 0x8637A0
using FEScriptBlock = FEPoolBlock<FEScript, 32>;                     // 0x6A0 bytes, pool list PC 0x839C30
// Concrete block lifecycles. Ctor signature fits FEPoolNew's constructBlock.
void FEMessageResponseBlock_Construct(void *mem);                                       // PC 53E710
void FEMessageResponseBlock_Destruct(FEMessageResponseBlock *b);                        // PC 53EB40
FEMessageResponseBlock *FEMessageResponseBlock_ScalarDtor(FEMessageResponseBlock *b, uint32_t flags);   // PC 53EB20
void FEScriptBlock_Construct(void *mem);                                                // PC 53E910
void FEScriptBlock_Destruct(FEScriptBlock *b);                                          // PC 53EC40
FEScriptBlock *FEScriptBlock_ScalarDtor(FEScriptBlock *b, uint32_t flags);              // PC 53EC20

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEMessageResponseBlock) == 0x620 && offsetof(FEMessageResponseBlock, freeList) == 0x60C);
static_assert(offsetof(FEMessageResponseBlock, used) == 0x61C);
static_assert(sizeof(FEScriptBlock) == 0x6A0 && offsetof(FEScriptBlock, freeList) == 0x68C);
#endif
