// Language string table lookups (GC _global: SearchForString / GetLocalizedString). Original: zFe2.cpp.
#pragma once
#include <cstdint>

// One entry of the loaded language's sorted table (PC: count *(0x8383D0), table *(0x8383D8), 8 bytes each).
struct LanguageStringRecord {
    uint32_t hash;
    const char *text;
};
#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(LanguageStringRecord) == 8);
#endif

// GC 801211E0 / PC 4FF9D0 (hash in EDX, not hooked): binary search; null if missing. An empty table reads entry
// 0x7FFFFFFF like the original (count - 1 wraps).
const char *SearchForString(uint32_t hash);
// GC 801212AC (PC: inlined everywhere): the string, or the "missing string" entry 0xC01A6F63 (may still be null).
const char *GetLocalizedString(uint32_t hash);
