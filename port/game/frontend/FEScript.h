// FEng scripts (animations attached to an FEObject): key nodes, key tracks, scripts. Original: zFe2.cpp.
// Layouts shared with original game code (PC). GC classes put the FEMinNode fields first and the vptr at +8;
// PC has the vtable first, sizes unchanged except FEKeyTrack (GC 0x34, PC 0x38).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"

// One key of a track. Pooled (PC pool list 0x839BD0, 256 per block, allocated by PC 53E650).
struct FEKeyNode {
    FEMinNode node;                  // +0x00 vtable PC 0x79AA34
    uint32_t _data[5];               // +0x0C time + value (not interpreted here)
};

// One animated property of a script (element of FEScript::mpTracks, MSVC new[] with count cookie).
struct FEKeyTrack {
    uint8_t _byte0[4];               // +0x00
    uint32_t _bits4;                 // +0x04 24-bit field + byte at +7 (copied as a whole)
    FEKeyNode mBaseKey;              // +0x08
    FERefList mKeys;                 // +0x28 owned key nodes, or a reference to another track's list

    static FEKeyTrack *Construct(void *mem);   // PC 525E80
    void Destruct();                           // PC 52B960 (GC: inlined in ~FEScript)
    FEKeyTrack &Assign(const FEKeyTrack &src); // GC 8014121C operator= / PC 54D350: deep copy of the keys
    void AssignReference(const FEKeyTrack &src);   // inlined in PC 551BE0: shares src's key list
};

struct FEScriptEvent {               // 0xC bytes, copied as raw data
    uint32_t _data[3];
};

// Resizable event array (count + new[] pointer).
struct FEScriptEventList {
    uint32_t mu32Count;              // +0x0
    FEScriptEvent *mpEvents;         // +0x4
    void SetCount(uint32_t count);   // PC 519A10: keeps the first entries, new ones zeroed
};

struct FEScript {
    FEMinNode node;                  // +0x00 vtable PC 0x79AD28
    uint32_t _unknown0C;             // +0x0C
    uint32_t _unknown10;             // +0x10 not set by the constructor
    uint32_t _unknown14;             // +0x14
    uint32_t _unknown18;             // +0x18 not copied by the copy constructor
    uint32_t mu32NumTracks;          // +0x1C
    FEKeyTrack *mpTracks;            // +0x20 new[] (count cookie at [-4])
    FEScriptEventList mEvents;       // +0x24
    char *mpcName;                   // +0x2C owned
    uint32_t mu32Id;                 // +0x30 FEHashUpper(name), 0xFFFFFFFF without a name

    static FEScript *Construct(void *mem);   // PC 511D70 (body = GC FEScript::Init 8013D1C4)
    static FEScript *CopyConstruct(void *mem, const FEScript &src, bool reference);   // GC 8013D2E0 / PC 551BE0
    void Destruct();                         // GC 8013D1E8 / PC 535170
    FEScript *ScalarDeletingDestructor(uint32_t flags);   // PC 53EE30 (vtable slot 0, pooled delete)
    void SetName(const char *name);          // PC 518730
    void SetTrackCount(uint32_t count);      // PC 535210: old tracks destroyed, new ones default-constructed
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEKeyNode) == 0x20);
static_assert(sizeof(FEKeyTrack) == 0x38 && offsetof(FEKeyTrack, mBaseKey) == 8 && offsetof(FEKeyTrack, mKeys) == 0x28);
static_assert(sizeof(FEScriptEvent) == 0xC);
static_assert(offsetof(FEScript, mu32NumTracks) == 0x1C && offsetof(FEScript, mpTracks) == 0x20);
static_assert(offsetof(FEScript, mEvents) == 0x24 && offsetof(FEScript, mpcName) == 0x2C);
static_assert(offsetof(FEScript, mu32Id) == 0x30 && sizeof(FEScript) == 0x34);
#endif
