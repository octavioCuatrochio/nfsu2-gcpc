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

// PC 511D70. GC FEScript::Init (8013D1C4) zeroes the same fields except +0x24/+0x28 (zeroed here too on PC).
FEScript *FEScript::Construct(void *mem)
{
    auto *s = static_cast<FEScript *>(mem);
    auto *unlinked = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
    s->node.next = s->node.prev = unlinked;
    s->node.vtable = bridge::FEScriptVTable();
    s->mpTracks = nullptr;
    s->_unknown24 = 0;
    s->_owned28 = nullptr;
    s->_owned2C = nullptr;
    s->mu32Id = 0;
    s->_unknown0C = 0;
    s->_unknown14[0] = s->_unknown14[1] = s->_unknown14[2] = 0;
    return s;
}

// PC 52B960: FERefList dtor (vtable, purge if owning), then the base key's FEMinNode dtor (vtable only).
void FEKeyTrack::Destruct()
{
    mKeys.vtable = bridge::FERefListVTable();
    if (!mKeys.mbIsReference) mKeys.Purge();
    mBaseKey.vtable = bridge::FEMinNodeVTable();
}

// PC 535170. Tracks: count cookie before the array, destroyed last to first, freed from the cookie; +0x2C freed
// if set, +0x28 always (delete of null is a no-op).
void FEScript::Destruct()
{
    node.vtable = bridge::FEScriptVTable();
    if (mpTracks) {
        auto *tracks = static_cast<FEKeyTrack *>(mpTracks);
        uint32_t *cookie = reinterpret_cast<uint32_t *>(mpTracks) - 1;
        for (uint32_t i = *cookie; i-- > 0;) tracks[i].Destruct();
        bridge::FreeArray(cookie);
    }
    mpTracks = nullptr;
    if (_owned2C) bridge::FreeArray(_owned2C);
    _owned2C = nullptr;
    bridge::FreeArray(_owned28);
    _owned28 = nullptr;
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
