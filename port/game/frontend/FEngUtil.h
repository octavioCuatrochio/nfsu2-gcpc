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
void FEngSetSize(FEObject *obj, float w, float h);
// GC 8015043C / PC 51CF70 and GC 801504A8 / PC 51CFE0: make the named / id'd script current (time reset when it
// changes) and optionally restart it. Missing script: nothing.
void FEngSetScript(FEObject *obj, const char *name, bool restart);
void FEngSetScript(FEObject *obj, uint32_t id, bool restart);
// GC 80150630 (+ 80150698 inlined) / PC 51D040: current script is `name` (FEngHashStringUpper) and not past its end.
bool FEngIsScriptRunning(const FEObject *obj, const char *name);
// GC 80150328 / PC 51CED0: rotation about z by `degrees` (matrix from bSinF, bMatrixToQuaternion, SetRotation).
void FEngSetRotationZ(FEObject *obj, float degrees);
// GC 80150714 / PC 51D070: top-left corner. Image / movie / CBV (1, 7, 9): position - size/2; string (2): font
// justification offset + position (text width/height scaled by size; no font: outputs untouched); group (5):
// untouched; others: position.
void FEngGetTopLeft(const FEObject *obj, float *x, float *y);
// GC 80150868 / PC 51D170: bottom-right corner. Boxes: position + size/2; string: position - offset + extent,
// measuring the localized label (flag 2 clear and found) else the text; everything else (groups too): position.
void FEngGetBottomRight(const FEObject *obj, float *x, float *y);
// GC 80150AC4 / PC 51D380: position so that the top-left corner is (x, y) (inverse of FEngGetTopLeft; string
// without its font: current position kept); GC 801509F0 / PC 51D2D0: same for the bottom-right corner (boxes
// only; everything else takes (x, y)). Types >= 0x100 are left alone; a changed position marks the object dirty,
// then SetTrackValue(position).
void FEngSetTopLeft(FEObject *obj, float x, float y);
void FEngSetBottomRight(FEObject *obj, float x, float y);
// GC 80150C1C / PC 51D4D0: position = (x, y), strings: (x, y) - (justification offset + extent/2) (no font: kept).
void FEngSetCenter(FEObject *obj, float x, float y);
// GC 80150D60 / PC 51D620: position, strings: justification offset + extent/2 + position (no font: untouched).
void FEngGetCenter(const FEObject *obj, float *x, float *y);
// GC 80150E8C / PC 51D730: boxes: size; strings: text extent * size (no font: untouched); others: 0, 0.
void FEngGetSize(const FEObject *obj, float *w, float *h);             // GC 80150F6C / PC 50CB00: data Size (+0x38/+0x3C)

constexpr uint32_t kFEObjectDirty = 0x400000;
