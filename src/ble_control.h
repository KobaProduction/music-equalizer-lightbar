#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
    MELB_CONTROL_OK = 0,
    MELB_CONTROL_ERR_ARGUMENT,
    MELB_CONTROL_ERR_LENGTH,
    MELB_CONTROL_ERR_OPCODE,
    MELB_CONTROL_ERR_VALUE,
} melb_control_result_t;

typedef enum {
    MELB_CONTROL_SET_POWER = 0x01,
    MELB_CONTROL_SET_BRIGHTNESS = 0x02,
    MELB_CONTROL_SET_RGB = 0x03,
    MELB_CONTROL_SET_MODE = 0x04,
    MELB_CONTROL_SET_SPEED = 0x05,
    MELB_CONTROL_SET_AUDIO_REACTIVE = 0x06,
} melb_control_opcode_t;

typedef struct {
    uint8_t power;
    uint8_t brightness;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t mode;
    uint8_t speed;
    uint8_t audio_reactive;
} melb_control_state_t;

void melb_control_state_init(melb_control_state_t *state);

melb_control_result_t melb_control_apply_packet(
    melb_control_state_t *state,
    const uint8_t *packet,
    size_t packet_size);
