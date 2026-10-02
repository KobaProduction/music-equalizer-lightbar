# Music-Light-V3-221101

## Identity

Exact PCB marking:

`Music-Light-V3-221101`

This marking is the canonical board identity used by the repository.

## Observed hardware

| Function | Observation | Mapping |
| --- | --- | --- |
| MCU | ST17H66B, Arm Cortex-M0 | confirmed component marking |
| RGB LEDs | 32 addressable LEDs | WS2812B-compatible protocol |
| LED data | single serial data path expected | GPIO TBD |
| Controls | reported functions/labels: Power, Color, Bright, Mode, Speed | physical grouping and GPIOs TBD |
| Microphone | populated analog microphone path | **P15 / AIO_4**, continuity confirmed |
| USB | USB Type-C connector populated | role beyond power not yet established |
| Battery | battery pads populated; no battery in inspected unit | power path TBD |
| Programming pads | GND, 3.3 V plus two signal pads | **P9 / P10**, continuity confirmed |

## Confirmed board pin map

Only continuity-tested mappings belong in this table.

| Signal/function | ST17H66B GPIO | TSSOP-16 pin | Status |
| --- | --- | ---: | --- |
| ROM UART TX (target → adapter RX) | **P9** | **5** | confirmed by continuity |
| ROM UART RX (adapter TX → target) | **P10** | **6** | confirmed by continuity |
| Microphone analog path | **P15 / AIO_4 / ADC4** | **9** | confirmed by continuity |
| WS2812B data | TBD | TBD | tracing pending |
| Power control | TBD | TBD | physical grouping/GPIO pending |
| Color control | TBD | TBD | physical grouping/GPIO pending |
| Bright control | TBD | TBD | physical grouping/GPIO pending |
| Mode control | TBD | TBD | physical grouping/GPIO pending |
| Speed control | TBD | TBD | physical grouping/GPIO pending |

P15/AIO_4 is multifunctional: the SoC pin supports GPIO, ADC4 and MICBIAS functionality. The PCB continuity confirms the microphone network reaches this pin; the exact analog operating mode will be established during ADC/microphone bring-up.

## Programming pads

The previously unidentified signal pads are now resolved at the MCU:

- target TX: P9, TSSOP-16 package pin 5;
- target RX: P10, TSSOP-16 package pin 6.

USB-UART wiring therefore follows:

- adapter RX ← target P9/TX;
- adapter TX → target P10/RX;
- common GND;
- 3.3 V logic/power domain.

The next programming step remains read-only: enter ROM UART and preserve the complete original Flash before any erase/write operation.

## LED chain

The board contains 32 addressable RGB LEDs and uses the WS2812B-compatible 800 kbit/s, 24-bit-per-pixel GRB protocol.

The repository implements the protocol-level GRB frame representation. Precise waveform generation will be bound as soon as the final LED-data GPIO is continuity-confirmed.

## BLE

BLE is mandatory for this target. External control uses a repository-owned transport-independent command protocol; the ST17H66B radio/GATT backend remains the next BLE implementation milestone.

## Bring-up sequence

1. **programming pads P9/P10 — confirmed;**
2. **microphone path P15/AIO_4 — confirmed;**
3. trace the remaining WS2812B data GPIO;
4. preserve original Flash through ROM UART;
5. validate the repository-generated bring-up image without erasing the backup;
6. validate one GPIO output and WS2812B chain;
7. validate controls;
8. validate microphone ADC path;
9. integrate and validate BLE radio/GATT.
