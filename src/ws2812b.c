#include "ws2812b.h"

size_t ws2812b_serialize_grb(
    const ws2812b_pixel_t *pixels,
    size_t pixel_count,
    uint8_t *output,
    size_t output_size)
{
    const size_t required = pixel_count * (size_t)WS2812B_BYTES_PER_PIXEL;

    if (pixels == NULL || output == NULL || output_size < required) {
        return 0u;
    }

    for (size_t i = 0u; i < pixel_count; ++i) {
        const size_t base = i * (size_t)WS2812B_BYTES_PER_PIXEL;
        output[base + 0u] = pixels[i].green;
        output[base + 1u] = pixels[i].red;
        output[base + 2u] = pixels[i].blue;
    }

    return required;
}

void ws2812b_clear(ws2812b_pixel_t *pixels, size_t pixel_count)
{
    if (pixels == NULL) {
        return;
    }

    for (size_t i = 0u; i < pixel_count; ++i) {
        pixels[i].red = 0u;
        pixels[i].green = 0u;
        pixels[i].blue = 0u;
    }
}
