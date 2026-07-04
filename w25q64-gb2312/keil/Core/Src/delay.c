/*
 * delay.c — MSPM0G3507 毫秒级阻塞延时
 *
 * CPUCLK_FREQ = 32 MHz, delay_cycles(32000) = 1 ms
 *
 * 依赖:
 *   - ti_msp_dl_config.h (CPUCLK_FREQ)
 *   - MSPM0 DriverLib    (delay_cycles)
 */

#include <stdint.h>
#include "ti_msp_dl_config.h"

/**
 * @brief  毫秒级阻塞延时
 * @param  x  延时毫秒数
 *
 * 注: 此实现使用 delay_cycles, 是阻塞式延时,
 *     在延时期间 CPU 无法处理其他任务。
 */
void delay(uint16_t x)
{
    do {
        delay_cycles(32000);  /* 32 MHz 下约等于 1 ms */
    } while (x--);
}
