#!/bin/sh
# Runs the differential referee: original SPEED2.EXE functions vs rewrites, inside a real SPEED2.EXE process.
# Uses a temporary copy of the exe; the game installation is never touched.
# Usage: tests/win32/run_referee.sh [iterations]      (build first: cmake --build build-win32)
#   default 2000 iterations (~25 s); full run before milestones: run_referee.sh 20000 (~4 min)
#   REFEREE_ONLY=ListBox,Reader tests/win32/run_referee.sh   runs only checks whose call contains a word
set -e
PORT=$(cd "$(dirname "$0")/../.." && pwd)
DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
cp "$PORT/../pc/SPEED2.EXE" "$PORT/build-win32/referee/dinput8.dll" "$DIR/"
cd "$DIR"
rc=0
REFEREE_ITERATIONS=${1:-2000} REFEREE_ONLY="${REFEREE_ONLY:-}" WINEDEBUG=-all WINEDLLOVERRIDES="dinput8=n,b" timeout 600 wine SPEED2.EXE >/dev/null 2>&1 || rc=$?
cat referee.log 2>/dev/null || { echo "referee: no log (exit $rc)"; exit 2; }
exit $rc
