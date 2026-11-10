// .fng package reader. Original: zFe2.cpp (FEPackageReader). PC layout (0x50 bytes, lives on FEngine::LoadPackage's
// stack). Chunks: {id, size, data[size]}; children follow each other inside a parent's data. Tags (inside chunk
// data): {uint16 id, uint16 len, data[len]}. GC byte-swaps everything (GC-only).
#pragma once
#include <cstddef>
#include <cstdint>
struct FEPackage;
struct FEObject;
struct FEngine;

struct FEPackageReader {
    FEPackage *mpPackage;            // +0x00 package being built (deleted on failure)
    const uint32_t *mpData;          // +0x04 root chunk (GC +0x04)
    uint8_t mbPackage16;             // +0x08 Load arg 6 -> FEPackage +0x16 before Startup
    uint8_t mbFlag09;                // +0x09 Load arg 4 (FEngine +3)
    uint8_t mbFlag0A;                // +0x0A Load arg 5 (FEngine +4)
    uint8_t _pad0B;
    FEObject *mpCurrentObject;       // +0x0C object whose response tags are being read (GC +0x14)
    void *mp10;                      // +0x10
    struct FEGroup *mpParentGroup;   // +0x14 group the current object goes into (null: package top level)
    uint8_t mbMergeResponses;        // +0x18 (GC +0x20) object responses: reuse an existing message id
    uint8_t _unknown19[0x30 - 0x19];
    uint32_t mu32NumTypes;           // +0x30 (GC +0x38) "TypS" entries {type, size}
    const uint32_t *mpTypes;         // +0x34 (GC +0x3C)
    void *mpGameInterface;           // +0x38 FEngine +0x10C
    FEngine *mpEngine;               // +0x3C
    uint32_t mu32Header18;           // +0x40 (GC +0x48) header dword 6
    uint32_t mu32Header1C;           // +0x44 (GC +0x4C) header dword 7
    uint32_t mu32NumButtons;         // +0x48 "Butn" count
    uint32_t mu32_4C;                // +0x4C

    // GC 8013A50C (PC: inlined everywhere): child with `id`, scanning up to and including the chunk at the parent's
    // end (like the original); null if none.
    static const uint32_t *FindChild(const uint32_t *parent, uint32_t id);
    // GC 8013A3CC / PC 555720 (ret 0x18): the package, or null (partially built package deleted).
    FEPackage *Load(const void *data, void *gameInterface, FEngine *engine, uint8_t flag09, uint8_t flag0A, uint8_t package16);
    bool ReadHeaderChunk();                  // GC 8013A6DC / PC 52B4D0: magic, version >= 0x20000, new FEPackage
    bool ReadTypeSizes();                    // GC 8013A660 / PC 5049A0 (keeps ECX: Load relies on it); always true
    uint32_t GetTypeSize(uint32_t type) const;   // GC 8013A5E4 (PC: inlined); 0 if unknown
    bool ReadLibraryRefsChunk();             // GC 8013AA14 / PC 5182E0: "LibR"; none -> true only if pkg list +0x74 empty
    bool ReadResourceChunk();                // GC 8013AB50 / PC 518380: "ResÌ" = "RsNm" names + "RsRq" 0x18-byte requests
    bool ReadPackageResponseChunk();         // GC 8013AE54 / PC 551580: "PkgR" (optional)
    // GC 8013CCE4 / PC 54D1D0 (ret 0xC): tags Mi (new response / reuse), CM (count), Ri/Rs/Ru/Rt (response i).
    bool ReadMessageResponseTags(const uint8_t *tags, uint32_t size, bool package);
    // GC 8013CF34 / PC 5184E0: "Targ" tags Tc (count: new[] of FEMessageTargets) and Mt (msg id + object GUIDs,
    // each looked up with FEFindByGUID, null kept). Always true.
    bool ReadMessageTargetListChunk();
    // GC 8013A858 / PC 52B550: "LibL" names -> FENodes in package list +0x74; each library is found (refcount++) or
    // fetched from the game (interface slot 0), loaded, announced (slot 3) and added to the engine's library list.
    bool ReadReferencedPackagesChunk();
    // GC 8013AEC8 / PC 5554A0: "Obj\xCC": "Butn" (button table size) entries up to the first "FObj\xEA"; from there
    // every chunk is an object: ObjD tags (ReadObjectTags; failure fails the load), MsgR responses, Scrp scripts;
    // then list-box fixups, chained-script hashes -> pointers, script 0x1744B3 made current (no null check after),
    // initial key interpolation (unless mbPackage16) and linking into the parent group or the package.
    bool ReadObjectChunk();
    // GC 8013D134 / PC 5186C0 (ret 0xC): library reference -> library package (FEngine::FindLibraryPackage) -> object.
    bool FindReferencedObject(uint32_t refHash, FEObject **obj, FEPackage **pkg);
};

// GC 80145A10 / PC 555800 (ret 8): a reader on the stack, Load with the engine's game interface and flags.
// GC 80145F48 / PC 50B240 (ret 4): library whose FEHashUpper(filename + 2) matches.
FEPackage *FEngine_FindLibraryPackage(FEngine *engine, uint32_t hash);
FEPackage *FEngine_LoadPackage(FEngine *engine, const void *data, uint8_t package16);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEPackageReader, mpCurrentObject) == 0xC && offsetof(FEPackageReader, mbMergeResponses) == 0x18);
static_assert(offsetof(FEPackageReader, mu32NumTypes) == 0x30 && offsetof(FEPackageReader, mpEngine) == 0x3C);
static_assert(sizeof(FEPackageReader) == 0x50);
#endif
