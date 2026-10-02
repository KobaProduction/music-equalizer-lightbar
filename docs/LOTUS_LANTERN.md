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

## Current validation boundary

The protocol parser is host-tested.

The ST17H66B BLE radio/GATT backend is not yet implemented in this repository. Public PHY62x2 BLE examples prove that a GCC BLE build exists, but the available vendor SDK material carries a restrictive license. That code is therefore not vendored under the project MIT license.

Protocol compatibility and radio-stack integration remain separate acceptance levels.
