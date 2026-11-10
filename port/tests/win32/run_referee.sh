#!/bin/sh
# Runs the differential referee: original SPEED2.EXE functions vs rewrites, inside real SPEED2.EXE processes.
# Uses temporary copies of the exe; the game installation is never touched.
# Usage: tests/win32/run_referee.sh [iterations]      (build first: cmake --build build-win32)
#   default 2000 iterations; full run before milestones: run_referee.sh 20000
#   REFEREE_ONLY=ListBox,Reader tests/win32/run_referee.sh   runs only checks whose call contains a word
#   REFEREE_JOBS=n   parallel shards (default: CPUs, at most 8; 1 = single process). Each shard runs the checks with
#                    index % n == k (REFEREE_SHARD=k/n); every check reseeds its rng from its name, so results do not
#                    depend on the sharding. Per-check progress: referee.progress in each shard's temp dir.
set -e
PORT=$(cd "$(dirname "$0")/../.." && pwd)
JOBS=${REFEREE_JOBS:-$(nproc 2>/dev/null || echo 1)}
[ "$JOBS" -gt 8 ] && JOBS=8
[ -n "$REFEREE_ONLY" ] && [ -z "$REFEREE_JOBS" ] && JOBS=1
DIRS=""
k=0
while [ "$k" -lt "$JOBS" ]; do
    DIR=$(mktemp -d)
    DIRS="$DIRS $DIR"
    cp "$PORT/../pc/SPEED2.EXE" "$PORT/build-win32/referee/dinput8.dll" "$DIR/"
    (cd "$DIR" && REFEREE_SHARD="$k/$JOBS" REFEREE_ITERATIONS=${1:-2000} REFEREE_ONLY="${REFEREE_ONLY:-}" WINEDEBUG=-all \
        WINEDLLOVERRIDES="dinput8=n,b" timeout 600 wine SPEED2.EXE >/dev/null 2>&1; echo $? > rc) &
    k=$((k + 1))
done
trap 'rm -rf $DIRS' EXIT
wait
rc=0
checks=0
fails=0
for DIR in $DIRS; do
    if [ ! -s "$DIR/referee.log" ]; then
        echo "referee: shard $DIR: no log (exit $(cat "$DIR/rc" 2>/dev/null)); last progress:"
        tail -2 "$DIR/referee.progress" 2>/dev/null
        rc=2
        continue
    fi
    grep -v '^referee: [0-9]* checks\|^referee: timing\|^  .* ms ' "$DIR/referee.log" || true
    line=$(grep '^referee: [0-9]* checks' "$DIR/referee.log" || echo "referee: 0 checks, 1 mismatches")
    c=$(echo "$line" | sed 's/referee: \([0-9]*\) checks, \([0-9]*\) mismatches.*/\1/')
    f=$(echo "$line" | sed 's/referee: \([0-9]*\) checks, \([0-9]*\) mismatches.*/\2/')
    checks=$((checks + c))
    fails=$((fails + f))
done
echo "referee: slowest checks:"
cat $(for DIR in $DIRS; do echo "$DIR/referee.progress"; done) 2>/dev/null | grep '^done' | sort -k2 -n -r | head -5 | sed 's/^done/ /'
if [ "$fails" -ne 0 ]; then status=FAILED; [ "$rc" -eq 0 ] && rc=1; else status=ok; fi
[ "$rc" -eq 2 ] && status=FAILED
echo "referee: $checks checks, $fails mismatches -> $status ($JOBS shards)"
exit $rc
