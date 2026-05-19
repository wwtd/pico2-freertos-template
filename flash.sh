#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
ELF="${BUILD_DIR}/pico2_freertos_demo.elf"
INTERFACE="cmsis-dap"
SPEED=5000

if [ ! -f "$ELF" ]; then
    echo "error: ${ELF} not found -- run 'cmake -B build && cmake --build build' first"
    exit 1
fi

echo "flashing ${ELF} ..."
openocd \
    -f "interface/${INTERFACE}.cfg" \
    -f "target/rp2350.cfg" \
    -c "adapter speed ${SPEED}" \
    -c "program ${ELF} verify reset exit"
