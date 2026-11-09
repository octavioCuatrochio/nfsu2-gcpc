#include "hooks.h"
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace hook {

static FILE *g_log;

void log(const char *fmt, ...)
{
    if (!g_log) {
        g_log = std::fopen("nfsu2port.log", "w");
        if (!g_log) return;
    }
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(g_log, fmt, ap);
    va_end(ap);
    std::fputc('\n', g_log);
    std::fflush(g_log);
}

// NFSU2PORT_DISABLE: "all", or a comma-separated list of hex addresses to leave original (for A/B testing)
static bool disabled(uint32_t addr)
{
    const char *env = std::getenv("NFSU2PORT_DISABLE");
    if (!env || !*env) return false;
    if (!std::strcmp(env, "all")) return true;
    char buf[512];
    std::snprintf(buf, sizeof buf, "%s", env);
    for (char *tok = std::strtok(buf, ","); tok; tok = std::strtok(nullptr, ","))
        if (std::strtoul(tok, nullptr, 16) == addr) return true;
    return false;
}

// Each hook jumps to a tiny counting stub:  lock inc dword [counter] ; jmp implementation
static volatile LONG g_calls[256];
static uint8_t *g_stubs;

static const void *make_counting_stub(size_t index, const void *target)
{
    if (!g_stubs) g_stubs = static_cast<uint8_t *>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    uint8_t *p = g_stubs + index * 16;
    uint32_t counter = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_calls[index]));
    p[0] = 0xF0; p[1] = 0xFF; p[2] = 0x05;               // lock inc dword ptr [counter]
    std::memcpy(p + 3, &counter, 4);
    p[7] = 0xE9;                                          // jmp target
    int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(target) - (reinterpret_cast<uintptr_t>(p) + 12));
    std::memcpy(p + 8, &rel, 4);
    return p;
}

static DWORD WINAPI report_calls(LPVOID)
{
    LONG last[256] = {};
    for (;;) {
        Sleep(10000);
        bool changed = false;
        for (size_t i = 0; i < kReplacementCount; ++i) changed |= g_calls[i] != last[i];
        if (!changed) continue;
        log("calls so far:");
        for (size_t i = 0; i < kReplacementCount; ++i) {
            last[i] = g_calls[i];
            log("  %10ld  %06X %s", last[i], kReplacements[i].addr, kReplacements[i].name);
        }
    }
}

// Register-preserving thunk (see Replacement::preserve):
//   push ecx; push edx; push [esp+12+argBytes-4] x(argBytes/4); mov ecx,[esp+argBytes+4]; call target;
//   (cdecl: add esp,argBytes); pop edx; pop ecx; ret calleePops ? argBytes : 0
static uint8_t *g_thunks;
static size_t g_thunkUsed;

static const void *make_preserving_thunk(const void *target, uint8_t argBytes, bool calleePops)
{
    if (!g_thunks) g_thunks = static_cast<uint8_t *>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    uint8_t *start = g_thunks + g_thunkUsed, *p = start;
    *p++ = 0x51;                                   // push ecx
    *p++ = 0x52;                                   // push edx
    for (int i = 0; i < argBytes / 4; ++i) {       // re-push stack args (same displacement each time)
        *p++ = 0xFF; *p++ = 0x74; *p++ = 0x24; *p++ = static_cast<uint8_t>(12 + argBytes - 4);
    }
    *p++ = 0x8B; *p++ = 0x4C; *p++ = 0x24; *p++ = static_cast<uint8_t>(argBytes + 4);   // mov ecx,[saved ecx]
    *p++ = 0xE8;                                   // call target
    int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(target) - (reinterpret_cast<uintptr_t>(p) + 4));
    std::memcpy(p, &rel, 4); p += 4;
    if (!calleePops && argBytes) { *p++ = 0x83; *p++ = 0xC4; *p++ = argBytes; }   // add esp,argBytes
    *p++ = 0x5A;                                   // pop edx
    *p++ = 0x59;                                   // pop ecx
    if (calleePops && argBytes) { *p++ = 0xC2; *p++ = argBytes; *p++ = 0; } else *p++ = 0xC3;
    g_thunkUsed += (p - start + 15) & ~size_t(15);
    return start;
}

static bool write_jump(uint32_t from, const void *to)
{
    DWORD old;
    if (!VirtualProtect(reinterpret_cast<void *>(from), 5, PAGE_EXECUTE_READWRITE, &old)) return false;
    auto *p = reinterpret_cast<uint8_t *>(from);
    int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(to) - (from + 5));
    p[0] = 0xE9;  // jmp rel32
    std::memcpy(p + 1, &rel, 4);
    VirtualProtect(p, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 5);
    return true;
}

// Developer options (environment variables), all off by default so the game behaves as shipped.
//   NFSU2PORT_SKIP_MOVIES=1  sets the game's own SkipMovies flag (PC 0x8650A8): MovieStart then plays nothing
//                            and immediately broadcasts "movie finished" (0xC3960EB9), so every FMV screen moves on.
//   NFSU2PORT_SKIP_NIS=1     sets the game's AnimCfg_DisableRaceAnimations (PC 0x865014) and
//                            AnimCfg_DisableShopAnimations (PC 0x865018): CAnimChooser::ChooseAnim then picks no
//                            animation, so race start/end, fly-by and shop/career-start NIS cutscenes don't play.
static bool env_on(const char *name)
{
    const char *v = std::getenv(name);
    return v && *v && std::strcmp(v, "0") != 0;
}

//   NFSU2PORT_QUICKRACE=1    sets the game's dev SkipFE flag (PC 0x864FAC): GameFlowManager::LoadFrontend then calls
//                            RaceStarter::StartSkipFERace instead of loading the frontend -> boots straight into a race
//                            with the player's current ride (fresh profile: Peugeot 206). Optional overrides:
//                            NFSU2PORT_QR_LAPS, NFSU2PORT_QR_AI (AI opponents), NFSU2PORT_QR_TRACK (race id, -1 auto),
//                            NFSU2PORT_QR_TRAFFIC. Addresses: see FINDINGS.md "SkipFE".
static void set_int_from_env(const char *name, uintptr_t addr)
{
    const char *v = std::getenv(name);
    if (v && *v) {
        *reinterpret_cast<volatile int *>(addr) = std::atoi(v);
        log("dev option: %s = %d", name, std::atoi(v));
    }
}

static void apply_dev_options()
{
    if (env_on("NFSU2PORT_QUICKRACE")) {
        *reinterpret_cast<volatile int *>(0x864FAC) = 1;   // SkipFE
        set_int_from_env("NFSU2PORT_QR_LAPS", 0x7FAA20);    // SkipFENumLaps (default 2)
        set_int_from_env("NFSU2PORT_QR_AI", 0x864FB0);      // SkipFENumAICars (default 0)
        set_int_from_env("NFSU2PORT_QR_TRACK", 0x7FAA18);   // SkipFETrackNumber (default -1: auto circuit)
        set_int_from_env("NFSU2PORT_QR_TRAFFIC", 0x864FBC); // SkipFETrafficDensity (default 0)
        log("dev option: SkipFE = 1 (boot straight into a race)");
    }
    if (env_on("NFSU2PORT_SKIP_NIS")) {
        *reinterpret_cast<volatile int *>(0x865014) = 1;
        *reinterpret_cast<volatile int *>(0x865018) = 1;
        log("dev option: AnimCfg_DisableRaceAnimations = AnimCfg_DisableShopAnimations = 1 (NIS cutscenes skipped)");
    }
    if (env_on("NFSU2PORT_SKIP_MOVIES")) {
        *reinterpret_cast<volatile int *>(0x8650A8) = 1;
        log("dev option: SkipMovies = 1 (all FMVs skipped)");
    }
}

void install_all()
{
    // only patch the exe this table was written for (PC v1.2, PE timestamp 0x214D4C48)
    auto *base = reinterpret_cast<uint8_t *>(GetModuleHandleA(nullptr));
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + reinterpret_cast<IMAGE_DOS_HEADER *>(base)->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != 0x214D4C48) {
        log("unexpected SPEED2.EXE (timestamp %08lX): no hooks installed", nt->FileHeader.TimeDateStamp);
        return;
    }
    int ok = 0, skipped = 0;
    for (size_t i = 0; i < kReplacementCount; ++i) {
        const Replacement &r = kReplacements[i];
        if (disabled(r.addr)) { log("  %06X %s: disabled", r.addr, r.name); ++skipped; continue; }
        if (std::memcmp(reinterpret_cast<void *>(r.addr), r.expect, sizeof r.expect) != 0) {
            log("  %06X %s: original bytes differ, NOT patched", r.addr, r.name);
            ++skipped;
            continue;
        }
        const void *target = make_counting_stub(i, r.fn);
        if (r.preserve) target = make_preserving_thunk(target, r.argBytes, r.calleePops != 0);
        if (write_jump(r.addr, target)) { log("  %06X %s: replaced%s", r.addr, r.name, r.preserve ? " (ECX/EDX preserved)" : ""); ++ok; }
        else { log("  %06X %s: VirtualProtect failed", r.addr, r.name); ++skipped; }
    }
    log("%d function(s) replaced, %d left original", ok, skipped);
    apply_dev_options();
    if (ok) CreateThread(nullptr, 0, report_calls, nullptr, 0, nullptr);
}

}  // namespace hook
