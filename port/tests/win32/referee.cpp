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
#include <array>
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
#include "frontend/FEScrollBar.h"
#include "frontend/FEngFont.h"
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
#include "frontend/cFEngRender.h"
#include "frontend/FEListBox.h"
#include "frontend/FECodeListBox.h"
#include "bWare/bSlotPool.h"
#include "camera/CameraNoise.h"
#include "camera/Camera.h"
#include "frontend/FEPackageManager.h"
#include "frontend/FENode.h"
#include "frontend/FELocalize.h"
#include "frontend/MenuScreen.h"
#include "frontend/UIWidgetMenu.h"
#include "frontend/TuningUtil.h"
#include "frontend/FEString.h"
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
            if (std::memcmp(&a, &b, sizeof a) != 0) {
                const uint32_t *wa = reinterpret_cast<const uint32_t *>(&a), *wb = reinterpret_cast<const uint32_t *>(&b);
                for (size_t w = 0; w < sizeof a / 4; ++w)
                    if (wa[w] != wb[w] && failures < 20) std::printf("  +0x%02X: %08X / %08X\n", static_cast<unsigned>(w * 4), wa[w], wb[w]);
            }
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

// FEScrollBar (null-base ctor, Update, SetPos*, arrow / backing visibility) + FEngSetWidth/Height on synthetic objects.
static void CheckScrollBar(int iters)
{
    auto oCtor = ORIG<FEScrollBar *(THISCALL *)(FEScrollBar *, const char *, const char *, uint32_t, uint32_t, uint32_t)>(0x542AC0);
    using M3 = void(THISCALL *)(FEScrollBar *, int32_t, int32_t, int32_t);
    const M3 o3[3] = {ORIG<M3>(0x538080), ORIG<M3>(0x52E200), ORIG<M3>(0x52E320)};
    auto oArrow = ORIG<void(THISCALL *)(FEScrollBar *, int32_t, uint32_t)>(0x50D820);
    auto oBacking = ORIG<void(THISCALL *)(FEScrollBar *, uint32_t)>(0x50D880);
    auto oW = ORIG<void(__cdecl *)(FEObject *, float)>(0x495590);
    auto oH = ORIG<void(__cdecl *)(FEObject *, float)>(0x525DB0);
    struct Arena { FEScrollBar b; unsigned char om[6][sizeof(FEObject)]; float d[6][20]; };
    static Arena A, B;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            std::memcpy(&B, &A, sizeof A);
            const uint32_t v = (rng() & ~0xFFu) | (rng() & 1), r = (rng() & ~0xFFu) | (rng() & 1), a5 = (rng() & ~0xFFu) | (rng() & 1);   // bools: garbage high bytes
            oCtor(&A.b, "Pkg", nullptr, v, r, a5);
            B.b.Construct("Pkg", nullptr, v & 0xFF, r & 0xFF, a5 & 0xFF);
            {
                int off = -1;
                for (size_t i = 0; i < sizeof A.b && off < 0; ++i)
                    if (reinterpret_cast<uint8_t *>(&A.b)[i] != reinterpret_cast<uint8_t *>(&B.b)[i]) off = static_cast<int>(i);
                EXPECT(off < 0, "FEScrollBar ctor: byte %x %02x vs %02x", off, off < 0 ? 0 : reinterpret_cast<uint8_t *>(&A.b)[off], off < 0 ? 0 : reinterpret_cast<uint8_t *>(&B.b)[off]);
            }
            // random state + objects
            for (size_t i = 0; i < sizeof A.b; ++i) reinterpret_cast<uint8_t *>(&A.b)[i] = static_cast<uint8_t>(rng());
            A.b.mbVertical = rng() % 2; A.b.mbResized = rng() % 2; A.b.mbNoMove = rng() % 3 == 0;
            float *fl[] = {&A.b.mBackingX, &A.b.mBackingY, &A.b.mBackingW, &A.b.mBackingH, &A.b.mHandleW, &A.b.mHandleH};
            for (float *f : fl) *f = RefCoord() / static_cast<float>(1 + rng() % 5);
            FEObject **slots[] = {&A.b.mpBacking, &A.b.mpHandle, &A.b.mpArrow1, &A.b.mpArrow2, &A.b.mpBackingEnd1, &A.b.mpBackingEnd2};
            for (int k = 0; k < 6; ++k) {
                for (float &f : A.d[k]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                auto *o = reinterpret_cast<FEObject *>(A.om[k]);
                std::memset(A.om[k], 0, sizeof A.om[k]);
                o->mpData = A.d[k];
                o->mu32Type = rng() % 3 ? 1 : 3;
                o->mu32Flags = rng() & 0x400001;
                *slots[k] = rng() % 8 ? o : nullptr;
            }
            std::memcpy(&B, &A, sizeof A);
            const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
            FEObject **bslots[] = {&B.b.mpBacking, &B.b.mpHandle, &B.b.mpArrow1, &B.b.mpArrow2, &B.b.mpBackingEnd1, &B.b.mpBackingEnd2};
            for (int k = 0; k < 6; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) + delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = B.d[k];
            }
            const int32_t total = static_cast<int32_t>(rng() % 40) - 2, visible = static_cast<int32_t>(rng() % 30) - 2;
            const int32_t index = static_cast<int32_t>(rng() % 45) - 2;
            const int op = static_cast<int>(rng() % 7);
            if (op < 3) {
                o3[op](&A.b, visible, total, index);
                if (op == 0) B.b.Update(visible, total, index);
                else if (op == 1) B.b.SetPosResized(visible, total, index);
                else B.b.SetPosNonResized(visible, total, index);
            } else if (op == 3) {
                const int32_t arrow = static_cast<int32_t>(rng() % 4);
                const uint32_t vis = rng();
                oArrow(&A.b, arrow, vis);
                B.b.SetArrowVisibility(arrow, vis & 0xFF);
            } else if (op == 4) {
                const uint32_t vis = rng();
                oBacking(&A.b, vis);
                B.b.SetBackingVisibility(vis & 0xFF);
            } else {
                const float x = RefCoord();
                (op == 5 ? oW : oH)(A.b.mpHandle, x);
                (op == 5 ? FEngSetWidth : FEngSetHeight)(B.b.mpHandle, x);
            }
            // compare (B's pointers rebased)
            for (int k = 0; k < 6; ++k)
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) - delta);
            for (int k = 0; k < 6; ++k) reinterpret_cast<FEObject *>(B.om[k])->mpData = A.d[k];
            EXPECT(std::memcmp(&A, &B, offsetof(Arena, d)) == 0 && std::memcmp(A.d, B.d, sizeof A.d) == 0, "FEScrollBar op %d pc=%x v%d t%d i%d vert %d", op, pc, visible, total, index, A.b.mbVertical);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// cSlider on synthetic objects (no names, no value string): ctor, Init, Update (fake vtable logs Draw), Draw
// dispatch, DrawHorizontal / DrawVertical, Toggle, SetCurValue, Highlight / UnHighlight.
namespace cslider_fake {
std::vector<int> log;
void __thiscall Draw(void *) { log.push_back(0x18); }
void __thiscall DrawV(void *) { log.push_back(0x1C); }
void __thiscall DrawH(void *) { log.push_back(0x20); }
void *vtable[9] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, (void *)&Draw, (void *)&DrawV, (void *)&DrawH};
}
static void CheckCSlider(int iters)
{
    auto oCtor = ORIG<cSlider *(THISCALL *)(cSlider *)>(0x4FF430);
    auto oUpdate = ORIG<uint32_t(THISCALL *)(cSlider *, uint32_t)>(0x4FF480);
    auto oInit = ORIG<void(THISCALL *)(cSlider *, const SliderInitParams *, uint32_t)>(0x540C10);
    auto oToggle = ORIG<void(THISCALL *)(cSlider *, uint32_t)>(0x512240);
    auto oSet = ORIG<void(THISCALL *)(cSlider *, float)>(0x4FF550);
    using M0 = void(THISCALL *)(cSlider *);
    const M0 o0[5] = {ORIG<M0>(0x4FF530), ORIG<M0>(0x540D20), ORIG<M0>(0x540E90), ORIG<M0>(0x526D90), ORIG<M0>(0x526F80)};
    struct Arena { cSlider s; unsigned char om[6][sizeof(FEObject)]; float d[6][20]; };
    static Arena A, B;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            std::memcpy(&B, &A, sizeof A);
            EXPECT(oCtor(&A.s) == &A.s && B.s.Construct() == &B.s && std::memcmp(&A.s, &B.s, sizeof A.s) == 0, "cSlider ctor");
            FEObject **slots[] = {&A.s.mpBacking, &A.s.mpHandle, &A.s.mpValue, &A.s.mpCurrent, &A.s.mpEndCap, &A.s.mpEnd};
            for (int k = 0; k < 6; ++k) {
                for (float &f : A.d[k]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                auto *o = reinterpret_cast<FEObject *>(A.om[k]);
                std::memset(A.om[k], 0, sizeof A.om[k]);
                o->mpData = A.d[k];
                o->mu32Type = rng() % 3 ? 1 : 3;
                o->mu32Flags = rng() & 0x400001;
                *slots[k] = k != 2 && rng() % 6 ? o : nullptr;
            }
            A.s.vtable = cslider_fake::vtable;
            A.s.mbVertical = rng() % 2;
            float *fl[] = {&A.s.mMax, &A.s.mMin, &A.s.mPrev, &A.s.mValue, &A.s.mStep, &A.s.mLength, &A.s.mMargin};
            for (float *f : fl) *f = rng() % 4 ? static_cast<float>(static_cast<int>(rng() % 40) - 10) / static_cast<float>(1 + rng() % 4) : RefCoord();
            std::memcpy(&B, &A, sizeof A);
            const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
            FEObject **bslots[] = {&B.s.mpBacking, &B.s.mpHandle, &B.s.mpValue, &B.s.mpCurrent, &B.s.mpEndCap, &B.s.mpEnd};
            for (int k = 0; k < 6; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) + delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = B.d[k];
            }
            const int op = static_cast<int>(rng() % 10);
            if (op == 5 || op == 6) {   // null backing / handle: the original reads uninitialized locals
                A.s.mpBacking = reinterpret_cast<FEObject *>(A.om[0]); B.s.mpBacking = reinterpret_cast<FEObject *>(B.om[0]);
                A.s.mpHandle = reinterpret_cast<FEObject *>(A.om[1]); B.s.mpHandle = reinterpret_cast<FEObject *>(B.om[1]);
            }
            uint32_t ra = 0, rb = 0;
            std::vector<int> la, lb;
            SliderInitParams ip = {};
            ip.mMin = RefCoord(); ip.mMax = RefCoord(); ip.mValue = RefCoord(); ip.mStep = RefCoord(); ip.mMargin = RefCoord();
            const uint32_t msgs[3] = {0x9120409Eu, 0xB5971BF1u, rng()};
            const uint32_t msg = msgs[rng() % 3], flag = (rng() & ~0xFFu) | (rng() & 1);
            const float v = static_cast<float>(static_cast<int>(rng() % 40) - 10);
            cslider_fake::log.clear();
            switch (op) {
            case 0: ra = oUpdate(&A.s, msg) & 0xFF; break;
            case 1: oInit(&A.s, &ip, flag); break;
            case 2: oToggle(&A.s, flag); break;
            case 3: oSet(&A.s, v); break;
            default: o0[op - 4 < 5 ? op - 4 : 0](&A.s);
            }
            la = cslider_fake::log;
            cslider_fake::log.clear();
            switch (op) {
            case 0: rb = B.s.Update(msg); break;
            case 1: B.s.Init(&ip, flag & 0xFF); break;
            case 2: B.s.Toggle(flag & 0xFF); break;
            case 3: B.s.SetCurValue(v); break;
            case 4: case 9: B.s.Draw(); break;
            case 5: B.s.DrawHorizontal(); break;
            case 6: B.s.DrawVertical(); break;
            case 7: B.s.Highlight(); break;
            case 8: B.s.UnHighlight(); break;
            }
            lb = cslider_fake::log;
            for (int k = 0; k < 6; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) - delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = A.d[k];
            }
            int off = -1;
            for (size_t i = 0; i < offsetof(Arena, d) && off < 0; ++i) if (reinterpret_cast<uint8_t *>(&A)[i] != reinterpret_cast<uint8_t *>(&B)[i]) off = static_cast<int>(i);
            EXPECT(ra == rb && la == lb && off < 0 && std::memcmp(A.d, B.d, sizeof A.d) == 0, "cSlider op %d pc=%x vert %d byte %x", op, pc, A.s.mbVertical, off);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// PreviewSlider / ThreeStageSlider on synthetic objects (backing always set: the draws read uninitialized locals
// without it). Uses the cSlider fake vtable (Draw logs).
static void CheckStageSliders(int iters)
{
    auto oToggle = ORIG<void(THISCALL *)(PreviewSlider *, uint32_t)>(0x5122B0);
    auto oPDraw = ORIG<void(THISCALL *)(PreviewSlider *)>(0x527170);
    auto oUpdate = ORIG<uint32_t(THISCALL *)(ThreeStageSlider *, uint32_t)>(0x4FF590);
    auto oTDraw = ORIG<void(THISCALL *)(ThreeStageSlider *)>(0x5273A0);
    struct Arena { ThreeStageSlider s; unsigned char om[7][sizeof(FEObject)]; float d[7][20]; };
    static Arena A, B;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            cSlider &cs = A.s.base.base;
            FEObject **slots[] = {&cs.mpBacking, &cs.mpHandle, &cs.mpValue, &cs.mpCurrent, &cs.mpEndCap, &cs.mpEnd, &A.s.base.mpPreviewBar};
            for (int k = 0; k < 7; ++k) {
                for (float &f : A.d[k]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                auto *o = reinterpret_cast<FEObject *>(A.om[k]);
                std::memset(A.om[k], 0, sizeof A.om[k]);
                o->mpData = A.d[k];
                o->mu32Type = rng() % 3 ? 1 : 3;
                o->mu32Flags = rng() & 0x400001;
                *slots[k] = k == 0 || rng() % 6 ? o : nullptr;
            }
            cs.vtable = cslider_fake::vtable;
            cs.mbVertical = rng() % 2;
            float *fl[] = {&cs.mMax, &cs.mMin, &cs.mPrev, &cs.mValue, &cs.mStep, &cs.mLength, &cs.mMargin, &A.s.base.mPreview, &A.s.mStageLow};
            for (float *f : fl) *f = rng() % 4 ? static_cast<float>(static_cast<int>(rng() % 40) - 10) / static_cast<float>(1 + rng() % 4) : RefCoord();
            std::memcpy(&B, &A, sizeof A);
            const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
            cSlider &bs = B.s.base.base;
            FEObject **bslots[] = {&bs.mpBacking, &bs.mpHandle, &bs.mpValue, &bs.mpCurrent, &bs.mpEndCap, &bs.mpEnd, &B.s.base.mpPreviewBar};
            for (int k = 0; k < 7; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) + delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = B.d[k];
            }
            const int op = static_cast<int>(rng() % 4);
            const uint32_t msgs[3] = {0x9120409Eu, 0xB5971BF1u, rng()};
            const uint32_t msg = msgs[rng() % 3], flag = (rng() & ~0xFFu) | (rng() & 1);
            uint32_t ra = 0, rb = 0;
            cslider_fake::log.clear();
            if (op == 0) oToggle(&A.s.base, flag); else if (op == 1) oPDraw(&A.s.base); else if (op == 2) ra = oUpdate(&A.s, msg) & 0xFF; else oTDraw(&A.s);
            const std::vector<int> la = cslider_fake::log;
            cslider_fake::log.clear();
            if (op == 0) B.s.base.Toggle(flag & 0xFF); else if (op == 1) B.s.base.Draw(); else if (op == 2) rb = B.s.Update(msg); else B.s.Draw();
            const std::vector<int> lb = cslider_fake::log;
            for (int k = 0; k < 7; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) - delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = A.d[k];
            }
            int off = -1;
            for (size_t i = 0; i < offsetof(Arena, d) && off < 0; ++i) if (reinterpret_cast<uint8_t *>(&A)[i] != reinterpret_cast<uint8_t *>(&B)[i]) off = static_cast<int>(i);
            int doff = -1;
            for (size_t i = 0; i < sizeof A.d && doff < 0; ++i) if (reinterpret_cast<uint8_t *>(A.d)[i] != reinterpret_cast<uint8_t *>(B.d)[i]) doff = static_cast<int>(i);
            EXPECT(ra == rb && la == lb && off < 0 && doff < 0, "stage slider op %d pc=%x byte %x data %x", op, pc, off, doff);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// cFEng interface pieces that need no heap / renderer: GetBaseName, Load/UnloadResources (non-type-4),
// GetViewTransformation, RenderObject while not rendering or hidden.
static void CheckCFEngInterface(int iters)
{
    auto oBase = ORIG<void(__cdecl *)(char *, const char *)>(0x50C250);
    auto oLoad = ORIG<uint32_t(THISCALL *)(cFEng *, FEPackage *, int32_t, FEResourceRequest *)>(0x50C300);
    auto oUnload = ORIG<uint32_t(THISCALL *)(cFEng *, FEPackage *, int32_t, FEResourceRequest *)>(0x50C3A0);
    auto oView = ORIG<void(THISCALL *)(cFEng *, void *)>(0x50C490);
    auto oRender = ORIG<void(THISCALL *)(cFEng *, FEObject *)>(0x51C670);
    auto oCtx = ORIG<uint32_t(THISCALL *)(cFEng *, uint32_t, float *)>(0x50C3D0);
    auto oBegin = ORIG<void(THISCALL *)(cFEng *, FEPackage *, uint32_t)>(0x51C690);
    auto oMouse = ORIG<void(THISCALL *)(cFEng *, void *)>(0x50C860);
    static const char alpha[] = "ab./\\.xY";
    for (int it = 0; it < iters; ++it) {
        char path[40];
        const int n = static_cast<int>(rng() % 30);
        for (int i = 0; i < n; ++i) path[i] = alpha[rng() % (sizeof alpha - 1)];
        path[n] = 0;
        char a[64], b[64];
        std::memset(a, 0x5A, sizeof a); std::memset(b, 0x5A, sizeof b);
        oBase(a, path); GetBaseName(b, path);
        EXPECT(std::memcmp(a, b, sizeof a) == 0, "GetBaseName '%s'", path);
        static char names[3][40];
        FEResourceRequest ra[3], rb[3];
        for (auto &r : ra) { uint8_t *q = reinterpret_cast<uint8_t *>(&r); for (size_t i = 0; i < sizeof r; ++i) q[i] = static_cast<uint8_t>(rng()); }
        for (int k = 0; k < 3; ++k) {
            const int m = static_cast<int>(rng() % 30);
            for (int i = 0; i < m; ++i) names[k][i] = alpha[rng() % (sizeof alpha - 1)];
            names[k][m] = 0;
            ra[k].mpcName = names[k];
            ra[k].mu32Type = rng() % 4;   // 4 would bMalloc / bFree (no heap here)
        }
        std::memcpy(rb, ra, sizeof ra);
        alignas(4) static unsigned char fa[sizeof(cFEng)], fb[sizeof(cFEng)];
        const int32_t cnt = static_cast<int32_t>(rng() % 4) - 1;
        const int op = static_cast<int>(rng() % 2);
        uint32_t x = op ? oUnload(reinterpret_cast<cFEng *>(fa), nullptr, cnt, ra) : oLoad(reinterpret_cast<cFEng *>(fa), nullptr, cnt, ra);
        bool y = op ? reinterpret_cast<cFEng *>(fb)->UnloadResources(nullptr, cnt, rb) : reinterpret_cast<cFEng *>(fb)->LoadResources(nullptr, cnt, rb);
        EXPECT((x & 0xFF) == y && std::memcmp(ra, rb, sizeof ra) == 0, "cFEng resources op %d n %d", op, cnt);
        float ma[16], mb[16];
        for (int i = 0; i < 16; ++i) ma[i] = mb[i] = RefCoord();
        oView(reinterpret_cast<cFEng *>(fa), ma);
        reinterpret_cast<cFEng *>(fb)->GetViewTransformation(mb);
        EXPECT(std::memcmp(ma, mb, sizeof ma) == 0, "cFEng view");
        alignas(4) unsigned char om[sizeof(FEObject)] = {};
        auto *o = reinterpret_cast<FEObject *>(om);
        o->mu32Flags = rng() % 2 ? 1 : 0;
        reinterpret_cast<cFEng *>(fa)->mbRendering = o->mu32Flags ? static_cast<uint8_t>(rng()) : 0;
        oRender(reinterpret_cast<cFEng *>(fa), o);   // must not reach the renderer
        reinterpret_cast<cFEng *>(fb)->mbRendering = reinterpret_cast<cFEng *>(fa)->mbRendering;
        reinterpret_cast<cFEng *>(fb)->RenderObject(o);
        // render contexts / begin rendering / mouse (real globals: zero while the referee runs)
        static uint8_t ra2[0x20 + 0x40 * 0xD0], rb2[sizeof ra2];
        for (auto &c : ra2) c = static_cast<uint8_t>(rng());
        std::memcpy(rb2, ra2, sizeof ra2);
        reinterpret_cast<cFEng *>(fa)->mpRender = ra2;
        reinterpret_cast<cFEng *>(fb)->mpRender = rb2;
        const uint32_t ctx = rng() % 3 ? rng() % 0x40 : 0;
        float ca[16], cb[16];
        for (int i = 0; i < 16; ++i) ca[i] = cb[i] = RefCoord();
        const uint32_t xa = oCtx(reinterpret_cast<cFEng *>(fa), ctx | (rng() << 16), ca) & 0xFF;
        const bool xb = reinterpret_cast<cFEng *>(fb)->GetContextTransform(static_cast<uint16_t>(ctx), cb);
        EXPECT(xa == xb && std::memcmp(ca, cb, sizeof ca) == 0, "GetContextTransform %u", ctx);
        alignas(4) static unsigned char pm[sizeof(FEPackage)], dm[sizeof(FEPackageData)];
        std::memset(pm, 0, sizeof pm); std::memset(dm, 0, sizeof dm);
        auto *pkg = reinterpret_cast<FEPackage *>(pm);
        pkg->mpPackageData = reinterpret_cast<FEPackageData *>(dm);
        reinterpret_cast<FEPackageData *>(dm)->mVisible = static_cast<int16_t>(rng() % 2);
        pkg->mu32NameHash = rng() % 2 ? 0x942C98B5u : (rng() % 2 ? bStringHash("UI_MAGAZINEBACK.FNG") : rng());
        const uint32_t mag = (rng() & ~0xFFu) | (rng() & 1);
        oBegin(reinterpret_cast<cFEng *>(fa), pkg, mag);
        reinterpret_cast<cFEng *>(fb)->BeginPackageRendering(pkg, mag & 0xFF);
        EXPECT(reinterpret_cast<cFEng *>(fa)->mbRendering == reinterpret_cast<cFEng *>(fb)->mbRendering && std::memcmp(ra2, rb2, sizeof ra2) == 0, "BeginPackageRendering");
        uint16_t mia[4] = {1, 2, 3, 4}, mib[4] = {1, 2, 3, 4};
        oMouse(reinterpret_cast<cFEng *>(fa), mia);
        reinterpret_cast<cFEng *>(fb)->GetMouseInfo(mib);
        EXPECT(std::memcmp(mia, mib, sizeof mia) == 0, "GetMouseInfo");
    }
}

// cFEngRender pure math: eMulVector, quaternion matrix, corner sort, edge clippers, ClipAligned / ClipGeneral,
// MakeRenderMatrix, GenerateClipInfo (synthetic render + objects), ePoly::Init. Both precisions.
static float RenderCoord() { return rng() % 5 ? static_cast<float>(static_cast<int>(rng() % 801) - 400) / static_cast<float>(1 + rng() % 4) : RefCoord(); }
static void CheckRenderMath(int iters)
{
    auto oMulV = ORIG<void(__cdecl *)(bVector3 *, const bMatrix4 *, const bVector3 *)>(0x441000);
    auto oQuat = ORIG<void(THISCALL *)(const float *, bMatrix4 *)>(0x4FD980);
    auto oSort = ORIG<void(__cdecl *)(float *)>(0x50B3C0);
    using Edge = uint32_t(__cdecl *)(float *, float *, float *, const float *, const float *, const float *, uint32_t, float);
    const Edge oEdge[4] = {ORIG<Edge>(0x506CB0), ORIG<Edge>(0x507D80), ORIG<Edge>(0x508E60), ORIG<Edge>(0x509F30)};
    using Clip = uint32_t(THISCALL *)(void *, const FEClipInfo *, float *, float *, float *, float *, float *, float *);
    auto oAligned = ORIG<Clip>(0x50B010);
    auto oGeneral = ORIG<Clip>(0x505660);
    auto oMake = ORIG<FEClipInfo *(THISCALL *)(cFEngRender *, const void *, bMatrix4 *, int32_t *, uint32_t, float)>(0x51B0B0);
    auto oGenClip = ORIG<void(THISCALL *)(cFEngRender *, cFEngRenderContext *)>(0x51B3A0);
    auto oPoly = ORIG<void(THISCALL *)(ePoly *)>(0x5BB1C0);
    static cFEngRender ra, rb;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            bMatrix4 m;
            for (float &f : m.m) f = RenderCoord() / 100.0f;
            bVector3 in = {RenderCoord(), RenderCoord(), RenderCoord()}, a, b;
            oMulV(&a, &m, &in);
            eMulVector(&b, &m, &in);
            EXPECT(std::memcmp(&a, &b, sizeof a) == 0, "eMulVector pc=%x", pc);
            float q[4] = {RenderCoord() / 400.0f, RenderCoord() / 400.0f, RenderCoord() / 400.0f, RenderCoord() / 400.0f};
            bMatrix4 qa, qb;
            for (int i = 0; i < 16; ++i) qa.m[i] = qb.m[i] = RefCoord();
            oQuat(q, &qa);
            FEQuatToMatrix(q, &qb);
            EXPECT(std::memcmp(&qa, &qb, sizeof qa) == 0, "quat matrix pc=%x", pc);
            float ca[16], cb[16];
            for (float &f : ca) f = rng() % 3 ? RenderCoord() : 0.0f;
            std::memcpy(cb, ca, sizeof ca);
            oSort(ca);
            FESortClipCorners(cb);
            EXPECT(std::memcmp(ca, cb, sizeof ca) == 0, "sort corners");
            // clipping: random polygon (3..6 vertices) for the edges, quads for the full clips
            float v[16 * 4], uv[16 * 2], c[16 * 4];
            for (float &f : v) f = RenderCoord();
            for (float &f : uv) f = static_cast<float>(rng() % 1000) / 999.0f;
            for (float &f : c) f = static_cast<float>(rng() % 256) / 255.0f;
            const uint32_t n = 3 + rng() % 4;
            const float d = RenderCoord();
            const int e = static_cast<int>(rng() % 4);
            float oa[3][16 * 4], ob[3][16 * 4];
            std::memset(oa, 0x5A, sizeof oa); std::memset(ob, 0x5A, sizeof ob);
            const uint32_t na = oEdge[e](oa[0], oa[1], oa[2], v, uv, c, n, d);
            const uint32_t nb = FEClipEdge(e & 1, e < 2, ob[0], ob[1], ob[2], v, uv, c, n, d);
            EXPECT(na == nb && std::memcmp(oa, ob, sizeof oa) == 0, "clip edge %d n %u pc=%x: %u vs %u", e, n, pc, na, nb);
            FEClipInfo clip;
            for (auto &pl : clip.mPlanes) { for (float &f : pl) f = RenderCoord() / 400.0f; }
            for (float &f : clip.mDist) f = RenderCoord();
            clip.mu32Flags = rng();
            float va[6][16 * 4], vb[6][16 * 4];
            for (int k = 0; k < 6; ++k) for (int i = 0; i < 16 * 4; ++i) va[k][i] = vb[k][i] = (k == 0 ? v[i] : k == 1 ? uv[i % 32] : c[i]);
            const bool aligned = rng() % 2;
            const uint32_t ka = (aligned ? oAligned : oGeneral)(nullptr, &clip, va[0], va[1], va[2], va[3], va[4], va[5]);
            const uint32_t kb = (aligned ? FEClipAligned : FEClipGeneral)(&clip, vb[0], vb[1], vb[2], vb[3], vb[4], vb[5]);
            {
                int diff = 0;
                for (int k = 0; k < 6; ++k) if (std::memcmp(va[k], vb[k], sizeof va[k])) diff |= 1 << k;
                int fk = -1, fi = -1;
                for (int k = 0; k < 6 && fk < 0; ++k) for (int i = 0; i < 64 && fk < 0; ++i) if (std::memcmp(&va[k][i], &vb[k][i], 4)) { fk = k; fi = i; }
                EXPECT(ka == kb && diff == 0, "clip %s pc=%x: %u vs %u bufs %x first %d[%d] %g vs %g", aligned ? "aligned" : "general", pc, ka, kb, diff,
                       fk, fi, fk >= 0 ? va[fk][fi] : 0.0, fk >= 0 ? vb[fk][fi] : 0.0);
            }
            // MakeRenderMatrix / GenerateClipInfo on a synthetic renderer
            uint8_t *pa = reinterpret_cast<uint8_t *>(&ra);
            for (size_t i = 0; i < sizeof ra; ++i) pa[i] = static_cast<uint8_t>(rng());
            for (auto &cx : ra.mContexts) {
                for (float &f : cx.mMatrix.m) f = RenderCoord() / 100.0f;
                for (int32_t &cc : cx.mColour) cc = static_cast<int32_t>(rng() % 0x200);
            }
            static float data[2][40];
            for (float &f : data[0]) f = RenderCoord() / 10.0f;
            int32_t *di = reinterpret_cast<int32_t *>(data[0]);
            for (int i = 0; i < 4; ++i) di[i] = static_cast<int32_t>(rng() % 0x100);
            if (rng() % 2) data[0][4] = data[0][5] = data[0][6] = 0;                                  // no pivot
            if (rng() % 2) data[0][10] = data[0][11] = data[0][12] = 0, data[0][13] = 1;              // no rotation
            if (rng() % 2) data[0][14] = data[0][15] = data[0][16] = 1;                               // unit size
            std::memcpy(data[1], data[0], sizeof data[0]);
            static uint8_t ka2[sizeof(FEString)], kb2[sizeof(FEString)];
            std::memset(ka2, 0, sizeof ka2);
            auto *ko = reinterpret_cast<FEObject *>(ka2);
            ko->mpData = data[0];
            ko->mu32Type = rng() % 3 ? 1 : (rng() % 2 ? 9 : 2);
            ko->mu16RenderContext = static_cast<uint16_t>(rng() % 3 ? 0 : rng() % 64);
            for (auto &cx : ra.mContexts) cx.mpClipObject = rng() % 2 ? ko : nullptr;
            std::memcpy(&rb, &ra, sizeof ra);
            std::memcpy(kb2, ka2, sizeof ka2);
            auto *kob = reinterpret_cast<FEObject *>(kb2);
            kob->mpData = data[1];
            for (int i = 0; i < 64; ++i) if (rb.mContexts[i].mpClipObject) rb.mContexts[i].mpClipObject = kob;
            const uint32_t ctx = rng() % 3 ? 0 : rng() % 64;
            const float scale = rng() % 2 ? 1.0f : RenderCoord() / 100.0f;
            bMatrix4 ma, mb;
            int32_t cola[4], colb[4];
            FEClipInfo *xa = oMake(&ra, data[0], &ma, cola, ctx, scale);
            FEClipInfo *xb = rb.MakeRenderMatrix(data[1], &mb, colb, ctx, scale);
            const intptr_t delta = reinterpret_cast<uint8_t *>(&rb) - reinterpret_cast<uint8_t *>(&ra);
            EXPECT((xa ? reinterpret_cast<uint8_t *>(xa) + delta : nullptr) == reinterpret_cast<uint8_t *>(xb) && std::memcmp(&ma, &mb, sizeof ma) == 0 &&
                   std::memcmp(cola, colb, sizeof cola) == 0, "MakeRenderMatrix ctx %u pc=%x", ctx, pc);
            const int gi = static_cast<int>(rng() % 64);
            ra.mContexts[gi].mpClipObject = ko;
            rb.mContexts[gi].mpClipObject = kob;
            oGenClip(&ra, &ra.mContexts[gi]);
            rb.GenerateClipInfo(&rb.mContexts[gi]);
            EXPECT(std::memcmp(&ra.mContexts[gi].mClip, &rb.mContexts[gi].mClip, sizeof(FEClipInfo)) == 0, "GenerateClipInfo type %u pc=%x", ko->mu32Type, pc);
            ePoly pa1, pb1;
            std::memset(&pa1, 0x5A, sizeof pa1); std::memset(&pb1, 0x5A, sizeof pb1);
            oPoly(&pa1);
            pb1.Init();
            EXPECT(std::memcmp(&pa1, &pb1, sizeof pa1) == 0, "ePoly init");
        }
    }
    _controlfp(saved, _MCW_PC);
}

// cFEng::RenderObjectSpecial on synthetic objects / matrices / sorters, both precisions.
static void CheckRenderObjectSpecial(int iters)
{
    auto o = ORIG<void(THISCALL *)(cFEng *, FEObject *, const float *, uint32_t, void *)>(0x51C9B0);
    static FEngine::ObjectSorter sa, sb;
    static unsigned char oa[sizeof(FEString)], ob[sizeof(FEString)];
    static float da[20], db[20];
    static unsigned char fe[sizeof(cFEng)];
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            for (float &f : da) f = RenderCoord() / 10.0f;
            reinterpret_cast<int32_t *>(da)[3] = rng() % 4 ? 0xFF : 0;
            std::memcpy(db, da, sizeof da);
            std::memset(oa, 0, sizeof oa);
            auto *a = reinterpret_cast<FEObject *>(oa);
            a->mpData = da;
            a->mu16RenderContext = static_cast<uint16_t>(rng());
            std::memcpy(ob, oa, sizeof ob);
            auto *b = reinterpret_cast<FEObject *>(ob);
            b->mpData = db;
            float m[16];
            for (float &f : m) f = RenderCoord() / 100.0f;
            sa.mu32Count = sb.mu32Count = rng() % 4;
            const uint32_t ctx = rng();
            o(reinterpret_cast<cFEng *>(fe), a, m, ctx, &sa);
            reinterpret_cast<cFEng *>(fe)->RenderObjectSpecial(b, m, static_cast<uint16_t>(ctx), &sb);
            bool same = sa.mu32Count == sb.mu32Count && a->mu16RenderContext == b->mu16RenderContext;
            for (uint32_t i = 0; same && i < sa.mu32Count; ++i)
                same = sa.mEntries[i].mu32Key == sb.mEntries[i].mu32Key && (sa.mEntries[i].mpObject == a) == (sb.mEntries[i].mpObject == b);
            EXPECT(same, "RenderObjectSpecial pc=%x", pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FESliderWidget: ctor, sound events, Position / Show / Hide / SetFocus / UnsetFocus on synthetic boxes with a fake
// cSlider vtable that logs its calls (both sides must make the same calls in the same order).
namespace slider_fake {
std::vector<std::pair<int, uint32_t>> log;
void __thiscall Toggle(void *s, uint32_t on) { log.push_back({0x0C, (on & 0xFF) | (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(s)) << 8)}); }
void __thiscall Hi(void *) { log.push_back({0x10, 0}); }
void __thiscall Unhi(void *) { log.push_back({0x14, 0}); }
void __thiscall Draw(void *) { log.push_back({0x18, 0}); }
void *vtable[7] = {nullptr, nullptr, nullptr, (void *)&Toggle, (void *)&Hi, (void *)&Unhi, (void *)&Draw};
}
static void CheckSliderWidget(int iters)
{
    auto oCtor = ORIG<FESliderWidget *(THISCALL *)(FESliderWidget *, const float *, const float *, const float *, const float *, uint32_t)>(0x51E840);
    auto oSound = ORIG<uint32_t(THISCALL *)(FESliderWidget *, uint32_t, uint32_t)>(0x50D6F0);
    using M0 = void(THISCALL *)(FESliderWidget *);
    const M0 o0[4] = {ORIG<M0>(0x52DEF0), ORIG<M0>(0x50D740), ORIG<M0>(0x50D780), ORIG<M0>(0x51E8D0)};
    auto oFocus = ORIG<void(THISCALL *)(FESliderWidget *, const char *)>(0x52DFF0);
    struct Arena { FESliderWidget w; unsigned char om[5][sizeof(FEObject)]; float d[5][20]; };
    static Arena A, B;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            std::memcpy(&B, &A, sizeof A);
            const float pos[2] = {RefCoord(), RefCoord()}, size[2] = {RefCoord(), RefCoord()};
            const float vs[2] = {RefCoord(), RefCoord()}, vp[2] = {RefCoord(), RefCoord()};
            const uint32_t en = (rng() & ~0xFFu) | (rng() & 1);
            FESliderWidget *ra = oCtor(&A.w, pos, size, vs, vp, en), *rb = B.w.Construct(pos, size, vs, vp, en & 0xFF);
            EXPECT(ra == &A.w && rb == &B.w && std::memcmp(&A.w, &B.w, sizeof A.w) == 0, "FESliderWidget ctor");
            // members on a constructed widget
            FEObject **slots[] = {&A.w.base.mpObject, &A.w.mpLeftArrow, &A.w.mpRightArrow, &A.w.mSlider.mpBacking, &A.w.base.base.mpLabel};
            for (int k = 0; k < 5; ++k) {
                for (float &f : A.d[k]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                auto *o = reinterpret_cast<FEObject *>(A.om[k]);
                std::memset(A.om[k], 0, sizeof A.om[k]);
                o->mpData = A.d[k];
                o->mu32Type = rng() % 3 ? 1 : 3;
                o->mu32Flags = rng() & 0x400001;
                *slots[k] = (k == 0 || k == 3 || rng() % 5) ? o : nullptr;   // object / thumb always set (PC uses them unchecked in some paths)
            }
            A.w.mSlider.vtable = slider_fake::vtable;
            A.w.mSlider.mMax = static_cast<float>(rng() % 3); A.w.mSlider.mMin = static_cast<float>(rng() % 3);
            A.w.mSlider.mPrev = static_cast<float>(rng() % 3);
            std::memcpy(&B, &A, sizeof A);
            const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
            FEObject **bslots[] = {&B.w.base.mpObject, &B.w.mpLeftArrow, &B.w.mpRightArrow, &B.w.mSlider.mpBacking, &B.w.base.base.mpLabel};
            for (int k = 0; k < 5; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) + delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = B.d[k];
            }
            const int op = static_cast<int>(rng() % 6);
            slider_fake::log.clear();
            if (op == 5) {
                const uint32_t ev = rng() % 7;
                EXPECT(oSound(&A.w, rng(), ev) == B.w.DetermineSpecialSound(0, ev), "FESliderWidget sound %u", ev);
                continue;
            }
            if (op < 4) o0[op](&A.w);
            else oFocus(&A.w, "Pkg");
            auto la = slider_fake::log;
            slider_fake::log.clear();
            if (op == 0) B.w.Position();
            else if (op == 1) B.w.Show();
            else if (op == 2) B.w.Hide();
            else if (op == 3) B.w.UnsetFocus();
            else B.w.SetFocus("Pkg");
            auto lb = slider_fake::log;
            for (auto &e : lb) if (e.first == 0x0C) e.second = (e.second & 0xFF) | (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&A.w.mSlider)) << 8);
            for (int k = 0; k < 5; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) - delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = A.d[k];
            }
            EXPECT(la == lb && std::memcmp(&A, &B, offsetof(Arena, d)) == 0 && std::memcmp(A.d, B.d, sizeof A.d) == 0, "FESliderWidget op %d pc=%x", op, pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FEToggleWidget / FEToggleImageWidget: ctor + every member except CheckMouse (virtual calls, sound) on synthetic
// boxes, both precisions. The image widget has no PC ctor of its own (inlined into BrandScrollerThing 521E30).
static void CheckToggleWidget(int iters)
{
    auto oCtor = ORIG<FEToggleWidget *(THISCALL *)(FEToggleWidget *, const float *, const float *, const float *, const float *, uint32_t)>(0x51E230);
    using M0 = void(THISCALL *)(void *);
    using M1 = void(THISCALL *)(void *, uintptr_t);
    // 0..6 toggle: Position Enable Disable Show Hide UnsetFocus SetFocus; 7..13 image: same; 14 BlinkArrows
    const uintptr_t addr[15] = {0x52DAC0, 0x51E270, 0x51E3E0, 0x50D5D0, 0x50D610, 0x51E550, 0x52DB40,
                                0x52DCC0, 0x51E6F0, 0x51E760, 0x50D650, 0x50D6A0, 0x51E7D0, 0x52DE80, 0x51E6B0};
    struct Arena { FEToggleImageWidget w; unsigned char om[6][sizeof(FEObject)]; float d[6][20]; };
    static Arena A, B;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            std::memcpy(&B, &A, sizeof A);
            const float pos[2] = {RefCoord(), RefCoord()}, size[2] = {RefCoord(), RefCoord()};
            const float vs[2] = {RefCoord(), RefCoord()}, vp[2] = {RefCoord(), RefCoord()};
            const uint32_t en = (rng() & ~0xFFu) | (rng() & 1);
            FEToggleWidget *ra = oCtor(&A.w.base, pos, size, vs, vp, en), *rb = B.w.base.Construct(pos, size, vs, vp, en & 0xFF);
            EXPECT(ra == &A.w.base && rb == &B.w.base && std::memcmp(&A.w.base, &B.w.base, sizeof A.w.base) == 0, "FEToggleWidget ctor");
            FEObject **slots[] = {&A.w.base.base.mpObject, &A.w.base.base.mpValue, &A.w.base.mpLeftArrow, &A.w.base.mpRightArrow,
                                  &A.w.base.base.base.mpLabel, &A.w.mpImage};
            for (int k = 0; k < 6; ++k) {
                for (float &f : A.d[k]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                auto *o = reinterpret_cast<FEObject *>(A.om[k]);
                std::memset(A.om[k], 0, sizeof A.om[k]);
                o->mpData = A.d[k];
                o->mu32Type = rng() % 3 ? 1 : 3;
                o->mu32Flags = rng() & 0x400001;
                *slots[k] = (k != 4 || rng() % 3) ? o : nullptr;   // only the label is optional on PC
            }
            std::memcpy(&B, &A, sizeof A);
            const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
            FEObject **bslots[] = {&B.w.base.base.mpObject, &B.w.base.base.mpValue, &B.w.base.mpLeftArrow, &B.w.base.mpRightArrow,
                                   &B.w.base.base.base.mpLabel, &B.w.mpImage};
            for (int k = 0; k < 6; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) + delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = B.d[k];
            }
            const int op = static_cast<int>(rng() % 15);
            const uint32_t msgs[3] = {0x9120409Eu, 0xB5971BF1u, rng()};
            const uint32_t msg = msgs[rng() % 3];
            if (op == 6 || op == 13) ORIG<M1>(addr[op])(&A.w, reinterpret_cast<uintptr_t>("Pkg"));
            else if (op == 14) ORIG<M1>(addr[op])(&A.w, msg);
            else ORIG<M0>(addr[op])(&A.w);
            FEToggleWidget &t = B.w.base;
            switch (op) {
            case 0: t.Position(); break;     case 1: t.Enable(); break;      case 2: t.Disable(); break;
            case 3: t.Show(); break;         case 4: t.Hide(); break;        case 5: t.UnsetFocus(); break;
            case 6: t.SetFocus("Pkg"); break;
            case 7: B.w.Position(); break;   case 8: B.w.Enable(); break;    case 9: B.w.Disable(); break;
            case 10: B.w.Show(); break;      case 11: B.w.Hide(); break;     case 12: B.w.UnsetFocus(); break;
            case 13: B.w.SetFocus("Pkg"); break;
            default: t.BlinkArrows(msg); break;
            }
            for (int k = 0; k < 6; ++k) {
                if (*bslots[k]) *bslots[k] = reinterpret_cast<FEObject *>(reinterpret_cast<uint8_t *>(*bslots[k]) - delta);
                reinterpret_cast<FEObject *>(B.om[k])->mpData = A.d[k];
            }
            EXPECT(std::memcmp(&A, &B, offsetof(Arena, d)) == 0 && std::memcmp(A.d, B.d, sizeof A.d) == 0, "FEToggleWidget op %d pc=%x", op, pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// TuningSlider: ctor, SetSliderObjects and Position / Show / Hide / SetFocus / UnsetFocus on synthetic boxes; the
// embedded ThreeStageSlider gets the logging fake vtable (both sides must make the same slider calls).
static void CheckTuningSlider(int iters)
{
    auto oCtor = ORIG<TuningSlider *(THISCALL *)(TuningSlider *, int32_t, const float *, const float *, const float *, const float *, uint32_t, uint32_t, uint32_t, float)>(0x522AF0);
    auto oObjs = ORIG<void(THISCALL *)(TuningSlider *, FEObject *, FEObject *, FEObject *, FEObject *, FEObject *, FEObject *, FEObject *)>(0x530D20);
    using M0 = void(THISCALL *)(TuningSlider *);
    const M0 o0[3] = {ORIG<M0>(0x530AA0), ORIG<M0>(0x522BC0), ORIG<M0>(0x522C30)};
    auto oUnset = ORIG<M0>(0x522CA0);
    auto oFocus = ORIG<void(THISCALL *)(TuningSlider *, const char *)>(0x530F30);
    constexpr int K = 10;
    struct Arena { TuningSlider w; unsigned char om[K][sizeof(FEObject)]; float d[K][20]; };
    static Arena A, B;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            std::memcpy(&B, &A, sizeof A);
            const float pos[2] = {RefCoord(), RefCoord()}, size[2] = {RefCoord(), RefCoord()};
            const float vs[2] = {RefCoord(), RefCoord()}, vp[2] = {RefCoord(), RefCoord()};
            const uint32_t en = (rng() & ~0xFFu) | (rng() & 1), lo = rng() % 3 ? rng() : 0, hi = rng() % 3 ? rng() : 0;
            const int32_t type = static_cast<int32_t>(rng() % 20);
            const float yo = RefCoord() / 10.0f;
            TuningSlider *ra = oCtor(&A.w, type, pos, size, vs, vp, lo, hi, en, yo);
            TuningSlider *rb = B.w.Construct(type, pos, size, vs, vp, lo, hi, en & 0xFF, yo);
            EXPECT(ra == &A.w && rb == &B.w && std::memcmp(&A.w, &B.w, sizeof A.w) == 0, "TuningSlider ctor");
            // objects: 0 object 1 value 2 left 3 right 4 label 5 min 6 max 7 extra | 8 backing 9 slider value
            for (int k = 0; k < K; ++k) {
                for (float &f : A.d[k]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                auto *o = reinterpret_cast<FEObject *>(A.om[k]);
                std::memset(A.om[k], 0, sizeof A.om[k]);
                o->mpData = A.d[k];
                o->mu32Type = rng() % 3 ? 1 : 3;
                o->mu32Flags = rng() & 0x400001;
            }
            auto obj = [](Arena &R, int k) { return reinterpret_cast<FEObject *>(R.om[k]); };
            auto wire = [&](Arena &R, bool label, bool extra) {
                R.w.base.base.mpObject = obj(R, 0); R.w.base.base.mpValue = obj(R, 1);
                R.w.base.mpLeftArrow = obj(R, 2); R.w.base.mpRightArrow = obj(R, 3);
                R.w.base.base.base.mpLabel = label ? obj(R, 4) : nullptr;
                R.w.mpMinLabel = obj(R, 5); R.w.mpMaxLabel = obj(R, 6); R.w.mpExtra = extra ? obj(R, 7) : nullptr;
            };
            const bool label = rng() % 3, extra = rng() % 2;
            wire(A, label, extra);
            A.w.mSlider.base.base.mMargin = RefCoord() / 20.0f;
            std::memcpy(&B, &A, sizeof A);
            for (int k = 0; k < K; ++k) reinterpret_cast<FEObject *>(B.om[k])->mpData = B.d[k];
            wire(B, label, extra);
            oObjs(&A.w, obj(A, 8), nullptr, obj(A, 9), nullptr, nullptr, nullptr, nullptr);
            B.w.SetSliderObjects(obj(B, 8), nullptr, obj(B, 9), nullptr, nullptr, nullptr, nullptr);
            A.w.mSlider.base.base.vtable = B.w.mSlider.base.base.vtable = slider_fake::vtable;
            const int op = static_cast<int>(rng() % 5);
            slider_fake::log.clear();
            if (op < 3) o0[op](&A.w); else if (op == 3) oUnset(&A.w); else oFocus(&A.w, "Pkg");
            auto la = slider_fake::log;
            slider_fake::log.clear();
            switch (op) {
            case 0: B.w.Position(); break; case 1: B.w.Hide(); break; case 2: B.w.Show(); break;
            case 3: B.w.UnsetFocus(); break; default: B.w.SetFocus("Pkg"); break;
            }
            auto lb = slider_fake::log;
            for (auto &e : lb) if (e.first == 0x0C) e.second = (e.second & 0xFF) | (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&A.w.mSlider)) << 8);
            // compare with B's pointers mapped back into A
            wire(B, label, extra);   // (pointers differ by arena only; rewire A-relative for the memcmp)
            const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
            uint8_t *wb = reinterpret_cast<uint8_t *>(&B.w);
            for (size_t off = 0; off + 4 <= sizeof B.w; off += 4) {
                uintptr_t v; std::memcpy(&v, wb + off, 4);
                if (v >= reinterpret_cast<uintptr_t>(&B) && v < reinterpret_cast<uintptr_t>(&B) + sizeof B) { v -= delta; std::memcpy(wb + off, &v, 4); }
            }
            for (int k = 0; k < K; ++k) reinterpret_cast<FEObject *>(B.om[k])->mpData = A.d[k];
            EXPECT(la == lb && std::memcmp(&A, &B, offsetof(Arena, d)) == 0 && std::memcmp(A.d, B.d, sizeof A.d) == 0, "TuningSlider op %d pc=%x", op, pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// Language string search (original takes the hash in EDX), tuning name/units hashes, FEngSetLanguageHash. The
// search runs on synthetic tables swapped into the game's globals (both sides read them).
static const char *OrigSearchForString(uint32_t hash)
{
    const char *r;
    uint32_t h = hash;
    asm volatile("call *%2" : "=a"(r), "+d"(h) : "r"(0x4FF9D0u) : "ecx", "memory", "cc");
    return r;
}
static void CheckLocalize(int iters)
{
    uint32_t &gCount = *reinterpret_cast<uint32_t *>(0x8383D0);
    const LanguageStringRecord *&gTable = *reinterpret_cast<const LanguageStringRecord **>(0x8383D8);
    const uint32_t savedCount = gCount;
    const LanguageStringRecord *savedTable = gTable;
    static LanguageStringRecord tab[64];
    static char texts[64];
    for (int it = 0; it < iters; ++it) {
        const uint32_t n = 1 + rng() % 64;
        uint32_t h = rng() % 50;
        for (uint32_t i = 0; i < n; ++i) { h += 1 + rng() % (rng() % 4 ? 3 : 0x10000000); tab[i] = {h, &texts[i]}; }
        if (rng() % 8 == 0) tab[rng() % n].hash = 0xC01A6F63u;   // unsorted entry: same probing on both sides
        gCount = n; gTable = tab;
        for (int q = 0; q < 8; ++q) {
            const uint32_t key = rng() % 2 ? tab[rng() % n].hash : (rng() % 2 ? rng() : tab[rng() % n].hash + 1);
            EXPECT(OrigSearchForString(key) == SearchForString(key), "SearchForString n=%u key=%08X", n, key);
        }
    }
    gCount = savedCount; gTable = savedTable;
    auto oName = ORIG<uint32_t(__cdecl *)(int32_t)>(0x50FE90);
    auto oUnits = ORIG<uint32_t(__cdecl *)(int32_t, int32_t)>(0x50FFB0);
    for (int32_t t = -3; t < 32; ++t) {
        EXPECT(oName(t) == GetNameHashForTuningType(t), "GetNameHashForTuningType %d", t);
        for (int32_t u = -1; u < 4; ++u) EXPECT(oUnits(t, u) == GetUnitsHashForTuningType(t, u), "GetUnitsHashForTuningType %d %d", t, u);
    }
    auto oLang = ORIG<void(__cdecl *)(FEObject *, uint32_t)>(0x50C900);
    for (int it = 0; it < iters; ++it) {
        alignas(8) static unsigned char a[sizeof(FEString)], b[sizeof(FEString)];
        for (auto &c : a) c = static_cast<unsigned char>(rng());
        std::memcpy(b, a, sizeof a);
        const uint32_t hash = rng() % 2 ? reinterpret_cast<FEString *>(a)->mu32LabelHash : rng();
        oLang(reinterpret_cast<FEObject *>(a), hash);
        FEngSetLanguageHash(reinterpret_cast<FEObject *>(b), hash);
        EXPECT(std::memcmp(a, b, sizeof a) == 0, "FEngSetLanguageHash");
    }
}

// MenuScreen: PC-only package classifiers, ctor (help bar mask 0: no FEng in DllMain) and dtor (no delete). The ctor's
// profile branch depends on 0x8500D0 (both sides read the same global).
static void CheckMenuScreen(int iters)
{
    static const char *names[] = {"ScreenPrintf.fng", "Chyron_FE.fng", "GarageMain.fng", "UI_PC_Help_Bar.fng", "chyron_fe.fng",
        "GenericDialog_ThreeButton.fng", "GenericDialog_Animate_SMALL.fng", "GenericDialog_SMALL.fng", "GenericDialog_Animate_MED.fng",
        "GenericDialog_MED.fng", "GenericDialog_Animate_LARGE.fng", "genericdialog_large.FNG", "GenericDialog.fng", "Options.fng", ""};
    auto oOverlay = ORIG<uint32_t(__cdecl *)(const char *)>(0x49FFA0);
    auto oDialog = ORIG<uint32_t(__cdecl *)(const char *)>(0x512170);
    for (const char *n : names) {
        EXPECT(oOverlay(n) == static_cast<uint32_t>(IsOverlayPackage(n)), "IsOverlayPackage %s", n);
        EXPECT((oDialog(n) & 0xFF) == static_cast<uint32_t>(IsGenericDialogPackage(n)), "IsGenericDialogPackage %s", n);
    }
    auto oCtor = ORIG<MenuScreen *(THISCALL *)(MenuScreen *, const ScreenConstructorData *)>(0x551DE0);
    auto oDtor = ORIG<MenuScreen *(THISCALL *)(MenuScreen *, uint32_t)>(0x51DC60);
    for (int it = 0; it < iters; ++it) {
        alignas(8) static unsigned char a[sizeof(MenuScreen)], b[sizeof(MenuScreen)];
        for (auto &c : a) c = static_cast<unsigned char>(rng());
        std::memcpy(b, a, sizeof a);
        ScreenConstructorData d{names[rng() % 15], reinterpret_cast<FEPackage *>(static_cast<uintptr_t>(rng())), rng(), 0, rng()};
        MenuScreen *ra = oCtor(reinterpret_cast<MenuScreen *>(a), &d), *rb = reinterpret_cast<MenuScreen *>(b)->Construct(&d);
        EXPECT(ra == reinterpret_cast<MenuScreen *>(a) && rb == reinterpret_cast<MenuScreen *>(b) && std::memcmp(a, b, sizeof a) == 0, "MenuScreen ctor %s", d.mpPackageName);
        reinterpret_cast<MenuScreen *>(a)->vtable = reinterpret_cast<MenuScreen *>(b)->vtable = nullptr;
        oDtor(reinterpret_cast<MenuScreen *>(a), 0);
        reinterpret_cast<MenuScreen *>(b)->ScalarDeletingDestructor(0);
        EXPECT(std::memcmp(a, b, sizeof a) == 0, "MenuScreen dtor");
    }
}

// UIWidgetMenu: list members on fake widgets whose vtable logs calls as
// (slot, widget index, arg), SetInitialPositions on synthetic master boxes, dtor body (deletes go to the fake slot 0).
namespace uiw_fake {
struct Arena;
uint8_t *base = nullptr;   // widget array of the arena being run
size_t stride = 0;
std::vector<std::array<uint32_t, 3>> log;
uint32_t Idx(void *w) { return static_cast<uint32_t>((static_cast<uint8_t *>(w) - base) / stride); }
void *__thiscall S0(void *w, uint32_t f) { log.push_back({0, Idx(w), f}); return w; }
template <int K> void __thiscall S(void *w) { log.push_back({K, Idx(w), 0}); }
void __thiscall S9(void *w, const char *p) { log.push_back({9, Idx(w), p ? static_cast<uint32_t>(p[0]) : 0u}); }
void __thiscall S13(void *w, float y) { uint32_t b; std::memcpy(&b, &y, 4); log.push_back({13, Idx(w), b}); }
void __thiscall S1(void *w, const char *p, uint32_t m) { log.push_back({1, Idx(w), m ^ (p ? static_cast<uint32_t>(p[0]) : 0u)}); }
void __thiscall S2(void *w, const char *, float x, float y) { uint32_t a, b; std::memcpy(&a, &x, 4); std::memcpy(&b, &y, 4); log.push_back({2, Idx(w), a ^ (b * 3)}); }
uint32_t __thiscall S14(void *w, uint32_t m, uint32_t e) { log.push_back({14, Idx(w), m ^ e}); return e * 3 + 1; }
uint32_t __thiscall S15(void *w) { log.push_back({15, Idx(w), 0}); return Idx(w) + 2; }
void *vtable[16] = {(void *)&S0, (void *)&S1, (void *)&S2, (void *)&S<3>, (void *)&S<4>, (void *)&S<5>, (void *)&S<6>, nullptr,
                    (void *)&S<8>, (void *)&S9, (void *)&S<10>, nullptr, nullptr, (void *)&S13, (void *)&S14, (void *)&S15};
void __thiscall MenuSetup(void *) { log.push_back({100, 0, 0}); }
void *menuVtable[5] = {nullptr, nullptr, nullptr, nullptr, (void *)&MenuSetup};
}
static void CheckUIWidgetMenu(int iters)
{
    using M0 = void(THISCALL *)(UIWidgetMenu *);
    const M0 o0[7] = {ORIG<M0>(0x51FF40), ORIG<M0>(0x51FF80), ORIG<M0>(0x52FD40), ORIG<M0>(0x52FDB0), ORIG<M0>(0x52FE60),
                      ORIG<M0>(0x52FEA0), ORIG<M0>(0x4CBC30)};
    auto oIndex = ORIG<uint32_t(THISCALL *)(UIWidgetMenu *, const FEWidget *)>(0x520020);
    auto oGetNode = ORIG<WidgetListNode *(THISCALL *)(WidgetListNode *, uint32_t)>(0x43BE60);
    auto oScroll = ORIG<void(THISCALL *)(UIWidgetMenu *, int32_t)>(0x5390E0);
    auto oScrollW = ORIG<void(THISCALL *)(UIWidgetMenu *, int32_t)>(0x539370);
    auto oStore = ORIG<void(THISCALL *)(UIWidgetMenu *, uint32_t, FEObject *, uint32_t, uint32_t)>(0x50DAE0);
    auto oInit = ORIG<void(THISCALL *)(UIWidgetMenu *, int32_t)>(0x539650);
    auto oNotify = ORIG<void(THISCALL *)(UIWidgetMenu *, uint32_t, FEObject *, uint32_t, uint32_t)>(0x556020);
    auto oAt = ORIG<FEWidget *(THISCALL *)(UIWidgetMenu *, float, float)>(0x51FEA0);
    auto oSelect = ORIG<void(THISCALL *)(UIWidgetMenu *, FEWidget *)>(0x50DB10);
    auto oMouse = ORIG<void(THISCALL *)(UIWidgetMenu *)>(0x52FC40);
    auto oSound = ORIG<uint32_t(THISCALL *)(FEWidget *, uint32_t)>(0x50D460);
    auto oRect = ORIG<uint8_t(__cdecl *)(float, float, const float *, const float *)>(0x50CD00);
    static const uint32_t kMsgs[] = {0x0C407210u, 0x911AB364u, 0x0153E234u, 0x35F8620Bu, 0x406415E3u, 0x72619778u, 0x911C0A4Bu,
                                     0x84378BEFu, 0x9120409Eu, 0xB5971BF1u, 0x92B703B5u, 0xAF0BBD92u, 0x9803F6E2u, 0xB5AF2461u, 0x12345u};
    uint8_t *mouse = reinterpret_cast<uint8_t *>(0x8763D4);
    uint8_t savedMouse[0x24];
    std::memcpy(savedMouse, mouse, sizeof savedMouse);
    struct Arena { UIWidgetMenu m; FEButtonWidget w[12]; unsigned char om[2][sizeof(FEObject)]; float d[2][20]; };
    static Arena A, B;
    uiw_fake::stride = sizeof(FEButtonWidget);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            std::memcpy(&B, &A, sizeof A);
            // (no ctor compare: the original's package lookups need cFEng, which doesn't exist in DllMain)
            const bool trace = std::getenv("REFEREE_TRACE") != nullptr;
            // build the same list in both arenas
            const int n = static_cast<int>(rng() % 12);
            for (Arena *R : {&A, &B}) {
                R->m.mWidgets.next = R->m.mWidgets.prev = &R->m.mWidgets;
                for (int i = 0; i < n; ++i) {
                    R->w[i].base.vtable = uiw_fake::vtable;
                    auto *nd = reinterpret_cast<WidgetListNode *>(&R->w[i].base._unknown04);
                    nd->next = &R->m.mWidgets; nd->prev = R->m.mWidgets.prev;
                    R->m.mWidgets.prev->next = nd; R->m.mWidgets.prev = nd;
                }
                for (int k = 0; k < 2; ++k) {
                    auto *o = reinterpret_cast<FEObject *>(R->om[k]);
                    std::memset(R->om[k], 0, sizeof R->om[k]);
                    o->mpData = R->d[k]; o->mu32Type = 1;
                }
                R->m.mpTitleMaster = reinterpret_cast<FEObject *>(R->om[0]);
                R->m.mpDataMaster = reinterpret_cast<FEObject *>(R->om[1]);
            }
            for (int k = 0; k < 2; ++k) for (int j = 0; j < 20; ++j) A.d[k][j] = B.d[k][j] = RefCoord();
            const int pick = n ? static_cast<int>(rng() % (n + 1)) : 0;   // n: not in the list
            const uint32_t cnt = 1 + rng() % 9, idx = rng() % 14;
            const bool cur = rng() % 4 != 0, done = rng() % 2;
            const uint32_t rpos = rng() % 14;
            const uint8_t sb = rng() % 2, dirty = rng() % 6 == 0;
            const int32_t dir = rng() % 2 ? 1 : static_cast<int32_t>(rng() % 3) - 1;
            for (Arena *R : {&A, &B}) {
                R->m.mpCurrent = cur && n ? &R->w[pick % n].base : nullptr;   // only widgets with the fake vtable
                R->m.mpViewTop = n ? &R->w[pick % n].base : nullptr;   // the game never has a null view top in a non-empty list
                R->m.mpDoneButton = done ? reinterpret_cast<FEObject *>(R->om[0]) : nullptr;
                R->m.mVisibleCount = cnt; R->m.mCurrentIndex = idx;
                R->m.base.mpPackageName = "Pkg";
                R->m.mScrollPos = rpos; R->m.mbScrollBarEnabled = sb; R->m.mbViewDirty = dirty;
                FEObject *so = reinterpret_cast<FEObject *>(R->om[1]);
                for (FEObject **f : {&R->m.mScrollBar.mpBacking, &R->m.mScrollBar.mpHandle, &R->m.mScrollBar.mpArrow1, &R->m.mScrollBar.mpArrow2,
                                     &R->m.mScrollBar.mpBackingEnd1, &R->m.mScrollBar.mpBackingEnd2}) *f = sb ? so : nullptr;
                R->m.mScrollBar.mbNoMove = 1;   // keep the shared object still (Update positions are compared via the bar fields)
            }
            const int op = static_cast<int>(rng() % 19);
            // FEngSetCurrentButton / package scripts need cFEng and the sound player (absent in DllMain)
            if (op >= 9 && op != 12) A.m.mpDoneButton = B.m.mpDoneButton = nullptr;
            const uint32_t msg = kMsgs[rng() % (sizeof kMsgs / sizeof kMsgs[0])];
            const uint32_t s1 = rng(), s2 = rng(), s3 = rng();
            const float px = RefCoord(), py = RefCoord();
            const int32_t initIdx = static_cast<int32_t>(rng() % (n + 2));
            for (Arena *R : {&A, &B}) {
                R->m.mbSoundsEnabled = 0;
                R->m.base.vtable = uiw_fake::menuVtable;
                if (op == 13 && msg == 0x0153E234u && !R->m.mpCurrent) R->m.mpCurrent = n ? &R->w[0].base : nullptr;
            }
            if (op == 13 && msg == 0x0153E234u && !n) continue;   // the original dereferences the selection
            for (size_t i = 0; i < sizeof savedMouse; ++i) mouse[i] = static_cast<uint8_t>(rng() % 4 == 0 ? rng() : 0);
            int32_t mx = static_cast<int32_t>(rng() % 640), my = static_cast<int32_t>(rng() % 480);
            std::memcpy(mouse, &mx, 4); std::memcpy(mouse + 4, &my, 4);
            uiw_fake::log.clear(); uiw_fake::base = reinterpret_cast<uint8_t *>(A.w);
            uint32_t ia = 0, ib = 0;
            if (trace) { std::printf("uiw op %d n %d pick %d cnt %u idx %u\n", op, n, pick, cnt, idx); std::fflush(stdout); }
            const bool target = rng() % 3 != 0;
            if (op < 7) o0[op](&A.m);
            else if (op == 11) oStore(&A.m, s1, reinterpret_cast<FEObject *>(static_cast<uintptr_t>(s2)), s3, s1 ^ s3);
            else if (op == 12) oInit(&A.m, initIdx);
            else if (op == 13) oNotify(&A.m, msg, reinterpret_cast<FEObject *>(static_cast<uintptr_t>(s2)), s3, s1);
            else if (op == 14) ia = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(oAt(&A.m, px, py)) ? reinterpret_cast<uint8_t *>(oAt(&A.m, px, py)) - reinterpret_cast<uint8_t *>(&A) : 0);
            else if (op == 15) { if (n) oSelect(&A.m, &A.w[pick % n].base); }
            else if (op == 16) oMouse(&A.m);
            else if (op == 17) { const float pp[2] = {RefCoord(), RefCoord()}, ss[2] = {RefCoord(), RefCoord()}; ia = oRect(px, py, pp, ss); ib = FEngTestForIntersection(px, py, pp, ss); }
            else if (op == 18) { if (n) ia = oSound(&A.w[pick % n].base, msg); }
            else if (op == 9) oScroll(&A.m, dir);
            else if (op == 10) oScrollW(&A.m, dir);
            else if (op == 7) ia = oIndex(&A.m, target ? &A.w[pick].base : nullptr);
            else ia = static_cast<uint32_t>(reinterpret_cast<uint8_t *>(oGetNode(&A.m.mWidgets, pick)) - reinterpret_cast<uint8_t *>(&A));
            auto la = uiw_fake::log;
            uiw_fake::log.clear(); uiw_fake::base = reinterpret_cast<uint8_t *>(B.w);
            switch (op) {
            case 0: B.m.RefreshWidgets(); break;      case 1: B.m.SetInitialPositions(); break;
            case 2: B.m.ClearWidgets(); break;        case 3: B.m.Reposition(); break;
            case 4: B.m.Reset(); break;               case 5: B.m.SyncViewToSelection(); break;
            case 6: B.m.DestructBody(); break;
            case 7: ib = B.m.GetWidgetIndex(target ? &B.w[pick].base : nullptr); break;
            case 11: B.m.StorePrevNotification(s1, reinterpret_cast<FEObject *>(static_cast<uintptr_t>(s2)), s3, s1 ^ s3); break;
            case 12: B.m.SetInitialOption(initIdx); break;
            case 13: B.m.NotificationMessage(msg, reinterpret_cast<FEObject *>(static_cast<uintptr_t>(s2)), s3, s1); break;
            case 14: { FEWidget *r = B.m.WidgetAtPoint(px, py); ib = r ? static_cast<uint32_t>(reinterpret_cast<uint8_t *>(r) - reinterpret_cast<uint8_t *>(&B)) : 0; break; }
            case 15: if (n) B.m.SelectWidget(&B.w[pick % n].base); break;
            case 16: B.m.HandleMouse(); break;
            case 17: break;   // compared above
            case 18: if (n) ib = B.w[pick % n].base.DetermineSound(msg); break;
            case 9: B.m.Scroll(dir); break;
            case 10: B.m.ScrollWrapped(dir); break;
            default: ib = static_cast<uint32_t>(reinterpret_cast<uint8_t *>(ListGetNode(&B.m.mWidgets, pick)) - reinterpret_cast<uint8_t *>(&B)); break;
            }
            auto lb = uiw_fake::log;
            // map B-arena pointers back into A for the byte compare (known pointer fields only: random bytes elsewhere
            // may happen to look like B addresses)
            const intptr_t delta = reinterpret_cast<uint8_t *>(&B) - reinterpret_cast<uint8_t *>(&A);
            auto unmap = [&](void *field) {
                uintptr_t v; std::memcpy(&v, field, 4);
                if (v >= reinterpret_cast<uintptr_t>(&B) && v < reinterpret_cast<uintptr_t>(&B) + sizeof B) { v -= delta; std::memcpy(field, &v, 4); }
            };
            unmap(&B.m.mWidgets.next); unmap(&B.m.mWidgets.prev); unmap(&B.m.mpCurrent); unmap(&B.m.mpViewTop);
            unmap(&B.m.mpTitleMaster); unmap(&B.m.mpDataMaster); unmap(&B.m.mpDoneButton);
            for (int i = 0; i < n; ++i) { unmap(&B.w[i].base._unknown04); unmap(&B.w[i].base._unknown08); }
            for (FEObject **f : {&B.m.mScrollBar.mpBacking, &B.m.mScrollBar.mpHandle, &B.m.mScrollBar.mpArrow1, &B.m.mScrollBar.mpArrow2,
                                 &B.m.mScrollBar.mpBackingEnd1, &B.m.mScrollBar.mpBackingEnd2}) unmap(f);
            for (int k = 0; k < 2; ++k) reinterpret_cast<FEObject *>(B.om[k])->mpData = A.d[k];
            if (trace && !(ia == ib && la == lb && std::memcmp(&A, &B, sizeof A) == 0)) {
                size_t off = 0;
                while (off < sizeof A && reinterpret_cast<uint8_t *>(&A)[off] == reinterpret_cast<uint8_t *>(&B)[off]) ++off;
                std::printf("  ia %u ib %u log %u/%u first diff +0x%x (m %u)\n", ia, ib, unsigned(la.size()), unsigned(lb.size()), unsigned(off), unsigned(sizeof A.m));
                for (size_t i = 0; i < la.size() && i < 4; ++i) std::printf("  a %u %u %08x\n", la[i][0], la[i][1], la[i][2]);
                for (size_t i = 0; i < lb.size() && i < 4; ++i) std::printf("  b %u %u %08x\n", lb[i][0], lb[i][1], lb[i][2]);
            }
            EXPECT(ia == ib && la == lb && std::memcmp(&A, &B, sizeof A) == 0, "UIWidgetMenu op %d n=%d pick=%d msg=%08x pc=%x", op, n, pick, msg, pc);
        }
    }
    _controlfp(saved, _MCW_PC);
    std::memcpy(mouse, savedMouse, sizeof savedMouse);
}

// FESliderWidget::SetSliderObjects (5275B0): random widget bytes, a synthetic backing box, both precisions.
static void CheckSliderObjects(int iters)
{
    auto orig = ORIG<void(THISCALL *)(FESliderWidget *, FEObject *, FEObject *, FEObject *, FEObject *, FEObject *, FEObject *)>(0x5275B0);
    static FESliderWidget A, B;
    alignas(8) static unsigned char om[sizeof(FEObject)];
    static float od[20];
    auto *o = reinterpret_cast<FEObject *>(om);
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            A.mSlider.mMargin = RefCoord() / 7.0f;
            std::memcpy(&B, &A, sizeof A);
            std::memset(om, 0, sizeof om);
            o->mpData = od; o->mu32Type = rng() % 3 ? 1 : 3;
            for (float &f : od) f = RefCoord();
            FEObject *p[5];
            for (auto &q : p) q = reinterpret_cast<FEObject *>(static_cast<uintptr_t>(rng()));
            orig(&A, o, p[0], p[1], p[2], p[3], p[4]);
            B.SetSliderObjects(o, p[0], p[1], p[2], p[3], p[4]);
            EXPECT(std::memcmp(&A, &B, sizeof A) == 0, "FESliderWidget::SetSliderObjects pc=%x", pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// MenuScreen members that run without cFEng: SetAlternatingText (synthetic objects without scripts) and CheckKeyboard
// on paths that don't reach the text input object (input handle 0, no finish message).
static void CheckMenuScreenMembers(int iters)
{
    auto oAlt = ORIG<void(THISCALL *)(MenuScreen *, FEObject *, FEObject *, float, float)>(0x51DD90);
    auto oKey = ORIG<uint8_t(THISCALL *)(MenuScreen *, uint32_t)>(0x54E5B0);
    static MenuScreen A, B;
    alignas(8) static unsigned char om[2][sizeof(FEObject)];
    uint8_t &flag = *reinterpret_cast<uint8_t *>(0x83A9C0);
    uint32_t &handle = *reinterpret_cast<uint32_t *>(0x83A9C4);
    const uint8_t savedFlag = flag;
    const uint32_t savedHandle = handle;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            std::memset(om, 0, sizeof om);
            FEObject *o[2] = {reinterpret_cast<FEObject *>(om[0]), reinterpret_cast<FEObject *>(om[1])};
            if (rng() % 2) {
                A.mpAltCurrent = rng() % 3 ? nullptr : o[0];
                std::memcpy(&B, &A, sizeof A);
                const float t1 = RefCoord() / 50.0f, t2 = rng() % 3 ? RefCoord() / 50.0f : (rng() % 2 ? 0.00005f : std::nanf(""));
                FEObject *a = rng() % 4 ? o[0] : nullptr, *b = rng() % 4 ? o[1] : nullptr;
                oAlt(&A, a, b, t1, t2);
                B.SetAlternatingText(a, b, t1, t2);
                EXPECT(std::memcmp(&A, &B, sizeof A) == 0, "MenuScreen::SetAlternatingText pc=%x", pc);
            } else {
                A.mpTextInput = rng() % 3 ? reinterpret_cast<void *>(om[0]) : nullptr;
                A.mTextInputState = static_cast<uint8_t>(rng() % 4);
                std::memcpy(&B, &A, sizeof A);
                const uint8_t f = static_cast<uint8_t>(rng() % 2);
                const uint32_t msg = rng() % 2 ? 0xC98356BAu : rng();
                if (msg == 0xD15C04ECu) continue;
                flag = f; handle = 0;
                const uint8_t ra = oKey(&A, msg) & 1;
                const uint8_t fa = flag;
                flag = f; handle = 0;
                const uint8_t rb = B.CheckKeyboard(msg);
                EXPECT(ra == rb && fa == flag && std::memcmp(&A, &B, sizeof A) == 0, "MenuScreen::CheckKeyboard %08x", msg);
            }
        }
    }
    _controlfp(saved, _MCW_PC);
    flag = savedFlag; handle = savedHandle;
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

// FELabelWidget / FEStatWidget constructors, Show/Hide, SetPosX/Y, Position on synthetic objects, both precisions.
static void CheckLabelStatWidgets(int iters)
{
    auto oLCtor = ORIG<FELabelWidget *(THISCALL *)(FELabelWidget *, const float *, const float *)>(0x51E090);
    auto oLPos = ORIG<void(THISCALL *)(FELabelWidget *)>(0x51E0F0);
    auto oSCtor = ORIG<FEStatWidget *(THISCALL *)(FEStatWidget *, const float *, const float *, const float *, const float *, uint32_t)>(0x51E1B0);
    auto oSShow = ORIG<void(THISCALL *)(FEStatWidget *)>(0x50D570);
    auto oSHide = ORIG<void(THISCALL *)(FEStatWidget *)>(0x50D5A0);
    auto oSX = ORIG<void(THISCALL *)(FEStatWidget *, float)>(0x52D930);
    auto oSY = ORIG<void(THISCALL *)(FEStatWidget *, float)>(0x52D990);
    auto oSPos = ORIG<void(THISCALL *)(FEStatWidget *)>(0x52D800);
    struct Arena { FEStatWidget s; FELabelWidget l; unsigned char om[3][sizeof(FEString)]; float d[3][20]; };
    static Arena A, B;
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            uint8_t *pa = reinterpret_cast<uint8_t *>(&A);
            for (size_t i = 0; i < sizeof A; ++i) pa[i] = static_cast<uint8_t>(rng());
            float v[4][2];
            for (auto &q : v) { q[0] = RefCoord(); q[1] = RefCoord(); }
            const uint32_t en = rng() % 2;
            std::memcpy(&B, &A, sizeof A);
            FEStatWidget *r1 = oSCtor(&A.s, v[0], v[1], v[2], v[3], en);
            FELabelWidget *r2 = oLCtor(&A.l, v[1], v[0]);
            FEStatWidget *r3 = B.s.Construct(v[0], v[1], v[2], v[3], en != 0);
            FELabelWidget *r4 = B.l.Construct(v[1], v[0]);
            EXPECT(r1 == &A.s && r3 == &B.s && r2 == &A.l && r4 == &B.l && std::memcmp(&A.s, &B.s, sizeof A.s) == 0 && std::memcmp(&A.l, &B.l, sizeof A.l) == 0, "label/stat ctor");
            for (int k = 0; k < 3; ++k) {
                std::memset(A.om[k], 0, sizeof A.om[k]);
                for (float &f : A.d[k]) f = RefCoord() / static_cast<float>(1 + rng() % 5);
                auto *o = reinterpret_cast<FEObject *>(A.om[k]);
                o->mpData = A.d[k];
                o->mu32Type = rng() % 2 ? 1 : 3;
                o->mu32Flags = rng() & 0x400001;
            }
            auto obj = [&](Arena &x, int k) { return reinterpret_cast<FEObject *>(x.om[k]); };
            A.s.mpObject = obj(A, 0); A.s.mpValue = obj(A, 1); A.s.base.mpLabel = rng() % 3 ? obj(A, 2) : nullptr;
            A.l.mpObject = obj(A, 1); A.l.base.mpLabel = rng() % 3 ? obj(A, 2) : nullptr;
            std::memcpy(&B, &A, sizeof A);
            B.s.mpObject = obj(B, 0); B.s.mpValue = obj(B, 1); B.s.base.mpLabel = A.s.base.mpLabel ? obj(B, 2) : nullptr;
            B.l.mpObject = obj(B, 1); B.l.base.mpLabel = A.l.base.mpLabel ? obj(B, 2) : nullptr;
            for (int k = 0; k < 3; ++k) obj(B, k)->mpData = B.d[k];
            const int op = static_cast<int>(rng() % 6);
            const float f = RefCoord();
            switch (op) {
            case 0: oSShow(&A.s); B.s.Show(); break;
            case 1: oSHide(&A.s); B.s.Hide(); break;
            case 2: oSX(&A.s, f); B.s.SetPosX(f); break;
            case 3: oSY(&A.s, f); B.s.SetPosY(f); break;
            case 4: oSPos(&A.s); B.s.Position(); break;
            default: oLPos(&A.l); B.l.Position(); break;
            }
            bool same = std::memcmp(A.d, B.d, sizeof A.d) == 0 && A.s.mValuePosX == B.s.mValuePosX && A.s.mValuePosY == B.s.mValuePosY && A.s.base.mPosX == B.s.base.mPosX && A.s.base.mPosY == B.s.base.mPosY;
            for (int k = 0; k < 3; ++k) same &= obj(A, k)->mu32Flags == obj(B, k)->mu32Flags;
            EXPECT(same, "label/stat op %d pc=%x", op, pc);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FEngFont: glyph binary search, justification offsets, text height on synthetic fonts, both precisions.
static void CheckFEngFontMeasure(int iters)
{
    auto oFind = ORIG<const void *(__cdecl *)(uint32_t, const void *, uint32_t, uint32_t)>(0x6F45B0);
    auto oX = ORIG<long double(THISCALL *)(void *, uint32_t, float)>(0x50C020);
    auto oY = ORIG<long double(THISCALL *)(void *, uint32_t, float)>(0x50C050);
    auto oH = ORIG<long double(THISCALL *)(FEngFont *, const uint16_t *, int32_t)>(0x50BF40);
    alignas(8) static uint8_t data[0x20 + 64 * 16];
    alignas(8) static uint8_t fm[sizeof(FEngFont)];
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            std::memset(data, 0, sizeof data);
            auto *fd = reinterpret_cast<FEngFontData *>(data);
            const uint32_t n = rng() % 64, size = rng() % 2 ? 16 : 12;
            fd->mu16NumGlyphs = static_cast<uint16_t>(n);
            fd->mu32Flags = size == 16 ? 0x40000 : (rng() & 0xFFFBFFFF);
            fd->mu32GlyphTableOffset = 0x20;
            uint16_t code = 0x20 + static_cast<uint16_t>(rng() % 3);
            for (uint32_t i = 0; i < n; ++i) {
                std::memcpy(data + 0x20 + i * size, &code, 2);
                code = static_cast<uint16_t>(code + 1 + (rng() % 4 == 0 ? rng() % 3 : 0));
            }
            const uint16_t key = static_cast<uint16_t>(0x1E + rng() % 90);
            EXPECT(oFind(key, data + 0x20, n, size) == FEngFindGlyph(key, data + 0x20, n, size), "FEngFindGlyph %x n %u", key, n);
            const float w = RefCoord();
            const uint32_t j = rng() & 0xFF;
            long double xa = oX(nullptr, j, w), xb = FEngFont::CalculateXOffset(j, w), ya = oY(nullptr, j, w), yb = FEngFont::CalculateYOffset(j, w);
            EXPECT(std::memcmp(&xa, &xb, 10) == 0 && std::memcmp(&ya, &yb, 10) == 0, "CalculateX/YOffset %x pc=%x", j, pc);
            std::memset(fm, 0, sizeof fm);
            auto *f = reinterpret_cast<FEngFont *>(fm);
            f->mpData = fd;
            f->mfLineHeight = RefCoord() / 7.0f;
            uint16_t text[12];
            const int len = static_cast<int>(rng() % 11);
            for (int i = 0; i < len; ++i) {
                const uint32_t r = rng() % 8;
                text[i] = r == 0 ? '\n' : r == 1 ? '^' : r == 2 ? '\r' : static_cast<uint16_t>(0x20 + rng() % 80 + (rng() % 9 == 0 ? 0x100 : 0));
            }
            text[len] = 0;
            const int32_t spacing = static_cast<int32_t>(rng() % 20) - 5;
            long double ha = oH(f, text, spacing), hb = f->GetTextHeight(text, spacing);
            EXPECT(std::memcmp(&ha, &hb, 10) == 0, "GetTextHeight len %d pc=%x: %Lg vs %Lg", len, pc, ha, hb);
        }
    }
    _controlfp(saved, _MCW_PC);
}

// FEngFont::GetLineWidth + FindCharacter / GetKerning / joy-event tokens on random synthetic fonts and strings.
static void CheckFEngLineWidth(int iters)
{
    auto oLine = ORIG<long double(THISCALL *)(FEngFont *, const uint16_t *, uint32_t, uint32_t, bool *)>(0x50BCE0);
    auto oKern = ORIG<int32_t(THISCALL *)(const FEngFontData *, const FEngGlyph *, uint32_t)>(0x6F45F0);
    auto oIs = ORIG<uint32_t(__cdecl *)(const uint16_t *, uint32_t)>(0x50BA70);
    auto oSkip = ORIG<const uint16_t *(__cdecl *)(const uint16_t *, uint32_t)>(0x50BAC0);
    alignas(8) static uint8_t data[0x200 + 96 * 16 + 0x400];
    alignas(8) static uint8_t fm[sizeof(FEngFont)];
    const unsigned int saved = _controlfp(0, 0);
    for (unsigned int pc : {static_cast<unsigned int>(_PC_53), static_cast<unsigned int>(_PC_24)}) {
        _controlfp(pc, _MCW_PC);
        for (int it = 0; it < iters; ++it) {
            for (auto &b : data) b = static_cast<uint8_t>(rng());
            auto *fd = reinterpret_cast<FEngFontData *>(data);
            const bool wide = rng() % 3 != 0;
            const uint32_t size = wide ? 16 : 12, n = 1 + rng() % 96;
            fd->mu16NumGlyphs = static_cast<uint16_t>(n);
            fd->mu32Flags = wide ? (rng() | 0x40000) : (rng() & ~0x40000u);
            fd->mu32GlyphTableOffset = 0x200;
            fd->mu32KernTableOffset = 0x200 + 96 * 16;
            uint16_t code = 0x20;
            for (uint32_t i = 0; i < n; ++i) {
                uint8_t *g = data + 0x200 + i * size;
                std::memcpy(g, &code, 2);
                g[0xB] = static_cast<uint8_t>(rng() % 4);
                const uint16_t ki = static_cast<uint16_t>(rng() % 64);
                if (wide) std::memcpy(g + 0xC, &ki, 2);
                code = static_cast<uint16_t>(code + 1 + (rng() % 6 == 0));
            }
            uint8_t *kt = data + fd->mu32KernTableOffset;
            const int32_t kc = static_cast<int32_t>(rng() % 40);
            if (!wide) std::memcpy(kt, &kc, 4);
            for (int k = 0; k < 64 * 4 + 4; ++k) {   // kerning entries: previous chars from the same small set
                uint8_t *e = kt + 4 + k * 4;
                const uint16_t pch = static_cast<uint16_t>(0x20 + rng() % 40);
                if (k < 255) std::memcpy(e, &pch, 2);
            }
            std::memset(fm, 0, sizeof fm);
            auto *f = reinterpret_cast<FEngFont *>(fm);
            f->mpData = fd;
            uint16_t text[24];
            const int len = static_cast<int>(rng() % 23);
            for (int i = 0; i < len; ++i) {
                const uint32_t r = rng() % 14;
                text[i] = r == 0 ? '\n' : r == 1 ? '$' : r == 2 ? ' ' : r == 3 ? 0xA0 : r == 4 ? '\r' : r == 5 ? 0xFF41 : static_cast<uint16_t>(0x20 + rng() % 60);
            }
            text[len] = 0;
            const uint32_t flags = rng() & 0xAA0, maxw = rng() % 3 ? 0 : (rng() % 4 ? rng() % 200 : rng());
            bool ta = false, tb = false;
            const bool withFlag = rng() % 2;
            long double a = oLine(f, text, flags, maxw, withFlag ? &ta : nullptr);
            long double b = f->GetLineWidth(text, flags, maxw, withFlag ? &tb : nullptr);
            EXPECT(std::memcmp(&a, &b, 10) == 0 && ta == tb, "GetLineWidth len %d flags %x max %u pc=%x: %Lg vs %Lg", len, flags, maxw, pc, a, b);
            {
                auto oWrap = ORIG<uint32_t(THISCALL *)(FEngFont *, const uint16_t *, uint32_t, uint32_t)>(0x50C080);
                auto oChar = ORIG<long double(THISCALL *)(FEngFont *, uint32_t, uint32_t, uint32_t)>(0x50BC10);
                const uint32_t wm = rng() % 3 ? rng() % 120 : rng();
                EXPECT(oWrap(f, text, flags, wm) == f->GetNumWordWrappedLines(text, flags, wm), "GetNumWordWrappedLines len %d max %u", len, wm);
                const uint16_t c = len ? text[rng() % len] : 'A', pv = rng() % 3 ? static_cast<uint16_t>(0x20 + rng() % 40) : 0;
                long double ca = oChar(f, c, pv, flags), cb = f->GetCharacterWidth(c, pv, flags);
                EXPECT(std::memcmp(&ca, &cb, 10) == 0, "GetCharacterWidth %x %x", c, pv);
            }
            const FEngGlyph *g = fd->FindCharacter(static_cast<uint16_t>(0x20 + rng() % 60));
            if (g) {
                const uint16_t prev = static_cast<uint16_t>(0x20 + rng() % 40);
                EXPECT(oKern(fd, g, prev) == fd->GetKerning(g, prev), "GetKerning");
            }
            EXPECT((oIs(text, flags) & 0xFF) == FEngIsJoyEventTexture(text, flags) && oSkip(text, flags) == FEngSkipJoyEventTexture(text, flags), "joy event token");
            {
                auto oW2C = ORIG<void(__cdecl *)(char *, uint32_t, const uint16_t *)>(0x4FF980);
                char ca[32], cb[32];
                std::memset(ca, 0x5A, sizeof ca);
                std::memset(cb, 0x5A, sizeof cb);
                const uint32_t sz = rng() % 4 ? rng() % 30 : 0;
                oW2C(ca, sz, text);
                FEngWideToCharString(cb, sz, text);
                EXPECT(std::memcmp(ca, cb, sizeof ca) == 0, "WideToCharString size %u", sz);
            }
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
    REFEREE_RUN(CheckScrollBar(iters));
    REFEREE_RUN(CheckSliderWidget(iters));
    REFEREE_RUN(CheckToggleWidget(iters));
    REFEREE_RUN(CheckTuningSlider(iters));
    REFEREE_RUN(CheckLocalize(iters));
    REFEREE_RUN(CheckMenuScreen(iters));
    REFEREE_RUN(CheckMenuScreenMembers(iters));
    REFEREE_RUN(CheckUIWidgetMenu(iters));
    REFEREE_RUN(CheckSliderObjects(iters));
    REFEREE_RUN(CheckCSlider(iters));
    REFEREE_RUN(CheckStageSliders(iters));
    REFEREE_RUN(CheckCFEngInterface(iters));
    REFEREE_RUN(CheckRenderMath(iters));
    REFEREE_RUN(CheckRenderObjectSpecial(iters));
    REFEREE_RUN(CheckFEngExtents(iters * 3));
    REFEREE_RUN(CheckLabelStatWidgets(iters));
    REFEREE_RUN(CheckFEngFontMeasure(iters * 3));
    REFEREE_RUN(CheckFEngLineWidth(iters * 3));
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
