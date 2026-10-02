#include "st17h66b_spi1.h"

#include <stdint.h>

/*
 * Minimal project-owned SPI1 bring-up for ST17H66B/PHY62x2.
 *
 * Register addresses and mux identifiers are reconstructed from public
 * PHY62x2 technical references. No vendor driver source is linked here.
 */

#define BIT_U32(n) (UINT32_C(1) << (n))

#define ST17H66B_PCR_BASE UINT32_C(0x40000000)
#define ST17H66B_COM_BASE UINT32_C(0x40003000)
#define ST17H66B_IOMUX_BASE UINT32_C(0x40003800)
#define ST17H66B_SPI1_BASE UINT32_C(0x40007000)

#define ST17H66B_PCR_SW_RESET0 (*(volatile uint32_t *)(ST17H66B_PCR_BASE + 0x00u))
#define ST17H66B_PCR_SW_CLK    (*(volatile uint32_t *)(ST17H66B_PCR_BASE + 0x08u))
#define ST17H66B_PCR_SW_CLK1   (*(volatile uint32_t *)(ST17H66B_PCR_BASE + 0x14u))

#define ST17H66B_COM_PERI_MASTER_SELECT     (*(volatile uint32_t *)(ST17H66B_COM_BASE + 0x2cu))

#define ST17H66B_IOMUX_FULL_MUX0_EN     (*(volatile uint32_t *)(ST17H66B_IOMUX_BASE + 0x0cu))
#define ST17H66B_IOMUX_GPIO_SEL(index)     (*(volatile uint32_t *)(ST17H66B_IOMUX_BASE + 0x18u + ((index) * 4u)))

#define ST17H66B_SPI1_CR0    (*(volatile uint16_t *)(ST17H66B_SPI1_BASE + 0x00u))
#define ST17H66B_SPI1_SSIEN  (*(volatile uint8_t  *)(ST17H66B_SPI1_BASE + 0x08u))
#define ST17H66B_SPI1_SER    (*(volatile uint8_t  *)(ST17H66B_SPI1_BASE + 0x10u))
#define ST17H66B_SPI1_BAUDR  (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x14u))
#define ST17H66B_SPI1_TXFLR  (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x20u))
#define ST17H66B_SPI1_SR     (*(volatile uint8_t  *)(ST17H66B_SPI1_BASE + 0x28u))
#define ST17H66B_SPI1_IMR    (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x2cu))
#define ST17H66B_SPI1_DATA   (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x60u))

#define ST17H66B_MOD_IOMUX 7u
#define ST17H66B_MOD_SPI1  12u
#define ST17H66B_MOD_COM   6u

/* GPIO driver index for P34 in the PHY62x2 GPIO block. */
#define ST17H66B_GPIO_INDEX_P34 22u

/* Full-mux function identifier for SPI1 TX/SDO. */
#define ST17H66B_FMUX_SPI1_TX 22u

#define ST17H66B_SPI_SR_BUSY UINT8_C(0x01)
#define ST17H66B_SPI_SR_TX_NOT_FULL UINT8_C(0x02)

/*
 * Public ROM symbol maps identify clk_get_pclk at this Thumb address.
 * This is used only as a ROM interface, not as copied SDK implementation.
 */
typedef uint32_t (*rom_clk_get_pclk_t)(void);
#define ST17H66B_ROM_CLK_GET_PCLK ((rom_clk_get_pclk_t)(uintptr_t)UINT32_C(0x0000a5d1))

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

    /* Enable IOMUX, SPI1 and COM register clocks. */
    ST17H66B_PCR_SW_CLK |= BIT_U32(ST17H66B_MOD_IOMUX)
        | BIT_U32(ST17H66B_MOD_SPI1);
    ST17H66B_PCR_SW_CLK1 |= BIT_U32(ST17H66B_MOD_COM);

    /* Reset SPI1 before configuring it. */
    ST17H66B_PCR_SW_RESET0 &= ~BIT_U32(ST17H66B_MOD_SPI1);
    ST17H66B_PCR_SW_RESET0 |= BIT_U32(ST17H66B_MOD_SPI1);

    configure_p34_spi1_tx();

    const uint32_t pclk_hz = ST17H66B_ROM_CLK_GET_PCLK();
    if (pclk_hz < (baud_hz * 2u)) {
        return -2;
    }

    uint32_t divider = (pclk_hz + (baud_hz / 2u)) / baud_hz;
    if (divider < 2u) {
        divider = 2u;
    }
    if (divider > UINT32_C(65534)) {
        divider = UINT32_C(65534);
    }

    /*
     * The DesignWare SSI baud divider is defined for even values. Select the
     * nearest even divider using cross-multiplied frequency error.
     */
    if ((divider & 1u) != 0u) {
        const uint32_t down = divider > 2u ? divider - 1u : 2u;
        const uint32_t up = divider < UINT32_C(65534)
            ? divider + 1u
            : UINT32_C(65534);

        const uint32_t down_product = baud_hz * down;
        const uint32_t up_product = baud_hz * up;

        const uint32_t down_error = pclk_hz > down_product
            ? pclk_hz - down_product
            : down_product - pclk_hz;
        const uint32_t up_error = pclk_hz > up_product
            ? pclk_hz - up_product
            : up_product - pclk_hz;

        divider = down_error <= up_error ? down : up;
    }

    ST17H66B_SPI1_SSIEN = 0u;

    /*
     * SPI mode 0, 8-bit frames, transmit-only.
     * CR0: DFS=7 (8 bits), TMOD=1 (TX only).
     */
    ST17H66B_SPI1_CR0 = (uint16_t)(UINT16_C(0x0007) | UINT16_C(0x0100));

    /* Select SPI1 as AP master and enable its master clock path. */
    ST17H66B_COM_PERI_MASTER_SELECT |= BIT_U32(1) | BIT_U32(5);

    ST17H66B_SPI1_BAUDR = divider;
    ST17H66B_SPI1_IMR = 0u;
    ST17H66B_SPI1_SER = 1u;
    ST17H66B_SPI1_SSIEN = 1u;

    return 0;
}

int st17h66b_spi1_write(const uint8_t *data, size_t size)
{
    if (data == NULL) {
        return -1;
    }

    size_t offset = 0u;

    while (offset < size) {
        if ((ST17H66B_SPI1_SR & ST17H66B_SPI_SR_TX_NOT_FULL) != 0u
            && ST17H66B_SPI1_TXFLR < 8u) {
            ST17H66B_SPI1_DATA = data[offset++];
        }
    }

    while ((ST17H66B_SPI1_SR & ST17H66B_SPI_SR_BUSY) != 0u) {
        /* Wait for the final serialized bit to leave the peripheral. */
    }

    return 0;
}
