#include "w25q64.h"
#include "ti_msp_dl_config.h"
#include "delay.h"

/* ============================================================
 * W25Q64 SPI 引脚配置 — 按实际接线修改
 * ============================================================ */
#ifndef W25Q64_CS_PORT
#define W25Q64_CS_PORT   GPIOB
#define W25Q64_CS_PIN    DL_GPIO_PIN_XX   /* TODO: 改实际引脚 */
#endif
#ifndef W25Q64_SCK_PORT
#define W25Q64_SCK_PORT  GPIOB
#define W25Q64_SCK_PIN   DL_GPIO_PIN_XX
#endif
#ifndef W25Q64_MOSI_PORT
#define W25Q64_MOSI_PORT GPIOB
#define W25Q64_MOSI_PIN  DL_GPIO_PIN_XX
#endif
#ifndef W25Q64_MISO_PORT
#define W25Q64_MISO_PORT GPIOB
#define W25Q64_MISO_PIN  DL_GPIO_PIN_XX
#endif

/* ============================================================
 * SPI 底层（软件模拟）
 * ============================================================ */
#define FLASH_CS(x)   ((x) ? DL_GPIO_setPins(W25Q64_CS_PORT, W25Q64_CS_PIN)   : DL_GPIO_clearPins(W25Q64_CS_PORT, W25Q64_CS_PIN))
#define FLASH_SCK(x)  ((x) ? DL_GPIO_setPins(W25Q64_SCK_PORT, W25Q64_SCK_PIN) : DL_GPIO_clearPins(W25Q64_SCK_PORT, W25Q64_SCK_PIN))
#define FLASH_MOSI(x) ((x) ? DL_GPIO_setPins(W25Q64_MOSI_PORT, W25Q64_MOSI_PIN) : DL_GPIO_clearPins(W25Q64_MOSI_PORT, W25Q64_MOSI_PIN))
#define FLASH_MISO()  (DL_GPIO_readPins(W25Q64_MISO_PORT, W25Q64_MISO_PIN) & W25Q64_MISO_PIN)

static void Flash_Delay(void) {
    for (volatile uint8_t i = 0; i < 4; i++);
}

static uint8_t Flash_SPI_IO(uint8_t dat) {
    uint8_t rx = 0;
    for (uint8_t i = 0; i < 8; i++) {
        FLASH_SCK(0);
        FLASH_MOSI((dat & 0x80) ? 1 : 0);
        dat <<= 1;
        Flash_Delay();
        FLASH_SCK(1);
        rx = (rx << 1) | (FLASH_MISO() ? 1 : 0);
        Flash_Delay();
    }
    FLASH_SCK(0);
    return rx;
}

static void Flash_Select(void)  { FLASH_CS(0); Flash_Delay(); }
static void Flash_Release(void) { Flash_Delay(); FLASH_CS(1); }

/* ============================================================
 * 指令
 * ============================================================ */
#define CMD_WREN      0x06
#define CMD_RDSR      0x05
#define CMD_READ      0x03
#define CMD_PP        0x02   // Page Program
#define CMD_SE        0x20   // Sector Erase (4KB)
#define CMD_CE        0xC7   // Chip Erase
#define CMD_RDID      0x90

/* ============================================================
 * 等待忙
 * ============================================================ */
static void Flash_WaitBusy(void) {
    Flash_Select();
    Flash_SPI_IO(CMD_RDSR);
    while (Flash_SPI_IO(0) & 0x01);  // BUSY bit
    Flash_Release();
}

/* ============================================================
 * Init
 * ============================================================ */
void W25Q64_Init(void) {
    Flash_Release();
    Flash_Delay();
}

/* ============================================================
 * ReadID
 * ============================================================ */
uint16_t W25Q64_ReadID(void) {
    uint16_t id = 0;
    Flash_Select();
    Flash_SPI_IO(CMD_RDID);
    Flash_SPI_IO(0x00);
    Flash_SPI_IO(0x00);
    id = (uint16_t)Flash_SPI_IO(0) << 8;
    id |= Flash_SPI_IO(0);
    Flash_Release();
    return id;
}

/* ============================================================
 * Read
 * ============================================================ */
void W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len) {
    Flash_Select();
    Flash_SPI_IO(CMD_READ);
    Flash_SPI_IO((uint8_t)(addr >> 16));
    Flash_SPI_IO((uint8_t)(addr >> 8));
    Flash_SPI_IO((uint8_t)(addr));
    for (uint32_t i = 0; i < len; i++)
        buf[i] = Flash_SPI_IO(0);
    Flash_Release();
}

/* ============================================================
 * Write Enable
 * ============================================================ */
static void Flash_WriteEnable(void) {
    Flash_Select();
    Flash_SPI_IO(CMD_WREN);
    Flash_Release();
}

/* ============================================================
 * Page Program (256 字节)
 * ============================================================ */
void W25Q64_Write(uint32_t addr, const uint8_t *data, uint32_t len) {
    while (len > 0) {
        uint32_t n = (len > W25Q64_PAGE_SIZE) ? W25Q64_PAGE_SIZE : len;
        Flash_WaitBusy();
        Flash_WriteEnable();
        Flash_Select();
        Flash_SPI_IO(CMD_PP);
        Flash_SPI_IO((uint8_t)(addr >> 16));
        Flash_SPI_IO((uint8_t)(addr >> 8));
        Flash_SPI_IO((uint8_t)(addr));
        for (uint32_t i = 0; i < n; i++)
            Flash_SPI_IO(data[i]);
        Flash_Release();
        addr += n;
        data += n;
        len  -= n;
    }
    Flash_WaitBusy();
}

/* ============================================================
 * Erase Sector (4KB)
 * ============================================================ */
void W25Q64_EraseSector(uint32_t addr) {
    Flash_WaitBusy();
    Flash_WriteEnable();
    Flash_Select();
    Flash_SPI_IO(CMD_SE);
    Flash_SPI_IO((uint8_t)(addr >> 16));
    Flash_SPI_IO((uint8_t)(addr >> 8));
    Flash_SPI_IO((uint8_t)(addr));
    Flash_Release();
    Flash_WaitBusy();
}

/* ============================================================
 * Erase Chip (~40s)
 * ============================================================ */
void W25Q64_EraseChip(void) {
    Flash_WaitBusy();
    Flash_WriteEnable();
    Flash_Select();
    Flash_SPI_IO(CMD_CE);
    Flash_Release();
    Flash_WaitBusy();
}
