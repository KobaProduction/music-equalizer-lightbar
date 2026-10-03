#pragma once

#include "ws2812b.h"

#include <stddef.h>
#include <stdint.h>

#define WS2812B_SPI_BAUD_HZ UINT32_C(2400000)
#define WS2812B_SPI_SYMBOL_0 UINT8_C(0x4)
#define WS2812B_SPI_SYMBOL_1 UINT8_C(0x6)
#define WS2812B_SPI_BYTES_PER_PIXEL UINT8_C(9)

/*
 * Keep the line low for a conservative reset/latch interval. 96 zero bytes
 * are about 320 us at 2.4 MHz and about 288 us at 2.667 MHz, covering newer
 * WS2812B revisions that specify a reset interval greater than 280 us.
 */
#define WS2812B_SPI_RESET_BYTES UINT8_C(96)

size_t ws2812b_spi_encoded_size(size_t pixel_count);

size_t ws2812b_spi_encode(
    const ws2812b_pixel_t *pixels,
    size_t pixel_count,
    uint8_t *output,
    size_t output_size);
