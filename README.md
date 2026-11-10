# NFSU2 — behavior-equivalent rewrite

**Working on this? Read `AGENTS.md`** — the step-by-step playbook (rewrite loop, verification, known traps).
Verified knowledge (addresses, ABI, layouts, game internals): `FINDINGS.md`.
Agents: follow its *Token economy* rules (terse replies, never read whole decomp files, filter all output).

## What is it
A function-by-function rewrite of *Need for Speed: Underground 2* (EA Black Box, 2004) in portable C++17,
heading for a platform-agnostic native port. The PC v1.2 executable defines the behavior; the GameCube build,
which shipped with full debug symbols, provides the original names, classes and types.

Until the port is complete, the rewritten code runs **inside the original PC game**. A `dinput8.dll` proxy
replaces each rewritten function with its new version. As a result the game stays playable at every step, and
each function can be compared against the original.

No game code, binaries or assets are in this repository. You need your own copy of the game (see *Disclaimer*).

## Progress
| | Done | Total | |
|---|---|---|---|
| Functions (GC symbols) | 1,706 | 14,666 | 11.6 % |
| — verified (referee: original vs rewrite) | 560 | | |
| — hooked into the running game | 898 | | |
| — rewritten (inlined on PC / not hooked) | 248 | | |
| Source files complete (every function done) | 207 | 1,363 | 15 % |
| GameCube ↔ PC function matches | 6,708 | 17,891 PC | |

Areas so far: `bWare` (containers, slot pools, lists), front-end (FEng renderer/objects, menus, magazines,
post-race, quick race, user profile, frontend database) and the camera. Not started: rendering back end,
physics, AI, audio, world/streaming. Live numbers: `SELECT status, COUNT(*) FROM rewrite GROUP BY 1;`.
The online, LAN and EA Messenger code is out of scope: it is never rewritten, and the native build stubs it.

## How it's made
**Goal: behavior match, not byte match.** Unlike matching decompilations, there is no requirement to
recompile to identical bytes. The goal is for the rewrite to behave the same as the PC exe: same results,
side effects, call order and float precision (x87 evaluation order is mirrored where it matters). When the
GameCube and PC builds differ, PC wins, and the difference is commented in the code.

Pipeline:
1. **Two decompilations in Ghidra.** The GameCube `Speed.elf` (DWARF1 symbols; GCC 2.95, PowerPC) and the
   PC `SPEED2.EXE` (MSVC, x86, no symbols).
2. **Matching.** `tooling/knowledge/match.py` pairs GC and PC functions using strings, call graph, call order,
   globals, vtables and constants. GC names, types and class layouts (converted to the MSVC layout) are then
   transferred onto the PC project.
3. **Knowledge base.** Everything learned goes into `knowledge/knowledge.db` and `FINDINGS.md`, always with a source.
4. **Rewrite loop, one whole class or source file at a time** (details in `AGENTS.md`):
   generate a work packet (`packet.py`) → verify the match by reading both decompilations and the x86 disassembly
   (calling convention, stack cleanup, signedness, ABI) → write C++ in `port/game/` (no Windows/D3D headers, no
   absolute addresses; code that isn't rewritten yet is reached through `game/bridge/Bridge.h`) → hook it into the exe.
5. **Verification, in order:**
   - native unit tests (Linux, 64-bit);
   - the **referee**: inside the real exe, the original and the rewrite run on the same synthetic inputs and
     their outputs are compared (must report `0 mismatches`);
   - an in-game run with call counts from `nfsu2port.log`, with A/B tests via `NFSU2PORT_DISABLE`.
6. **Record:** add a `rewrite` row (`rewritten` → `hooked` → `verified`), then commit.

## knowledge.db is in the repo
`Speed.elf`, the disc, `SPEED2.EXE` and the Ghidra decompilation exports (`gc_decomp/`, `pc_decomp/`) are
**not** distributed: they are EA's code. `knowledge/knowledge.db` (SQLite, ~22 MB) **is** included. It holds
the *information* extracted from them, but no code:
function names, addresses, signatures, class/type layouts, call graphs, referenced strings and constants,
GC ↔ PC matches, curated facts and the rewrite status. This means you can read the code in `port/` and see
what each function is, where it lives, and what it is matched to, without the symbol file. To regenerate the
decompilations yourself, supply your own `main.dol` (= `Speed.elf`, US `GUGE69` rev 0) and PC v1.2 exe, then
follow *Rebuilding* in `AGENTS.md`. Curated rows (`facts`, manual `matches`, `rewrite`) are never deleted by rebuilds.

## License
Decompilation/re-implementation projects mostly pick one of two options. Matching decomps (e.g. zeldaret's
*Ocarina of Time*) commonly use **CC0** or ship with no license at all. Engine re-implementations (OpenRCT2,
OpenMW) commonly use the **GPL**.

This project uses **CC0 1.0** for the code and tooling written here (`port/`, `tooling/` except third-party parts,
docs, `knowledge.db`'s curated content). Third-party parts keep their own licenses: the Ghidra extension
patches in `tooling/extensions/patches/` (see their upstream projects); GCC (GPL, not included, built locally);
yugecin/nfsu2-re (reference only, not included). The license covers only the original work in this repository.
It grants no rights to *Need for Speed: Underground 2*, which belongs to Electronic Arts.

## Disclaimer
This is an unofficial fan project for preservation, education and interoperability. It is not affiliated with,
endorsed by or sponsored by Electronic Arts or EA Black Box. "Need for Speed" and "Underground" are trademarks
of Electronic Arts Inc. The repository contains no game executables, assets or decompiled game code. To use it you
must own a legitimate copy of the game; the project does not and will not include disc-check bypasses or piracy
aids. The online features are not reimplemented.

## Setup (local inputs, not in the repo)
| Path | What |
|---|---|
| `Speed.elf` | GameCube build with symbols. Byte-identical to retail US `GUGE69` rev 0 `main.dol` (verified). GCC 2.95.3 (SN Systems), PowerPC. |
| `gc_disc/` | Extracted US GameCube disc (game data). |
| `pc/SPEED2.EXE` | PC v1.2 exe (MSVC, x86), copied from the Wine install. No symbols. |
| `ext/nfsu2-re/` | yugecin/nfsu2-re docs (PC reverse engineering notes; reference only, no license file). |

## Ghidra projects (Ghidra 12.2, `~/Desktop/ghidra_12.2_PUBLIC`)
| Project | What |
|---|---|
| `NFSU2.gpr` | GameCube: GameCube CPU spec, DWARF1 debug info (EA libraries), GCC 2.x demangling, inferred class layouts, ABI fixes. |
| `pc/ghidra/NFSU2_PC.gpr` | PC: GameCube names/classes, types and signatures transferred onto matched functions (MSVC-converted layouts, signatures validated against the machine code), nfsu2-re field names. |
| `pc/baseline/ghidra/` | PC project before any type transfer — the starting point for a clean rebuild. |

## Decompiled code (one `.c` per class/namespace, `types.h`, `functions.tsv` index)
| Path | What |
|---|---|
| `gc_decomp/` | GameCube decompilation, all 14,666 functions. |
| `pc_decomp/` | PC decompilation, all 17,891 functions — **the main reference for the rewrite**. |
| `pc/baseline/pc_decomp/` | PC decompilation before transfer (metrics reference). |

## Knowledge base: `knowledge/knowledge.db` (SQLite)
Everything known, with sources. Main tables:
- `gc_functions`, `pc_functions` (+ `*_calls`, `*_strings`, `*_globals`, `*_intconsts`, `*_features`); `decomp_file` points into `gc_decomp/` / `pc_decomp/`
- `matches` (GameCube ↔ PC functions, method + confidence), `global_matches`, view `v_matched`
- `re_functions`, `re_globals`, `re_structs` (nfsu2-re)
- `facts` — free-form knowledge: user notes, validation findings. **Never wiped by rebuilds.**
- `rewrite` — rewrite progress per function

Adding knowledge, e.g. a field name:
```sql
INSERT INTO facts(subject_kind, subject, key, value, source)
VALUES ('field', 'PlayerCareerState+0x7148', 'name', 'eventPile', 'user');
```

## Tooling
| Path | What |
|---|---|
| `tooling/knowledge/build_db.py` | Rebuild knowledge.db from the Ghidra exports (`--nfsu2-re ext/nfsu2-re`). |
| `tooling/knowledge/match.py` | GameCube ↔ PC function matching (strings, call graph, call order, globals, vtables, constants, scoring). Deterministic. |
| `tooling/pc_pipeline.sh` | Applies everything to the PC Ghidra project (names, types, layouts, fields, signature validation). |
| `tooling/knowledge/decomp_metrics.py` | Readability metrics for a decompilation export. |
| `tooling/knowledge/packet.py` | Work packet for rewriting a function/class: both decompilations, match, facts, callee status. |
| `tooling/scripts/` | Ghidra scripts (exports, pipeline steps, GameCube project fixes). |
| `tooling/extensions/` | Patched Ghidra extensions (DWARF1 analyzer, GameCube loader) with prebuilt zips. |
| `tooling/compiler/` | GCC 2.95.3 powerpc-eabi cross compiler (same compiler family as the game). |
| `tooling/rebuild.sh` | Rebuilds the GameCube project from `Speed.elf`. |

Refresh after new knowledge: `build_db.py` → `match.py` → clean `pc_pipeline.sh` rebuild → re-export `pc_decomp/`.

## Running the game with the rewritten code (dinput8 override)
The hook DLL is `port/build-win32/dinput8.dll`, copied next to `SPEED2.EXE`. Wine ignores a `dinput8.dll` in the
game folder unless told to prefer it ("native") over its own ("builtin"):

```sh
G="$HOME/.wine/drive_c/Program Files (x86)/Need for Speed Underground 2"
cp port/build-win32/dinput8.dll "$G/"

# per launch (nothing permanent):
cd "$G" && WINEDLLOVERRIDES="dinput8=n,b" wine SPEED2.EXE

# permanent, for SPEED2.EXE only (then any launcher / desktop shortcut runs the rewritten code):
wine reg add 'HKCU\Software\Wine\AppDefaults\SPEED2.EXE\DllOverrides' /v dinput8 /d native,builtin /f
# undo:
wine reg delete 'HKCU\Software\Wine\AppDefaults\SPEED2.EXE\DllOverrides' /v dinput8 /f
```
(`winecfg` → Applications → add SPEED2.EXE → Libraries → `dinput8` → "Native then Builtin" does the same.)

Check it worked: `nfsu2port.log` appears next to the exe, listing the replaced functions and call counts.
Run the original code with the DLL in place: `NFSU2PORT_DISABLE=all` (or comma-separated hex addresses).

Dev switches (env vars, off by default; they set the game's own debug flags):
| Variable | Effect |
|---|---|
| `NFSU2PORT_SKIP_MOVIES=1` | no FMVs (boot to title in ~15 s) |
| `NFSU2PORT_SKIP_NIS=1` | no engine cutscenes (race intro/end, shop, career start) |
| `NFSU2PORT_QUICKRACE=1` | boot straight into a race (no menus); `NFSU2PORT_QR_LAPS`, `_AI`, `_TRACK`, `_TRAFFIC` override |

Fastest path to 3D rendering: `NFSU2PORT_QUICKRACE=1 NFSU2PORT_SKIP_MOVIES=1 NFSU2PORT_SKIP_NIS=1 WINEDLLOVERRIDES="dinput8=n,b" wine SPEED2.EXE`
(race running ~5 s after launch). Headless (no window, logic only, screenshots black): `port/tools/run_headless.sh`.
To go back to the untouched game: remove the override, or delete `dinput8.dll` from the game folder.

## The port: `port/`
- `game/` — rewritten, platform-agnostic C++ (no Windows/D3D/console headers). Builds natively on Linux for tests.
- `host/win32-hook/` — `dinput8.dll` proxy that runs `game/` inside the original PC exe: each rewritten function
  replaces the original by a patched jump (checked against exe version and original bytes).
- `game/bridge/Bridge.h` — the only way rewritten code reaches not-yet-rewritten game code.
- `tests/` — native unit tests; `tests/win32/run_referee.sh` — original-vs-rewrite comparison inside the real exe.

```sh
cd port
cmake -B build-native -G Ninja && cmake --build build-native && ctest --test-dir build-native
cmake -B build-win32 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/i686-w64-mingw32.cmake && cmake --build build-win32
cp build-win32/dinput8.dll ~/.wine/drive_c/Program\ Files\ \(x86\)/Need\ for\ Speed\ Underground\ 2/
# run with the hooks (without the override the game ignores the DLL):
WINEDLLOVERRIDES="dinput8=n,b" wine SPEED2.EXE        # log: nfsu2port.log next to the exe
NFSU2PORT_DISABLE=all ...                              # A/B: hooks off  (or a list of hex addresses)
```
