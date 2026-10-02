#pragma once

#include "ble_control.h"

typedef void (*melb_phyplus_lotus_state_changed_fn)(
    const melb_control_state_t *state);

int melb_phyplus_lotus_gatt_register(
    melb_control_state_t *state,
    melb_phyplus_lotus_state_changed_fn on_state_changed);
