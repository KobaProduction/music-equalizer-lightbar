#pragma once

#include "ws2812b.h"

#include <stddef.h>
#include <stdint.h>

#define WS2812B_SPI_BAUD_HZ UINT32_C(2400000)
#define WS2812B_SPI_SYMBOL_0 UINT8_C(0x4)
#define WS2812B_SPI_SYMBOL_1 UINT8_C(0x6)
#define WS2812B_SPI_BYTES_PER_PIXEL UINT8_C(9)

/*
 * 16 zero bytes at 2.4 MHz hold the data line low for ~53.3 us,
 * satisfying the WS2812B reset/latch interval.
 */
#define WS2812B_SPI_RESET_BYTES UINT8_C(16)

size_t ws2812b_spi_encoded_size(size_t pixel_count);

size_t ws2812b_spi_encode(
    const ws2812b_pixel_t *pixels,
    size_t pixel_count,
    uint8_t *output,
    size_t output_size);
