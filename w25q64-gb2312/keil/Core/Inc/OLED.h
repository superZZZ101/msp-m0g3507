#ifndef __OLED_H
#define __OLED_H
#include "stdint.h"
#include "w25q64.h"

/* ============================================================
 * 基础函数 — 全角 16×16 (从 W25Q64 GB2312 字库取模)
 *
 * 所有函数使用统一坐标:
 *   Line:   1~4 (4 行, 每行 16 像素高)
 *   Column: 1~8 (8 列, 每字 16 像素宽)
 * ============================================================ */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/* ============================================================
 * 中文显示 (GB2312, 16×16, 从 W25Q64 读字模)
 *   坐标与基础函数一致: Line 1~4, Column 1~8
 * ============================================================ */

/* W25Q64 中 GB2312 字库的起始地址（4KB 扇区对齐）*/
#define GB2312_FONT_FLASH_ADDR      0
#define GB2312_ZONE_COUNT           94
#define GB2312_BYTES_PER_CHAR       32

void OLED_SetFontFlashBase(uint32_t addr);
int8_t OLED_ShowChinese(uint8_t Line, uint8_t Column, uint16_t gbCode);
void OLED_ShowChineseString(uint8_t Line, uint8_t Column, const uint8_t *str);
void OLED_ShowMixedString(uint8_t Line, uint8_t Column, const uint8_t *str);

#endif
