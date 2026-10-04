#pragma once

#include <stddef.h>
#include <stdint.h>

int st17h66b_spi1_init_p34(uint32_t baud_hz);
int st17h66b_spi1_write(const uint8_t *data, size_t size);
uint32_t st17h66b_spi1_completed_frames(void);
uint32_t st17h66b_spi1_pclk_hz(void);
uint32_t st17h66b_spi1_effective_baud_hz(void);
uint32_t st17h66b_spi1_divider(void);
