#pragma once

#include "ble_control.h"

#include <stddef.h>
#include <stdint.h>

enum {
    HAPPY_LIGHTING_SERVICE_UUID16 = 0xFFD5,
    HAPPY_LIGHTING_WRITE_UUID16 = 0xFFD9,
    HAPPY_LIGHTING_NOTIFY_UUID16 = 0xFFD4,
    HAPPY_LIGHTING_STATUS_SIZE = 12,
    HAPPY_LIGHTING_MODE_STATIC = 0x41,
    HAPPY_LIGHTING_MODE_MIN = 0x25,
    HAPPY_LIGHTING_MODE_MAX = 0x38,
};

typedef enum {
    HAPPY_LIGHTING_OK = 0,
    HAPPY_LIGHTING_STATUS_REQUEST,
    HAPPY_LIGHTING_ERR_ARGUMENT,
    HAPPY_LIGHTING_ERR_FRAME,
    HAPPY_LIGHTING_ERR_UNSUPPORTED,
} happy_lighting_result_t;

happy_lighting_result_t happy_lighting_apply_command(
    melb_control_state_t *state,
    const uint8_t *data,
    size_t size);

size_t happy_lighting_build_status(
    const melb_control_state_t *state,
    uint8_t *output,
    size_t output_size);
