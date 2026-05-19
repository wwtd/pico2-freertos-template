#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
ELF="${BUILD_DIR}/pico2_freertos_demo.elf"
INTERFACE="cmsis-dap"
SPEED=5000

OPENOCD_PORT=3333

cleanup() {
    echo "shutting down openocd..."
    kill $OPENOCD_PID 2>/dev/null || true
    wait $OPENOCD_PID 2>/dev/null || true
}
trap cleanup EXIT

echo "starting openocd (background) ..."
openocd \
    -f "interface/${INTERFACE}.cfg" \
    -f "target/rp2350.cfg" \
    -c "adapter speed ${SPEED}" \
    -c "gdb_port ${OPENOCD_PORT}" \
    -l /dev/null &
OPENOCD_PID=$!

sleep 1

if [ ! -f "$ELF" ]; then
    echo "error: ${ELF} not found -- run 'cmake -B build && cmake --build build' first"
    exit 1
fi

gdb-multiarch "$ELF" \
    -ex "target extended-remote :${OPENOCD_PORT}" \
    -ex "load" \
    -ex "monitor reset halt" \
    "${@}"
