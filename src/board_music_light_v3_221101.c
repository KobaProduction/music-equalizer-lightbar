#include "board.h"

static const melb_board_config_t board_config = {
    .board_name = "Music Equalizer Light Bar",
    .pcb_marking = "Music-Light-V3-221101",
    .mcu_name = "ST17H66B",
    .led_count = 32u,

    /*
     * Confirmed by continuity tracing on the inspected PCB.
     */
    .programming_uart_tx_pin = ST17H66B_PIN_P9,
    .programming_uart_rx_pin = ST17H66B_PIN_P10,
    .microphone_pin = ST17H66B_PIN_P15,

    /*
     * LED data remains the last required signal to trace.
     */
    .led_data_pin = ST17H66B_PIN_UNMAPPED,
};

const melb_board_config_t *melb_board_config(void)
{
    return &board_config;
}
