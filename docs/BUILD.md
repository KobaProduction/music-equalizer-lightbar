# Build and target model

## Target identity

Build selection is board-oriented.

Current board:

| Build preset | PCB marking | MCU | Status |
| --- | --- | --- | --- |
| `music-light-v3-221101` | `Music-Light-V3-221101` | ST17H66B | bring-up |

A future board is added as a new board preset even when it shares the same application behavior or LED protocol.

The MCU is a property of the board target. This keeps the repository ready for other controllers, including low-cost non-ST17H66B variants.

## Local build

```sh
cmake --preset music-light-v3-221101
cmake --build --preset music-light-v3-221101
```

Host-side protocol tests are a separate preset and do not require the Arm toolchain.

## CI

Every push and pull request:

- runs repository-owned protocol tests on the host;
- builds all declared firmware board presets;
- validates the ST17H66B BIN/HEX layout consistency;
- uploads a separate artifact bundle per board.

Each firmware bundle contains ELF, HEX, BIN, linker MAP, and a JSON firmware manifest.

## Releases

Tags matching `v*` run the same board matrix. Each board build uploads its own artifacts, then one publish job downloads all board bundles and creates a single GitHub prerelease for the tag.

This separation is required for multi-target releases: board matrix jobs must never race to create the same release.

Releases remain prereleases while the current target has not passed hardware validation. Removing that restriction is a product-readiness decision, not a build-system decision.

## Validation contract

The current ST17H66B builder proves:

- C/ASM compilation with GNU Arm Embedded;
- target-specific link using the repository linker script;
- generation of ELF, Intel HEX, raw BIN and MAP artifacts;
- vector-table and image-origin consistency for the experimental layout;
- HEX/BIN byte consistency.

It does not yet prove:

- that the inferred ST17H66B load/run memory layout is correct for this PCB;
- that the ROM UART loader accepts and executes the generated image on this board;
- that BLE radio/GATT initializes;
- that any board GPIO mapping is correct.
