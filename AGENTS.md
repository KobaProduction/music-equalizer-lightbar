# Repository Map

## Scope

This repository contains embedded software for a music-reactive RGB light bar.

The project is intentionally multi-target. ST17H66B is the first bring-up target, not the permanent architectural boundary.

## Repository authority

- `src/` — project source code.
- `cmake/` — build-system and cross-toolchain support.
- `docs/` — hardware bring-up, reverse-engineering findings, and target notes.
- `.github/workflows/` — CI automation.
- root `CMakeLists.txt` / `CMakePresets.json` — canonical build entry points.

Do not introduce deeper source decomposition until hardware and firmware ownership boundaries are proven by implementation needs.

## Current technical direction

- Primary toolchain: GNU Arm Embedded (`arm-none-eabi-gcc`).
- Build system: CMake.
- First MCU target: ST17H66B / Cortex-M0.
- BLE support is a required project capability.
- Keil is not a project dependency unless a future target makes it unavoidable.
- Additional MCU targets must be added without coupling application code to ST17H66B.

## Third-party code

Do not copy vendor SDK, BLE stack, ROM symbol maps, linker scripts, or reverse-engineered third-party sources into this repository until their provenance and license are recorded.

Project-owned source is MIT-licensed. Third-party material retains its own license and must not be relicensed implicitly.

## Validation levels

Keep these states distinct:

1. source-confirmed;
2. build-pass;
3. hardware-pass;
4. product-ready.

A compiler-only bootstrap is not a flashable firmware image, and a successful build is not hardware validation.
