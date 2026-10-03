#include <stdint.h>

#include "bcomdef.h"
#include "sbp_profile.h"

#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "ble_control.h"
#include "lotus_lantern.h"
#include "local_controls.h"
#include "log.h"
#include "gpio.h"
#include "st17h66b_spi1.h"
#include "ws2812b.h"
#include "ws2812b_spi.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
    MELB_LOTUS_LED_COUNT = 32,
    MELB_LOTUS_SPI_FRAME_SIZE =
        (MELB_LOTUS_LED_COUNT * WS2812B_SPI_BYTES_PER_PIXEL)
        + WS2812B_SPI_RESET_BYTES,
};

static melb_control_state_t control_state;
static ws2812b_pixel_t pixels[MELB_LOTUS_LED_COUNT];
static uint8_t spi_frame[MELB_LOTUS_SPI_FRAME_SIZE];
static int renderer_ready;
static melb_local_controls_t local_controls;
static uint8_t animation_phase;
static uint8_t animation_ticks;

static CONST uint8 lotus_service_uuid[ATT_BT_UUID_SIZE] = {
    LO_UINT16(LOTUS_LANTERN_SERVICE_UUID16),
    HI_UINT16(LOTUS_LANTERN_SERVICE_UUID16),
};

static CONST uint8 lotus_write_uuid[ATT_BT_UUID_SIZE] = {
    LO_UINT16(LOTUS_LANTERN_WRITE_UUID16),
    HI_UINT16(LOTUS_LANTERN_WRITE_UUID16),
};

static CONST gattAttrType_t lotus_service = {
    ATT_BT_UUID_SIZE,
    lotus_service_uuid,
};

static CONST uint8 lotus_write_props =
    GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;

static uint8 lotus_write_value[LOTUS_LANTERN_FRAME_SIZE];

static uint8_t scale_channel(uint8_t channel, uint8_t brightness)
{
    return (uint8_t)(((uint16_t)channel * (uint16_t)brightness) / UINT16_C(255));
}

static ws2812b_pixel_t wheel(uint8_t pos)
{
    ws2812b_pixel_t color = {0};

    if (pos < 85u) {
        color.red = (uint8_t)(255u - (pos * 3u));
        color.green = (uint8_t)(pos * 3u);
    } else if (pos < 170u) {
        pos = (uint8_t)(pos - 85u);
        color.green = (uint8_t)(255u - (pos * 3u));
        color.blue = (uint8_t)(pos * 3u);
    } else {
        pos = (uint8_t)(pos - 170u);
        color.blue = (uint8_t)(255u - (pos * 3u));
        color.red = (uint8_t)(pos * 3u);
    }

    return color;
}

static void render_state(void)
{
    if (!renderer_ready) {
        return;
    }

    ws2812b_clear(pixels, MELB_LOTUS_LED_COUNT);

    if (control_state.power != 0u) {
        const uint8_t effect =
            control_state.mode == 0u
                ? 0u
                : (uint8_t)(((control_state.mode - 1u) % 3u) + 1u);

        if (effect == 0u) {
            const uint8_t red =
                scale_channel(control_state.red, control_state.brightness);
            const uint8_t green =
                scale_channel(control_state.green, control_state.brightness);
            const uint8_t blue =
                scale_channel(control_state.blue, control_state.brightness);

            for (size_t i = 0u; i < MELB_LOTUS_LED_COUNT; ++i) {
                pixels[i].red = red;
                pixels[i].green = green;
                pixels[i].blue = blue;
            }
        } else if (effect == 1u) {
            for (size_t i = 0u; i < MELB_LOTUS_LED_COUNT; ++i) {
                if (((i + animation_phase) & 3u) == 0u) {
                    pixels[i].red =
                        scale_channel(control_state.red, control_state.brightness);
                    pixels[i].green =
                        scale_channel(control_state.green, control_state.brightness);
                    pixels[i].blue =
                        scale_channel(control_state.blue, control_state.brightness);
                }
            }
        } else if (effect == 2u) {
            for (size_t i = 0u; i < MELB_LOTUS_LED_COUNT; ++i) {
                ws2812b_pixel_t color =
                    wheel((uint8_t)(animation_phase + (uint8_t)(i * 8u)));
                pixels[i].red =
                    scale_channel(color.red, control_state.brightness);
                pixels[i].green =
                    scale_channel(color.green, control_state.brightness);
                pixels[i].blue =
                    scale_channel(color.blue, control_state.brightness);
            }
        } else {
            const uint8_t triangle = animation_phase < 128u
                ? (uint8_t)(animation_phase * 2u)
                : (uint8_t)((255u - animation_phase) * 2u);
            const uint8_t level =
                scale_channel(control_state.brightness, triangle);

            for (size_t i = 0u; i < MELB_LOTUS_LED_COUNT; ++i) {
                pixels[i].red = scale_channel(control_state.red, level);
                pixels[i].green = scale_channel(control_state.green, level);
                pixels[i].blue = scale_channel(control_state.blue, level);
            }
        }
    }

    const size_t encoded = ws2812b_spi_encode(
        pixels,
        MELB_LOTUS_LED_COUNT,
        spi_frame,
        sizeof(spi_frame));

    if (encoded == sizeof(spi_frame)) {
        (void)st17h66b_spi1_write(spi_frame, encoded);
    }
}

void melb_lotus_local_init(void)
{
    melb_local_controls_init(&local_controls);

    hal_gpio_pin_init(GPIO_P11, GPIO_INPUT);
    hal_gpio_pull_set(GPIO_P11, GPIO_PULL_UP);
    hal_gpio_pin_init(GPIO_P03, GPIO_INPUT);
    hal_gpio_pull_set(GPIO_P03, GPIO_PULL_UP);
    hal_gpio_pin_init(GPIO_P07, GPIO_INPUT);
    hal_gpio_pull_set(GPIO_P07, GPIO_PULL_UP);
}

void melb_lotus_local_tick(void)
{
    const bool changed = melb_local_controls_tick(
        &local_controls,
        &control_state,
        !hal_gpio_read(GPIO_P11),
        !hal_gpio_read(GPIO_P03),
        !hal_gpio_read(GPIO_P07));

    if (changed) {
        LOG("MELB: button state power=%u mode=%u bright=%u speed=%u rgb=%u,%u,%u\n",
            control_state.power, control_state.mode, control_state.brightness,
            control_state.speed, control_state.red, control_state.green, control_state.blue);
        animation_ticks = 0u;
        render_state();
    }

    if (control_state.power == 0u || control_state.mode == 0u) {
        return;
    }

    const uint8_t interval =
        (uint8_t)(2u + (((uint16_t)(255u - control_state.speed) * 18u) / 255u));

    if (++animation_ticks >= interval) {
        animation_ticks = 0u;
        ++animation_phase;
        render_state();
    }
}

static bStatus_t lotus_read_attr(
    uint16 conn_handle,
    gattAttribute_t *attr,
    uint8 *value,
    uint16 *length,
    uint16 offset,
    uint8 max_length)
{
    (void)conn_handle;
    (void)attr;
    (void)value;
    (void)offset;
    (void)max_length;

    if (length != NULL) {
        *length = 0u;
    }

    return ATT_ERR_ATTR_NOT_FOUND;
}

static bStatus_t lotus_write_attr(
    uint16 conn_handle,
    gattAttribute_t *attr,
    uint8 *value,
    uint16 length,
    uint16 offset)
{
    (void)conn_handle;

    if (attr == NULL || value == NULL) {
        return ATT_ERR_UNLIKELY;
    }

    if (offset != 0u) {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    if (attr->type.len != ATT_BT_UUID_SIZE
        || BUILD_UINT16(attr->type.uuid[0], attr->type.uuid[1])
            != LOTUS_LANTERN_WRITE_UUID16) {
        return ATT_ERR_ATTR_NOT_FOUND;
    }

    if (length != LOTUS_LANTERN_FRAME_SIZE) {
        return ATT_ERR_INVALID_VALUE_SIZE;
    }

    LOG("MELB: Lotus write cmd=0x%02x p=%02x %02x %02x %02x %02x len=%u\n",
        value[2], value[3], value[4], value[5], value[6], value[7], length);

    if (lotus_lantern_apply_frame(&control_state, value, length)
        != LOTUS_LANTERN_OK) {
        LOG("MELB: Lotus frame rejected\n");
        return ATT_ERR_INVALID_VALUE;
    }

    memcpy(lotus_write_value, value, LOTUS_LANTERN_FRAME_SIZE);
    LOG("MELB: state power=%u mode=%u bright=%u speed=%u rgb=%u,%u,%u\n",
        control_state.power, control_state.mode, control_state.brightness,
        control_state.speed, control_state.red, control_state.green, control_state.blue);
    render_state();

    return SUCCESS;
}

static CONST gattServiceCBs_t lotus_callbacks = {
    lotus_read_attr,
    lotus_write_attr,
    NULL,
};

static gattAttribute_t lotus_attributes[] = {
    {
        { ATT_BT_UUID_SIZE, primaryServiceUUID },
        GATT_PERMIT_READ,
        0,
        (uint8 *)&lotus_service,
    },
    {
        { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ,
        0,
        (uint8 *)&lotus_write_props,
    },
    {
        { ATT_BT_UUID_SIZE, lotus_write_uuid },
        GATT_PERMIT_WRITE,
        0,
        lotus_write_value,
    },
};

bStatus_t SimpleProfile_AddService(uint32 services)
{
    (void)services;

    melb_control_state_init(&control_state);
    control_state.power = 0u;
    animation_phase = 0u;
    animation_ticks = 0u;

    renderer_ready =
        st17h66b_spi1_init_p34(WS2812B_SPI_BAUD_HZ) == 0;

    LOG("MELB: WS2812 SPI1/P34 init=%s\n", renderer_ready ? "ok" : "FAIL");
    if (renderer_ready) {
        /* Explicit black frame on every boot before BLE starts. */
        control_state.power = 0u;
        render_state();
        render_state();
        LOG("MELB: WS2812 black boot frame sent\n");
    }

    return GATTServApp_RegisterService(
        lotus_attributes,
        GATT_NUM_ATTRS(lotus_attributes),
        &lotus_callbacks);
}

void new_cmd_data(void) {}
void new_ota_data(void) {}
void wrk_notify(void) {}
void measure_notify(void) {}

uint16_t make_measure_msg(uint8_t *buffer)
{
    (void)buffer;
    return 0u;
}
