#include "key.h"
#include "ti_msp_dl_config.h"
#include "delay.h"

#define KEY_UP_PORT  KEY_PORT
#define KEY_DOWN_PORT	KEY_PORT
#define KEY_ENTER_PORT	KEY_PORT

/* ============================================================
 * 电平
 * ============================================================ */
#define KEY_PRESSED     KEY_PRESSED_LEVEL
#define KEY_RELEASED    (1 - KEY_PRESSED_LEVEL)

/* ============================================================
 * 状态
 * ============================================================ */
static uint8_t lastUp      = KEY_RELEASED;
static uint8_t lastDown    = KEY_RELEASED;
static uint8_t lastEnter   = KEY_RELEASED;
static uint8_t startupDone = 0;

/* ============================================================
 * 读一个按键（返回 1=按下, 0=松开）
 * ============================================================ */
static uint8_t KEY_Read(GPIO_Regs *port, uint32_t pin)
{
    uint8_t level = (DL_GPIO_readPins(port, pin) & pin) ? 1 : 0;
    return (level == KEY_PRESSED_LEVEL) ? 1 : 0;
}

/* ============================================================
 * Init
 * ============================================================ */
void KEY_Init(void)
{
    startupDone = 0;
}

/* ============================================================
 * Scan — 返回按键事件（消抖 + 边沿检测）
 * ============================================================ */
KeyEvent KEY_Scan(void)
{
    uint8_t up    = KEY_Read(KEY_UP_PORT,    KEY_UP_PIN);
    uint8_t down  = KEY_Read(KEY_DOWN_PORT,  KEY_DOWN_PIN);
    uint8_t enter = KEY_Read(KEY_ENTER_PORT, KEY_ENTER_PIN);

    /* 检测到按下 → 消抖 */
    if (up == KEY_PRESSED)    { delay(KEY_DEBOUNCE_MS); up    = KEY_Read(KEY_UP_PORT,    KEY_UP_PIN); }
    if (down == KEY_PRESSED)  { delay(KEY_DEBOUNCE_MS); down  = KEY_Read(KEY_DOWN_PORT,  KEY_DOWN_PIN); }
    if (enter == KEY_PRESSED) { delay(KEY_DEBOUNCE_MS); enter = KEY_Read(KEY_ENTER_PORT, KEY_ENTER_PIN); }

    KeyEvent event = KEY_NONE;

    /* 边沿检测：上次松开 → 这次按下 */
    if (up == KEY_PRESSED && lastUp == KEY_RELEASED)
        event = KEY_UP;
    else if (down == KEY_PRESSED && lastDown == KEY_RELEASED)
        event = KEY_DOWN;
    else if (enter == KEY_PRESSED && lastEnter == KEY_RELEASED)
        event = KEY_ENTER;

    /* 启动后第一次按键仅消费不触发（避免上电误触发）*/
    if (event != KEY_NONE && !startupDone) {
        startupDone = 1;
        event = KEY_NONE;
    }

    /* 保存当前状态 */
    lastUp    = up;
    lastDown  = down;
    lastEnter = enter;

    return event;
}
