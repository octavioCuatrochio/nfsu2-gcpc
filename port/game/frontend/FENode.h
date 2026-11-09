// FEng named list node. Original: zFe2.cpp. Layout shared with original game code (PC).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"

struct FENode {
    FEMinNode node;                  // +0x00 (vtable: subclass; GC FENode vtable)
    char *mpcName;                   // +0x0C owned copy (operator new[] / delete[])
    uint32_t mu32NameHash;           // +0x10 FEHashUpper(mpcName)

    // GC 80141828 / PC 519DE0 (__thiscall, ret 4). Frees the old name, copies the new one (null -> no name) and
    // rehashes (null name -> 0xFFFFFFFF). True if a copy was made.
    bool SetName(const char *name);
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FENode) == 0x14 && offsetof(FENode, mpcName) == 0xC && offsetof(FENode, mu32NameHash) == 0x10);
#endif
