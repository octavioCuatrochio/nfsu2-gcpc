#include "FEString.h"
#include "FEHash.h"
#include "bridge/Bridge.h"
#include <cstring>
#include <new>

// PC behavior: the object is marked dirty, EXCEPT when UseOldFEngDirtyFlag is off, the text checksum is
// unchanged and FEOBJ_FLAG_0x2 is set. (The GameCube version marks it dirty only when the checksum changed.)
void FEString::MarkDirtyIfChanged(uint32_t oldChecksum)
{
    if (!bridge::UseOldFEngDirtyFlag()) {
        if (mstrText.GetChecksum() == oldChecksum && (base.mu32Flags & FEOBJ_FLAG_0x2)) return;
    }
    base.mu32Flags |= FEOBJ_DIRTY;
}

void FEString::SetString(const uint16_t *text)
{
    uint32_t old = mstrText.GetChecksum();
    mstrText = text;
    MarkDirtyIfChanged(old);
}

void FEString::SetString(const char *packed)
{
    uint32_t old = mstrText.GetChecksum();
    mstrText = packed;
    MarkDirtyIfChanged(old);
}

FEString *FEString::Construct(void *mem)
{
    auto *s = static_cast<FEString *>(mem);
    FEObject::Construct(&s->base);
    s->base.node.vtable = bridge::FEStringVTable();
    s->mpcLabel = nullptr;
    s->mu32LabelHash = 0xFFFFFFFFu;
    new (&s->mstrText) FEWideString();
    s->_unknown6C = s->_unknown70 = s->_unknown74 = 0;
    s->base.mu32Type = FE_TYPE_STRING;
    return s;
}

FEString *FEString::CopyConstruct(void *mem, const FEString &src, bool deep)
{
    auto *s = static_cast<FEString *>(mem);
    FEObject::CopyConstruct(&s->base, src.base, deep);
    s->base.node.vtable = bridge::FEStringVTable();
    s->mpcLabel = nullptr;
    new (&s->mstrText) FEWideString(src.mstrText);
    s->_unknown6C = src._unknown6C;
    s->_unknown70 = src._unknown70;
    s->_unknown74 = src._unknown74;
    s->SetLabel(src.mpcLabel);
    s->mstrText.SetLength(src.mstrText.mulBufferLength);   // keep the source's capacity
    return s;
}

void FEString::Destruct()
{
    base.node.vtable = bridge::FEStringVTable();
    bridge::FreeArray(mpcLabel);
    mstrText.~FEWideString();
    base.Destruct();
}

FEString *FEString::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

FEString *FEString::Clone(bool deep) const
{
    void *mem = bridge::Alloc(sizeof(FEString));
    return mem ? CopyConstruct(mem, *this, deep) : nullptr;
}

// Replaces the label (owned copy, game heap), rehashes it, marks the object dirty, notifies the label callback.
void FEString::SetLabel(const char *label)
{
    bridge::FreeArray(mpcLabel);
    mpcLabel = nullptr;
    if (label) {
        size_t n = std::strlen(label) + 1;
        mpcLabel = static_cast<char *>(bridge::Alloc(static_cast<uint32_t>(n)));
        std::memcpy(mpcLabel, label, n);
    }
    mu32LabelHash = FEHashUpper(mpcLabel);
    base.mu32Flags |= FEOBJ_DIRTY;
    bridge::CallLabelCallback(this);
}
