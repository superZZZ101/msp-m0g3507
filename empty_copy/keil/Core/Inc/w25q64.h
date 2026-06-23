#ifndef __W25Q64_H
#define __W25Q64_H

#include "stdint.h"

/* ============================================================
 * W25Q64 SPI Flash 驱动
 * 容量: 8MB (64Mbit)
 * 页: 256 字节
 * 扇区: 4KB
 * 擦除寿命: ~100K 次
 * ============================================================ */

#define W25Q64_PAGE_SIZE     256
#define W25Q64_SECTOR_SIZE   4096
#define W25Q64_TOTAL_SIZE    (8UL * 1024 * 1024)  // 8MB

/**
 * @brief 初始化 SPI 引脚
 */
void W25Q64_Init(void);

/**
 * @brief 读取芯片 ID (0x90 指令)
 * @return 0xEF14 = W25Q64 正常
 */
uint16_t W25Q64_ReadID(void);

/**
 * @brief 读取数据
 */
void W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len);

/**
 * @brief 写入数据（调用前需先擦除）
 */
void W25Q64_Write(uint32_t addr, const uint8_t *data, uint32_t len);

/**
 * @brief 擦除一个扇区 (4KB)
 */
void W25Q64_EraseSector(uint32_t addr);

/**
 * @brief 擦除整片（用时较长 ~40s）
 */
void W25Q64_EraseChip(void);

#endif
