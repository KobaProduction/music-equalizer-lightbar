#pragma once

#include "ble_control.h"

#include <stddef.h>
#include <stdint.h>

#define LOTUS_LANTERN_SERVICE_UUID16 UINT16_C(0xFFF0)
#define LOTUS_LANTERN_WRITE_UUID16   UINT16_C(0xFFF3)
#define LOTUS_LANTERN_FRAME_SIZE     9u
#define LOTUS_LANTERN_DEVICE_NAME    "ELK-BLEDOM-MELB"

typedef enum {
    LOTUS_LANTERN_OK = 0,
    LOTUS_LANTERN_ERR_ARGUMENT,
    LOTUS_LANTERN_ERR_LENGTH,
    LOTUS_LANTERN_ERR_FRAME,
    LOTUS_LANTERN_ERR_UNSUPPORTED,
    LOTUS_LANTERN_ERR_VALUE,
} lotus_lantern_result_t;

lotus_lantern_result_t lotus_lantern_apply_frame(
    melb_control_state_t *state,
    const uint8_t *frame,
    size_t frame_size);
