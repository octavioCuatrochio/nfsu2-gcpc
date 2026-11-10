#include "FEScript.h"
#include "FEHash.h"
#include "FEPool.h"
#include "FEObject.h"
#include "bridge/Bridge.h"
#include <cstring>

void DestroyTrackArray(FEKeyTrack *tracks)
{
    uint32_t *cookie = reinterpret_cast<uint32_t *>(tracks) - 1;
    for (uint32_t i = *cookie; i-- > 0;) tracks[i].Destruct();
    bridge::FreeArray(cookie);
}

namespace {
FEMinNode *Unlinked() { return reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED); }

// Tracks array: count cookie in the 4 bytes before it; elements destroyed last to first (eh vector dtor).
void DestroyTracks(FEKeyTrack *tracks)
{
    uint32_t *cookie = reinterpret_cast<uint32_t *>(tracks) - 1;
    for (uint32_t i = *cookie; i-- > 0;) tracks[i].Destruct();
    bridge::FreeArray(cookie);
}

// FERefList::ReferenceList (GC 80142694, inlined on PC): a null list clears; otherwise an owning list is
// purged first, then it points at `other` and becomes a reference.
void ReferenceList(FERefList &l, FERefList *other)
{
    if (!other) {
        l.head = l.tail = nullptr;
        l.mbIsReference = 0;
        return;
    }
    if (!l.mbIsReference) l.Purge();
    l.head = reinterpret_cast<FEMinNode *>(other);
    l.mbIsReference = 1;
}
}  // namespace

// PC 4FE8B0
FEKeyNode *FEKeyNode::Construct(void *mem)
{
    auto *k = static_cast<FEKeyNode *>(mem);
    k->node.next = k->node.prev = Unlinked();
    k->node.vtable = bridge::FEKeyNodeVTable();
    return k;
}

void FEKeyNode::Destruct() { node.vtable = bridge::FEMinNodeVTable(); }

// PC 531FE0. Same double destruction as the other pooled classes.
FEKeyNode *FEKeyNode::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) {
        bridge::VirtualScalarDtor(this, 0);
        PoolDelete(this);
    }
    return this;
}

void *FEKeyNode::PoolNew() { return FEPoolNew<FEKeyNode, 256>(*bridge::FEKeyNodePool(), FEKeyNodeBlock_Construct); }
void FEKeyNode::PoolDelete(void *p) { FEPoolDelete<FEKeyNode, 256>(*bridge::FEKeyNodePool(), p); }

// PC 525E80. Base key data (+0x14..+0x24) and the ref list's padding are left as they were.
FEKeyTrack *FEKeyTrack::Construct(void *mem)
{
    auto *t = static_cast<FEKeyTrack *>(mem);
    std::memset(t, 0, kHeaderBytes);
    t->mBaseKey.node.next = t->mBaseKey.node.prev = Unlinked();
    t->mBaseKey.node.vtable = bridge::FEKeyNodeVTable();
    t->mKeys.vtable = bridge::FERefListVTable();
    t->mKeys.head = t->mKeys.tail = nullptr;
    t->mKeys.mbIsReference = 0;
    return t;
}

// PC 52B960: FERefList dtor (vtable, purge if owning), then the base key's FEMinNode dtor (vtable only).
void FEKeyTrack::Destruct()
{
    mKeys.vtable = bridge::FERefListVTable();
    if (!mKeys.mbIsReference) mKeys.Purge();
    mBaseKey.node.vtable = bridge::FEMinNodeVTable();
}

// PC 54D350. Like the original, the own key list is purged unconditionally first (even if it is a reference).
// Owned source keys are cloned into new pooled nodes (appended in order); a referencing source makes this
// list reference the same list.
FEKeyTrack &FEKeyTrack::Assign(const FEKeyTrack &src)
{
    mKeys.Purge();
    std::memcpy(this, &src, kHeaderBytes);
    std::memcpy(mBaseKey._data, src.mBaseKey._data, sizeof mBaseKey._data);
    if (src.mKeys.mbIsReference) {
        ReferenceList(mKeys, reinterpret_cast<FERefList *>(src.mKeys.head));
        return *this;
    }
    for (FEMinNode *n = src.mKeys.head; n; n = n->next) {
        auto *k = static_cast<FEKeyNode *>(FEKeyNode::PoolNew());
        if (k) FEKeyNode::Construct(k);   // original: null -> the copy below writes through null (crash)
        std::memcpy(k->_data, reinterpret_cast<const FEKeyNode *>(n)->_data, sizeof k->_data);
        // append after tail (FEMinList::AddNode logic without a count)
        FEMinNode *after = mKeys.tail;
        if (!after) {
            k->node.next = mKeys.head;
            if (mKeys.head) mKeys.head->prev = &k->node;
            k->node.prev = nullptr;
            mKeys.head = &k->node;
        } else {
            k->node.next = after->next;
            if (after->next) after->next->prev = &k->node;
            k->node.prev = after;
            after->next = &k->node;
        }
        if (mKeys.tail == after) mKeys.tail = &k->node;
    }
    return *this;
}

// Inlined in PC 551BE0 (reference copy). Unlike Assign it also copies the base key's links (+0xC/+0x10) and
// does not purge first; the key list becomes a reference to src's list object itself.
void FEKeyTrack::AssignReference(const FEKeyTrack &src)
{
    std::memcpy(this, &src, kHeaderBytes);
    mBaseKey.node.next = src.mBaseKey.node.next;
    mBaseKey.node.prev = src.mBaseKey.node.prev;
    std::memcpy(mBaseKey._data, src.mBaseKey._data, sizeof mBaseKey._data);
    ReferenceList(mKeys, const_cast<FERefList *>(&src.mKeys));
}

// PC 519A10
void FEScriptEventList::SetCount(uint32_t count)
{
    if (count == mu32Count) return;
    if (count == 0) {
        if (mpEvents) bridge::FreeArray(mpEvents);
        mpEvents = nullptr;
        mu32Count = 0;
        return;
    }
    auto *n = static_cast<FEScriptEvent *>(bridge::Alloc(count * sizeof(FEScriptEvent)));
    if (count < mu32Count) {
        std::memcpy(n, mpEvents, count * sizeof(FEScriptEvent));
    } else {
        if (mu32Count) std::memcpy(n, mpEvents, mu32Count * sizeof(FEScriptEvent));
        std::memset(n + mu32Count, 0, (count - mu32Count) * sizeof(FEScriptEvent));
    }
    bridge::FreeArray(mpEvents);
    mpEvents = n;
    mu32Count = count;
}

// PC 511D70. GC FEScript::Init (8013D1C4) zeroes the same fields except +0x24/+0x28 (zeroed here too on PC).
FEScript *FEScript::Construct(void *mem)
{
    auto *s = static_cast<FEScript *>(mem);
    s->node.next = s->node.prev = Unlinked();
    s->node.vtable = bridge::FEScriptVTable();
    s->mEvents.mu32Count = 0;
    s->mEvents.mpEvents = nullptr;
    s->mpcName = nullptr;
    s->mu32Id = 0;
    s->mi32Length = 0;
    s->mu32Flags = 0;
    s->mpChainedScript = nullptr;
    s->mu32NumTracks = 0;
    s->mpTracks = nullptr;
    return s;
}

// PC 551BE0. reference=false: tracks deep-copied (new key nodes); true: tracks share the source's key lists.
FEScript *FEScript::CopyConstruct(void *mem, const FEScript &src, bool reference)
{
    FEScript *s = Construct(mem);
    s->SetName(src.mpcName);
    s->mu32Id = src.mu32Id;
    s->mi32Length = src.mi32Length;
    s->mi32Time = src.mi32Time;
    s->mu32Flags = src.mu32Flags;
    s->SetTrackCount(src.mu32NumTracks);
    for (uint32_t i = 0; i < s->mu32NumTracks; ++i) {
        if (reference) s->mpTracks[i].AssignReference(src.mpTracks[i]);
        else s->mpTracks[i].Assign(src.mpTracks[i]);
    }
    s->mEvents.SetCount(src.mEvents.mu32Count);
    if (s->mEvents.mu32Count)
        std::memcpy(s->mEvents.mpEvents, src.mEvents.mpEvents, s->mEvents.mu32Count * sizeof(FEScriptEvent));
    return s;
}

// PC 535170
void FEScript::Destruct()
{
    node.vtable = bridge::FEScriptVTable();
    if (mpTracks) DestroyTracks(mpTracks);
    mpTracks = nullptr;
    if (mpcName) bridge::FreeArray(mpcName);
    mpcName = nullptr;
    bridge::FreeArray(mEvents.mpEvents);   // unconditional in the original (delete of null)
    mEvents.mpEvents = nullptr;
    node.vtable = bridge::FEMinNodeVTable();
}

// PC 53EE30. Like FEMessageResponse: bit 0 -> second (trivial) destruction through the vtable, back to the pool.
FEScript *FEScript::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) {
        bridge::VirtualScalarDtor(this, 0);
        FEPoolDelete<FEScript, 32>(*bridge::FEScriptPool(), this);
    }
    return this;
}

// PC 518730
void FEScript::SetName(const char *name)
{
    if (mpcName) {
        bridge::FreeArray(mpcName);
        mpcName = nullptr;
    }
    mu32Id = 0xFFFFFFFFu;
    if (!name) return;
    size_t len = std::strlen(name) + 1;
    mpcName = static_cast<char *>(bridge::Alloc(static_cast<uint32_t>(len)));
    std::memcpy(mpcName, name, len);
    mu32Id = FEHashUpper(mpcName);
}

// PC 535210. Allocation failure leaves mpTracks null with the new count (like the original).
void FEScript::SetTrackCount(uint32_t count)
{
    if (mpTracks) DestroyTracks(mpTracks);
    mpTracks = nullptr;
    mu32NumTracks = count;
    if (!count) return;
    auto *raw = static_cast<uint32_t *>(bridge::Alloc(count * sizeof(FEKeyTrack) + 4));
    if (!raw) return;
    raw[0] = count;
    auto *tracks = reinterpret_cast<FEKeyTrack *>(raw + 1);
    for (uint32_t i = 0; i < count; ++i) FEKeyTrack::Construct(&tracks[i]);
    mpTracks = tracks;
}

// PC 504A00. Track index -> data index via the same table as FEObject::GetDataOffset (FETrackOffsets, PC 0x7992F4,
// in dwords). The original reads the table without a bounds check; valid indices are 0..10.
FEKeyTrack *FEScript::FindTrack(int track) const
{
    int want = static_cast<int>(FEObject::GetDataOffset(track) / 4);
    for (uint32_t i = 0; i < mu32NumTracks; ++i)
        if (mpTracks[i].mi8DataIndex == want) return &mpTracks[i];
    return nullptr;
}
