// FEng base object. Original: zFe2.cpp. Layout shared with original game code (PC, 0x5C bytes).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
#include "FEMessageResponse.h"
#include "FEScript.h"
#include "FEMath.h"

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
    uint16_t mu16RenderContext;      // +0x20 render context id (FEngine::RenderGroup numbering; gi +0x2C transform)
    uint16_t _unknown22;
    uint32_t _unknown24, _unknown28; // +0x24
    void *mpData;                    // +0x2C ObjDataPool block (track data, see GetDataOffset)
    uint32_t mu32DataSize;           // +0x30
    FEMinList mResponses;            // +0x34 FEMessageResponse list
    FEMinList mScripts;              // +0x44 FEScript list
    FEScript *mpCurrentScript;       // +0x54
    struct FERenderObject *mpCachedRender;   // +0x58 PC: cached polygons (cFEngRender)

    // Lifecycle on raw memory (objects are created/destroyed by game code while hooked)
    static FEObject *Construct(void *mem);                 // GC 801379F0 / PC 52AF20
    // GC 80137A80 / PC 554300. `reference`: scripts share the source's key lists (FEScript copy ctor).
    static FEObject *CopyConstruct(void *mem, const FEObject &src, bool reference);
    FEObject *Clone(bool reference) const;                 // GC 80138C44 / PC 5545D0 (vtable slot 1)
    void Destruct();                                       // GC 80137CD0 / PC 52AF90
    FEObject *ScalarDeletingDestructor(uint32_t flags);    // PC 534BB0 (vtable slot 0)

    void SetupMoveToTracks();                         // GC 80137EA4 / PC 517720
    void SetCurrentScript(FEScript *s) { mpCurrentScript = s; if (s) SetupMoveToTracks(); }   // GC 801381FC / PC 5178B0
    // Sets (add=false) or adds to (add=true) a track's value: in every script's matching track (base key value,
    // flag bit 7 cleared) and in the object's data. GC 8013825C/801383B0/801384EC, PC 5178D0/5179D0/517AD0.
    // Setters used by game code. Ignored for factory types >= 0x100. Mark the object dirty (FEOBJ_DIRTY) unless the
    // value would not change (Close/ColorClose against the current value, or against zero when adding);
    // SetRotation always marks it. GC 801385D0/80138700/80138858/80138988, PC 517C10/517C80/517DB0/517E20.
    void SetPosition(const FEVector3 &v, bool add);
    void SetRotation(const FEQuaternion &q, bool add);   // add: track/data = value * q
    void SetSize(const FEVector3 &v, bool add);
    void SetColor(const FEColor &c, bool add);
    void SetTrackValue(int track, const FEVector3 &v, bool add);
    void SetTrackValue(int track, const FEVector2 &v, bool add);
    void SetTrackValue(int track, const FEColor &v, bool add);
    void SetName(const char *name);                   // GC 80137DE8 / PC 5176B0
    FEScript *FindScript(uint32_t id) const;          // GC 80137E70 / PC 5046B0
    FEMessageResponse *FindResponse(uint32_t msg) const;   // GC 80138228 / PC 5046D0
    static uint32_t GetDataOffset(int track);          // GC 80138B70 / PC 5046F0 (byte offset of a key track in mpData)
    // GC 80137D90 (PC: inlined into the copy ctor 554300 and FEPackageReader::CreateObject 534DF0): new
    // uninitialised ObjDataPool block of `size` bytes.
    void SetDataSize(uint32_t size);
    // GC 80138B20 (PC: inlined into the FEngSetScript helpers 51CF70 / 51CFE0..): a different script becomes current
    // (SetupMoveToTracks) at time 0; `restart` rewinds it. GC: `restart` also redoes SetCurrentScript on the same one.
    void SetScript(FEScript *s, bool restart);
    // GC 80138AE0 (PC: inlined): by id. PC callers do nothing for an unknown id (GC would pass null on).
    void SetScript(uint32_t id, bool restart) { if (FEScript *s = FindScript(id)) SetScript(s, restart); }
};

// FEObject type 5: a group of child objects.
struct FEImage {                    // factory types 1, 9, 10 (FEPackageReader "If" tag)
    FEObject obj;                    // +0x00
    uint32_t mu32ImageFlags;         // +0x5C
};

struct FEGroup {
    FEObject obj;                    // +0x00
    FEMinList mChildren;             // +0x5C
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEGroup, mChildren) == 0x5C);
static_assert(offsetof(FEObject, mu32NameHash) == 0x10 && offsetof(FEObject, mpcName) == 0x14);
static_assert(offsetof(FEObject, mu32Type) == 0x18 && offsetof(FEObject, mu32Flags) == 0x1C);
static_assert(offsetof(FEObject, mpData) == 0x2C && offsetof(FEObject, mResponses) == 0x34);
static_assert(offsetof(FEObject, mScripts) == 0x44 && offsetof(FEObject, mpCurrentScript) == 0x54);
static_assert(sizeof(FEObject) == 0x5C);
#endif
