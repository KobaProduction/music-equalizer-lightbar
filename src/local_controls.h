#pragma once

#include "ble_control.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    MELB_LOCAL_BUTTON_POLL_MS = 10,
    MELB_LOCAL_BUTTON_DEBOUNCE_TICKS = 4,
    MELB_LOCAL_BUTTON_LONG_TICKS = 60,
    MELB_LOCAL_BUTTON_REPEAT_TICKS = 15,
};

typedef struct {
    uint8_t candidate_pressed;
    uint8_t stable_pressed;
    uint8_t debounce_ticks;
    uint16_t hold_ticks;
    uint16_t repeat_ticks;
    uint8_t long_active;
} melb_local_button_t;

typedef struct {
    melb_local_button_t power;
    melb_local_button_t color_bright;
    melb_local_button_t mode_speed;
    uint8_t palette_index;
    uint8_t mode_index;
} melb_local_controls_t;

void melb_local_controls_init(melb_local_controls_t *controls);

bool melb_local_controls_tick(
    melb_local_controls_t *controls,
    melb_control_state_t *state,
    bool power_pressed,
    bool color_bright_pressed,
    bool mode_speed_pressed);
