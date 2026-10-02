# Music-Light-V3-221101

## Identity

Exact PCB marking:

`Music-Light-V3-221101`

This marking is the canonical board identity used by the repository.

## Confirmed board pin map

The following mappings have been established by continuity tracing on the inspected PCB.

| Signal/function | ST17H66B GPIO | TSSOP-16 pin | Status |
| --- | --- | ---: | --- |
| WS2812B data | **P34** | **1** | confirmed |
| Color/Bright button | **P3** | **3** | confirmed |
| Mode/Speed button | **P7** | **4** | confirmed |
| ROM UART TX (target → adapter RX) | **P9** | **5** | confirmed |
| ROM UART RX (adapter TX → target) | **P10** | **6** | confirmed |
| Power button | **P11 / AIO_0** | **7** | confirmed |
| Microphone analog path | **P15 / AIO_4 / ADC4** | **9** | confirmed |

## Unresolved / observed nets

These are physical observations only; no firmware function is assigned yet.

| GPIO | Observation | Interpretation status |
| --- | --- | --- |
| **P2** | resistor network into a transistor stage, continuing into another transistor / diode-related network | unknown; do not assign yet |
| **P18 / AIO_7** | resistor network reaching a USB Type-C-related node / diode path | unknown; may be power/USB sensing, not confirmed |
| **P20 / AIO_9** | reaches GND through a resistor | unknown; exact purpose and resistor value not yet recorded |

The observations above must remain separate from confirmed functional mappings. In particular, the fact that P18/P20 have analog/PGA capabilities does not prove that this board uses them for audio.

## Microphone

Continuity confirms the microphone network reaches **P15 / AIO_4 / ADC4**.

P15 is multifunctional and also supports MICBIAS. The exact analog configuration and bias topology will be verified during powered ADC/microphone bring-up.

## Programming pads

Confirmed:

- target TX: **P9**, package pin 5;
- target RX: **P10**, package pin 6.

USB-UART wiring:

- adapter RX ← target P9/TX;
- adapter TX → target P10/RX;
- common GND;
- 3.3 V logic/power domain.

The first ROM-UART operation remains read-only: preserve the complete original Flash before any erase/write operation.

## LED chain

The board contains 32 addressable RGB LEDs using the WS2812B-compatible 800 kbit/s, 24-bit-per-pixel GRB protocol.

The LED data GPIO is now confirmed as **P34 / package pin 1**.

This removes the main hardware blocker for a first LED bring-up firmware. The remaining implementation work is the ST17H66B GPIO/timing backend required to generate the WS2812B waveform.

## Controls

Confirmed control pins:

- Power → **P11**
- Color/Bright → **P3**
- Mode/Speed → **P7**

The electrical active level and pull configuration are not yet assumed; those can be established either by static resistor tracing or by powered observation.

## BLE

BLE is mandatory for this target. External control uses a repository-owned transport-independent command protocol; the ST17H66B radio/GATT backend remains a separate implementation milestone.

## Bring-up status

1. **P9/P10 programming UART — confirmed**
2. **P15 microphone path — confirmed**
3. **P34 WS2812B data — confirmed**
4. **P11 / P3 / P7 controls — confirmed**
5. preserve original Flash through ROM UART
6. validate GPIO output and WS2812B waveform
7. determine button active levels/pulls
8. validate microphone ADC path
9. identify P2/P18/P20 auxiliary circuitry if required by product behavior
10. integrate and validate BLE radio/GATT
