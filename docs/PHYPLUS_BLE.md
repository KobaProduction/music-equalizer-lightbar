# External Phyplus BLE integration

## Purpose

The ST17H66B BLE backend follows the GCC path used by pvvx/THB2. CMake fetches a pinned THB2 revision into the build tree with FetchContent; the third-party tree keeps its upstream licenses and is not vendored into this repository.

Pinned THB2 revision: `48db5245d235aef57cfdf5fcc58ec74fa753d89c`.

## Build the profile check

```sh
cmake --preset phyplus-profile-check
cmake --build --preset phyplus-profile-check
```

The profile check compiles project-owned HappyLighting/Triones GATT glue against the fetched PHY62x2 SDK headers.

## License boundary

The project pins and fetches the upstream repository, preserves upstream source/license material unchanged in the build tree, does not relicense the Phyplus SDK, and keeps project-owned GATT/application code in this repository.

## Project-owned GATT profile

The HappyLighting/Triones backend registers:

- primary service `0xFFD5`;
- writable characteristic `0xFFD9` with Write Request and Write Without Response;
- readable/notifiable status characteristic `0xFFD4` with CCC configuration.

Writes are parsed by `happy_lighting_apply_command()`. Status request `EF 01 77` is answered with the Triones 12-byte state notification.

The layers remain separate: fetched Phyplus BLE stack/radio, project-owned GATT glue, and project-owned HappyLighting/Triones semantics.

## Full HappyLighting/Triones firmware

```sh
cmake --preset music-light-v3-221101-happylighting-ble
cmake --build --preset music-light-v3-221101-happylighting-ble
```

The build prepares an isolated overlay of pinned `bthome_phy6222` sources. The upstream BLE host/controller/radio/startup/linker substrate remains third-party material under its original licenses. The overlay replaces the application GATT profile with project-owned HappyLighting/Triones code and adds the WS2812 renderer and local-button controls.

For target safety, inherited KEY2 peripheral behavior and battery/ADC policy are not used. Runtime configuration uses project-owned RAM defaults rather than interpreting configuration bytes left by the factory firmware.

Artifacts are produced under:

`build/music-light-v3-221101-happylighting-ble/thb2-happylighting/build/`

including `melb_happylighting.elf`, `melb_happylighting.hex`, `melb_happylighting.bin`, and `melb_happylighting.map`.

## Validation boundary

Terminal validation establishes source compilation, host protocol tests, complete BLE/RF/startup link, and PHY62x2 image generation. Actual HappyLighting discovery/connection, GATT traffic, WS2812 output and button behavior remain physical-hardware acceptance items until exercised on the board.
