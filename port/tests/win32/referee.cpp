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
#include "frontend/FEButtonMap.h"
#include "frontend/FERender.h"
#include "frontend/FETypeLib.h"
#include "frontend/FEngUtil.h"
#include "frontend/FEWidget.h"
#include "frontend/FEScript.h"
#include "frontend/FEMultiPool.h"
#include "frontend/FEPool.h"
#include "frontend/FEScript.h"
#include "frontend/FEMath.h"
#include "frontend/FEPackage.h"
#include "frontend/FEString.h"
#include "frontend/FEPackageReader.h"
#include "frontend/FEngine.h"
#include "frontend/cFEng.h"
#include "frontend/FEListBox.h"
#include "frontend/FECodeListBox.h"
#include "bWare/bSlotPool.h"
#include "camera/CameraNoise.h"
#include "camera/Camera.h"
#include "frontend/FEPackageManager.h"
#include "frontend/FENode.h"
#include "bWare/bMath.h"
#include <cmath>
#include <float.h>
#include <string>
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

// bStr* family: random strings over a small alphabet (case pairs, high bytes) so prefixes and ties are common.
static std::string StrSample()
{
    static const char kAlpha[] = "aAbBzZ_\xE9\xC9{[";
    std::string s;
    int n = rng() % 6;
    for (int i = 0; i < n; ++i) s.push_back(kAlpha[rng() % (sizeof kAlpha - 1)]);
    return s;
}
static void CheckStrFuncs(int iters)
{
    using Cmp = int(__cdecl *)(const char *, const char *);
    using NCmp = int(__cdecl *)(const char *, const char *, int);
    for (int i = 0; i < iters; ++i) {
        std::string a = StrSample(), b = rng() % 3 ? a.substr(0, rng() % (a.size() + 1)) + StrSample() : StrSample();
        // a guard byte before each string: stopping at the start reads s[-1] in the originals
        char ga[16] = {static_cast<char>(rng())}, gb[16] = {static_cast<char>(rng())};
        std::memcpy(ga + 1, a.c_str(), a.size() + 1);
        std::memcpy(gb + 1, b.c_str(), b.size() + 1);
        const char *pa = ga + 1, *pb = gb + 1;
        int n = static_cast<int>(rng() % 8);
        EXPECT(ORIG<int(__cdecl *)(const char *)>(0x43DBB0)(pa) == bStrLen(pa), "bStrLen");
        EXPECT(ORIG<Cmp>(0x43DC30)(pa, pb) == bStrCmp(pa, pb), "bStrCmp(%s,%s)", pa, pb);
        EXPECT(ORIG<Cmp>(0x43DCC0)(pa, pb) == bStrICmp(pa, pb), "bStrICmp(%s,%s)", pa, pb);
        EXPECT(ORIG<NCmp>(0x43DC60)(pa, pb, n) == bStrNCmp(pa, pb, n), "bStrNCmp(%s,%s,%d)", pa, pb, n);
        EXPECT(ORIG<NCmp>(0x43DD10)(pa, pb, n) == bStrNICmp(pa, pb, n), "bStrNICmp(%s,%s,%d)", pa, pb, n);
        int c = (rng() % 4 == 0) ? 0 : static_cast<signed char>(StrSample().c_str()[0]);
        if (rng() % 8 == 0) c = static_cast<unsigned char>(c);
        EXPECT(ORIG<char *(__cdecl *)(const char *, int)>(0x43DDF0)(pa, c) == bStrChr(pa, c), "bStrChr(%s,%d)", pa, c);
        char d1[32], d2[32];
        std::memset(d1, 'x', sizeof d1); std::memset(d2, 'x', sizeof d2);
        EXPECT(ORIG<char *(__cdecl *)(char *, const char *, const char *)>(0x43DDA0)(d1, pa, pb) == d1 && bStrCat(d2, pa, pb) == d2 &&
               !std::memcmp(d1, d2, sizeof d1), "bStrCat");
        std::memset(d1, 'x', sizeof d1); std::memset(d2, 'x', sizeof d2);
        EXPECT(ORIG<char *(__cdecl *)(char *, const char *, int)>(0x43DC00)(d1, pa, n) == d1 && bStrNCpy(d2, pa, n) == d2 &&
               !std::memcmp(d1, d2, sizeof d1), "bStrNCpy n=%d", n);
        EXPECT(ORIG<char *(__cdecl *)(char *, const char *)>(0x43DBD0)(d1, pb) == d1 && bStrCpy(d2, pb) == d2 &&
               !std::memcmp(d1, d2, sizeof d1), "bStrCpy");
        char *r1 = ORIG<char *(__cdecl *)(char *)>(0x43DE20)(d1), *r2 = bToUpper(d2);
        EXPECT(r1 - d1 == r2 - d2 && !std::memcmp(d1, d2, sizeof d1), "bToUpper");
    }
    EXPECT(ORIG<int(__cdecl *)(const char *)>(0x43DBB0)(nullptr) == bStrLen(nullptr), "bStrLen(null)");
    EXPECT(ORIG<char *(__cdecl *)(const char *, int)>(0x43DDF0)(nullptr, 0) == bStrChr(nullptr, 0), "bStrChr(null)");
}

// bStrToLong / bStrToFloat: random numeric strings; float results compared as raw 80-bit x87 values under the
// default (53-bit) and D3D9 (24-bit) precision control.
static std::string NumSample()
{
    static const char kChars[] = "0123456789abcdefABCDEFxX.+-eEg ";
    std::string s;
    switch (rng() % 4) {
    case 0: s = "0x"; break;
    case 1: s = "-"; break;
    case 2: s = "+"; break;
    }
    int n = rng() % 14;
    for (int i = 0; i < n; ++i) s.push_back(rng() % 3 ? static_cast<char>('0' + rng() % 10) : kChars[rng() % (sizeof kChars - 1)]);
    return s;
}
static void CheckStrToNumber(int iters)
{
    auto oLong = ORIG<int32_t(__cdecl *)(const char *)>(0x43DE50);
    auto oFloat = ORIG<long double(__cdecl *)(const char *)>(0x43DEF0);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int i = 0; i < iters; ++i) {
            std::string t = NumSample();
            if (rng() % 8 == 0) t += "e" + std::to_string(static_cast<int>(rng() % 90) - 45);
            EXPECT(oLong(t.c_str()) == bStrToLong(t.c_str()), "bStrToLong(\"%s\")", t.c_str());
            long double x = oFloat(t.c_str()), y = bStrToFloat(t.c_str());
            EXPECT(std::memcmp(&x, &y, 10) == 0, "bStrToFloat(\"%s\") pc=%x: %.21Lg vs %.21Lg", t.c_str(), pc, x, y);
        }
    }
    _controlfp(saved, _MCW_PC);
}

static void CheckLevelSortingList(int iters)
{
    auto orig = ORIG<cFEngLevelSorter *(THISCALL *)(cFEng *, const char *)>(0x51CCD0);
    static cFEng fe;
    cFEngLevelSorter nodes[6];
    std::string names[6];
    for (int it = 0; it < iters / 4; ++it) {
        int n = rng() % 7;
        cFEngLevelSorter *prev = fe.SortSentinel();
        fe.mpSortHead = prev;
        for (int i = 0; i < n; ++i) {
            names[i] = StrSample();
            nodes[i] = cFEngLevelSorter{nullptr, prev, names[i].c_str()};
            prev->next = &nodes[i];
            prev = &nodes[i];
        }
        prev->next = fe.SortSentinel();
        fe.mpSortTail = prev;
        std::string q = (n && rng() % 2) ? names[rng() % n] : StrSample();
        if (rng() % 2) for (auto &ch : q) ch = static_cast<char>(ch ^ ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ? 0x20 : 0));
        EXPECT(orig(&fe, q.c_str()) == fe.FindInLevelSortingList(q.c_str()), "FindInLevelSortingList(%s) n=%d", q.c_str(), n);
    }
}

static float RandSlotFloat()
{
    switch (rng() % 6) {
    case 0: return 0.0f;
    case 1: return static_cast<float>(rng() % 200);
    default: return static_cast<float>(static_cast<int32_t>(rng() % 200000) - 50000) / 997.0f;
    }
}
static void CheckListBoxScroll(int iters)
{
    auto orig = ORIG<void(THISCALL *)(FEListBox *, int32_t, int32_t)>(0x516E90);
    const unsigned int saved = _controlfp(0, 0);
    FEListBoxSlot cols[6], rows[6];
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            alignas(8) static FEListBox a, b;
            std::memset(&a, 0, sizeof a);
            a.mu32NumColumns = 1 + rng() % 6;
            a.mu32NumRows = 1 + rng() % 6;
            for (auto *sl : {cols, rows})
                for (int i = 0; i < 6; ++i) sl[i] = FEListBoxSlot{RandSlotFloat(), RandSlotFloat(), rng()};
            a.mpColumns = cols; a.mpRows = rows;
            a.mu32Flags = rng() & 0x7F;
            a.mu32SelColumn = rng() % a.mu32NumColumns;
            a.mu32SelRow = rng() % a.mu32NumRows;
            for (float *f : {&a.mfViewWidth, &a.mfViewHeight, &a.mfScrollX, &a.mfScrollY, &a.mfTargetX, &a.mfTargetY, &a.mfDirX, &a.mfDirY})
                *f = rng() % 3 ? RandSlotFloat() : 0.0f;
            int32_t dx = static_cast<int32_t>(rng() % (2 * a.mu32NumColumns + 1)) - static_cast<int32_t>(a.mu32NumColumns);
            int32_t dy = static_cast<int32_t>(rng() % (2 * a.mu32NumRows + 1)) - static_cast<int32_t>(a.mu32NumRows);
            if (rng() % 4 == 0) dx = 0;
            if (rng() % 4 == 0) dy = 0;
            std::memcpy(&b, &a, sizeof a);
            orig(&a, dx, dy);
            b.ScrollSelection(dx, dy);
            EXPECT(std::memcmp(&a, &b, sizeof a) == 0, "FEListBox::ScrollSelection flags=%02X n=%u/%u sel=%u/%u d=%d/%d pc=%x: flags %X/%X tgt %g/%g %g/%g dir %g/%g %g/%g",
                   b.mu32Flags, a.mu32NumColumns, a.mu32NumRows, a.mu32SelColumn, a.mu32SelRow, dx, dy, pc,
                   a.mu32Flags, b.mu32Flags, a.mfTargetX, b.mfTargetX, a.mfTargetY, b.mfTargetY, a.mfDirX, b.mfDirX, a.mfDirY, b.mfDirY);
        }
    }
    _controlfp(saved, _MCW_PC);
}

static void CheckListBoxUpdate(int iters)
{
    auto oUpdate = ORIG<void(THISCALL *)(FEListBox *, float)>(0x517230);
    auto oComplete = ORIG<void(THISCALL *)(FEListBox *)>(0x5043B0);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            alignas(8) static FEListBox a, b;
            std::memset(&a, 0, sizeof a);
            a.mu32Flags = rng() & 0x7F;
            a.mu32SelColumn = rng() % 3; a.mu32SelRow = rng() % 3;
            for (float *f : {&a.mfScrollX, &a.mfScrollY, &a.mfTargetX, &a.mfTargetY, &a.mfDirX, &a.mfDirY, &a.mfSpeedX, &a.mfSpeedY})
                *f = rng() % 4 ? RandSlotFloat() : 0.0f;
            a.mfPulse = static_cast<float>(rng() % 1200) / 1000.0f - 0.1f;
            a.mfPulseRate = RandSlotFloat();
            float dt = rng() % 8 ? static_cast<float>(rng() % 100) / 997.0f : 0.0f;
            std::memcpy(&b, &a, sizeof a);
            if (it % 8 == 0) {
                oComplete(&a);
                b.CompleteScroll();
            } else {
                oUpdate(&a, dt);
                b.Update(dt);
            }
            EXPECT(std::memcmp(&a, &b, sizeof a) == 0, "FEListBox::Update dt=%g pc=%x flags %X/%X scroll %g/%g %g/%g pulse %g/%g",
                   dt, pc, a.mu32Flags, b.mu32Flags, a.mfScrollX, b.mfScrollX, a.mfScrollY, b.mfScrollY, a.mfPulse, b.mfPulse);
        }
    }
    _controlfp(saved, _MCW_PC);
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
// bCos / bSinCos: every angle, under 53- and 24-bit precision.
static float AtanSample();
static void CheckSinCos()
{
    auto oCos = ORIG<long double(__cdecl *)(uint32_t)>(0x43C370);
    auto oSinCos = ORIG<void(__cdecl *)(float *, float *, uint32_t)>(0x43C390);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (uint32_t a = 0; a < 0x10000; ++a) {
            long double o = oCos(a), r = bCos(uint16_t(a));
            EXPECT(std::memcmp(&o, &r, 10) == 0, "bCos %04X pc=%x", a, pc);
            {
                auto oTan = ORIG<long double(__cdecl *)(uint32_t)>(0x440F50);
                long double t1 = oTan(a), t2 = bTan(uint16_t(a));
                EXPECT(std::memcmp(&t1, &t2, 10) == 0 || (t1 != t1 && t2 != t2), "bTan %04X pc=%x", a, pc);
                auto oLen = ORIG<long double(__cdecl *)(const bVector3 *)>(0x440F70);
                bVector3 v{AtanSample(), AtanSample(), AtanSample()};
                long double l1 = oLen(&v), l2 = bLength(&v);
                EXPECT(std::memcmp(&l1, &l2, 10) == 0, "bLength pc=%x", pc);
            }
            float os = 7, oc = 7, rs = 7, rc = 7;
            oSinCos(&os, &oc, a | (rng() & 0xFFFF0000u));   // only the low 16 bits are read (movzx)
            bSinCos(&rs, &rc, uint16_t(a));
            EXPECT(std::memcmp(&os, &rs, 4) == 0 && std::memcmp(&oc, &rc, 4) == 0, "bSinCos %04X pc=%x: %.9g/%.9g %.9g/%.9g",
                   a, pc, double(os), double(rs), double(oc), double(rc));
        }
    }
    _controlfp(saved, _MCW_PC);
}

static float AtanSample()
{
    switch (rng() % 8) {
    case 0: return 0.0f;
    case 1: return -0.0f;
    case 2: return static_cast<float>(static_cast<int32_t>(rng() % 9) - 4);
    default: return static_cast<float>(static_cast<int32_t>(rng() % 2000001) - 1000000) / static_cast<float>(1 + rng() % 5000);
    }
}
static int32_t FixSample()
{
    switch (rng() % 6) {
    case 0: return static_cast<int32_t>(rng() % 9) - 4;
    case 1: return static_cast<int32_t>(rng() % 0x80000) - 0x40000;
    case 2: return static_cast<int32_t>(rng() % 0x400000) - 0x200000;
    case 3: return static_cast<int32_t>(rng() % 0x2000000) - 0x1000000;
    default: { int32_t v = static_cast<int32_t>(rng()); return v == INT32_MIN ? 0 : v; }
    }
}
static void CheckATan(int iters)
{
    auto oFast = ORIG<uint32_t(__cdecl *)(float, float)>(0x43C570);
    auto oFix = ORIG<int32_t(__cdecl *)(int32_t)>(0x43C650);
    auto oFix2 = ORIG<int32_t(__cdecl *)(int32_t, int32_t)>(0x43C6F0);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int i = 0; i < iters; ++i) {
            float x = AtanSample(), y = rng() % 6 ? AtanSample() : (rng() % 2 ? x : -x);
            EXPECT((oFast(x, y) & 0xFFFF) == bFastATan(x, y), "bFastATan(%.9g, %.9g): %X vs %X", double(x), double(y), oFast(x, y) & 0xFFFF, bFastATan(x, y));
            int32_t r = FixSample(), a = FixSample(), b = FixSample();
            EXPECT(oFix(r) == bFixATan(r), "bFixATan(%d): %d vs %d", r, oFix(r), bFixATan(r));
            EXPECT(oFix2(a, b) == bFixATan(a, b), "bFixATan(%d, %d): %d vs %d", a, b, oFix2(a, b), bFixATan(a, b));
        }
    }
    _controlfp(saved, _MCW_PC);
}

static void CheckRandomMod(int iters)
{
    auto oRandI = ORIG<uint32_t(__cdecl *)(int32_t, uint32_t *)>(0x43C140);
    auto oRandF = ORIG<long double(__cdecl *)(float, uint32_t *)>(0x43C170);
    auto oRandGI = ORIG<uint32_t(__cdecl *)(int32_t)>(0x43C1C0);
    auto oRandGF = ORIG<long double(__cdecl *)(float)>(0x43C1F0);
    auto oFMod = ORIG<long double(__cdecl *)(float, float)>(0x43C240);
    auto oSMod = ORIG<int32_t(__cdecl *)(int32_t, int32_t)>(0x43C280);
    uint32_t *g = reinterpret_cast<uint32_t *>(0x7EA624);
    const uint32_t savedSeed = *g;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int i = 0; i < iters; ++i) {
            uint32_t s0 = rng(), s1 = s0;
            int32_t range = static_cast<int32_t>(rng());
            if (rng() % 2) range = 1 + rng() % 1000;
            if (range == 0) range = 7;
            EXPECT(oRandI(range, &s0) == bRandom(range, &s1) && s0 == s1, "bRandom(int,seed) %d", range);
            float fr = AtanSample();
            long double x = oRandF(fr, &s0), y = bRandom(fr, &s1);
            EXPECT(std::memcmp(&x, &y, 10) == 0 && s0 == s1, "bRandom(float,seed) %g", double(fr));
            *g = s0;
            uint32_t a = oRandGI(range);
            uint32_t g1 = *g;
            *g = s0;
            EXPECT(a == bRandom(range) && g1 == *g, "bRandom(int) global");
            *g = s0; x = oRandGF(fr); g1 = *g; *g = s0; y = bRandom(fr);
            EXPECT(std::memcmp(&x, &y, 10) == 0 && g1 == *g, "bRandom(float) global");
            float fa = AtanSample(), fb = AtanSample();
            if (rng() % 4 == 0) fa = fb * static_cast<float>(static_cast<int32_t>(rng() % 9) - 4);   // exact multiples
            x = oFMod(fa, fb); y = bFMod(fa, fb);
            EXPECT(std::memcmp(&x, &y, 10) == 0 || (x != x && y != y), "bFMod(%.9g, %.9g): %.21Lg vs %.21Lg", double(fa), double(fb), x, y);
            int32_t ia = FixSample(), ib = FixSample();
            if (ib == 0) ib = 3;
            if (rng() % 4 == 0) ia = ib * (static_cast<int32_t>(rng() % 7) - 3);
            EXPECT(oSMod(ia, ib) == bSignedMod(ia, ib), "bSignedMod(%d, %d): %d vs %d", ia, ib, oSMod(ia, ib), bSignedMod(ia, ib));
        }
    }
    _controlfp(saved, _MCW_PC);
    *g = savedSeed;
}

static void CheckMatrices(int iters)
{
    auto oId = ORIG<void(__cdecl *)(bMatrix4 *)>(0x43C780);
    auto oEMul = ORIG<void(__cdecl *)(bMatrix4 *, const bMatrix4 *, const bMatrix4 *)>(0x4013F0);
    auto oMul = ORIG<void(__cdecl *)(bMatrix4 *, const bMatrix4 *, const bMatrix4 *)>(0x43C990);
    auto oTr = ORIG<void(__cdecl *)(bMatrix4 *, const bMatrix4 *)>(0x43C9D0);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters / 2; ++it) {
            bMatrix4 a, b, o1, o2;
            for (int i = 0; i < 16; ++i) { a.m[i] = AtanSample(); b.m[i] = AtanSample(); o1.m[i] = o2.m[i] = AtanSample(); }
            int mode = rng() % 4;   // 0: separate, 1: out == a, 2: out == b, 3: transpose / identity
            if (mode == 0) { oEMul(&o1, &a, &b); eMulMatrix(&o2, &a, &b); }
            else if (mode == 1) { o1 = a; o2 = a; oMul(&o1, &o1, &b); bMulMatrix(&o2, &o2, &b); }
            else if (mode == 2) { o1 = b; o2 = b; oMul(&o1, &a, &o1); bMulMatrix(&o2, &a, &o2); }
            else if (it % 2) { oTr(&o1, &a); bTransposeMatrix(&o2, &a); }
            else { oId(&o1); bIdentity(&o2); }
            EXPECT(std::memcmp(&o1, &o2, sizeof o1) == 0, "matrix op mode %d pc=%x", mode, pc);
            {
                auto oInv = ORIG<void(__cdecl *)(bMatrix4 *, const bMatrix4 *)>(0x43C7C0);
                bMatrix4 i1 = o1, i2 = o1;
                oInv(&i1, &a); bInvertMatrix(&i2, &a);
                // NaN results (singular input) may differ in sign: x87 picks between two NaN operands by position
                // and the compiler may commute; any NaN is accepted for a NaN (documented in bMath.h).
                auto same = [](float x, float y) { return std::memcmp(&x, &y, 4) == 0 || (x != x && y != y); };
                int k = 0;
                while (k < 16 && same(i1.m[k], i2.m[k])) ++k;
                uint32_t b1 = 0, b2 = 0;
                if (k < 16) { std::memcpy(&b1, &i1.m[k], 4); std::memcpy(&b2, &i2.m[k], 4); }
                EXPECT(k == 16, "bInvertMatrix pc=%x elem %d: %08X vs %08X (%.9g vs %.9g)", pc, k, b1, b2, k < 16 ? double(i1.m[k]) : 0.0, k < 16 ? double(i2.m[k]) : 0.0);
                i1 = a; i2 = a;
                oInv(&i1, &i1); bInvertMatrix(&i2, &i2);
                k = 0;
                while (k < 16 && same(i1.m[k], i2.m[k])) ++k;
                EXPECT(k == 16, "bInvertMatrix in place pc=%x", pc);
            }
            {
                auto oMQ = ORIG<void(__cdecl *)(bVector4 *, const bMatrix4 *)>(0x43CBD0);
                bMatrix4 r = a;
                if (it % 3 == 0) for (int d : {0, 5, 10}) r.m[d] = -__builtin_fabsf(r.m[d]);   // force the non-trace path
                if (it % 7 == 0) r.m[5] = r.m[0];                                              // ties
                bVector4 q1{1, 2, 3, 4}, q2 = q1;
                oMQ(&q1, &r); bMatrixToQuaternion(&q2, &r);
                EXPECT(std::memcmp(&q1, &q2, sizeof q1) == 0 || (q1.x != q1.x && q2.x != q2.x), "bMatrixToQuaternion pc=%x", pc);
            }
            if (mode == 3) { o1 = a; o2 = a; oTr(&o1, &o1); bTransposeMatrix(&o2, &o2); EXPECT(std::memcmp(&o1, &o2, sizeof o1) == 0, "transpose in place"); }
        }
    }
    _controlfp(saved, _MCW_PC);
}

static void CheckVectors(int iters)
{
    auto oDist = ORIG<long double(__cdecl *)(const bVector3 *, const bVector3 *)>(0x43CE10);
    auto oN2 = ORIG<void(__cdecl *)(bVector2 *, const bVector2 *)>(0x43CE50);
    auto oN2S = ORIG<void(__cdecl *)(bVector2 *, const bVector2 *, float)>(0x43CEB0);
    auto oN3 = ORIG<void(__cdecl *)(bVector3 *, const bVector3 *)>(0x43CF10);
    auto oN3S = ORIG<void(__cdecl *)(bVector3 *, const bVector3 *, float)>(0x43CF90);
    auto oN4 = ORIG<void(__cdecl *)(bVector4 *, const bVector4 *)>(0x43D010);
    auto oSA = ORIG<void(__cdecl *)(bVector2 *, const bVector2 *, const bVector2 *, float)>(0x43D090);
    auto v = []() { return rng() % 5 == 0 ? 0.0f : AtanSample(); };
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            bVector4 in{v(), v(), v(), v()}, o1{v(), v(), v(), v()}, o2 = o1, i1 = in, i2 = in;
            float s = rng() % 6 == 0 ? 0.0f : AtanSample();
            const bool alias = rng() % 3 == 0;
            int mode = rng() % 7;
            auto run = [&](bool orig, bVector4 *o, bVector4 *i) {
                bVector4 *src = alias ? o : i;
                if (alias) *o = *i;
                switch (mode) {
                case 0: { long double d = orig ? oDist(reinterpret_cast<bVector3 *>(o), reinterpret_cast<bVector3 *>(i)) : bDistBetween(reinterpret_cast<bVector3 *>(o), reinterpret_cast<bVector3 *>(i));
                          std::memcpy(&o->w, &d, 4); break; }   // fold a little of the result in
                case 1: orig ? oN2(reinterpret_cast<bVector2 *>(o), reinterpret_cast<bVector2 *>(src)) : bNormalize(reinterpret_cast<bVector2 *>(o), reinterpret_cast<bVector2 *>(src)); break;
                case 2: orig ? oN2S(reinterpret_cast<bVector2 *>(o), reinterpret_cast<bVector2 *>(src), s) : bNormalize(reinterpret_cast<bVector2 *>(o), reinterpret_cast<bVector2 *>(src), s); break;
                case 3: orig ? oN3(reinterpret_cast<bVector3 *>(o), reinterpret_cast<bVector3 *>(src)) : bNormalize(reinterpret_cast<bVector3 *>(o), reinterpret_cast<bVector3 *>(src)); break;
                case 4: orig ? oN3S(reinterpret_cast<bVector3 *>(o), reinterpret_cast<bVector3 *>(src), s) : bNormalize(reinterpret_cast<bVector3 *>(o), reinterpret_cast<bVector3 *>(src), s); break;
                case 5: orig ? oN4(o, src) : bNormalize(o, src); break;
                default: orig ? oSA(reinterpret_cast<bVector2 *>(o), reinterpret_cast<bVector2 *>(src), reinterpret_cast<bVector2 *>(&i->z), s)
                              : bScaleAdd(reinterpret_cast<bVector2 *>(o), reinterpret_cast<bVector2 *>(src), reinterpret_cast<bVector2 *>(&i->z), s);
                }
            };
            run(true, &o1, &i1);
            run(false, &o2, &i2);
            EXPECT(std::memcmp(&o1, &o2, sizeof o1) == 0, "vector op %d alias=%d pc=%x: %08X %08X %08X %08X", mode, alias, pc,
                   *reinterpret_cast<uint32_t *>(&o1.x) ^ *reinterpret_cast<uint32_t *>(&o2.x), *reinterpret_cast<uint32_t *>(&o1.y) ^ *reinterpret_cast<uint32_t *>(&o2.y),
                   *reinterpret_cast<uint32_t *>(&o1.z) ^ *reinterpret_cast<uint32_t *>(&o2.z), *reinterpret_cast<uint32_t *>(&o1.w) ^ *reinterpret_cast<uint32_t *>(&o2.w));
        }
    }
    _controlfp(saved, _MCW_PC);
}

static void CheckBoxes(int iters)
{
    auto oEq2 = ORIG<int32_t(__cdecl *)(const bVector2 *, const bVector2 *, float)>(0x43D150);
    auto oEq3 = ORIG<int32_t(__cdecl *)(const bVector3 *, const bVector3 *, float)>(0x43D190);
    auto oCross = ORIG<void(__cdecl *)(bVector3 *, const bVector3 *, const bVector3 *)>(0x43D1E0);
    auto oI0 = ORIG<void(__cdecl *)(bVector2 *, bVector2 *)>(0x43D220);
    auto oI1 = ORIG<void(__cdecl *)(bVector2 *, bVector2 *, const bVector2 *)>(0x43D250);
    auto oE = ORIG<void(__cdecl *)(bVector2 *, bVector2 *, const bVector2 *)>(0x43D270);
    auto oER = ORIG<void(__cdecl *)(bVector2 *, bVector2 *, const bVector2 *, float)>(0x43D2F0);
    auto oE2 = ORIG<void(__cdecl *)(bVector2 *, bVector2 *, const bVector2 *, const bVector2 *)>(0x43D380);
    auto v = []() { return rng() % 5 == 0 ? 0.0f : AtanSample(); };
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            float buf1[12], buf2[12];
            for (int i = 0; i < 12; ++i) buf1[i] = buf2[i] = v();
            if (rng() % 3 == 0) { buf1[4] = buf2[4] = buf1[0]; buf1[5] = buf2[5] = buf1[1]; }   // equal values
            float e = rng() % 4 == 0 ? 0.0f : __builtin_fabsf(AtanSample());
            auto V2 = [](float *b, int i) { return reinterpret_cast<bVector2 *>(b + i); };
            auto V3 = [](float *b, int i) { return reinterpret_cast<bVector3 *>(b + i); };
            int mode = rng() % 8;
            switch (mode) {
            case 0: EXPECT(oEq2(V2(buf1, 0), V2(buf1, 4), e) == bEqual(V2(buf2, 0), V2(buf2, 4), e), "bEqual2"); break;
            case 1: EXPECT(oEq3(V3(buf1, 0), V3(buf1, 4), e) == bEqual(V3(buf2, 0), V3(buf2, 4), e), "bEqual3"); break;
            case 2: { int o = rng() % 2 ? 8 : 0; oCross(V3(buf1, o), V3(buf1, 0), V3(buf1, 3)); bCross(V3(buf2, o), V3(buf2, 0), V3(buf2, 3)); break; }
            case 3: oI0(V2(buf1, 0), V2(buf1, 2)); bInitializeBoundingBox(V2(buf2, 0), V2(buf2, 2)); break;
            case 4: oI1(V2(buf1, 0), V2(buf1, 2), V2(buf1, 4)); bInitializeBoundingBox(V2(buf2, 0), V2(buf2, 2), V2(buf2, 4)); break;
            case 5: oE(V2(buf1, 0), V2(buf1, 2), V2(buf1, 4)); bExpandBoundingBox(V2(buf2, 0), V2(buf2, 2), V2(buf2, 4)); break;
            case 6: oER(V2(buf1, 0), V2(buf1, 2), V2(buf1, 4), e); bExpandBoundingBox(V2(buf2, 0), V2(buf2, 2), V2(buf2, 4), e); break;
            default: oE2(V2(buf1, 0), V2(buf1, 2), V2(buf1, 4), V2(buf1, 6)); bExpandBoundingBox(V2(buf2, 0), V2(buf2, 2), V2(buf2, 4), V2(buf2, 6));
            }
            EXPECT(std::memcmp(buf1, buf2, sizeof buf1) == 0, "box/cross op %d pc=%x", mode, pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// SlotPool: two chained pools in one buffer; original and rewrite run on the same memory (snapshot/restore).
static void CheckSlotPoolB(int iters)
{
    auto oFull = ORIG<int32_t(__cdecl *)(const SlotPool *)>(0x43CD10);
    auto oFree = ORIG<int32_t(__cdecl *)(const SlotPool *)>(0x43CD20);
    auto oTotal = ORIG<int32_t(__cdecl *)(const SlotPool *)>(0x43CD30);
    auto oFlush = ORIG<void(THISCALL *)(SlotPool *)>(0x43CD40);
    auto oNum = ORIG<int32_t(THISCALL *)(SlotPool *, const void *)>(0x43CD80);
    auto oMalloc = ORIG<void *(THISCALL *)(SlotPool *)>(0x43CDD0);
    auto oRelease = ORIG<void(THISCALL *)(SlotPool *, void *)>(0x43CDF0);
    alignas(16) static uint8_t buf[4096], snap[4096];
    for (int it = 0; it < iters / 8; ++it) {
        std::memset(buf, 0xCD, sizeof buf);
        uint32_t size = 4 * (1 + rng() % 6);
        int32_t n1 = rng() % 9, n2 = rng() % 9;
        auto *a = reinterpret_cast<SlotPool *>(buf);
        auto *b = reinterpret_cast<SlotPool *>(buf + 0x30 + n1 * size + 16);
        std::memset(a, 0, sizeof(SlotPool)); std::memset(b, 0, sizeof(SlotPool));
        a->mu32SlotSize = b->mu32SlotSize = size;
        a->mi32NumSlots = n1; b->mi32NumSlots = n2;
        a->mpNext = rng() % 2 ? b : nullptr;
        a->mi32TotalSlots = n1 + (a->mpNext ? n2 : 0);
        a->mi32NumAllocated = rng() % 3;
        std::vector<void *> live;
        for (int step = 0; step < 24; ++step) {
            int op = step == 0 ? 0 : rng() % 6;
            std::memcpy(snap, buf, sizeof buf);
            int32_t r1 = 0, r2 = 0;
            void *p1 = nullptr, *p2 = nullptr;
            void *probe = buf + rng() % sizeof buf;
            if (op == 4 && !live.empty() && rng() % 2) probe = live[rng() % live.size()];
            void *freed = (op == 3 && !live.empty()) ? live.back() : nullptr;
            switch (op) {
            case 0: oFlush(a); break;
            case 1: r1 = oFull(a) * 10000 + oFree(a) * 100 + oTotal(a); break;
            case 2: p1 = oMalloc(a); break;
            case 3: if (freed) oRelease(a, freed); break;
            default: r1 = oNum(rng() % 8 ? a : nullptr, probe); break;
            }
            uint8_t after[4096];
            std::memcpy(after, buf, sizeof buf);
            std::memcpy(buf, snap, sizeof buf);
            SlotPool *pa = a;
            switch (op) {
            case 0: SlotPool::FlushSlotPool(a); break;
            case 1: r2 = bIsSlotPoolFull(a) * 10000 + bCountFreeSlots(a) * 100 + bCountTotalSlots(a); break;
            case 2: p2 = a->FastMalloc(); break;
            case 3: if (freed) a->Free(freed); break;
            default: r2 = 0; break;
            }
            if (op >= 4) { (void)pa; }
            EXPECT(r1 == r2 || op >= 4, "SlotPool op %d: %d vs %d", op, r1, r2);
            EXPECT(p1 == p2 && std::memcmp(after, buf, sizeof buf) == 0, "SlotPool op %d state/result", op);
            if (op == 2 && p2) live.push_back(p2);
            if (op == 3 && freed) live.pop_back();
        }
        {   // bOMalloc / bFree wrappers on the same state
            auto oO = ORIG<void *(__cdecl *)(SlotPool *)>(0x43F8F0);
            auto oF = ORIG<void(__cdecl *)(SlotPool *, void *)>(0x43F920);
            std::memcpy(snap, buf, sizeof buf);
            void *x1 = oO(a); if (x1) oF(a, x1); void *y1 = oO(a);
            uint8_t after[4096]; std::memcpy(after, buf, sizeof buf); std::memcpy(buf, snap, sizeof buf);
            void *x2 = bOMalloc(a); if (x2) bFree(a, x2); void *y2 = bOMalloc(a);
            EXPECT(x1 == x2 && y1 == y2 && std::memcmp(after, buf, sizeof buf) == 0, "bOMalloc/bFree");
        }
        // GetSlotNumber over a sweep of addresses (incl. below/above both pools) and a null pool
        for (int k = -8; k < 300; k += 1 + rng() % 5) {
            const void *q = buf + 0x30 + k * 2;
            EXPECT(oNum(a, q) == SlotPool::GetSlotNumber(a, q), "GetSlotNumber %d", k);
        }
        EXPECT(oNum(nullptr, buf) == SlotPool::GetSlotNumber(nullptr, buf), "GetSlotNumber(null)");
    }
}

static void CheckNoise(int iters)
{
    auto oNI = ORIG<long double(__cdecl *)(float)>(0x4411B0);
    auto oN = ORIG<long double(__cdecl *)(float)>(0x441270);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int i = 0; i < iters; ++i) {
            float x = rng() % 4 ? static_cast<float>(static_cast<int32_t>(rng() % 2000001) - 1000000) / 1000.0f
                                : static_cast<float>(static_cast<int32_t>(rng() % 200) - 100);
            long double a = oNI(x), b = NoiseInterpolated(x);
            EXPECT(std::memcmp(&a, &b, 10) == 0, "NoiseInterpolated(%.9g) pc=%x", double(x), pc);
            {
                auto oFov = ORIG<uint32_t(THISCALL *)(const Camera *, uint32_t)>(0x4412E0);
                alignas(4) static Camera cam;
                cam.mu16Fov = static_cast<uint16_t>(rng());
                uint32_t ang = rng();
                EXPECT((oFov(&cam, ang) & 0xFFFF) == cam.FovRelativeAngle(static_cast<uint16_t>(ang)), "FovRelativeAngle fov=%04X a=%04X", cam.mu16Fov, ang & 0xFFFF);
            }
            if (i % 4 == 0) { a = oN(x); b = Noise(x); EXPECT(std::memcmp(&a, &b, 10) == 0, "Noise(%.9g) pc=%x", double(x), pc); }
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FEPackageData ctor / non-deleting dtor on raw (0x30203) and LZ (0x30210, dtor only with null data) chunks.
static void CheckPackageDataCtor(int iters)
{
    auto oCtor = ORIG<FEPackageData *(THISCALL *)(FEPackageData *, uint32_t *)>(0x51B8E0);
    auto oDtor = ORIG<void(THISCALL *)(FEPackageData *)>(0x51B990);
    static const char *names[] = {"MAIN_MENU.fng", "Options.fng", "garage.FNG", "x", ""};
    for (int it = 0; it < iters; ++it) {
        alignas(4) uint8_t chunk[8 + 0x40];
        for (auto &b : chunk) b = static_cast<uint8_t>(rng());
        auto *c = reinterpret_cast<uint32_t *>(chunk);
        const int kind = rng() % 4;
        c[0] = kind == 3 ? rng() : kind == 2 ? 0x30210u : 0x30203u;
        if (kind == 0) {
            c[2] = 0xE76E4546u; c[4] = 0x64486B50u; c[6] = 0x20000u + rng() % 3 - (rng() % 4 == 0);
            std::strcpy(reinterpret_cast<char *>(chunk + 8 + 0x28), names[rng() % 5]);
        }
        uint8_t a[sizeof(FEPackageData)], b[sizeof(FEPackageData)];
        for (size_t i = 0; i < sizeof a; ++i) a[i] = b[i] = static_cast<uint8_t>(rng());
        auto *da = reinterpret_cast<FEPackageData *>(a), *db = reinterpret_cast<FEPackageData *>(b);
        FEPackageData *ra = oCtor(da, c);
        db->Construct(c);
        EXPECT(da->vtable == reinterpret_cast<void *>(0x79B15C) && db->vtable == bridge::FEPackageDataVTable(), "FEPackageData vtable");
        db->vtable = da->vtable;   // stub bridge vtable differs
        EXPECT(ra == da && std::memcmp(a, b, sizeof a) == 0, "FEPackageData ctor kind %d", kind);
        if (c[0] == 0x30210) { da->mpData = db->mpData = nullptr; }
        oDtor(da);
        db->Destruct();
        db->vtable = da->vtable;
        EXPECT(std::memcmp(a, b, sizeof a) == 0, "FEPackageData dtor kind %d", kind);
    }
}

// FEPackage ctor on random memory (vtables normalized: the referee bridge returns stub vtables).
static void CheckPackageCtor(int iters)
{
    auto oCtor = ORIG<FEPackage *(THISCALL *)(FEPackage *)>(0x52B100);
    for (int it = 0; it < iters; ++it) {
        alignas(4) uint8_t a[sizeof(FEPackage)], b[sizeof(FEPackage)];
        for (size_t i = 0; i < sizeof a; ++i) a[i] = b[i] = static_cast<uint8_t>(rng());
        auto *pa = reinterpret_cast<FEPackage *>(a), *pb = reinterpret_cast<FEPackage *>(b);
        FEPackage *r = oCtor(pa);
        pb->Construct();
        EXPECT(pa->node.vtable == reinterpret_cast<void *>(0x79B8F8) && pa->mList74.vtable == reinterpret_cast<void *>(0x79B878), "FEPackage ctor vtables");
        pb->node.vtable = pa->node.vtable;
        for (FEMinList FEPackage::*l : {&FEPackage::mObjects, &FEPackage::mResponses, &FEPackage::mList74, &FEPackage::mList9C}) (pb->*l).vtable = (pa->*l).vtable;
        EXPECT(r == pa && std::memcmp(a, b, sizeof a) == 0, "FEPackage ctor");
    }
}

// FEPackageReader::ReadTypeSizes (inlined FindChild) on random chunk lists: the "TypS" chunk anywhere, including the
// chunk read at the parent's end, or missing.
static void CheckReaderTypeSizes(int iters)
{
    auto oRead = ORIG<bool(THISCALL *)(FEPackageReader *)>(0x5049A0);
    for (int it = 0; it < iters; ++it) {
        uint32_t buf[128] = {};
        const int n = 1 + rng() % 6;
        uint32_t at = 2;
        for (int i = 0; i < n && at < 100; ++i) {
            const uint32_t words = rng() % 4;
            buf[at] = (rng() % 3 == 0) ? 0x53707954u : rng();
            buf[at + 1] = words * 4;
            for (uint32_t k = 0; k < words; ++k) buf[at + 2 + k] = rng();
            at += 2 + words;
        }
        buf[0] = 0xE76E4546u;
        buf[1] = (at - 2) * 4;   // consistent sizes: the scan always reaches the end chunk
        if (rng() % 4 == 0 && at < 120) buf[at] = 0x53707954u;   // only reachable as the "end" chunk
        FEPackageReader a{}, b{};
        a.mpData = b.mpData = buf;
        a.mu32NumTypes = b.mu32NumTypes = 0x77;
        const bool ra = oRead(&a), rb = b.ReadTypeSizes();
        EXPECT(ra == rb && a.mu32NumTypes == b.mu32NumTypes && a.mpTypes == b.mpTypes, "ReadTypeSizes it %d", it);
    }
}

// FEPackageReader::ProcessStringTag with the non-allocating tags (SH -> FEString::SetLabelHash, Sj/Sl/Sw, unknown).
static void CheckReaderStringTags(int iters)
{
    auto oTag = ORIG<void(THISCALL *)(FEPackageReader *, const uint8_t *)>(0x534FC0);
    static const uint16_t ids[] = {0x4853, 0x6A53, 0x6C53, 0x7753, 0x1234, 0x6253 + 1};
    for (int it = 0; it < iters; ++it) {
        alignas(8) uint8_t a[sizeof(FEString)], b[sizeof(FEString)];
        for (size_t i = 0; i < sizeof a; ++i) a[i] = b[i] = static_cast<uint8_t>(rng());
        FEPackageReader ra{}, rb{};
        ra.mpCurrentObject = reinterpret_cast<FEObject *>(a);
        rb.mpCurrentObject = reinterpret_cast<FEObject *>(b);
        uint8_t tag[8];
        const uint16_t id = ids[rng() % 6];
        uint32_t v = rng() % 3 == 0 ? reinterpret_cast<FEString *>(a)->mu32LabelHash : rng();
        std::memcpy(tag, &id, 2); tag[2] = 4; tag[3] = 0; std::memcpy(tag + 4, &v, 4);
        oTag(&ra, tag);
        rb.ProcessStringTag(tag);
        EXPECT(std::memcmp(a, b, sizeof a) == 0, "ProcessStringTag %04X", id);
    }
}

// FEListBox non-allocating methods on a synthetic list box (cells/columns/rows on the stack).
static void CheckListBoxCells(int iters)
{
    auto oSelC = ORIG<void(THISCALL *)(FEListBox *, uint32_t)>(0x504180);
    auto oSelR = ORIG<void(THISCALL *)(FEListBox *, uint32_t)>(0x5041B0);
    auto oInc = ORIG<void(THISCALL *)(FEListBox *)>(0x504230);
    auto oWrap = ORIG<void(THISCALL *)(FEListBox *, uint32_t)>(0x504270);
    auto oRecalc = ORIG<void(THISCALL *)(FEListBox *)>(0x504360);
    auto oColor = ORIG<void(THISCALL *)(FEListBox *, const FEColor *)>(0x516CD0);
    auto oScale = ORIG<void(THISCALL *)(FEListBox *, const float *)>(0x516D00);
    auto oJust = ORIG<void(THISCALL *)(FEListBox *, uint32_t, uint32_t, uint32_t)>(0x516D40);
    auto oData = ORIG<void(THISCALL *)(FEListBox *, const uint32_t *)>(0x516D80);
    auto oInit = ORIG<void(__cdecl *)(FEListBoxCell *, uint32_t)>(0x504290);
    for (int it = 0; it < iters; ++it) {
        struct Arena { FEListBox lb; FEListBoxCell cells[12]; FEListBoxSlot cols[4], rows[3]; } A, B;
        uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
        for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
        A.lb.mu32NumColumns = rng() % 5;
        A.lb.mu32NumRows = A.lb.mu32NumColumns ? rng() % 4 : 0;
        if (A.lb.mu32NumRows == 0 && A.lb.mu32NumColumns) A.lb.mu32NumRows = 1;
        A.lb.mu32SelColumn = A.lb.mu32NumColumns ? rng() % A.lb.mu32NumColumns : 0;
        A.lb.mu32SelRow = A.lb.mu32NumRows ? rng() % A.lb.mu32NumRows : 0;
        for (auto &c : A.cols) c.mfSize = static_cast<float>(rng() % 1000) / 7.0f;
        for (auto &r : A.rows) r.mfSize = static_cast<float>(rng() % 1000) / 3.0f;
        std::memcpy(&B, &A, sizeof A);
        A.lb.mpCells = A.cells; A.lb.mpColumns = A.cols; A.lb.mpRows = A.rows;
        B.lb.mpCells = B.cells; B.lb.mpColumns = B.cols; B.lb.mpRows = B.rows;
        const uint32_t x = rng() % 7, y = rng(), z = rng();
        const int op = rng() % 10;
        FEColor col{{static_cast<int32_t>(rng() % 600) - 150, static_cast<int32_t>(rng() % 600) - 150, static_cast<int32_t>(rng()), static_cast<int32_t>(rng() % 300)}};
        float xy[2] = {static_cast<float>(y % 100), static_cast<float>(z % 100)};
        uint32_t d4[4] = {y, z, x, y ^ z};
        const bool hasCell = A.lb.mu32NumColumns && A.lb.mu32NumRows;
        switch (op) {
        case 0: oSelC(&A.lb, x); B.lb.SetSelColumn(x); break;
        case 1: oSelR(&A.lb, x); B.lb.SetSelRow(x); break;
        case 2: if (hasCell) { oInc(&A.lb); B.lb.IncrementCellByColumn(); } break;
        case 3: oWrap(&A.lb, y & 0xFF); B.lb.SetAutoWrap((y & 0xFF) != 0); break;
        case 4: oRecalc(&A.lb); B.lb.RecalculateCummulative(); break;
        case 5: if (hasCell) { oColor(&A.lb, &col); B.lb.SetCellColor(col); } break;
        case 6: if (hasCell) { oScale(&A.lb, xy); B.lb.SetCellScale(xy); } break;
        case 7: if (hasCell) { oJust(&A.lb, x, y, z); B.lb.SetCellJustification(x, y, z); } break;
        case 8: if (hasCell) { oData(&A.lb, d4); B.lb.SetCellData(d4); } break;
        case 9: oInit(A.cells + (x % 4), x % 9); FEListBox::InitializeCells(B.cells + (x % 4), x % 9); break;
        }
        A.lb.mpCells = B.lb.mpCells = nullptr; A.lb.mpColumns = B.lb.mpColumns = nullptr; A.lb.mpRows = B.lb.mpRows = nullptr;
        EXPECT(std::memcmp(&A, &B, sizeof A) == 0, "FEListBox op %d", op);
    }
}

// FECodeListBox: virtual sizes / centered offsets, range setters over the visible grid, FEPositiveMod.
static void CheckCodeListBox(int iters)
{
    auto oCols = ORIG<void(THISCALL *)(FECodeListBox *, uint32_t)>(0x518BB0);
    auto oRows = ORIG<void(THISCALL *)(FECodeListBox *, uint32_t)>(0x518C00);
    auto oColor = ORIG<void(THISCALL *)(FECodeListBox *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(0x5196B0);
    auto oScale = ORIG<void(THISCALL *)(FECodeListBox *, uint32_t, uint32_t, const float *, uint32_t, uint32_t)>(0x5197D0);
    auto oJust = ORIG<void(THISCALL *)(FECodeListBox *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(0x5198F0);
    auto oMod = ORIG<int32_t(__cdecl *)(int32_t, int32_t)>(0x504790);
    for (int it = 0; it < iters; ++it) {
        struct Arena { FECodeListBox lb; FEListBoxCell cells[32]; } A, B;   // index can reach numColumns * (numRows + 1)
        uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
        for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
        A.lb.mu32NumColumns = 1 + rng() % 4;
        A.lb.mu32NumRows = 1 + rng() % 4;
        A.lb.mu32TotalColumns = 1 + rng() % 12;
        A.lb.mu32TotalRows = 1 + rng() % 12;
        A.lb.mi32ColumnOffset = static_cast<int32_t>(rng() % 12);
        A.lb.mi32RowOffset = static_cast<int32_t>(rng() % 12);
        A.lb.mu32SelColumn = rng() % 20;
        A.lb.mu32SelRow = rng() % 20;
        A.lb.mu32Flags = rng() & 0xF;
        std::memcpy(&B, &A, sizeof A);
        A.lb.mpCells = A.cells; B.lb.mpCells = B.cells;
        const int op = rng() % 6;
        const uint32_t c0 = rng() % 8, r0 = rng() % 8, nc = rng() % 4, nr = rng() % 4, v = rng();
        float xy[2] = {static_cast<float>(v % 50), 1.5f};
        const uint32_t n = 1 + rng() % 12;
        switch (op) {
        case 0: if ((A.lb.mu32Flags & 8) && A.lb.mu32NumColumns < 2 && n == 0) break; oCols(&A.lb, n); B.lb.SetTotalNumColumns(n); break;
        case 1: oRows(&A.lb, n); B.lb.SetTotalNumRows(n); break;
        case 2: oColor(&A.lb, c0, r0, v, nc, nr); B.lb.SetCellColor(c0, r0, v, nc, nr); break;
        case 3: oScale(&A.lb, c0, r0, xy, nc, nr); B.lb.SetCellScale(c0, r0, xy, nc, nr); break;
        case 4: oJust(&A.lb, c0, r0, v, nc, nr); B.lb.SetCellJustification(c0, r0, v, nc, nr); break;
        case 5: {
            const int32_t a = static_cast<int32_t>(v % 41) - 20, m = static_cast<int32_t>(1 + rng() % 6) - (rng() % 4 == 0 ? 1 : 0);
            if (m == 0 && a >= 0) break;
            EXPECT(oMod(a, m) == FEPositiveMod(a, m), "FEPositiveMod(%d, %d)", a, m);
            break;
        }
        }
        A.lb.mpCells = B.lb.mpCells = nullptr;
        if (std::memcmp(&A, &B, sizeof A) != 0 && failures < 6) {
            const uint8_t *x = reinterpret_cast<const uint8_t *>(&A), *y = reinterpret_cast<const uint8_t *>(&B);
            for (size_t i = 0; i < sizeof A; i += 4)
                if (std::memcmp(x + i, y + i, 4)) std::printf("  op %d n %u off %zx orig %08x rewrite %08x (sel %u vis %u flags %x)\n", op, n, i,
                    *reinterpret_cast<const uint32_t *>(x + i), *reinterpret_cast<const uint32_t *>(y + i), A.lb.mu32SelColumn, A.lb.mu32NumColumns, A.lb.mu32Flags);
        }
        EXPECT(std::memcmp(&A, &B, sizeof A) == 0, "FECodeListBox op %d", op);
    }
}

// FECodeListBox::FillAllCells (recording fill callback) and Update (x87 pulse, no select callback).
static std::vector<int32_t> g_fillCalls;
static void __cdecl RecordFill(void *data, FECodeListBox *, int32_t c, int32_t r)
{
    g_fillCalls.push_back(static_cast<int32_t>(reinterpret_cast<uintptr_t>(data)));
    g_fillCalls.push_back(c);
    g_fillCalls.push_back(r);
}
static void CheckCodeListBoxFillUpdate(int iters)
{
    auto oFill = ORIG<void(THISCALL *)(FECodeListBox *)>(0x504C40);
    auto oUpdate = ORIG<void(THISCALL *)(FECodeListBox *, float)>(0x504DC0);
    for (int it = 0; it < iters; ++it) {
        FECodeListBox A, B;
        uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
        for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
        A.mpSelectCallback = nullptr;
        if (rng() % 2) {
            A.mu32NumColumns = rng() % 5; A.mu32NumRows = rng() % 5;
            A.mu32TotalColumns = rng() % 7; A.mu32TotalRows = rng() % 7;
            A.mi32ColumnOffset = static_cast<int32_t>(rng() % 9) - 2; A.mi32RowOffset = static_cast<int32_t>(rng() % 9) - 2;
            A.mpFillCallback = RecordFill;
            A.mpFillCallbackData = reinterpret_cast<void *>(uintptr_t{0x1234});
            std::memcpy(&B, &A, sizeof A);
            g_fillCalls.clear();
            oFill(&A);
            std::vector<int32_t> orig = g_fillCalls;
            g_fillCalls.clear();
            B.FillAllCells();
            EXPECT(orig == g_fillCalls && std::memcmp(&A, &B, sizeof A) == 0, "FECodeListBox::FillAllCells");
        } else {
            A.mfPulse = static_cast<float>(static_cast<int32_t>(rng() % 3000) - 1000) / 1000.0f;
            A.mfPulseRate = static_cast<float>(static_cast<int32_t>(rng() % 2001) - 1000) / 777.0f;
            std::memcpy(&B, &A, sizeof A);
            const float dt = static_cast<float>(rng() % 1000) / 333.0f;
            oUpdate(&A, dt);
            B.Update(dt);
            EXPECT(std::memcmp(&A, &B, sizeof A) == 0, "FECodeListBox::Update dt %g", dt);
        }
    }
}

// FECodeListBox movement: CheckMovement / MakeMove / per-axis ScrollSelection (cell shifts + refills through a fake
// game interface whose vtable records messages and fill requests) and the default select callback.
static std::vector<uint32_t> g_giLog;
static void THISCALL FakeGiMessage(void *, uint32_t m, void *o, uint32_t a, uint32_t e)
{
    g_giLog.insert(g_giLog.end(), {1u, m, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(o)), a, e});
}
static void THISCALL FakeGiFill(void *, void *lb, int32_t c, int32_t r)
{
    g_giLog.insert(g_giLog.end(), {2u, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lb)), static_cast<uint32_t>(c), static_cast<uint32_t>(r)});
}
static void CheckCodeListBoxMove(int iters)
{
    static void *vtable[32] = {};
    vtable[0x18 / 4] = reinterpret_cast<void *>(&FakeGiMessage);
    vtable[0x4C / 4] = reinterpret_cast<void *>(&FakeGiFill);
    static void *gi = vtable;
    auto oCheck = ORIG<bool(THISCALL *)(FECodeListBox *, int32_t, int32_t, uint32_t, int32_t, int32_t)>(0x518E70);
    auto oMove = ORIG<bool(THISCALL *)(FECodeListBox *, int32_t, int32_t *, uint32_t *, int32_t, int32_t)>(0x504E40);
    auto oAxis = ORIG<bool(THISCALL *)(FECodeListBox *, int32_t, int32_t *, uint32_t *, uint32_t, uint32_t, uint32_t)>(0x518F20);
    auto oSel = ORIG<void(__cdecl *)(FECodeListBox *)>(0x518E20);
    for (int it = 0; it < iters; ++it) {
        struct Arena { FECodeListBox lb; FEListBoxCell cells[25]; } A, B;
        uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
        for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
        A.lb.mu32NumColumns = 1 + rng() % 5;
        A.lb.mu32NumRows = 1 + rng() % 5;
        A.lb.mu32TotalColumns = 1 + rng() % 9;
        A.lb.mu32TotalRows = 1 + rng() % 9;
        A.lb.mi32ColumnOffset = static_cast<int32_t>(rng() % A.lb.mu32TotalColumns);
        A.lb.mi32RowOffset = static_cast<int32_t>(rng() % A.lb.mu32TotalRows);
        A.lb.mu32SelColumn = rng() % A.lb.mu32TotalColumns;
        A.lb.mu32SelRow = rng() % A.lb.mu32TotalRows;
        A.lb.mu32Flags = rng() & 0xE;
        A.lb.mpGameInterface = &gi;
        A.lb.mpFillCallback = (rng() % 2) ? RecordFill : nullptr;
        A.lb.mpFillCallbackData = reinterpret_cast<void *>(uintptr_t{0x77});
        A.lb.mfPulse = static_cast<float>(rng() % 1000) / 999.0f;
        std::memcpy(&B, &A, sizeof A);
        A.lb.mpCells = A.cells; B.lb.mpCells = B.cells;
        const int op = rng() % 4;
        const int32_t dir = static_cast<int32_t>(rng() % 5) - 2;
        bool ra = false, rb = false;
        std::vector<int32_t> fa;
        std::vector<uint32_t> ga;
        g_fillCalls.clear(); g_giLog.clear();
        switch (op) {
        case 0: ra = oCheck(&A.lb, dir, A.lb.mi32ColumnOffset, A.lb.mu32SelColumn, static_cast<int32_t>(A.lb.mu32TotalColumns), static_cast<int32_t>(A.lb.mu32NumColumns)); break;
        case 1: ra = oMove(&A.lb, dir, &A.lb.mi32RowOffset, &A.lb.mu32SelRow, static_cast<int32_t>(A.lb.mu32TotalRows), static_cast<int32_t>(A.lb.mu32NumRows)); break;
        case 2: {
            const bool cols = rng() % 2;
            ra = cols ? oAxis(&A.lb, dir, &A.lb.mi32ColumnOffset, &A.lb.mu32SelColumn, A.lb.mu32TotalColumns, A.lb.mu32NumColumns, 1)
                      : oAxis(&A.lb, dir, &A.lb.mi32RowOffset, &A.lb.mu32SelRow, A.lb.mu32TotalRows, A.lb.mu32NumRows, 0);
            fa = g_fillCalls; ga = g_giLog; g_fillCalls.clear(); g_giLog.clear();
            rb = cols ? B.lb.ScrollSelection(dir, &B.lb.mi32ColumnOffset, &B.lb.mu32SelColumn, B.lb.mu32TotalColumns, B.lb.mu32NumColumns, true)
                      : B.lb.ScrollSelection(dir, &B.lb.mi32RowOffset, &B.lb.mu32SelRow, B.lb.mu32TotalRows, B.lb.mu32NumRows, false);
            break;
        }
        case 3: oSel(&A.lb); FECodeListBox::DefaultSelectCallback(&B.lb); break;
        }
        if (op == 0) { fa = g_fillCalls; ga = g_giLog; g_fillCalls.clear(); g_giLog.clear();
            rb = B.lb.CheckMovement(dir, B.lb.mi32ColumnOffset, B.lb.mu32SelColumn, static_cast<int32_t>(B.lb.mu32TotalColumns), static_cast<int32_t>(B.lb.mu32NumColumns)); }
        if (op == 1) rb = B.lb.MakeMove(dir, &B.lb.mi32RowOffset, &B.lb.mu32SelRow, static_cast<int32_t>(B.lb.mu32TotalRows), static_cast<int32_t>(B.lb.mu32NumRows));
        // object pointers in the logs differ (A vs B): compare after mapping them
        for (size_t i = 0; i < ga.size(); ++i) if (ga[i] == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&A.lb))) ga[i] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&B.lb));
        A.lb.mpCells = B.lb.mpCells = nullptr;
        EXPECT(ra == rb && fa == g_fillCalls && ga == g_giLog && std::memcmp(&A, &B, sizeof A) == 0, "FECodeListBox move op %d dir %d flags %x", op, dir, A.lb.mu32Flags);
    }
}

// FEMultMatrix / FEButtonMap::ComputeButtonLocation / GetButtonFrom on random layouts; fake game interface whose
// +0x2C transform succeeds for odd contexts (matrix derived from the context id). Both FPU precisions.
static uint8_t THISCALL FakeGiTransform(void *, uint32_t ctx, float *m)
{
    if (!(ctx & 1)) return 0;
    uint32_t x = ctx * 2654435761u;
    for (int i = 0; i < 16; ++i) { x = x * 1103515245u + 12345u; m[i] = static_cast<float>(static_cast<int32_t>(x >> 8) % 4000) / 997.0f; }
    return 1;
}
static float RefCoord()
{
    switch (rng() % 4) {
    case 0: return static_cast<float>(static_cast<int32_t>(rng() % 1400) - 380);
    case 1: return static_cast<float>(static_cast<int32_t>(rng() % 1400) - 380) * 0.37f;
    case 2: return static_cast<float>(rng() % 5) * 160.0f;
    default: return static_cast<float>(static_cast<int32_t>(rng() % 200001) - 100000) / 77.0f;
    }
}
static void CheckButtonMap(int iters)
{
    static void *vtable[32] = {};
    vtable[0x2C / 4] = reinterpret_cast<void *>(&FakeGiTransform);
    static void *gi = vtable;
    auto oMult = ORIG<void(__cdecl *)(FEVector3 *, const float *, const FEVector3 *)>(0x504B50);
    auto oLoc = ORIG<void(THISCALL *)(const FEButtonMap *, FEObject *, void *, FEVector2 *)>(0x504BB0);
    auto oGet = ORIG<FEObject *(THISCALL *)(const FEButtonMap *, FEObject *, uint32_t, void *, uint32_t)>(0x5189E0);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            float m[16];
            for (float &f : m) f = RefCoord() / 300.0f;
            FEVector3 v{RefCoord(), RefCoord(), RefCoord()}, a{}, b{};
            oMult(&a, m, &v);
            FEMultMatrix(&b, m, &v);
            EXPECT(std::memcmp(&a, &b, sizeof a) == 0, "FEMultMatrix pc=%x", pc);
            alignas(8) static unsigned char om[12][sizeof(FEObject)];
            static float data[12][10];
            FEObject *buttons[12];
            const uint32_t n = 1 + rng() % 12;
            for (uint32_t i = 0; i < n; ++i) {
                buttons[i] = reinterpret_cast<FEObject *>(om[i]);
                std::memset(om[i], 0, sizeof om[i]);
                buttons[i]->mpData = data[i];
                buttons[i]->mu32Flags = rng() % 6 == 0 ? 0x4000000 : 0;
                buttons[i]->mu16RenderContext = static_cast<uint16_t>(rng() % 3 ? 0 : rng() % 4);
                const bool dup = i && rng() % 8 == 0;
                data[i][7] = dup ? data[i - 1][7] : RefCoord();
                data[i][8] = dup ? data[i - 1][8] : RefCoord();
                data[i][9] = RefCoord();
            }
            FEButtonMap map{buttons, rng() % 10 == 0 ? 0 : n};
            void *g = rng() % 4 ? &gi : nullptr;
            FEObject *from = buttons[rng() % n];
            FEVector2 la{}, lb{};
            oLoc(&map, from, g, &la);
            map.ComputeButtonLocation(from, g, &lb);
            EXPECT(std::memcmp(&la, &lb, sizeof la) == 0, "ComputeButtonLocation ctx %u pc=%x: %a %a vs %a %a (pos %a %a %a)", from->mu16RenderContext, pc, la.x, la.y, lb.x, lb.y, static_cast<float *>(from->mpData)[7], static_cast<float *>(from->mpData)[8], static_cast<float *>(from->mpData)[9]);
            const uint32_t dir = rng() % 8, wrap = rng() % 4;
            FEObject *ra = oGet(&map, from, dir, g, wrap), *rb = map.GetButtonFrom(from, dir, g, wrap);
            EXPECT(ra == rb, "GetButtonFrom n %u dir %u wrap %u pc=%x: %p vs %p", n, dir, wrap, pc, (void *)ra, (void *)rb);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// Render path: FEQuaternion::GetMatrix, FEMultMatrix(4x4), sorting globals, radix sort, RenderObject, RenderGroup
// (synthetic engine, fake game interface logging GenerateRenderContext). Both FPU precisions.
static std::vector<uint32_t> g_ctxLog;
static void THISCALL FakeGiGenCtx(void *, uint32_t c, FEObject *g)
{
    g_ctxLog.push_back(c);
    g_ctxLog.push_back(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(g)));
}
static void CheckRender(int iters)
{
    static void *vtable[32] = {};
    vtable[0x28 / 4] = reinterpret_cast<void *>(&FakeGiGenCtx);
    static void *gi = vtable;
    auto oQuat = ORIG<void(THISCALL *)(const FEQuaternion *, FEMatrix4 *)>(0x504A50);
    auto oMul = ORIG<void(__cdecl *)(FEMatrix4 *, const FEMatrix4 *, const FEMatrix4 *)>(0x5187A0);
    auto oInit = ORIG<void(__cdecl *)(FEPackage *, void *, int32_t)>(0x50B410);
    auto oClose = ORIG<void(__cdecl *)(FEPackage *)>(0x50B430);
    auto oSort = ORIG<void(THISCALL *)(FEngine::ObjectSorter *)>(0x5114F0);
    auto oObj = ORIG<void(THISCALL *)(FEngine *, FEObject *, const FEMatrix4 *, uint32_t)>(0x51A970);
    auto oGroup = ORIG<void(THISCALL *)(FEngine *, FEGroup *, const FEMatrix4 *, const FEMatrix4 *, uint32_t)>(0x52C5D0);
    alignas(16) static unsigned char ea[sizeof(FEngine)], eb[sizeof(FEngine)];
    auto *A = reinterpret_cast<FEngine *>(ea), *B = reinterpret_cast<FEngine *>(eb);
    auto rf = [] { return rng() % 5 == 0 ? 0.0f : RefCoord() / static_cast<float>(1 + rng() % 400); };
    const unsigned int saved = _controlfp(0, 0);
    FESortGlobals *g = bridge::FEngSortGlobals();
    const FESortGlobals savedGlobals = *g;
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            FEQuaternion q{rf(), rf(), rf(), rf()};
            FEMatrix4 ma, mb, x, y;
            for (int i = 0; i < 16; ++i) { x.m[i] = rf(); y.m[i] = rf(); }
            oQuat(&q, &ma);
            FEQuatGetMatrix(&q, &mb);
            EXPECT(std::memcmp(&ma, &mb, sizeof ma) == 0, "FEQuaternion::GetMatrix pc=%x", pc);
            oMul(&ma, &x, &y);
            FEMultMatrix(&mb, &x, &y);
            EXPECT(std::memcmp(&ma, &mb, sizeof ma) == 0, "FEMultMatrix(4x4) pc=%x", pc);
            // sorting globals
            FESortGlobals start{static_cast<int32_t>(rng() % 50), rng() % 50, static_cast<int32_t>(rng() % 50), reinterpret_cast<void *>(uintptr_t(rng()))};
            *g = start;
            const int32_t size = static_cast<int32_t>(rng() % 0x3000);
            oInit(nullptr, &q, size);
            oClose(nullptr);
            const FESortGlobals ga = *g;
            *g = start;
            InitFEngObjectSorting(nullptr, &q, size);
            CloseFEngObjectSorting(nullptr);
            EXPECT(std::memcmp(&ga, g, sizeof ga) == 0, "Init/CloseFEngObjectSorting");
            *g = start;
            oInit(nullptr, &q, size);
            const FESortGlobals gi1 = *g;
            *g = start;
            InitFEngObjectSorting(nullptr, &q, size);
            EXPECT(std::memcmp(&gi1, g, sizeof gi1) == 0, "InitFEngObjectSorting");
            // radix sort (every 4th iteration: big arrays)
            if (it % 4 == 0) {
                const uint32_t n = rng() % 3 ? rng() % 40 : rng() % 1025;
                const uint32_t mask = rng() % 2 ? 0xFFFFFFFFu : 0x0303030Fu;
                A->mSorter.mu32Count = n;
                for (uint32_t i = 0; i < 2048; ++i) {
                    auto &en = i < 1024 ? A->mSorter.mEntries[i] : A->mSorter.mScratch[i - 1024];
                    en = {reinterpret_cast<FEObject *>(uintptr_t(rng())), rng() & mask};
                }
                std::memcpy(&B->mSorter, &A->mSorter, sizeof A->mSorter);
                oSort(&A->mSorter);
                B->mSorter.SortObjects();
                EXPECT(std::memcmp(&A->mSorter, &B->mSorter, sizeof A->mSorter) == 0, "SortObjects n %u", n);
            }
            // RenderObject / RenderGroup on a small random tree
            alignas(8) static unsigned char om[8][sizeof(FEGroup)];
            static float data[8][16];
            FEObject *o[8];
            for (int i = 0; i < 8; ++i) {
                o[i] = reinterpret_cast<FEObject *>(om[i]);
                std::memset(om[i], 0, sizeof om[i]);
                o[i]->mpData = data[i];
                for (int k = 0; k < 16; ++k) data[i][k] = rf();
                const int32_t alpha = rng() % 6 == 0 ? 0 : 255;
                std::memcpy(&data[i][3], &alpha, 4);
                o[i]->mu32Flags = rng() % 4 == 0 ? 0x80000000u : 0;
                o[i]->mu16RenderContext = static_cast<uint16_t>(rng());
            }
            // o[0] group: children 1, 2 (group: children 3, 4), 5; others plain objects
            auto *g0 = reinterpret_cast<FEGroup *>(o[0]);
            auto *g2 = reinterpret_cast<FEGroup *>(o[2]);
            o[0]->mu32Type = 5;
            o[2]->mu32Type = 5;
            auto link = [](FEGroup *grp, std::initializer_list<FEObject *> kids) {
                FEObject *prev = nullptr;
                grp->mChildren.head = kids.size() ? &(*kids.begin())->node : nullptr;
                for (FEObject *k : kids) {
                    k->node.prev = prev ? &prev->node : nullptr;
                    if (prev) prev->node.next = &k->node;
                    k->node.next = nullptr;
                    prev = k;
                }
            };
            link(g0, {o[1], o[2], o[5]});
            link(g2, {o[3], o[4]});
            for (unsigned char *e : {ea, eb}) std::memset(e, 0, 0x2200);   // header + sorter entries
            A->mpUIData = &gi;
            A->_unknown000 = static_cast<uint8_t>(rng() % 2);
            A->mu16RenderContextCount = static_cast<uint16_t>(rng() % 70000);
            A->mSorter.mu32Count = rng() % 4;
            std::memcpy(eb, ea, 0x2200);
            const bool asGroup = rng() % 3 != 0;
            const uint16_t ctx = static_cast<uint16_t>(rng());
            unsigned char before[8][sizeof(FEGroup)];
            std::memcpy(before, om, sizeof om);
            g_ctxLog.clear();
            if (asGroup) oGroup(A, g0, &x, &y, ctx); else oObj(A, o[1], &x, ctx);
            std::vector<uint32_t> logA = g_ctxLog;
            unsigned char after[8][sizeof(FEGroup)];
            std::memcpy(after, om, sizeof om);
            std::memcpy(om, before, sizeof om);
            g_ctxLog.clear();
            if (asGroup) B->RenderGroup(g0, &x, &y, ctx); else B->RenderObject(o[1], &x, ctx);
            EXPECT(std::memcmp(ea, eb, 0x2200) == 0 && logA == g_ctxLog && std::memcmp(after, om, sizeof om) == 0,
                   "%s pc=%x count %u vs %u", asGroup ? "RenderGroup" : "RenderObject", pc, A->mSorter.mu32Count, B->mSorter.mu32Count);
        }
    }
    *g = savedGlobals;
    _controlfp(saved, _MCW_PC);
}

// FETypeNode::AddFieldNode, FETypeLib::FindType on synthetic lists (no allocation).
static void CheckTypeLib(int iters)
{
    auto oGet = ORIG<FEFieldNode *(THISCALL *)(const FETypeNode *, const char *)>(0x519F10);
    auto oFind = ORIG<FETypeNode *(THISCALL *)(const FETypeLib *, uint32_t)>(0x505480);
    auto oAdd = ORIG<void(THISCALL *)(FETypeNode *, FEFieldNode *)>(0x514780);
    static const char *const names[] = {"Color", "color", "COLOR", "Pivot", "Upper Left", "upper left", "Size", "", "Sizes", "Frame Number", "Top Left"};
    for (int it = 0; it < iters; ++it) {
        static FEFieldNode fa[8], fb[8];
        static FETypeNode ta, tb;
        const uint32_t n = rng() % 8;   // fa[n] is what AddFieldNode appends (not yet in the list)
        std::memset(&ta, 0, sizeof ta);
        std::memset(fa, 0, sizeof fa);
        FEMinNode *prev = nullptr;
        for (uint32_t i = 0; i < n; ++i) {
            fa[i].node.mpcName = const_cast<char *>(names[rng() % 11]);
            fa[i].mu32ValueSize = rng() % 3 ? kFEKeyTypeSize[rng() % 8] : rng();
            fa[i].mu32Offset = rng();
            fa[i].node.node.prev = prev;
            if (prev) prev->next = &fa[i].node.node; else ta.mParams.head = &fa[i].node.node;
            prev = &fa[i].node.node;
        }
        ta.mParams.tail = prev;
        ta.mParams.count = n;
        // GetField (519F10) is not compared here: the original calls the CRT _stricmp (75DD6F), which crashes before
        // the CRT is initialized (DllMain). Covered by unit tests + in game.
        (void)oGet;
        // FindType over the fields reused as type nodes is not meaningful; build a type list instead
        static FETypeNode types[6];
        FETypeLib lib{};
        FEMinNode *tp = nullptr;
        const uint32_t nt = rng() % 7;
        for (uint32_t i = 0; i < nt; ++i) {
            std::memset(&types[i], 0, sizeof types[i]);
            types[i].mu32Type = rng() % 12;
            types[i].node.node.prev = tp;
            if (tp) tp->next = &types[i].node.node; else lib.mTypes.head = &types[i].node.node;
            tp = &types[i].node.node;
        }
        const uint32_t want = rng() % 13;
        EXPECT(oFind(&lib, want) == lib.FindType(want), "FindType %u", want);
        // AddFieldNode: same list in two copies (fb mirrors fa)
        std::memcpy(&tb, &ta, sizeof ta);
        std::memcpy(fb, fa, sizeof fa);
        auto rebase = [&](FEMinNode *x) -> FEMinNode * { return x ? reinterpret_cast<FEMinNode *>(reinterpret_cast<uint8_t *>(x) - reinterpret_cast<uint8_t *>(fa) + reinterpret_cast<uint8_t *>(fb)) : nullptr; };
        tb.mParams.head = rebase(ta.mParams.head);
        tb.mParams.tail = rebase(ta.mParams.tail);
        for (uint32_t i = 0; i < n; ++i) { fb[i].node.node.next = rebase(fa[i].node.node.next); fb[i].node.node.prev = rebase(fa[i].node.node.prev); }
        const bool addNull = rng() % 4 == 0;
        oAdd(&ta, addNull ? nullptr : &fa[n]);
        tb.AddFieldNode(addNull ? nullptr : &fb[n]);
        bool same = ta.mParams.count == tb.mParams.count;
        for (int i = 0; i < 8; ++i) same &= fa[i].mu32Offset == fb[i].mu32Offset && rebase(fa[i].node.node.next) == fb[i].node.node.next;
        EXPECT(same, "AddFieldNode n %u null %d", n, addNull);
    }
}

// cFEng::HandleJoy on the real event table (saved and restored).
static void CheckHandleJoy(int iters)
{
    auto o = ORIG<void(__cdecl *)(uint32_t, uint32_t, uint32_t, void *)>(0x50C630);
    FEngJoyMapEntry *map = bridge::FEngJoyMap();
    FEngJoyMapEntry saved[kFEngJoyMapCount], a[kFEngJoyMapCount];
    std::memcpy(saved, map, sizeof saved);
    for (int it = 0; it < iters; ++it) {
        const uint32_t port = rng() % 4, event = rng() % 3 ? map[rng() % kFEngJoyMapCount].mu32Event : rng() % 0x60;
        const uint32_t state = rng();   // only the low byte counts
        o(port, event, state, nullptr);
        std::memcpy(a, map, sizeof a);
        std::memcpy(map, saved, sizeof saved);
        cFEng::HandleJoy(port, event, static_cast<uint8_t>(state), nullptr);
        int bad = -1;
        for (int i = 0; i < kFEngJoyMapCount && bad < 0; ++i) if (std::memcmp(&a[i], &map[i], sizeof a[i])) bad = i;
        EXPECT(bad < 0, "HandleJoy port %u event %x state %x: entry %d ev %x orig %x,%x,%x,%x vs %x,%x,%x,%x", port, event, state, bad,
               bad < 0 ? 0 : map[bad].mu32Event, bad < 0 ? 0 : a[bad].mState[0], bad < 0 ? 0 : a[bad].mState[1], bad < 0 ? 0 : a[bad].mState[2], bad < 0 ? 0 : a[bad].mState[3],
               bad < 0 ? 0 : map[bad].mState[0], bad < 0 ? 0 : map[bad].mState[1], bad < 0 ? 0 : map[bad].mState[2], bad < 0 ? 0 : map[bad].mState[3]);
        std::memcpy(map, saved, sizeof saved);
    }
}

// FEng helpers (texture hash, visibility with group recursion, script finished, size) on random object trees.
static void CheckFEngUtil(int iters)
{
    auto oGetTex = ORIG<uint32_t(__cdecl *)(const FEObject *)>(0x50C980);
    auto oSetTex = ORIG<void(__cdecl *)(FEObject *, uint32_t)>(0x50C990);
    auto oIsVis = ORIG<int32_t(__cdecl *)(const FEObject *)>(0x50C9E0);
    auto oInvis = ORIG<void(__cdecl *)(FEObject *)>(0x50CA00);
    auto oVis = ORIG<void(__cdecl *)(FEObject *)>(0x50CA50);
    auto oFin = ORIG<uint32_t(__cdecl *)(const FEObject *, const char *)>(0x50CAB0);
    auto oSize = ORIG<void(__cdecl *)(FEObject *, float, float)>(0x50CB00);
    static const char *const names[] = {"Fade_In", "fade_in", "Highlight", "Init", "", "UnHighlight"};
    struct Arena { unsigned char om[6][sizeof(FEGroup)]; float data[6][20]; unsigned char sm[4][sizeof(FEScript)]; };
    static Arena A, B;
    for (int it = 0; it < iters; ++it) {
        Arena &a = A;
        std::memset(&a, 0, sizeof a);
        FEObject *o[6];
        for (int i = 0; i < 6; ++i) {
            o[i] = reinterpret_cast<FEObject *>(a.om[i]);
            o[i]->mpData = a.data[i];
            o[i]->mu32Flags = rng() & 0x400003;
            o[i]->mu32Type = rng() % 3 ? 1 : 5;
            o[i]->_unknown24 = rng() % 3;
        }
        // o[0] group with children 1..3; o[1] group (if type 5) with children 4..5
        auto link = [&](int grp, std::initializer_list<int> kids) {
            auto *g = reinterpret_cast<FEGroup *>(o[grp]);
            FEObject *prev = nullptr;
            for (int k : kids) {
                if (prev) prev->node.next = &o[k]->node; else g->mChildren.head = &o[k]->node;
                prev = o[k];
                ++g->mChildren.count;
            }
        };
        o[0]->mu32Type = 5;
        link(0, {1, 2, 3});
        if (o[1]->mu32Type == 5) link(1, {4, 5});
        for (int k = 0; k < 4; ++k) {   // scripts on o[2]
            auto *sc = reinterpret_cast<FEScript *>(a.sm[k]);
            sc->mu32Id = FEHashUpper(names[rng() % 6]);
            sc->mi32Length = static_cast<int32_t>(rng() % 5) - 1;
            sc->mi32Time = static_cast<int32_t>(rng() % 5) - 1;
            if (k) reinterpret_cast<FEScript *>(a.sm[k - 1])->node.next = &sc->node; else o[2]->mScripts.head = &sc->node;
        }
        // B mirrors A at a different address: rebase every pointer
        std::memcpy(&B, &A, sizeof A);
        const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
        auto rb = [&](void *p) -> void * {
            uint8_t *q = static_cast<uint8_t *>(p);
            return (q >= reinterpret_cast<uint8_t *>(&A) && q < reinterpret_cast<uint8_t *>(&A) + sizeof A) ? q + delta : p;
        };
        FEObject *ob[6];
        for (int i = 0; i < 6; ++i) {
            ob[i] = reinterpret_cast<FEObject *>(B.om[i]);
            ob[i]->mpData = rb(ob[i]->mpData);
            ob[i]->node.next = static_cast<FEMinNode *>(rb(ob[i]->node.next));
            ob[i]->mScripts.head = static_cast<FEMinNode *>(rb(ob[i]->mScripts.head));
            auto *g = reinterpret_cast<FEGroup *>(ob[i]);
            g->mChildren.head = static_cast<FEMinNode *>(rb(g->mChildren.head));
        }
        for (int k = 0; k < 4; ++k) { auto *sc = reinterpret_cast<FEScript *>(B.sm[k]); sc->node.next = static_cast<FEMinNode *>(rb(sc->node.next)); }
        const int which = static_cast<int>(rng() % 6), op = static_cast<int>(rng() % 7);
        const uint32_t v = rng() % 3;
        const char *nm = names[rng() % 6];
        const float w = static_cast<float>(rng() % 100), h = static_cast<float>(rng() % 100);
        uint32_t ra = 0, rbv = 0;
        switch (op) {
        case 0: ra = oGetTex(o[which]); rbv = FEngGetTextureHash(ob[which]); break;
        case 1: oSetTex(o[which], v); FEngSetTextureHash(ob[which], v); break;
        case 2: ra = static_cast<uint32_t>(oIsVis(o[which])); rbv = static_cast<uint32_t>(FEngIsVisible(ob[which])); break;
        case 3: oInvis(o[which]); FEngSetInvisible(ob[which]); break;
        case 4: oVis(o[which]); FEngSetVisible(ob[which]); break;
        case 5: ra = oFin(o[which], nm) & 0xFF; rbv = FEngIsScriptFinished(ob[which], nm); break;
        default: oSize(o[which], w, h); FEngSetSize(ob[which], w, h); break;
        }
        bool same = ra == rbv;
        for (int i = 0; i < 6; ++i) same &= o[i]->mu32Flags == ob[i]->mu32Flags && o[i]->_unknown24 == ob[i]->_unknown24 && std::memcmp(A.data[i], B.data[i], sizeof A.data[i]) == 0;
        EXPECT(same, "FEng util op %d obj %d: %x vs %x", op, which, ra, rbv);
    }
}

// FEngSetRotationZ on objects without scripts (rotation data + dirty flag), both FPU precisions.
static void CheckFEngRotationZ(int iters)
{
    auto o = ORIG<void(__cdecl *)(FEObject *, float)>(0x51CED0);
    alignas(8) static unsigned char ma[sizeof(FEObject)], mb[sizeof(FEObject)];
    static float da[20], db[20];
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            std::memset(ma, 0, sizeof ma);
            for (float &f : da) f = RefCoord() / 300.0f;
            auto *a = reinterpret_cast<FEObject *>(ma);
            a->mpData = da;
            a->mu32Flags = rng() & 3;
            std::memcpy(mb, ma, sizeof mb);
            std::memcpy(db, da, sizeof db);
            auto *b = reinterpret_cast<FEObject *>(mb);
            b->mpData = db;
            const float deg = rng() % 4 ? static_cast<float>(static_cast<int32_t>(rng() % 721) - 360) : RefCoord();
            o(a, deg);
            FEngSetRotationZ(b, deg);
            EXPECT(std::memcmp(da, db, sizeof da) == 0 && a->mu32Flags == b->mu32Flags, "FEngSetRotationZ(%g) pc=%x", deg, pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FEng position helpers on random objects (boxes, strings without a loaded font, groups, others), both precisions.
static void CheckFEngPosition(int iters)
{
    using Get = void(__cdecl *)(const FEObject *, float *, float *);
    using Set = void(__cdecl *)(FEObject *, float, float);
    const Get og[4] = {ORIG<Get>(0x51D070), ORIG<Get>(0x51D170), ORIG<Get>(0x51D620), ORIG<Get>(0x51D730)};
    const Get rg[4] = {&FEngGetTopLeft, &FEngGetBottomRight, &FEngGetCenter, &FEngGetSize};
    const Set os[3] = {ORIG<Set>(0x51D380), ORIG<Set>(0x51D2D0), ORIG<Set>(0x51D4D0)};
    const Set rs[3] = {&FEngSetTopLeft, &FEngSetBottomRight, &FEngSetCenter};
    // no strings (type 2): FindFont (51BD70) walks the font list, which is not set up while the referee runs
    static const uint32_t types[] = {0, 1, 3, 3, 4, 5, 6, 7, 8, 9, 10, 0x100, 0x200};
    alignas(8) static unsigned char ma[sizeof(FEString)], mb[sizeof(FEString)];
    static float da[20], db[20];
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            std::memset(ma, 0, sizeof ma);
            for (float &f : da) f = rng() % 5 ? RefCoord() / static_cast<float>(1 + rng() % 7) : 0.0f;
            auto *a = reinterpret_cast<FEObject *>(ma);
            a->mpData = da;
            a->mu32Type = types[rng() % 13];
            a->mu32Flags = rng() & 0x400003;
            a->_unknown24 = rng();   // font hash: no fonts are loaded while the referee runs
            std::memcpy(mb, ma, sizeof mb);
            std::memcpy(db, da, sizeof db);
            auto *b = reinterpret_cast<FEObject *>(mb);
            b->mpData = db;
            const int op = static_cast<int>(rng() % 7);
            if (op < 4) {
                float ax = 1234.5f, ay = -77.25f, bx = ax, by = ay;
                og[op](a, &ax, &ay);
                rg[op](b, &bx, &by);
                EXPECT(std::memcmp(&ax, &bx, 4) == 0 && std::memcmp(&ay, &by, 4) == 0, "FEng get op %d type %x pc=%x: %g,%g vs %g,%g", op, a->mu32Type, pc, ax, ay, bx, by);
            } else {
                const float x = RefCoord(), y = RefCoord();
                os[op - 4](a, x, y);
                rs[op - 4](b, x, y);
                EXPECT(std::memcmp(da, db, sizeof da) == 0 && a->mu32Flags == b->mu32Flags, "FEng set op %d type %x pc=%x", op, a->mu32Type, pc);
            }
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FEButtonWidget constructor / Show / Hide / Position on synthetic objects (boxes and plain objects), both precisions.
static void CheckButtonWidget(int iters)
{
    auto oCtor = ORIG<FEButtonWidget *(THISCALL *)(FEButtonWidget *, const float *, const float *, uint32_t)>(0x51DFC0);
    auto oShow = ORIG<void(THISCALL *)(FEButtonWidget *)>(0x50D530);
    auto oHide = ORIG<void(THISCALL *)(FEButtonWidget *)>(0x50D550);
    auto oPos = ORIG<void(THISCALL *)(FEButtonWidget *)>(0x52D6B0);
    struct Arena { FEButtonWidget w; unsigned char om[2][sizeof(FEString)]; float d[2][20]; };
    static Arena A, B;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            const float pos[2] = {RefCoord(), RefCoord()}, size[2] = {RefCoord(), RefCoord()};
            oCtor(&A.w, pos, size, rng() & 1);   // a constructed widget (vtable, offsets) for the member functions
            // set up objects for the member functions
            for (int k = 0; k < 2; ++k) {
                for (float &f : A.d[k]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                auto *o = reinterpret_cast<FEObject *>(A.om[k]);
                std::memset(A.om[k], 0, sizeof A.om[k]);
                o->mpData = A.d[k];
                o->mu32Type = rng() % 2 ? 1 : 3;
                o->mu32Flags = rng() & 0x400001;
            }
            A.w.mpObject = rng() % 6 ? reinterpret_cast<FEObject *>(A.om[0]) : nullptr;
            A.w.base.mpLabel = rng() % 3 ? reinterpret_cast<FEObject *>(A.om[1]) : nullptr;
            std::memcpy(&B, &A, sizeof A);
            const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
            auto rbp = [&](void *q) { return q ? reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(q) + delta) : nullptr; };
            B.w.mpObject = static_cast<FEObject *>(rbp(A.w.mpObject));
            B.w.base.mpLabel = static_cast<FEObject *>(rbp(A.w.base.mpLabel));
            for (int k = 0; k < 2; ++k) reinterpret_cast<FEObject *>(B.om[k])->mpData = B.d[k];
            const int op = static_cast<int>(rng() % 3);
            if (op == 0) { oShow(&A.w); B.w.Show(); }
            else if (op == 1) { oHide(&A.w); B.w.Hide(); }
            else { oPos(&A.w); B.w.Position(); }
            bool same = std::memcmp(A.d, B.d, sizeof A.d) == 0;
            for (int k = 0; k < 2; ++k) same &= reinterpret_cast<FEObject *>(A.om[k])->mu32Flags == reinterpret_cast<FEObject *>(B.om[k])->mu32Flags;
            EXPECT(same, "FEButtonWidget op %d pc=%x", op, pc);
        }
        // constructor
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A.w);
            for (size_t i = 0; i < sizeof A.w; ++i) pa[i] = static_cast<uint8_t>(rng());
            std::memcpy(&B.w, &A.w, sizeof A.w);
            const float pos[2] = {RefCoord(), RefCoord()}, size[2] = {RefCoord(), RefCoord()};
            const uint32_t en = rng() % 2;
            FEButtonWidget *ra = oCtor(&A.w, pos, size, en), *rb = B.w.Construct(pos, size, en != 0);
            EXPECT(ra == &A.w && rb == &B.w && std::memcmp(&A.w, &B.w, sizeof A.w) == 0, "FEButtonWidget ctor");
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FEngGet2DExtents / FEngTestForIntersection on random groups of non-string objects (strings need fonts).
static void CheckFEngExtents(int iters)
{
    auto oExt = ORIG<uint32_t(__cdecl *)(const FEObject *, FERect *)>(0x51D9F0);
    auto oHit = ORIG<uint32_t(__cdecl *)(float, float, const FEObject *)>(0x52D020);
    static const uint32_t types[] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    struct Arena { unsigned char om[6][sizeof(FEGroup)]; float d[6][20]; };
    static Arena A;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            std::memset(&A, 0, sizeof A);
            FEObject *o[6];
            for (int i = 0; i < 6; ++i) {
                o[i] = reinterpret_cast<FEObject *>(A.om[i]);
                o[i]->mpData = A.d[i];
                for (float &f : A.d[i]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                o[i]->mu32Type = types[rng() % 12];
            }
            o[0]->mu32Type = 5;   // group: children 1..4 (child 1 may be a group with child 5)
            auto *g = reinterpret_cast<FEGroup *>(o[0]);
            FEObject *prev = nullptr;
            for (int k = 1; k <= 4; ++k) {
                if (rng() % 4 == 0) continue;
                if (prev) prev->node.next = &o[k]->node; else g->mChildren.head = &o[k]->node;
                prev = o[k];
            }
            if (o[1]->mu32Type == 5) reinterpret_cast<FEGroup *>(o[1])->mChildren.head = rng() % 2 ? &o[5]->node : nullptr;
            if (o[5]->mu32Type == 5) o[5]->mu32Type = 1;
            const int which = rng() % 3 ? 0 : static_cast<int>(rng() % 6);
            FERect ra{1, 2, 3, 4}, rb{1, 2, 3, 4};
            const uint32_t a = oExt(o[which], &ra) & 0xFF;
            const bool b = FEngGet2DExtents(o[which], &rb);
            EXPECT(a == b && std::memcmp(&ra, &rb, sizeof ra) == 0, "FEngGet2DExtents type %u pc=%x", o[which]->mu32Type, pc);
            const float x = a && rng() % 2 ? ra.left + (ra.right - ra.left) * static_cast<float>(rng() % 3) / 2 : RefCoord();
            const float y = a && rng() % 2 ? ra.top + (ra.bottom - ra.top) * static_cast<float>(rng() % 3) / 2 : RefCoord();
            EXPECT((oHit(x, y, o[which]) & 0xFF) == FEngTestForIntersection(x, y, o[which]), "FEngTestForIntersection pc=%x", pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FEJoyPad::Update + HeldFor / FEMouse::Update on random state.
static void CheckPadMouse(int iters)
{
    auto oPad = ORIG<void(THISCALL *)(FEJoyPad *, uint32_t, uint32_t)>(0x504FA0);
    auto oMouse = ORIG<void(THISCALL *)(FEMouse *, const FEMouse::Info *, uint32_t)>(0x50B0C0);
    for (int it = 0; it < iters; ++it) {
        if (rng() % 2) {
            FEJoyPad a, b;
            uint8_t *pa = reinterpret_cast<uint8_t *>(&a);
            for (size_t i = 0; i < sizeof a; ++i) pa[i] = static_cast<uint8_t>(rng());
            b = a;
            const uint32_t bits = rng() & rng(), d = rng() % 100;
            oPad(&a, bits, d);
            b.Update(bits, d);
            EXPECT(std::memcmp(&a, &b, sizeof a) == 0, "FEJoyPad::Update");
            const uint32_t m = rng() % 4 ? rng() & rng() & rng() : rng() % 2 ? 0u : 0xFFFFFFFFu;
            const uint32_t ha = ORIG<uint32_t(THISCALL *)(const FEJoyPad *, uint32_t)>(0x504FF0)(&a, m), hb = b.HeldFor(m);
            EXPECT(ha == hb, "FEJoyPad::HeldFor %x: %x vs %x", m, ha, hb);
            FEJoyPad ca, cb;
            std::memset(&ca, 0x5A, sizeof ca);
            std::memset(&cb, 0x5A, sizeof cb);
            FEJoyPad *ra = ORIG<FEJoyPad *(THISCALL *)(FEJoyPad *)>(0x519AE0)(&ca), *rb = cb.Construct();
            EXPECT(ra == &ca && rb == &cb && std::memcmp(&ca, &cb, sizeof ca) == 0, "FEJoyPad::FEJoyPad");
        } else {
            FEMouse a, b;
            uint8_t *pa = reinterpret_cast<uint8_t *>(&a);
            for (size_t i = 0; i < sizeof a; ++i) pa[i] = static_cast<uint8_t>(rng());
            if (rng() % 2) { a.mi32X = static_cast<int16_t>(a.mi32X); a.mi32Y = static_cast<int16_t>(a.mi32Y); }
            b = a;
            FEMouse::Info info{static_cast<int16_t>(rng() % 3 ? rng() : a.mi32X), static_cast<int16_t>(rng() % 3 ? rng() : a.mi32Y),
                               static_cast<int16_t>(rng()), static_cast<uint16_t>(rng())};
            const uint32_t d = rng() % 100;
            oMouse(&a, &info, d);
            b.Update(info, d);
            EXPECT(std::memcmp(&a, &b, sizeof a) == 0, "FEMouse::Update");
        }
    }
}

// FEngine::IsInputMessage: every table hash, neighbours, random values.
static void CheckIsInputMessage(int iters)
{
    auto o = ORIG<bool(__cdecl *)(uint32_t)>(0x50B2A0);
    std::vector<uint32_t> v = {0, 0x0C407210u, 0x936A6A7Fu};
    for (uint32_t h : kFEPadPressedHash) { v.push_back(h); v.push_back(h + 1); }
    for (uint32_t h : kFEPadHeldHash) v.push_back(h);
    for (uint32_t h : kFEPadReleasedHash) { v.push_back(h); v.push_back(h - 1); }
    for (int i = 0; i < iters; ++i) v.push_back(rng());
    for (uint32_t m : v) EXPECT(o(m) == FEngine::IsInputMessage(m), "IsInputMessage %08x", m);
}

// FEPackageManager lookups on a synthetic list (one arena, original and rewrite on the same memory).
static void CheckPackageManager(int iters)
{
    auto oFind = ORIG<FEPackageData *(THISCALL *)(FEPackageManager *, const char *)>(0x51BB70);
    auto oFindC = ORIG<FEPackageData *(THISCALL *)(FEPackageManager *, const void *)>(0x51BB30);
    auto oGet = ORIG<uint8_t *(THISCALL *)(FEPackageManager *, const char *)>(0x51BC40);
    auto oSum = ORIG<uint32_t(THISCALL *)(FEPackageManager *)>(0x51BAD0);
    auto oHash = ORIG<uint32_t(THISCALL *)(const FEPackageData *)>(0x50B920);
    static const char *kNames[] = {"MAIN.FNG", "Opts.fng", "a", "LZ_ONE", "garage"};
    struct Arena { FEPackageManager m; FEPackageData d[5]; uint32_t chunk[5][6]; uint8_t hdr[5][0x40]; FEPackage pk[2]; };
    alignas(8) static Arena ar, snap;
    for (int it = 0; it < iters / 8; ++it) {
        std::memset(&ar, 0, sizeof ar);
        ar.m.mList.next = ar.m.mList.prev = &ar.m.mList;
        ar.pk[0].mpcName = "PKG0"; ar.pk[1].mpcName = "pkg1";
        int n = rng() % 6;
        for (int i = 0; i < n; ++i) {
            FEPackageData &d = ar.d[i];
            int nm = rng() % 5;
            if (rng() % 4 == 0) { ar.chunk[i][0] = 0x30210; ar.chunk[i][2] = FEHashUpper(kNames[nm]); }
            else {
                ar.chunk[i][0] = rng() % 8 ? 0x30203 : 0x1234;
                auto *h = reinterpret_cast<uint32_t *>(ar.hdr[i]);
                h[0] = rng() % 8 ? 0xE76E4546u : 1; h[2] = 0x64486B50u; h[4] = rng() % 8 ? 0x20000u + rng() % 3 : 0x1FFFF;
                std::strcpy(reinterpret_cast<char *>(ar.hdr[i] + 0x28), kNames[nm]);
                d.mpData = ar.hdr[i];
            }
            d.mpChunk = ar.chunk[i];
            d.mpPackage = rng() % 2 ? &ar.pk[rng() % 2] : nullptr;
            d.node.prev = ar.m.mList.prev; d.node.next = &ar.m.mList; ar.m.mList.prev->next = &d.node; ar.m.mList.prev = &d.node;
        }
        for (int q = 0; q < 6; ++q) {
            char name[32];
            std::snprintf(name, sizeof name, "%s%s", rng() % 2 ? "x\\" : "", kNames[rng() % 5]);
            if (rng() % 3 == 0) for (char *c = name; *c; ++c) *c = static_cast<char>(*c ^ ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ? 0x20 : 0));
            int op = rng() % 4;
            const void *qc = ar.chunk[rng() % 5];
            std::memcpy(&snap, &ar, sizeof ar);
            uintptr_t r1 = 0, r2 = 0;
            // LZ chunks would allocate in GetPackageData: only query names whose data is raw (or absent)
            switch (op) {
            case 0: r1 = reinterpret_cast<uintptr_t>(oFind(&ar.m, name)); break;
            case 1: r1 = reinterpret_cast<uintptr_t>(oFindC(&ar.m, qc)); break;
            case 2: { FEPackageData *d = ar.m.FindFEPackageData(name); std::memcpy(&ar, &snap, sizeof ar);
                      if (d && d->mpChunk[0] == 0x30210) { op = 0; r1 = reinterpret_cast<uintptr_t>(oFind(&ar.m, name)); }
                      else r1 = reinterpret_cast<uintptr_t>(oGet(&ar.m, name)); break; }
            default: r1 = oSum(&ar.m); for (int i = 0; i < n; ++i) r1 = r1 * 31 + oHash(&ar.d[i]); break;
            }
            Arena after;
            std::memcpy(&after, &ar, sizeof ar);
            std::memcpy(&ar, &snap, sizeof ar);
            switch (op) {
            case 0: r2 = reinterpret_cast<uintptr_t>(ar.m.FindFEPackageData(name)); break;
            case 1: r2 = reinterpret_cast<uintptr_t>(ar.m.FindFEPackageData(qc)); break;
            case 2: r2 = reinterpret_cast<uintptr_t>(ar.m.GetPackageData(name)); break;
            default: r2 = ar.m.GetActiveScreensChecksum(); for (int i = 0; i < n; ++i) r2 = r2 * 31 + ar.d[i].GetNameHash(); break;
            }
            EXPECT(r1 == r2 && std::memcmp(&after, &ar, sizeof ar) == 0, "FEPackageManager op %d '%s' n=%d", op, name, n);
        }
    }
}

static void CheckFENodeFind(int iters)
{
    auto oCmp = ORIG<int32_t(__cdecl *)(const char *, const char *)>(0x505310);
    auto oFind = ORIG<FENode *(THISCALL *)(void *, const char *, FENode *)>(0x519E50);
    static const char *kNames[] = {"Main.fng", "MAIN.FNG", "shop", "Shop", "x", ""};
    FENode nodes[6];
    for (int it = 0; it < iters / 4; ++it) {
        std::memset(nodes, 0, sizeof nodes);
        int n = 1 + rng() % 6;
        for (int i = 0; i < n; ++i) {
            if (rng() % 5) { nodes[i].mpcName = const_cast<char *>(kNames[rng() % 6]); nodes[i].mu32NameHash = FEHashUpper(nodes[i].mpcName); }
            if (rng() % 7 == 0) nodes[i].mu32NameHash ^= 1;   // stale hash
            nodes[i].node.next = i + 1 < n ? &nodes[i + 1].node : nullptr;
        }
        const char *q = rng() % 8 ? kNames[rng() % 6] : nullptr;
        if (!q)   // null query vs a node named "" (both hash 0xFFFFFFFF) compares against null: crashes in the original too
            for (int i = 0; i < n; ++i) if (nodes[i].mpcName && nodes[i].mu32NameHash == 0xFFFFFFFFu) { nodes[i].mpcName = const_cast<char *>("x"); nodes[i].mu32NameHash = FEHashUpper("x"); }
        EXPECT(oFind(nullptr, q, &nodes[0]) == FEListFindNode(q, &nodes[0]), "FEList::FindNode");
        std::string a = StrSample(), b2 = rng() % 2 ? a : StrSample();
        EXPECT(oCmp(a.c_str(), b2.c_str()) == FEStricmp(a.c_str(), b2.c_str()), "FEStricmp(%s,%s)", a.c_str(), b2.c_str());
    }
}

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

// FEPackage::UpdateObject (PC 54CE60): script clock + end handling (stop/loop/ping-pong/chain) + tracks + movie
// time, on synthetic worlds without events or list/group types (those call still-original engine code).
static void CheckUpdateObject(int iters)
{
    auto orig = ORIG<void(THISCALL *)(void *, FEObject *, int32_t)>(0x54CE60);
    struct World { UotWorld u; FEScript next; int32_t movie; uint8_t pkg[0x40]; };
    static World a, b;
    auto build = [](World &w, uint32_t seed) {
        BuildUot(w.u, seed);
        std::mt19937 r(seed ^ 0x9E3779B9u);
        std::memset(&w.next, 0, sizeof w.next);
        std::memset(w.pkg, 0, sizeof w.pkg);
        FEScript &sc = w.u.sc;
        sc.mu32NumTracks &= 0xFF;                           // SetupMoveToTracks uses the full count
        sc.mi32Length = int32_t(r() % 120) - 5;
        sc.mi32Time = int32_t(r() % 150);
        sc.mu32Flags = r() % 4;
        sc.mEvents.mu32Count = 0;
        w.next = sc;                                        // chained target shares the tracks
        w.next.mi32Time = 7;
        w.next.mpChainedScript = nullptr;
        sc.mpChainedScript = r() % 4 == 0 ? &w.next : nullptr;
        w.u.obj.mpCurrentScript = &sc;
        w.u.obj.mu32Type = r() % 2 ? 7 : r() % 4;
        w.u.obj.mu32Flags |= r() & 0x400000;
        w.pkg[0x14] = uint8_t(r() % 3);
        int32_t d = int32_t(r() % 70) - 20;
        std::memcpy(w.pkg + 0x34, &d, 4);
        w.movie = 3;
    };
    for (int it = 0; it < iters / 10; ++it) {
        uint32_t seed = rng();
        build(a, seed);
        build(b, seed);
        int32_t delta = int32_t(rng() % 40);
        if (std::getenv("REFEREE_TRACE")) { std::printf("UO it %d seed %u mode %u len %d t %d chain %d\n", it, seed, a.u.sc.mu32Flags, a.u.sc.mi32Length, a.u.sc.mi32Time, a.u.sc.mpChainedScript != nullptr); std::fflush(stdout); }
        orig(a.pkg, &a.u.obj, delta);
        if (std::getenv("REFEREE_TRACE")) { std::printf(" orig ok\n"); std::fflush(stdout); }
        reinterpret_cast<FEPackage *>(b.pkg)->UpdateObject(&b.u.obj, delta);
        bool cur = (a.u.obj.mpCurrentScript == &a.next) == (b.u.obj.mpCurrentScript == &b.next);
        bool same = cur && a.u.sc.mi32Time == b.u.sc.mi32Time && a.next.mi32Time == b.next.mi32Time &&
                    a.u.obj.mu32Flags == b.u.obj.mu32Flags && std::memcmp(a.u.data, b.u.data, sizeof a.u.data) == 0 &&
                    a.movie == b.movie;
        for (int i = 0; i < 5; ++i) same = same && a.u.tracks[i].mu8Flags == b.u.tracks[i].mu8Flags;
        EXPECT(same, "UpdateObject mode %u fast %d len %d t %d", a.u.sc.mu32Flags, a.pkg[0x14], a.u.sc.mi32Length, a.u.sc.mi32Time);
    }
}

// FEPackage::Update + UpdateGroup (PC 54D190/504910) on synthetic packages (top-level objects, groups with
// children, scripts with random end modes, no events/tracks).
static void CheckPackageUpdate(int iters)
{
    auto orig = ORIG<void(THISCALL *)(void *, void *, int32_t)>(0x54D190);
    struct Pkg {
        uint8_t pkg[0x60];
        alignas(8) uint8_t objs[6][sizeof(FEGroup)];
        FEScript sc[6];
        uint32_t data[6][40];
    };
    static Pkg a, b;
    auto build = [](Pkg &p, uint32_t seed) {
        std::mt19937 r(seed);
        std::memset(&p, 0, sizeof p);
        auto *list = reinterpret_cast<FEMinList *>(p.pkg + 0x44);
        FEGroup *lastGroup = nullptr;
        int n = 1 + r() % 6;
        for (int i = 0; i < n; ++i) {
            auto *g = reinterpret_cast<FEGroup *>(p.objs[i]);
            g->obj.mpCurrentScript = &p.sc[i];
            g->obj.mpData = p.data[i];
            g->obj.mu32Type = r() % 3 == 0 ? 5 : (r() % 2 ? 7 : 1);
            g->obj.mu32Flags = r() & 0x3C00000u;
            p.sc[i].mi32Length = int32_t(r() % 60);
            p.sc[i].mi32Time = int32_t(r() % 70);
            p.sc[i].mu32Flags = r() % 4;
            if (lastGroup && r() % 2) lastGroup->mChildren.AddNode(lastGroup->mChildren.tail, &g->obj.node);
            else list->AddNode(list->tail, &g->obj.node);
            if (g->obj.mu32Type == 5) lastGroup = g;
        }
        p.pkg[0x14] = uint8_t(r() % 2);
    };
    for (int it = 0; it < iters / 20; ++it) {
        uint32_t seed = rng();
        build(a, seed);
        build(b, seed);
        int32_t delta = int32_t(rng() % 50);
        orig(a.pkg, nullptr, delta);
        reinterpret_cast<FEPackage *>(b.pkg)->Update(nullptr, delta);
        bool same = true;
        for (int i = 0; i < 6; ++i) {
            auto *oa = reinterpret_cast<FEObject *>(a.objs[i]);
            auto *ob = reinterpret_cast<FEObject *>(b.objs[i]);
            same = same && a.sc[i].mi32Time == b.sc[i].mi32Time && oa->mu32Flags == ob->mu32Flags &&
                   std::memcmp(a.objs[i] + sizeof(FEObject), b.objs[i] + sizeof(FEObject), 4) == 0;
        }
        EXPECT(same, "FEPackage::Update seed %u", seed);
    }
}

// FindObjectByGUID/Hash (PC 517FE0/517F80) and ForAllObjects (504880, with the exe's FEFindByGUID callback)
// on random object trees with duplicate GUIDs/hashes.
static void CheckFindObject(int iters)
{
    auto oGuid = ORIG<FEObject *(THISCALL *)(void *, uint32_t)>(0x517FE0);
    auto oHash = ORIG<FEObject *(THISCALL *)(void *, uint32_t)>(0x517F80);
    auto oAll = ORIG<bool(THISCALL *)(void *, void *)>(0x504880);
    struct Tree { uint8_t pkg[0x60]; alignas(8) uint8_t objs[8][sizeof(FEGroup)]; };
    static Tree t;
    for (int it = 0; it < iters / 10; ++it) {
        std::memset(&t, 0, sizeof t);
        auto *list = reinterpret_cast<FEMinList *>(t.pkg + 0x44);
        FEGroup *groups[8];
        int ng = 0, n = rng() % 9;
        for (int i = 0; i < n; ++i) {
            auto *g = reinterpret_cast<FEGroup *>(t.objs[i]);
            g->obj.mu32GUID = rng() % 6;
            g->obj.mu32NameHash = rng() % 6;
            g->obj.mu32Type = rng() % 3 == 0 ? 5 : 1;
            FEMinList *parent = ng && rng() % 2 ? &groups[rng() % ng]->mChildren : list;
            parent->AddNode(parent->tail, &g->obj.node);
            if (g->obj.mu32Type == 5) groups[ng++] = g;
        }
        auto *pkg = reinterpret_cast<FEPackage *>(t.pkg);
        uint32_t key = rng() % 7;
        EXPECT(oGuid(t.pkg, key) == pkg->FindObjectByGUID(key), "FindObjectByGUID %u", key);
        EXPECT(oHash(t.pkg, key) == pkg->FindObjectByHash(key), "FindObjectByHash %u", key);
        struct { uint32_t vt, guid; void *result; } ca{0x79AAFC, key, nullptr}, cb{0x79AAFC, key, nullptr};
        bool ra = oAll(t.pkg, &ca), rb = pkg->ForAllObjects(reinterpret_cast<FEObjectCallback *>(&cb));
        EXPECT(ra == rb && ca.result == cb.result, "ForAllObjects %u", key);
    }
}

// FEPackage small members: SetControlMask (5047E0), FindLibraryReference (504960), SetCurrentButton without
// notification (52B3A0; with notification it queues engine messages).
static void CheckPackageSmall(int iters)
{
    auto oMask = ORIG<void(THISCALL *)(void *, uint32_t)>(0x5047E0);
    auto oLib = ORIG<void *(THISCALL *)(const void *, uint32_t)>(0x504960);
    auto oBtn = ORIG<void(THISCALL *)(void *, void *, void *, int)>(0x52B3A0);
    static uint8_t a[0x100], b[0x100];
    static FELibraryRef refs[8];
    for (int it = 0; it < iters / 10; ++it) {
        for (size_t i = 0; i < sizeof a; ++i) a[i] = b[i] = uint8_t(rng());
        uint32_t m = rng();
        oMask(a, m); reinterpret_cast<FEPackage *>(b)->SetControlMask(m);
        void *btn = reinterpret_cast<void *>(uintptr_t(rng()));
        oBtn(a, nullptr, btn, 0); reinterpret_cast<FEPackage *>(b)->SetCurrentButton(nullptr, static_cast<FEObject *>(btn), false);
        EXPECT(std::memcmp(a, b, sizeof a) == 0, "SetControlMask/SetCurrentButton");
        uint32_t n = rng() % 9;
        for (uint32_t i = 0; i < 8; ++i) refs[i].mu32Hash = rng() % 6;
        FELibraryRef *p = refs;
        std::memcpy(a + 0x84, &n, 4); std::memcpy(a + 0x88, &p, 4);
        uint32_t key = rng() % 7;
        EXPECT(oLib(a, key) == reinterpret_cast<FEPackage *>(a)->FindLibraryReference(key), "FindLibraryReference");
    }
}

// FEMessageResponse::FindConditionBranchTarget (PC 504490) on random opcode lists with nested conditions.
static void CheckConditionBranch(int iters)
{
    auto orig = ORIG<uint32_t(THISCALL *)(const FEMessageResponse *, uint32_t)>(0x504490);
    static const uint32_t kOps[] = {0x300, 0x301, 0x500, 0x501, 0x200, 0x0, 0x1};
    for (int it = 0; it < iters / 4; ++it) {
        FEResponse code[16]{};
        FEMessageResponse r{};
        r.mu32Count = 1 + rng() % 16;
        r.mpResponses = code;
        for (uint32_t i = 0; i < 16; ++i) code[i].mu32Type = kOps[rng() % 7];
        uint32_t start = rng() % r.mu32Count;
        // keep the walk inside the array: the original reads up to index count (one past the end) at worst
        if (r.mu32Count < 16) code[r.mu32Count].mu32Type = 0x501;
        EXPECT(orig(&r, start) == r.FindConditionBranchTarget(start), "FindConditionBranchTarget n=%u i=%u", r.mu32Count, start);
    }
}

// FEngine package markers + last-button ring: same random operation sequence on two engine images.
static void CheckPackageMarkers(int iters)
{
    auto oRecord = ORIG<bool(THISCALL *)(FEngine *, const char *)>(0x51B070);
    auto oClear = ORIG<void(THISCALL *)(FEngine *)>(0x50B390);
    auto oRecBtn = ORIG<void(THISCALL *)(FEngine *, uint32_t, uint32_t)>(0x50B300);
    auto oRecall = ORIG<uint32_t(THISCALL *)(FEngine *, uint32_t)>(0x50B360);
    static FEngine a, b;
    std::memset(&a, 0, sizeof a);
    for (auto &m : a.macPackageMarkers) std::memset(m, 'q', sizeof m);
    std::memcpy(&b, &a, sizeof a);
    for (int i = 0; i < iters; ++i) {
        uint32_t h = rng() % 40, g = rng();
        switch (rng() % 8) {
        case 0: case 1: {
            std::string n = RandomText(200, true);
            bool x = oRecord(&a, n.c_str()), y = b.RecordPackageMarker(n.c_str());
            EXPECT(x == y, "RecordPackageMarker ret %d vs %d", x, y);
            break;
        }
        case 2: if (rng() % 8 == 0) { oClear(&a); b.ClearPackageMarkers(); } break;
        case 3: case 4: case 5: oRecBtn(&a, h, g); b.RecordLastPackageButton(h, g); break;
        default: {
            uint32_t x = oRecall(&a, h), y = b.RecallLastPackageButton(h);
            EXPECT(x == y, "RecallLastPackageButton(%u) %08X vs %08X", h, x, y);
        }
        }
        if (i % 64 == 0) EXPECT(std::memcmp(&a, &b, sizeof a) == 0, "package marker state differs after op %d", i);
    }
    EXPECT(std::memcmp(&a, &b, sizeof a) == 0, "package marker final state differs");
}

// PackagePopQueuedPushPackage on mirrored command lists (synthetic nodes) + two packages.
static void CheckPopQueuedPush(int iters)
{
    auto orig = ORIG<bool(THISCALL *)(FEngine *, const char *)>(0x51AE80);
    static FEngine e[2];
    static FEPackageCommand cmd[2][8];
    static FEPackage pkg[2][2];
    static const char *kNames[] = {"MAIN", "main", "Opts", "X", ""};
    for (int it = 0; it < iters / 4; ++it) {
        int n = rng() % 9;
        uint32_t types[8], masks[2][4];
        int names[8];
        for (int i = 0; i < n; ++i) { types[i] = rng() % 0x14; names[i] = rng() % 5; }
        for (auto &m : masks) for (auto &v : m) v = rng();
        for (int k = 0; k < 2; ++k) {
            e[k].mPackageCommands = FEMinList{nullptr, 0, nullptr, nullptr};
            e[k].mpPackages = &pkg[k][0];
            for (int j = 0; j < 2; ++j) {
                auto *pn = reinterpret_cast<FEMinNode *>(&pkg[k][j]);
                pn->next = j == 0 ? reinterpret_cast<FEMinNode *>(&pkg[k][1]) : nullptr;
                pkg[k][j].mu32ControlMask = masks[j][0]; pkg[k][j].mu32SavedControlMask = masks[j][1];
                pkg[k][j].mu32InputControlMask = masks[j][2]; pkg[k][j].mu32SavedInputControlMask = masks[j][3];
            }
            for (int i = 0; i < n; ++i) {
                cmd[k][i] = FEPackageCommand{};
                cmd[k][i].mu32Type = types[i];
                cmd[k][i].node.mu32NameHash = FEHashUpper(kNames[names[i]]);
                e[k].mPackageCommands.AddTail(&cmd[k][i].node.node);
            }
        }
        int q = rng() % 6;
        const char *name = q == 5 ? nullptr : kNames[q];
        bool x = orig(&e[0], name), y = e[1].PackagePopQueuedPushPackage(name);
        auto idx = [&](int k, FEMinNode *p) -> int {
            if (!p) return -1;
            if (p == reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED)) return -2;
            return static_cast<int>(reinterpret_cast<FEPackageCommand *>(p) - cmd[k]);
        };
        bool same = x == y && e[0].mPackageCommands.count == e[1].mPackageCommands.count &&
                    idx(0, e[0].mPackageCommands.head) == idx(1, e[1].mPackageCommands.head) &&
                    idx(0, e[0].mPackageCommands.tail) == idx(1, e[1].mPackageCommands.tail);
        for (int i = 0; i < n; ++i)
            same = same && idx(0, cmd[0][i].node.node.next) == idx(1, cmd[1][i].node.node.next) &&
                   idx(0, cmd[0][i].node.node.prev) == idx(1, cmd[1][i].node.node.prev);
        for (int j = 0; j < 2; ++j)
            same = same && std::memcmp(&pkg[0][j].mu32ControlMask, &pkg[1][j].mu32ControlMask, 16) == 0;
        EXPECT(same, "PackagePopQueuedPushPackage n=%d name=%s: %d vs %d", n, name ? name : "(null)", x, y);
    }
}

// REFEREE_ONLY=a,b: run only the checks whose call text contains one of the comma-separated words.
static bool RefereeSelected(const char *call)
{
    const char *only = std::getenv("REFEREE_ONLY");
    if (!only || !*only) return true;
    std::string list(only);
    size_t pos = 0;
    while (pos <= list.size()) {
        size_t end = list.find(',', pos);
        if (end == std::string::npos) end = list.size();
        std::string word = list.substr(pos, end - pos);
        if (!word.empty() && std::strstr(call, word.c_str())) return true;
        pos = end + 1;
    }
    return false;
}
#define REFEREE_RUN(...) do { if (RefereeSelected(#__VA_ARGS__)) { __VA_ARGS__; } } while (0)

static int RunAll()
{
    auto *base = reinterpret_cast<uint8_t *>(GetModuleHandleA(nullptr));
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS32 *>(base + reinterpret_cast<IMAGE_DOS_HEADER *>(base)->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != 0x214D4C48) { std::printf("referee: not SPEED2.EXE v1.2\n"); return 2; }
    const char *env = std::getenv("REFEREE_ITERATIONS");
    int iters = env ? std::atoi(env) : 2000;
    // sanity: prove we really execute the ORIGINAL machine code (known answer: POSIX cksum of "123456789")
    uint32_t known = ORIG<uint32_t(__cdecl *)(const void *, int, uint32_t)>(0x43BCD0)("123456789", 9, 0);
    if (known != 0x377A6011u) { std::printf("referee: original code not callable (got %08X)\n", known); return 2; }
    REFEREE_RUN(CheckCrc(iters));
    REFEREE_RUN(CheckStringHash(iters));
    REFEREE_RUN(CheckStrFuncs(iters));
    REFEREE_RUN(CheckStrToNumber(iters));
    REFEREE_RUN(CheckLevelSortingList(iters));
    REFEREE_RUN(CheckListBoxScroll(iters));
    REFEREE_RUN(CheckListBoxUpdate(iters));
    REFEREE_RUN(CheckWideCopies(iters));
    REFEREE_RUN(CheckMinList(iters));
    REFEREE_RUN(CheckFEObject(iters));
    REFEREE_RUN(CheckSlotPool(iters));
    REFEREE_RUN(CheckRefList(iters));
    REFEREE_RUN(CheckKeyTrackCtor(iters));
    REFEREE_RUN(CheckFEMath(iters));
    REFEREE_RUN(CheckSetTrackValue(iters));
    REFEREE_RUN(CheckSetters(iters));
    REFEREE_RUN(CheckKeyInterp(iters));
    REFEREE_RUN(CheckInterpLinear(iters));
    REFEREE_RUN(CheckTrigAndSlerp(iters));
    REFEREE_RUN(CheckSinCos());
    REFEREE_RUN(CheckATan(iters));
    REFEREE_RUN(CheckRandomMod(iters));
    REFEREE_RUN(CheckMatrices(iters));
    REFEREE_RUN(CheckVectors(iters));
    REFEREE_RUN(CheckBoxes(iters));
    REFEREE_RUN(CheckSlotPoolB(iters));
    REFEREE_RUN(CheckNoise(iters));
    REFEREE_RUN(CheckPackageDataCtor(iters / 10 + 1));
    REFEREE_RUN(CheckPackageCtor(iters / 40 + 1));
    REFEREE_RUN(CheckReaderTypeSizes(iters));
    REFEREE_RUN(CheckReaderStringTags(iters / 4 + 1));
    REFEREE_RUN(CheckListBoxCells(iters));
    REFEREE_RUN(CheckCodeListBox(iters * 5));
    REFEREE_RUN(CheckCodeListBoxFillUpdate(iters * 5));
    REFEREE_RUN(CheckCodeListBoxMove(iters * 10));
    REFEREE_RUN(CheckPadMouse(iters * 5));
    REFEREE_RUN(CheckButtonMap(iters * 5));
    REFEREE_RUN(CheckRender(iters * 3));
    REFEREE_RUN(CheckTypeLib(iters * 5));
    REFEREE_RUN(CheckHandleJoy(iters));
    REFEREE_RUN(CheckFEngUtil(iters * 5));
    REFEREE_RUN(CheckFEngRotationZ(iters));
    REFEREE_RUN(CheckFEngPosition(iters * 3));
    REFEREE_RUN(CheckButtonWidget(iters));
    REFEREE_RUN(CheckFEngExtents(iters * 3));
    REFEREE_RUN(CheckIsInputMessage(iters));
    REFEREE_RUN(CheckPackageManager(iters));
    REFEREE_RUN(CheckFENodeFind(iters));
    REFEREE_RUN(CheckUpdateObjectTracks(iters));
    REFEREE_RUN(CheckUpdateObject(iters));
    REFEREE_RUN(CheckPackageUpdate(iters));
    REFEREE_RUN(CheckFindObject(iters));
    REFEREE_RUN(CheckPackageSmall(iters));
    REFEREE_RUN(CheckConditionBranch(iters));
    REFEREE_RUN(CheckPackageMarkers(iters));
    REFEREE_RUN(CheckPopQueuedPush(iters));
    REFEREE_RUN(CheckBlockCtor<FEMessageResponseBlock>(0x53E710, FEMessageResponseBlock_Construct, "FEMessageResponse block", {{&teststub::feMsgRespBlockVTable, 0x79BF50}, {&teststub::feMinListVTable, 0x79A9EC}, {&teststub::feMsgRespVTable, 0x79AA38}}));
    REFEREE_RUN(CheckBlockCtor<FEKeyNodeBlock>(0x5319C0, FEKeyNodeBlock_Construct, "FEKeyNode block", {{&teststub::feKeyNodeBlockVTable, 0x79B9EC}, {&teststub::feMinListVTable, 0x79A9EC}, {&teststub::feKeyNodeVTable, 0x79AA34}}));
    REFEREE_RUN(CheckBlockCtor<FEScriptBlock>(0x53E910, FEScriptBlock_Construct, "FEScript block", {{&teststub::feScriptBlockVTable, 0x79BF54}, {&teststub::feMinListVTable, 0x79A9EC}, {&teststub::feScriptVTable, 0x79AD28}}));
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
