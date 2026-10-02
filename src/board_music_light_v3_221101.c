#include "board.h"

static const melb_board_config_t board_config = {
    .board_name = "Music Equalizer Light Bar",
    .pcb_marking = "Music-Light-V3-221101",
    .mcu_name = "ST17H66B",
    .led_count = 32u,

    /*
     * These stay unmapped until continuity tracing establishes the actual
     * Music-Light-V3-221101 connections.
     */
    .led_data_pin = MELB_PIN_UNMAPPED,
    .microphone_pin = MELB_PIN_UNMAPPED,
};

const melb_board_config_t *melb_board_config(void)
{
    return &board_config;
}
