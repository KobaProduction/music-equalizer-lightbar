#pragma once

#include <stddef.h>
#include <stdint.h>

int st17h66b_spi1_init_p34(uint32_t baud_hz);
int st17h66b_spi1_write(const uint8_t *data, size_t size);
