// Win32 hook host: patches functions of the original SPEED2.EXE (PC v1.2) to jump into rewritten code.
// This is the only Windows/x86-specific part of the port; game code never includes it.
#pragma once
#include <cstdint>
#include <cstddef>

namespace hook {

struct Replacement {
    uint32_t    addr;          // function entry in SPEED2.EXE
    const void *fn;            // rewritten implementation (same calling convention as the original)
    const char *name;          // original (GameCube) name, for the log
    uint8_t     expect[6];     // original first bytes: refuses to patch anything else
    // Callers of some functions keep using ECX/EDX after the call (MSVC knew the original left them alone).
    // preserve=1 routes the call through a thunk that saves/restores ECX and EDX. Needs the stack argument
    // bytes and whether the callee pops them (__thiscall/__stdcall) or not (__cdecl).
    // Find such functions with tooling/knowledge/audit_hooks.py.
    uint8_t     preserve = 0;
    uint8_t     argBytes = 0;
    uint8_t     calleePops = 1;
};

extern const Replacement kReplacements[];
extern const size_t kReplacementCount;

void log(const char *fmt, ...);
void install_all();

}  // namespace hook
