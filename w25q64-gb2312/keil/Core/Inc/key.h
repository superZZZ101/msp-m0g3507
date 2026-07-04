#ifndef __KEY_H
#define __KEY_H

#include "stdint.h"

/* ============================================================
 * 三按键导航模块 — UP / DOWN / ENTER
 *
 * 硬件事项：
 *   请在使用前在 SysConfig (ti_msp_dl_config.h) 中定义以下宏，
 *   或在编译选项中添加：
 *
 *   #define KEY_UP_PORT     GPIOB
 *   #define KEY_UP_PIN      DL_GPIO_PIN_27
 *   #define KEY_DOWN_PORT   GPIOB
 *   #define KEY_DOWN_PIN    DL_GPIO_PIN_23
 *   #define KEY_ENTER_PORT  GPIOA
 *   #define KEY_ENTER_PIN   DL_GPIO_PIN_12
 *
 *   若你的按键是低电平按下，无需修改；若高电平按下，修改 KEY_PRESSED_LEVEL。
 * ============================================================ */

/* ── 按键事件 ── */
typedef enum {
    KEY_NONE   = 0,
    KEY_UP     = 1,
    KEY_DOWN   = 2,
    KEY_ENTER  = 3,
} KeyEvent;

/* ── 按键有效电平（0=低电平按下，1=高电平按下）── */
#ifndef KEY_PRESSED_LEVEL
#define KEY_PRESSED_LEVEL    0
#endif

/* ── 消抖延时 (ms) ── */
#ifndef KEY_DEBOUNCE_MS
#define KEY_DEBOUNCE_MS     20
#endif

/* ============================================================
 * API
 * ============================================================ */

/** @brief 按键初始化（若已由 SysConfig 配置 GPIO 方向，可为空）*/
void KEY_Init(void);

/**
 * @brief 扫描按键，返回单次按下事件（边沿检测，含消抖）
 *        需在主循环中周期性调用（建议 10~20ms 间隔）
 */
KeyEvent KEY_Scan(void);

#endif
