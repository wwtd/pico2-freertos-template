# pico2-freertos-template

[![build](https://github.com/your-username/pico2-freertos-template/actions/workflows/build.yml/badge.svg)](https://github.com/your-username/pico2-freertos-template/actions/workflows/build.yml)

FreeRTOS SMP template for Raspberry Pi Pico 2 (RP2350, Cortex-M33 dual-core).

## Hardware

### Bill of Materials

| Item | Role |
|------|------|
| Raspberry Pi Pico 2 | Target board |
| Raspberry Pi Debug Probe | CMSIS-DAP debugger + UART adapter |
| Raspberry Pi 4B | Host development machine |
| 4x female-to-female dupont wires | SWD + UART connections |

### Wiring

```
Debug Probe          Pico 2
─────────────        ──────
D port (SWD):
  Pin 1 (SWDIO)  ──  SWDIO (GPIO 24, pin 12)
  Pin 2 (SWCLK)  ──  SWCLK (GPIO 25, pin 13)
  Pin 3 (GND)    ──  GND  (pin 18)

U port (UART):
  Pin 1 (TX)     ──  GP0  (UART0 TX, pin 1)
  Pin 2 (RX)     ──  GP1  (UART0 RX, pin 2)
  Pin 3 (GND)    ──  GND  (pin 3)
```

Both D and U ports plug directly into the Probe's 3-pin JST-SH connectors using the ribbon cable that ships with the Debug Probe.

### Debug Probe → Raspberry Pi 4B

Connect the Debug Probe to the Pi 4B via USB micro-B.

## Build

```bash
# Install dependencies on Pi 4B
sudo apt update
sudo apt install -y cmake gcc-arm-none-eabi build-essential openocd gdb-multiarch

# Clone with submodules
git clone --recursive https://github.com/your-username/pico2-freertos-template.git

# Build
cmake -B build -DPICO_BOARD=pico2
cmake --build build -j$(nproc)
```

Outputs in `build/`:

| File | Format |
|------|--------|
| `pico2_freertos_demo.uf2` | UF2 (drag-and-drop to Pico 2) |
| `pico2_freertos_demo.elf` | ELF (OpenOCD/GDB) |
| `pico2_freertos_demo.bin` | Raw binary |

## Flash

### OpenOCD (recommended)

```bash
./flash.sh
```

Or manually:

```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
  -c "adapter speed 5000" \
  -c "program build/pico2_freertos_demo.elf verify reset exit"
```

### UF2 bootloader

Hold the BOOTSEL button while connecting Pico 2 to the Pi via USB, then:

```bash
cp build/pico2_freertos_demo.uf2 /media/pi/RP2350/
```

## Debug

```bash
# Launch GDB session with background OpenOCD
./debug.sh

# With breakpoint
./debug.sh -ex "b vGpioTask" -ex "c"

# Check which task runs on each core
./debug.sh -ex "thread 1" -ex "bt" -ex "thread 2" -ex "bt"
```

### VSCode

Install the **Cortex-Debug** extension and use the provided `.vscode/launch.json` configuration.

### UART monitor

```bash
screen /dev/ttyACM0 115200
```

## Project structure

```
├── CMakeLists.txt              # Build system
├── config/
│   └── FreeRTOSConfig.h        # FreeRTOS SMP configuration
├── main.c                      # Dual-core demo application
├── FreeRTOS-Kernel/            # Raspberry Pi FreeRTOS SMP fork
├── flash.sh                    # One-command OpenOCD flashing
├── debug.sh                    # One-command GDB debug session
└── .github/workflows/build.yml # CI pipeline
```

## License

MIT
