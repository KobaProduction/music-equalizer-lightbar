#pragma once

#include "ws2812b.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Factory firmware behavior contract recovered from the original dump:
 * requested SPI baud 3 MHz, 4 SPI bits per LED bit,
 * 0 -> 1000, 1 -> 1110, 12 SPI bytes/pixel.
 *
 * Hardware acceptance on this board established RBG wire order for the
 * repository's logical RGB pixel representation. Eight zero-valued guard
 * pixel slots are emitted after the physical pixels before the zero-filled
 * latch trailer. This preserves the hardware-proven 40-slot wire behavior
 * while the application continues to model exactly 32 physical LEDs.
 */
#define WS2812B_SPI_BAUD_HZ UINT32_C(3000000)
#define WS2812B_SPI_SYMBOL_0 UINT8_C(0x8)
#define WS2812B_SPI_SYMBOL_1 UINT8_C(0xe)
#define WS2812B_SPI_BYTES_PER_PIXEL UINT8_C(12)
#define WS2812B_SPI_GUARD_PIXELS UINT8_C(8)
#define WS2812B_SPI_RESET_BYTES UINT8_C(128)

size_t ws2812b_spi_encoded_size(size_t pixel_count);

size_t ws2812b_spi_encode(
    const ws2812b_pixel_t *pixels,
    size_t pixel_count,
    uint8_t *output,
    size_t output_size);
