# Music Equalizer Light Bar

Firmware project for a music-reactive RGB light bar with BLE control and support for multiple MCU targets.

## Status

The repository is in initial hardware bring-up.

- Build system: CMake
- Toolchain: GNU Arm Embedded
- First target: ST17H66B (Arm Cortex-M0)
- BLE: required; GCC-compatible stack integration is the next firmware milestone
- Hardware validation on the target light-bar PCB: pending

The current build is deliberately a **compiler/bootstrap target**. It proves the repository, cross-toolchain, target selection, and CI path, but it does not yet link a flashable ST17H66B image.

## Layout

```text
.
├── src/                  Project source
├── cmake/                Cross-toolchain/build support
├── docs/                 Hardware and bring-up notes
├── .github/workflows/    CI
├── CMakeLists.txt
└── CMakePresets.json
```

The source tree will be decomposed further only when real target/application boundaries require it.

## Build

Requirements:

- CMake 3.24 or newer
- Ninja
- GNU Arm Embedded toolchain providing `arm-none-eabi-gcc`

Host-shell build:

```sh
cmake --preset st17h66b
cmake --build --preset st17h66b
```

Build output is written under `build/st17h66b/`.

## ST17H66B bring-up

Current technical notes and source references are maintained in [docs/ST17H66B.md](docs/ST17H66B.md).

The first hardware milestone is:

1. preserve/read the original flash;
2. establish a reproducible GCC-linked firmware image;
3. verify UART ROM programming on the real board;
4. bring up BLE;
5. bring up the addressable LED data path.

## Licensing

Project-owned code in this repository is covered by the repository MIT license.

Third-party SDK, BLE, ROM-support, linker, and reverse-engineered sources are not automatically covered by that license. They will only be imported after their provenance and applicable license have been reviewed.
