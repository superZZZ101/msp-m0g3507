#ifndef __OLED_SPI_H
#define __OLED_SPI_H

#include "stdint.h"

/* ============================================================
 * ST7735S 0.96" 80x160 RGB IPS OLED 驱动
 * 接口: 4-wire SPI (软件模拟)
 * 色深: 65K (RGB565, 16-bit)
 * ============================================================ */

/* --- 显示方向 --- */
#define OLED_ORIENTATION_PORTRAIT         0   // 竖屏  80x160
#define OLED_ORIENTATION_LANDSCAPE        1   // 横屏 160x80
#define OLED_ORIENTATION_PORTRAIT_REV     2   // 竖屏倒置  80x160
#define OLED_ORIENTATION_LANDSCAPE_REV    3   // 横屏倒置 160x80

/* --- 运行时分辨率 & 偏移 --- */
extern uint16_t OLED_Width;
extern uint16_t OLED_Height;
extern uint16_t OLED_X_Off;
extern uint16_t OLED_Y_Off;
extern uint8_t  OLED_Orientation;

/* --- 像素颜色 --- */
#define RGB565(r, g, b)  ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
#define COLOR_BLACK      RGB565(0,0,0)
#define COLOR_WHITE      RGB565(255,255,255)
#define COLOR_RED        RGB565(255,0,0)
#define COLOR_GREEN      RGB565(0,255,0)
#define COLOR_BLUE       RGB565(0,0,255)
#define COLOR_YELLOW     RGB565(255,255,0)
#define COLOR_CYAN       RGB565(0,255,255)
#define COLOR_MAGENTA    RGB565(255,0,255)

/* --- 全局前景色/背景色 (用于文字渲染) --- */
extern uint16_t OLED_ForeColor;
extern uint16_t OLED_BackColor;

/* ============================================================
 * API — 所有带颜色参数的函数都直接以此名调用
 * ============================================================ */

void OLED_Init(void);
void OLED_SetOrientation(uint8_t mode);

void OLED_DrawPixel_Ex(uint8_t x, uint8_t y, uint16_t color);
void OLED_Fill(uint16_t color);
void OLED_FillRow(uint8_t Line, uint16_t color);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char, uint16_t color);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String, uint16_t color);

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

void OLED_MenuRow(uint8_t Line, const char *text, uint16_t textColor, uint16_t selColor, uint8_t selected);

void OLED_CalibrateOffset(void);
void OLED_ShowChinese16(uint8_t x, uint8_t y, const uint8_t bitmap[32]);

/* ============================================================
 * 便捷宏
 * ============================================================ */
#define OLED_DrawPixel(x, y)       OLED_DrawPixel_Ex(x, y, COLOR_WHITE)
#define OLED_FillWhite()           OLED_Fill(COLOR_WHITE)
#define OLED_Clear()               OLED_Fill(COLOR_BLACK)

/* 预置汉字字模 */
extern const uint8_t CN_NIU[32];
extern const uint8_t CN_CHU[32];
extern const uint8_t CN_JI[32];

#endif
