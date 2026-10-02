#include "ble_control.h"
#include "ws2812b.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void test_ws2812b_grb_serialization(void)
{
    const ws2812b_pixel_t pixels[] = {
        {.red = 0x11u, .green = 0x22u, .blue = 0x33u},
        {.red = 0xaau, .green = 0xbbu, .blue = 0xccu},
    };
    uint8_t output[6] = {0};
    const uint8_t expected[] = {0x22u, 0x11u, 0x33u, 0xbbu, 0xaau, 0xccu};

    assert(ws2812b_serialize_grb(pixels, 2u, output, sizeof(output)) == sizeof(output));
    assert(memcmp(output, expected, sizeof(expected)) == 0);
}

static void test_ws2812b_rejects_short_buffer(void)
{
    const ws2812b_pixel_t pixel = {.red = 1u, .green = 2u, .blue = 3u};
    uint8_t output[2] = {0};

    assert(ws2812b_serialize_grb(&pixel, 1u, output, sizeof(output)) == 0u);
}

static void test_control_defaults(void)
{
    melb_control_state_t state = {0};

    melb_control_state_init(&state);

    assert(state.power == 1u);
    assert(state.brightness == 255u);
    assert(state.red == 255u);
    assert(state.green == 255u);
    assert(state.blue == 255u);
    assert(state.mode == 0u);
    assert(state.speed == 128u);
    assert(state.audio_reactive == 0u);
}

static void test_control_packets(void)
{
    melb_control_state_t state = {0};
    const uint8_t power_off[] = {MELB_CONTROL_SET_POWER, 0u};
    const uint8_t brightness[] = {MELB_CONTROL_SET_BRIGHTNESS, 64u};
    const uint8_t rgb[] = {MELB_CONTROL_SET_RGB, 10u, 20u, 30u};
    const uint8_t mode[] = {MELB_CONTROL_SET_MODE, 7u};
    const uint8_t speed[] = {MELB_CONTROL_SET_SPEED, 200u};
    const uint8_t audio[] = {MELB_CONTROL_SET_AUDIO_REACTIVE, 1u};

    melb_control_state_init(&state);

    assert(melb_control_apply_packet(&state, power_off, sizeof(power_off)) == MELB_CONTROL_OK);
    assert(melb_control_apply_packet(&state, brightness, sizeof(brightness)) == MELB_CONTROL_OK);
    assert(melb_control_apply_packet(&state, rgb, sizeof(rgb)) == MELB_CONTROL_OK);
    assert(melb_control_apply_packet(&state, mode, sizeof(mode)) == MELB_CONTROL_OK);
    assert(melb_control_apply_packet(&state, speed, sizeof(speed)) == MELB_CONTROL_OK);
    assert(melb_control_apply_packet(&state, audio, sizeof(audio)) == MELB_CONTROL_OK);

    assert(state.power == 0u);
    assert(state.brightness == 64u);
    assert(state.red == 10u);
    assert(state.green == 20u);
    assert(state.blue == 30u);
    assert(state.mode == 7u);
    assert(state.speed == 200u);
    assert(state.audio_reactive == 1u);
}

static void test_control_rejects_invalid_packets(void)
{
    melb_control_state_t state = {0};
    const uint8_t unknown[] = {0xffu};
    const uint8_t bad_bool[] = {MELB_CONTROL_SET_POWER, 2u};
    const uint8_t short_rgb[] = {MELB_CONTROL_SET_RGB, 1u, 2u};

    melb_control_state_init(&state);

    assert(melb_control_apply_packet(&state, unknown, sizeof(unknown)) == MELB_CONTROL_ERR_OPCODE);
    assert(melb_control_apply_packet(&state, bad_bool, sizeof(bad_bool)) == MELB_CONTROL_ERR_VALUE);
    assert(melb_control_apply_packet(&state, short_rgb, sizeof(short_rgb)) == MELB_CONTROL_ERR_LENGTH);
    assert(melb_control_apply_packet(NULL, unknown, sizeof(unknown)) == MELB_CONTROL_ERR_ARGUMENT);
}

int main(void)
{
    test_ws2812b_grb_serialization();
    test_ws2812b_rejects_short_buffer();
    test_control_defaults();
    test_control_packets();
    test_control_rejects_invalid_packets();

    return 0;
}
