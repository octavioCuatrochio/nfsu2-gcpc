#include "FEObject.h"
#include "FEHash.h"
#include "FEMultiPool.h"
#include "bridge/Bridge.h"
#include "FEPool.h"
#include <cstring>

// Exactly the fields both originals set; type (+0x18), +0x22 and the current script (+0x54) are left as they were.
FEObject *FEObject::Construct(void *mem)
{
    auto *o = static_cast<FEObject *>(mem);
    auto *unlinked = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
    o->node.vtable = bridge::FEObjectVTable();
    o->node.next = unlinked;
    o->node.prev = unlinked;
    o->mu32NameHash = 0;
    o->mpcName = nullptr;
    o->mu32Flags = 0;
    o->_unknown20 = 0;
    o->_unknown24 = o->_unknown28 = 0;
    o->mpData = nullptr;
    o->mu32DataSize = 0;
    o->mResponses = FEMinList{bridge::FEMinListVTable(), 0, nullptr, nullptr};
    o->mScripts = FEMinList{bridge::FEMinListVTable(), 0, nullptr, nullptr};
    o->_unknown58 = 0;
    o->mu32GUID = bridge::NextFEObjectGUID();
    return o;
}

void FEObject::Destruct()
{
    node.vtable = bridge::FEObjectVTable();
    bridge::CallDestructorCallback(this);
    bridge::ObjDataPool()->Free(mpData);
    if (mpcName) bridge::FreeArray(mpcName);
    mScripts.Destruct();
    mResponses.Destruct();
    node.vtable = bridge::FEMinNodeVTable();
}

FEObject *FEObject::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

// Replaces the name (owned copy, game heap); hash = FEHashUpper(name), or 0xFFFFFFFF for no name.
void FEObject::SetName(const char *name)
{
    if (mpcName) {
        bridge::FreeArray(mpcName);
        mpcName = nullptr;
    }
    mu32NameHash = 0xFFFFFFFFu;
    if (name) {
        size_t n = std::strlen(name) + 1;
        mpcName = static_cast<char *>(bridge::Alloc(static_cast<uint32_t>(n)));
        std::memcpy(mpcName, name, n);
        mu32NameHash = FEHashUpper(mpcName);
    }
}

FEScript *FEObject::FindScript(uint32_t id) const
{
    for (FEMinNode *n = mScripts.head; n; n = n->next) {
        auto *s = reinterpret_cast<FEScript *>(n);
        if (s->mu32Id == id) return s;
    }
    return nullptr;
}

FEMessageResponse *FEObject::FindResponse(uint32_t msg) const
{
    for (FEMinNode *n = mResponses.head; n; n = n->next) {
        auto *r = reinterpret_cast<FEMessageResponse *>(n);
        if (r->mu32MsgId == msg) return r;
    }
    return nullptr;
}

uint32_t FEObject::GetDataOffset(int track)
{
    static const uint32_t kOffsets[] = {0x0, 0x10, 0x1C, 0x28, 0x38, 0x44, 0x4C, 0x54, 0x64, 0x74, 0x84};
    return (track >= 0 && track <= 10) ? kOffsets[track] : 0;
}


namespace {
// FEMinList::AddNode(tail, node) as inlined in PC 554300
void AppendNode(FEMinList &l, FEMinNode *n) { l.AddTail(n); }

FEScript *FindScriptIn(const FEMinList &scripts, uint32_t id)
{
    for (FEMinNode *n = scripts.head; n; n = n->next)
        if (reinterpret_cast<FEScript *>(n)->mu32Id == id) return reinterpret_cast<FEScript *>(n);
    return nullptr;
}
}  // namespace

// PC 554300. Differences from Construct: +0x20 and +0x58 are not initialized (left as they were), the GUID is
// still a fresh one. Data block: same size, zeroed (contents not copied). Responses are deep-copied; scripts via
// FEScript's copy ctor; the current script and each script's chained script are re-found by id in the copy.
// Like the original, the source's current script is dereferenced while searching (null + scripts = crash).
FEObject *FEObject::CopyConstruct(void *mem, const FEObject &src, bool reference)
{
    auto *o = static_cast<FEObject *>(mem);
    auto *unlinked = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
    o->node.next = o->node.prev = unlinked;
    o->node.vtable = bridge::FEObjectVTable();
    o->mu32NameHash = 0;
    o->mpcName = nullptr;
    o->mu32Flags = 0;
    o->_unknown24 = o->_unknown28 = 0;
    o->mpData = nullptr;
    o->mResponses = FEMinList{bridge::FEMinListVTable(), 0, nullptr, nullptr};
    o->mScripts = FEMinList{bridge::FEMinListVTable(), 0, nullptr, nullptr};
    o->mu32GUID = bridge::NextFEObjectGUID();

    // SetDataSize (GC, inlined on PC)
    uint32_t size = src.mu32DataSize;
    bridge::ObjDataPool()->Free(o->mpData);
    o->mpData = nullptr;
    void *data = bridge::ObjDataPool()->Alloc(size);
    o->mu32DataSize = size;
    o->mpData = data;
    std::memset(data, 0, size);

    o->mu32Type = src.mu32Type;
    o->mu32Flags = src.mu32Flags;
    o->_unknown20 = src._unknown20;
    o->_unknown22 = src._unknown22;
    o->_unknown24 = src._unknown24;
    o->SetName(src.mpcName);

    for (FEMinNode *n = src.mResponses.head; n; n = n->next) {
        auto *from = reinterpret_cast<const FEMessageResponse *>(n);
        FEMessageResponse *m = FEMessageResponse::Construct(FEMessageResponse::PoolNew());
        uint32_t count = from->mu32Count;
        m->SetCount(count);
        m->mu32MsgId = from->mu32MsgId;
        for (uint32_t i = 0; i < count; ++i) m->mpResponses[i].Assign(from->mpResponses[i]);
        AppendNode(o->mResponses, &m->node);
    }

    for (FEMinNode *n = src.mScripts.head; n; n = n->next) {
        void *mem2 = FEPoolNew<FEScript, 32>(*bridge::FEScriptPool(), FEScriptBlock_Construct);
        FEScript *s = FEScript::CopyConstruct(mem2, *reinterpret_cast<const FEScript *>(n), reference);
        if (s) AppendNode(o->mScripts, &s->node);
    }

    FEScript *cur = nullptr;
    for (FEMinNode *n = o->mScripts.head; n; n = n->next)
        if (reinterpret_cast<FEScript *>(n)->mu32Id == src.mpCurrentScript->mu32Id) { cur = reinterpret_cast<FEScript *>(n); break; }
    o->mpCurrentScript = cur;
    if (cur) bridge::FEObject_SetupMoveToTracks(o);

    FEMinNode *d = o->mScripts.head;
    for (FEMinNode *n = src.mScripts.head; n; n = n->next, d = d->next) {
        const FEScript *from = reinterpret_cast<const FEScript *>(n);
        if (from->mpChainedScript)
            reinterpret_cast<FEScript *>(d)->mpChainedScript = FindScriptIn(o->mScripts, from->mpChainedScript->mu32Id);
    }
    return o;
}

// PC 5545D0: new (game heap, 0x5C) + copy ctor; null on allocation failure.
FEObject *FEObject::Clone(bool reference) const
{
    void *mem = bridge::Alloc(sizeof(FEObject));
    if (!mem) return nullptr;
    return CopyConstruct(mem, *this, reference);
}
