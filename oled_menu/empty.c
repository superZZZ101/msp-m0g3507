/**
 * empty.c — MSPM0G3507 OLED 显示 + 字库下载
 *
 * 所有字符为全角 16×16 字体 (从 W25Q64 GB2312 字库取模)
 * 每行 8 字, 共 4 行
 *
 * 首次使用:
 *   1. 编译下载本固件到 MCU
 *   2. 运行 send_font.bat COMx 下载字库到 W25Q64
 *   3. MCU 自动在 OLED 上显示演示内容
 *
 * 再次上电:
 *   字库已在 W25Q64 中, OLED 直接显示演示内容
 */

#include "w25q64.h"
#include "font_receiver.h"
#include "ti_msp_dl_config.h"
#include "led.h"
#include "delay.h"
#include "uart.h"
#include "OLED.h"

static uint8_t g_count = 0;

/* ============================================================
 * 主函数
 * ============================================================ */
int main(void)
{
    uint8_t ledState = 0;

    /* ── 初始化 ── */
    SYSCFG_DL_init();
    W25Q64_Init();
    OLED_Init();

    /* ── 测试 OLED 本身 (不依赖 W25Q64) ── */
    {
        uint8_t x, y;
        for (y = 0; y < 4; y++)       /* OLED 有 4 页 = 8 行 */
        {
            OLED_SetCursor(y, 0);
            for (x = 0; x < 128; x++)
                OLED_WriteData(0xAA);  /* 交替图案 10101010 */
        }
        delay(1000);  /* 亮 1 秒 */
    }

    /* ── 字库下载完成, 切换显示 ── */
    OLED_SetFontFlashBase(0);
    OLED_Clear();

    /* ── 主循环 ── */
    while (1)
    {
        /* 更新计数 */
        g_count++;
        OLED_ShowNum(3, 5, g_count, 4);

        /* LED 闪烁 */
        if (ledState)
            DL_GPIO_setPins(LED_PORT, LED_LIGHT_PIN);
        else
            DL_GPIO_clearPins(LED_PORT, LED_LIGHT_PIN);
        ledState = !ledState;

        /* 响应 PC 校验命令 */
        if (!DL_UART_isRXFIFOEmpty(UART0_INST))
        {
            uint8_t cmd = DL_UART_receiveData(UART0_INST);
            if (cmd == 'V')
                FontReceiver_VerifyFlash();
        }

        delay(20);
    }
}
