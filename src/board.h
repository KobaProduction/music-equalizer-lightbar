#pragma once

#include <stdint.h>

#define MELB_PIN_UNMAPPED UINT8_C(0xff)

typedef struct {
    const char *board_name;
    const char *pcb_marking;
    const char *mcu_name;
    uint8_t led_count;
    uint8_t led_data_pin;
    uint8_t microphone_pin;
} melb_board_config_t;

const melb_board_config_t *melb_board_config(void);
