#pragma once

#include "ws2812b.h"

#include <stddef.h>
#include <stdint.h>

#define WS2812B_SPI_BAUD_HZ UINT32_C(4000000)
#define WS2812B_SPI_SYMBOL_0 UINT8_C(0x18)
#define WS2812B_SPI_SYMBOL_1 UINT8_C(0x1c)
#define WS2812B_SPI_BYTES_PER_PIXEL UINT8_C(15)

/*
 * Keep the line low for a conservative reset/latch interval. 160 zero bytes
 * are exactly 320 us at 4 MHz, covering newer WS2812B-compatible revisions
 * that require a reset interval substantially longer than the legacy 50 us.
 */
#define WS2812B_SPI_RESET_BYTES UINT8_C(160)

size_t ws2812b_spi_encoded_size(size_t pixel_count);

size_t ws2812b_spi_encode(
    const ws2812b_pixel_t *pixels,
    size_t pixel_count,
    uint8_t *output,
    size_t output_size);
