#pragma once

#include <stddef.h>
#include <stdint.h>

#define WS2812B_BIT_RATE_HZ UINT32_C(800000)
#define WS2812B_BITS_PER_PIXEL UINT8_C(24)
#define WS2812B_BYTES_PER_PIXEL UINT8_C(3)
#define WS2812B_RESET_MIN_US UINT32_C(50)

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} ws2812b_pixel_t;

/*
 * Serialize logical RGB pixels into the byte order transmitted by WS2812B:
 * G7..G0, R7..R0, B7..B0.
 *
 * This function creates protocol bytes only. Precise 800 kbit/s waveform
 * generation is an MCU/board backend responsibility.
 */
size_t ws2812b_serialize_grb(
    const ws2812b_pixel_t *pixels,
    size_t pixel_count,
    uint8_t *output,
    size_t output_size);

void ws2812b_clear(ws2812b_pixel_t *pixels, size_t pixel_count);
