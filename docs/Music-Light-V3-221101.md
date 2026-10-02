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
| Buttons | three physical controls: Power, Color/Bright, Mode/Speed | GPIOs TBD |
| Microphone | populated | analog/digital path TBD |
| USB | USB Type-C connector populated | role beyond power not yet established |
| Battery | battery pads populated; no battery in inspected unit | power path TBD |
| Programming pads | GND, 3.3 V and two unlabeled pads observed | unlabeled pads not yet board-confirmed |

## Board pin map

Only continuity-tested mappings belong in this table.

| Signal | ST17H66B GPIO/package pin | Status |
| --- | --- | --- |
| WS2812B data | TBD | tracing pending |
| Power button | TBD | tracing pending |
| Color/Bright button | TBD | tracing pending |
| Mode/Speed button | TBD | tracing pending |
| Microphone signal | TBD | tracing pending |
| Microphone power/bias control, if MCU-controlled | TBD | tracing pending |
| UART TX/RX test pads | TBD | tracing pending |

## LED chain

The board contains 32 addressable RGB LEDs and uses the WS2812B-compatible 800 kbit/s, 24-bit-per-pixel GRB protocol.

The repository currently implements the protocol-level GRB frame representation. Precise waveform generation is board/MCU timing work and will be bound after the LED data GPIO and usable ST17H66B peripheral/timing method are confirmed.

## BLE

BLE is mandatory for this target. External control is expected to use BLE; the exact application GATT protocol will be defined separately from the low-level radio/stack integration.

## Bring-up sequence

1. complete continuity map;
2. identify the two unlabeled programming pads;
3. preserve original flash through the ROM UART path;
4. validate the repository-generated ST17H66B image format without erasing the backup;
5. validate one GPIO output;
6. validate the WS2812B chain;
7. validate buttons;
8. validate microphone/ADC path;
9. integrate and validate BLE.
