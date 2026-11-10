# FINDINGS.md — verified knowledge (append as you learn; keep terse)
Addresses: GC = Speed.elf, PC = SPEED2.EXE v1.2. Per-function details also live in `knowledge.db` (`matches`, `facts`).

## Port status / next
- Progress: `SELECT status, count(*) FROM rewrite GROUP BY 1;` (frontend FEng core: strings, lists, pools, objects,
  scripts, key tracks, setters, step interpolation all rewritten; referee 191k checks, 0 mismatches).
- TARGET: the whole frontend. zFe2.cpp (2505 GC funcs) + zFe.cpp (1321). Plan:
  1. FEng core (~600 funcs in FE*/cFEng/FEngine classes; done: FEPackageReader, FEPackageData/Manager (but
     NotifySoundMessage), FEPackage (but InitializePackage), FEListBox, FECodeListBox, lists/pools/scripts).
     Left: FEngine rest (PushPackage 555860, UnloadPackage 5362E0, Update 558C00, ProcessPads 54D4B0 + ProcessMouse 54E020 + Render 536520 done;
     ctor/ReleaseAll), cFEng (~30: render glue, joy registration), widgets (FEWidget family ~110), input
     (FEJoyPad/FEMouse), FERenderObject/FEngFont/cFEngRender (render glue: bridge D3D), FEHashList/FERefList rest.
  2. Screens/state (zFe.cpp + game classes in zFe2: PlayerCareerState, CarCustomizeManager, RaceStarter...).
  Query: SELECT namespace,count(*) FROM gc_functions WHERE src_file IN ('zFe2.cpp','zFe.cpp') GROUP BY 1.
- FEngine pad input (54D4B0): +0x84 mPressedButton[19] (release goes only to the button that got the press; k=4
  release msg 0x936A6A7F), +0xD0 FEButtonMap::GetButtonFrom arg. Directions: table 0x7F7CFC {a,b,dir} diagonals
  first, hashes 0x7F7C90; repeat 320 ticks, 120 if FEngine+0x2C bit set; +0x30 = repeating bits. First fired
  direction returns. Held msgs (buttons 7, 8) carry the *pressed* pad mask (PC quirk).
- Mouse (54E020): game interface vtable +0x48 = hit test (float x, float y, FEObject*) -> AL (clicks need ==1).
  PC-only: package +0xAC/+0xAD gate package-level click msgs; hover selection skipped while a text box has focus
  (globals 0x83A9C0 byte && 0x83A9C4) unless package +0xAF; buttons with flag 0x4000000 are not hover-selectable.
- FEButtonMap (FEPackage +0x90 = {buttons, count}): GetButtonFrom 5189E0 / ComputeButtonLocation 504BB0 /
  FEMultMatrix 504B50 rewritten bit-exact (referee _PC_53 + _PC_24). FEngine +0xD0 = wrap mode (bit0 X +-640,
  bit1 Y +-480). FEObject +0x20 = render context id (u16) for gi vtable +0x2C transform (fails -> raw position).
- FEngine render (PC unnamed): Render 536520 (caller 5378C0), RenderGroup 52C5D0, RenderObject 51A970, sorter
  5114F0 (FEngine+0x110 count, +0x114 {obj, key}[1024], +0x2114 scratch[1024]), quat matrix 504A50, 4x4 mult
  5187A0 (out = b x a). Sort key = float bits of view z (PC; GC int). Render context ids: FEngine +0xD8 counter,
  FEObject +0x20. FEPackage objects head +0x4C, FEGroup children head +0x64. Data: +0xC alpha (int, 0 = skip),
  +0x10 pivot, +0x1C position, +0x28 rotation quaternion. gi vtable 0x79BCD8: +0x20 Begin, +0x24 End,
  +0x28 GenerateRenderContext, +0x2C GetContextTransform, +0x30 RenderObjectList, +0x3C GetViewTransformation.
- FETypeLib (FEngine +0x4134, 0x14 bytes) rewritten: Startup 5359D0 builds types Image 1, CBV Image 9, Anim Image
  10, Simple Image 11, String 2, Model 3, Movie 7, Effect 8, List 4, Group 5, Code List 6. Base fields Color(6)/
  Pivot(4)/Position(4)/Rotation(5)/Size(4) -> data offsets 0/16/28/40/56; image + Upper Left/Lower Right (3).
  FEngine ctor 536170 rewritten (FEngine::Construct); teardown ReleaseAll 51A640 / ~FEngine 52C430 / type+field
  node dtors rewritten but unreachable in game: exit (Alt+F4, exit code 0) never destroys cFEng.
- cFEng joy (done): table 0x7F91C8 = 21 x {event, pad mask, name, state[4]}; registered flag 0x8384B8; handler
  50C630 stays the registered address (hooked). 579410 = AddJoyHandler (DB name wrong).
- Referee: game data the rewrite reads through bridge:: must also resolve to the real address in bridge_stub.cpp
  under NFSU2_REFEREE (else ORIGINAL uses real data, REWRITE the stub's).
- FEng helpers (FEngUtil.cpp): visibility/texture/script/size/position done. Position helpers switch on type:
  boxes 1/7/9 pivot at the center (pos +- size/2), strings 2 use the FEngFont (bridged: FindFont 51BD70,
  GetTextWidth/Height 50BEC0/50BF40, X/YOffset 50C020/50C050; FEString +0x6C justify, +0x70 height arg), groups
  differ per function. 50B9C0 FEngHashStringUpper = printf-format then hash (bridged). Referee can't run string
  paths (FindFont crashes in DllMain).
- Widgets (FEWidget.cpp): PC FEWidget 0x2C (GC + 4), FEButtonWidget 0x38 (+0x2C object, +0x30 size copy). Next:
  FEngTestForIntersection 52D020 <- FEngGet2DExtents 51D9F0 <- FEngGetObjBBox 51D8A0 (string bbox, inlined
  localized fetch), then FELabelWidget / FEStatWidget. Options screens are hard to reach by key automation.
- FEngFont (FEngFont.cpp): +0x0C font data (+0x0A glyph count, +0x0C flags 0x40000 = 16-byte glyphs, +0x14 table
  offset), +0x24 line height. Done: GetTextWidth/Height, GetLineWidth, X/YOffset, FindCharacter, kerning, joy tokens.
  Glyph: +0 code, +0xB kern count, +0xC kern index (16-byte), +0xE advance (fonts in use are 16-byte). Control chars
  index up to 32 entries before the glyph table (PC reads it). FEngFont unit complete: ctor/dtor/loader/FindFont,
  NotifyTextureLoading, RenderString 51C110 + glyph quad 51BFC0 (joy tokens are skipped, not drawn, on PC). In-game
  text pixel-identical to the original (A/B diff of a settled screen: grab 3 s and 9 s after input to avoid crossfades).
- FEScrollBar (FEScrollBar.cpp, PC 0x58, bools at +0..+3; GC 0x64): done (ctor 542AC0, Update 538080, SetPos*
  52E200/52E320, arrow/backing visibility). PC helpers: 4953E0 FindImage by name, 495590 SetWidth, 525DB0 SetHeight.
  FEngSNPrintf 50C960 ignores its size. Referee: originals copy bool bytes raw -> pass 0/1 low bytes.
- FESliderWidget (FEWidget.cpp): PC 0x98 = FEStatWidget + arrows +0x4C/+0x50 + cSlider +0x54 (0x44, virtuals via
  bridge::cSlider_*). Done: ctor 51E840, sound 50D6F0, Position/Show/Hide/SetFocus/UnsetFocus. Only on options screens
  (in-game unreached); referee uses a fake cSlider vtable. cSlider (vtable 78F544) done too (Init/Update/Draw*/Toggle/SetCurValue/Highlight;
  in-game Init+DrawHorizontal run in the race). FEngFindString 537A10 done. PreviewSlider (79AE30, 0x4C) / ThreeStageSlider
  (79AE54, 0x50) done (ctors inlined in screens / TuningSlider 522AF0). Next (whole files): FEScrollBar PC mouse members (538130, 495810/495850, 52E670, 52D100), FEWidget file leftovers, then TuningSlidersMenu.
- cFEng game interface (vtable 79BCD8) slots done: 1 LoadResources, 2 UnloadResources, 7 NotifySoundMessage, 9 End-
  PackageRendering, 10 GenerateRenderContext, 12 RenderObjectList, 13 RenderObject, 15 GetViewTransformation, 18 Does-
  PointTouchObject, 20 OutputWarning (+ GetBaseName 50C250). cFEng +0x06 = rendering flag. cFEngRender: 64 contexts x 0xD0 from +0x20 (matrix +0x10, used byte +0xC0).
  Globals: g_discErrorOccured 0x864F2C, FEngPleaseRenderSinglePackage 0x8384B4, mouse 0x8763D4. Also 0, 8, 11, 17.  cFEng unit now complete (ctor/dtor,
  InitPackage (+FEPackage::InitializePackage), PackageWillUnload, Service, DrawBackground, RenderPackage/ObjectSpecial,
  MakeLoadedPackagesDirty, FEngIsPackagePushed, functors). cFEngRender (51B7F0, 51B850, 5371F0...).
- cFEngRender unit (cFEngRender.cpp: cFEngRender + FERenderObject + clipping) fully rewritten; all FE drawing goes
  through it (only FEngFont::RenderString 51C110 and eView 5C7EA0 bridged). Lessons: MSVC's unrolled loops can sum in a
  different order per copy (ClipGeneral) -> referee at _PC_24 catches it; emulate 24-bit with numpy float32 to
  brute-force the order. eMulVector 441000 and ePoly ctor 5BB1C0 (engine) are hooked too.
- FEToggleWidget (PC 0x54, vtable 0x79B3B8, 17 slots; +0x40 BlinkArrows) = FEStatWidget + arrows +0x4C/+0x50; ctor 51E230.
  FEToggleImageWidget (+0x54 image) has no PC vtable: ctor inlined into BrandScrollerThing 521E30 (vtable 0x79B6A0,
  +0x58 brand data, +0x5C index; ChooseBrandsScreen). Both rewritten + hooked, referee CheckToggleWidget 0 mismatches. Widget vtable slots: 0 dtor, 1 Act, 2 CheckMouse, 3 ?, 4 Position, 5 Show, 6 Hide, 7 Enable, 8 Disable,
  9 SetFocus, 10 UnsetFocus, 11 SetPos, 12/13 SetPosX/Y, 14 DetermineSpecialSound, 15 DetermineSoundTrigger.
  MakeFrontendSound inlined on PC: table 0x837FE0[event * 0x1B + trigger] (int8, -1 = none) -> PlayUISoundFX 458520.
  Toggle CheckMouse fetches the trigger (+0x3C) before the special sound (+0x38), GC the other way round.
- Build host (glibc too old for /opt/llvm-mingw's clang): cmake/i686-host-clang.cmake = host clang-22 + ld.lld 22
  (unpacked from the lld-22/liblld-22 .debs into ~/.local/opt/lld-22, no sudo) + llvm-mingw as sysroot; resource
  dir shim ~/.local/opt/mingw-clang-rd (clang-22 include + llvm-mingw lib/windows). MUST be -march=pentium4 (llvm-mingw's
  i686 default): plain i686 keeps floats on x87 and drops float casts / reuses unrounded regs after fst -> referee
  mismatches in slerp + FEListBox scroll. FEListBox Raw()/StoreF() now force the bit copy / rounding through memory.
- audit_hooks.py: indexes all E8 calls in one pass + one objdump sweep (2m12s -> 4.5s, same output).
- TuningSlider (PC 0xC0, vtable 0x79B7B0 = toggle's 17 slots): FEToggleWidget + hashes +0x54/+0x58, range labels +0x5C/
  +0x60, PC-only optional +0x64, type +0x68, ThreeStageSlider +0x6C, y offset +0xBC. Done + referee: ctor 522AF0,
  SetSliderObjects 530D20, Position/Show/Hide/SetFocus/UnsetFocus; Draw 53CD30, Act 564F10, CheckMouse 565050, InitValues
  530D90 hooked (native tests; tuning object *(0x83A540): 59B080 avail, 5B0BD0 min, 5B0C00 max, 5B3E40 get, 5B4670 set,
  5B2BD0/5B2BA0 stage limits, step 522560 on 0x839C40). Inlined GC FEngSendMessageToPackage = FEPackageManager::
  SendMessageToPackage. Strings: SearchForString 4FF9D0 (EDX arg; table *(0x8383D8), count *(0x8383D0)), GetLocalized-
  String inlined (fallback 0xC01A6F63). Referee calls EDX-arg originals with an inline-asm `call`.
- MenuScreen (PC 0x4C, vtable 0x79AC00: 0 dtor 51DC60, 1 MaybeOverrideActiveButtons 40D080, 2 NotificationMessage,
  3 NotifySoundMessage 570A70): ctor 551DE0 + dtor rewritten (MenuScreen.cpp). PC layout = GC +4 up to +0x24; GC +0x24
  EAM flag is global 0x838528 on PC. ScreenConstructorData +0x0C = PC help bar mask. Next: BaseNotify 55B5C0, then
  UIWidgetMenu (ctor 552780, vtable 0x795CDC, FEScrollBar +0x6C, list +0x4C, string table +0xC4..+0xF0).
- UIWidgetMenu (UIWidgetMenu.cpp, PC 0x14C): ctor, dtor, Refresh/SetInitialPositions/GetWidgetIndex/Clear/Reposition/
  Reset/Sync done (+ generic bTList::DeleteAllElements 4358E0, bList::GetNode 43BE60). Widget list nodes at FEWidget +4.
  Widget vtable slots: 0 dtor, 1 Act, 2 CheckMouse, 3 Draw, 4 Position, 5 Show, 6 Hide, 7 Enable, 8 Disable, 9 SetFocus,
  10 UnsetFocus, 11 SetPos, 12 SetPosX, 13 SetPosY, 14 DetermineSpecialSound, 15 DetermineSoundTrigger (GC index = +1).
  Scroll/ScrollWrapped and all Add*Option (+ GetCurrentFE*, FESliderWidget::SetSliderObjects 5275B0) done too. PC inlines
  IncrementStartPos in two evaluation orders (AddToggleOption: (y + row) + gap; others (row + gap) + y). UNIT COMPLETE:
  + NotificationMessage 556020, SetInitialOption, StorePrevNotification, PC-only WidgetAtPoint 51FEA0 / SelectWidget
  50DB10 / HandleMouse 52FC40 / HandleScrollBarMouse 544490; AddInputOption = AddStatOption (folded). Flags +0x144 wrap,
  +0x145 initial option set, +0x146 scroll bar, +0x147 view dirty, +0x148 input enabled, +0x149 sounds.
- Referee: originals that look up packages/objects (FEngFindPackage 52CEF0) crash in DllMain (no cFEng). bridge_stub
  VirtualScalarDelete calls slot 0 for vtables outside the exe (check fakes). Remap only known pointer fields when
  comparing two arenas (blanket "looks like a B address" remaps give rare false mismatches; TuningSlider check still does).
- MenuScreen::BaseNotify 55B5C0 (ret 0x10; rewritten, MenuScreen unit COMPLETE: + CheckKeyboard, Begin/EndTextInput,
  BaseNotifySound 51DCD0, SetAlternating* 537E60/51DD90, AlternateTheText, UpdateEAMIcons, FEngUtil pkg wrappers 4954xx): byte 0x838521 -> zero pkg masks +0x1C..+0x28;
  CheckKeyboard 54E5B0 (true: swallow); input gate (+0x11 or no pkg or pkg+0x1C == 0) && FEngine::IsInputMessage 50B2A0
  -> drop. Messages: 0x2C0E485A (5F00F0(0x89CF48) false: 4A0140(0), unload FEngine+0xE8 list non-overlays via 5379A0,
  555F10(.., "GarageMain.fng", 0xFF, 0), return), 0x35F8620B/0x9757A0C0 UpdateEAMIcons 542720, 0x9803F6E2 (data pkg
  +0x1C == 0: swallow), 0xC519BFC0 EAM button (cooldown (*0x8651AC - *0x8637C8) * [784268] > [7F942C]; 11 dialog pkgs
  pushed -> no; PlayUISound 0x4A; calling screen = this; 555EB0("UI_OLEAMessenger.fng", pkg, 0xFF, 0); freeze 838520),
  0xC519BFC3 help (mask & arg3, help hash: MakeFrontendSound(pkg,2), ShowHelpMessage 558AC0), 0xC98356BA
  AlternateTheText 537F50, 0xF764CBE7 online login (58F6D0/5847F0..., dead servers). 0x911AB364/0x0C407210/0x406415E3/
  0xC519BFC0 with arg3 == 3 && HelpBar 50CF10: swallow. Else vtable +8 NotificationMessage(msg, obj, 1 (!), arg4).
- PC 79B7B0 vtable (ctor 522AF0) is a TuningSlider-like class built on FEStatWidget, not FEToggleWidget (bad match).
- Hook log: "calls at exit:" block written on DLL_PROCESS_DETACH (shutdown-only functions).
- Referee: CRT functions (e.g. _stricmp 75DD6F) crash in DllMain -> originals that call them can't be compared.
- Referee: bridge_stub.cpp under NFSU2_REFEREE must call game-interface slots through the vtable (fake gi).
- Single instance: an earlier SPEED2.EXE still running makes a new launch exit quietly -> check pgrep before A/B.
- Referee tip: REFEREE_TRACE=1 makes CheckUpdateObject print each case before calling the original (hang hunting).
- bStrToFloat (43DEF0) returns unrounded x87 ST0 (port: long double); a nonzero fraction is rounded to float and
  bumped by +1 ulp before being added (PC quirk). Referee can switch precision with _controlfp(_PC_24/_PC_53).
- FEListBox (PC 0xAC, layout in port/game/frontend/FEListBox.h; GC offsets identical from +0x5C). ScrollSelection flags:
  2 scrolling, 4 clamp, 8/0x10 X/Y wrapped-or-short (Y wrap also sets 8), 0x20/0x40 axis pending.
- tooling/x87sym.py <exe> <start> <stop> arg0=.. : symbolic x87 executor for branch-free code; prints C++ in the
  original evaluation order with float roundings at DWORD stores (used for eMulMatrix 4013F0, bInvertMatrix 43C7C0).
  fchs must be opaque (clang folds -(a*b) into a*(-b): differs for NaN); NaN-vs-NaN results can still differ in sign
  (x87 picks by operand position, compilers commute) -> referee accepts any NaN there.
- bWare trig rewritten bit-exact (bSin/bSinF/bASin); bASin table is game data read from the exe (bridge).
- FEInterpLinear (52BDC0): wrap mode = flags&0x7F (0 clamp, 1 loop over track length, 2 ping-pong: reads a null
  "from" key in the original). t rounded to float; t==0/1 -> key + base (quaternion: key*base); no keys -> base copy.
- FEPackageCommand (PC 0x20, vtable 0x79AB64): FENode (+0xC name, +0x10 FEHashUpper) + type +0x14 (1 pop, 2 push,
  3 switch, 4/8 user transfer false/true, 0x12 modal push), mask +0x18, pkg +0x1C; queue = FEngine+0x4124 FEMinList.
  FEngine +0x4148 marker count, +0x414C 16x0x100 marker names, +0x514C ring index, +0x5150 32x{pkg hash, button GUID}.
  FEPackage +0x18 priority (GC +0x24, <0 = not pushed), +0x3C parent (GC +0x48). ProcessPackageCommands (5559C0) is
  called every frame after ProcessMessageQueue (558DFE); 0x50D510 is an empty debug-print stub (xor eax; ret).
  FENode::SetName is PC 519DE0 (was mis-matched to the FENode ctor). PC Switch/Pop/UserTransfer don't null-check pkg.
- FEPackageData (PC 0x54, vtable 0x79B15C, scalar dtor 52CC20) = GC +4 from +8: +0x1C permanent, +0x1E visible, +0x20
  screen create data, +0x24 render info, +0x4C pkg control mask, +0x50. FEPackage +0x40 = its FEPackageData. Chunk
  loaders LoaderFEngPackage 51BD30 / UnloaderFEngPackage 5425A0. 8384D0 is also GC gInsideScreenConstructor.
  PC 51B9D0 = Activate (old auto-match said FEngDebugPrintf; that is the 50D510 stub).
- FEPackage (PC 0xB0, vtable 0x79B8F8, scalar dtor 534C50 = slot 0): +0x38 filename, +0x64/+0x68 resources (count,
  array; Startup passes them to FEGameInterface vtable slot 1), +0x70 message targets new[] (cookie at -4, elem dtor
  4FF200 frees +0xC), +0x74 list (ctor vtable 0x79B878), +0x98 buffer, +0x9C list of nodes owning +0x10, +0xAC 4x1.
  FENode vtable 0x79AB0C (dtor 505350); 0x79B8FC is another FENode subclass. FEPackageReader::Load = 555720.
- FEPackageReader (PC 0x50, on FEngine::LoadPackage 555800's stack): Load 555720; chunk ids LibL, TypS, LibR, ResÌ
  (RsNm+RsRq), PkgR, Targ (+ object chunk 5554A0 still original). FindChild scans up to AND including the chunk at
  the parent's end. FEngine +0xFC = library FEMinList (FEPackage +0x18 = refcount for libraries), +0x10C game
  interface (slot 0 GetPackageData(name,&buf,&free), 1 LoadResources, 3 library loaded). FEMessageTargets +4 = capacity.
- FEObject factory (CreateObject 534DF0) types: 0 none, 1 image 0x60 (532640), 2 FEString 0x78, 3/8/>11 FEObject 0x5C,
  4 list box 0xAC (5349C0), 5 group 0x6C (52B080), 6 code list box 0xC8 (52B9E0, +0x5C = game interface), 7 0x60
  (52B0B0), 9 0x60 (534B90), 10 0x60 (534DB0), 11 0x5C (52B480). Image types 1/9/10 take tag "If" -> +0x5C.
- FEPackageReader done (all tags).
- FECodeListBox (0xC8, vtable 0x79B904) data side done: Initialize, totals (offset = clamped selection, or centered
  with flag 8), string pool, range setters (virtual -> visible via FEPositiveMod 504790, which can yield index n).
  Behaviour done too (FillAllCells, Update, CheckMovement, MakeMove, ScrollSelection axis/xy, DefaultSelectCallback):
  referee-verified with a fake game interface (vtable +0x18 messages "CLB AT MIN/MAX", +0x4C fill cell).
  Quirk: backward row shift refills every column with the column offset itself. All done:
  done (Clone 554830, CopyProperties 5352D0); helpers inlined on PC. Not on the auto-race path.
- FEListBox (vtable 0x79BAE4) done. Cells 0x30 (FEListBoxCell, +0x90). SetNumColumns/Rows copy min(old,new) BYTES (GC
  too); Terminate frees columns/rows first so CleanupCells frees nothing (leak, original). Not reached in menus/race.
  Script chunk: one script per Scrp; "Kd" first key overwrites the base key; "Ti" inserts tracks via the type library.
- Only one SPEED2.EXE can run: never run the referee and the in-game test at the same time.
- Referee: default 2000 iterations (~13 s, ~770k checks); full `run_referee.sh 20000` (~4 min) before milestones;
  `REFEREE_ONLY=ListBox,Reader` runs matching checks only (~1 s). Random chunk tests must keep sizes consistent.
- FEngine: Update 558C00, PushPackage 555860, UnloadPackage 5362E0 rewritten. Layout: +2 mouse enabled, +8 FEJoyPad*
  (0x88 each, count +0xD4), +0xC FEMouse, +0x34/+0x38[19] hold-time releases. FEGameInterface (+0x10C) slots: see
  knowledge facts. Lesson: a `push` right before a vtable call may belong to the NEXT call (pad bits slot takes 1 arg).
- Debug crashes: `pkill -f <pattern>` kills my own shell when the command line contains the pattern; use -x.
- Hook host: counters/stubs sized by kMaxHooks (hooks.cpp); >256 hooks overflowed the old 4 KB stub page (DllMain AV).
- Not reachable by driving menus: FEObject copy ctor/Clone (unit tests only).

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

- FERefList PC: vtable 0x79AA30, byte mbIsReference @+4 (borrowed list: head points at another list; never
  purged), head +8, tail +C, no count (RemNode 5055C0 doesn't count). FEKeyTrack PC 0x38: FEKeyNode @+8, FERefList @+0x28.

- FEScript PC layout: +1C numTracks, +20 tracks (new[] 0x38 + cookie), +24 event list {count, 0xC-byte events},
  +2C owned name, +30 FEHashUpper(name). Copy ctor 551BE0(src, bool reference): reference=true shares each track's
  key list (FERefList points at the source list object) and also copies the base key's next/prev; false clones
  keys (FEKeyTrack::operator= 54D350, which purges its own list unconditionally first). FEKeyNode 0x20, pool 0x839BD0.

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
