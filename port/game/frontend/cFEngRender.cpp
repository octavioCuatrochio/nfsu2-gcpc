#include "cFEngRender.h"
#include <cstring>
#include "FEObject.h"
#include "FEPackage.h"
#include "FEPackageManager.h"
#include "FERender.h"
#include "FEString.h"
#include "FEngFont.h"
#include "FEMath.h"
#include "bWare/bSlotPool.h"
#include "bridge/Bridge.h"

namespace {
SlotPool *&ObjectPool() { return *bridge::FERenderObjectPool(); }
SlotPool *&PolyPool() { return *bridge::FERenderEPolyPool(); }
SlotPool *&OverflowPool() { return *bridge::FERenderEPolyOverflowPool(); }

// PC 50B970 (__cdecl): the package's render info (its FEPackageData +0x24), null without data.
void *PackageRenderInfo(FEPackage *pkg) { return pkg->mpPackageData ? reinterpret_cast<uint8_t *>(pkg->mpPackageData) + 0x24 : nullptr; }

// A poly goes back to the main pool when it came from there, else to the overflow pool, which is deleted once
// empty (PC: inlined in 525AC0 and 52C3A0).
void FreePoly(FERenderEPoly *p)
{
    if (SlotPool::GetSlotNumber(PolyPool(), p) >= 0) {
        PolyPool()->Free(p);
        return;
    }
    OverflowPool()->Free(p);
    if (OverflowPool()->mi32NumAllocated == 0) {
        bDeleteSlotPool(OverflowPool());
        OverflowPool() = nullptr;
    }
}
}  // namespace

void ePoly::Init()
{
    for (auto &v : mVerts) v[0] = v[1] = v[2] = 0;   // PC: w (+0x0C/+0x1C/+0x2C/+0x3C) left alone
    mVerts[1][0] = 64.0f;
    mVerts[2][0] = 64.0f;
    mVerts[2][1] = 64.0f;
    mVerts[3][1] = 64.0f;
    const float uv[8] = {0, 0, 1, 0, 1, 1, 0, 1};
    std::memcpy(mUVs, uv, sizeof uv);
    for (uint32_t &c : mColours) c = 0x80808080u;
    mu8Flags = 0;
}

FERenderObject *FERenderObject::Construct(FEObject *obj, void *texture)
{
    mpObject = obj;
    mu32Flags = 0;
    mu32RenderCount = 0;
    mpTexture = texture;
    mpPolyHead = mpPolyTail = PolySentinel();
    bIdentity(&mTransform);
    return this;
}

void FERenderObject::FreePolys()
{
    FERenderEPoly *s = PolySentinel();
    while (mpPolyHead != s) {
        FERenderEPoly *p = mpPolyHead;
        p->mpPrev->mpNext = p->mpNext;
        p->mpNext->mpPrev = p->mpPrev;
        FreePoly(p);
    }
}

void FERenderObject::Clear(uint32_t)
{
    FreePolys();
    mu32Flags &= ~2u;
    mu32RenderCount = 0;
}

FERenderEPoly *FERenderObject::AddPoly(float x0, float y0, float x1, float y1, float z, float u0, float v0, float u1,
                                       float v1, const uint32_t *colours, void *, bool flag2)
{
    FERenderEPoly *p;
    if (PolyPool()->mi32NumAllocated != PolyPool()->mi32TotalSlots) {
        p = static_cast<FERenderEPoly *>(PolyPool()->FastMalloc());
    } else {
        if (!OverflowPool()) OverflowPool() = bNewSlotPool(sizeof(FERenderEPoly), 0x200, "FERenderEPolySlotPoolOverflow", 0);
        p = static_cast<FERenderEPoly *>(OverflowPool()->Malloc());
    }
    if (p) p->mPoly.Init();   // PC: a null slot crashes below (same here)
    p->mpTexture = nullptr;
    FERenderEPoly *s = PolySentinel();
    mpPolyTail->mpNext = p;
    p->mpPrev = mpPolyTail;
    mpPolyTail = p;
    p->mpNext = s;
    ePoly &e = p->mPoly;
    const float xs[4] = {x0, x1, x1, x0}, ys[4] = {y0, y0, y1, y1};
    for (int i = 0; i < 4; ++i) {
        e.mVerts[i][0] = xs[i];
        e.mVerts[i][1] = ys[i];
        e.mVerts[i][2] = z;
    }
    for (int i = 0; i < 4; ++i) eMulVector(reinterpret_cast<bVector3 *>(e.mVerts[i]), &mTransform, reinterpret_cast<const bVector3 *>(e.mVerts[i]));
    for (int i = 0; i < 4; ++i) e.mVerts[i][2] = z;   // depth stays untransformed
    const float uv[8] = {u0, v0, u1, v0, u1, v1, u0, v1};
    std::memcpy(e.mUVs, uv, sizeof uv);
    std::memcpy(e.mColours, colours, sizeof e.mColours);
    e.mu8Blend = 1;
    const uint32_t guid = mpObject->mu32GUID;   // PC: hard-coded objects
    if (guid == 0xDBF6E || guid == 0xE0BAE || guid == 0xE0BE8 || guid == 0xE16D8) e.mu8Blend = 0x72;
    if (guid == 0xDCCD1) e.mu8Blend = 0x73;
    if (guid == 0xE093F) {
        const float full[8] = {0, 0, 1, 0, 1, 1, 0, 1};
        std::memcpy(e.mUVs, full, sizeof full);
    }
    if (flag2) e.mu8Flags |= 2;
    return p;
}

void FERenderObject::Render()
{
    void *view = bridge::eGetView(0);
    for (FERenderEPoly *p = mpPolyHead; p != PolySentinel(); p = p->mpNext)
        bridge::eView_Render(view, &p->mPoly, p->mpTexture ? p->mpTexture : mpTexture, mu32RenderCount > 2 ? 1 : 0);
    ++mu32RenderCount;
    mu32Flags |= 2;
}

cFEngRender *cFEngRender::Construct()
{
    vtable = reinterpret_cast<void *>(bridge::cFEngRenderVTable());
    mpCachedHead = mpCachedTail = CachedSentinel();
    ObjectPool() = bNewSlotPool(sizeof(FERenderObject), 0x1C0, "FERenderObjectSlotPool", 0);
    PolyPool() = bNewSlotPool(sizeof(FERenderEPoly), 0x384, "FERenderEPolySlotPool", 0);
    std::memset(mContexts, 0, sizeof mContexts);
    return this;
}

void cFEngRender::Destroy()
{
    vtable = reinterpret_cast<void *>(bridge::cFEngRenderVTable());
    bDeleteSlotPool(ObjectPool());
    ObjectPool() = nullptr;
    bDeleteSlotPool(PolyPool());
    // PC 531900: free what is still cached (the object pool is already gone: bFree(null, slot) like the original)
    while (mpCachedHead != CachedSentinel()) {
        FERenderObject *r = mpCachedHead;
        r->mpPrev->mpNext = r->mpNext;
        r->mpNext->mpPrev = r->mpPrev;
        r->FreePolys();
        bFree(ObjectPool(), r);
    }
}

FERenderObject *cFEngRender::CreateCachedRender(FEObject *obj, void *texture)
{
    auto *r = static_cast<FERenderObject *>(bMalloc(ObjectPool()));
    if (r) r->Construct(obj, texture);
    obj->mpCachedRender = r;
    mpCachedTail->mpNext = r;   // PC: no null check here either
    r->mpPrev = mpCachedTail;
    mpCachedTail = r;
    r->mpNext = CachedSentinel();
    return r;
}

void cFEngRender::RemoveCachedRender(FEObject *obj, uint32_t arg)
{
    FERenderObject *r = obj->mpCachedRender;
    if (!r) return;
    obj->mpCachedRender = nullptr;
    r->mpPrev->mpNext = r->mpNext;
    r->mpNext->mpPrev = r->mpPrev;
    r->Clear(arg);
    r->FreePolys();
    bFree(ObjectPool(), r);
}

void cFEngRender::GenerateRenderContext(uint16_t context, FEObject *group)
{
    cFEngRenderContext &c = mContexts[context];
    MakeRenderMatrix(group->mpData, &c.mMatrix, c.mColour, group->mu16RenderContext, 1.0f);
    c.mbUsed = 1;
    if (c.mpGroup != group) {   // PC then walks the group's child list to its end (no effect)
        c.mpGroup = group;
        c.mpClipObject = nullptr;
    }
}

void cFEngRender::AddToRenderList(FEObject *obj)
{
    long double z = static_cast<const float *>(obj->mpData)[9];   // data +0x24: position z
    if (obj->mu16RenderContext) z += mContexts[obj->mu16RenderContext].mMatrix.m[14];
    int32_t bucket = FEFtol(z);
    if (bucket > 0xFF) bucket = 0xFF;
    else if (bucket < 0) bucket = 0;
    FESortGlobals *g = bridge::FEngSortGlobals();
    if (g->mi32Num >= static_cast<int32_t>(g->mu32Max)) return;
    FERenderNode *n = static_cast<FERenderNode *>(g->mpBuffer) + g->mi32Num;
    ++g->mi32Num;
    if (g->mi32Most < g->mi32Num) g->mi32Most = g->mi32Num;   // PC: kept when most > num
    if (!n) return;
    FERenderBucket &b = bridge::FERenderBuckets()[bucket];
    FERenderNode *tail = b.mpTail;
    b.mpTail = n;
    tail->mpNext = n;
    n->mpNext = b.Sentinel();
    n->mpObject = obj;
}

void cFEngRender::PackageFinished(FEPackage *pkg)
{
    for (cFEngRenderContext &c : mContexts)
        if (c.mbUsed && c.mpClipObject && c.mpGroup && (c.mpGroup->mu32Flags & 0x2000000))
            GenerateClipInfo(&c);
    FERenderBucket *buckets = bridge::FERenderBuckets();
    for (int i = 0xFF; i >= 0; --i) {
        FERenderBucket &b = buckets[i];
        if (b.mpHead == b.Sentinel()) continue;
        for (FERenderNode *n = b.mpHead; n != b.Sentinel(); n = n->mpNext) RenderObject(n->mpObject, PackageRenderInfo(pkg));
        b.mpHead = b.mpTail = b.Sentinel();
    }
}

void cFEngRender::RenderObject(FEObject *obj, void *renderInfo)
{
    if (obj->mu32Type == 7) obj->mu32Flags |= 0x2000000;
    FERenderObject *cached = obj->mpCachedRender;
    const int32_t *mode = bridge::FERenderStatsMode();
    if (mode && *mode == 4) {
        uint32_t *s = bridge::FERenderStats();
        ++s[0];
        if (cached) {
            ++s[1];
            if (cached->mu32Flags & 2) ++s[2];
            if (obj->mu32Flags & 0x2000000) ++s[3];
        }
    }
    if (cached && (cached->mu32Flags & 2) && !(obj->mu32Flags & 0x2000000)) {
        cached->Render();
        return;
    }
    switch (obj->mu32Type) {
    case 1: RenderImage(obj, cached, renderInfo); break;
    case 2: RenderString(obj, cached, renderInfo); break;
    case 7: RenderMovie(obj, cached, renderInfo); break;
    case 9: RenderCBVImage(obj, cached, renderInfo); break;
    default: break;
    }
}

namespace {
using L = long double;
bool Inside(const float *v, int axis, bool greater, float d) { return greater ? v[axis] >= d : v[axis] <= d; }   // NaN: false

void CopyVertex(float *outV, float *outUV, float *outC, const float *inV, const float *inUV, const float *inC)
{
    std::memcpy(outV, inV, 12);
    std::memcpy(outUV, inUV, 8);
    std::memcpy(outC, inC, 16);
}

// ClipGeneral plane value n.v + d. PC: the vertex loop is unrolled 4x and each copy sums in its own order
// (a, b, c: ((a + b) + c) + d with 0 = x, 1 = y, 2 = z); visible at 24-bit precision.
L PlaneValue(const float *n, float d, const float *v, int a = 2, int b = 1, int c = 0)
{
    return ((L(n[a]) * v[a] + L(n[b]) * v[b]) + L(n[c]) * v[c]) + d;
}
// copy 0..3 of the unrolled loop, 4 = remainder loop: inside-test order, intersection numerator order
constexpr int kTestOrder[5][3] = {{2, 1, 0}, {0, 2, 1}, {2, 1, 0}, {2, 1, 0}, {1, 2, 0}};
constexpr int kNumOrder[5][3] = {{2, 1, 0}, {2, 1, 0}, {0, 2, 1}, {2, 1, 0}, {2, 1, 0}};

// out = prev + t (cur - prev); t from the clip axis (axis >= 0) or the plane (x87 roundings as the original)
void Intersect(int axis, float d, const float *plane, float *oV, float *oUV, float *oC, const float *pV, const float *pUV,
               const float *pC, const float *cV, const float *cUV, const float *cC, const int *numOrder = kNumOrder[0])
{
    for (int k = 0; k < 3; ++k) oV[k] = static_cast<float>(L(cV[k]) - pV[k]);
    L t;
    if (axis >= 0) {
        t = (L(d) - pV[axis]) / oV[axis];
    } else {
        const L num = PlaneValue(plane, d, pV, numOrder[0], numOrder[1], numOrder[2]);
        const L den = (L(plane[0]) * oV[0] + L(plane[2]) * oV[2]) + L(plane[1]) * oV[1];
        t = -(num / den);
    }
    for (int k = 0; k < 3; ++k) oV[k] = static_cast<float>(t * oV[k]);
    for (int k = 0; k < 3; ++k) oV[k] = static_cast<float>(L(oV[k]) + pV[k]);
    const float tu = static_cast<float>((L(cUV[0]) - pUV[0]) * t), tv = static_cast<float>((L(cUV[1]) - pUV[1]) * t);
    oUV[0] = static_cast<float>(L(tu) + pUV[0]);
    oUV[1] = static_cast<float>(L(pUV[1]) + tv);
    const float c2 = static_cast<float>(L(cC[2]) - pC[2]), c3 = static_cast<float>(L(cC[3]) - pC[3]);
    const float t0 = static_cast<float>((L(cC[0]) - pC[0]) * t), t1 = static_cast<float>((L(cC[1]) - pC[1]) * t);
    const float t2 = static_cast<float>(L(c2) * t);
    oC[0] = static_cast<float>(L(t0) + pC[0]);
    oC[1] = static_cast<float>(L(pC[1]) + t1);
    oC[2] = static_cast<float>(L(pC[2]) + t2);
    oC[3] = static_cast<float>(L(pC[3]) + t * c3);
}
}  // namespace

namespace {
// One Sutherland-Hodgman pass; axis < 0: against `plane` (inside: value > -0.5).
uint32_t ClipPass(int axis, bool greater, const float *plane, float *outV, float *outUV, float *outC, const float *inV,
                  const float *inUV, const float *inC, uint32_t n, float d)
{
    auto Inside = [&](const float *v, int copy) {
        const int *o = kTestOrder[copy];
        return axis >= 0 ? ::Inside(v, axis, greater, d) : PlaneValue(plane, d, v, o[0], o[1], o[2]) > -0.5L;
    };
    const uint32_t unrolled = n >= 4 ? (n & ~3u) : 0;   // PC: groups of 4 while i + 3 < n, then the remainder loop
    uint32_t count = 0;
    uint32_t prev = n - 1;
    bool prevIn = Inside(inV + prev * 4, 0);
    if (prevIn) {
        CopyVertex(outV, outUV, outC, inV + prev * 4, inUV + prev * 2, inC + prev * 4);
        count = 1;
    }
    for (uint32_t i = 0; i < n; ++i) {
        const float *cv = inV + i * 4, *cuv = inUV + i * 2, *cc = inC + i * 4;
        const float *pv = inV + prev * 4, *puv = inUV + prev * 2, *pc = inC + prev * 4;
        const int copy = i < unrolled ? static_cast<int>(i & 3) : 4;
        const bool in = Inside(cv, copy);
        if (in != prevIn) {
            Intersect(axis, d, plane, outV + count * 4, outUV + count * 2, outC + count * 4, pv, puv, pc, cv, cuv, cc, kNumOrder[copy]);
            ++count;
        }
        if (in) {
            CopyVertex(outV + count * 4, outUV + count * 2, outC + count * 4, cv, cuv, cc);
            ++count;
        }
        prevIn = in;
        prev = i;
    }
    return count;
}
}  // namespace

uint32_t FEClipEdge(int axis, bool greater, float *outV, float *outUV, float *outC, const float *inV, const float *inUV,
                    const float *inC, uint32_t n, float d)
{
    return ClipPass(axis, greater, nullptr, outV, outUV, outC, inV, inUV, inC, n, d);
}

uint32_t FEClipGeneral(const FEClipInfo *clip, float *v, float *uv, float *c, float *tv, float *tuv, float *tc)
{
    uint32_t n = 4;
    for (int i = 0; i < 4; ++i) {
        if (!n) return 0;
        if (i & 1) n = ClipPass(-1, false, clip->mPlanes[i], v, uv, c, tv, tuv, tc, n, clip->mDist[i]);
        else n = ClipPass(-1, false, clip->mPlanes[i], tv, tuv, tc, v, uv, c, n, clip->mDist[i]);
    }
    return n;
}

uint32_t FEClipAligned(const FEClipInfo *clip, float *v, float *uv, float *c, float *tv, float *tuv, float *tc)
{
    uint32_t n = FEClipEdge(0, true, tv, tuv, tc, v, uv, c, 4, clip->mDist[3]);
    if (!n) return 0;
    n = FEClipEdge(1, true, v, uv, c, tv, tuv, tc, n, clip->mDist[0]);
    if (!n) return 0;
    n = FEClipEdge(0, false, tv, tuv, tc, v, uv, c, n, clip->mDist[1]);
    if (!n) return 0;
    return FEClipEdge(1, false, v, uv, c, tv, tuv, tc, n, clip->mDist[2]);
}

void FEQuatToMatrix(const float *q, bMatrix4 *out)
{
    const L x2 = L(q[0]) + q[0], y2 = L(q[1]) + q[1];
    const float z2 = static_cast<float>(L(q[2]) + q[2]);
    const float xx = static_cast<float>(x2 * q[0]), xy = static_cast<float>(y2 * q[0]), xz = static_cast<float>(L(z2) * q[0]);
    const float yy = static_cast<float>(y2 * q[1]), yz = static_cast<float>(L(z2) * q[1]), zz = static_cast<float>(L(z2) * q[2]);
    const float wx = static_cast<float>(x2 * q[3]);
    const L wy = y2 * q[3], wz = L(z2) * q[3];
    float *m = out->m;
    m[12] = m[13] = m[14] = m[3] = m[7] = m[11] = 0.0f;
    m[15] = 1.0f;
    m[0] = static_cast<float>((1.0L - yy) - zz);
    m[4] = static_cast<float>(L(xy) - wz);
    m[8] = static_cast<float>(wy + xz);
    m[1] = static_cast<float>(wz + xy);
    const L oneXX = 1.0L - xx;
    m[5] = static_cast<float>(oneXX - zz);
    m[9] = static_cast<float>(L(yz) - wx);
    m[2] = static_cast<float>(L(xz) - wy);
    m[6] = static_cast<float>(L(wx) + yz);
    m[10] = static_cast<float>(oneXX - yy);
}

void FESortClipCorners(float *c)
{
    if (c[0] > c[4]) {   // PC: test 0x41 (NaN keeps)
        const float lo = c[4], hi = c[0];
        c[4] = hi; c[8] = hi; c[0] = lo; c[12] = lo;
    }
    if (c[1] > c[13]) {
        const float lo = c[13], hi = c[1];
        c[9] = hi; c[13] = hi; c[1] = lo; c[5] = lo;
    }
}

FEClipInfo *cFEngRender::MakeRenderMatrix(const void *data, bMatrix4 *out, int32_t *colour, uint32_t context, float scale)
{
    const float *f = static_cast<const float *>(data);
    const int32_t *ci = static_cast<const int32_t *>(data);
    const uint32_t *bits = static_cast<const uint32_t *>(data);
    const float *pivot = f + 4, *pos = f + 7, *rot = f + 10, *size = f + 14;
    const bool hasPivot = !(pivot[0] == 0.0f && pivot[1] == 0.0f && pivot[2] == 0.0f);
    const bool hasScale = !(bits[14] == 0x3F800000u && bits[15] == 0x3F800000u && bits[16] == 0x3F800000u && scale == 1.0f);
    const bool hasRot = !(rot[0] == 0.0f && rot[1] == 0.0f && rot[2] == 0.0f && bits[13] == 0x3F800000u);
    bMatrix4 t, pPos, pNeg, sc, r;
    bIdentity(&t);
    t.m[12] = pos[0]; t.m[13] = pos[1]; t.m[14] = pos[2];
    if (hasPivot) {
        bIdentity(&pPos);
        bIdentity(&pNeg);
        pNeg.m[12] = -pivot[0]; pNeg.m[13] = -pivot[1]; pNeg.m[14] = -pivot[2];
        pPos.m[12] = pivot[0]; pPos.m[13] = pivot[1]; pPos.m[14] = pivot[2];
    }
    if (hasScale) {
        bIdentity(&sc);
        sc.m[0] = static_cast<float>(L(scale) * size[0]);
        sc.m[5] = static_cast<float>(L(scale) * size[1]);
        sc.m[10] = static_cast<float>(L(scale) * size[2]);
    }
    if (hasRot) {
        bIdentity(&r);
        const float q[4] = {rot[0], rot[1], rot[2], rot[3]};
        FEQuatToMatrix(q, &r);
    }
    bIdentity(out);
    bMulMatrix(out, out, &t);
    if (hasRot) {
        if (hasPivot) bMulMatrix(out, out, &pPos);
        bMulMatrix(out, out, &r);
        if (hasPivot) bMulMatrix(out, out, &pNeg);
    }
    if (hasScale) bMulMatrix(out, out, &sc);
    if (!context) {
        for (int i = 0; i < 4; ++i) colour[i] = ci[i];
        return nullptr;
    }
    cFEngRenderContext &c = mContexts[context];
    for (int i = 0; i < 4; ++i)
        colour[i] = static_cast<int32_t>(static_cast<uint32_t>(c.mColour[i]) * static_cast<uint32_t>(ci[i]) + 0x80u) >> 8;
    bMulMatrix(out, &c.mMatrix, out);
    return c.mpClipObject ? &c.mClip : nullptr;
}

void cFEngRender::GenerateClipInfo(cFEngRenderContext *ctx)
{
    bMatrix4 view, m;
    bIdentity(&view);
    view.m[12] = 320.0f; view.m[13] = 240.0f; view.m[14] = 0.0f;
    FEObject *clipObj = ctx->mpClipObject;
    int32_t colour[4];
    MakeRenderMatrix(clipObj->mpData, &m, colour, clipObj->mu16RenderContext, 1.0f);
    bMulMatrix(&m, &view, &m);
    if (clipObj->mu32Type != 1 && clipObj->mu32Type != 9) return;
    float c[16] = {-0.5f, -0.5f, 0, 0, 0.5f, -0.5f, 0, 0, 0.5f, 0.5f, 0, 0, -0.5f, 0.5f, 0, 0};
    for (int i = 0; i < 4; ++i) {
        eMulVector(reinterpret_cast<bVector3 *>(c + i * 4), &m, reinterpret_cast<const bVector3 *>(c + i * 4));
        c[i * 4 + 2] = 0;
    }
    FEClipInfo &ci = ctx->mClip;
    ci.mu32Flags &= ~1u;
    const L eps = 0.001f;   // PC 0x799364
    auto near = [&](float a, float b) { return !(L(a) + eps < b) && L(a) - eps <= b; };   // PC: fcom / test 0x41 (NaN: not near)
    // aligned as is: x0~x3, x1~x2, y0~y1, y2~y3; or rotated by one corner: x0~x1, x2~x3, y0~y3, y1~y2
    if (near(c[0], c[12]) && near(c[4], c[8]) && near(c[1], c[5]) && near(c[9], c[13])) {
        ci.mu32Flags |= 1;
        FESortClipCorners(c);
    } else if (near(c[0], c[4]) && near(c[8], c[12]) && near(c[1], c[13]) && near(c[5], c[9])) {
        ci.mu32Flags |= 1;
        float r[16];
        std::memcpy(r, c + 4, 12 * sizeof(float));
        r[12] = c[0]; r[13] = c[1]; r[14] = c[2]; r[15] = c[3];
        std::memcpy(c, r, sizeof r);
        FESortClipCorners(c);
    }
    for (int i = 0; i < 4; ++i) {
        const float *a = c + i * 4, *b = c + ((i + 1) & 3) * 4;
        const float ex = static_cast<float>(L(b[0]) - a[0]), ey = static_cast<float>(L(b[1]) - a[1]);
        float *pl = ci.mPlanes[i];
        pl[2] = 0;
        pl[1] = ex;
        pl[0] = -ey;
        bNormalize(reinterpret_cast<bVector3 *>(pl), reinterpret_cast<const bVector3 *>(pl));
        ci.mDist[i] = static_cast<float>(-(((L(pl[2]) * a[2]) + L(pl[0]) * a[0]) + L(pl[1]) * a[1]));
    }
    if (ci.mu32Flags & 1) {
        ci.mDist[3] = c[0];
        ci.mDist[0] = c[5];
        ci.mDist[1] = c[8];
        ci.mDist[2] = c[13];
    }
}

void FERenderObject::AddPolyClipped(float x0, float y0, float x1, float y1, float z, float u0, float v0, float u1, float v1,
                                    const uint32_t *colours, FEClipInfo *clip, void *renderInfo)
{
    if (!clip) {
        AddPoly(x0, y0, x1, y1, z, u0, v0, u1, v1, colours, renderInfo, false);
        return;
    }
    // PC buffers hold 8 entries (a longer result overruns them); 16 here
    float V[16 * 4] = {}, UV[16 * 2] = {}, C[16 * 4] = {}, TV[16 * 4] = {}, TUV[16 * 2] = {}, TC[16 * 4] = {};
    const float xs[4] = {x0, x1, x1, x0}, ys[4] = {y0, y0, y1, y1};
    for (int i = 0; i < 4; ++i) {
        V[i * 4 + 0] = xs[i];
        V[i * 4 + 1] = ys[i];
        V[i * 4 + 2] = z;
    }
    const float uv[8] = {u0, v0, u1, v0, u1, v1, u0, v1};
    std::memcpy(UV, uv, sizeof uv);
    for (int i = 0; i < 4; ++i) {
        const uint8_t *b = reinterpret_cast<const uint8_t *>(colours + i);
        for (int k = 0; k < 4; ++k) C[i * 4 + k] = static_cast<float>(L(b[k]) * L(0.003921568859368563f));
    }
    for (int i = 0; i < 4; ++i) eMulVector(reinterpret_cast<bVector3 *>(V + i * 4), &mTransform, reinterpret_cast<const bVector3 *>(V + i * 4));
    const uint32_t n = (clip->mu32Flags & 1) ? FEClipAligned(clip, V, UV, C, TV, TUV, TC) : FEClipGeneral(clip, V, UV, C, TV, TUV, TC);
    if (!n) return;
    uint32_t packed[16];
    for (uint32_t i = 0; i < n && i < 16; ++i) {
        uint8_t *b = reinterpret_cast<uint8_t *>(packed + i);
        for (int k = 0; k < 4; ++k) b[k] = static_cast<uint8_t>(FEFtol(L(TC[i * 4 + k]) * 255.0L));
    }
    const uint32_t tris = n - 2;   // PC: unsigned (n == 1 would run away; never produced)
    for (uint32_t k = 0; k < tris && k + 2 < 16; ++k) {
        FERenderEPoly *p;
        if (PolyPool()->mi32NumAllocated != PolyPool()->mi32TotalSlots) {
            p = static_cast<FERenderEPoly *>(PolyPool()->FastMalloc());
        } else {
            if (!OverflowPool()) OverflowPool() = bNewSlotPool(sizeof(FERenderEPoly), 0x200, "FERenderEPolySlotPoolOverflow", 0);
            p = static_cast<FERenderEPoly *>(OverflowPool()->Malloc());
        }
        if (p) p->mPoly.Init();
        p->mpTexture = nullptr;
        p->mpPrev = mpPolyTail;
        mpPolyTail->mpNext = p;
        mpPolyTail = p;
        p->mpNext = PolySentinel();
        ePoly &e = p->mPoly;
        const uint32_t idx[4] = {0, k + 1, k + 2, k + 2};
        for (int j = 0; j < 4; ++j) {
            std::memcpy(e.mVerts[j], V + idx[j] * 4, 12);
            std::memcpy(e.mUVs[j], UV + idx[j] * 2, 8);
            e.mColours[j] = packed[idx[j]];
        }
        e.mu8Blend = 1;
    }
}

namespace {
// PC: smallest power of two >= v, but 2 for 1 and 0 for 0
uint32_t TexturePow2(int32_t v)
{
    if (!v) return 0;
    uint32_t e = static_cast<uint32_t>(v - 1) >> 1, c = 2;
    while (e) { e >>= 1; c <<= 1; }
    return c;
}
L Unsigned(uint32_t v) { return static_cast<L>(v); }   // PC: fild + 2^32 when negative
uint32_t PackColour(const int32_t *c)
{
    const uint32_t r = static_cast<uint32_t>(c[0] / 2), g = static_cast<uint32_t>(c[1] / 2), b = static_cast<uint32_t>(c[2] / 2);
    return ((((r | (static_cast<uint32_t>(c[3]) << 8)) << 8) | g) << 8) | b;
}
struct TexDims { int16_t w, h; };
TexDims Dims(const void *tex) { const auto *p = static_cast<const uint8_t *>(tex); TexDims d; std::memcpy(&d.w, p + 0x44, 2); std::memcpy(&d.h, p + 0x46, 2); return d; }
void ScreenMatrix(bMatrix4 *view) { bIdentity(view); view->m[12] = 320.0f; view->m[13] = 240.0f; view->m[14] = 0.0f; }
}  // namespace

void cFEngRender::RenderImage(FEObject *obj, FERenderObject *cached, void *renderInfo)
{
    const float *data = static_cast<const float *>(obj->mpData);
    bMatrix4 view, m;
    ScreenMatrix(&view);
    int32_t colour[4];
    FEClipInfo *clip = MakeRenderMatrix(data, &m, colour, obj->mu16RenderContext, 1.0f);
    bMulMatrix(&m, &view, &m);
    void *tex = bridge::GetTextureInfo(obj->_unknown24, 1, 0);
    if (!cached) cached = CreateCachedRender(obj, tex);
    else cached->Clear(reinterpret_cast<uintptr_t>(renderInfo));
    const TexDims d = Dims(tex);
    const L w = Unsigned(static_cast<uint32_t>(static_cast<int32_t>(d.w)));
    const float h = static_cast<float>(Unsigned(static_cast<uint32_t>(static_cast<int32_t>(d.h))));
    const L wr = w / Unsigned(TexturePow2(d.w)), hr = L(h) / Unsigned(TexturePow2(d.h));
    const uint32_t packed = PackColour(colour);
    const uint32_t colours[4] = {packed, packed, packed, packed};
    const float u0 = static_cast<float>(wr * data[17]), u1 = static_cast<float>(wr * data[19]);
    const float v0 = static_cast<float>(hr * data[18]), v1 = static_cast<float>(hr * data[20]);
    cached->mTransform = m;
    cached->AddPolyClipped(-0.5f, -0.5f, 0.5f, 0.5f, 1.0f, u0, v0, u1, v1, colours, clip, renderInfo);
    cached->mpTexture = tex;
    cached->Render();
}

void cFEngRender::RenderCBVImage(FEObject *obj, FERenderObject *cached, void *renderInfo)
{
    const float *data = static_cast<const float *>(obj->mpData);
    const int32_t *ci = static_cast<const int32_t *>(obj->mpData);
    bMatrix4 view, m;
    ScreenMatrix(&view);
    int32_t colour[4];
    FEClipInfo *clip = MakeRenderMatrix(data, &m, colour, obj->mu16RenderContext, 1.0f);
    bMulMatrix(&m, &view, &m);
    void *tex = bridge::GetTextureInfo(obj->_unknown24, 1, 0);
    if (!cached) cached = CreateCachedRender(obj, tex);
    else cached->Clear(reinterpret_cast<uintptr_t>(renderInfo));
    const TexDims d = Dims(tex);
    const L w = Unsigned(static_cast<uint32_t>(static_cast<int32_t>(d.w)));
    const float h = static_cast<float>(Unsigned(static_cast<uint32_t>(static_cast<int32_t>(d.h))));
    const L wr = w / Unsigned(TexturePow2(d.w)), hr = L(h) / Unsigned(TexturePow2(d.h));
    const uint32_t colours[4] = {PackColour(ci + 21), PackColour(ci + 25), PackColour(ci + 29), PackColour(ci + 33)};   // data +0x54 ..
    const float u0 = static_cast<float>(wr * data[17]), u1 = static_cast<float>(wr * data[19]);
    const float v0 = static_cast<float>(hr * data[18]), v1 = static_cast<float>(hr * data[20]);
    cached->mTransform = m;
    cached->AddPolyClipped(-0.5f, -0.5f, 0.5f, 0.5f, 1.0f, u0, v0, u1, v1, colours, clip, renderInfo);
    cached->mpTexture = tex;
    cached->Render();
}

void cFEngRender::RenderMovie(FEObject *obj, FERenderObject *cached, void *renderInfo)
{
    if (!bridge::MovieActive()) return;
    void *tex = bridge::MoviePlayer() ? bridge::MovieTexture() : bridge::GetTextureInfo(0, 1, 0);
    bMatrix4 view, m;
    ScreenMatrix(&view);
    int32_t colour[4];
    FEClipInfo *clip = MakeRenderMatrix(obj->mpData, &m, colour, obj->mu16RenderContext, 1.0f);
    bMulMatrix(&m, &view, &m);
    if (!cached) cached = CreateCachedRender(obj, tex);
    else cached->Clear(reinterpret_cast<uintptr_t>(renderInfo));
    const uint32_t packed = PackColour(colour);
    const uint32_t colours[4] = {packed, packed, packed, packed};
    const TexDims d = Dims(tex);
    const L hr = Unsigned(static_cast<uint32_t>(static_cast<int32_t>(d.h))) / Unsigned(TexturePow2(d.h));   // PC also rounds the width up, unused
    float v0, v1;
    if (bridge::MoviePlayer()[0xC]) {   // PC: no null check
        v0 = static_cast<float>(hr - L(0.001f));
        v1 = 0.001f;
    } else {
        v0 = 0.0f;
        v1 = static_cast<float>(hr);
    }
    cached->mTransform = m;
    cached->AddPolyClipped(-0.5f, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, v0, 1.0f, v1, colours, clip, renderInfo);
    cached->mpTexture = tex;
    cached->Render();
}

void cFEngRender::RenderString(FEObject *obj, FERenderObject *cached, void *renderInfo)
{
    auto *str = reinterpret_cast<FEString *>(obj);
    void *font = bridge::FEngFindFont(obj->_unknown24);
    if (!font) return;
    void *fontTex = *reinterpret_cast<void **>(static_cast<uint8_t *>(font) + 8);
    if (!fontTex) return;
    if (!cached) cached = CreateCachedRender(obj, fontTex);
    else cached->Clear(reinterpret_cast<uintptr_t>(renderInfo));
    float scale = 1.0f;
    const int32_t lang = bridge::CurrentLanguage();
    if ((lang == 8 || lang == 9) && obj->_unknown24 == 0x9583AA1Au) scale = 1.6f;
    bMatrix4 view, m;
    ScreenMatrix(&view);
    int32_t colour[4];
    MakeRenderMatrix(obj->mpData, &m, colour, obj->mu16RenderContext, scale);
    bMulMatrix(&m, &view, &m);
    static uint16_t buffer[0x400];   // PC: 0x800 bytes on the stack
    const uint16_t *text = str->mstrText.mpsString;
    if (!(obj->mu32Flags & 2) && bridge::FEngGetLocalizedString(buffer, 0x800, str->mu32LabelHash)) text = buffer;
    static_cast<FEngFont *>(font)->RenderString(colour, text, obj, &m, cached, renderInfo);
}

FERenderEPoly *FERenderObject::AddPolyTextured(float x0, float y0, float x1, float y1, float z, float u0, float v0,
                                               float u1, float v1, const uint32_t *colours, void *texture, void *renderInfo)
{
    FERenderEPoly *p = AddPoly(x0, y0, x1, y1, z, u0, v0, u1, v1, colours, renderInfo, false);
    if (p) p->mpTexture = texture;
    return p;
}
