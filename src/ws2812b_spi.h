#pragma once

#include "ws2812b.h"

#include <stddef.h>
#include <stdint.h>

#define WS2812B_SPI_BAUD_HZ UINT32_C(8000000)
#define WS2812B_SPI_SYMBOL_0 UINT8_C(0xe0)
#define WS2812B_SPI_SYMBOL_1 UINT8_C(0xfc)
#define WS2812B_SPI_BYTES_PER_PIXEL UINT8_C(24)

/*
 * Keep the line low for a conservative reset/latch interval. 320 zero bytes
 * are at least 320 us at 8 MHz. Any SSI inter-frame idle time is also LOW and
 * only lengthens the reset interval.
 */
#define WS2812B_SPI_RESET_BYTES UINT16_C(320)

size_t ws2812b_spi_encoded_size(size_t pixel_count);

size_t ws2812b_spi_encode(
    const ws2812b_pixel_t *pixels,
    size_t pixel_count,
    uint8_t *output,
    size_t output_size);
