// FEng object type library: per object type, the key-track fields (name, value type, size, offset in
// FEObject::mpData, default value). Original: zFe2.cpp (FETypeLib / FETypeNode / FEFieldNode).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
#include "FENode.h"

// GC FEFieldNode (PC 0x24 bytes, vtable 0x79B018).
struct FEFieldNode {
    FENode node;                     // +0x00 name = field name ("Color", "Position", ...)
    uint32_t mu32ParamType;          // +0x14 FEKeyTrack::mu8ParamType (low byte used)
    uint32_t mu32ValueSize;          // +0x18 kFEKeyTypeSize[type]
    uint32_t mu32Offset;             // +0x1C byte offset in FEObject::mpData (UpdateOffsets)
    void *mpDefault;                 // +0x20 default value (operator new[]), may be null

    void SetDefault(const void *value);   // GC 80142344 (PC: inlined): replaces the default with a copy
};
using FETypeParam = FEFieldNode;

// GC FETypeNode (PC 0x28 bytes, vtable 0x79B90C).
struct FETypeNode {
    FENode node;                     // +0x00 name = type name ("Image", "String", ...)
    FEMinList mParams;               // +0x14 FEFieldNode list (vtable 0x79A9EC)
    uint32_t mu32Type;               // +0x24 FEObject factory type (not initialized by CreateBaseObjectType)

    void AddField(const char *name, uint32_t type);   // GC 801423B4 / PC 519EB0 (ret 8)
    void AddFieldNode(FEFieldNode *field);            // PC 514780 (ret 4; GC: inlined): append + UpdateOffsets
    void UpdateOffsets();                             // GC 80142454 (PC: inlined): offsets = running size sum
    FEFieldNode *GetField(const char *name) const;    // GC 80142480 / PC 519F10 (ret 4): case-insensitive name
};

// GC FETypeLib (PC FEngine +0x4134): FEMinList of FETypeNode (vtable 0x79B878) + one byte.
struct FETypeLib {
    FEMinList mTypes;
    uint8_t _unknown10;

    // GC 80141D9C / PC 535610 (ret 4): new type with Color / Pivot / Position / Rotation / Size and their defaults.
    FETypeNode *CreateBaseObjectType(const char *name);
    // GC 80141F5C / PC 5358D0 (ret 4): base + "Upper Left" (0, 0) / "Lower Right" (1, 1) texture coordinates.
    FETypeNode *CreateImageObjectType(const char *name);
    // GC 80142034 / PC 5359D0: the 11 built-in types (ids 1..11); returns true.
    bool Startup();
    FETypeNode *FindType(uint32_t type) const;        // GC 80142310 / PC 505480 (ret 4)
};

// PC 0x799320 (GC FEKeyTypeSize): value size per FEKeyTrack parameter type.
extern const uint32_t kFEKeyTypeSize[8];

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEFieldNode) == 0x24 && offsetof(FEFieldNode, mpDefault) == 0x20);
static_assert(sizeof(FETypeNode) == 0x28 && offsetof(FETypeNode, mu32Type) == 0x24);
#endif
