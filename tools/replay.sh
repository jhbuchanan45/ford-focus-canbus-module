#!/usr/bin/env bash
# tools/replay.sh — replay a candump log through canmod-host
#
# Usage:
#   tools/replay.sh <log-file> [interface]
#
# Example:
#   tools/replay.sh tools/captures/forscan_baseline.log vcan0
#
# Dependencies (apt install can-utils):
#   modprobe vcan, ip, canplayer, cansend
#
# The script:
#   1. Creates and brings up the virtual CAN interface (default: vcan0)
#   2. Starts canmod-host in the background (output to stdout or a JSON file)
#   3. Replays the log at real-time speed
#   4. Tears down canmod-host when the replay finishes

set -euo pipefail

LOG="${1:-}"
IFACE="${2:-vcan0}"
BINARY="$(dirname "$0")/../build/host/canmod-host"

if [[ -z "$LOG" ]]; then
    echo "Usage: $0 <log-file> [interface]" >&2
    exit 1
fi

if [[ ! -f "$LOG" ]]; then
    echo "Error: log file not found: $LOG" >&2
    exit 1
fi

if [[ ! -x "$BINARY" ]]; then
    echo "Error: canmod-host not built at $BINARY" >&2
    echo "  Build it first:" >&2
    echo "    cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json \\" >&2
    echo "          -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake \\" >&2
    echo "          -B build/host . && cmake --build build/host" >&2
    exit 1
fi

# Create vcan interface if not already up
if ! ip link show "$IFACE" &>/dev/null; then
    echo "[replay] Creating $IFACE..."
    sudo modprobe vcan
    sudo ip link add dev "$IFACE" type vcan
fi

if ! ip link show "$IFACE" | grep -q "UP"; then
    echo "[replay] Bringing up $IFACE..."
    sudo ip link set up "$IFACE"
fi

echo "[replay] Starting canmod-host on $IFACE..."
"$BINARY" --interface "$IFACE" &
CANMOD_PID=$!

# Give the process a moment to open the socket
sleep 0.2

echo "[replay] Replaying: $LOG"
canplayer -I "$LOG" -l 1 vcan0=can0 2>/dev/null || true

echo "[replay] Replay complete. Stopping canmod-host..."
kill "$CANMOD_PID" 2>/dev/null || true
wait "$CANMOD_PID" 2>/dev/null || true
echo "[replay] Done."
