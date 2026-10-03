#include "ble_control.h"

static int melb_control_boolean_is_valid(uint8_t value)
{
    return value <= 1u;
}

void melb_control_state_init(melb_control_state_t *state)
{
    if (state == NULL) {
        return;
    }

    state->power = 1u;
    state->brightness = 128u;
    state->red = 255u;
    state->green = 64u;
    state->blue = 0u;
    state->mode = 0x25u;
    state->speed = 24u;
    state->audio_reactive = 0u;
}

melb_control_result_t melb_control_apply_packet(
    melb_control_state_t *state,
    const uint8_t *packet,
    size_t packet_size)
{
    if (state == NULL || packet == NULL) {
        return MELB_CONTROL_ERR_ARGUMENT;
    }

    if (packet_size < 1u) {
        return MELB_CONTROL_ERR_LENGTH;
    }

    switch ((melb_control_opcode_t)packet[0]) {
    case MELB_CONTROL_SET_POWER:
        if (packet_size != 2u) {
            return MELB_CONTROL_ERR_LENGTH;
        }
        if (!melb_control_boolean_is_valid(packet[1])) {
            return MELB_CONTROL_ERR_VALUE;
        }
        state->power = packet[1];
        return MELB_CONTROL_OK;

    case MELB_CONTROL_SET_BRIGHTNESS:
        if (packet_size != 2u) {
            return MELB_CONTROL_ERR_LENGTH;
        }
        state->brightness = packet[1];
        return MELB_CONTROL_OK;

    case MELB_CONTROL_SET_RGB:
        if (packet_size != 4u) {
            return MELB_CONTROL_ERR_LENGTH;
        }
        state->red = packet[1];
        state->green = packet[2];
        state->blue = packet[3];
        return MELB_CONTROL_OK;

    case MELB_CONTROL_SET_MODE:
        if (packet_size != 2u) {
            return MELB_CONTROL_ERR_LENGTH;
        }
        state->mode = packet[1];
        return MELB_CONTROL_OK;

    case MELB_CONTROL_SET_SPEED:
        if (packet_size != 2u) {
            return MELB_CONTROL_ERR_LENGTH;
        }
        state->speed = packet[1];
        return MELB_CONTROL_OK;

    case MELB_CONTROL_SET_AUDIO_REACTIVE:
        if (packet_size != 2u) {
            return MELB_CONTROL_ERR_LENGTH;
        }
        if (!melb_control_boolean_is_valid(packet[1])) {
            return MELB_CONTROL_ERR_VALUE;
        }
        state->audio_reactive = packet[1];
        return MELB_CONTROL_OK;

    default:
        return MELB_CONTROL_ERR_OPCODE;
    }
}
