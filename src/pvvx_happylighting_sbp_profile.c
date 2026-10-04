#include <stdint.h>

#include "bcomdef.h"
#include "sbp_profile.h"

#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "linkdb.h"

#include "ble_control.h"
#include "happylighting.h"
#include "local_controls.h"
#include "log.h"
#include "gpio.h"
#include "st17h66b_spi1.h"
#include "ws2812b.h"
#include "ws2812b_spi.h"

#include <stddef.h>
#include <string.h>

enum {
    MELB_LED_COUNT = 32,
    MELB_SPI_FRAME_SIZE =
        (MELB_LED_COUNT * WS2812B_SPI_BYTES_PER_PIXEL)
        + WS2812B_SPI_RESET_BYTES,
    HAPPY_ATTR_WRITE_VALUE_IDX = 2,
    HAPPY_ATTR_NOTIFY_VALUE_IDX = 4,
};

static melb_control_state_t control_state;
static ws2812b_pixel_t pixels[MELB_LED_COUNT];
static uint8_t spi_frame[MELB_SPI_FRAME_SIZE];
static int renderer_ready;
static melb_local_controls_t local_controls;
static uint8_t animation_phase;
static uint8_t animation_ticks;
static uint32_t render_count;
static uint16_t rgb_test_ticks;
static uint8_t rgb_test_phase;

static CONST uint8 happy_service_uuid[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HAPPY_LIGHTING_SERVICE_UUID16),
    HI_UINT16(HAPPY_LIGHTING_SERVICE_UUID16),
};

static CONST uint8 happy_write_uuid[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HAPPY_LIGHTING_WRITE_UUID16),
    HI_UINT16(HAPPY_LIGHTING_WRITE_UUID16),
};

static CONST uint8 happy_notify_uuid[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HAPPY_LIGHTING_NOTIFY_UUID16),
    HI_UINT16(HAPPY_LIGHTING_NOTIFY_UUID16),
};

static CONST gattAttrType_t happy_service = {
    ATT_BT_UUID_SIZE,
    happy_service_uuid,
};

static CONST uint8 happy_write_props =
    GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static CONST uint8 happy_notify_props =
    GATT_PROP_READ | GATT_PROP_NOTIFY;

static uint8 happy_write_value[20];
static uint8 happy_notify_value[HAPPY_LIGHTING_STATUS_SIZE];
static gattCharCfg_t happy_notify_cfg[GATT_MAX_NUM_CONN];

static uint8_t gamma_correct(uint8_t linear)
{
    /*
     * Integer gamma ~= 2.0. This keeps bring-up deterministic and cheap on
     * Cortex-M0 while preventing low UI brightness values from looking harsh.
     */
    const uint16_t squared = (uint16_t)linear * (uint16_t)linear;
    return (uint8_t)((squared + UINT16_C(254)) / UINT16_C(255));
}

static uint8_t scale_channel(uint8_t channel, uint8_t brightness)
{
    const uint8_t linear =
        (uint8_t)(((uint16_t)channel * (uint16_t)brightness) / UINT16_C(255));
    return gamma_correct(linear);
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

static ws2812b_pixel_t scale_pixel(ws2812b_pixel_t color, uint8_t brightness)
{
    color.red = scale_channel(color.red, brightness);
    color.green = scale_channel(color.green, brightness);
    color.blue = scale_channel(color.blue, brightness);
    return color;
}

static ws2812b_pixel_t fixed_mode_color(uint8_t mode)
{
    ws2812b_pixel_t color = {0};

    switch (mode) {
    case 0x26u:
    case 0x31u:
        color.red = 255u;
        break;
    case 0x27u:
    case 0x32u:
        color.green = 255u;
        break;
    case 0x28u:
    case 0x33u:
        color.blue = 255u;
        break;
    case 0x29u:
    case 0x34u:
        color.red = 255u;
        color.green = 255u;
        break;
    case 0x2Au:
    case 0x35u:
        color.green = 255u;
        color.blue = 255u;
        break;
    case 0x2Bu:
    case 0x36u:
        color.red = 255u;
        color.blue = 255u;
        break;
    case 0x2Cu:
    case 0x37u:
        color.red = 255u;
        color.green = 255u;
        color.blue = 255u;
        break;
    default:
        break;
    }

    return color;
}

static ws2812b_pixel_t lerp_color(
    ws2812b_pixel_t first,
    ws2812b_pixel_t second,
    uint8_t amount)
{
    ws2812b_pixel_t out;
    const uint16_t inv = (uint16_t)(255u - amount);

    out.red = (uint8_t)(
        (((uint16_t)first.red * inv) + ((uint16_t)second.red * amount)) / 255u);
    out.green = (uint8_t)(
        (((uint16_t)first.green * inv) + ((uint16_t)second.green * amount)) / 255u);
    out.blue = (uint8_t)(
        (((uint16_t)first.blue * inv) + ((uint16_t)second.blue * amount)) / 255u);

    return out;
}

static void fill_pixels(ws2812b_pixel_t color)
{
    for (size_t i = 0u; i < MELB_LED_COUNT; ++i) {
        pixels[i] = color;
    }
}

static void render_state(void)
{
    if (!renderer_ready) {
        return;
    }

    ws2812b_clear(pixels, MELB_LED_COUNT);

    if (control_state.power != 0u) {
        if (control_state.mode == HAPPY_LIGHTING_MODE_STATIC) {
            ws2812b_pixel_t color = {
                .red = control_state.red,
                .green = control_state.green,
                .blue = control_state.blue,
            };
            fill_pixels(scale_pixel(color, control_state.brightness));
        } else if (control_state.mode == 0x25u) {
            /* Calm seven-color cross fade, spatially distributed across 32 LEDs. */
            for (size_t i = 0u; i < MELB_LED_COUNT; ++i) {
                ws2812b_pixel_t color =
                    wheel((uint8_t)(animation_phase + (uint8_t)(i * 8u)));
                pixels[i] = scale_pixel(color, control_state.brightness);
            }
        } else if (control_state.mode >= 0x26u && control_state.mode <= 0x2Cu) {
            const uint8_t triangle = animation_phase < 128u
                ? (uint8_t)(animation_phase * 2u)
                : (uint8_t)((255u - animation_phase) * 2u);
            ws2812b_pixel_t color = fixed_mode_color(control_state.mode);
            color = scale_pixel(color, scale_channel(control_state.brightness, triangle));
            fill_pixels(color);
        } else if (control_state.mode >= 0x2Du && control_state.mode <= 0x2Fu) {
            ws2812b_pixel_t first = {0};
            ws2812b_pixel_t second = {0};

            if (control_state.mode == 0x2Du) {
                first.red = 255u;
                second.green = 255u;
            } else if (control_state.mode == 0x2Eu) {
                first.red = 255u;
                second.blue = 255u;
            } else {
                first.green = 255u;
                second.blue = 255u;
            }

            const uint8_t triangle = animation_phase < 128u
                ? (uint8_t)(animation_phase * 2u)
                : (uint8_t)((255u - animation_phase) * 2u);
            fill_pixels(scale_pixel(
                lerp_color(first, second, triangle),
                control_state.brightness));
        } else if (control_state.mode == 0x30u) {
            if ((animation_phase & 0x08u) == 0u) {
                fill_pixels(scale_pixel(
                    wheel((uint8_t)(animation_phase * 8u)),
                    control_state.brightness));
            }
        } else if (control_state.mode >= 0x31u && control_state.mode <= 0x37u) {
            if ((animation_phase & 0x08u) == 0u) {
                fill_pixels(scale_pixel(
                    fixed_mode_color(control_state.mode),
                    control_state.brightness));
            }
        } else if (control_state.mode == 0x38u) {
            fill_pixels(scale_pixel(
                wheel((uint8_t)((animation_phase >> 4u) * 36u)),
                control_state.brightness));
        }
    }

    const size_t encoded = ws2812b_spi_encode(
        pixels,
        MELB_LED_COUNT,
        spi_frame,
        sizeof(spi_frame));

    if (encoded == sizeof(spi_frame)) {
        const int spi_result = st17h66b_spi1_write(spi_frame, encoded);
        ++render_count;
        if (spi_result != 0) {
            renderer_ready = 0;
            LOG("MELB: WS2812 SPI timeout/error=%d; renderer disabled, BLE kept alive\n",
                spi_result);
        } else if ((render_count % 50u) == 0u) {
            LOG("MELB: render frame=%lu spi=0 mode=%02x power=%u\n",
                (unsigned long)render_count, control_state.mode, control_state.power);
        }
    }
}

static void refresh_status_value(void)
{
    (void)happy_lighting_build_status(
        &control_state,
        happy_notify_value,
        sizeof(happy_notify_value));
}

static void send_status_notification(uint16 conn_handle)
{
    if ((GATTServApp_ReadCharCfg(conn_handle, happy_notify_cfg)
        & GATT_CLIENT_CFG_NOTIFY) == 0u) {
        LOG("MELB: HappyLighting status requested without FFD4 notify subscription\n");
        return;
    }

    attHandleValueNoti_t noti;
    memset(&noti, 0, sizeof(noti));
    refresh_status_value();

    noti.handle = 0u; /* assigned after the attribute table is defined below */
    noti.len = HAPPY_LIGHTING_STATUS_SIZE;
    memcpy(noti.value, happy_notify_value, HAPPY_LIGHTING_STATUS_SIZE);

    extern gattAttribute_t happy_attributes[];
    noti.handle = happy_attributes[HAPPY_ATTR_NOTIFY_VALUE_IDX].handle;

    const bStatus_t status = GATT_Notification(conn_handle, &noti, FALSE);
    LOG("MELB: HappyLighting status notify result=%u power=%u mode=%02x speed=%u\n",
        status, control_state.power, control_state.mode, control_state.speed);
}

void melb_happylighting_local_init(void)
{
    melb_local_controls_init(&local_controls);

    hal_gpio_pin_init(GPIO_P11, GPIO_INPUT);
    hal_gpio_pull_set(GPIO_P11, GPIO_PULL_UP);
    hal_gpio_pin_init(GPIO_P03, GPIO_INPUT);
    hal_gpio_pull_set(GPIO_P03, GPIO_PULL_UP);
    hal_gpio_pin_init(GPIO_P07, GPIO_INPUT);
    hal_gpio_pull_set(GPIO_P07, GPIO_PULL_UP);
}

void melb_happylighting_local_tick(void)
{
    const bool changed = melb_local_controls_tick(
        &local_controls,
        &control_state,
        !hal_gpio_read(GPIO_P11),
        !hal_gpio_read(GPIO_P03),
        !hal_gpio_read(GPIO_P07));

    if (changed) {
        LOG("MELB: buttons power=%u mode=%02x bright=%u speed=%u rgb=%u,%u,%u\n",
            control_state.power, control_state.mode, control_state.brightness,
            control_state.speed, control_state.red, control_state.green, control_state.blue);
        refresh_status_value();
    }

    /*
     * Hardware bring-up pattern: SLOT0/OFF/SLOT1/OFF/SLOT2/OFF at 500 ms
     * per phase. This identifies the physical colour behind each transmitted
     * byte without assuming a GRB/RGB/BRG order.
     */
    if (++rgb_test_ticks < 50u) {
        return;
    }
    rgb_test_ticks = 0u;

    /*
     * Raw wire-slot probe. ws2812b_spi_encode() currently serializes fields as
     * [green, red, blue], so SLOT0/SLOT1/SLOT2 intentionally address those
     * three transmitted bytes without claiming a physical colour identity.
     */
    ws2812b_pixel_t probe = {0};
    const uint8_t wire_level = gamma_correct(32u);
    const char *phase_name = "OFF";

    if (rgb_test_phase == 0u) {
        probe.green = wire_level; /* transmitted byte 0 */
        phase_name = "SLOT0";
    } else if (rgb_test_phase == 2u) {
        probe.red = wire_level;   /* transmitted byte 1 */
        phase_name = "SLOT1";
    } else if (rgb_test_phase == 4u) {
        probe.blue = wire_level;  /* transmitted byte 2 */
        phase_name = "SLOT2";
    }

    fill_pixels(probe);
    const size_t encoded = ws2812b_spi_encode(
        pixels, MELB_LED_COUNT, spi_frame, sizeof(spi_frame));
    int spi_result = -1;
    if (encoded == sizeof(spi_frame)) {
        spi_result = st17h66b_spi1_write(spi_frame, encoded);
    }

    LOG("MELB: wire test %s raw=%u dma_done=%lu spi=%d\n",
        phase_name, wire_level,
        (unsigned long)st17h66b_spi1_completed_frames(), spi_result);

    rgb_test_phase = (uint8_t)((rgb_test_phase + 1u) % 6u);
}

static uint8 happy_read_attr(
    uint16 conn_handle,
    gattAttribute_t *attr,
    uint8 *value,
    uint16 *length,
    uint16 offset,
    uint8 max_length)
{
    (void)conn_handle;

    const uint16 uuid = BUILD_UINT16(attr->type.uuid[0], attr->type.uuid[1]);

    if (uuid != HAPPY_LIGHTING_NOTIFY_UUID16) {
        return ATT_ERR_ATTR_NOT_FOUND;
    }

    if (offset != 0u) {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    refresh_status_value();

    uint16 count = HAPPY_LIGHTING_STATUS_SIZE;
    if (count > max_length) {
        count = max_length;
    }

    memcpy(value, happy_notify_value, count);
    *length = count;
    return SUCCESS;
}

static bStatus_t happy_write_attr(
    uint16 conn_handle,
    gattAttribute_t *attr,
    uint8 *value,
    uint16 length,
    uint16 offset)
{
    if (attr == NULL || value == NULL) {
        return ATT_ERR_UNLIKELY;
    }

    const uint16 uuid = BUILD_UINT16(attr->type.uuid[0], attr->type.uuid[1]);

    if (uuid == GATT_CLIENT_CHAR_CFG_UUID) {
        return GATTServApp_ProcessCCCWriteReq(
            conn_handle,
            attr,
            value,
            length,
            offset,
            GATT_CLIENT_CFG_NOTIFY);
    }

    if (uuid != HAPPY_LIGHTING_WRITE_UUID16) {
        return ATT_ERR_ATTR_NOT_FOUND;
    }

    if (offset != 0u) {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    if (length > sizeof(happy_write_value)) {
        return ATT_ERR_INVALID_VALUE_SIZE;
    }

    memcpy(happy_write_value, value, length);
    LOG("MELB: HappyLighting FFD9 write len=%u\n", length);
    LOG_DUMP_BYTE(value, length);

    const happy_lighting_result_t result =
        happy_lighting_apply_command(&control_state, value, length);

    if (result == HAPPY_LIGHTING_STATUS_REQUEST) {
        send_status_notification(conn_handle);
        return SUCCESS;
    }

    if (result != HAPPY_LIGHTING_OK) {
        LOG("MELB: HappyLighting command rejected result=%u\n", result);
        return ATT_ERR_INVALID_VALUE;
    }

    animation_ticks = 0u;
    refresh_status_value();
    render_state();

    LOG("MELB: state power=%u mode=%02x bright=%u speed=%u rgb=%u,%u,%u\n",
        control_state.power, control_state.mode, control_state.brightness,
        control_state.speed, control_state.red, control_state.green, control_state.blue);

    return SUCCESS;
}

static CONST gattServiceCBs_t happy_callbacks = {
    happy_read_attr,
    happy_write_attr,
    NULL,
};

gattAttribute_t happy_attributes[] = {
    {
        { ATT_BT_UUID_SIZE, primaryServiceUUID },
        GATT_PERMIT_READ,
        0,
        (uint8 *)&happy_service,
    },
    {
        { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ,
        0,
        (uint8 *)&happy_write_props,
    },
    {
        { ATT_BT_UUID_SIZE, happy_write_uuid },
        GATT_PERMIT_WRITE,
        0,
        happy_write_value,
    },
    {
        { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ,
        0,
        (uint8 *)&happy_notify_props,
    },
    {
        { ATT_BT_UUID_SIZE, happy_notify_uuid },
        GATT_PERMIT_READ,
        0,
        happy_notify_value,
    },
    {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)happy_notify_cfg,
    },
};

static void happy_handle_conn_status(uint16 conn_handle, uint8 change_type)
{
    if (conn_handle == LOOPBACK_CONNHANDLE) {
        return;
    }

    if ((change_type == LINKDB_STATUS_UPDATE_REMOVED)
        || ((change_type == LINKDB_STATUS_UPDATE_STATEFLAGS)
            && !linkDB_Up(conn_handle))) {
        GATTServApp_InitCharCfg(conn_handle, happy_notify_cfg);
    }
}

bStatus_t SimpleProfile_AddService(uint32 services)
{
    (void)services;

    melb_control_state_init(&control_state);
    animation_phase = 0u;
    animation_ticks = 0u;
    rgb_test_ticks = 0u;
    rgb_test_phase = 0u;
    control_state.brightness = 32u;
    control_state.red = 255u;
    control_state.green = 0u;
    control_state.blue = 0u;
    control_state.mode = HAPPY_LIGHTING_MODE_STATIC;
    refresh_status_value();

    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, happy_notify_cfg);
    VOID linkDB_Register(happy_handle_conn_status);

    renderer_ready =
        st17h66b_spi1_init_p34(WS2812B_SPI_BAUD_HZ) == 0;

    LOG("MELB: WS2812 SPI1/P34 init=%s pclk=%lu divider=%lu actual=%luHz target=%luHz\n",
        renderer_ready ? "ok" : "FAIL",
        (unsigned long)st17h66b_spi1_pclk_hz(),
        (unsigned long)st17h66b_spi1_divider(),
        (unsigned long)st17h66b_spi1_effective_baud_hz(),
        (unsigned long)WS2812B_SPI_BAUD_HZ);
    if (renderer_ready) {
        const uint8_t boot_power = control_state.power;
        control_state.power = 0u;
        render_state();
        control_state.power = boot_power;
        render_state();
        LOG("MELB: boot RGB diagnostic RED brightness=%u gamma_out=%u dma=1\n",
            control_state.brightness, gamma_correct(control_state.brightness));
    }

    return GATTServApp_RegisterService(
        happy_attributes,
        GATT_NUM_ATTRS(happy_attributes),
        &happy_callbacks);
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
