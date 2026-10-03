#include "phyplus_happylighting_gatt.h"

#include "happylighting.h"

#include "att.h"
#include "bcomdef.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static melb_control_state_t *happy_state;
static melb_phyplus_happylighting_state_changed_fn state_changed_cb;

static CONST uint8 happy_service_uuid[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HAPPY_LIGHTING_SERVICE_UUID16),
    HI_UINT16(HAPPY_LIGHTING_SERVICE_UUID16),
};

static CONST uint8 happy_write_uuid[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HAPPY_LIGHTING_WRITE_UUID16),
    HI_UINT16(HAPPY_LIGHTING_WRITE_UUID16),
};

static CONST gattAttrType_t happy_service = {
    ATT_BT_UUID_SIZE,
    happy_service_uuid,
};

static CONST uint8 happy_write_props =
    GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;

static uint8 happy_write_value[20];

static bStatus_t happy_read_attr(
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

static bStatus_t happy_write_attr(
    uint16 conn_handle,
    gattAttribute_t *attr,
    uint8 *value,
    uint16 length,
    uint16 offset)
{
    (void)conn_handle;

    if (attr == NULL || value == NULL || happy_state == NULL) {
        return ATT_ERR_UNLIKELY;
    }

    if (offset != 0u) {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    if (attr->type.len != ATT_BT_UUID_SIZE
        || BUILD_UINT16(attr->type.uuid[0], attr->type.uuid[1])
            != HAPPY_LIGHTING_WRITE_UUID16) {
        return ATT_ERR_ATTR_NOT_FOUND;
    }

    if (length > sizeof(happy_write_value)) {
        return ATT_ERR_INVALID_VALUE_SIZE;
    }

    const happy_lighting_result_t result =
        happy_lighting_apply_command(happy_state, value, length);

    if (result != HAPPY_LIGHTING_OK
        && result != HAPPY_LIGHTING_STATUS_REQUEST) {
        return ATT_ERR_INVALID_VALUE;
    }

    memcpy(happy_write_value, value, length);

    if (result == HAPPY_LIGHTING_OK && state_changed_cb != NULL) {
        state_changed_cb(happy_state);
    }

    return SUCCESS;
}

static CONST gattServiceCBs_t happy_callbacks = {
    happy_read_attr,
    happy_write_attr,
    NULL,
};

static gattAttribute_t happy_attributes[] = {
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
};

int melb_phyplus_happylighting_gatt_register(
    melb_control_state_t *state,
    melb_phyplus_happylighting_state_changed_fn on_state_changed)
{
    if (state == NULL) {
        return -1;
    }

    happy_state = state;
    state_changed_cb = on_state_changed;

    const bStatus_t status = GATTServApp_RegisterService(
        happy_attributes,
        GATT_NUM_ATTRS(happy_attributes),
        &happy_callbacks);

    return status == SUCCESS ? 0 : (int)status;
}
