// FEng base object. Original: zFe2.cpp. Layout shared with original game code (PC, 0x5C bytes).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
#include "FEMessageResponse.h"

// FEObject flag bits (FEObject::mu32Flags)
enum : uint32_t {
    FEOBJ_FLAG_0x2 = 0x2,            // meaning unknown; PC FEString::SetString skips the dirty mark when set and
                                     // the text did not change
    FEOBJ_DIRTY = 0x400000,          // object must be re-rendered
};
enum : uint32_t { FE_TYPE_STRING = 2 };   // FEObject::mu32Type (factory type, FEPackageReader::CreateObject)

// Script attached to an object (not rewritten yet). Only what FEObject uses.
struct FEScript {
    FEMinNode node;                  // +0x0  vtable PC 0x79AD28 (GC: FEMinNode fields first, vptr at +8; same size)
    uint32_t _unknown0C;             // +0xC
    uint32_t _unknown10;             // +0x10 not set by the constructor
    uint32_t _unknown14[3];          // +0x14
    void *mpTracks;                  // +0x20 new[] array of 0x38-byte entries (dtor PC 52B960)
    uint32_t _unknown24;             // +0x24
    void *_owned28;                  // +0x28 new[]-owned
    char *_owned2C;                  // +0x2C new[]-owned (GC: char *)
    uint32_t mu32Id;                 // +0x30 script name hash

    static FEScript *Construct(void *mem);   // PC 511D70 (GC FEScript::FEScript() inlined; body = GC Init 8013D1C4)
    void Destruct();                         // GC 8013D1E8 / PC 535170
    FEScript *ScalarDeletingDestructor(uint32_t flags);   // PC 53EE30 (vtable slot 0, pooled delete)
};

// One animated property of a script (element of FEScript::mpTracks, MSVC new[] with count cookie). PC 0x38 bytes
// (GC 0x34). Only what the destructor touches is named.
struct FEKeyTrack {
    uint8_t _unknown00[8];
    FEMinNode mBaseKey;              // +0x08 FEKeyNode (base key; FEMinNode-derived)
    uint8_t _unknown14[0x14];
    FERefList mKeys;                 // +0x28 key nodes (owned unless mbIsReference)

    void Destruct();                 // PC 52B960 (GC: inlined in ~FEScript)
};

struct FEObject {
    FEMinNode node;                  // +0x00 vtable (PC FEObject 0x79B8E0) + list links (0xABADCAFE when unlinked)
    uint32_t mu32GUID;               // +0x0C FEngine::SysGUID at construction
    uint32_t mu32NameHash;           // +0x10 FEHashUpper(name), 0xFFFFFFFF without a name
    char *mpcName;                   // +0x14 owned (game heap)
    uint32_t mu32Type;               // +0x18 factory type
    uint32_t mu32Flags;              // +0x1C
    uint16_t _unknown20, _unknown22; // +0x20
    uint32_t _unknown24, _unknown28; // +0x24
    void *mpData;                    // +0x2C ObjDataPool block (track data, see GetDataOffset)
    uint32_t mu32DataSize;           // +0x30
    FEMinList mResponses;            // +0x34 FEMessageResponse list
    FEMinList mScripts;              // +0x44 FEScript list
    FEScript *mpCurrentScript;       // +0x54
    uint32_t _unknown58;             // +0x58

    // Lifecycle on raw memory (objects are created/destroyed by game code while hooked)
    static FEObject *Construct(void *mem);                 // GC 801379F0 / PC 52AF20
    void Destruct();                                       // GC 80137CD0 / PC 52AF90
    FEObject *ScalarDeletingDestructor(uint32_t flags);    // PC 534BB0 (vtable slot 0)

    void SetName(const char *name);                   // GC 80137DE8 / PC 5176B0
    FEScript *FindScript(uint32_t id) const;          // GC 80137E70 / PC 5046B0
    FEMessageResponse *FindResponse(uint32_t msg) const;   // GC 80138228 / PC 5046D0
    static uint32_t GetDataOffset(int track);          // GC 80138B70 / PC 5046F0 (byte offset of a key track in mpData)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEObject, mu32NameHash) == 0x10 && offsetof(FEObject, mpcName) == 0x14);
static_assert(offsetof(FEObject, mu32Type) == 0x18 && offsetof(FEObject, mu32Flags) == 0x1C);
static_assert(offsetof(FEObject, mpData) == 0x2C && offsetof(FEObject, mResponses) == 0x34);
static_assert(offsetof(FEObject, mScripts) == 0x44 && offsetof(FEObject, mpCurrentScript) == 0x54);
static_assert(sizeof(FEObject) == 0x5C);
static_assert(sizeof(FEKeyTrack) == 0x38 && offsetof(FEKeyTrack, mKeys) == 0x28);
static_assert(offsetof(FEScript, mu32Id) == 0x30 && offsetof(FEScript, mpTracks) == 0x20 && sizeof(FEScript) == 0x34);
#endif
