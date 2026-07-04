/**
 * empty.c -- MSPM0G3507 OLED 显示 + W25Q64 GB2312 字库展示 Demo
 *
 * 所有字符为全角 16x16 字体 (从 W25Q64 中 GB2312 字库取模)
 * 显示布局: 每行 8 个汉字, 共 4 行 (128x64 OLED)
 *
 * 首次使用:
 *   1. 编译下载固件到 MCU
 *   2. 运行 send_font.bat COMx 下载字库到 W25Q64
 *   3. MCU 自动在 OLED 上显示演示内容
 *
 * 再次上电:
 *   字库已在 W25Q64 中持久保存, OLED 直接显示演示内容
 */

#include <stdio.h>
#include "w25q64.h"
#include "font_receiver.h"
#include "ti_msp_dl_config.h"
#include "led.h"
#include "delay.h"
#include "uart.h"
#include "OLED.h"

/* ============================================================
 * 主函数 - W25Q64 驱动 + OLED 显示 + GB2312 字库演示
 * ============================================================ */
int main(void)
{
    SYSCFG_DL_init();                        /* SysConfig 初始化 (时钟/GPIO/UART) */
    W25Q64_Init();                           /* W25Q64 SPI 初始化 */
    OLED_Init();                             /* OLED 初始化 (SH1106/SSD1306) */
		FontReceiver_Run(GB2312_FONT_FLASH_ADDR);
    OLED_SetFontFlashBase(GB2312_FONT_FLASH_ADDR);  /* 字库基地址 = 0 */
		
    OLED_ShowNum(1, 1, 1234, 4);             /* 第一行显示数字 1234 */
    OLED_ShowChineseString(2, 1, (const uint8_t*)"何意味？");  /* 第二行显示汉字 "" (GB2312) */
    led_on();                                /* 点亮 LED 指示运行 */

    while (1);   /* 主循环卡住, 不让程序退出 */
}

/* --- printf 重定向 (Keil MicroLIB) --- */
int fputc(int ch, FILE *f)
{
    (void)f;
    uart_putc((uint8_t)ch);
    if (ch == '\n') uart_putc('\r');
    return ch;
}
