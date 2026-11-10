#include "FEMinList.h"
#include "bridge/Bridge.h"

namespace {
FEMinNode *const kUnlinked = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
}

FEMinNode *FEMinNode::ScalarDeletingDestructor(uint32_t flags)
{
    vtable = bridge::FEMinNodeVTable();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

// GC/PC identical: links `node` after `after` (or at the head), updates tail and count. Null node: no-op.
void FEMinList::AddNode(FEMinNode *after, FEMinNode *node)
{
    if (!node) return;
    if (!after) {
        node->next = head;
        if (head) head->prev = node;
        node->prev = nullptr;
        head = node;
    } else {
        node->next = after->next;
        if (after->next) after->next->prev = node;
        node->prev = after;
        after->next = node;
    }
    if (tail == after) tail = node;
    ++count;
}

// Unlinks `node` (no membership check) and poisons its links with 0xABADCAFE. Null node: no-op.
void FEMinList::RemNode(FEMinNode *node)
{
    if (!node) return;
    if (node == head) head = node->next;
    if (node == tail) tail = node->prev;
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    node->next = kUnlinked;
    node->prev = kUnlinked;
    --count;
}

FEMinNode *FEMinList::RemHead()
{
    FEMinNode *n = head;
    if (n) RemNode(n);
    return n;
}

FEMinNode *FEMinList::FindNode(uint32_t index) const
{
    FEMinNode *n = head;
    for (uint32_t i = 0; n && i < index; ++i) n = n->next;
    return n;
}

void FEMinList::Purge()
{
    while (FEMinNode *n = RemHead()) bridge::VirtualScalarDelete(n);
}

void FEMinList::Destruct()
{
    vtable = bridge::FEMinListVTable();
    Purge();
}

// MSVC "vector deleting destructor": bit 1 -> `this` is an array allocated with new[] (element count stored in
// the 4 bytes before it), destroy every element (last first); bit 0 -> free the memory.
FEMinList *FEMinList::VectorDeletingDestructor(uint32_t flags)
{
    if (flags & 2) {
        auto *block = reinterpret_cast<uint8_t *>(this) - 4;
        int32_t n = *reinterpret_cast<int32_t *>(block);
        for (int32_t i = n - 1; i >= 0; --i) this[i].Destruct();
        if (flags & 1) bridge::FreeArray(block);
        return reinterpret_cast<FEMinList *>(block);
    }
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

// PC 5055C0. Unlike FEMinList::RemNode there is no count; head/tail fixed only if they are this node.
FEMinNode *FERefList::RemNode(FEMinNode *node)
{
    if (!node) return nullptr;
    if (node == head) head = node->next;
    if (node == tail) tail = node->prev;
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    node->next = node->prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
    return node;
}

// PC 511D00 (GC deletes via the GCC deleting-dtor slot with flag 3; PC slot 0 with flag 1).
void FERefList::Purge()
{
    while (FEMinNode *n = RemHead()) bridge::VirtualScalarDelete(n);
}

// PC 4FE890
FEMinNode *FERefList::GetHead() const
{
    const FERefList *l = this;
    while (l->mbIsReference) l = reinterpret_cast<const FERefList *>(l->head);
    return l->head;
}

FERefList *FERefList::ScalarDeletingDestructor(uint32_t flags)
{
    vtable = bridge::FERefListVTable();
    if (!mbIsReference) Purge();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}
