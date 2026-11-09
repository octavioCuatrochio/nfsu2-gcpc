#include "FEMultiPool.h"
#include "bridge/Bridge.h"
#include <new>

namespace {
FEMinNode *const kUnlinked = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
}

// ---------------------------------------------------------------- FESlotNode
FESlotNode *FESlotNode::Construct(void *mem, uint16_t blockSize)
{
    auto *n = static_cast<FESlotNode *>(mem);
    n->node.next = n->node.prev = kUnlinked;
    n->mu16BlockSize = blockSize;
    n->node.vtable = bridge::FESlotNodeVTable();
    n->mu16Used = 0;
    n->mpBuffer = nullptr;
    n->mpBuffer = static_cast<uint8_t *>(bridge::Alloc(static_cast<uint32_t>(blockSize) << 5));
    for (auto &b : n->mau8Mask) b = 0;
    return n;
}

void FESlotNode::Destruct()
{
    node.vtable = bridge::FESlotNodeVTable();
    bridge::FreeArray(mpBuffer);
    node.vtable = bridge::FEMinNodeVTable();
}

FESlotNode *FESlotNode::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

// First free block: skip mask bytes that are 0xFF, then scan bits upward from that byte.
uint8_t *FESlotNode::AllocBlock()
{
    if (mu16Used == 32) return nullptr;
    uint32_t byte = 0;
    while (mau8Mask[byte] == 0xFF) ++byte;
    uint32_t bit = byte * 8;
    while (mau8Mask[bit >> 3] & (1u << (bit & 7))) ++bit;
    ++mu16Used;
    mau8Mask[bit >> 3] |= static_cast<uint8_t>(1u << (bit & 7));
    return mpBuffer + static_cast<uint32_t>(mu16BlockSize) * bit;
}

void FESlotNode::FreeBlock(const uint8_t *block)
{
    uint32_t i = static_cast<uint32_t>(block - mpBuffer) / mu16BlockSize;
    --mu16Used;
    mau8Mask[i >> 3] &= static_cast<uint8_t>(~(1u << (i & 7)));
}

// ---------------------------------------------------------------- FESlotPool
uint8_t *FESlotPool::Alloc()
{
    FESlotNode *n = nullptr;
    for (FEMinNode *p = mNodes.head; p; p = p->next) {
        auto *s = reinterpret_cast<FESlotNode *>(p);
        if (s->mu16Used != 32) { n = s; break; }
    }
    if (!n) {
        if (void *mem = bridge::Alloc(sizeof(FESlotNode))) {
            n = FESlotNode::Construct(mem, static_cast<uint16_t>(mu32BlockSize));
            mNodes.AddNode(nullptr, &n->node);
        }
    }
    return n->AllocBlock();   // like the original: out of memory -> null node -> crash
}

bool FESlotPool::Free(const uint8_t *block)
{
    for (FEMinNode *p = mNodes.head; p; p = p->next) {
        auto *s = reinterpret_cast<FESlotNode *>(p);
        if (block >= s->mpBuffer && block < s->mpBuffer + (static_cast<uint32_t>(s->mu16BlockSize) << 5)) {
            s->FreeBlock(block);
            if (s->mu16Used == 0) {
                mNodes.RemNode(&s->node);
                bridge::VirtualScalarDelete(s);
            }
            return true;
        }
    }
    return false;
}

void FESlotPool::Destruct()
{
    mNodes.Destruct();
    node.vtable = bridge::FEMinNodeVTable();
}

FESlotPool *FESlotPool::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

// ---------------------------------------------------------------- FEMultiPool
void *FEMultiPool::Alloc(uint32_t size)
{
    if (size == 0) return nullptr;
    FESlotPool *pool = nullptr;
    for (FEMinNode *p = mPools.head; p; p = p->next) {
        auto *s = reinterpret_cast<FESlotPool *>(p);
        if (s->mu32BlockSize == size) { pool = s; break; }
    }
    if (!pool) {
        if (void *mem = bridge::Alloc(sizeof(FESlotPool))) {
            pool = static_cast<FESlotPool *>(mem);
            pool->node.next = pool->node.prev = kUnlinked;
            pool->node.vtable = bridge::FESlotPoolVTable();
            pool->mNodes = FEMinList{bridge::FEMinListVTable(), 0, nullptr, nullptr};
            pool->mu32BlockSize = size;
            mPools.AddNode(nullptr, &pool->node);
        }
    }
    return pool->Alloc();
}

// Like the original, a block that belongs to no pool dereferences a null pool (crash).
void FEMultiPool::Free(void *block)
{
    if (!block) return;
    auto *b = static_cast<const uint8_t *>(block);
    FESlotPool *pool = reinterpret_cast<FESlotPool *>(mPools.head);
    while (pool && !pool->Free(b)) pool = reinterpret_cast<FESlotPool *>(pool->node.next);
    if (pool->mNodes.count == 0) {
        mPools.RemNode(&pool->node);
        bridge::VirtualScalarDelete(pool);
    }
}
