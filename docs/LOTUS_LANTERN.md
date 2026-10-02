# Lotus Lantern compatibility

## Why this protocol

Lotus Lantern is a widely deployed mobile application for inexpensive Bluetooth LED controllers. The Android application has more than 10 million installs and supports color, brightness, effects, music-reactive modes, timers, and multiple strips.

The protocol used here is community reverse-engineered from the stock Android application and is implemented as a compatibility adapter over the repository-owned control state.

Reference implementation/protocol documentation:

- https://github.com/Rxflex/LotusLantern
- GATT service: `0xFFF0`
- write characteristic: `0xFFF3`
- command frame: 9 bytes, `7E LEN CMD P1 P2 P3 P4 P5 EF`

No third-party source is copied into this repository.

## Advertising identity

The compatibility backend should advertise a name matching a Lotus Lantern device prefix.

Current project identity:

`ELK-BLEDOM-MELB`

The application family recognizes device-name prefixes such as `ELK-`, `ELK~`, `LED LIGHT STRIP`, and `XSL-`.

## Supported command subset

The initial compatibility layer supports:

- power on/off;
- static RGB color;
- brightness;
- dynamic mode ID;
- effect speed;
- music-amplitude RGB frames;
- external-microphone on/off;
- external-microphone sensitivity as an accepted compatibility command;
- RGB pin-order command as an accepted compatibility command.

Unsupported commands are rejected by the parser without changing device state.

## GATT backend contract

A future ST17H66B BLE transport needs to:

1. advertise as `ELK-BLEDOM-MELB`;
2. expose primary service UUID `0000fff0-0000-1000-8000-00805f9b34fb`;
3. expose writable characteristic UUID `0000fff3-0000-1000-8000-00805f9b34fb`;
4. pass every complete 9-byte write to `lotus_lantern_apply_frame()`;
5. apply the resulting repository control state to the LED renderer.

The original Lotus Lantern application uses GATT writes with response. Community-compatible clients also report that write-without-response works on many controllers.

## Full ST17H66B bring-up target

The repository now provides the preset:

```sh
cmake --preset music-light-v3-221101-lotus-ble
cmake --build --preset music-light-v3-221101-lotus-ble
```

CMake fetches the pinned pvvx/THB2 revision, prepares an isolated build-tree overlay and builds a complete GCC BLE image using the upstream PHY62x2 startup, host/controller/radio stack and linker model.

The overlay replaces the upstream application GATT profile with the project-owned Lotus Lantern `FFF0/FFF3` profile and includes the project-owned P34/SPI1 WS2812 renderer.

Current rendered commands:

- power;
- brightness;
- static RGB;
- music-amplitude RGB frames.

Mode and speed values are parsed and retained in the common control state, but local animation engines are not implemented yet.

The image boots dark by default.

## Current validation boundary

Validated in the terminal with Arm GNU Toolchain 13.2.1:

- FetchContent resolution of pinned THB2;
- complete BLE host/controller/radio link;
- `melb_lotus.elf`, `.hex`, `.bin`, and `.map` generation;
- embedded device identity `ELK-BLEDOM-MELB`;
- linked project Lotus parser and P34/SPI1 WS2812 implementation.

Not yet validated:

- execution on the physical `Music-Light-V3-221101`;
- BLE advertising/connection from the board;
- GATT writes from the Lotus Lantern phone application;
- P34 waveform and LED response under the full BLE image.

Build validation must not be treated as hardware validation.
