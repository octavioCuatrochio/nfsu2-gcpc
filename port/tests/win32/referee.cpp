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
#include "frontend/FEngine.h"
#include "frontend/cFEng.h"
#include "frontend/FEListBox.h"
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
            if (mode == 3) { o1 = a; o2 = a; oTr(&o1, &o1); bTransposeMatrix(&o2, &o2); EXPECT(std::memcmp(&o1, &o2, sizeof o1) == 0, "transpose in place"); }
        }
    }
    _controlfp(saved, _MCW_PC);
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
    CheckStrFuncs(iters);
    CheckStrToNumber(iters);
    CheckLevelSortingList(iters);
    CheckListBoxScroll(iters);
    CheckListBoxUpdate(iters);
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
    CheckSinCos();
    CheckATan(iters);
    CheckRandomMod(iters);
    CheckMatrices(iters);
    CheckUpdateObjectTracks(iters);
    CheckUpdateObject(iters);
    CheckPackageUpdate(iters);
    CheckFindObject(iters);
    CheckPackageSmall(iters);
    CheckConditionBranch(iters);
    CheckPackageMarkers(iters);
    CheckPopQueuedPush(iters);
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
