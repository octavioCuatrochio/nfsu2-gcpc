# FINDINGS.md — verified knowledge (append as you learn; keep terse)
Addresses: GC = Speed.elf, PC = SPEED2.EXE v1.2. Per-function details also live in `knowledge.db` (`matches`, `facts`).

## Binaries
- `Speed.elf` == retail US `GUGE69` rev0 `main.dol` (all sections byte-identical). Build: changelist 114536, 2004-10-12.
- GC compiler: GNU C++ 2.95.3 SN BUILD v1.76 (not CodeWarrior). Unity build: `zAI.cpp`, `zFe2.cpp`, ... (each includes many .cpp).
- GC DWARF v1 covers only EA libs (snd, VP6, RCMP, realcore, memcard): ~1,150 funcs + types. Game code: symbols only.
- GC small data: r13=_SDA_BASE_ 0x80474DE0, r2=_SDA2_BASE_ 0x80484DE0. Floats mostly loaded via lis+lfs (r9/r10/r11), not r2.
- PC: MSVC, image base 0x400000, PE timestamp 0x214D4C48. .text VA 0x401000 file 0x1000; .rdata VA 0x783000 file 0x383000.
  RTTI only for 5 std classes. Imports normal (dinput8 imports only DirectInput8Create -> proxy DLL works).

## ABI differences GC (GCC 2.95 PPC) vs PC (MSVC x86)
- vptr: GCC after the first polymorphic class's fields; MSVC at 0. Fields before the GCC vptr move +4 on PC.
  Ex: FEObject GC vptr @0x8, PC @0x0; from 0x1C on offsets are equal.
- GCC vtable entry = {short delta, short index, pfn} (8 bytes), slot 0 empty; MSVC = pfn (4 bytes). GC slot k+1 = PC slot k.
- MSVC vtable dtor slot = "scalar deleting destructor": `dtor(); if (arg & 1) operator delete(this); return this;` (ret 4).
- GCC dtors take hidden `int __in_chrg`; GCC returns structs via r3 pointer; floats in FPRs. None of that on PC.
- MSVC: `this` in ECX (`__thiscall`, callee pops: `ret N`); free functions mostly `__cdecl` (caller `add esp,N`).
- MSVC folds identical functions (one PC address for many GC functions) and inlines small helpers.
- Class sizes can differ: PlayerCareerState GC 0xD34, PC 0x854C.
- GC loaders byte-swap chunk data (5-10x bigger than PC loaders).

## PC runtime
- Some callers rely on non-ABI register preservation (MSVC knew the callee): e.g. callers of GetDataOffset 5046F0 keep
  using ECX. Replacements of such functions must preserve ECX/EDX (hook thunk). Audit: tooling/knowledge/audit_hooks.py.
- Array new[] layout: GCC 2.95 cookie 8 bytes (count at array-8), MSVC 4 bytes (count at array-4); MSVC builds/destroys
  elements with __ehvec_ctor 0x75C461 / __ehvec_dtor 0x75C401 (forward / reverse).
- 0x575620 = jmp operator new (0x75C6C8), NOT zeroing (nfsu2-re calls it MEM_allocz). 0x575630 = jmp operator delete (0x75C6DA);
  used for both delete and delete[].
- 0x75C39A = `_purecall`. 0x4022C0 = shared empty function (MSVC-folded no-op virtuals).
- PC ctors have SEH frames (fs:0); irrelevant for behavior.

## bWare (EA core)
- bStringHash(s[, seed=0xFFFFFFFF]): h = h*33 + (uint8)c. PC 43DB50 / 43DB80.
- FEHashUpper(s): same but 'a'..'z' -> uppercase only (signed compare: bytes >= 0x80 unchanged); null -> 0xFFFFFFFF. PC 505450.
- bCalculateCrc32(data, len, seed): POSIX cksum (MSB-first CRC-32 0x04C11DB7, then length bytes, ~). len<=0 -> ~seed.
  Table PC 0x7868E8. PC 43BCD0.
- bStrCpy(uint16*, char*): SIGN-extends bytes (movsx), returns dst. PC 43E000.
- CopyString<short>(dst, src): null dst -> no-op; null src -> "". PC 511630.

## FEng (frontend engine, zFe2.cpp)
- Object factory FEPackageReader::CreateObject(type): GC 8013B2E0 / PC 534DF0. type -> size/class:
  1 0x60, 2 0x78 FEString, default 0x5C FEObject, 4 0xAC FEListBox, 5 0x6C FEGroup?, 6 200 FECodeListBox, 7/9/10 0x60, 0xB 0x5C FESimpleImage.
  After construction sets FEObject+0x18 = type.
- FEObject (PC 0x5C): +0 FEMinNode (vtable 0x79B8E0, next/prev 0xABADCAFE when unlinked), +0xC GUID (SysGUID++, PC 0x7F7CF8),
  +0x10 name hash, +0x14 name, +0x18 type, +0x1C flags (0x400000 dirty, 0x2 see FEString), +0x2C data (ObjDataPool), +0x30 size,
  +0x34 responses FEMinList (head +0x3C), +0x44 scripts FEMinList (head +0x4C), +0x54 current script.
  ctor 52AF20 (leaves +0x18, +0x22, +0x54 untouched), dtor 52AF90 (destructor callback 0x838474, ObjDataPool.Free
  517520 on 0x863770, free name, purge scripts then responses, ends with FEMinNode vtable), scalar dtor 534BB0.
  SetName 5176B0, FindScript 5046B0, FindResponse 5046D0, GetDataOffset 5046F0 (track 0..10 -> 0,10,1C,28,38,44,4C,54,64,74,84).
  FEScript id at +0x30. FEResponse {type, param, extra} (0xC): ctor 4FE910, ReleaseParam 517600 (= PC dtor), SetParam 517630,
  operator= 52AD80; types 0x200-0x204 except 0x203 own a heap string in param. FEMessageResponse (0x18): node, msg id +0xC,
  count +0x10, responses +0x14 (MSVC new[]); SetCount 52AE00; pooled new 54BDF0 (64 per 0x620 block). FEScript pooled new
  54BEB0 (0x6A0 block), copy ctor 551BE0, vtable 0x79AD28.
- FEPoolNode<T,N> block: FEMinNode, N entries at +0xC, free FEMinList, used count; new = first block with free entries
  (else new block at head via block ctor), pop free head; delete = AddTail to its block, delete empty block (vtable).
  FEMessageResponse: 0x18 x 64, list 0x8637A0, new 54BDF0 / delete 525B30, block ctor 53E710 (vtable 0x79BF50),
  ctor 4FE920 (vtable 0x79AA38), dtor 534B10, scalar dtor 53EE60 (destroys twice). FEScript: 0x34 x 32, list 0x839C30,
  new 54BEB0 / delete 525B90, block ctor 53E910. FEMinList::AddTail 4951B0. ctor PC 52AF20, copy ctor 554300 (src,bool),
  dtor 52AF90, Clone 5545D0, scalar dtor 534BB0.
- FEString (PC 0x78): +0x5C label char* (owned), +0x60 FEHashUpper(label), +0x64 FEWideString text, +0x6C..+0x74 copied fields.
  PC vtable 0x78F53C. default ctor 495630 (inlined on GC), copy ctor 554970, dtor 4956A0, Clone 495710, scalar dtor 495780,
  SetLabel 519F40, SetString(wide) 535F40, SetString(packed) 535FE0.
  PC SetString marks dirty unless (UseOldFEngDirtyFlag==0 && checksum unchanged && flags&2); GC marks dirty only if changed.
- FEWideString {uint16* str; uint32 capacity (chars, excl. terminator)}: ctor 51DE80, copy 537FF0, dtor 50D390, SetLength 51DE90,
  operator=(char*) 532670. Others inlined on PC. SetLength compares current TEXT length, not capacity. GetChecksum = CRC over capacity*2 bytes.
- Packed 8-bit text -> wide: if pWideCharHistogram (PC 0x8383DC) then WideCharHistogram::UnpackString (PC 50D3C0, thiscall, ret 0xC)
  else bStrCpy.
- FEMinNode (PC 0xC): vtable 0x79A9E8 (+0, scalar dtor 4FDB00; 0x79A9F0/4FE150 is another class), next +4, prev +8; unlinked = 0xABADCAFE. FEMinList (PC 0x10): vtable 0x79A9EC,
  count +4, head +8, tail +0xC. AddNode(after,node) 5053B0, RemNode 505400, Purge 511670 (deletes via node vtable[0](1)),
  vector deleting dtor 52C290. RemHead/FindNode/~FEMinList inlined on PC.
- FEMultiPool (PC 0x10, = FEMinList of FESlotPool): Alloc 52ACE0 (size 0 -> null; new pool at head), Free 517520 (block in
  no pool -> null deref, like original). ObjDataPool = global FEMultiPool at PC 0x863770.
  FESlotPool (0x20): node, FEMinList nodes +0xC (head +0x14), block size +0x1C; vtable 0x79B8DC; Alloc 517400 (first
  non-full node, else new node at head), Free 5174A0 (bool; FreeBlock inlined; empty node deleted), dtor 534AC0, scalar 534AA0.
  FESlotNode (0x18): node, u16 size +0xC, u16 used +0xE, u8 mask[4] +0x10 (bit i = byte i/8 bit i%8), buffer +0x14
  (32*size); vtable 0x79B13C; ctor 517310, dtor 5173B0, scalar 517390, AllocBlock 504400.
  FEObject embeds FEMinList at +0x34 (FEMessageResponse list) and +0x44 (FEScript list). FEObject copy ctor also needs
  FEMultiPool (ObjDataPool, Alloc 52ACE0), FEResponse::operator= 52AD80 (types 0x200-0x204 own a buffer),
  FEMessageResponse::SetCount 52AE00, FEScript copy.
- Globals: UseOldFEngDirtyFlag PC 0x8384D4; FEString::pLabelCallback PC 0x838478 (SetLabel calls callback->vtable[0](FEString*)).

## Cinematics / dev switches (all are original game globals, 0 in the shipped game)
- FMV: `SkipMovies` PC 0x8650A8 (GC sdata). MovieStart (GC 801197CC / PC 566020): if set, plays nothing and broadcasts
  0xC3960EB9 ("movie finished", via 0x55DC50 on 0x8637B0) -> movie screens advance. Only other writer: Joylog replay restore.
- NIS (engine cutscenes): CAnimChooser::ChooseAnim (GC 800464AC / PC 436840) returns no anim when
  AnimCfg_DisableRaceAnimations (PC 0x865014: race start/end/fly-by/online) or AnimCfg_DisableShopAnimations
  (PC 0x865018: shop + career start) is set. Skip paths otherwise: Player::PreRaceJoyHandler / FinishRaceJoyHandler.
- SkipFE (dev boot straight into a race): PC GameFlowManager::LoadFrontend 580E00: replay -> 4FEF60; else if SkipFE ->
  RaceStarter::StartSkipFERace 53FEB0; else frontend. MSVC split the settings: nonzero defaults in .data, zeros in .bss:
  SkipFE 864FAC, TrackNumber 7FAA18 (-1 auto), NumPlayerCars 7FAA1C (1), NumAICars 864FB0 (0), NumLaps 7FAA20 (2),
  TrackDirection 864FB4, PovType1 864FB8, RearView 7FAA24, TrafficDensity 864FBC, RaceType 7FAA28 (1), Drag 864FC0,
  Drift 864FC4, DriftTeamed 864FC8, Burnout 864FCC, ShortTrack 864FD0, Point2Point 864FD4, SpeedoMode 7FAA2C,
  Difficulty 7FAA30, JumpCam 7FAA34, ZoneCam 7FAA38, P1/P2Transmission 864FD8/864FDC, P1/P2DrivingStyle 7FAA3C/7FAA40,
  Damage 7FAA44, RollingStartSpeed 864FE0. TheRaceParameters PC 0x89E7A0.
- Car of a SkipFE race: BuildCurrentRideForPlayer (inlined on PC): handle = career car (0x863480) if game mode
  (0x850078)==1, else QuickRaceCarHandle[player] (PC 0x83AAB8+4*p); looked up in the player's FEPlayerCarDB
  (0x5B records at +0x9BD8, handle at record+8). No profile -> RX-8. Stock car handle = FEngHashString("STOCK_%s", name)
  (FEPlayerCarDB::DefaultStockCars, PC 516500) -> 206 would be "STOCK_PEUGOT" (unverified).
- Car types (CarTypeInfoArray, 46 x 0x890, PC ptr 0x8A1CCC; data in GLOBAL/GLOBALB.BUN): 0 PEUGOT (206), 6 PEUGOT106,
  21 RX8, 28 LANCEREVO8, 27 SKYLINE, 22 350Z, 31+ traffic. Name at +0, geometry path at +0x40, name hash at +0xD0.
- Joylog = the game's input record/replay system (also saves/restores debug flags) -> candidate for deterministic tests.

- FEPoolNode<T,N> blocks (PC): vtable MR 0x79BF50 / Script 0x79BF54 (1 slot: scalar dtor, frees with operator delete
  575630). Ctor: guards, entries via eh-vector-ctor, empty free list, used=0, AddTail each entry. Dtor: drains free list
  (unlinks heads, count-- even if head null), ~FEMinList, entries destroyed last->first, vtable -> FEMinNode.
- FEScript PC: vtable 0x79AD28, same 0x34 layout as GC from +0xC; +0x20 new[] of 0x38-byte tracks (dtor 52B960),
  +0x28/+0x2C new[]-owned, +0x30 id. Default ctor 511D70 leaves +0x10 uninitialized.

## Tooling facts
- Ghidra can't read DWARF v1 natively; patched dw1-emoose extension does. GC loader extension needed for Gekko (paired singles).
- Ghidra's PC param counts are undercounted; trust `ret N` / caller cleanup.
- Wine maps its own data around 0x390000-0x750000: a test exe can't map SPEED2.EXE at 0x400000 -> referee runs as dinput8.dll
  inside the real process (DllMain runs before the game's entry point; CRT not initialized there).
- Under Wine the game asks for Disc 2 unless the FOOBAR file is present in the install.
- xdotool works on the game window (XWayland); Enter skips intro and walks menus. Game language here: Spanish.
- Headless: Xvfb + Wine virtual desktop (`explorer /desktop=...`) + builtin d3d9 runs the game (logic, logs, call counts),
  but X screenshots are black (rendering offscreen). Without the virtual desktop the game crashes after a failed mode change.
- Wine prefix uses DXVK (native d3d9/dxgi).
