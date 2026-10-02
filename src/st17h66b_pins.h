#pragma once

#include <stdint.h>

/*
 * ST17H66B logical GPIO identifiers.
 *
 * Numeric values follow the Pxx signal number, not the TSSOP-16 package pin.
 * Package-pin numbers for the currently relevant bring-up signals are exposed
 * separately below.
 */
typedef enum {
    ST17H66B_PIN_P2  = 2,
    ST17H66B_PIN_P3  = 3,
    ST17H66B_PIN_P7  = 7,
    ST17H66B_PIN_P9  = 9,
    ST17H66B_PIN_P10 = 10,
    ST17H66B_PIN_P11 = 11,
    ST17H66B_PIN_P14 = 14,
    ST17H66B_PIN_P15 = 15,
    ST17H66B_PIN_P18 = 18,
    ST17H66B_PIN_P20 = 20,
    ST17H66B_PIN_P34 = 34,

    ST17H66B_PIN_UNMAPPED = 0xff,
} st17h66b_pin_t;

/* Analog-function aliases from the ST17H66B TSSOP-16 pin table. */
#define ST17H66B_PIN_AIO0 ST17H66B_PIN_P11
#define ST17H66B_PIN_AIO3 ST17H66B_PIN_P14
#define ST17H66B_PIN_AIO4 ST17H66B_PIN_P15
#define ST17H66B_PIN_AIO7 ST17H66B_PIN_P18
#define ST17H66B_PIN_AIO9 ST17H66B_PIN_P20

/* TSSOP-16 package positions relevant to current board bring-up. */
#define ST17H66B_PACKAGE_PIN_P9  UINT8_C(5)
#define ST17H66B_PACKAGE_PIN_P10 UINT8_C(6)
#define ST17H66B_PACKAGE_PIN_P15 UINT8_C(9)
