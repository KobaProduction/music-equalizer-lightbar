#include "ble_control.h"
#include "board.h"
#include "happylighting.h"
#include "local_controls.h"
#include "ws2812b.h"
#include "ws2812b_spi.h"

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

static void test_ws2812b_spi_encoding(void)
{
    const ws2812b_pixel_t black = {0};
    const ws2812b_pixel_t white = {
        .red = 0xffu,
        .green = 0xffu,
        .blue = 0xffu,
    };

    uint8_t black_encoded[WS2812B_SPI_BYTES_PER_PIXEL] = {0};
    uint8_t white_encoded[WS2812B_SPI_BYTES_PER_PIXEL] = {0};

    assert(ws2812b_spi_encode(
        &black, 1u, black_encoded, sizeof(black_encoded)) == sizeof(black_encoded));
    assert(ws2812b_spi_encode(
        &white, 1u, white_encoded, sizeof(white_encoded)) == sizeof(white_encoded));

    for (size_t i = 0u; i < sizeof(black_encoded); ++i) {
        assert(black_encoded[i] == 0x88u);
        assert(white_encoded[i] == 0xeeu);
    }

    assert(ws2812b_spi_encoded_size(32u) == 384u);
}


static void test_happylighting_protocol(void)
{
    melb_control_state_t state = {0};
    uint8_t status[HAPPY_LIGHTING_STATUS_SIZE] = {0};

    melb_control_state_init(&state);

    const uint8_t power_off[] = {0xCCu, 0x24u, 0x33u};
    const uint8_t power_on[] = {0xCCu, 0x23u, 0x33u};
    const uint8_t rgb[] = {0x56u, 0x11u, 0x22u, 0x33u, 0x00u, 0xF0u, 0xAAu};
    const uint8_t white[] = {0x56u, 0xDEu, 0xADu, 0xFFu, 0x40u, 0x0Fu, 0xAAu};
    const uint8_t effect[] = {0xBBu, 0x2Du, 0x1Fu, 0x44u};
    const uint8_t status_request[] = {0xEFu, 0x01u, 0x77u};

    assert(happy_lighting_apply_command(&state, power_off, sizeof(power_off)) == HAPPY_LIGHTING_OK);
    assert(state.power == 0u);

    assert(happy_lighting_apply_command(&state, power_on, sizeof(power_on)) == HAPPY_LIGHTING_OK);
    assert(state.power == 1u);

    assert(happy_lighting_apply_command(&state, rgb, sizeof(rgb)) == HAPPY_LIGHTING_OK);
    assert(state.red == 0x11u);
    assert(state.green == 0x22u);
    assert(state.blue == 0x33u);
    assert(state.mode == HAPPY_LIGHTING_MODE_STATIC);
    assert(state.brightness == 255u);

    assert(happy_lighting_apply_command(&state, white, sizeof(white)) == HAPPY_LIGHTING_OK);
    assert(state.red == 0x40u);
    assert(state.green == 0x40u);
    assert(state.blue == 0x40u);
    assert(state.mode == HAPPY_LIGHTING_MODE_STATIC);

    assert(happy_lighting_apply_command(&state, effect, sizeof(effect)) == HAPPY_LIGHTING_OK);
    assert(state.mode == 0x2Du);
    assert(state.speed == 0x1Fu);

    assert(happy_lighting_apply_command(
        &state, status_request, sizeof(status_request)) == HAPPY_LIGHTING_STATUS_REQUEST);

    assert(happy_lighting_build_status(&state, status, sizeof(status)) == sizeof(status));
    assert(status[0] == 0x66u);
    assert(status[2] == 0x23u);
    assert(status[3] == 0x2Du);
    assert(status[5] == 0x1Fu);
    assert(status[6] == 0x40u);
    assert(status[7] == 0x40u);
    assert(status[8] == 0x40u);
    assert(status[11] == 0x99u);

    const uint8_t bad_power[] = {0xCCu, 0x99u, 0x33u};
    const uint8_t bad_effect[] = {0xBBu, 0x10u, 0x20u, 0x44u};
    const uint8_t unknown[] = {0x01u, 0x02u};

    assert(happy_lighting_apply_command(
        &state, bad_power, sizeof(bad_power)) == HAPPY_LIGHTING_ERR_FRAME);
    assert(happy_lighting_apply_command(
        &state, bad_effect, sizeof(bad_effect)) == HAPPY_LIGHTING_ERR_FRAME);
    assert(happy_lighting_apply_command(
        &state, unknown, sizeof(unknown)) == HAPPY_LIGHTING_ERR_UNSUPPORTED);
}


static void run_button_ticks(
    melb_local_controls_t *controls,
    melb_control_state_t *state,
    bool power,
    bool color,
    bool mode,
    unsigned ticks)
{
    for (unsigned i = 0; i < ticks; ++i) {
        (void)melb_local_controls_tick(
            controls, state, power, color, mode);
    }
}

static void test_local_buttons(void)
{
    melb_local_controls_t controls;
    melb_control_state_t state;

    melb_control_state_init(&state);
    melb_local_controls_init(&controls);

    run_button_ticks(&controls, &state, true, false, false, 5u);
    assert(state.power == 0u);
    run_button_ticks(
        &controls,
        &state,
        true,
        false,
        false,
        MELB_LOCAL_BUTTON_LONG_TICKS + MELB_LOCAL_BUTTON_REPEAT_TICKS + 2u);
    assert(state.power == 0u);
    run_button_ticks(&controls, &state, false, false, false, 5u);
    assert(state.power == 0u);

    run_button_ticks(&controls, &state, false, true, false, 5u);
    run_button_ticks(&controls, &state, false, false, false, 5u);
    assert(state.green == 255u);
    assert(state.red == 0u);
    assert(state.mode == HAPPY_LIGHTING_MODE_STATIC);

    const uint8_t old_brightness = state.brightness;
    run_button_ticks(
        &controls,
        &state,
        false,
        true,
        false,
        MELB_LOCAL_BUTTON_LONG_TICKS + MELB_LOCAL_BUTTON_DEBOUNCE_TICKS + 1u);
    assert(state.brightness != old_brightness);
    run_button_ticks(&controls, &state, false, false, false, 5u);

    run_button_ticks(&controls, &state, false, false, true, 5u);
    run_button_ticks(&controls, &state, false, false, false, 5u);
    assert(state.mode == 0x2Du);

    const uint8_t old_speed = state.speed;
    run_button_ticks(
        &controls,
        &state,
        false,
        false,
        true,
        MELB_LOCAL_BUTTON_LONG_TICKS + MELB_LOCAL_BUTTON_DEBOUNCE_TICKS + 1u);
    assert(state.speed != old_speed);
}

static void test_music_light_v3_confirmed_pin_map(void)
{
    const melb_board_config_t *board = melb_board_config();

    assert(strcmp(board->pcb_marking, "Music-Light-V3-221101") == 0);
    assert(board->led_count == 32u);

    assert(board->led_data_pin == ST17H66B_PIN_P34);

    assert(board->programming_uart_tx_pin == ST17H66B_PIN_P9);
    assert(board->programming_uart_rx_pin == ST17H66B_PIN_P10);

    assert(board->button_power_pin == ST17H66B_PIN_P11);
    assert(board->button_color_bright_pin == ST17H66B_PIN_P3);
    assert(board->button_mode_speed_pin == ST17H66B_PIN_P7);

    assert(board->microphone_pin == ST17H66B_PIN_AIO4);

    assert(ST17H66B_PACKAGE_PIN_P9 == 5u);
    assert(ST17H66B_PACKAGE_PIN_P10 == 6u);
    assert(ST17H66B_PACKAGE_PIN_P15 == 9u);
}

static void test_control_defaults(void)
{
    melb_control_state_t state = {0};

    melb_control_state_init(&state);

    assert(state.power == 1u);
    assert(state.brightness == 128u);
    assert(state.red == 0u);
    assert(state.green == 0u);
    assert(state.blue == 255u);
    assert(state.mode == 0x41u);
    assert(state.speed == 24u);
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
    test_ws2812b_spi_encoding();
    test_happylighting_protocol();
    test_local_buttons();
    test_music_light_v3_confirmed_pin_map();
    test_control_defaults();
    test_control_packets();
    test_control_rejects_invalid_packets();

    return 0;
}
