#include "FENode.h"
#include <cstring>
#include "FEHash.h"
#include "bridge/Bridge.h"

bool FENode::SetName(const char *name)
{
    bool copied = false;
    if (mpcName) {
        bridge::FreeArray(mpcName);
        mpcName = nullptr;
    }
    if (name) {
        const size_t n = std::strlen(name) + 1;
        mpcName = static_cast<char *>(bridge::Alloc(static_cast<uint32_t>(n)));
        if (mpcName) {
            std::memcpy(mpcName, name, n);
            copied = true;
        }
    }
    mu32NameHash = FEHashUpper(mpcName);
    return copied;
}

int32_t FEStricmp(const char *a, const char *b)
{
    auto up = [](signed char c) { return (c >= 'a' && c <= 'z') ? static_cast<signed char>(c + 0xE0) : c; };
    signed char x, y;
    do {
        x = up(static_cast<signed char>(*a++));
        y = up(static_cast<signed char>(*b++));
    } while (x != 0 && x == y);
    return x - y;
}

FENode *FEListFindNode(const char *name, FENode *start)
{
    const uint32_t hash = FEHashUpper(name);
    for (FENode *n = start; n; n = reinterpret_cast<FENode *>(n->node.next)) {
        if (!n->mpcName) {
            if (!name) return n;
        } else if (n->mu32NameHash == hash && FEStricmp(n->mpcName, name) == 0) {
            return n;
        }
    }
    return nullptr;
}
