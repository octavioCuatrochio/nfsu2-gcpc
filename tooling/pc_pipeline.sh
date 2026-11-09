#!/bin/sh
# Rebuilds the knowledge-enriched PC Ghidra project from a project that has only been imported+analyzed
# (plus CreateMissingFunctions/FixFunctionBoundaries) — i.e. everything after matching. Re-runnable.
#   1. names/comments from knowledge.db         4. grow classes PC code overflows, refill those
#   2. GameCube types + signatures (MSVC conv.)   5. repair embedded structs
#   3. nfsu2-re field names; fill class layouts   6. validate signatures (ret N, ECX)   7. names again
# Usage: tooling/pc_pipeline.sh [project-dir]   (default: pc/ghidra)
# Clean rebuild (recommended after improving matches/facts):
#   rm -rf pc/ghidra && cp -r pc/baseline/ghidra pc/ghidra && tooling/pc_pipeline.sh
#   then re-export: analyzeHeadless pc/ghidra NFSU2_PC -process SPEED2.EXE -noanalysis -readOnly \
#     -scriptPath tooling/scripts -postScript ExportDecomp.java $PWD/pc_decomp/src
# pc/baseline/ holds the project and decompilation from before any type transfer (metrics reference:
#   tooling/knowledge/decomp_metrics.py pc/baseline/pc_decomp/src pc_decomp/src)
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
PROJ=${1:-$ROOT/pc/ghidra}
G=${GHIDRA:-$HOME/Desktop/ghidra_12.2_PUBLIC}/support/analyzeHeadless
T=$ROOT/knowledge/raw/transfer
run() { "$G" "$PROJ" NFSU2_PC -process SPEED2.EXE -noanalysis -scriptPath "$ROOT/tooling/scripts" "$@"; }
python3 -I "$ROOT/tooling/knowledge/apply_pc.py"
python3 -I "$ROOT/tooling/knowledge/field_names.py"
sqlite3 -separator "$(printf '\t')" "$ROOT/knowledge/knowledge.db" \
  "SELECT printf('%X',pc_addr), printf('%X',gc_addr) FROM matches WHERE confidence>=0.6" > "$T/pairs.tsv"
run -postScript ApplyKnowledge.java "$ROOT/knowledge/raw/pc/apply.tsv" \
    -postScript ImportTypesAndSignatures.java "$T/gc_types.gdt" "$T/signatures.jsonl" "$T/pairs.tsv" \
    -postScript ApplyFieldKnowledge.java "$T/fields.tsv" \
    -postScript FillClassStructs.java \
    -postScript GrowClassStructs.java "$T/grown.txt" > "$T/pipeline1.log" 2>&1
run -postScript FillClassStructs.java $(cat "$T/grown.txt") \
    -postScript ApplyFieldKnowledge.java "$T/fields.tsv" \
    -postScript RepairStructs.java \
    -postScript ImportTypesAndSignatures.java "$T/gc_types.gdt" "$T/signatures.jsonl" "$T/pairs.tsv" \
    -postScript ValidateSignatures.java "$T" "$T/ecx_methods.txt" \
    -postScript ResetSignatures.java "$T/reset_signatures.txt" \
    -postScript ApplyKnowledge.java "$ROOT/knowledge/raw/pc/apply.tsv" > "$T/pipeline2.log" 2>&1
grep -hE 'APPLY functions|TRANSFER|FIELDS|FILL extended|GROW |REPAIR|VALIDATE|RESET' "$T/pipeline1.log" "$T/pipeline2.log" | sed 's/ (GhidraScript)//; s/.*> //'
