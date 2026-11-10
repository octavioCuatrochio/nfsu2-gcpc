# AGENTS.md — how to work on the NFSU2 rewrite

Read `README.md` for the folder layout. Progress lives in `knowledge.db` (`rewrite` table); durable knowledge and
the current "next" targets live in `FINDINGS.md`.

## Rules
- **Goal:** platform-agnostic native port, rewritten function by function. Match the **PC** (`pc/SPEED2.EXE` v1.2)
  **behavior**; byte-for-byte is not a goal. GameCube `Speed.elf` (= retail US `main.dol`, full symbols) gives names
  and types. When they differ, PC wins: comment the difference.
- **Never guess.** Every claim comes from reading code (both decomps, disassembly) or `knowledge/knowledge.db`.
  Matches below ~0.8 are often wrong for small functions: verify by reading.
- **Record** durable knowledge in `FINDINGS.md` (terse) and `knowledge.db` (`facts`, `matches` with
  `source='manual'`, `rewrite`). Rebuilds never delete curated rows.
- **Network code is not ported.** Online / LAN / EA Messenger functions (game mode flags 8 / 0x200, `MU_*`, `UI_OL_*`,
  `OO*`, lobby, password, chat) are never rewritten: callers keep the online branch but reach it through a bridge stub
  (hooked build: the original function; native: no-op / default). Don't spend effort decompiling them.
- **Game install:** only copy `port/build-win32/dinput8.dll` into it. No disc-check bypasses.
- Shell is **zsh**: `$var` doesn't word-split (`${=var}`); a line starting with `====` is an error;
  use `pgrep -x SPEED2.EXE` (not `-f`).
- **Token economy:** terse replies (results, numbers, decisions). Never read whole decomp files: use
  `tooling/knowledge/packet.py` or extract one function
  (`awk '$0~"^// .* @ 00519f40$"{p=1;next} p&&/^\/\/ .* @ [0-9a-f]+$/{exit} p' pc_decomp/src/<file>`, file from
  `pc_functions.decomp_file`). Exact SQL with `LIMIT`. Narrow disassembly ranges. Filter output
  (`grep -E 'error|warning'`, `head`). Long jobs in background, don't poll. Don't re-read what you just wrote.

## Environment
Ghidra 12.2 `~/Desktop/ghidra_12.2_PUBLIC` (patched DWARF1 + GameCube loader extensions installed; patches in
`tooling/extensions/patches/`). Cross compiler `/opt/llvm-mingw` (i686). GCC 2.95.3 PPC `tooling/compiler/`.
Game under Wine: `~/.wine/drive_c/Program Files (x86)/Need for Speed Underground 2/`. Dolphin + US disc image in
`~/Downloads/`. nfsu2-re docs `ext/nfsu2-re/`.

## Rebuilding the decompilation (rarely needed)
GC: `tooling/rebuild.sh`, export `gc_decomp/` with `tooling/scripts/ExportDecomp.java`. PC: clean rebuild
`rm -rf pc/ghidra && cp -r pc/baseline/ghidra pc/ghidra && tooling/pc_pipeline.sh`, then export `pc_decomp/`.
Knowledge: `ExportKnowledge.java` + `ExportDecompFeatures.java` on both, then
`python3 -I tooling/knowledge/build_db.py --nfsu2-re ext/nfsu2-re` and `python3 -I tooling/knowledge/match.py`.
Measure changes with `tooling/knowledge/decomp_metrics.py pc/baseline/pc_decomp/src pc_decomp/src`; keep steps idempotent.

## The loop (per file / class)
Work in **whole units**: pick a class or source group (e.g. cFEngRender + FERenderObject + FERenderEPoly) and
write **every** function of it into its port file first (steps 2-6 for all of them), then test the unit as a
whole (step 7) and record/commit once. Don't scatter single functions across many classes.
1. **Pick** something whose callees are rewritten, bridged or simple. Prefer whole small classes.
2. **Packet:** `python3 -I tooling/knowledge/packet.py <Name|GC/PC addr|--class C>`.
3. **Verify the match by reading** (callees, constants, offsets, how PC callers use it). Fix wrong/missing ones:
   `DELETE FROM matches WHERE gc_addr=.. OR pc_addr=..; INSERT INTO matches(gc_addr,pc_addr,method,confidence,source,note) VALUES (..,..,'manual',1.0,'manual','why');`
   No match? Find it through PC callers/callees. Inlined on PC: record a fact, rewrite anyway, no hook.
4. **Read the machine code** (`objdump -d -M intel --start-address=.. --stop-address=.. pc/SPEED2.EXE`):
   `mov esi,ecx` = thiscall; `ret N` = callee pops; caller `add esp,N` = cdecl; EAX return values callers use;
   `movsx` vs `movzx`; null checks; bool args (use the low byte). Ghidra's branch structure can be wrong: trust asm.
   File offset of a VA: `.text` VA−0x400000; `.rdata` 0x783000 → 0x383000.
5. **Write** C++17 in `port/game/<area>/`: no `windows.h`/D3D/absolute addresses. Not-yet-rewritten things go
   through `port/game/bridge/Bridge.h` (+ `host/win32-hook/bridge_win32.cpp` by address, + `tests/bridge_stub.cpp`).
   Memory the exe may free: `bridge::Alloc`/`bridge::FreeArray`. Shared structs: exact PC layout, no virtuals or
   inheritance (embed the base), `static_assert` offsets inside `#if UINTPTR_MAX == 0xFFFFFFFFu`, unknowns as
   `_unknownXX`. Text is `uint16_t`. Comment GC + PC addresses and PC/GC differences.
6. **Hook** in `host/win32-hook/replacements.cpp` with the first 6 bytes (`xxd -s $((0xADDR-0x400000)) -l 6 -i pc/SPEED2.EXE`);
   member functions via `THISCALL` wrappers. Then **always** `python3 -I tooling/knowledge/audit_hooks.py`: flagged hooks
   get `, 1, <stackArgBytes>, <calleePops>}` (register-preserving thunk); check call sites by eye (false positives:
   `push ecx` used to reserve a stack slot, misaligned bytes, `or reg,-1`).
7. **Verify, in order** (from `port/`):
   - `cmake --build build-native && ctest --test-dir build-native --output-on-failure` — add unit tests (edge cases).
   - `cmake --build build-win32 && tests/win32/run_referee.sh` — add a `Check...()` comparing ORIGINAL vs REWRITE for
     every self-contained function (no game heap/CRT: runs in DllMain); synthetic stack objects are fine. Must say `0 mismatches`.
   - In-game (anything touching game state) — race starts ~25 s after launch; screenshot then:
     ```sh
     G="$HOME/.wine/drive_c/Program Files (x86)/Need for Speed Underground 2"
     cp build-win32/dinput8.dll "$G/" && cd "$G" && rm -f nfsu2port.log
     (WINEDEBUG=-all WINEDLLOVERRIDES="dinput8=n,b" NFSU2PORT_SKIP_MOVIES=1 NFSU2PORT_SKIP_NIS=1 timeout 90 wine SPEED2.EXE >/dev/null 2>&1 &)
     for i in $(seq 25); do sleep 1; W=$(xdotool search --name 'NFS Underground 2' | head -1); [ -n "$W" ] && xdotool windowactivate --sync $W key Return; done
     sleep 6; spectacle -b -n -a -o shot.png; tac nfsu2port.log | grep -m5 <ADDR>   # call counts; must be > 0 when reachable
     ```
     A/B: `NFSU2PORT_DISABLE=all` or hex list. `NFSU2PORT_QUICKRACE=1` starts in a race (README).
8. **Record:** `INSERT OR REPLACE INTO rewrite(gc_addr,pc_addr,status,file,note) VALUES (..)` — status `rewritten`
   (not hooked/inlined) | `hooked` | `verified` (referee too). Update `FINDINGS.md` ("Next" + facts), commit.

## Traps
- GCC 2.95 puts the vptr after the first polymorphic class's fields; MSVC at 0 (fields before it shift +4 on PC).
  Some classes are much bigger on PC: never assume GC layouts.
- GC-only: `__in_chrg`, r3 struct returns, FPR floats. GC loaders are 5-10x bigger (byte-swapping).
- MSVC folds identical functions and inlines small ones; Ghidra undercounts PC params (trust `ret N`/cleanup).
- Crash outside our code after a new hook: register reliance first (`WINEDEBUG=err+seh`, read the caller after the call).
- Native tests are 64-bit: use `sizeof(T)`, never hard-coded 32-bit sizes.
- Verify vtable identities via the destructor that installs them.
- x87: MSVC keeps float intermediates in x87 registers (control-word precision: 53-bit default, 24-bit after D3D9).
  Mirror with `long double` in the original's evaluation order; round to float only where the original stores
  to memory (`fstp DWORD`). `_ftol` (75BED4) is the conversion for float→int.
- Test stubs: `bridge_stub.cpp` VirtualScalarDelete must route pooled classes to their own scalar dtor.
