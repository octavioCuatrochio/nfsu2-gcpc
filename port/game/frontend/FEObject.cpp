#include "FEObject.h"
#include "FEHash.h"
#include "FEMultiPool.h"
#include "bridge/Bridge.h"
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

