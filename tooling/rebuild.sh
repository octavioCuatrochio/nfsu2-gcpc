#!/bin/sh
# Rebuilds the NFSU2 Ghidra project from Speed.elf with every fix, then exports the decompilation.
# Requires both extensions in extensions/*/dist installed into Ghidra 12.2 (see README.md).
# Output: a fresh project in ./out/ (move NFSU2.gpr + NFSU2.rep into the parent folder to replace the current one).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(dirname "$HERE")
GHIDRA=${GHIDRA:-$HOME/Desktop/ghidra_12.2_PUBLIC}
mkdir -p "$HERE/out"
"$GHIDRA/support/analyzeHeadless" "$HERE/out" NFSU2 -import "$ROOT/Speed.elf" \
  -processor "PowerPC:BE:32:Gekko_Broadway" -scriptPath "$HERE/scripts" \
  -preScript SetupGameCube.java -preScript RemoveGccMarkers.java -preScript ConfigureAnalysis.java \
  -postScript MergeDemanglerTypes.java -postScript FixEnumParams.java -postScript FixThisCall.java \
  -postScript FixDestructors.java -postScript FixStructReturns.java -postScript FillClassStructs.java \
  -postScript FixVptrs.java -postScript RepairStructs.java -postScript Stats.java -postScript Mangled.java \
  -postScript VerifyLayouts.java "$HERE/scripts/expected_layouts.json" \
  -postScript ArtCount.java -postScript DecompAll.java \
  -postScript ExportDecomp.java "$HERE/out/decomp/src" > "$HERE/out/build.log" 2>&1
grep -E "EXPORT|DALL|VERIFY|SCRIPT ERROR" "$HERE/out/build.log" || true
