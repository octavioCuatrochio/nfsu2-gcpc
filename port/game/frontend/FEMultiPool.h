// FEng small-block allocator. Original: zFe2.cpp. Layouts shared with original game code (PC).
// FEMultiPool = list of FESlotPools (one per block size); FESlotPool = list of FESlotNodes; FESlotNode = 32 blocks.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"

struct FESlotNode {
    FEMinNode node;              // +0x0  vtable PC 0x79B13C
    uint16_t mu16BlockSize;      // +0xC
    uint16_t mu16Used;           // +0xE  blocks in use (32 = full)
    uint8_t mau8Mask[4];         // +0x10 bit i (byte i/8, bit i%8) = block i in use
    uint8_t *mpBuffer;           // +0x14 32 * block size bytes (game heap)

    static FESlotNode *Construct(void *mem, uint16_t blockSize);   // PC 517310
    void Destruct();                                                // GC 8030D5DC / PC 5173B0
    FESlotNode *ScalarDeletingDestructor(uint32_t flags);           // PC 517390
    uint8_t *AllocBlock();                                          // GC 80136BB4 / PC 504400; null if full
    void FreeBlock(const uint8_t *block);                           // GC 80136C60 (PC: inlined in FESlotPool::Free)
};

struct FESlotPool {
    FEMinNode node;              // +0x0  vtable PC 0x79B8DC
    FEMinList mNodes;            // +0xC  FESlotNodes
    uint32_t mu32BlockSize;      // +0x1C

    uint8_t *Alloc();                                   // GC 80136CA0 / PC 517400
    bool Free(const uint8_t *block);                    // GC 80136D78 / PC 5174A0; false if not in this pool
    void Destruct();                                    // GC 8030D640 / PC 534AC0
    FESlotPool *ScalarDeletingDestructor(uint32_t flags);   // PC 534AA0
};

struct FEMultiPool {
    FEMinList mPools;            // +0x0  FESlotPools

    void *Alloc(uint32_t size);                         // GC 80136E2C / PC 52ACE0; size 0 -> null
    void Free(void *block);                             // GC 80136EFC / PC 517520
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FESlotNode) == 0x18 && offsetof(FESlotNode, mau8Mask) == 0x10 && offsetof(FESlotNode, mpBuffer) == 0x14);
static_assert(sizeof(FESlotPool) == 0x20 && offsetof(FESlotPool, mNodes) == 0xC && offsetof(FESlotPool, mu32BlockSize) == 0x1C);
static_assert(sizeof(FEMultiPool) == 0x10);
#endif
