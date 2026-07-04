#ifndef __OLED_FONT_H
#define __OLED_FONT_H

#include "stdint.h"

/* ============================================================
 * 外置字库 — 从 W25Q64 读取 GB2312 16×16 汉字并显示
 *
 * 字库烧录:
 *   将 HZK16 文件（6763 个汉字 + 常用符号）原始二进制
 *   从 W25Q64 的 FONT_BASE_ADDR 地址开始烧入。
 *
 * GB2312 编码:
 *   高字节 = 区码 + 0xA1, 低字节 = 位码 + 0xA1
 *   "牛" = 0xC5A3
 * ============================================================ */

/* 字库在 W25Q64 中的起始地址 */
#define FONT_BASE_ADDR   0

/**
 * @brief 显示一个 GB2312 汉字（从 W25Q64 读取字模）
 * @param x, y    像素坐标
 * @param gb2312  汉字 GB2312 编码 (2 字节, 如 0xC5A3 为"牛")
 * @param color   前景色
 *
 * 注: 背景不填充，需自行用 OLED_FillRow 或预刷背景
 */
void OLED_ShowGB2312(uint8_t x, uint8_t y, uint16_t gb2312, uint16_t color);

/**
 * @brief 在行/列坐标显示 GB2312 字符串（自动从 W25Q64 读字模）
 * @param Line, Column  行/列坐标 (1-based)
 * @param str           GB2312 编码的字符串
 * @param color         前景色
 *
 * 每行自动换行，英文/数字暂不支持混排（全角汉字等宽处理）
 */
void OLED_ShowChineseStr(uint8_t Line, uint8_t Column, const char *str, uint16_t color);

#endif
