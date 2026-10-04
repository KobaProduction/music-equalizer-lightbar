#include "local_controls.h"
#include "happylighting.h"

#include <stddef.h>

typedef enum {
    MELB_BUTTON_EVENT_NONE = 0,
    MELB_BUTTON_EVENT_PRESS,
    MELB_BUTTON_EVENT_SHORT,
    MELB_BUTTON_EVENT_LONG_STEP,
} melb_button_event_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} melb_palette_color_t;

static const melb_palette_color_t palette[] = {
    {255u, 0u, 0u},
    {0u, 255u, 0u},
    {0u, 0u, 255u},
    {0u, 255u, 255u},
    {255u, 0u, 255u},
    {255u, 255u, 0u},
    {255u, 96u, 16u},
    {255u, 180u, 96u},
};

static const uint8_t local_modes[] = {
    0x25u, /* seven-color cross fade */
    0x2Du, /* red/green cross fade */
    0x30u, /* seven-color strobe */
    0x38u, /* seven-color jump */
};

static melb_button_event_t update_button(
    melb_local_button_t *button,
    bool pressed)
{
    const uint8_t raw = pressed ? 1u : 0u;

    if (raw != button->candidate_pressed) {
        button->candidate_pressed = raw;
        button->debounce_ticks = 1u;
        return MELB_BUTTON_EVENT_NONE;
    }

    if (button->debounce_ticks < MELB_LOCAL_BUTTON_DEBOUNCE_TICKS) {
        ++button->debounce_ticks;

        if (button->debounce_ticks == MELB_LOCAL_BUTTON_DEBOUNCE_TICKS
            && button->stable_pressed != raw) {
            button->stable_pressed = raw;

            if (raw != 0u) {
                button->hold_ticks = 0u;
                button->repeat_ticks = 0u;
                button->long_active = 0u;
                return MELB_BUTTON_EVENT_PRESS;
            }

            const uint8_t was_long = button->long_active;
            button->hold_ticks = 0u;
            button->repeat_ticks = 0u;
            button->long_active = 0u;

            if (was_long == 0u) {
                return MELB_BUTTON_EVENT_SHORT;
            }
        }

        return MELB_BUTTON_EVENT_NONE;
    }

    if (button->stable_pressed == 0u) {
        return MELB_BUTTON_EVENT_NONE;
    }

    if (button->hold_ticks < UINT16_MAX) {
        ++button->hold_ticks;
    }

    if (button->long_active == 0u) {
        if (button->hold_ticks >= MELB_LOCAL_BUTTON_LONG_TICKS) {
            button->long_active = 1u;
            button->repeat_ticks = 0u;
            return MELB_BUTTON_EVENT_LONG_STEP;
        }

        return MELB_BUTTON_EVENT_NONE;
    }

    if (++button->repeat_ticks >= MELB_LOCAL_BUTTON_REPEAT_TICKS) {
        button->repeat_ticks = 0u;
        return MELB_BUTTON_EVENT_LONG_STEP;
    }

    return MELB_BUTTON_EVENT_NONE;
}

static uint8_t stepped_brightness(uint8_t value)
{
    if (value < 32u || value >= 192u) {
        return 32u;
    }

    return (uint8_t)(value + 32u);
}

static uint8_t stepped_speed(uint8_t value)
{
    if (value < 16u || value >= 224u) {
        return 32u;
    }

    return (uint8_t)(value + 32u);
}

void melb_local_controls_init(melb_local_controls_t *controls)
{
    if (controls == NULL) {
        return;
    }

    *controls = (melb_local_controls_t){0};
}

bool melb_local_controls_tick(
    melb_local_controls_t *controls,
    melb_control_state_t *state,
    bool power_pressed,
    bool color_bright_pressed,
    bool mode_speed_pressed)
{
    if (controls == NULL || state == NULL) {
        return false;
    }

    bool changed = false;

    const melb_button_event_t power_event =
        update_button(&controls->power, power_pressed);
    const melb_button_event_t color_event =
        update_button(&controls->color_bright, color_bright_pressed);
    const melb_button_event_t mode_event =
        update_button(&controls->mode_speed, mode_speed_pressed);

    if (power_event == MELB_BUTTON_EVENT_SHORT) {
        state->power = state->power == 0u ? 1u : 0u;
        changed = true;
    }

    if (color_event == MELB_BUTTON_EVENT_SHORT) {
        controls->palette_index =
            (uint8_t)((controls->palette_index + 1u)
                % (sizeof(palette) / sizeof(palette[0])));

        const melb_palette_color_t color = palette[controls->palette_index];
        state->red = color.red;
        state->green = color.green;
        state->blue = color.blue;
        state->mode = HAPPY_LIGHTING_MODE_STATIC;
        state->audio_reactive = 0u;
        changed = true;
    } else if (color_event == MELB_BUTTON_EVENT_LONG_STEP) {
        state->brightness = stepped_brightness(state->brightness);
        changed = true;
    }

    if (mode_event == MELB_BUTTON_EVENT_SHORT) {
        controls->mode_index =
            (uint8_t)((controls->mode_index + 1u)
                % (sizeof(local_modes) / sizeof(local_modes[0])));
        state->mode = local_modes[controls->mode_index];
        state->audio_reactive = 0u;
        changed = true;
    } else if (mode_event == MELB_BUTTON_EVENT_LONG_STEP) {
        state->speed = stepped_speed(state->speed);
        changed = true;
    }

    return changed;
}
