#include "st17h66b_spi1.h"

#include <stdint.h>

#include "gpio.h"
#include "spi.h"

typedef uint32_t (*rom_clk_get_pclk_t)(void);
#define ST17H66B_ROM_CLK_GET_PCLK \
    ((rom_clk_get_pclk_t)(uintptr_t)UINT32_C(0x0000a5d1))

static hal_spi_t s_spi1 = {
    .spi_index = SPI1,
};

static uint32_t s_spi1_baud_hz;
static uint32_t s_spi1_pclk_hz;
static uint32_t s_spi1_effective_baud_hz;
static uint32_t s_spi1_divider;
static uint32_t s_completed_frames;

int st17h66b_spi1_init_p34(uint32_t baud_hz)
{
    if (baud_hz == 0u) {
        return -1;
    }

    s_spi1_baud_hz = baud_hz;
    s_spi1_pclk_hz = ST17H66B_ROM_CLK_GET_PCLK();

    /*
     * Match the factory PHYplus hal_spi_master_init formula exactly.
     * Do not force an even divider: the stock firmware does not.
     */
    uint32_t divider =
        (s_spi1_pclk_hz + (baud_hz >> 1u)) / baud_hz;
    if (divider < 2u) {
        divider = 2u;
    } else if (divider > UINT32_C(65534)) {
        divider = UINT32_C(65534);
    }

    s_spi1_divider = divider;
    s_spi1_effective_baud_hz = s_spi1_pclk_hz / divider;

    if (hal_spi_init(SPI1) != 0) {
        return -2;
    }

    spi_Cfg_t cfg = {
        .sclk_pin = GPIO_DUMMY,
        .ssn_pin = GPIO_DUMMY,
        .MOSI = GPIO_P34,
        .MISO = GPIO_DUMMY,
        .baudrate = baud_hz,
        .spi_tmod = SPI_TRXD,
        .spi_scmod = SPI_MODE1,
        .spi_dfsmod = SPI_8BIT,
#if DMAC_USE
        .dma_tx_enable = true,
        .dma_rx_enable = false,
#endif
        .int_mode = false,
        .force_cs = true,
        .evt_handler = NULL,
    };

    if (hal_spi_bus_init(&s_spi1, cfg) != 0) {
        return -3;
    }

    return 0;
}

int st17h66b_spi1_write(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0u || size > UINT16_MAX) {
        return -1;
    }

    const int result = hal_spi_transmit(
        &s_spi1,
        SPI_TXD,
        (uint8_t *)data,
        NULL,
        (uint16_t)size,
        0u);

    if (result == 0) {
        ++s_completed_frames;
    }

    return result;
}

uint32_t st17h66b_spi1_completed_frames(void)
{
    return s_completed_frames;
}

uint32_t st17h66b_spi1_pclk_hz(void)
{
    return s_spi1_pclk_hz;
}

uint32_t st17h66b_spi1_effective_baud_hz(void)
{
    return s_spi1_effective_baud_hz;
}

uint32_t st17h66b_spi1_divider(void)
{
    return s_spi1_divider;
}
