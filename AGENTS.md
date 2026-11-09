# AGENTS.md — how to work on the NFSU2 rewrite

Read `README.md` first for the folder layout. This file is the playbook: how the decompilation was built,
how to rewrite a function, how to verify it, and the traps already found. Follow it step by step.

## 0. Ground rules
- **Goal:** platform-agnostic native port of NFSU2, rewritten function by function. **Behavior** must match the
  original; byte-for-byte matching is NOT a goal.
- **The PC version is the behavior to reproduce.** The GameCube build (`Speed.elf`, full symbols) gives real
  names and types; the PC exe (`pc/SPEED2.EXE`, v1.2, no symbols) is the target. When the two differ, PC wins;
  note the difference in a comment.
- **Never guess.** Every claim about a function must come from reading code (GameCube and PC decompilation,
  disassembly) or from `knowledge/knowledge.db`. Matches below confidence ~0.8 are often wrong for small
  helper functions: verify by reading.
- **Record what you learn** in `FINDINGS.md` (durable knowledge, terse) and `knowledge.db` (`facts`, `matches` with `source='manual'`, `rewrite`). Rebuilds
  never delete curated rows.
- **Don't touch the user's game install** except copying `port/build-win32/dinput8.dll` into it for in-game
  tests (inert unless launched with `WINEDLLOVERRIDES="dinput8=n,b"`). Don't write disc-check bypasses.
- Shell is **zsh**: `$var` does not word-split (use `${=var}`), and `====` at line start is an error.
  `pgrep -f SPEED2` matches your own shell; use `pgrep -x SPEED2.EXE`.

## 0.1 Token economy (mandatory)
- Replies: terse. Results, numbers, decisions. No restating the task, no recaps of unchanged things.
- Never read whole decomp files (`gc_decomp/`, `pc_decomp/` are MBs). Use `tooling/knowledge/packet.py` or
  extract one function: `awk '$0~"^// .* @ 00519f40$"{p=1;next} p&&/^\/\/ .* @ [0-9a-f]+$/{exit} p' pc_decomp/src/<file>`
  (file from `pc_decomp/functions.tsv` or `pc_functions.decomp_file`).
- Query `knowledge.db` with exact SQL (`LIMIT`, needed columns only) instead of dumping tables.
- Disassemble narrow ranges (`--start-address/--stop-address`, pipe through `grep -E 'call|ret|ds:'`).
- Always filter tool output: Ghidra headless logs -> `grep -E 'REPORT|ERROR|<SCRIPT TAG>'`; build output ->
  `grep -E 'error|warning'`; long lists -> `head`.
- Long jobs (Ghidra pipeline, exports, full decompiles): run in background, don't poll.
- Don't re-read files you just wrote/edited; don't re-run unchanged steps.
- Batch independent commands in one call; prefer existing scripts over ad-hoc code.
- Record findings in `knowledge.db` once instead of re-deriving them next session.

## 1. Environment (already set up)
| Thing | Where |
|---|---|
| Ghidra 12.2 | `~/Desktop/ghidra_12.2_PUBLIC` (headless: `support/analyzeHeadless`) |
| Patched extensions (DWARF1, GameCube loader) | installed in `~/.config/ghidra/ghidra_12.2_PUBLIC/Extensions`; source + zips in `tooling/extensions/` |
| Windows cross compiler | `/opt/llvm-mingw` (i686-w64-mingw32-clang++) |
| GCC 2.95.3 PowerPC (same compiler as the game) | `tooling/compiler/ppc-gcc`, `ppc-g++` |
| Game (PC) under Wine | `~/.wine/drive_c/Program Files (x86)/Need for Speed Underground 2/` |
| Dolphin | `dolphin-emu` (pacman); US disc image in `~/Downloads/Need for Speed - Underground 2 (USA)/` |
| nfsu2-re docs | `ext/nfsu2-re/` |

## 2. How the decompilation was built (only needed to rebuild or improve it)
1. **GameCube project** (`NFSU2.gpr`): `tooling/rebuild.sh` — imports `Speed.elf` with the GameCube CPU spec,
   DWARF1 debug info, GCC 2.x demangler, ABI fixes (this/`__in_chrg`/struct returns/enums), class layouts,
   vptrs. `gc_decomp/` is exported with `tooling/scripts/ExportDecomp.java`.
2. **PC project** (`pc/ghidra`): imported + analyzed; `CreateMissingFunctions.java` (vtable entries, nfsu2-re
   addresses) and `FixFunctionBoundaries.java`. The state after that is kept in `pc/baseline/ghidra`.
3. **Knowledge base:** `ExportKnowledge.java` and `ExportDecompFeatures.java` on both projects into
   `knowledge/raw/{gc,pc}`, then `python3 -I tooling/knowledge/build_db.py --nfsu2-re ext/nfsu2-re`.
4. **Matching:** `python3 -I tooling/knowledge/match.py` (deterministic; ~6,300 pairs, 67% of game code).
   Signals: unique strings, call graph, call order, globals, vtables (GCC slot k+1 = MSVC slot k, MSVC
   destructor slots point to a "scalar deleting destructor" wrapper), float/int constants, a mutual-best
   scorer, address order. Nintendo SDK functions are excluded (no PC counterpart).
5. **PC enrichment:** clean rebuild with
   `rm -rf pc/ghidra && cp -r pc/baseline/ghidra pc/ghidra && tooling/pc_pipeline.sh`, then export
   `pc_decomp/` with `ExportDecomp.java`. The pipeline: names → GameCube types (layouts converted ONCE from
   GCC vptr placement to MSVC vptr-at-0) → nfsu2-re field names → fill/grow class layouts from PC code →
   repair embedded structs → validate signatures against the machine code (`ret N`, caller `add esp,N`,
   ECX use) → reset signatures that make callers worse → names again.
6. **Measure, don't eyeball:** `tooling/knowledge/decomp_metrics.py pc/baseline/pc_decomp/src pc_decomp/src`.
   If a change makes a metric worse, find out why before keeping it.

## 3. Rewriting a function or class — the loop
**3.1 Pick** something whose callees are already rewritten, bridged, or simple. Prefer whole small classes.

**3.2 Get the work packet:**
```sh
python3 -I tooling/knowledge/packet.py FEString::SetLabel      # or a GC/PC address, or --class FEString
```
It shows both decompilations, the match and its confidence, nfsu2-re notes, facts, every callee's status,
and the class layout.

**3.3 Verify the match by reading.** Check the PC function really does what the GameCube one does (same
callees, constants, field offsets). Look at how PC *callers* use it: argument count and meaning. If wrong
or missing, fix it:
```sql
DELETE FROM matches WHERE gc_addr=0x8015340C OR pc_addr=0x51DE90;
INSERT INTO matches(gc_addr,pc_addr,method,confidence,source,note)
VALUES (0x8015340C,0x51DE90,'manual',1.0,'manual','why you are sure');
```
Many small functions are **inlined on PC** (no PC function exists). Record that as a fact
(`key='pc_inlined'`); rewrite them anyway as part of their class.

**3.4 Read the PC machine code for anything the decompiler might hide:** calling convention
(`mov esi,ecx` = `__thiscall`; `ret N` = callee pops N bytes; caller `add esp,N` = `__cdecl`), return values
callers rely on (e.g. `bStrCpy` returns dst in EAX), sign vs zero extension (`movsx`), null checks. Use
`objdump -d -M intel --start-address=0x... --stop-address=0x... pc/SPEED2.EXE`. File offset of a VA:
`.text` VA−0x400000; `.rdata` VA 0x783000 = file 0x383000.

**3.5 Write the code** in `port/game/<area>/`:
- Plain C++17. No `windows.h`, no Direct3D, no console SDK, no absolute addresses.
- Anything not rewritten yet goes through `port/game/bridge/Bridge.h`: add a declaration there, implement it
  in `host/win32-hook/bridge_win32.cpp` (calls the original exe function/global by address, exact calling
  convention), and in `tests/bridge_stub.cpp`. Memory that original game code may free MUST come from
  `bridge::Alloc` / `bridge::FreeArray`.
- Structs that original code also uses (objects created by the exe) must have the **exact PC layout**: no
  virtual functions, no inheritance between layout structs (embed the base as a first member), `static_assert`
  every offset you rely on (inside `#if UINTPTR_MAX == 0xFFFFFFFFu`). Unknown parts: `uint8_t _unknownXX[n]`.
- Game text is 16-bit: use `uint16_t`, never `wchar_t`.
- Comment every function with its GC and PC address, and any PC-vs-GameCube difference.

**3.6 Hook it** in `host/win32-hook/replacements.cpp`: one line per PC function with its first 6 original
bytes (`xxd -s $((0xADDR-0x400000)) -l 6 -i pc/SPEED2.EXE`). Member functions need a `__thiscall` wrapper
(see the `THISCALL` wrappers there). Inlined-on-PC functions get no hook.

**3.6b Audit register reliance (mandatory for every new hook):** `python3 -I tooling/knowledge/audit_hooks.py`.
MSVC callers sometimes keep using ECX/EDX after a call because it knew the original left them alone; a C++ replacement
clobbers them -> crash far from the hook (first seen: GetDataOffset 5046F0, crash in caller at 5179B3). Flagged hooks get
`, 1, <stackArgBytes>, <calleePops>}` in replacements.cpp (register-preserving thunk). Check flagged call sites by eye:
the script has false positives (it stops at misaligned bytes; `or reg,-1` is a write).

**3.7 Verify — all three, in this order:**
```sh
cd port
cmake --build build-native && ctest --test-dir build-native --output-on-failure   # 1. unit tests (add some!)
cmake --build build-win32 && tests/win32/run_referee.sh                           # 2. referee
```
- **Unit tests** (`tests/test_*.cpp`, stub bridge): known vectors, edge cases (null, empty, long, high bytes).
- **Referee** (`tests/win32/referee.cpp`): runs inside a real SPEED2.EXE process before the game starts and
  compares ORIGINAL vs REWRITE on thousands of random inputs. Add a `Check...()` for every self-contained
  function (no game heap/globals). Must end with `0 mismatches`.
- **In-game** (for anything touching game state):
```sh
G="$HOME/.wine/drive_c/Program Files (x86)/Need for Speed Underground 2"
cp build-win32/dinput8.dll "$G/" && cd "$G" && rm -f nfsu2port.log
(WINEDEBUG=-all WINEDLLOVERRIDES="dinput8=n,b" timeout 100 wine SPEED2.EXE >/dev/null 2>&1 &)
# Enter skips intro screens and walks the menus; screenshot with spectacle -b -n -a -o shot.png
for i in $(seq 12); do sleep 5; W=$(xdotool search --name 'NFS Underground 2' | head -1); [ -n "$W" ] && xdotool windowactivate --sync $W key Return; done
cat nfsu2port.log      # hook install result + call counts every 10 s: your function must be called
```
  A/B: `NFSU2PORT_DISABLE=all` (or comma-separated hex addresses) runs the original code for comparison.
  Faster: `NFSU2PORT_SKIP_MOVIES=1 NFSU2PORT_SKIP_NIS=1`, and `NFSU2PORT_QUICKRACE=1` to start in a race (README).

Persistent alternative to `WINEDLLOVERRIDES`: see "Running the game with the rewritten code" in `README.md`.

**3.8 Record:**
```sql
INSERT OR REPLACE INTO rewrite(gc_addr,pc_addr,status,file,note)
VALUES (0x8014297C,0x535FE0,'hooked','port/game/frontend/FEString.cpp','unit+in-game ok');
-- status: 'rewritten' (no PC function / not hooked) | 'hooked' (live in the game) | 'verified' (referee too)
```

## 4. Traps already found (don't repeat them)
- GCC 2.95 puts the vtable pointer after the first polymorphic class's fields; MSVC at offset 0. Fields
  before the GCC vptr shift +4 on PC. Some classes are much bigger on PC (e.g. `PlayerCareerState`
  0xD34 → 0x854C): never assume GameCube layouts, check PC code.
- GameCube-only constructs: `__in_chrg` (destructor flag), r3 struct-return pointer, floats in FPRs. Not
  present on PC.
- GameCube loaders are 5-10x bigger than PC ones (they byte-swap data): size ratio is not a match signal there.
- MSVC folds identical functions (one PC address for several GameCube functions) and inlines small ones.
- Ghidra's own PC parameter counts are unreliable (undercounted); `ret N` and caller cleanup are the truth.
- Crash at an address outside our code after adding hooks: suspect register reliance (3.6b) first; get the address
  with `WINEDEBUG=err+seh` and look at the instructions right after the call into our hook.
- Native unit tests are 64-bit: stubs/tests/templates must use the structs (sizeof(T)), never hard-coded 32-bit
  offsets/sizes; pin PC sizes with static_asserts inside `#if UINTPTR_MAX == 0xFFFFFFFFu`.
- Verify vtable identities via the destructor that installs them (FEMinNode was first misidentified).
- Make pipeline steps idempotent; a second run of the layout conversion corrupted 46 structs once.
- The GameCube `Speed.elf` is byte-identical to retail US `main.dol`: it is the real game, not a prototype.

## 5. Status (update this section when you finish a batch)
- Done: `bStringHash` (2), `FEHashUpper`, `bCalculateCrc32`, `CopyString<short>`, `bStrCpy`, `FEWideString` (whole class),
  `FEString` (all but FEObject base, which goes through the bridge). Referee: 120k checks, 0 mismatches. In-game: menus render correctly
  (incl. accented text), call counts confirm the code runs.
- Also done: `FEMinList`/`FEMinNode` (referee-verified). Dev switches: SKIP_MOVIES, SKIP_NIS, QUICKRACE (README).
- Also done: `FEObject` ctor/dtor/scalar dtor, SetName, FindScript, FindResponse, GetDataOffset.
- Also done: FEMultiPool/FESlotPool/FESlotNode (referee-verified bit logic).
- Also done: FEResponse, FEMessageResponse::SetCount (MSVC new[] cookie layout).
- Also done: FEPool<T,N> (pooled new/delete for FEMessageResponse and FEScript), FEMessageResponse lifecycle, AddTail.
- Also done: pool blocks (ctor/dtor/scalar dtor 53E710/53EB40/53EB20, 53E910/53EC40/53EC20; referee byte-compare
  of both ctors), FEScript default ctor 511D70. In-game: race starts, no crash.
- Next in the frontend: FEScript dtor 535170 (needs key-track dtor 52B960 + FERefList purge 511D00), scalar dtor
  53EE30, copy ctor 551BE0; then FEObject copy ctor 554300 + Clone 5545D0.
  (`FEWidget`, `FELabelWidget`, `FEButtonWidget`, ...). `python3 -I tooling/knowledge/packet.py --class FEObject`.
- Progress: `SELECT status, count(*) FROM rewrite GROUP BY 1;`
