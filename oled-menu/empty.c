/**
 * @file main.c
 * @brief 多级中文菜单演示 —— MSPM0 + I2C OLED + W25Q64 GB2312 字库
 *
 * ╔══════════════════════════════════════════════════════════════╗
 * ║   ⚠  此文件必须以 GB2312 / GBK 编码保存  ⚠               ║
 * ║                                                             ║
 * ║  文件中包含 GB2312 编码的中文字符串（如 "返回"、"设置"），  ║
 * ║  如果以 UTF-8 保存，中文字符会被编码成多字节 UTF-8 序列，   ║
 * ║  OLED 显示时无法正确识别为 GB2312 字符，会出现乱码。       ║
 * ║                                                             ║
 * ║  ✅ Keil MDK 设置：                                         ║
 * ║     Edit → Configuration → Editor → Encoding: Chinese       ║
 * ║     Simplified (GB2312)                                     ║
 * ║                                                             ║
 * ║  ✅ VS Code 设置：                                          ║
 * ║     Settings → Files: Encoding → gb2312                     ║
 * ║     或文件底部状态栏点击 "UTF-8" → "Save with Encoding"     ║
 * ║     → "Chinese Simplified (GB2312)"                        ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * ============================================================
 * 硬件平台：MSPM0G3507（Keil MDK, ARMClang V6.16）
 * 显示器： I2C OLED 128x64 (SSD1306/SH1106), SCL=PB2, SDA=PB3
 * 字库：  W25Q64 (8MB NOR Flash), 存储 GB2312 16x16 点阵字库
 *         首次使用需通过 tools/send_font.bat 烧录字库
 * 按键：  *** PB8/DOWN PB7/ENTER PB6, 内部上拉, 按下=低电平
 * LED：   PB22, 高电平点亮
 *
 * ============================================================
 * 菜单结构：
 *   主菜单 10 项（PID设置 / 八路巡线显示 / 小车电机 / 舵机
 *                / K210串口 / 设置6~10）
 *   每个子菜单含功能X-1、功能X-2、返回
 *   功能执行 LED 闪烁 1~20 次，次数体现选中项
 *
 * 按键操作：
 *   UP/DOWN → 切换选中项
 *   ENTER   → 进入子菜单 / 执行功能
 *
 * 首次使用步骤：
 *   1. 将此文件以 GB2312 编码保存
 *   2. Keil 编译下载固件到 MCU
 *   3. 运行 tools/send_font.bat COMx 烧录字库到 W25Q64
 *   4. 重新上电，OLED 显示中文菜单
 */
#pragma clang diagnostic ignored "-Winvalid-source-encoding"
#include "ti_msp_dl_config.h"
#include "OLED.h"
#include "delay.h"
#include "key.h"
#include "led.h"
#include "oled_menu.h"
#include "w25q64.h"

/* ============================================================
 * LED 控制基础函数
 * ============================================================ */

/* 通用闪烁：闪烁 n 次，每次亮 150ms + 灭 150ms */
static void blink_times(uint8_t n) {
    uint8_t i;
    for (i = 0; i < n; i++) {
        led_on();  delay(150);
        led_off(); delay(150);
    }
}

/* ============================================================
 * 宏批量生成 20 个动作函数 (blink_1 ~ blink_20)
 * ============================================================ */
#define DEFINE_BLINK_FUNC(n) \
    static void blink_##n(void) { blink_times(n); }

DEFINE_BLINK_FUNC(1)
DEFINE_BLINK_FUNC(2)
DEFINE_BLINK_FUNC(3)
DEFINE_BLINK_FUNC(4)
DEFINE_BLINK_FUNC(5)
DEFINE_BLINK_FUNC(6)
DEFINE_BLINK_FUNC(7)
DEFINE_BLINK_FUNC(8)
DEFINE_BLINK_FUNC(9)
DEFINE_BLINK_FUNC(10)
DEFINE_BLINK_FUNC(11)
DEFINE_BLINK_FUNC(12)
DEFINE_BLINK_FUNC(13)
DEFINE_BLINK_FUNC(14)
DEFINE_BLINK_FUNC(15)
DEFINE_BLINK_FUNC(16)
DEFINE_BLINK_FUNC(17)
DEFINE_BLINK_FUNC(18)
DEFINE_BLINK_FUNC(19)
DEFINE_BLINK_FUNC(20)

/* ============================================================
 * 前向声明主菜单数组（子菜单需要引用主菜单的 "返回" 项）
 * ============================================================ */
static const OLED_MenuItem mainItems[10];

/* ============================================================
 * 子菜单定义（10个，每个 3~4 项）
 *
 * 中文文本以 GB2312 编码直接写在字符串中。
 * 文件必须以 GB2312 编码保存，否则编译出的字节序列错误。
 * ============================================================ */
static const OLED_MenuItem subMenu1[] = {
    {(const uint8_t*)"P", NULL, 0, blink_1},
    {(const uint8_t*)"I", NULL, 0, blink_2},
    {(const uint8_t*)"D", NULL, 0, blink_3},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu2[] = {
    {(const uint8_t*)"功能2-1", NULL, 0, blink_3},
    {(const uint8_t*)"功能2-2", NULL, 0, blink_4},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu3[] = {
    {(const uint8_t*)"功能3-1", NULL, 0, blink_5},
    {(const uint8_t*)"功能3-2", NULL, 0, blink_6},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu4[] = {
    {(const uint8_t*)"功能4-1", NULL, 0, blink_7},
    {(const uint8_t*)"功能4-2", NULL, 0, blink_8},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu5[] = {
    {(const uint8_t*)"功能5-1", NULL, 0, blink_9},
    {(const uint8_t*)"功能5-2", NULL, 0, blink_10},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu6[] = {
    {(const uint8_t*)"功能6-1", NULL, 0, blink_11},
    {(const uint8_t*)"功能6-2", NULL, 0, blink_12},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu7[] = {
    {(const uint8_t*)"功能7-1", NULL, 0, blink_13},
    {(const uint8_t*)"功能7-2", NULL, 0, blink_14},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu8[] = {
    {(const uint8_t*)"功能8-1", NULL, 0, blink_15},
    {(const uint8_t*)"功能8-2", NULL, 0, blink_16},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu9[] = {
    {(const uint8_t*)"功能9-1", NULL, 0, blink_17},
    {(const uint8_t*)"功能9-2", NULL, 0, blink_18},
    {(const uint8_t*)"返回",     mainItems, 10}
};

static const OLED_MenuItem subMenu10[] = {
    {(const uint8_t*)"功能10-1", NULL, 0, blink_19},
    {(const uint8_t*)"功能10-2", NULL, 0, blink_20},
    {(const uint8_t*)"返回",      mainItems, 10}
};

/* ============================================================
 * 主菜单（10 项）
 * ============================================================ */
static const OLED_MenuItem mainItems[10] = {
    {(const uint8_t*)"PID设置",   subMenu1,  OLED_MENU_COUNT(subMenu1)},
    {(const uint8_t*)"八路巡线显示", subMenu2, OLED_MENU_COUNT(subMenu2)},
    {(const uint8_t*)"小车电机",   subMenu3,  OLED_MENU_COUNT(subMenu3)},
    {(const uint8_t*)"舵机",       subMenu4,  OLED_MENU_COUNT(subMenu4)},
    {(const uint8_t*)"K210串口",  subMenu5,  OLED_MENU_COUNT(subMenu5)},
    {(const uint8_t*)"设置6",      subMenu6,  OLED_MENU_COUNT(subMenu6)},
    {(const uint8_t*)"设置7",      subMenu7,  OLED_MENU_COUNT(subMenu7)},
    {(const uint8_t*)"设置8",      subMenu8,  OLED_MENU_COUNT(subMenu8)},
    {(const uint8_t*)"设置9",      subMenu9,  OLED_MENU_COUNT(subMenu9)},
    {(const uint8_t*)"设置10",    subMenu10, OLED_MENU_COUNT(subMenu10)}
};

static OLED_Menu menu;

/* ============================================================
 * 主函数
 * ============================================================ */
int main(void)
{
    SYSCFG_DL_init();                  /* SysConfig 初始化 (时钟, GPIO, UART) */
    W25Q64_Init();                     /* W25Q64 SPI 初始化 */
    delay(5);

    /* 可选：检测 W25Q64 是否正常（0xEF4017 = W25Q64JV） */
    uint32_t id = W25Q64_ReadID();
    if (id == 0xEF4017) {
        /* W25Q64 正常, 字库已就绪 */
    }

    KEY_Init();                        /* 按键初始化 */
    OLED_Init();                       /* OLED 初始化 */
    OLED_SetFontFlashBase(GB2312_FONT_FLASH_ADDR);  /* 字库基地址 */

    OLED_Menu_Init(&menu);
    OLED_Menu_SetRoot(&menu, mainItems, 10);
    OLED_Menu_Draw(&menu);

    /* 主循环：按键扫描 → 菜单导航 */
    while (1) {
        KeyEvent key = KEY_Scan();
        switch (key) {
            case KEY_UP:    OLED_Menu_Prev(&menu); break;
            case KEY_DOWN:  OLED_Menu_Next(&menu); break;
            case KEY_ENTER: OLED_Menu_Enter(&menu); break;
            default: break;
        }
    }
}
