// FEng base object. Original: zFe2.cpp. Layout shared with original game code (PC, 0x5C bytes).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
#include "FEMessageResponse.h"
#include "FEScript.h"

// FEObject flag bits (FEObject::mu32Flags)
enum : uint32_t {
    FEOBJ_FLAG_0x2 = 0x2,            // meaning unknown; PC FEString::SetString skips the dirty mark when set and
                                     // the text did not change
    FEOBJ_DIRTY = 0x400000,          // object must be re-rendered
};
enum : uint32_t { FE_TYPE_STRING = 2 };   // FEObject::mu32Type (factory type, FEPackageReader::CreateObject)

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
    // GC 80137A80 / PC 554300. `reference`: scripts share the source's key lists (FEScript copy ctor).
    static FEObject *CopyConstruct(void *mem, const FEObject &src, bool reference);
    FEObject *Clone(bool reference) const;                 // GC 80138C44 / PC 5545D0 (vtable slot 1)
    void Destruct();                                       // GC 80137CD0 / PC 52AF90
    FEObject *ScalarDeletingDestructor(uint32_t flags);    // PC 534BB0 (vtable slot 0)

    void SetupMoveToTracks();                         // GC 80137EA4 / PC 517720
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
#endif
