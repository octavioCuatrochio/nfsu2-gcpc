// FEng free helper functions used by game screens and widgets (GC zFe2.cpp "FEng*" API). All __cdecl on PC.
#pragma once
#include <cstdint>

struct FEObject;
struct FEPackage;

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
// GC 8014FB1C / PC 50C900 (__cdecl): FEString::SetLabelHash, then clear flag 2. PC only: if flag 2 was set before,
// the object is marked again (0x400000).
void FEngSetLanguageHash(FEObject *str, uint32_t hash);
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

// Package-level helpers (cFEng::pInstance / the global FEPackageManager; PC inlines the FEPkgMgr_* wrappers).
void FEngSetUseIdleList(const char *pkg, bool use);                      // GC 8014F69C / PC 51CD10: package +0x15
void FEngSetPermanent(const char *pkg, int32_t permanent);               // GC 8014F6D8 / PC 51CD40 (low 16 bits)
FEObject *FEngGetCurrentButton(const char *pkg);                         // GC 8014F778 / PC 51CD60
void FEngSetButtonState(const char *pkg, uint32_t button, bool enable);  // GC 8014F7B0 / PC 51CDA0
// GC 8014F7F4 / PC 51CDC0: FEngine::QueueMessage(msg, obj, pkg, target, 0) when FEng runs.
void FEngPostMessageToFEng(uint32_t msg, FEObject *obj, FEObject *target, FEPackage *pkg);
// GC 8014F844 / PC 51CDF0: QueueMessage(msg, null, package, -1, arg) if the package is loaded.
void FEngQueueControllerMessageToPackage(uint32_t msg, const char *pkg, uint32_t arg);
void FEngQueueMessageToPackage(uint32_t msg, const char *pkg);           // GC 8014F8E0 / PC 51CE60 (arg 0)
void FEngSetPackageVisibility(const char *pkg, uint8_t visible);         // GC 80150274 / PC 51CE80

// Object lookup by package name + object name hash: the loaded package first, else FEngine::FindObject (also
// when no package name). GC 8014FA3C / PC 5379C0; FEngFindImage GC 8014FAE4 / PC 537A60 (only type 1).
FEObject *FEngFindObject(const char *pkg, uint32_t objHash);
FEObject *FEngFindImage(const char *pkg, uint32_t objHash);
FEObject *FEngFindString(const char *pkg, uint32_t objHash);   // GC 8014FAAC / PC 537A10 (only type 2)
// GC 801503E8 (name variant) / PC 542630: position track of the named object (types < 0x100; dirty on change).
void FEngSetPosition(const char *pkg, const char *obj, const struct FEVector3 *pos);
// FEngSetScript lookups: PC 5426C0 (object name, script name), 537C00 (object hash, script name), 537C60
// (object hash, script id); a missing object reaches FEngSetScript(null, ...), which does nothing.
void FEngSetScript(const char *pkg, const char *obj, const char *script, bool restart);
void FEngSetScript(const char *pkg, uint32_t objHash, const char *script, bool restart);
void FEngSetScript(const char *pkg, uint32_t objHash, uint32_t scriptId, bool restart);

void FEngSetCurrentButton(const char *pkg, uint32_t buttonHash);         // GC 8014F6F8 / PC 52CEA0
void FEngSetCurrentButton(const char *pkg, const FEObject *button);      // GC 8014F734 / PC 52CEC0 (null: nothing)

struct FERect { float left, top, right, bottom; };
// PC 51D8A0 (GC 80152F64 FEngGetObjBBox, PC differs): string objects only. topLeft = position + justification
// multiplier (flags +0x6C: 1 -> -0.5 / 2 -> -1 for x, 4 -> -0.5 / 8 -> -1 for y, else 0) * extent; extent =
// localized-or-own text width/height * size. Both zeroed first; no font: stays zero.
void FEngGetObjBBox(const FEObject *obj, float *topLeft, float *extent);
// GC 80151384 / PC 51D9F0: screen rectangle. Types 1, 3, 4, 6, 7, 9, 10, 11: top-left / bottom-right (8: none); strings: bbox (false
// when the extent is 0 in x or y); groups: union of the children's rectangles moved by the group position.
bool FEngGet2DExtents(const FEObject *obj, FERect *r);
// GC 80151280 / PC 52D020: (x, y) inside the object's rectangle (edges included; NaN: false).
bool FEngTestForIntersection(float x, float y, const FEObject *obj);
// GC overload (position, size) / PC 50CD00 (__cdecl): left <= x <= left + w, top <= y <= top + h (edges rounded to
// float; NaN fails)
bool FEngTestForIntersection(float x, float y, const float *pos, const float *size);
