# Runtime UART observations

This document records observations from the original firmware running on the inspected `Music-Light-V3-221101` board.

These observations are runtime evidence from the original device firmware. They are separate from the ST17H66B ROM-UART bootloader protocol described from external tooling.

## Confirmed application UART

The original application firmware produces readable debug output on the confirmed P9/P10 UART pair at **115200 baud**.

A test at 9600 baud produced unreadable characters only. Therefore 9600 baud is **not confirmed for the running application firmware on this board**.

The external ST17H66B/PHY62x2 ROM-loader tooling uses 9600 baud as its initial connection rate, but that remains an external reference until ROM-loader entry is reproduced on this specific board.

## Original firmware boot log

Observed at 115200 baud after normal power-up:

```text
dev_mac_data: 0xff ff 11 2a 35 c4
OTA Boot Version: 3.1.1
=====SimpleBLEPeripheral_Init Done=======
1
[REST CAUSE] 0
 noice=0
awaken = 1
2
GPIO_INIT
Gaprole_start:
[GAP ROLE 1]
==> HCI_COMMAND_COMPLETE_EVENT_CODE: 202a
Gaprole_adversting:
[GAP ROLE 2]
LEVEL =4
```

Confirmed from the log only:

- the original firmware reports `OTA Boot Version: 3.1.1`;
- the application emits BLE-related initialization/advertising log messages;
- the debug UART is readable at 115200 baud.

No additional meaning is assigned to `REST CAUSE`, `noice`, `awaken`, `LEVEL`, `202a`, or the printed MAC-byte ordering without further evidence.

## Button event codes

Button presses were correlated manually with the original firmware debug log:

| Physical control | Confirmed GPIO | Observed log code |
| --- | --- | ---: |
| Mode/Speed | P7 | **4** |
| Color/Bright | P3 | **2** |
| Power | P11 | **1** |

Representative output:

```text
KEY_UP_1111: 4
KEY_UP_1111: 2
KEY_UP_1111: 1
```

The log also emitted:

```text
double=6
```

The meaning of `double=6` is not yet confirmed.

## Evidence boundary

Current board-level facts:

- application debug output: **115200 baud confirmed**;
- ROM-loader initial baud: **not yet confirmed on this board**;
- ROM-loader 9600-baud behavior exists only as an external reference from known ST17H66B/PHY62x2 tooling until reproduced here.
