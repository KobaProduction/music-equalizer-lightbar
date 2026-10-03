# HappyLighting / Triones compatibility

## Target

The mobile compatibility target is the HappyLighting/Triones BLE controller family.

The firmware presents a Triones-compatible BLE identity and GATT shape:

- advertised name: `Triones-XXXXXX`;
- advertised 16-bit service UUID: `0xFFD5`;
- primary GATT service: `0xFFD5`;
- writable characteristic: `0xFFD9`;
- readable/notifiable status characteristic: `0xFFD4`.

This contract is implemented by project-owned code. No third-party client or firmware source is copied into the repository.

Protocol behavior was cross-checked against independent implementations/documentation, including `Dazaike/lighting` and `sysofwan/ha-triones`. These repositories are references for interoperability, not vendored dependencies.

## Commands

| Command | Bytes | Firmware behavior |
| --- | --- | --- |
| Power on | `CC 23 33` | enable renderer |
| Power off | `CC 24 33` | clear LEDs |
| RGB | `56 RR GG BB 00 F0 AA` | static RGB |
| White | `56 xx xx xx WW 0F AA` | static grayscale using `WW` |
| Effect | `BB MODE SPEED 44` | select native effect and speed |
| Status | `EF 01 77` | reply on `FFD4` notification |

The status response is 12 bytes:

`66 15 POWER MODE 20 SPEED R G B W 06 99`

`POWER` is `23` for on and `24` for off. Static RGB uses mode `0x41`.

## Dynamic modes

The firmware accepts Triones effect IDs `0x25..0x38`. It implements smooth rainbow/cross-fades, single-color gradual effects, pair cross-fades, seven-color and single-color strobes, and seven-color jump. Lower speed values are faster; higher values are slower.

## Boot behavior

The board starts powered on in a deliberately low-stress visual state:

- mode: `0x25` smooth rainbow;
- brightness: `128/255` (about 50%);
- speed: `24`;
- initial RGB seed: `255,64,0`.

The renderer sends a black WS2812 frame before the first visible effect frame, avoiding an all-white maximum-current startup flash.

## Physical controls

Confirmed active-low controls are polled every 10 ms:

- P11 Power: press toggles on/off.
- P3 Color/Bright: short cycles static colors; hold steps brightness and wraps.
- P7 Mode/Speed: short cycles useful Triones effects; hold steps effect speed and wraps.

Buttons and BLE writes share the same `melb_control_state_t`.

## UART diagnostics

Runtime diagnostics use UART0 on P9/P10 at 115200 8N1.

Expected startup includes:

```text
MELB boot: Music-Light-V3-221101 / ST17H66B
MELB: UART0 P9/P10 115200
MELB: BLE name=Triones-......
MELB: register HappyLighting FFD5/FFD9/FFD4
MELB: WS2812 SPI1/P34 init=ok
MELB: boot effect rainbow mode=25 brightness=128 speed=24
```

Writes to `FFD9`, status requests/replies, GAP state changes and physical-button changes are logged.

## Build

```sh
cmake --preset music-light-v3-221101-happylighting-ble
cmake --build --preset music-light-v3-221101-happylighting-ble
```

Artifacts are generated under:

`build/music-light-v3-221101-happylighting-ble/thb2-happylighting/build/`

with basename `melb_happylighting`.

## Validation boundary

Host protocol tests and GNU Arm compilation/link validate source behavior and image construction. HappyLighting discovery, GATT connection, status notification and LED behavior remain physical-hardware acceptance items until tested on the board.
