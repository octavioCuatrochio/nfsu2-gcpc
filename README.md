# NFSU2 — behavior-equivalent rewrite

**Working on this? Read `AGENTS.md`** — the step-by-step playbook (rewrite loop, verification, known traps).
Verified knowledge (addresses, ABI, layouts, game internals): `FINDINGS.md`.
Agents: follow its *Token economy* rules (terse replies, never read whole decomp files, filter all output).

Goal: a platform-agnostic native port of Need for Speed: Underground 2, rewritten function by function
from the decompiled game. Behavior must match; byte-for-byte matching is not a goal.

## Inputs
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
