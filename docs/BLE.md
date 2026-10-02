# BLE control contract

## Scope

BLE is mandatory for the project, but the application control protocol is intentionally separated from the ST17H66B radio/stack implementation.

This lets the same control model survive future MCU targets and lets external integrations be developed against a stable packet contract.

## Transport model

The intended GATT shape is:

- one custom 128-bit service;
- one writable control characteristic carrying command packets;
- one readable/notifiable status characteristic carrying device state.

The concrete UUIDs remain unassigned until the first BLE backend is integrated. The command packet format below is already repository-owned and stable enough for implementation/testing.

## Command packet format

All packets are binary.

Byte 0 is the opcode. Remaining bytes are opcode-specific payload.

| Opcode | Command | Payload | Meaning |
| --- | --- | --- | --- |
| `0x01` | Set power | `u8` | 0 = off, 1 = on |
| `0x02` | Set brightness | `u8` | 0..255 |
| `0x03` | Set RGB | `u8 R, u8 G, u8 B` | logical RGB color |
| `0x04` | Set mode | `u8` | application-defined mode ID |
| `0x05` | Set speed | `u8` | 0..255 |
| `0x06` | Set audio-reactive | `u8` | 0 = disabled, 1 = enabled |

Packets use exact lengths. Unknown opcodes, invalid boolean values and malformed lengths are rejected.

## State model

The repository-owned state contains:

- power;
- brightness;
- RGB color;
- mode;
- speed;
- audio-reactive enable.

This is an application contract. It does not depend on GAP/GATT/OSAL APIs and can therefore be tested on the host.

## Backend boundary

The ST17H66B BLE backend will be responsible for:

1. advertising and connection lifecycle;
2. custom service/characteristic registration;
3. forwarding writes to `melb_control_apply_packet()`;
4. exposing current state through the status characteristic;
5. security/pairing policy if required.

The backend must not redefine command semantics.

## Current validation

Host tests cover:

- valid command decoding;
- malformed packet rejection;
- boolean validation;
- default state;
- WS2812B GRB serialization.

BLE radio/GATT operation is not yet hardware-validated.
