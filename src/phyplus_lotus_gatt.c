#include "phyplus_lotus_gatt.h"

#include "lotus_lantern.h"

#include "att.h"
#include "bcomdef.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static melb_control_state_t *lotus_state;
static melb_phyplus_lotus_state_changed_fn state_changed_cb;

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

    if (attr == NULL || value == NULL || lotus_state == NULL) {
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

    const lotus_lantern_result_t result =
        lotus_lantern_apply_frame(lotus_state, value, length);

    if (result != LOTUS_LANTERN_OK) {
        return ATT_ERR_INVALID_VALUE;
    }

    memcpy(lotus_write_value, value, LOTUS_LANTERN_FRAME_SIZE);

    if (state_changed_cb != NULL) {
        state_changed_cb(lotus_state);
    }

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

int melb_phyplus_lotus_gatt_register(
    melb_control_state_t *state,
    melb_phyplus_lotus_state_changed_fn on_state_changed)
{
    if (state == NULL) {
        return -1;
    }

    lotus_state = state;
    state_changed_cb = on_state_changed;

    const bStatus_t status = GATTServApp_RegisterService(
        lotus_attributes,
        GATT_NUM_ATTRS(lotus_attributes),
        &lotus_callbacks);

    return status == SUCCESS ? 0 : (int)status;
}
