#include "board.h"
#include "st17h66b_spi1.h"
#include "ws2812b.h"
#include "ws2812b_spi.h"

#include <stddef.h>
#include <stdint.h>

enum {
    LED_COUNT = 32,
    SPI_FRAME_SIZE =
        (LED_COUNT * WS2812B_SPI_BYTES_PER_PIXEL) + WS2812B_SPI_RESET_BYTES,
};

static ws2812b_pixel_t pixels[LED_COUNT];
static uint8_t spi_frame[SPI_FRAME_SIZE];

static void prepare_pattern(void)
{
    ws2812b_clear(pixels, LED_COUNT);

    /*
     * Four 8-pixel blocks make polarity/order mistakes immediately visible:
     * red, green, blue, low-intensity white.
     */
    for (size_t i = 0u; i < 8u; ++i) {
        pixels[i].red = 32u;
    }
    for (size_t i = 8u; i < 16u; ++i) {
        pixels[i].green = 32u;
    }
    for (size_t i = 16u; i < 24u; ++i) {
        pixels[i].blue = 32u;
    }
    for (size_t i = 24u; i < 32u; ++i) {
        pixels[i].red = 12u;
        pixels[i].green = 12u;
        pixels[i].blue = 12u;
    }
}

int main(void)
{
    const melb_board_config_t *board = melb_board_config();

    if (board->led_data_pin != ST17H66B_PIN_P34 || board->led_count != LED_COUNT) {
        for (;;) {
            __asm volatile ("nop");
        }
    }

    prepare_pattern();

    const size_t encoded = ws2812b_spi_encode(
        pixels,
        LED_COUNT,
        spi_frame,
        sizeof(spi_frame));

    if (encoded != sizeof(spi_frame)) {
        for (;;) {
            __asm volatile ("nop");
        }
    }

    if (st17h66b_spi1_init_p34(WS2812B_SPI_BAUD_HZ) != 0) {
        for (;;) {
            __asm volatile ("nop");
        }
    }

    (void)st17h66b_spi1_write(spi_frame, encoded);

    /*
     * One static frame is enough for first bring-up. WS2812B pixels retain the
     * latched color until another valid frame arrives or power is removed.
     */
    for (;;) {
        __asm volatile ("nop");
    }
}
