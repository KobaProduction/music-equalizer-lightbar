#include "st17h66b_spi1.h"

#include <stdint.h>

#define BIT_U32(n) (UINT32_C(1) << (n))

#define ST17H66B_PCR_BASE UINT32_C(0x40000000)
#define ST17H66B_COM_BASE UINT32_C(0x40003000)
#define ST17H66B_IOMUX_BASE UINT32_C(0x40003800)
#define ST17H66B_SPI1_BASE UINT32_C(0x40007000)

#define ST17H66B_PCR_SW_RESET0 (*(volatile uint32_t *)(ST17H66B_PCR_BASE + 0x00u))
#define ST17H66B_PCR_SW_CLK    (*(volatile uint32_t *)(ST17H66B_PCR_BASE + 0x08u))
#define ST17H66B_PCR_SW_CLK1   (*(volatile uint32_t *)(ST17H66B_PCR_BASE + 0x14u))

#define ST17H66B_COM_PERI_MASTER_SELECT (*(volatile uint32_t *)(ST17H66B_COM_BASE + 0x2cu))

#define ST17H66B_IOMUX_FULL_MUX0_EN (*(volatile uint32_t *)(ST17H66B_IOMUX_BASE + 0x0cu))
#define ST17H66B_IOMUX_GPIO_SEL(index) (*(volatile uint32_t *)(ST17H66B_IOMUX_BASE + 0x18u + ((index) * 4u)))

#define ST17H66B_SPI1_CR0    (*(volatile uint16_t *)(ST17H66B_SPI1_BASE + 0x00u))
#define ST17H66B_SPI1_SSIEN  (*(volatile uint8_t  *)(ST17H66B_SPI1_BASE + 0x08u))
#define ST17H66B_SPI1_SER    (*(volatile uint8_t  *)(ST17H66B_SPI1_BASE + 0x10u))
#define ST17H66B_SPI1_BAUDR  (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x14u))
#define ST17H66B_SPI1_TXFTLR (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x18u))
#define ST17H66B_SPI1_TXFLR  (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x20u))
#define ST17H66B_SPI1_SR     (*(volatile uint8_t  *)(ST17H66B_SPI1_BASE + 0x28u))
#define ST17H66B_SPI1_IMR    (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x2cu))
#define ST17H66B_SPI1_DATA   (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x60u))

#define ST17H66B_MOD_IOMUX 7u
#define ST17H66B_MOD_SPI1  12u
#define ST17H66B_MOD_COM   6u

#define ST17H66B_GPIO_INDEX_P34 22u
#define ST17H66B_FMUX_SPI1_TX 22u

#define ST17H66B_SPI_SR_BUSY UINT8_C(0x01)
#define ST17H66B_SPI_SR_TX_NOT_FULL UINT8_C(0x02)

typedef uint32_t (*rom_clk_get_pclk_t)(void);
#define ST17H66B_ROM_CLK_GET_PCLK ((rom_clk_get_pclk_t)(uintptr_t)UINT32_C(0x0000a5d1))

static uint32_t s_pclk;
static uint32_t s_divider;
static uint32_t s_effective;
static uint32_t s_completed;

static void configure_p34_spi1_tx(void)
{
    const uint32_t pin = ST17H66B_GPIO_INDEX_P34;
    const uint32_t register_index = pin >> 2u;
    const uint32_t bit_index = pin & 0x03u;
    const uint32_t shift = bit_index * 8u;
    const uint32_t mask = UINT32_C(0x3f) << shift;

    uint32_t value = ST17H66B_IOMUX_GPIO_SEL(register_index);
    value &= ~mask;
    value |= ST17H66B_FMUX_SPI1_TX << shift;
    ST17H66B_IOMUX_GPIO_SEL(register_index) = value;
    ST17H66B_IOMUX_FULL_MUX0_EN |= BIT_U32(pin);
}

int st17h66b_spi1_init_p34(uint32_t baud_hz)
{
    if (baud_hz == 0u) {
        return -1;
    }

    ST17H66B_PCR_SW_CLK |= BIT_U32(ST17H66B_MOD_IOMUX) | BIT_U32(ST17H66B_MOD_SPI1);
    ST17H66B_PCR_SW_CLK1 |= BIT_U32(ST17H66B_MOD_COM);

    ST17H66B_PCR_SW_RESET0 &= ~BIT_U32(ST17H66B_MOD_SPI1);
    ST17H66B_PCR_SW_RESET0 |= BIT_U32(ST17H66B_MOD_SPI1);

    configure_p34_spi1_tx();

    s_pclk = ST17H66B_ROM_CLK_GET_PCLK();
    uint32_t divider = (s_pclk + (baud_hz >> 1u)) / baud_hz;
    if (divider < 2u) {
        divider = 2u;
    } else if (divider > UINT32_C(65534)) {
        divider = UINT32_C(65534);
    }

    s_divider = divider;
    s_effective = s_pclk / divider;

    ST17H66B_SPI1_SSIEN = 0u;
    /* DFS=8-bit, MODE1 (SCPH=1), TX-only transfer mode. */
    ST17H66B_SPI1_CR0 = UINT16_C(0x0147);
    ST17H66B_COM_PERI_MASTER_SELECT |= BIT_U32(1) | BIT_U32(5);
    ST17H66B_SPI1_BAUDR = divider;
    ST17H66B_SPI1_TXFTLR = 4u;
    ST17H66B_SPI1_IMR = 0u;
    ST17H66B_SPI1_SER = 1u;
    ST17H66B_SPI1_SSIEN = 1u;

    return 0;
}

int st17h66b_spi1_write(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0u) {
        return -1;
    }

    size_t offset = 0u;
    uint32_t budget = UINT32_C(1000000);

    while (offset < size && budget-- != 0u) {
        if ((ST17H66B_SPI1_SR & ST17H66B_SPI_SR_TX_NOT_FULL) != 0u) {
            uint32_t room = 8u - ST17H66B_SPI1_TXFLR;
            while (room-- != 0u && offset < size) {
                ST17H66B_SPI1_DATA = data[offset++];
            }
        }
    }

    if (offset != size) {
        return -2;
    }

    budget = UINT32_C(1000000);
    while ((ST17H66B_SPI1_SR & ST17H66B_SPI_SR_BUSY) != 0u && budget-- != 0u) {
    }

    if (budget == 0u) {
        return -3;
    }

    ++s_completed;
    return 0;
}

uint32_t st17h66b_spi1_completed_frames(void)
{
    return s_completed;
}

uint32_t st17h66b_spi1_pclk_hz(void)
{
    return s_pclk;
}

uint32_t st17h66b_spi1_effective_baud_hz(void)
{
    return s_effective;
}

uint32_t st17h66b_spi1_divider(void)
{
    return s_divider;
}
