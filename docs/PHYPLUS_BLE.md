# External Phyplus BLE integration

## Purpose

The ST17H66B BLE backend follows the proven GCC path used by pvvx/THB2 while keeping the Phyplus SDK outside this MIT-licensed repository.

The external SDK is not downloaded, copied, relicensed, or redistributed by the project build.

## Reference tree

The currently tested reference layout is the SDK tree inside:

https://github.com/pvvx/THB2/tree/master/bthome_phy6222/SDK

pvvx/THB2 demonstrates a complete GCC build of the PHY62x2 BLE host, controller, GATT/GAP profiles, radio driver, startup and ROM-symbol integration.

Its root license explicitly separates permissive project source from Phyplus SDK material carrying the vendor SDK license. This repository therefore treats that SDK tree as an external dependency.

## Configure the profile compile-check

Point CMake at an existing SDK checkout:

```sh
cmake -S . -B build/phyplus-profile \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DMELB_PHYPLUS_PROFILE_CHECK=ON \
  -DMELB_PHYPLUS_SDK_ROOT=/path/to/THB2/bthome_phy6222/SDK
cmake --build build/phyplus-profile --target phyplus-ble-profile-check
```

The check compiles the project-owned Lotus Lantern GATT profile against the real Phyplus headers. It intentionally does not link or redistribute the vendor BLE stack.

## Project-owned GATT profile

The project-owned adapter registers:

- primary service `0xFFF0`;
- writable characteristic `0xFFF3`;
- both Write Request and Write Without Response properties.

A successful write is passed through `lotus_lantern_apply_frame()`. The backend then invokes a project callback with the updated `melb_control_state_t`.

This keeps three layers separate:

1. Phyplus BLE stack / radio: external SDK;
2. GATT transport glue: project-owned;
3. Lotus Lantern command semantics: project-owned.

## Next integration step

The next target is a complete `lotus-lantern-ble` firmware variant linked against the external SDK and its `bb_rom_sym_m0.gcc` symbol map.

That target must establish:

- PHY62x2 clock/RF/heap initialization;
- OSAL task initialization;
- GAP peripheral role;
- advertising name `ELK-BLEDOM-MELB`;
- GATT service registration;
- state changes routed to the WS2812 renderer;
- local P11/P3/P7 button events routed to the same state model.

Build success will still be distinct from radio/hardware validation.
