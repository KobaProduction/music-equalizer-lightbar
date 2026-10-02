# Music Equalizer Light Bar

Multi-target embedded firmware for a music-reactive RGB light bar with BLE control.

## First hardware target

The first supported board is the PCB marked exactly:

**`Music-Light-V3-221101`**

Current board facts:

- MCU: ST17H66B, Arm Cortex-M0
- light source: 32 addressable RGB LEDs
- LED protocol: WS2812B-compatible one-wire data stream
- controls: reported functions/labels include Power, Color, Bright, Mode, and Speed; exact physical button grouping and GPIO mapping are being traced
- microphone: populated; MCU connection is being traced
- USB Type-C: populated
- battery pads: populated on the PCB; battery is absent on the inspected unit
- BLE: required project capability
- exact GPIO map: bring-up in progress

See [docs/Music-Light-V3-221101.md](docs/Music-Light-V3-221101.md).

## Build

The canonical build identity is the **board**, not the MCU. MCU selection is derived from the board definition so additional hardware revisions can be added later without renaming the project.

Requirements:

- CMake 3.24 or newer
- Ninja
- Python 3
- GNU Arm Embedded toolchain providing `arm-none-eabi-gcc`

Build the current board:

```sh
cmake --preset music-light-v3-221101
cmake --build --preset music-light-v3-221101
```

Outputs are produced under `build/music-light-v3-221101/`:

- `music-equalizer-lightbar-Music-Light-V3-221101-st17h66b.elf`
- `music-equalizer-lightbar-Music-Light-V3-221101-st17h66b.hex`
- `music-equalizer-lightbar-Music-Light-V3-221101-st17h66b.bin`
- linker map file
- firmware manifest JSON

The build also verifies the internal ST17H66B image/vector/HEX contract before succeeding.

The ST17H66B startup/linker contract is currently an experimental bring-up implementation inferred from public reverse-engineering evidence. A green CI build does **not** mean the image has been hardware-validated.

## Repository layout

```text
.
├── src/                  Firmware and target source
├── cmake/                Cross-toolchain/build support
├── docs/                 Board, MCU and build notes
├── tests/                Host-side protocol tests
├── .github/workflows/    CI and release automation
├── CMakeLists.txt
└── CMakePresets.json
```

The source tree remains deliberately shallow during bring-up.

## Current milestones

1. reproducible GCC linked image, image validation and CI artifacts;
2. preserve/read the original `Music-Light-V3-221101` flash;
3. verify UART ROM programming and the startup/linker contract on hardware;
4. integrate a legally distributable GCC BLE radio/GATT backend;
5. map and drive the 32-LED WS2812B chain;
6. map controls and microphone/ADC path;
7. add additional board/MCU targets.

## Licensing

Project-owned code is MIT-licensed.

Third-party SDK, BLE, ROM-support and reverse-engineered source is not automatically covered by that license. Such code is only imported after provenance and applicable license are reviewed.
