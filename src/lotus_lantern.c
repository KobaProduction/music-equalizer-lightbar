#include "lotus_lantern.h"

static int valid_boolean(uint8_t value)
{
    return value <= 1u;
}

lotus_lantern_result_t lotus_lantern_apply_frame(
    melb_control_state_t *state,
    const uint8_t *frame,
    size_t frame_size)
{
    if (state == NULL || frame == NULL) {
        return LOTUS_LANTERN_ERR_ARGUMENT;
    }

    if (frame_size != LOTUS_LANTERN_FRAME_SIZE) {
        return LOTUS_LANTERN_ERR_LENGTH;
    }

    if (frame[0] != UINT8_C(0x7E) || frame[8] != UINT8_C(0xEF)) {
        return LOTUS_LANTERN_ERR_FRAME;
    }

    switch (frame[2]) {
    case 0x01: /* Brightness */
        if (frame[1] != 0x04u) {
            return LOTUS_LANTERN_ERR_FRAME;
        }
        state->brightness = frame[3];
        return LOTUS_LANTERN_OK;

    case 0x02: /* Dynamic-mode speed */
        if (frame[1] != 0x04u) {
            return LOTUS_LANTERN_ERR_FRAME;
        }
        state->speed = frame[3];
        return LOTUS_LANTERN_OK;

    case 0x03: /* Dynamic mode / external mic EQ family */
        if (frame[1] != 0x05u) {
            return LOTUS_LANTERN_ERR_FRAME;
        }

        if (frame[4] == 0x03u) {
            state->mode = (uint8_t)(frame[3] & UINT8_C(0x7F));
            return LOTUS_LANTERN_OK;
        }

        if (frame[4] == 0x04u) {
            state->audio_reactive = 1u;
            state->mode = (uint8_t)(frame[3] & UINT8_C(0x7F));
            return LOTUS_LANTERN_OK;
        }

        return LOTUS_LANTERN_ERR_UNSUPPORTED;

    case 0x04: /* Power / RGBW-status family */
        if (frame[1] != 0x04u) {
            return LOTUS_LANTERN_ERR_FRAME;
        }

        if (frame[4] == 0u && frame[5] == frame[3]) {
            if (!valid_boolean(frame[3])) {
                return LOTUS_LANTERN_ERR_VALUE;
            }
            state->power = frame[3];
            return LOTUS_LANTERN_OK;
        }

        return LOTUS_LANTERN_ERR_UNSUPPORTED;

    case 0x05: /* RGB / color-temperature / music-amplitude family */
        if (frame[3] != 0x03u || frame[1] != 0x07u) {
            return LOTUS_LANTERN_ERR_UNSUPPORTED;
        }

        if (frame[7] != 0x10u && frame[7] != 0x20u) {
            return LOTUS_LANTERN_ERR_UNSUPPORTED;
        }

        state->red = frame[4];
        state->green = frame[5];
        state->blue = frame[6];

        if (frame[7] == 0x20u) {
            state->audio_reactive = 1u;
        }

        return LOTUS_LANTERN_OK;

    case 0x06: /* External microphone sensitivity */
        if (frame[1] != 0x04u) {
            return LOTUS_LANTERN_ERR_FRAME;
        }
        /*
         * Sensitivity has no repository-owned state field yet. Accept the
         * command for app compatibility but do not invent a hardware effect.
         */
        return LOTUS_LANTERN_OK;

    case 0x07: /* External microphone on/off */
        if (frame[1] != 0x04u || !valid_boolean(frame[3])) {
            return LOTUS_LANTERN_ERR_VALUE;
        }
        state->audio_reactive = frame[3];
        return LOTUS_LANTERN_OK;

    case 0x81: /* RGB pin order */
        /*
         * Our physical WS2812 path is fixed to GRB serialization. Accept the
         * compatibility command without changing the board wiring contract.
         */
        if (frame[1] != 0x06u) {
            return LOTUS_LANTERN_ERR_FRAME;
        }
        return LOTUS_LANTERN_OK;

    default:
        return LOTUS_LANTERN_ERR_UNSUPPORTED;
    }
}
