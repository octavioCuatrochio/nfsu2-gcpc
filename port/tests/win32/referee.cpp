// Differential referee: built as a dinput8.dll that SPEED2.EXE loads at startup. Its DllMain runs before
// the game's entry point -- the exe is mapped at 0x400000 by the real loader, imports are resolved, but the
// game's runtime (CRT heap, globals) is NOT initialized -- calls ORIGINAL functions and their REWRITES with
// the same random inputs, writes referee.log and exits the process. Only for self-contained functions.
//
// Run: tests/win32/run_referee.sh   (copies the exe and this DLL to a temp dir; never touches the game)
// Add a check: write a Check...() comparing ORIG(addr) against the rewrite, call it from RunAll().
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>
#include <initializer_list>
#include <utility>
#include "bWare/bCrc32.h"
#include "bWare/bString.h"
#include "bWare/bWideString.h"
#include "frontend/FEHash.h"
#include "frontend/FEMinList.h"
#include "frontend/FEObject.h"
#include "frontend/FEMultiPool.h"
#include "frontend/FEPool.h"
#include "frontend/FEScript.h"
#include "frontend/FEMath.h"
#include "frontend/FEPackage.h"
#include "bWare/bMath.h"
#include <cmath>
#include "bridge_stub.h"
#define THISCALL __attribute__((thiscall))

static std::mt19937 rng(12345);
static int failures = 0, checks = 0;

template <class F> static F ORIG(uintptr_t addr) { return reinterpret_cast<F>(addr); }

#define EXPECT(cond, ...) do { ++checks; if (!(cond)) { if (++failures <= 20) { std::printf("MISMATCH " __VA_ARGS__); std::printf("\n"); } } } while (0)

static std::string RandomText(int maxLen, bool highBytes)
{
    std::string s;
    int n = rng() % (maxLen + 1);
    for (int i = 0; i < n; ++i) {
        int c = highBytes ? 1 + rng() % 255 : 32 + rng() % 95;
        s.push_back(static_cast<char>(c));
    }
    return s;
}

static void CheckCrc(int iters)
{
    auto orig = ORIG<uint32_t(__cdecl *)(const void *, int, uint32_t)>(0x43BCD0);
    for (int i = 0; i < iters; ++i) {
        std::vector<uint8_t> data(rng() % 300);
        for (auto &b : data) b = rng();
        int len = static_cast<int>(data.size()) - (i % 7 == 0 ? 400 : 0);   // also negative lengths
        uint32_t seed = (i % 3 == 0) ? 0xFFFFFFFFu : rng();
        uint32_t a = orig(data.data(), len, seed), b = bCalculateCrc32(data.data(), len, seed);
        EXPECT(a == b, "bCalculateCrc32 len=%d seed=%08X: orig %08X rewrite %08X", len, seed, a, b);
    }
}

static void CheckStringHash(int iters)
{
    auto orig1 = ORIG<uint32_t(__cdecl *)(const char *)>(0x43DB50);
    auto orig2 = ORIG<uint32_t(__cdecl *)(const char *, uint32_t)>(0x43DB80);
    for (int i = 0; i < iters; ++i) {
        std::string s = RandomText(64, true);
        uint32_t seed = rng();
        EXPECT(orig1(s.c_str()) == bStringHash(s.c_str()), "bStringHash(\"%s\")", s.c_str());
        EXPECT(orig2(s.c_str(), seed) == bStringHash(s.c_str(), seed), "bStringHash(\"%s\", %08X)", s.c_str(), seed);
        auto origUpper = ORIG<uint32_t(__cdecl *)(const char *)>(0x505450);
        EXPECT(origUpper(s.c_str()) == FEHashUpper(s.c_str()), "FEHashUpper(\"%s\")", s.c_str());
    }
}

static void CheckWideCopies(int iters)
{
    auto origCopy = ORIG<void(__cdecl *)(uint16_t *, const uint16_t *)>(0x511630);
    auto origStrCpy = ORIG<uint16_t *(__cdecl *)(uint16_t *, const char *)>(0x43E000);
    for (int i = 0; i < iters; ++i) {
        std::string s = RandomText(100, true);
        uint16_t a[128], b[128];
        for (int k = 0; k < 128; ++k) a[k] = b[k] = static_cast<uint16_t>(0xAA00 + k);   // same garbage
        uint16_t *ra = origStrCpy(a, s.c_str()), *rb = bStrCpy(b, s.c_str());
        EXPECT(std::memcmp(a, b, sizeof a) == 0 && ra == a && rb == b, "bStrCpy(\"%s\")", s.c_str());
        uint16_t c[128], d[128];
        for (int k = 0; k < 128; ++k) c[k] = d[k] = static_cast<uint16_t>(0x5500 + k);
        const uint16_t *src = (i % 10 == 0) ? nullptr : a;
        origCopy(c, src);
        CopyString(d, src);
        EXPECT(std::memcmp(c, d, sizeof c) == 0, "CopyString(src=%s)", src ? "text" : "null");
    }
    origCopy(nullptr, nullptr);   // null destination: both must simply return
    CopyString(nullptr, nullptr);
}

// Random AddNode/RemNode sequences on two identical node sets: original vs rewrite, compare all links.
static void CheckMinList(int iters)
{
    auto origAdd = ORIG<void(THISCALL *)(FEMinList *, FEMinNode *, FEMinNode *)>(0x5053B0);
    auto origRem = ORIG<void(THISCALL *)(FEMinList *, FEMinNode *)>(0x505400);
    constexpr int N = 16;
    for (int it = 0; it < iters / 50; ++it) {
        FEMinNode na[N] = {}, nb[N] = {};
        bool in[N] = {};
        FEMinList la{nullptr, 0, nullptr, nullptr}, lb = la;
        auto idx = [&](FEMinNode *p, FEMinNode *base) -> intptr_t { return p ? (p >= base && p < base + N ? p - base : (intptr_t)p) : -1; };
        for (int step = 0; step < 60; ++step) {
            int k = rng() % N;
            if (!in[k]) {
                int after = -1;
                for (int t = 0; t < 4; ++t) { int j = rng() % N; if (in[j]) { after = j; break; } }
                origAdd(&la, after < 0 ? nullptr : &na[after], &na[k]);
                lb.AddNode(after < 0 ? nullptr : &nb[after], &nb[k]);
                in[k] = true;
            } else {
                origRem(&la, &na[k]);
                lb.RemNode(&nb[k]);
                in[k] = false;
            }
            bool same = la.count == lb.count && idx(la.head, na) == idx(lb.head, nb) && idx(la.tail, na) == idx(lb.tail, nb);
            for (int j = 0; j < N && same; ++j)
                same = idx(na[j].next, na) == idx(nb[j].next, nb) && idx(na[j].prev, na) == idx(nb[j].prev, nb);
            EXPECT(same, "FEMinList sequence %d step %d", it, step);
        }
    }
}

// FEObject lookups on synthetic objects: original vs rewrite.
static void CheckFEObject(int iters)
{
    auto origFindScript = ORIG<FEScript *(THISCALL *)(const FEObject *, uint32_t)>(0x5046B0);
    auto origFindResp = ORIG<FEMessageResponse *(THISCALL *)(const FEObject *, uint32_t)>(0x5046D0);
    auto origOffset = ORIG<uint32_t(THISCALL *)(const FEObject *, int)>(0x5046F0);
    for (int t = -3; t < 20; ++t)
        EXPECT(origOffset(nullptr, t) == FEObject::GetDataOffset(t), "GetDataOffset(%d)", t);
    for (int it = 0; it < iters / 20; ++it) {
        FEObject o{};
        FEScript s[6]{};
        FEMessageResponse r[6]{};
        int ns = rng() % 7, nr = rng() % 7;
        for (int i = 0; i < ns; ++i) { s[i].mu32Id = rng() % 8; o.mScripts.AddNode(i ? &s[i - 1].node : nullptr, &s[i].node); }
        for (int i = 0; i < nr; ++i) { r[i].mu32MsgId = rng() % 8; o.mResponses.AddNode(i ? &r[i - 1].node : nullptr, &r[i].node); }
        uint32_t key = rng() % 9;
        EXPECT(origFindScript(&o, key) == o.FindScript(key), "FindScript(%u)", key);
        EXPECT(origFindResp(&o, key) == o.FindResponse(key), "FindResponse(%u)", key);
    }
}

// FESlotNode::AllocBlock (PC 504400) and FESlotPool::Free (PC 5174A0, frees without emptying the node) on
// synthetic nodes with random masks: original vs rewrite.
static void CheckSlotPool(int iters)
{
    auto origAlloc = ORIG<uint8_t *(THISCALL *)(FESlotNode *)>(0x504400);
    auto origFree = ORIG<bool(THISCALL *)(FESlotPool *, const uint8_t *)>(0x5174A0);
    static uint8_t buf[32 * 40];
    for (int it = 0; it < iters / 10; ++it) {
        FESlotNode a{}, b{};
        a.mu16BlockSize = 1 + rng() % 40;
        a.mpBuffer = buf;
        uint32_t mask = rng() | (rng() & 1 ? 0xFFFF : 0);
        std::memcpy(a.mau8Mask, &mask, 4);
        a.mu16Used = static_cast<uint16_t>(__builtin_popcount(mask));
        b = a;
        uint8_t *ra = origAlloc(&a), *rb = b.AllocBlock();
        EXPECT(ra == rb && std::memcmp(&a, &b, sizeof a) == 0, "AllocBlock mask %08X size %u", mask, a.mu16BlockSize);
        if (a.mu16Used >= 2) {
            // free one used block through the pool (node keeps >= 1 block, so it is not deleted)
            int k;
            do { k = rng() % 32; } while (!(a.mau8Mask[k >> 3] & (1 << (k & 7))));
            const uint8_t *blk = buf + a.mu16BlockSize * k + rng() % a.mu16BlockSize;   // anywhere inside the block
            FESlotPool pa{}, pb{};
            pa.mNodes.AddNode(nullptr, &a.node); pb.mNodes.AddNode(nullptr, &b.node);
            bool fa = origFree(&pa, blk), fb = pb.Free(blk);
            EXPECT(fa == fb && std::memcmp(a.mau8Mask, b.mau8Mask, 4) == 0 && a.mu16Used == b.mu16Used,
                   "FESlotPool::Free block %d", k);
            EXPECT(origFree(&pa, buf + 32 * 40) == pb.Free(buf + 32 * 40), "FESlotPool::Free foreign");
        }
    }
}

// FERefList::RemNode (PC 5055C0) on random lists, removing a random node (or null): original vs rewrite.
static void CheckRefList(int iters)
{
    auto orig = ORIG<FEMinNode *(THISCALL *)(FERefList *, FEMinNode *)>(0x5055C0);
    for (int it = 0; it < iters / 10; ++it) {
        FEMinNode na[6]{}, nb[6]{};
        FERefList la{}, lb{};
        int n = rng() % 7;
        for (FERefList *l : {&la, &lb}) {
            FEMinNode *nodes = l == &la ? na : nb;
            for (int i = 0; i < n; ++i) {
                nodes[i].prev = i ? &nodes[i - 1] : nullptr;
                nodes[i].next = i + 1 < n ? &nodes[i + 1] : nullptr;
            }
            l->head = n ? &nodes[0] : nullptr;
            l->tail = n ? &nodes[n - 1] : nullptr;
        }
        int k = n ? static_cast<int>(rng() % (n + 1)) - 1 : -1;   // -1 = null
        FEMinNode *ra = orig(&la, k < 0 ? nullptr : &na[k]);
        FEMinNode *rb = lb.RemNode(k < 0 ? nullptr : &nb[k]);
        bool same = (ra ? ra - na : -1) == (rb ? rb - nb : -1);
        auto idxA = [&](FEMinNode *p) { return p == reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED) ? 99 : p ? p - na : -1; };
        auto idxB = [&](FEMinNode *p) { return p == reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED) ? 99 : p ? p - nb : -1; };
        same = same && idxA(la.head) == idxB(lb.head) && idxA(la.tail) == idxB(lb.tail);
        for (int i = 0; i < n; ++i) same = same && idxA(na[i].next) == idxB(nb[i].next) && idxA(na[i].prev) == idxB(nb[i].prev);
        EXPECT(same, "FERefList::RemNode n=%d k=%d", n, k);
    }
}

// Pool block constructors (PC 53E710/53E910): original and rewrite built in turn in the same buffer, compared
// word by word (the rewrite links the stub bridge, so its vtable words are mapped to the real vtables).
template <class Block>
static void CheckBlockCtor(uintptr_t orig, void (*rewrite)(void *), const char *name,
                           std::initializer_list<std::pair<const void *, uint32_t>> vtables)
{
    static uint32_t buf[0x900];   // >= largest block (0x2020 bytes)
    static uint32_t a[sizeof(Block) / 4];
    std::memset(buf, 0xCD, sizeof buf);
    ORIG<void *(THISCALL *)(void *)>(orig)(buf);
    std::memcpy(a, buf, sizeof a);
    std::memset(buf, 0xCD, sizeof buf);
    rewrite(buf);
    int bad = -1;
    for (size_t i = 0; i < sizeof a / 4 && bad < 0; ++i) {
        uint32_t w = buf[i];
        for (auto &v : vtables) if (w == reinterpret_cast<uintptr_t>(v.first)) w = v.second;
        if (w != a[i]) bad = static_cast<int>(i);
    }
    EXPECT(bad < 0, "%s: word +0x%X orig %08X rewrite %08X", name, bad * 4, bad < 0 ? 0 : a[bad], bad < 0 ? 0 : buf[bad]);
}

// FEKeyTrack::FEKeyTrack (PC 525E80) on random garbage: fields it leaves alone must stay, vtables mapped.
static void CheckKeyTrackCtor(int iters)
{
    auto orig = ORIG<void *(THISCALL *)(void *)>(0x525E80);
    for (int it = 0; it < iters / 20; ++it) {
        uint32_t a[0x38 / 4], b[0x38 / 4];
        for (auto &w : a) w = rng();
        std::memcpy(b, a, sizeof a);
        orig(a);
        FEKeyTrack::Construct(b);
        for (auto &w : b) {
            if (w == reinterpret_cast<uintptr_t>(&teststub::feKeyNodeVTable)) w = 0x79AA34;
            else if (w == reinterpret_cast<uintptr_t>(&teststub::feRefListVTable)) w = 0x79AA30;
        }
        EXPECT(std::memcmp(a, b, sizeof a) == 0, "FEKeyTrack ctor");
    }
}

// Quaternion multiply (PC 4FE6B0) and FEColor::operator- (PC 505580): random values, original vs rewrite.
// Exact: the rewrite mirrors the x87 evaluation order and precision.
static void CheckFEMath(int iters)
{
    auto oq = ORIG<FEQuaternion *(THISCALL *)(const FEQuaternion *, FEQuaternion *, const FEQuaternion *)>(0x4FE6B0);
    auto oc = ORIG<FEColor *(THISCALL *)(const FEColor *, FEColor *, const FEColor *)>(0x505580);
    std::uniform_real_distribution<float> d(-4.f, 4.f);
    for (int it = 0; it < iters / 4; ++it) {
        FEQuaternion a{d(rng), d(rng), d(rng), d(rng)}, b{d(rng), d(rng), d(rng), d(rng)}, ro{};
        oq(&a, &ro, &b);
        FEQuaternion rr = FEQuatMultiply(a, b);
        EXPECT(std::memcmp(&ro, &rr, sizeof ro) == 0, "quat multiply");
        FEColor ca, cb, co{};
        for (int k = 0; k < 4; ++k) { ca.c[k] = static_cast<int32_t>(rng()); cb.c[k] = static_cast<int32_t>(rng()); }
        oc(&ca, &co, &cb);
        FEColor cr = FEColorSub(ca, cb);
        EXPECT(std::memcmp(&co, &cr, sizeof co) == 0, "FEColor sub");
    }
}

// FEScript::FindTrack (PC 504A00) and FEObject::SetTrackValue x3 (PC 5178D0/5179D0/517AD0) on two identical
// synthetic objects (stack scripts/tracks, no heap): original vs rewrite, compared byte for byte.
struct TrackWorld {
    FEObject obj;
    FEScript scripts[3];
    FEKeyTrack tracks[3][4];
    uint32_t data[48];
};
static void BuildTrackWorld(TrackWorld &w, uint32_t seed)
{
    std::mt19937 r(seed);
    std::memset(&w, 0, sizeof w);
    static const int8_t kIdx[] = {0, 4, 7, 10, 14, 17, 19, 21, 25, 29, 33, 5};
    int ns = r() % 4;
    for (int k = 0; k < ns; ++k) {
        FEScript &s = w.scripts[k];
        s.mu32NumTracks = r() % 5;
        s.mpTracks = w.tracks[k];
        for (uint32_t i = 0; i < s.mu32NumTracks; ++i) {
            w.tracks[k][i].mi8DataIndex = kIdx[r() % 12];
            w.tracks[k][i].mu8Flags = static_cast<uint8_t>(r());
            for (auto &d : w.tracks[k][i].mBaseKey._data) d = r() % 1000;
        }
        w.obj.mScripts.AddNode(k ? &w.scripts[k - 1].node : nullptr, &s.node);
    }
    for (auto &d : w.data) d = r() % 1000;
    w.obj.mpData = w.data;
}
static void CheckSetTrackValue(int iters)
{
    auto oFind = ORIG<FEKeyTrack *(THISCALL *)(const FEScript *, int)>(0x504A00);
    auto o3 = ORIG<void(THISCALL *)(FEObject *, int, const FEVector3 *, int)>(0x5178D0);
    auto o2 = ORIG<void(THISCALL *)(FEObject *, int, const FEVector2 *, int)>(0x5179D0);
    auto oc = ORIG<void(THISCALL *)(FEObject *, int, const FEColor *, int)>(0x517AD0);
    static TrackWorld a, b;
    for (int it = 0; it < iters / 10; ++it) {
        uint32_t seed = rng();
        BuildTrackWorld(a, seed);
        BuildTrackWorld(b, seed);
        int track = rng() % 11, kind = rng() % 3, add = rng() & 1;
        EXPECT((oFind(&a.scripts[0], track) == nullptr ? -1 : oFind(&a.scripts[0], track) - a.tracks[0]) ==
               (b.scripts[0].FindTrack(track) == nullptr ? -1 : b.scripts[0].FindTrack(track) - b.tracks[0]),
               "FindTrack(%d)", track);
        if (kind == 0) { FEVector3 v{float(rng() % 100), 0.5f, -2.f}; o3(&a.obj, track, &v, add); b.obj.SetTrackValue(track, v, add); }
        if (kind == 1) { FEVector2 v{float(rng() % 100), 0.25f}; o2(&a.obj, track, &v, add); b.obj.SetTrackValue(track, v, add); }
        if (kind == 2) { FEColor v{{int32_t(rng()), 1, -1, int32_t(rng())}}; oc(&a.obj, track, &v, add); b.obj.SetTrackValue(track, v, add); }
        bool same = std::memcmp(a.data, b.data, sizeof a.data) == 0 && std::memcmp(a.tracks, b.tracks, sizeof a.tracks) == 0;
        EXPECT(same, "SetTrackValue kind %d track %d add %d", kind, track, add);
    }
}

// Close comparators (PC 504500/5045A0/504640) near their thresholds, and SetPosition/Rotation/Size/Color
// (PC 517C10/517C80/517DB0/517E20) on synthetic objects.
static void CheckSetters(int iters)
{
    auto oc1 = ORIG<bool(__cdecl *)(const FEVector3 *, const FEVector3 *)>(0x504500);
    auto oc2 = ORIG<bool(__cdecl *)(const FEVector3 *, const FEVector3 *)>(0x5045A0);
    auto occ = ORIG<bool(__cdecl *)(const FEColor *, const FEColor *)>(0x504640);
    std::uniform_int_distribution<int> small(-3, 3);
    for (int it = 0; it < iters / 4; ++it) {
        FEVector3 a{float(rng() % 5) * 0.25f, float(small(rng)), 1000.f}, b = a;
        float *pb = &b.x;
        pb[rng() % 3] += float(small(rng)) * 5e-7f;               // around the 1e-6 threshold
        if (rng() % 50 == 0) pb[rng() % 3] = NAN;
        EXPECT((oc1(&a, &b) & 1) == FEVector3Close(a, b) && (oc2(&a, &b) & 1) == FEVector3Close(a, b), "Vec3 Close");
        FEColor ca{{int32_t(rng()), small(rng), INT32_MAX, INT32_MIN}}, cb = ca;
        cb.c[rng() % 4] += small(rng);
        EXPECT((occ(&ca, &cb) & 1) == FEColorClose(ca, cb), "Color Close");
    }
    auto oPos = ORIG<void(THISCALL *)(FEObject *, const FEVector3 *, int)>(0x517C10);
    auto oRot = ORIG<void(THISCALL *)(FEObject *, const FEQuaternion *, int)>(0x517C80);
    auto oSize = ORIG<void(THISCALL *)(FEObject *, const FEVector3 *, int)>(0x517DB0);
    auto oCol = ORIG<void(THISCALL *)(FEObject *, const FEColor *, int)>(0x517E20);
    static TrackWorld a, b;
    for (int it = 0; it < iters / 10; ++it) {
        uint32_t seed = rng();
        BuildTrackWorld(a, seed);
        BuildTrackWorld(b, seed);
        uint32_t type = rng() % 3 == 0 ? 0x100 + rng() % 4 : rng() % 8, flags = rng() & ~0x400000u;
        a.obj.mu32Type = b.obj.mu32Type = type;
        a.obj.mu32Flags = b.obj.mu32Flags = flags;
        int kind = rng() % 4, add = rng() & 1;
        bool same_value = rng() % 3 == 0;
        if (kind == 0 || kind == 2) {
            int off = kind == 0 ? 7 : 14;
            FEVector3 v{float(rng() % 9), 2.f, 3.f};
            if (same_value) std::memcpy(&v, &a.data[off], sizeof v);
            if (same_value && add) v = FEVector3{0, 0, 0};
            if (kind == 0) { oPos(&a.obj, &v, add); b.obj.SetPosition(v, add); }
            else { oSize(&a.obj, &v, add); b.obj.SetSize(v, add); }
        } else if (kind == 1) {
            FEQuaternion q{0.f, 0.6f, 0.f, 0.8f};
            oRot(&a.obj, &q, add); b.obj.SetRotation(q, add);
        } else {
            FEColor c{{int32_t(rng() % 300), 1, 2, 3}};
            if (same_value) std::memcpy(&c, &a.data[0], sizeof c);
            oCol(&a.obj, &c, add); b.obj.SetColor(c, add);
        }
        bool same = std::memcmp(a.data, b.data, sizeof a.data) == 0 && std::memcmp(a.tracks, b.tracks, sizeof a.tracks) == 0 &&
                    a.obj.mu32Flags == b.obj.mu32Flags;
        EXPECT(same, "setter kind %d add %d type %X", kind, add, type);
    }
}

// Key lookup (PC 505260/5052B0/4FE8D0), FEInterpNone (519D40) and FEKeyInterpFast (5354F0, step/none tracks
// only: linear is still the original) on random synthetic key lists, incl. reference lists.
static void CheckKeyInterp(int iters)
{
    auto oKeyAt = ORIG<FEKeyNode *(THISCALL *)(FEKeyTrack *, int32_t)>(0x505260);
    auto oDelta = ORIG<FEKeyNode *(THISCALL *)(const FEKeyTrack *, int32_t)>(0x5052B0);
    auto oFirst = ORIG<FEKeyNode *(THISCALL *)(const FEKeyTrack *)>(0x4FE8D0);
    auto oNone = ORIG<void(__cdecl *)(FEKeyTrack *, int32_t, void *)>(0x519D40);
    auto oFast = ORIG<void(__cdecl *)(FEKeyTrack *, int32_t, void *)>(0x5354F0);
    for (int it = 0; it < iters / 10; ++it) {
        FEKeyTrack t{}, owner{};
        FEKeyNode k[6]{};
        int n = rng() % 7;
        int32_t time = 0;
        for (int i = 0; i < n; ++i) {
            k[i].node.next = i + 1 < n ? &k[i + 1].node : nullptr;
            k[i].node.prev = i ? &k[i - 1].node : nullptr;
            time += rng() % 3 == 0 ? 0 : int32_t(rng() % 20);
            k[i]._data[0] = uint32_t(time);
            for (int j = 1; j < 5; ++j) k[i]._data[j] = rng();
        }
        t.mu8ValueSize = uint8_t(1 + rng() % 16);
        int32_t len = int32_t(rng() % 120) - 10;
        t._length[0] = uint8_t(len); t._length[1] = uint8_t(len >> 8); t._length[2] = uint8_t(len >> 16);
        t.mBaseKey.node.next = nullptr;                         // keep the past-the-end path readable
        for (int j = 0; j < 5; ++j) t.mBaseKey._data[j] = rng() % 50;
        FEKeyTrack *keys = &t;
        if (rng() % 4 == 0) {                                  // keys live in another track's list
            owner.mKeys.head = n ? &k[0].node : nullptr;
            t.mKeys.mbIsReference = 1;
            t.mKeys.head = reinterpret_cast<FEMinNode *>(&owner.mKeys);
        } else {
            t.mKeys.head = n ? &k[0].node : nullptr;
            t.mKeys.tail = n ? &k[n - 1].node : nullptr;
        }
        (void)keys;
        int32_t q = int32_t(rng() % 140) - 20;
        EXPECT(oKeyAt(&t, q) == t.GetKeyAt(q), "GetKeyAt n=%d t=%d", n, q);
        EXPECT(oDelta(&t, q) == t.GetDeltaKeyAt(q), "GetDeltaKeyAt n=%d t=%d", n, q);
        EXPECT(oFirst(&t) == t.GetFirstKey(), "first key");
        uint8_t oa[20], ob[20];
        std::memset(oa, 0xCD, sizeof oa); std::memset(ob, 0xCD, sizeof ob);
        oNone(&t, q, oa); FEInterpNone(&t, q, ob);
        EXPECT(std::memcmp(oa, ob, sizeof oa) == 0, "FEInterpNone n=%d t=%d len=%d", n, q, len);
        FEKeyTrack ta = t, tb = t;
        uint8_t interp = rng() % 2 ? 0 : 2;
        ta.mu8InterpType = tb.mu8InterpType = interp;
        ta.mu8Flags = tb.mu8Flags = uint8_t(rng() & 0x80);
        std::memset(oa, 0xCD, sizeof oa); std::memset(ob, 0xCD, sizeof ob);
        oFast(&ta, q, oa); FEKeyInterpFast(&tb, q, ob);
        EXPECT(std::memcmp(oa, ob, sizeof oa) == 0 && ta.mu8Flags == tb.mu8Flags, "FEKeyInterpFast interp %d", interp);
    }
}

// FELerp* (PC 5050D0/505110/505150/5051A0) and FEInterpLinear (52BDC0, modes 0/1, all types but quaternion lerp)
// on random inputs: byte-exact.
static void CheckInterpLinear(int iters)
{
    auto oLI = ORIG<void(__cdecl *)(int32_t, int32_t, float, const int32_t *, int32_t *)>(0x5050D0);
    auto oL2 = ORIG<void(__cdecl *)(const FEVector2 *, const FEVector2 *, float, const FEVector2 *, FEVector2 *)>(0x505110);
    auto oL3 = ORIG<void(__cdecl *)(const FEVector3 *, const FEVector3 *, float, const FEVector3 *, FEVector3 *)>(0x505150);
    auto oLC = ORIG<void(__cdecl *)(const FEColor *, const FEColor *, float, const FEColor *, FEColor *)>(0x5051A0);
    auto oLin = ORIG<void(__cdecl *)(FEKeyTrack *, int32_t, void *)>(0x52BDC0);
    std::uniform_real_distribution<float> fd(-500.f, 500.f), td(0.f, 1.f);
    auto rf = [&] { return rng() % 8 == 0 ? float(int(rng() % 9) - 4) : fd(rng); };
    for (int it = 0; it < iters / 4; ++it) {
        float t = rng() % 10 == 0 ? 0.5f : td(rng);
        int32_t ia = int32_t(rng()) >> (rng() % 31), ib = int32_t(rng()) >> (rng() % 31), ibase = int32_t(rng()), ro = 0, rr = 0;
        oLI(ia, ib, t, &ibase, &ro); FELerpInteger(ia, ib, t, &ibase, &rr);
        EXPECT(ro == rr, "FELerpInteger %d %d %f", ia, ib, double(t));
        FEVector3 a{rf(), rf(), rf()}, b{rf(), rf(), rf()}, base{rf(), rf(), rf()}, o3{}, r3{};
        oL3(&a, &b, t, &base, &o3); FELerpVector3(&a, &b, t, &base, &r3);
        EXPECT(std::memcmp(&o3, &r3, sizeof o3) == 0, "FELerpVector3");
        FEVector2 o2{}, r2{};
        oL2(reinterpret_cast<FEVector2 *>(&a), reinterpret_cast<FEVector2 *>(&b), t, reinterpret_cast<FEVector2 *>(&base), &o2);
        FELerpVector2(reinterpret_cast<FEVector2 *>(&a), reinterpret_cast<FEVector2 *>(&b), t, reinterpret_cast<FEVector2 *>(&base), &r2);
        EXPECT(std::memcmp(&o2, &r2, sizeof o2) == 0, "FELerpVector2");
        FEColor ca, cb, cbase, oc{}, rc{};
        for (int k = 0; k < 4; ++k) { ca.c[k] = int32_t(rng() % 512) - 256; cb.c[k] = int32_t(rng() % 512) - 256; cbase.c[k] = int32_t(rng()); }
        oLC(&ca, &cb, t, &cbase, &oc); FELerpColor(&ca, &cb, t, &cbase, &rc);
        EXPECT(std::memcmp(&oc, &rc, sizeof oc) == 0, "FELerpColor");
    }
    static const uint8_t kTypes[] = {1, 2, 3, 4, 6};
    for (int it = 0; it < iters / 4; ++it) {
        FEKeyTrack tr{};
        FEKeyNode k[6]{};
        int n = rng() % 7;
        int32_t time = int32_t(rng() % 10);
        for (int i = 0; i < n; ++i) {
            k[i].node.next = i + 1 < n ? &k[i + 1].node : nullptr;
            k[i].node.prev = i ? &k[i - 1].node : nullptr;
            k[i]._data[0] = uint32_t(time);
            time += rng() % 4 == 0 ? 0 : int32_t(rng() % 25);
            for (int j = 1; j < 5; ++j) { float f = rf(); std::memcpy(&k[i]._data[j], &f, 4); }
        }
        tr.mu8ParamType = kTypes[rng() % 5];
        tr.mu8Flags = uint8_t(rng() % 2 | (rng() & 0x80));
        int32_t len = int32_t(rng() % 150);
        tr._length[0] = uint8_t(len);
        for (int j = 1; j < 5; ++j) { float f = rf(); std::memcpy(&tr.mBaseKey._data[j], &f, 4); }
        if (tr.mu8ParamType == 1 || tr.mu8ParamType == 6)
            for (int i = 0; i < n; ++i) for (int j = 1; j < 5; ++j) k[i]._data[j] = uint32_t(int32_t(rng() % 2000) - 1000);
        tr.mKeys.head = n ? &k[0].node : nullptr;
        tr.mKeys.tail = n ? &k[n - 1].node : nullptr;
        int32_t q = int32_t(rng() % 200) - 20;
        uint8_t oa[16], ob[16];
        std::memset(oa, 0xCD, sizeof oa); std::memset(ob, 0xCD, sizeof ob);
        oLin(&tr, q, oa); FEInterpLinear(&tr, q, ob);
        EXPECT(std::memcmp(oa, ob, sizeof oa) == 0, "FEInterpLinear type %d mode %d n=%d t=%d", tr.mu8ParamType, tr.mu8Flags & 0x7F, n, q);
    }
}

// bWare trig (bSin exhaustive over all angles; bSinF/bASin random + boundaries, compared at 80-bit precision) and
// the quaternion helpers + slerp (PC 4FE770/4FE7D0/4FE830/511C80/519B00).
static void CheckTrigAndSlerp(int iters)
{
    auto oSin = ORIG<long double(__cdecl *)(uint32_t)>(0x43C2B0);
    auto oSinF = ORIG<long double(__cdecl *)(float)>(0x43C350);
    auto oASin = ORIG<uint32_t(__cdecl *)(float)>(0x43C490);
    int bad = 0;
    for (uint32_t a = 0; a < 0x10000; ++a) {
        long double o = oSin(a), r = bSin(uint16_t(a));
        if (std::memcmp(&o, &r, 10) != 0 && ++bad <= 3) std::printf("bSin %04X\n", a);
        ++checks;
    }
    failures += bad;
    std::uniform_real_distribution<float> big(-20.f, 20.f), unit(-1.05f, 1.05f);
    for (int it = 0; it < iters; ++it) {
        float f = big(rng);
        long double o = oSinF(f), r = bSinF(f);
        EXPECT(std::memcmp(&o, &r, 10) == 0, "bSinF %g", double(f));
        float x = it % 4 == 0 ? float(int(rng() % 65537) - 32768) / 32768.f : unit(rng);   // segment boundaries too
        EXPECT((oASin(x) & 0xFFFF) == bASin(x), "bASin %.9g: %X vs %X", double(x), oASin(x) & 0xFFFF, bASin(x));
    }
    auto oScale = ORIG<FEQuaternion *(__cdecl *)(FEQuaternion *, const FEQuaternion *, float)>(0x4FE770);
    auto oAdd = ORIG<FEQuaternion *(__cdecl *)(FEQuaternion *, const FEQuaternion *, const FEQuaternion *)>(0x4FE7D0);
    auto oSub = ORIG<FEQuaternion *(__cdecl *)(FEQuaternion *, const FEQuaternion *, const FEQuaternion *)>(0x4FE830);
    auto oNorm = ORIG<void(__cdecl *)(FEQuaternion *)>(0x511C80);
    auto oSlerp = ORIG<void(__cdecl *)(const FEQuaternion *, const FEQuaternion *, float, const FEQuaternion *, FEQuaternion *)>(0x519B00);
    std::uniform_real_distribution<float> qd(-1.f, 1.f), td(0.f, 1.f);
    auto rq = [&](bool norm) {
        FEQuaternion q{qd(rng), qd(rng), qd(rng), qd(rng)};
        if (norm) FEQuatNormalize(&q);
        return q;
    };
    for (int it = 0; it < iters / 2; ++it) {
        FEQuaternion a = rq(false), b = rq(false), o{}, r{};
        float s = qd(rng) * 3.f;
        oScale(&o, &a, s); FEQuatScale(&r, &a, s);
        EXPECT(std::memcmp(&o, &r, sizeof o) == 0, "quat scale");
        oAdd(&o, &a, &b); FEQuatAdd(&r, &a, &b);
        EXPECT(std::memcmp(&o, &r, sizeof o) == 0, "quat add");
        oSub(&o, &a, &b); FEQuatSub(&r, &a, &b);
        EXPECT(std::memcmp(&o, &r, sizeof o) == 0, "quat sub");
        FEQuaternion na = a, nb = a;
        if (it % 50 == 0) { na = nb = FEQuaternion{2e5f, 0, 0, 0}; }
        oNorm(&na); FEQuatNormalize(&nb);
        EXPECT(std::memcmp(&na, &nb, sizeof na) == 0, "quat normalize");
        // slerp: unit quaternions, sometimes nearly parallel / opposite
        FEQuaternion qa = rq(true), qb = rq(true), base = rq(true);
        if (it % 5 == 0) { qb = qa; qb.x += 0.01f * qd(rng); FEQuatNormalize(&qb); }
        if (it % 7 == 0) qb = FEQuaternion{-qb.x, -qb.y, -qb.z, -qb.w};
        float t = it % 11 == 0 ? 0.5f : td(rng);
        oSlerp(&qa, &qb, t, &base, &o); FELerpQuaternion(&qa, &qb, t, &base, &r);
        EXPECT(std::memcmp(&o, &r, sizeof o) == 0, "slerp t=%g", double(t));
    }
}

// FEPackage::UpdateObjectTracks (PC 541DB0): synthetic object + script with random step/linear tracks (no
// quaternion), both interpolation modes, random alpha/flags. Compares data, track flags and object flags.
struct UotWorld {
    FEObject obj;
    FEScript sc;
    FEKeyTrack tracks[5];
    FEKeyNode keys[5][3];
    uint32_t data[40];
    uint8_t pkg[0x18];
};
static void BuildUot(UotWorld &w, uint32_t seed)
{
    std::mt19937 r(seed);
    std::memset(&w, 0, sizeof w);
    static const int8_t kIdx[] = {0, 0, 4, 7, 10, 14, 19, 33, -2};
    static const uint8_t kSize[] = {0, 4, 4, 8, 12, 16, 16};
    int n = r() % 6;
    w.sc.mu32NumTracks = (r() % 8 == 0 ? 0x100 : 0) | uint32_t(n);   // only the low byte counts
    w.sc.mpTracks = n || r() % 2 ? w.tracks : nullptr;
    w.sc.mi32Time = r() % 60;
    for (int i = 0; i < n; ++i) {
        FEKeyTrack &t = w.tracks[i];
        uint8_t type = uint8_t(1 + r() % 4);                // int/float/vec2/vec3 (+ color below)
        if (r() % 4 == 0) type = 6;
        t.mu8ParamType = type;
        t.mu8ValueSize = kSize[type];
        t.mu8InterpType = uint8_t(r() % 5);
        t.mu8Flags = uint8_t((r() & 0x80) | (r() % 2));
        t.mi8DataIndex = kIdx[r() % 8];
        t._length[0] = uint8_t(r() % 80);
        t.mBaseKey.node.next = nullptr;
        int nk = r() % 4;
        for (int k = 0; k < nk; ++k) {
            FEKeyNode &kn = w.keys[i][k];
            kn.node.next = k + 1 < nk ? &w.keys[i][k + 1].node : nullptr;
            kn.node.prev = k ? &w.keys[i][k - 1].node : nullptr;
            kn._data[0] = uint32_t(k * 20 + r() % 10);
            for (int j = 1; j < 5; ++j) kn._data[j] = type == 1 || type == 6 ? r() % 300 : 0x3F800000u + (r() % 0x100000);
        }
        t.mKeys.head = nk ? &w.keys[i][0].node : nullptr;
        t.mKeys.tail = nk ? &w.keys[i][nk - 1].node : nullptr;
    }
    for (auto &d : w.data) d = r() % 3 == 0 ? 0 : r();
    if (r() % 3 == 0) w.data[3] = 0;                       // invisible
    w.obj.mpData = w.data;
    w.obj.mu32Flags = r() & 0x03C00000u;
    w.pkg[0x14] = uint8_t(r() % 2);
}
static void CheckUpdateObjectTracks(int iters)
{
    auto orig = ORIG<void(THISCALL *)(const void *, FEObject *, FEScript *)>(0x541DB0);
    static UotWorld a, b;
    for (int it = 0; it < iters / 10; ++it) {
        uint32_t seed = rng();
        BuildUot(a, seed);
        BuildUot(b, seed);
        orig(a.pkg, &a.obj, &a.sc);
        reinterpret_cast<const FEPackage *>(b.pkg)->UpdateObjectTracks(&b.obj, &b.sc);
        bool same = std::memcmp(a.data, b.data, sizeof a.data) == 0 && a.obj.mu32Flags == b.obj.mu32Flags;
        for (int i = 0; i < 5; ++i) same = same && a.tracks[i].mu8Flags == b.tracks[i].mu8Flags;
        EXPECT(same, "UpdateObjectTracks fast=%d n=%u", a.pkg[0x14], a.sc.mu32NumTracks);
    }
}

static int RunAll()
{
    auto *base = reinterpret_cast<uint8_t *>(GetModuleHandleA(nullptr));
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS32 *>(base + reinterpret_cast<IMAGE_DOS_HEADER *>(base)->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != 0x214D4C48) { std::printf("referee: not SPEED2.EXE v1.2\n"); return 2; }
    const char *env = std::getenv("REFEREE_ITERATIONS");
    int iters = env ? std::atoi(env) : 20000;
    // sanity: prove we really execute the ORIGINAL machine code (known answer: POSIX cksum of "123456789")
    uint32_t known = ORIG<uint32_t(__cdecl *)(const void *, int, uint32_t)>(0x43BCD0)("123456789", 9, 0);
    if (known != 0x377A6011u) { std::printf("referee: original code not callable (got %08X)\n", known); return 2; }
    CheckCrc(iters);
    CheckStringHash(iters);
    CheckWideCopies(iters);
    CheckMinList(iters);
    CheckFEObject(iters);
    CheckSlotPool(iters);
    CheckRefList(iters);
    CheckKeyTrackCtor(iters);
    CheckFEMath(iters);
    CheckSetTrackValue(iters);
    CheckSetters(iters);
    CheckKeyInterp(iters);
    CheckInterpLinear(iters);
    CheckTrigAndSlerp(iters);
    CheckUpdateObjectTracks(iters);
    CheckBlockCtor<FEMessageResponseBlock>(0x53E710, FEMessageResponseBlock_Construct, "FEMessageResponse block",
        {{&teststub::feMsgRespBlockVTable, 0x79BF50}, {&teststub::feMinListVTable, 0x79A9EC}, {&teststub::feMsgRespVTable, 0x79AA38}});
    CheckBlockCtor<FEKeyNodeBlock>(0x5319C0, FEKeyNodeBlock_Construct, "FEKeyNode block",
        {{&teststub::feKeyNodeBlockVTable, 0x79B9EC}, {&teststub::feMinListVTable, 0x79A9EC}, {&teststub::feKeyNodeVTable, 0x79AA34}});
    CheckBlockCtor<FEScriptBlock>(0x53E910, FEScriptBlock_Construct, "FEScript block",
        {{&teststub::feScriptBlockVTable, 0x79BF54}, {&teststub::feMinListVTable, 0x79A9EC}, {&teststub::feScriptVTable, 0x79AD28}});
    EXPECT(ORIG<uint32_t(__cdecl *)(const char *)>(0x505450)(nullptr) == FEHashUpper(nullptr), "FEHashUpper(null)");
    std::printf("referee: %d checks, %d mismatches -> %s\n", checks, failures, failures ? "FAILED" : "ok");
    return failures != 0;
}

// the game imports this; it is never called because the process exits in DllMain
extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN) { return E_FAIL; }

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        std::freopen("referee.log", "w", stdout);
        int rc = RunAll();
        std::fflush(stdout);
        ExitProcess(rc);
    }
    return TRUE;
}
