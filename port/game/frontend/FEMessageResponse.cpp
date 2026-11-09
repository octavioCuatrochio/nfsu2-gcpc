#include "FEMessageResponse.h"
#include "bridge/Bridge.h"
#include "FEPool.h"
#include <cstring>

void FEResponse::Construct()
{
    mu32Type = 0;
    mParam = 0;
    mu32Extra = 0;
}

void FEResponse::ReleaseParam()
{
    if (OwnsString(mu32Type) && mParam) bridge::FreeArray(reinterpret_cast<void *>(mParam));
    mParam = 0;
}

void FEResponse::SetParam(const char *text)
{
    ReleaseParam();
    if (text) {
        size_t n = std::strlen(text) + 1;
        auto *copy = static_cast<char *>(bridge::Alloc(static_cast<uint32_t>(n)));
        std::memcpy(copy, text, n);
        mParam = reinterpret_cast<uintptr_t>(copy);
    }
}

FEResponse &FEResponse::Assign(const FEResponse &src)
{
    ReleaseParam();
    mu32Type = src.mu32Type;
    if (OwnsString(mu32Type))
        SetParam(reinterpret_cast<const char *>(src.mParam));
    else
        mParam = src.mParam;
    mu32Extra = src.mu32Extra;
    return *this;
}

namespace {
// MSVC new[]/delete[] for FEResponse: [count][elements...]; elements built in order, destroyed in reverse.
FEResponse *NewResponses(uint32_t n)
{
    auto *block = static_cast<uint32_t *>(bridge::Alloc(n * sizeof(FEResponse) + 4));
    if (!block) return nullptr;
    *block = n;
    auto *arr = reinterpret_cast<FEResponse *>(block + 1);
    for (uint32_t i = 0; i < n; ++i) arr[i].Construct();
    return arr;
}

void DeleteResponses(FEResponse *arr)
{
    if (!arr) return;
    auto *block = reinterpret_cast<uint32_t *>(arr) - 1;
    for (uint32_t i = *block; i-- > 0;) arr[i].ReleaseParam();
    bridge::FreeArray(block);
}
}  // namespace

void FEMessageResponse::PurgeResponses()
{
    DeleteResponses(mpResponses);
    mpResponses = nullptr;
    mu32Count = 0;
}

void FEMessageResponse::SetCount(uint32_t count)
{
    if (count == mu32Count) return;
    if (count == 0) {
        PurgeResponses();
        return;
    }
    FEResponse *arr = NewResponses(count);
    uint32_t keep = count < mu32Count ? count : mu32Count;
    for (uint32_t i = 0; i < keep; ++i) arr[i].Assign(mpResponses[i]);   // like the original: no null check
    DeleteResponses(mpResponses);
    mpResponses = arr;
    mu32Count = count;
}

uint32_t FEMessageResponse::FindResponse(uint32_t type) const
{
    for (uint32_t i = 0; i < mu32Count; ++i)
        if (mpResponses[i].mu32Type == type) return i;
    return 0xFFFFFFFFu;
}

FEMessageResponse *FEMessageResponse::Construct(void *mem)
{
    auto *m = static_cast<FEMessageResponse *>(mem);
    auto *unlinked = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
    m->node.next = m->node.prev = unlinked;
    m->node.vtable = bridge::FEMessageResponseVTable();
    m->mu32MsgId = 0;
    m->mu32Count = 0;
    m->mpResponses = nullptr;
    return m;
}

void FEMessageResponse::Destruct()
{
    node.vtable = bridge::FEMessageResponseVTable();
    PurgeResponses();
    node.vtable = bridge::FEMinNodeVTable();
}

// Like the original: with bit 0 the object is destroyed a second time through its vtable (flag 0) before it
// returns to the pool (harmless: the first run already cleared it).
FEMessageResponse *FEMessageResponse::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) {
        bridge::VirtualScalarDtor(this, 0);
        PoolDelete(this);
    }
    return this;
}

void *FEMessageResponse::PoolNew()
{
    return FEPoolNew<FEMessageResponse, 64>(*bridge::FEMessageResponsePool(), FEMessageResponseBlock_Construct);
}

void FEMessageResponse::PoolDelete(void *p)
{
    FEPoolDelete<FEMessageResponse, 64>(*bridge::FEMessageResponsePool(), p);
}

// PC 504490. Conditions 0x300/0x301 nest; 0x500 (else) ends the branch only at depth 1; 0x501 closes one level.
uint32_t FEMessageResponse::FindConditionBranchTarget(uint32_t index) const
{
    const uint32_t n = mu32Count;
    if (index == n - 1) return n;
    int32_t depth = 1;
    do {
        ++index;
        uint32_t t = mpResponses[index].mu32Type;
        if (t == 0x500) { if (depth == 1) depth = 0; }
        else if (t == 0x300 || t == 0x301) ++depth;
        else if (t == 0x501) --depth;
    } while (index < n && depth != 0);
    return index;
}
