#include "FETypeLib.h"
#include <cstring>
#include "bridge/Bridge.h"

const uint32_t kFEKeyTypeSize[8] = {4, 4, 4, 8, 12, 16, 16, 1};

namespace {
// CRT _stricmp (PC 75DD6F) == 0 in the C locale: ASCII case folding only.
bool NameEquals(const char *a, const char *b)
{
    for (;; ++a, ++b) {
        unsigned char x = static_cast<unsigned char>(*a), y = static_cast<unsigned char>(*b);
        if (x >= 'A' && x <= 'Z') x = static_cast<unsigned char>(x + 32);
        if (y >= 'A' && y <= 'Z') y = static_cast<unsigned char>(y + 32);
        if (x != y) return false;
        if (!x) return true;
    }
}
}  // namespace

void FEFieldNode::SetDefault(const void *value)
{
    bridge::FreeArray(mpDefault);
    mpDefault = nullptr;
    if (mu32ValueSize) {
        mpDefault = bridge::Alloc(mu32ValueSize);
        std::memcpy(mpDefault, value, mu32ValueSize);
    }
}

void FETypeNode::UpdateOffsets()
{
    uint32_t offset = 0;
    for (auto *f = reinterpret_cast<FEFieldNode *>(mParams.head); f; f = reinterpret_cast<FEFieldNode *>(f->node.node.next)) {
        f->mu32Offset = offset;
        offset += f->mu32ValueSize;
    }
}

void FETypeNode::AddFieldNode(FEFieldNode *field)
{
    mParams.AddNode(mParams.tail, field ? &field->node.node : nullptr);   // null: nothing added, offsets redone
    UpdateOffsets();
}

void FETypeNode::AddField(const char *name, uint32_t type)
{
    auto *f = static_cast<FEFieldNode *>(bridge::Alloc(sizeof(FEFieldNode)));
    if (f) {
        f->node.mpcName = nullptr;
        f->node.mu32NameHash = 0;
        f->mu32ValueSize = 0;
        f->mu32Offset = 0;
        f->mpDefault = nullptr;
        f->node.node.next = f->node.node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
        f->node.node.vtable = bridge::FEFieldNodeVTable();
    }
    f->node.SetName(name);   // like the original, a failed allocation crashes here
    f->mu32ParamType = type;
    f->mu32ValueSize = kFEKeyTypeSize[type];
    AddFieldNode(f);
}

FEFieldNode *FETypeNode::GetField(const char *name) const
{
    auto *f = reinterpret_cast<FEFieldNode *>(mParams.head);
    while (f && !NameEquals(f->node.mpcName, name)) f = reinterpret_cast<FEFieldNode *>(f->node.node.next);
    return f;
}

FETypeNode *FETypeLib::FindType(uint32_t type) const
{
    auto *t = reinterpret_cast<FETypeNode *>(mTypes.head);
    while (t && t->mu32Type != type) t = reinterpret_cast<FETypeNode *>(t->node.node.next);
    return t;
}

FETypeNode *FETypeLib::CreateBaseObjectType(const char *name)
{
    auto *t = static_cast<FETypeNode *>(bridge::Alloc(sizeof(FETypeNode)));
    if (t) {
        t->node.node.next = t->node.node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
        t->node.mpcName = nullptr;
        t->node.mu32NameHash = 0;
        t->node.node.vtable = bridge::FETypeNodeVTable();
        t->mParams = FEMinList{bridge::FEMinListVTable(), 0, nullptr, nullptr};
    }
    t->node.SetName(name);
    t->AddField("Color", 6);
    t->AddField("Pivot", 4);
    t->AddField("Position", 4);
    t->AddField("Rotation", 5);
    t->AddField("Size", 4);
    const uint32_t color[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    const float zero[3] = {0, 0, 0}, rotation[4] = {0, 0, 0, 1.0f}, size[3] = {1.0f, 1.0f, 1.0f};
    auto *f = reinterpret_cast<FEFieldNode *>(t->mParams.head);   // fields in the order just added
    f->SetDefault(color);
    f = reinterpret_cast<FEFieldNode *>(f->node.node.next);
    f->SetDefault(zero);
    f = reinterpret_cast<FEFieldNode *>(f->node.node.next);
    f->SetDefault(zero);
    f = reinterpret_cast<FEFieldNode *>(f->node.node.next);
    f->SetDefault(rotation);
    f = reinterpret_cast<FEFieldNode *>(f->node.node.next);
    f->SetDefault(size);
    return t;
}

FETypeNode *FETypeLib::CreateImageObjectType(const char *name)
{
    FETypeNode *t = CreateBaseObjectType(name);
    t->AddField("Upper Left", 3);
    t->AddField("Lower Right", 3);
    const float upperLeft[2] = {0, 0}, lowerRight[2] = {1.0f, 1.0f};
    FEFieldNode *f = t->GetField("Upper Left");
    f->SetDefault(upperLeft);
    f = reinterpret_cast<FEFieldNode *>(f->node.node.next);   // the field after it (PC does not look it up by name)
    f->SetDefault(lowerRight);
    return t;
}

bool FETypeLib::Startup()
{
    auto add = [this](FETypeNode *t, uint32_t id) {
        t->mu32Type = id;
        mTypes.AddNode(mTypes.tail, &t->node.node);
    };
    add(CreateImageObjectType("Image"), 1);
    FETypeNode *cbv = CreateImageObjectType("CBV Image");
    static const char *const kCorners[4] = {"Top Left", "Top Right", "Bottom Right", "Bottom Left"};   // PC 0x7F7CE4
    for (const char *c : kCorners) cbv->AddField(c, 6);
    const uint32_t white[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    for (const char *c : kCorners) cbv->GetField(c)->SetDefault(white);
    add(cbv, 9);
    FETypeNode *anim = CreateImageObjectType("Anim Image");
    anim->AddField("Frame Number", 1);   // PC 0x7F7CF4
    const uint32_t frame = 0;
    anim->GetField("Frame Number")->SetDefault(&frame);
    add(anim, 10);
    add(CreateBaseObjectType("Simple Image"), 11);
    add(CreateBaseObjectType("String"), 2);
    add(CreateBaseObjectType("Model"), 3);
    add(CreateBaseObjectType("Movie"), 7);
    add(CreateBaseObjectType("Effect"), 8);
    add(CreateBaseObjectType("List"), 4);
    add(CreateBaseObjectType("Group"), 5);
    add(CreateBaseObjectType("Code List"), 6);
    return true;
}

void FEFieldNode::Destruct()
{
    node.node.vtable = bridge::FEFieldNodeVTable();
    bridge::FreeArray(mpDefault);
    node.Destruct();
}

FEFieldNode *FEFieldNode::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

void FETypeNode::Destruct()
{
    mParams.vtable = bridge::FEMinListVTable();
    mParams.Purge();
    node.Destruct();
}

FETypeNode *FETypeNode::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}
