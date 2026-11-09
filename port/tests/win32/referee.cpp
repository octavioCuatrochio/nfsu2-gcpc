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

// Pool block constructors (PC 53E710/53E910): original and rewrite built in turn in the same buffer, compared
// word by word (the rewrite links the stub bridge, so its vtable words are mapped to the real vtables).
template <class Block>
static void CheckBlockCtor(uintptr_t orig, void (*rewrite)(void *), const char *name,
                           std::initializer_list<std::pair<const void *, uint32_t>> vtables)
{
    static uint32_t buf[0x800];
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
    CheckBlockCtor<FEMessageResponseBlock>(0x53E710, FEMessageResponseBlock_Construct, "FEMessageResponse block",
        {{&teststub::feMsgRespBlockVTable, 0x79BF50}, {&teststub::feMinListVTable, 0x79A9EC}, {&teststub::feMsgRespVTable, 0x79AA38}});
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
