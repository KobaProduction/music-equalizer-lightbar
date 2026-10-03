#include "st17h66b_spi1.h"

#include <stdint.h>
#include <string.h>

#include "dma.h"
#include "pwrmgr.h"

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
#define ST17H66B_SPI1_SR     (*(volatile uint8_t  *)(ST17H66B_SPI1_BASE + 0x28u))
#define ST17H66B_SPI1_IMR    (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x2cu))
#define ST17H66B_SPI1_DMACR  (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x4cu))
#define ST17H66B_SPI1_DMATDLR (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x50u))
#define ST17H66B_SPI1_DATA   (*(volatile uint32_t *)(ST17H66B_SPI1_BASE + 0x60u))

#define ST17H66B_MOD_IOMUX 7u
#define ST17H66B_MOD_SPI1  12u
#define ST17H66B_MOD_COM   6u

#define ST17H66B_GPIO_INDEX_P34 22u
#define ST17H66B_FMUX_SPI1_TX 22u

#define ST17H66B_SPI_SR_BUSY UINT8_C(0x01)

#define ST17H66B_DMA_FRAME_CAPACITY 1024u
#define ST17H66B_DMA_CHUNK_MAX 0x07ffu

typedef uint32_t (*rom_clk_get_pclk_t)(void);
#define ST17H66B_ROM_CLK_GET_PCLK     ((rom_clk_get_pclk_t)(uintptr_t)UINT32_C(0x0000a5d1))

static uint32_t s_spi1_baud_hz;
static uint32_t s_spi1_pclk_hz;
static uint32_t s_spi1_effective_baud_hz;
static uint32_t s_spi1_divider;
static uint8_t s_dma_buffers[2][ST17H66B_DMA_FRAME_CAPACITY];
static volatile uint8_t s_dma_active;
static volatile uint8_t s_active_buffer;
static volatile uint8_t s_pending_valid;
static volatile uint8_t s_pending_buffer;
static volatile uint16_t s_active_size;
static volatile uint16_t s_active_offset;
static volatile uint16_t s_pending_size;
static volatile uint32_t s_completed_frames;

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

static int st17h66b_spi1_hw_init(uint32_t baud_hz)
{
    if (baud_hz == 0u) {
        return -1;
    }

    ST17H66B_PCR_SW_CLK |= BIT_U32(ST17H66B_MOD_IOMUX)
        | BIT_U32(ST17H66B_MOD_SPI1);
    ST17H66B_PCR_SW_CLK1 |= BIT_U32(ST17H66B_MOD_COM);

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
     * DW-SSI requires an even BAUDR divider. If normal rounding lands on an
     * odd value, choose the neighbouring even divider that gives the smaller
     * frequency error. The old code always rounded upward, which can turn a
     * 16 MHz PCLK / 2.4 MHz request into 2.0 MHz instead of the much closer
     * 2.667 MHz and pushes 3-bit WS2812 symbols outside their useful window.
     */
    if ((divider & 1u) != 0u) {
        const uint32_t lower = divider > 2u ? divider - 1u : 2u;
        const uint32_t upper =
            divider < UINT32_C(65534) ? divider + 1u : UINT32_C(65534);
        const uint32_t lower_hz = pclk_hz / lower;
        const uint32_t upper_hz = pclk_hz / upper;
        const uint32_t lower_error =
            lower_hz > baud_hz ? lower_hz - baud_hz : baud_hz - lower_hz;
        const uint32_t upper_error =
            upper_hz > baud_hz ? upper_hz - baud_hz : baud_hz - upper_hz;
        divider = lower_error <= upper_error ? lower : upper;
    }

    ST17H66B_SPI1_SSIEN = 0u;
    ST17H66B_SPI1_CR0 = (uint16_t)(UINT16_C(0x0007) | UINT16_C(0x0100));
    ST17H66B_COM_PERI_MASTER_SELECT |= BIT_U32(1) | BIT_U32(5);
    ST17H66B_SPI1_BAUDR = divider;
    s_spi1_pclk_hz = pclk_hz;
    s_spi1_divider = divider;
    s_spi1_effective_baud_hz = pclk_hz / divider;
    ST17H66B_SPI1_IMR = 0u;
    ST17H66B_SPI1_DMACR = 0u;
    ST17H66B_SPI1_DMATDLR = 0u;
    ST17H66B_SPI1_SER = 1u;
    ST17H66B_SPI1_SSIEN = 1u;

    return 0;
}

static void st17h66b_spi1_wakeup_restore(void)
{
    if (s_spi1_baud_hz != 0u) {
        (void)st17h66b_spi1_hw_init(s_spi1_baud_hz);
    }
}

static int start_dma_chunk(void)
{
    const uint16_t remaining =
        (uint16_t)(s_active_size - s_active_offset);
    const uint16_t chunk =
        remaining > ST17H66B_DMA_CHUNK_MAX
            ? ST17H66B_DMA_CHUNK_MAX
            : remaining;

    DMA_CH_CFG_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.transf_size = chunk;
    cfg.sinc = DMA_INC_INC;
    cfg.src_tr_width = DMA_WIDTH_BYTE;
    cfg.src_msize = DMA_BSIZE_1;
    cfg.src_addr =
        (uint32_t)&s_dma_buffers[s_active_buffer][s_active_offset];

    cfg.dinc = DMA_INC_NCHG;
    cfg.dst_tr_width = DMA_WIDTH_BYTE;
    cfg.dst_msize = DMA_BSIZE_1;
    cfg.dst_addr = (uint32_t)&ST17H66B_SPI1_DATA;
    cfg.enable_int = true;

    /*
     * Match the PHYplus SPI driver's ordering exactly: disable TX DMA,
     * configure and start the channel first, then expose the SSI request.
     * Enabling DMACR before the DMA channel is armed can lose the initial
     * TX-empty request and corrupt the first WS2812 symbols.
     */
    ST17H66B_SPI1_DMACR &= ~UINT32_C(0x02);
    ST17H66B_SPI1_DMATDLR = 0u;

    const int cfg_result = hal_dma_config_channel(DMA_CH_0, &cfg);
    if (cfg_result != 0) {
        return -10;
    }

    s_active_offset = (uint16_t)(s_active_offset + chunk);

    const int start_result = hal_dma_start_channel(DMA_CH_0);
    if (start_result != 0) {
        return -11;
    }

    ST17H66B_SPI1_DMACR |= UINT32_C(0x02);
    return 0;
}

static void finish_spi_frame(void)
{
    /*
     * DMA completion means the final bytes reached the SSI FIFO. Wait only
     * for the small hardware FIFO to drain before allowing sleep again.
     */
    uint32_t budget = 20000u;
    while ((ST17H66B_SPI1_SR & ST17H66B_SPI_SR_BUSY) != 0u
        && budget-- != 0u) {
    }

    ++s_completed_frames;
}

static void dma_complete(DMA_CH_t channel)
{
    (void)channel;

    if (s_active_offset < s_active_size) {
        (void)start_dma_chunk();
        return;
    }

    finish_spi_frame();

    if (s_pending_valid != 0u) {
        s_active_buffer = s_pending_buffer;
        s_active_size = s_pending_size;
        s_active_offset = 0u;
        s_pending_valid = 0u;

        if (start_dma_chunk() == 0) {
            return;
        }
    }

    s_dma_active = 0u;
    ST17H66B_SPI1_DMACR &= ~UINT32_C(0x02);
    (void)hal_pwrmgr_unlock(MOD_SPI1);
}

int st17h66b_spi1_init_p34(uint32_t baud_hz)
{
    const int hw_result = st17h66b_spi1_hw_init(baud_hz);
    if (hw_result != 0) {
        return hw_result;
    }

    s_spi1_baud_hz = baud_hz;

    if (hal_pwrmgr_register(
            MOD_SPI1, NULL, st17h66b_spi1_wakeup_restore) != 0) {
        return -3;
    }

    if (hal_dma_init() != 0) {
        return -4;
    }

    const HAL_DMA_t channel_cfg = {
        .dma_channel = DMA_CH_0,
        .evt_handler = dma_complete,
    };

    if (hal_dma_init_channel(channel_cfg) != 0) {
        return -5;
    }

    return 0;
}

int st17h66b_spi1_write(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0u) {
        return -1;
    }
    if (size > ST17H66B_DMA_FRAME_CAPACITY) {
        return -2;
    }

    HAL_ENTER_CRITICAL_SECTION();

    if (s_dma_active == 0u) {
        s_active_buffer = 0u;
        memcpy(s_dma_buffers[0], data, size);
        s_active_size = (uint16_t)size;
        s_active_offset = 0u;
        s_dma_active = 1u;

        if (hal_pwrmgr_lock(MOD_SPI1) != 0) {
            s_dma_active = 0u;
            HAL_EXIT_CRITICAL_SECTION();
            return -3;
        }

        const int start_result = start_dma_chunk();
        if (start_result != 0) {
            s_dma_active = 0u;
            (void)hal_pwrmgr_unlock(MOD_SPI1);
            HAL_EXIT_CRITICAL_SECTION();
            return start_result;
        }

        HAL_EXIT_CRITICAL_SECTION();
        return 0;
    }

    const uint8_t buffer = (uint8_t)(s_active_buffer ^ 1u);
    memcpy(s_dma_buffers[buffer], data, size);
    s_pending_buffer = buffer;
    s_pending_size = (uint16_t)size;
    s_pending_valid = 1u;

    HAL_EXIT_CRITICAL_SECTION();
    return 0;
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
