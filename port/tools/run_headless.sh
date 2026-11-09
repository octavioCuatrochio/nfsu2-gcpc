#!/bin/sh
# Runs the hooked game on a virtual X display (Xvfb): nothing appears on the desktop, no focus stealing.
# Usage: tools/run_headless.sh [seconds=120] [enter_presses=0] [shot_every=0] [outdir=/tmp/nfsu2-headless]
#   enter_presses: presses Enter every 5 s from 15 s on (walks menus: title -> Quick Race -> race)
#   shot_every:    screenshot every N seconds into outdir
# Env passthrough: NFSU2PORT_SKIP_MOVIES, NFSU2PORT_SKIP_NIS (default 1 here), NFSU2PORT_DISABLE.
# Uses Wine's builtin d3d9 (OpenGL, software on Xvfb) for this launch only: DXVK/Vulkan can't present to Xvfb.
# Runs inside a Wine virtual desktop: the game changes display modes, which Xvfb refuses (game then crashes).
set -e
SECS=${1:-120}; PRESSES=${2:-0}; EVERY=${3:-0}; OUT=${4:-/tmp/nfsu2-headless}
G="$HOME/.wine/drive_c/Program Files (x86)/Need for Speed Underground 2"
PORT=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$OUT"; rm -f "$OUT"/*.png
pgrep -x SPEED2.EXE >/dev/null && { echo "SPEED2.EXE already running"; exit 1; }
cp "$PORT/build-win32/dinput8.dll" "$G/"
for n in $(seq 90 99); do [ -e /tmp/.X11-unix/X$n ] || { D=:$n; break; }; done
Xvfb $D -screen 0 1366x768x24 -nolisten tcp >/dev/null 2>&1 & XPID=$!
trap 'kill $XPID 2>/dev/null' EXIT
sleep 1
cd "$G" && rm -f nfsu2port.log
DISPLAY=$D NFSU2PORT_SKIP_MOVIES=${NFSU2PORT_SKIP_MOVIES:-1} NFSU2PORT_SKIP_NIS=${NFSU2PORT_SKIP_NIS:-1} \
  WINEDEBUG=-all WINEDLLOVERRIDES="dinput8=n,b;d3d9=b;dxgi=b" timeout "$SECS" wine explorer /desktop=NFSU2,1366x768 "C:\\Program Files (x86)\\Need for Speed Underground 2\\SPEED2.EXE" >/dev/null 2>&1 &
GPID=$!
t=0
while kill -0 $GPID 2>/dev/null && [ $t -lt "$SECS" ]; do
  sleep 5; t=$((t + 5))
  if [ "$PRESSES" -gt 0 ] && [ $t -ge 15 ]; then
    W=$(DISPLAY=$D xdotool search --name 'NFS Underground 2' 2>/dev/null | head -1)
    [ -n "$W" ] && DISPLAY=$D xdotool key --window "$W" Return 2>/dev/null && PRESSES=$((PRESSES - 1))
  fi
  [ "$EVERY" -gt 0 ] && [ $((t % EVERY)) -eq 0 ] && DISPLAY=$D import -window root "$OUT/shot_$(printf %03d $t).png" 2>/dev/null
done
wait $GPID 2>/dev/null || true
grep -E 'replaced,|dev option|NOT patched|failed' nfsu2port.log
awk '/calls so far/{n=NR} {a[NR]=$0} END{if(n) for(i=n;i<=NR;i++) print a[i]}' nfsu2port.log
ls "$OUT"/*.png 2>/dev/null | wc -l | xargs echo "screenshots in $OUT:"
