#pragma once

#include "st17h66b_pins.h"

#include <stdint.h>

typedef struct {
    const char *board_name;
    const char *pcb_marking;
    const char *mcu_name;
    uint8_t led_count;

    st17h66b_pin_t led_data_pin;

    /*
     * ROM-UART direction is named from the target MCU perspective:
     * target TX connects to adapter RX; target RX connects to adapter TX.
     */
    st17h66b_pin_t programming_uart_tx_pin;
    st17h66b_pin_t programming_uart_rx_pin;

    /*
     * Confirmed controls on Music-Light-V3-221101.
     */
    st17h66b_pin_t button_power_pin;
    st17h66b_pin_t button_color_bright_pin;
    st17h66b_pin_t button_mode_speed_pin;

    /*
     * Confirmed analog microphone path.
     * P15 also carries AIO_4 / ADC4 and can provide MICBIAS.
     */
    st17h66b_pin_t microphone_pin;
} melb_board_config_t;

const melb_board_config_t *melb_board_config(void);
