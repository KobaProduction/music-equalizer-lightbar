#include "board.h"

static const melb_board_config_t board_config = {
    .board_name = "Music Equalizer Light Bar",
    .pcb_marking = "Music-Light-V3-221101",
    .mcu_name = "ST17H66B",
    .led_count = 32u,

    /*
     * Confirmed by continuity tracing on the inspected PCB.
     */
    .led_data_pin = ST17H66B_PIN_P34,

    .programming_uart_tx_pin = ST17H66B_PIN_P9,
    .programming_uart_rx_pin = ST17H66B_PIN_P10,

    .button_power_pin = ST17H66B_PIN_P11,
    .button_color_bright_pin = ST17H66B_PIN_P3,
    .button_mode_speed_pin = ST17H66B_PIN_P7,

    .microphone_pin = ST17H66B_PIN_P15,
};

const melb_board_config_t *melb_board_config(void)
{
    return &board_config;
}
