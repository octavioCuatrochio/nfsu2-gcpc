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

    static FEKeyNode *Construct(void *mem);   // PC 4FE8B0
    void Destruct();                          // PC 51AB10 (folded: plain FEMinNode dtor; not hooked)
    FEKeyNode *ScalarDeletingDestructor(uint32_t flags);   // PC 531FE0 (vtable slot 0, pooled delete)
    static void *PoolNew();                   // PC 53E650 (operator new, pool 0x839BD0)
    static void PoolDelete(void *p);          // PC 525BF0 (operator delete)
};

// One animated property of a script (element of FEScript::mpTracks, MSVC new[] with count cookie).
struct FEKeyTrack {
    uint8_t mu8ParamType;            // +0x00 value type: 1 int, 2 float, 3 vec2, 4 vec3, 5 quaternion, 6 color
    uint8_t mu8ValueSize;            // +0x01 value size in bytes (copied by FEInterpNone)
    uint8_t mu8InterpType;           // +0x02 3/4: relative ("move to") tracks, see FEObject::SetupMoveToTracks
    uint8_t mu8Flags;                // +0x03 bit 7 cleared by SetupMoveToTracks
    uint8_t _length[3];              // +0x04 signed 24-bit track length (time), see Length()
    int8_t mi8DataIndex;             // +0x07 value position in FEObject::mpData, in dwords
    FEKeyNode mBaseKey;              // +0x08
    FERefList mKeys;                 // +0x28 owned key nodes, or a reference to another track's list

    static constexpr size_t kHeaderBytes = 8;   // +0x00..+0x07, copied as a block

    int32_t Length() const                      // the original: dword at +4, shl 8 / sar 8
    {
        uint32_t u = uint32_t(_length[0]) | uint32_t(_length[1]) << 8 | uint32_t(_length[2]) << 16;
        return static_cast<int32_t>(u << 8) >> 8;
    }
    FEKeyNode *GetFirstKey() const;             // PC 4FE8D0: head of the (resolved) key list
    FEKeyNode *GetKeyAt(int32_t time);          // GC 80141594 / PC 505260: base key if time <= -1 or no keys
    FEKeyNode *GetDeltaKeyAt(int32_t time) const;   // GC 80141628 / PC 5052B0: null if no keys
    void SetBaseValueAt(int32_t time, const struct FEColor &value);   // GC 801416B4 / PC 535570

    static FEKeyTrack *Construct(void *mem);   // PC 525E80
    void Destruct();                           // PC 52B960 (GC: inlined in ~FEScript)
    FEKeyTrack &Assign(const FEKeyTrack &src); // GC 8014121C operator= / PC 54D350: deep copy of the keys
    void AssignReference(const FEKeyTrack &src);   // inlined in PC 551BE0: shares src's key list
};

// Key-track evaluation. out = value at `time` (size per track type). PC: all __cdecl.
void FEInterpNone(FEKeyTrack *track, int32_t time, void *out);      // GC 8014115C / PC 519D40 (step)
void FEInterpLinear(FEKeyTrack *track, int32_t time, void *out);    // GC 80140318 / PC 52BDC0
void FEKeyInterpFast(FEKeyTrack *track, int32_t time, void *out);   // GC 80140244 / PC 5354F0

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
    FEScript *mpChainedScript;       // +0x18 script of the same object (re-linked by FEObject's copy ctor)
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
    FEKeyTrack *FindTrack(int track) const;  // GC 8013D920 / PC 504A00: first track whose data index matches
    void SetTrackCount(uint32_t count);      // PC 535210: old tracks destroyed, new ones default-constructed
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEKeyNode) == 0x20);
static_assert(offsetof(FEKeyTrack, _length) == 4 && offsetof(FEKeyTrack, mi8DataIndex) == 7 && sizeof(FEKeyTrack) == 0x38 && offsetof(FEKeyTrack, mBaseKey) == 8 && offsetof(FEKeyTrack, mKeys) == 0x28);
static_assert(sizeof(FEScriptEvent) == 0xC);
static_assert(offsetof(FEScript, mu32NumTracks) == 0x1C && offsetof(FEScript, mpTracks) == 0x20);
static_assert(offsetof(FEScript, mEvents) == 0x24 && offsetof(FEScript, mpcName) == 0x2C);
static_assert(offsetof(FEScript, mu32Id) == 0x30 && sizeof(FEScript) == 0x34);
#endif
