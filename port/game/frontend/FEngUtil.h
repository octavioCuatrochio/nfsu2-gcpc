// FEng free helper functions used by game screens and widgets (GC zFe2.cpp "FEng*" API). All __cdecl on PC.
#pragma once
#include <cstdint>

struct FEObject;

uint32_t FEngGetTextureHash(const FEObject *obj);              // GC 801500BC / PC 50C980 (null -> 0)
void FEngSetTextureHash(FEObject *obj, uint32_t hash);         // GC 801500D4 / PC 50C990 (dirty only on change)
void FEngSetMovieName(FEObject *movie, const char *name);      // GC 801500FC / PC 50C9B0 (bStrNCpy into +0x24, 0x100)
int32_t FEngIsVisible(const FEObject *obj);                    // GC 80150140 / PC 50C9E0 (flag 1 clear)
// GC 80150168 / PC 50CA00 and GC 801501F0 / PC 50CA50: set/clear the hidden flag (1) + dirty (0x400000) when it
// changes or UseOldFEngDirtyFlag; groups recurse into their children (count from the list, unchecked).
void FEngSetInvisible(FEObject *obj);
void FEngSetVisible(FEObject *obj);
// GC 801505D0 / PC 50CAB0: the object's script named `name` has reached its length (false if absent).
bool FEngIsScriptFinished(const FEObject *obj, const char *name);
void FEngSetSize(FEObject *obj, float w, float h);             // GC 80150F6C / PC 50CB00: data Size (+0x38/+0x3C)

constexpr uint32_t kFEObjectDirty = 0x400000;
