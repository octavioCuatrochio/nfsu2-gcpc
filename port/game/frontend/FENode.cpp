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
