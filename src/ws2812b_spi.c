#include "ws2812b_spi.h"

static void encode_byte(uint8_t value, uint8_t output[4])
{
    for (uint8_t pair = 0u; pair < 4u; ++pair) {
        const uint8_t shift = (uint8_t)(6u - (pair * 2u));
        const uint8_t first =
            ((value >> (shift + 1u)) & 1u) != 0u
                ? WS2812B_SPI_SYMBOL_1
                : WS2812B_SPI_SYMBOL_0;
        const uint8_t second =
            ((value >> shift) & 1u) != 0u
                ? WS2812B_SPI_SYMBOL_1
                : WS2812B_SPI_SYMBOL_0;

        output[pair] = (uint8_t)((first << 4u) | second);
    }
}

size_t ws2812b_spi_encoded_size(size_t pixel_count)
{
    return pixel_count * (size_t)WS2812B_SPI_BYTES_PER_PIXEL;
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
         * Recovered factory action node reads input[1], input[0], input[2]:
         * RGB input therefore becomes GRB on the wire.
         */
        encode_byte(pixels[i].green, &output[offset]);
        offset += 4u;
        encode_byte(pixels[i].red, &output[offset]);
        offset += 4u;
        encode_byte(pixels[i].blue, &output[offset]);
        offset += 4u;
    }

    return offset;
}
