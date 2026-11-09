// FEng intrusive doubly linked list. Original: zFe2.cpp. Layout shared with original game code (PC).
#pragma once
#include <cstddef>
#include <cstdint>

constexpr uintptr_t FEMINNODE_UNLINKED = 0xABADCAFE;   // value of next/prev on a node in no list

struct FEMinNode {
    void *vtable;                // +0x0  PC FEMinNode vtable 0x79A9E8 (subclasses replace it)
    FEMinNode *next;             // +0x4
    FEMinNode *prev;             // +0x8

    FEMinNode *ScalarDeletingDestructor(uint32_t flags);   // PC 4FDB00 (vtable slot 0)
};

struct FEMinList {
    void *vtable;                // +0x0  PC FEMinList vtable 0x79A9EC
    uint32_t count;              // +0x4
    FEMinNode *head;             // +0x8
    FEMinNode *tail;             // +0xC

    void AddNode(FEMinNode *after, FEMinNode *node);   // GC 801418B4 / PC 5053B0; after=null -> insert at head
    void RemNode(FEMinNode *node);                     // GC 80141920 / PC 505400
    void AddTail(FEMinNode *node) { AddNode(tail, node); }   // PC 4951B0
    FEMinNode *RemHead();                              // GC 8014199C (PC: inlined)
    FEMinNode *FindNode(uint32_t index) const;         // GC 801419D8 (PC: inlined); null if out of range
    void Purge();                                      // GC 8030C788 / PC 511670: deletes every node (virtual)
    void Destruct();                                   // ~FEMinList (PC: inlined into 52C290)
    FEMinList *VectorDeletingDestructor(uint32_t flags);   // PC 52C290 (vtable slot 0): bit1 array, bit0 free
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEMinNode) == 0xC && offsetof(FEMinNode, next) == 4 && offsetof(FEMinNode, prev) == 8);
static_assert(sizeof(FEMinList) == 0x10 && offsetof(FEMinList, head) == 8 && offsetof(FEMinList, tail) == 0xC);
#endif
