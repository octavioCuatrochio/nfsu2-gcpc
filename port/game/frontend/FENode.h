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
    // GC 801417C4 / PC 505350 (non-deleting): vtable 0x79AB0C, frees the name, leaves the FEMinNode vtable.
    void Destruct();
};

// GC 80141730 / PC 505310: case-insensitive compare ('a'..'z' + 0xE0), stops at the end of a or a mismatch.
int32_t FEStricmp(const char *a, const char *b);
// GC 80141A0C FEList::FindNode(name, start) / PC 519E50 (__thiscall, list unused, ret 8): first node from `start`
// (following next) whose name matches (hash, then FEStricmp); a null name matches unnamed nodes.
FENode *FEListFindNode(const char *name, FENode *start);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FENode) == 0x14 && offsetof(FENode, mpcName) == 0xC && offsetof(FENode, mu32NameHash) == 0x10);
#endif
