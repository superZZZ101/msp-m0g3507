#include "oled_font.h"
#include "oled_spi.h"
#include "w25q64.h"

/* ============================================================
 * 从 W25Q64 读取一个 GB2312 16×16 字模
 * ============================================================ */
static void ReadGlyph(uint16_t gb2312, uint8_t glyph[32]) {
    uint8_t hi = (uint8_t)(gb2312 >> 8);
    uint8_t lo = (uint8_t)(gb2312);

    // GB2312 区码/位码 (0-based)
    uint16_t area = hi - 0xA1;
    uint16_t pos  = lo - 0xA1;

    uint32_t offset = (uint32_t)(area * 94 + pos) * 32;
    uint32_t addr   = FONT_BASE_ADDR + offset;

    W25Q64_Read(addr, glyph, 32);
}

/* ============================================================
 * 显示一个 GB2312 汉字（不画背景）
 * ============================================================ */
void OLED_ShowGB2312(uint8_t x, uint8_t y, uint16_t gb2312, uint16_t color) {
    uint8_t glyph[32];
    ReadGlyph(gb2312, glyph);

    for (uint8_t row = 0; row < 16; row++) {
        uint8_t b1 = glyph[row * 2];
        uint8_t b2 = glyph[row * 2 + 1];
        for (uint8_t col = 0; col < 8; col++) {
            if (b1 & (0x80 >> col))
                OLED_DrawPixel_Ex(x + col, y + row, color);
        }
        for (uint8_t col = 0; col < 8; col++) {
            if (b2 & (0x80 >> col))
                OLED_DrawPixel_Ex(x + 8 + col, y + row, color);
        }
    }
}

/* ============================================================
 * 行/列坐标显示 GB2312 字符串
 * ============================================================ */
void OLED_ShowChineseStr(uint8_t Line, uint8_t Column, const char *str, uint16_t color) {
    uint8_t x = (Column - 1) * 16;   // 汉字等宽 16px
    uint8_t y = (Line - 1) * 16;

    while (*str != '\0') {
        uint8_t hi = (uint8_t)(*str);
        str++;
        if (*str == '\0') break;
        uint8_t lo = (uint8_t)(*str);
        str++;

        // 跳过 ASCII 字符
        if (hi < 0xA1 || lo < 0xA1) continue;

        uint16_t gb2312 = ((uint16_t)hi << 8) | lo;
        OLED_ShowGB2312(x, y, gb2312, color);
        x += 16;
    }
}
