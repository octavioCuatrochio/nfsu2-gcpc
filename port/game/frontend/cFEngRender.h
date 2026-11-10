// Frontend render glue (GC zFe2.cpp cFEngRender / FERenderObject / FERenderEPoly). The PC version differs from
// GC: every visible FEObject keeps a cached FERenderObject (FEObject +0x58) holding engine ePolys that eView
// draws. Engine-side pieces (eView, texture lookup, fonts) go through the bridge.
#pragma once
#include <cstddef>
#include <cstdint>
#include "bWare/bMath.h"

struct FEObject;
struct FEPackage;
struct FEClipInfo;

// Engine ePoly (PC 0x80): 4 vertices (x, y, z, w), 4 UVs, 4 colours (0xAARRGGBB, 0x80 = 1.0), flags.
struct ePoly {
    float mVerts[4][4];        // +0x00
    float mUVs[4][2];          // +0x40
    uint32_t mColours[4];      // +0x60
    uint8_t mu8Flags;          // +0x70 2: set by AddPoly's last argument
    uint8_t mu8Blend;          // +0x71 1 normal; 0x72 / 0x73 for a few hard-coded objects (GUIDs)
    uint8_t _unknown72[0xE];
    void Init();               // PC 5BB1C0 (engine ctor): 64x64 quad at the origin, full UVs, grey, flags 0
};

// GC FERenderEPoly (PC 0xA0, slot pools 0x838480 "FERenderEPoly" (900) / overflow 0x838484 (512, on demand)).
struct FERenderEPoly {
    FERenderEPoly *mpNext;     // +0x00 FERenderObject list (sentinel = the object's +0x18)
    FERenderEPoly *mpPrev;     // +0x04
    uint32_t _unknown08[2];
    ePoly mPoly;               // +0x10
    void *mpTexture;           // +0x90 TextureInfo override (null: the object's)
    uint32_t _unknown94[3];
};

// GC FERenderObject (PC 0x60, slot pool 0x83847C "FERenderObject" (448)): an FEObject's cached polygons.
struct FERenderObject {
    FERenderObject *mpNext;    // +0x00 cFEngRender cached list
    FERenderObject *mpPrev;    // +0x04
    FEObject *mpObject;        // +0x08
    uint32_t mu32Flags;        // +0x0C 2: built (Render sets, Clear clears)
    uint32_t mu32RenderCount;  // +0x10
    void *mpTexture;           // +0x14 TextureInfo
    FERenderEPoly *mpPolyHead; // +0x18 list sentinel {head, tail}
    FERenderEPoly *mpPolyTail; // +0x1C
    bMatrix4 mTransform;       // +0x20

    FERenderObject *Construct(FEObject *obj, void *texture);   // GC 80142B28 / PC 52C330 (ret 8): identity
    void FreePolys();          // GC ~FERenderObject / PC 52C390 (-> 525AC0): every poly back to its pool
    void Clear(uint32_t unused);   // GC 80143554 / PC 52C3A0 (ret 4): FreePolys, flag 2 off, render count 0
    // GC 80142CF8 / PC 519FC0 (ret 0x30): quad (x0,y0)-(x1,y1) at depth z through mTransform (z kept), UVs
    // (u0,v0)-(u1,v1), 4 colours; appended. `renderInfo` unused on PC.
    FERenderEPoly *AddPoly(float x0, float y0, float x1, float y1, float z, float u0, float v0, float u1, float v1,
                           const uint32_t *colours, void *renderInfo, bool flag2);
    // GC 80142FC4 (GC-only, used by FEngFont::RenderJoyEventTexture): AddPoly(.., false) with a texture override.
    FERenderEPoly *AddPolyTextured(float x0, float y0, float x1, float y1, float z, float u0, float v0, float u1,
                                   float v1, const uint32_t *colours, void *texture, void *renderInfo);
    // GC 80143044 / PC 51A1C0 (ret 0x30): without clip info: AddPoly(.., renderInfo, false). Else the quad
    // (transformed, z too) is clipped (ClipAligned when clip flag 1, else ClipGeneral) and fanned into quads
    // (v0, v[k+1], v[k+2], v[k+2]); colours become floats (/255) for the clip and bytes (*255, _ftol) after, taken
    // from the clip's scratch colour array (PC quirk: the 3rd pass's output).
    void AddPolyClipped(float x0, float y0, float x1, float y1, float z, float u0, float v0, float u1, float v1,
                        const uint32_t *colours, FEClipInfo *clip, void *renderInfo);
    void Render();             // GC 801434B8 / PC 51A5D0: eView 0 draws every poly; count++, flag 2
    FERenderEPoly *PolySentinel() { return reinterpret_cast<FERenderEPoly *>(&mpPolyHead); }
};

// Clip region of a render context (PC 0x54 at context +0x60): 4 edge planes (normal x, y, z; +0xC unused) and their
// distances; flag 1 = axis-aligned rectangle (then d3 = x min, d0 = y min, d1 = x max, d2 = y max).
struct FEClipInfo {
    float mPlanes[4][4];       // +0x00
    float mDist[4];            // +0x40
    uint32_t mu32Flags;        // +0x50
};

// One render context (PC 0xD0; 64 of them in cFEngRender from +0x20).
struct cFEngRenderContext {
    int32_t mColour[4];        // +0x00 colour modulation (x/256)
    bMatrix4 mMatrix;          // +0x10 group transform (MakeRenderMatrix)
    FEObject *mpGroup;         // +0x50
    FEObject *mpClipObject;    // +0x54 image whose quad clips the group (reset when the group changes)
    uint32_t _unknown58[2];
    FEClipInfo mClip;          // +0x60
    uint8_t _unknownB4[0xC];
    uint8_t mbUsed;            // +0xC0
    uint8_t _unknownC1[0xF];
};

// Clipping (PC-only helpers; GC FERenderObject::ClipAligned / ClipGeneral). Vertices are 16 bytes (x, y, z, -),
// UVs 8, colours 4 floats. Edge clippers (Sutherland-Hodgman, PC 506CB0 x >= d, 507D80 y >= d, 508E60 x <= d,
// 509F30 y <= d; NaN = outside): start with the last vertex (emitted if inside), then each vertex in order; the
// last vertex is emitted again when inside (PC). Returns the output count.
uint32_t FEClipEdge(int axis, bool greater, float *outV, float *outUV, float *outC, const float *inV, const float *inUV,
                    const float *inC, uint32_t n, float d);
// PC 50B010 (thiscall, `this` unused, ret 0x1C): the four edges (x >= d3, y >= d0, x <= d1, y <= d2), ping-ponging
// between the in (v, uv, c) and scratch arrays; the result ends in the in arrays (0 when any pass empties).
uint32_t FEClipAligned(const FEClipInfo *clip, float *v, float *uv, float *c, float *tv, float *tuv, float *tc);
// PC 505660 (thiscall, `this` unused, ret 0x1C): the four planes in order (inside: ((nz z + ny y) + nx x) + d > -0.5;
// t = -(value(prev) / ((nx dx + nz dz) + ny dy))), same ping-pong and output as FEClipAligned.
uint32_t FEClipGeneral(const FEClipInfo *clip, float *v, float *uv, float *c, float *tv, float *tuv, float *tc);
// PC 4FD980 (thiscall on the quaternion, ret 4): rotation matrix (x87 order differs from FEQuatGetMatrix 504A50).
void FEQuatToMatrix(const float *q, bMatrix4 *out);
// PC 50B3C0 (__cdecl): axis-aligned quad: x min to corners 0/3, max to 1/2; y min to 0/1, max to 2/3.
void FESortClipCorners(float *corners);

// Depth-sort buckets (PC 0x8639A0: 256 x {head, tail}, circular through the sentinel) and their nodes (from the
// FESortGlobals buffer).
struct FERenderNode {
    FERenderNode *mpNext;
    FEObject *mpObject;
};
struct FERenderBucket {
    FERenderNode *mpHead;
    FERenderNode *mpTail;
    FERenderNode *Sentinel() { return reinterpret_cast<FERenderNode *>(this); }
};

// GC cFEngRender (PC 0x3420, vtable 0x79BBA0 = scalar dtor 5360E0).
struct cFEngRender {
    void *vtable;                        // +0x00
    uint32_t _unknown04[3];
    FERenderObject *mpCachedHead;        // +0x10 list sentinel
    FERenderObject *mpCachedTail;        // +0x14
    uint32_t _unknown18[2];
    cFEngRenderContext mContexts[64];    // +0x20

    // GC 80148D98 (+ FERenderObject::Initialize) / PC 536050: pools, empty list, contexts zeroed.
    cFEngRender *Construct();
    void Destroy();                      // GC 8030DF24 / PC 536100 (+ 531900 cached list, + Terminate)
    FERenderObject *CachedSentinel() { return reinterpret_cast<FERenderObject *>(&mpCachedHead); }
    // PC 52CBA0 (ret 8; GC CreateCachedRender): new FERenderObject for obj (+0x58), appended to the cached list.
    FERenderObject *CreateCachedRender(FEObject *obj, void *texture);
    // GC 80149F38 / PC 5371A0 (ret 8): drop obj's cached render (Clear(arg), free polys, back to the pool).
    void RemoveCachedRender(FEObject *obj, uint32_t arg);
    // GC 8014A018 / PC 51B7F0 (ret 8): context transform from the group; group change resets +0x54.
    void GenerateRenderContext(uint16_t context, FEObject *group);
    // GC 8014A1FC / PC 51B850 (ret 4): append to the bucket of clamp(_ftol(z [+ context z]), 0, 255).
    void AddToRenderList(FEObject *obj);
    // GC 8014A104 / PC 5371F0 (ret 4): clip info for used clipping groups (flag 0x2000000), then render the
    // buckets far (255) to near and empty them.
    void PackageFinished(FEPackage *pkg);
    // GC 80149DA4 / PC 5370A0 (ret 8): movies get flag 0x2000000; a built cached render is reused, else by type
    // (1 image, 2 string, 7 movie, 9 CBV image).
    void RenderObject(FEObject *obj, void *renderInfo);
    // GC 80148E78 / PC 51B0B0 (ret 0x14): out = T(position) [x P(pivot) R(rotation) P(-pivot)] [x S(size * scale)]
    // (each part only when not neutral), colour = data colour; with a context: colour = (ctx * data + 0x80) >> 8,
    // out x= context matrix, returns the context's clip info when it has a clip object. Else null.
    FEClipInfo *MakeRenderMatrix(const void *data, bMatrix4 *out, int32_t *colour, uint32_t context, float scale);
    // GC 80149858 / PC 51B3A0 (ret 4): clip planes from the clip object's quad (image / CBV only) on screen.
    void GenerateClipInfo(cFEngRenderContext *ctx);
    // Per-type renderers (ret 0xC; `cached` null: created): a unit quad (-0.5..0.5, z 1) through the object's
    // screen matrix (MakeRenderMatrix x translation (320, 240)), then drawn.
    void RenderImage(FEObject *obj, FERenderObject *cached, void *renderInfo);      // GC 80149198 / PC 536A50
    void RenderCBVImage(FEObject *obj, FERenderObject *cached, void *renderInfo);   // GC 801493F0 / PC 536C80: corner colours
    void RenderMovie(FEObject *obj, FERenderObject *cached, void *renderInfo);      // GC 80149178 / PC 536850
    void RenderString(FEObject *obj, FERenderObject *cached, void *renderInfo);     // GC 801496C8 / PC 536F60
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(ePoly) == 0x80 && sizeof(FERenderEPoly) == 0xA0 && sizeof(FERenderObject) == 0x60, "render sizes");
static_assert(sizeof(cFEngRenderContext) == 0xD0 && sizeof(cFEngRender) == 0x3420, "cFEngRender size");
#endif
