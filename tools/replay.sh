#!/usr/bin/env bash
# tools/replay.sh — replay a candump log through canmod-host
#
# Usage:
#   tools/replay.sh <log-file> [interface] [--golden <golden-ndjson>]
#
# Examples:
#   tools/replay.sh tools/captures/forscan_baseline.log vcan0
#
#   # Assert output matches a saved baseline (exits 1 + prints diff on mismatch):
#   tools/replay.sh tools/captures/forscan_baseline.log vcan0 \
#       --golden tools/captures/forscan_baseline_golden.ndjson
#
# Dependencies (apt install can-utils):
#   modprobe vcan, ip, canplayer

set -euo pipefail

LOG="${1:-}"
IFACE="${2:-vcan0}"
BINARY="$(dirname "$0")/../build/host/canmod-host"
GOLDEN=""

# Parse optional --golden flag
shift 2 2>/dev/null || true
while [[ $# -gt 0 ]]; do
    case "$1" in
        --golden)
            GOLDEN="${2:-}"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [[ -z "$LOG" ]]; then
    echo "Usage: $0 <log-file> [interface] [--golden <file>]" >&2
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

# If --golden was given, capture output to a temp file; otherwise stream to stdout
TMP_ACTUAL=""
if [[ -n "$GOLDEN" ]]; then
    TMP_ACTUAL="$(mktemp /tmp/canmod_replay_XXXXXX.ndjson)"
    echo "[replay] Starting canmod-host on $IFACE (capturing output)..."
    "$BINARY" --interface "$IFACE" > "$TMP_ACTUAL" 2>/dev/null &
else
    echo "[replay] Starting canmod-host on $IFACE..."
    "$BINARY" --interface "$IFACE" &
fi
CANMOD_PID=$!

# Give the process a moment to open the socket
sleep 0.2

echo "[replay] Replaying: $LOG"
# Map frames from log's source interface to our target interface.
# Covers both real-hw captures (can0) and vcan captures (vcan0).
canplayer -I "$LOG" "$IFACE=can0" 2>/dev/null || \
canplayer -I "$LOG" "$IFACE=$IFACE" 2>/dev/null || true

sleep 0.3  # allow last frames to be processed

echo "[replay] Replay complete. Stopping canmod-host..."
kill "$CANMOD_PID" 2>/dev/null || true
wait "$CANMOD_PID" 2>/dev/null || true

# --golden assertion mode
if [[ -n "$GOLDEN" && -n "$TMP_ACTUAL" ]]; then
    if [[ ! -f "$GOLDEN" ]]; then
        echo "[replay] Error: golden file not found: $GOLDEN" >&2
        rm -f "$TMP_ACTUAL"
        exit 1
    fi

    # Strip "ts":<number> fields before comparing (timestamps differ between runs)
    TMP_STRIPPED="$(mktemp /tmp/canmod_replay_stripped_XXXXXX.ndjson)"
    TMP_GOLDEN_STRIPPED="$(mktemp /tmp/canmod_golden_stripped_XXXXXX.ndjson)"
    sed 's/"ts":[0-9]*,//g' "$TMP_ACTUAL"  > "$TMP_STRIPPED"
    sed 's/"ts":[0-9]*,//g' "$GOLDEN"       > "$TMP_GOLDEN_STRIPPED"

    echo "[replay] Comparing output against golden file: $GOLDEN"
    if diff -u "$TMP_GOLDEN_STRIPPED" "$TMP_STRIPPED"; then
        echo "[replay] PASS: output matches golden file."
        RC=0
    else
        echo "[replay] FAIL: output differs from golden file."
        echo "         To update golden: cp $TMP_ACTUAL $GOLDEN"
        RC=1
    fi

    rm -f "$TMP_ACTUAL" "$TMP_STRIPPED" "$TMP_GOLDEN_STRIPPED"
    exit "$RC"
fi

echo "[replay] Done."
