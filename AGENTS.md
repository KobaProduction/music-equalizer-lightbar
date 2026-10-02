# Repository Map

## Scope

This repository contains embedded software for a music-reactive RGB light bar.

The project is intentionally multi-target. The first canonical board target is the PCB marked `Music-Light-V3-221101`, currently identified with an ST17H66B MCU.

## Repository authority

- `src/` — project-owned firmware source, board support, MCU startup/linker code, and protocol code.
- `cmake/` — build-system and cross-toolchain support.
- `docs/` — hardware bring-up, reverse-engineering findings, build contracts, and target notes.
- `.github/workflows/` — CI/release automation.
- root `CMakeLists.txt` / `CMakePresets.json` — canonical build entry points.

Do not introduce deep source decomposition until hardware and firmware ownership boundaries are proven by implementation needs.

## Current technical direction

- Primary toolchain: GNU Arm Embedded (`arm-none-eabi-gcc`).
- Build system: CMake.
- Canonical board target: `music-light-v3-221101`.
- First MCU target: ST17H66B / Cortex-M0.
- LED chain: 32 addressable RGB LEDs using the WS2812B-compatible one-wire protocol.
- BLE support is a required project capability.
- Keil is not a project dependency unless a future target makes it unavoidable.
- Additional board/MCU targets must be selectable without coupling project logic to ST17H66B.

## Hardware evidence rule

The exact PCB marking is `Music-Light-V3-221101`.

Do not invent GPIO assignments. Until continuity tracing establishes a connection, board pins remain explicitly unmapped/TBD in source and documentation.

## Third-party code

Do not copy vendor SDK, BLE stack, ROM symbol maps, linker scripts, or reverse-engineered third-party sources into this repository until their provenance and license are recorded.

Project-owned source is MIT-licensed. Third-party material retains its own license and must not be relicensed implicitly.

## Validation levels

Keep these states distinct:

1. source-confirmed;
2. build-pass;
3. hardware-pass;
4. product-ready.

A successfully linked image is not hardware validation. ST17H66B memory/startup assumptions remain experimental until executed on the actual board.
