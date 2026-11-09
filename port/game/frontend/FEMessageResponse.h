// FEng message responses. Original: zFe2.cpp. Layouts shared with original game code (PC).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"

// One response: a type plus a parameter. Types 0x200..0x204 except 0x203 own a heap string in mParam.
struct FEResponse {
    uint32_t mu32Type;           // +0x0
    uintptr_t mParam;            // +0x4  value, or owned char* for string types
    uint32_t mu32Extra;          // +0x8

    static bool OwnsString(uint32_t type) { return type - 0x200u < 5 && type != 0x203; }
    void Construct();                                  // PC 4FE910 (GC: inlined): all zero
    void ReleaseParam();                               // GC 80137428 / PC 517600 (also PC ~FEResponse)
    void SetParam(const char *text);                   // GC 801373C8 / PC 517630: owned copy (null -> 0)
    FEResponse &Assign(const FEResponse &src);         // GC 80137348 / PC 52AD80 (operator=)
};

// Message id -> array of responses. Arrays use the MSVC new[] layout: count in the 4 bytes before the array.
struct FEMessageResponse {
    FEMinNode node;              // +0x0
    uint32_t mu32MsgId;          // +0xC
    uint32_t mu32Count;          // +0x10
    FEResponse *mpResponses;     // +0x14

    static FEMessageResponse *Construct(void *mem);    // PC 4FE920 (GC: inlined): guards, vtable, zero
    void Destruct();                                   // GC 80137490 / PC 534B10
    FEMessageResponse *ScalarDeletingDestructor(uint32_t flags);   // PC 53EE60 (pooled delete)
    static void *PoolNew();                            // GC 801374E8 / PC 54BDF0 (operator new)
    static void PoolDelete(void *p);                   // GC 80137638 / PC 525B30 (operator delete)

    void SetCount(uint32_t count);                     // GC 80137798 / PC 52AE00: resize, keeping the first entries
    void PurgeResponses();                             // GC 8013771C (PC: inlined)
    uint32_t FindResponse(uint32_t type) const;        // GC 801378C4: index, or 0xFFFFFFFF
    // GC 80137908 / PC 504490: from a condition at `index`, the index of its matching else (0x500) / end (0x501).
    uint32_t FindConditionBranchTarget(uint32_t index) const;
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEResponse) == 0xC);
static_assert(sizeof(FEMessageResponse) == 0x18 && offsetof(FEMessageResponse, mu32MsgId) == 0xC);
static_assert(offsetof(FEMessageResponse, mpResponses) == 0x14);
#endif
