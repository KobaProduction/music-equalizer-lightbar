#include "sbp_profile.h"

#include "att.h"
#include "bcomdef.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "ble_control.h"
#include "lotus_lantern.h"
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

static void render_state(void)
{
    if (!renderer_ready) {
        return;
    }

    if (control_state.power == 0u) {
        ws2812b_clear(pixels, MELB_LOTUS_LED_COUNT);
    } else {
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

    if (lotus_lantern_apply_frame(&control_state, value, length)
        != LOTUS_LANTERN_OK) {
        return ATT_ERR_INVALID_VALUE;
    }

    memcpy(lotus_write_value, value, LOTUS_LANTERN_FRAME_SIZE);
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

    renderer_ready =
        st17h66b_spi1_init_p34(WS2812B_SPI_BAUD_HZ) == 0;

    if (renderer_ready) {
        render_state();
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
