#include "ws2812b_spi.h"

static void encode_byte(uint8_t value, uint8_t output[8])
{
    for (uint8_t bit = 0u; bit < 8u; ++bit) {
        const uint8_t mask = (uint8_t)(UINT8_C(0x80) >> bit);
        output[bit] =
            (value & mask) != 0u ? WS2812B_SPI_SYMBOL_1 : WS2812B_SPI_SYMBOL_0;
    }
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
         * One 8-bit SSI frame is one WS2812-compatible protocol bit.
         *
         * The ST17H66B SSI visibly inserts a short LOW inter-frame interval.
         * Previous 3-bit/5-bit packing let those gaps land at arbitrary places
         * inside WS2812 cells. At 8 MHz, byte-aligning every protocol bit makes
         * that hardware interval repeat at the same point after every bit:
         *
         *   0 -> 11100000 : 0.375 us HIGH + LOW tail + inter-frame LOW
         *   1 -> 11111100 : 0.750 us HIGH + LOW tail + inter-frame LOW
         *
         * If the observed inter-frame idle is one SPI clock, the complete cell
         * is about 1.125 us with LOW times about 0.750/0.375 us respectively.
         */
        encode_byte(pixels[i].green, &output[offset]);
        offset += 8u;
        encode_byte(pixels[i].red, &output[offset]);
        offset += 8u;
        encode_byte(pixels[i].blue, &output[offset]);
        offset += 8u;
    }

    for (size_t i = 0u; i < (size_t)WS2812B_SPI_RESET_BYTES; ++i) {
        output[offset++] = 0u;
    }

    return offset;
}
