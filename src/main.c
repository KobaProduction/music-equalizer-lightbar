#include "board.h"
#include "ws2812b.h"

enum {
    MELB_LED_COUNT = 32,
    MELB_WS2812B_FRAME_BYTES = MELB_LED_COUNT * WS2812B_BYTES_PER_PIXEL,
};

static ws2812b_pixel_t frame[MELB_LED_COUNT];
static uint8_t serialized_frame[MELB_WS2812B_FRAME_BYTES];

int main(void)
{
    const melb_board_config_t *board = melb_board_config();

    ws2812b_clear(frame, board->led_count);
    (void)ws2812b_serialize_grb(
        frame,
        board->led_count,
        serialized_frame,
        sizeof(serialized_frame));

    /*
     * Hardware output is deliberately not attempted until the actual board
     * GPIO mapping is continuity-tested.
     */
    for (;;) {
        __asm volatile ("nop");
    }
}
