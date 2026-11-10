// FEng text object. Original: zFe2.cpp
// Objects are created by original game code while running hooked, so the layout must match the PC
// game exactly. FEObject: see FEObject.h.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEObject.h"
#include "FEWideString.h"

struct FEString {
    FEObject base;                   // +0x00 (embedded, not inherited: keeps standard layout)
    char *mpcLabel;                  // +0x5C owned label text (game heap)
    uint32_t mu32LabelHash;          // +0x60 FEHashUpper(label)
    FEWideString mstrText;           // +0x64 displayed text
    uint32_t _unknown6C;             // +0x6C copied by the copy constructor; meaning unknown
    uint32_t _unknown70;             // +0x70
    uint32_t _unknown74;             // +0x74

    // Lifecycle on raw memory (objects are created/destroyed by game code while hooked)
    static FEString *Construct(void *mem);                                     // PC 495630 (GC: inlined)
    static FEString *CopyConstruct(void *mem, const FEString &src, bool deep); // GC 80142884 / PC 554970
    void Destruct();                                                           // GC 8030D158 / PC 4956A0
    FEString *ScalarDeletingDestructor(uint32_t flags);                        // PC 495780 (MSVC vtable slot 0)
    FEString *Clone(bool deep) const;                                          // GC 8030D1B8 / PC 495710

    void SetLabel(const char *label);       // GC 801429EC / PC 519F40
    // PC 4FE940 (GC: inlined): a new hash marks the object (flag 0x400000); the label callback runs either way.
    void SetLabelHash(uint32_t hash);
    void SetString(const uint16_t *text);   // GC 8014290C / PC 535F40
    void SetString(const char *packed);     // GC 8014297C / PC 535FE0

private:
    void MarkDirtyIfChanged(uint32_t oldChecksum);
};

#if UINTPTR_MAX == 0xFFFFFFFFu   // 32-bit: must be the game's exact layout
static_assert(offsetof(FEString, mpcLabel) == 0x5C);
static_assert(offsetof(FEString, mstrText) == 0x64);
static_assert(sizeof(FEString) == 0x78);   // GC FEString::Clone allocates 0x78
#endif
