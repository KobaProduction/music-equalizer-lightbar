#include "ws2812b_spi.h"

static void encode_byte(uint8_t value, uint8_t output[3])
{
    uint32_t encoded = 0u;

    for (uint8_t bit = 0u; bit < 8u; ++bit) {
        const uint8_t mask = (uint8_t)(UINT8_C(0x80) >> bit);
        const uint8_t symbol =
            (value & mask) != 0u ? WS2812B_SPI_SYMBOL_1 : WS2812B_SPI_SYMBOL_0;

        encoded = (encoded << 3u) | symbol;
    }

    output[0] = (uint8_t)(encoded >> 16u);
    output[1] = (uint8_t)(encoded >> 8u);
    output[2] = (uint8_t)encoded;
}

size_t ws2812b_spi_encoded_size(size_t pixel_count)
{
    return (pixel_count * (size_t)WS2812B_SPI_BYTES_PER_PIXEL)
        + (size_t)WS2812B_SPI_RESET_BYTES;
}

size_t ws2812b_spi_encode(
    const ws2812b_pixel_t *pixels,
    size_t pixel_count,
    uint8_t *output,
    size_t output_size)
{
    const size_t required = ws2812b_spi_encoded_size(pixel_count);

    if (pixels == NULL || output == NULL || output_size < required) {
        return 0u;
    }

    size_t offset = 0u;

    for (size_t i = 0u; i < pixel_count; ++i) {
        /*
         * WS2812B wire order is GRB. Each source byte expands from 8 protocol
         * bits to 24 SPI bits, therefore exactly three SPI bytes.
         */
        encode_byte(pixels[i].green, &output[offset]);
        offset += 3u;
        encode_byte(pixels[i].red, &output[offset]);
        offset += 3u;
        encode_byte(pixels[i].blue, &output[offset]);
        offset += 3u;
    }

    for (size_t i = 0u; i < (size_t)WS2812B_SPI_RESET_BYTES; ++i) {
        output[offset++] = 0u;
    }

    return offset;
}
