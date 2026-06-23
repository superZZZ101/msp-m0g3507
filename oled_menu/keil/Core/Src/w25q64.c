/*
 * w25q64.c — W25Q64 SPI NOR Flash 驱动 (GPIO 模拟 SPI, Mode 0)
 *
 * SPI Mode 0 时序:
 *   CLK 空闲为低
 *   MOSI 数据在 CLK 下降沿改变
 *   MISO 数据在 CLK 上升沿采样
 *
 * 依赖:
 *   - ti_msp_dl_config.h (引脚宏定义及 SYSCFG_DL_init 已调用)
 *   - delay.h           (delay(uint16_t ms))
 *   - MSPM0 DriverLib   (DL_GPIO_*)
 */

#include "w25q64.h"
#include "ti_msp_dl_config.h"
#include "delay.h"

/* ============================================================
 * 微秒级阻塞延时 (基于 delay_cycles)
 *   CPUCLK_FREQ = 32 MHz ⇒ 1 µs ≈ 32 cycles
 * 如果实际 CPU 频率有变化, 修改 DELAY_US_SCALE 即可
 * ============================================================ */
#define DELAY_US_SCALE  (CPUCLK_FREQ / 1000000UL)   /* ≈ 32 */

static inline void delay_us(uint32_t us)
{
    delay_cycles(us * DELAY_US_SCALE);
}

/* ============================================================
 * SPI 硬件抽象 — 位带操作
 * ============================================================ */

/* --- CS --- */
#define W25Q64_CS_LOW()    DL_GPIO_clearPins(W25Q64_cs_PORT, W25Q64_cs_PIN)
#define W25Q64_CS_HIGH()   DL_GPIO_setPins(W25Q64_cs_PORT, W25Q64_cs_PIN)

/* --- CLK --- */
#define W25Q64_CLK_LOW()   DL_GPIO_clearPins(W25Q64_clk_PORT, W25Q64_clk_PIN)
#define W25Q64_CLK_HIGH()  DL_GPIO_setPins(W25Q64_clk_PORT, W25Q64_clk_PIN)

/* --- MOSI (DI) — 写数据到 Flash --- */
#define W25Q64_MOSI_LOW()  DL_GPIO_clearPins(W25Q64_di_PORT, W25Q64_di_PIN)
#define W25Q64_MOSI_HIGH() DL_GPIO_setPins(W25Q64_di_PORT, W25Q64_di_PIN)

/* --- MISO (DO) — 从 Flash 读数据 --- */
#define W25Q64_MISO_READ() \
    ((DL_GPIO_readPins(W25Q64_do_PORT, W25Q64_do_PIN) & W25Q64_do_PIN) ? 1U : 0U)

/* ============================================================
 * SPI 速度控制
 *   在 bit-bang 模式下, 通过延时调整 CLK 半周期
 *   默认半周期延时 1 µs ⇒ SCK ≈ 500 kHz (兼顾可靠性与速度)
 *   若接线较长或干扰大, 可调大 HALF_CYCLE_US
 * ============================================================ */
#define SPI_HALF_CYCLE_US  1

/* ============================================================
 * 内部函数: SPI 收发一个字节 (MSB first)
 * ============================================================ */
static uint8_t W25Q64_SwapByte(uint8_t txByte)
{
    uint8_t rxByte = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        /* MOSI: 在 CLK 下降沿改变数据 */
        if (txByte & 0x80)
            W25Q64_MOSI_HIGH();
        else
            W25Q64_MOSI_LOW();

        txByte <<= 1;                   /* 准备下一位 */

        /* CLK 上升沿 — Flash 在此时输出 MISO, 我们采样 */
        W25Q64_CLK_HIGH();
        delay_us(SPI_HALF_CYCLE_US);

        rxByte <<= 1;
        rxByte  |= W25Q64_MISO_READ();  /* 读 MISO */

        /* CLK 下降沿 — 我们说 MOSI 随后改变 */
        W25Q64_CLK_LOW();
        delay_us(SPI_HALF_CYCLE_US);
    }

    return rxByte;
}

/* ============================================================
 * 内部函数: 发送连续字节 (不关心 MISO)
 * ============================================================ */
static void W25Q64_SendBytes(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        W25Q64_SwapByte(data[i]);
}

/* ============================================================
 * 内部函数: 接收连续字节 (MOSI 一直发 0xFF)
 * ============================================================ */
static void W25Q64_RecvBytes(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        buf[i] = W25Q64_SwapByte(0xFF);
}

/* ============================================================
 * 内部函数: 发送 3 字节地址 (MSB first)
 * ============================================================ */
static inline void W25Q64_SendAddr24(uint32_t addr)
{
    uint8_t addrBuf[3];
    addrBuf[0] = (uint8_t)(addr >> 16);
    addrBuf[1] = (uint8_t)(addr >>  8);
    addrBuf[2] = (uint8_t)(addr);
    W25Q64_SendBytes(addrBuf, 3);
}

/* ============================================================
 * 内部函数: 发送指令 + 可选地址
 * ============================================================ */
static void W25Q64_SendCmd(uint8_t cmd)
{
    W25Q64_CS_LOW();
    W25Q64_SwapByte(cmd);
    /* 不拉高 CS —— 调用者继续使用总线或自己 CS_HIGH */
}

static void W25Q64_SendCmdEnd(uint8_t cmd)
{
    W25Q64_SendCmd(cmd);
    W25Q64_CS_HIGH();
}

/* ============================================================
 * 初始化
 * ============================================================ */
void W25Q64_Init(void)
{
    /* CS 默认置高 (片选无效) */
    W25Q64_CS_HIGH();
    /* CLK 默认低 (SPI Mode 0) */
    W25Q64_CLK_LOW();
    /* MOSI 默认低 */
    W25Q64_MOSI_LOW();

    /* 少量延时让 Flash 就绪 */
    delay(1);
}

/* ============================================================
 * 读取 JEDEC ID
 *   返回值: 0xEF4017(W25Q64JV)
 * ============================================================ */
uint32_t W25Q64_ReadID(void)
{
    uint32_t id = 0;

    W25Q64_CS_LOW();
    W25Q64_SwapByte(W25Q64_CMD_READ_JEDEC_ID);
    id  = (uint32_t)W25Q64_SwapByte(0xFF) << 16;   /* Manufacturer ID */
    id |= (uint32_t)W25Q64_SwapByte(0xFF) << 8;     /* Memory Type     */
    id |= (uint32_t)W25Q64_SwapByte(0xFF);           /* Capacity        */
    W25Q64_CS_HIGH();

    return id;
}

/* ============================================================
 * 读状态寄存器 1
 * ============================================================ */
uint8_t W25Q64_ReadStatusReg1(void)
{
    W25Q64_CS_LOW();
    W25Q64_SwapByte(W25Q64_CMD_READ_STATUS_REG1);
    uint8_t sr = W25Q64_SwapByte(0xFF);
    W25Q64_CS_HIGH();
    return sr;
}

/* ============================================================
 * 清除块保护 (BP0-BP2), 使能全片擦写
 * ============================================================ */
void W25Q64_DisableProtection(void)
{
    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    W25Q64_SwapByte(0x01);  /* Write Status Register */
    W25Q64_SwapByte(0x00);  /* 清除 BP0-BP2, TB, SEC, SRP */
    W25Q64_CS_HIGH();

    W25Q64_WaitBusy(50);
}


/* ============================================================
 * 等待芯片空闲 (超时机制)
 * ============================================================ */
int8_t W25Q64_WaitBusy(uint32_t timeoutMs)
{
    uint32_t elapsed = 0;

    while (W25Q64_ReadStatusReg1() & W25Q64_SR1_BUSY)
    {
        if (timeoutMs > 0)
        {
            if (++elapsed >= timeoutMs)
                return W25Q64_TIMEOUT;
        }
        delay(1);
    }

    return W25Q64_OK;
}

/* ============================================================
 * 写使能 / 写禁止
 * ============================================================ */
void W25Q64_WriteEnable(void)
{
    W25Q64_SendCmdEnd(W25Q64_CMD_WRITE_ENABLE);
}

void W25Q64_WriteDisable(void)
{
    W25Q64_SendCmdEnd(W25Q64_CMD_WRITE_DISABLE);
}

/* ============================================================
 * 扇区擦除 (4KB — 0x20)
 * ============================================================ */
int8_t W25Q64_EraseSector(uint32_t addr)
{
    /* 地址 4KB 对齐检查 (非必须, 但有助于调试) */
    /* if (addr & (W25Q64_SECTOR_SIZE - 1)) return W25Q64_ERR; */

    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    W25Q64_SwapByte(W25Q64_CMD_SECTOR_ERASE);
    W25Q64_SendAddr24(addr);
    W25Q64_CS_HIGH();

    return W25Q64_WaitBusy(1000);   /* 扇区擦除通常 < 400ms */
}

/* ============================================================
 * 块擦除 32KB / 64KB
 * ============================================================ */
int8_t W25Q64_EraseBlock32K(uint32_t addr)
{
    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    W25Q64_SwapByte(W25Q64_CMD_BLOCK_ERASE_32K);
    W25Q64_SendAddr24(addr);
    W25Q64_CS_HIGH();

    return W25Q64_WaitBusy(3000);   /* < 2s 典型 */
}

int8_t W25Q64_EraseBlock64K(uint32_t addr)
{
    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    W25Q64_SwapByte(W25Q64_CMD_BLOCK_ERASE_64K);
    W25Q64_SendAddr24(addr);
    W25Q64_CS_HIGH();

    return W25Q64_WaitBusy(5000);   /* < 4s 典型 */
}

/* ============================================================
 * 全片擦除
 * ============================================================ */
int8_t W25Q64_ChipErase(void)
{
    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    W25Q64_SwapByte(W25Q64_CMD_CHIP_ERASE);
    W25Q64_CS_HIGH();

    return W25Q64_WaitBusy(100000); /* 全片擦除可能 > 40s */
}

/* ============================================================
 * 写一页 (最大 256 字节)
 * ============================================================ */
int8_t W25Q64_WritePage(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (len == 0 || len > W25Q64_PAGE_SIZE)
        return W25Q64_ERR;

    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    W25Q64_SwapByte(W25Q64_CMD_PAGE_PROGRAM);
    W25Q64_SendAddr24(addr);
    W25Q64_SendBytes(data, len);
    W25Q64_CS_HIGH();

    return W25Q64_WaitBusy(50);     /* 页编程 < 3ms (典型 0.7ms) */
}

/* ============================================================
 * 写入任意长度 (自动跨页 + 可选扇区擦除)
 *
 * 注意: NOR Flash 只能 bit 1→0。如果目标地址之前写过,
 *       必须先擦除 (擦除将扇区恢复为全 0xFF)。
 *       设 doErase = 1 将在每次跨入新扇区前执行扇区擦除。
 *       如果已经确保整片已擦除, 设 doErase = 0 可加速。
 * ============================================================ */
int8_t W25Q64_Write(uint32_t addr, const uint8_t *data, uint32_t len, uint8_t doErase)
{
    while (len > 0)
    {
        uint16_t pageOffset = (uint16_t)(addr & (W25Q64_PAGE_SIZE - 1));
        uint16_t chunk = W25Q64_PAGE_SIZE - pageOffset;
        if (chunk > len) chunk = (uint16_t)len;

        /* 如果需要擦除且跨入新扇区起始 */
        if (doErase && (addr & (W25Q64_SECTOR_SIZE - 1)) == 0 && pageOffset == 0)
        {
            int8_t ret = W25Q64_EraseSector(addr);
            if (ret != W25Q64_OK) return ret;
        }

        int8_t ret = W25Q64_WritePage(addr, data, chunk);
        if (ret != W25Q64_OK) return ret;

        addr += chunk;
        data += chunk;
        len  -= chunk;
    }

    return W25Q64_OK;
}

/* ============================================================
 * 读取数据
 * ============================================================ */
void W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint32_t len)
{
    W25Q64_CS_LOW();
    W25Q64_SwapByte(W25Q64_CMD_READ_DATA);
    W25Q64_SendAddr24(addr);
    W25Q64_RecvBytes(buf, len);
    W25Q64_CS_HIGH();
}
