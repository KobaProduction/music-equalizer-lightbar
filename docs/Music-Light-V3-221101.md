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
| **P14 / AIO_3** | reaches GND through a resistor | unknown; exact purpose and resistor value not yet recorded |
| **P18 / AIO_7** | resistor network reaching a USB Type-C-related node / diode path | unknown; may be power/USB sensing, not confirmed |
| **P20 / AIO_9** | reaches GND through a resistor | unknown; exact purpose and resistor value not yet recorded |

The observations above must remain separate from confirmed functional mappings. In particular, the fact that P14/P18/P20 have analog capabilities does not prove that this board uses them for audio or sensing.

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

The board contains 32 addressable RGB LEDs on **P34 / package pin 1**.

Behavior recovery of the original firmware and subsequent hardware execution established the factory LED control contract:

- SPI1 MOSI on P34;
- requested SPI baud 3 MHz;
- effective 3.2 MHz with the observed 16 MHz PCLK and factory divider 5;
- MODE1, 8-bit SPI frames;
- PHYplus TX DMA path;
- 24-bit-per-pixel GRB wire order;
- four SPI bits per LED bit;
- `0 -> 1000`, `1 -> 1110`;
- 12 SPI bytes per pixel, 384 bytes for all 32 pixels;
- latch/reset provided by low idle time between frames rather than an encoded zero tail.

The factory-equivalent backend has passed hardware validation on the inspected board. See `docs/WS2812B.md` for the recovered action route and implementation details.

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
6. **P34 LED transport — hardware-pass using recovered factory SPI/DMA contract**
7. **button active levels/pulls — confirmed active-low**
8. validate microphone ADC path
9. identify P2/P14/P18/P20 auxiliary circuitry if required by product behavior
10. BLE radio/GATT — runtime bring-up validated; product behavior integration remains ongoing


## Local buttons

Continuity/runtime confirmation:

- P11 / Power: button shorts GPIO to GND when pressed;
- P3 / Color/Bright: button shorts GPIO to GND when pressed;
- P7 / Mode/Speed: button shorts GPIO to GND when pressed.

The BLE bring-up firmware therefore configures all three as active-low inputs with weak internal pull-ups.

Local behavior:

- Power short press: toggle output power state;
- Color/Bright short press: cycle the local static-color palette;
- Color/Bright hold: step brightness, repeating while held;
- Mode/Speed short press: cycle local modes 0..3;
- Mode/Speed hold: step animation speed, repeating while held.

Input polling is 10 ms, debounce is 40 ms, long-press threshold is 600 ms and hold repeat is 150 ms.

All local actions update the same `melb_control_state_t` used by BLE commands.
