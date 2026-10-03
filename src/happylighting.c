#include "happylighting.h"

static int is_effect_mode(uint8_t mode)
{
    return mode >= HAPPY_LIGHTING_MODE_MIN
        && mode <= HAPPY_LIGHTING_MODE_MAX;
}

happy_lighting_result_t happy_lighting_apply_command(
    melb_control_state_t *state,
    const uint8_t *data,
    size_t size)
{
    if (state == NULL || data == NULL) {
        return HAPPY_LIGHTING_ERR_ARGUMENT;
    }

    if (size == 3u && data[0] == 0xEFu && data[1] == 0x01u && data[2] == 0x77u) {
        return HAPPY_LIGHTING_STATUS_REQUEST;
    }

    if (size == 3u && data[0] == 0xCCu && data[2] == 0x33u) {
        if (data[1] == 0x23u) {
            state->power = 1u;
            return HAPPY_LIGHTING_OK;
        }
        if (data[1] == 0x24u) {
            state->power = 0u;
            return HAPPY_LIGHTING_OK;
        }
        return HAPPY_LIGHTING_ERR_FRAME;
    }

    if (size == 7u && data[0] == 0x56u && data[6] == 0xAAu) {
        if (data[5] == 0xF0u) {
            state->red = data[1];
            state->green = data[2];
            state->blue = data[3];
            state->brightness = 255u;
            state->mode = HAPPY_LIGHTING_MODE_STATIC;
            state->audio_reactive = 0u;
            return HAPPY_LIGHTING_OK;
        }

        if (data[5] == 0x0Fu) {
            state->red = data[4];
            state->green = data[4];
            state->blue = data[4];
            state->brightness = 255u;
            state->mode = HAPPY_LIGHTING_MODE_STATIC;
            state->audio_reactive = 0u;
            return HAPPY_LIGHTING_OK;
        }

        return HAPPY_LIGHTING_ERR_FRAME;
    }

    if (size == 4u && data[0] == 0xBBu && data[3] == 0x44u) {
        if (!is_effect_mode(data[1])) {
            return HAPPY_LIGHTING_ERR_FRAME;
        }

        state->mode = data[1];
        state->speed = data[2] == 0u ? 1u : data[2];
        state->audio_reactive = 0u;
        return HAPPY_LIGHTING_OK;
    }

    return HAPPY_LIGHTING_ERR_UNSUPPORTED;
}

size_t happy_lighting_build_status(
    const melb_control_state_t *state,
    uint8_t *output,
    size_t output_size)
{
    if (state == NULL || output == NULL || output_size < HAPPY_LIGHTING_STATUS_SIZE) {
        return 0u;
    }

    output[0] = 0x66u;
    output[1] = 0x15u;
    output[2] = state->power != 0u ? 0x23u : 0x24u;
    output[3] = state->mode;
    output[4] = 0x20u;
    output[5] = state->mode == HAPPY_LIGHTING_MODE_STATIC ? 0u : state->speed;
    output[6] = state->red;
    output[7] = state->green;
    output[8] = state->blue;
    output[9] = 0u;
    output[10] = 0x06u;
    output[11] = 0x99u;

    return HAPPY_LIGHTING_STATUS_SIZE;
}
