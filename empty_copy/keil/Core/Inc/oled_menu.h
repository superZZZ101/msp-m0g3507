#ifndef __OLED_MENU_H
#define __OLED_MENU_H

#include "stdint.h"
#include <stddef.h>
/* ============================================================
 * 多层菜单模块 — 支持子菜单自动导航
 *
 * 用法:
 *   static const OLED_MenuItem pwmItems[];
 *
 *   static const OLED_MenuItem mainItems[] = {
 *       {"PWM",     pwmItems, OLED_MENU_COUNT(pwmItems)},
 *       {"PID",     NULL, 0},
 *       {"ENCODER", NULL, 0},
 *   };
 *   static const OLED_MenuItem pwmItems[] = {
 *       {"Set Freq", NULL, 0},
 *       {"Set Duty", NULL, 0},
 *       {"< Back",   NULL, 0},
 *   };
 *
 *   OLED_Menu menu;
 *   OLED_Menu_Init(&menu, COLOR_GREEN, COLOR_BLUE);
 *   OLED_Menu_SetRoot(&menu, mainItems, OLED_MENU_COUNT(mainItems));
 *
 *   uint8_t sel = OLED_Menu_Enter(&menu);
 *   if (sel != 0xFF) {  叶子节点被选中 }//JIAZHUSHI
 *   OLED_Menu_Back(&menu);  // 返回上级
 * ============================================================ */

#define OLED_MENU_COUNT(arr)       (sizeof(arr) / sizeof((arr)[0]))
#define OLED_MENU_MAX_DEPTH        4
#define OLED_MENU_ENTER_SUBMENU   0xFF   // Enter 返回此值表示进入了子菜单

/* 菜单项回调 — 选中叶子节点时自动调用 */
typedef void (*MenuAction)(void);

/* --- 菜单项（支持嵌套子菜单） --- */
typedef struct OLED_MenuItem OLED_MenuItem;

struct OLED_MenuItem {
    const char *text;                // 显示文本
    const OLED_MenuItem *submenu;    // 子菜单数组（NULL = 无子菜单）
    uint8_t submenuCount;           // 子菜单项数（0 = 无子菜单）
    MenuAction action;              // 叶子节点回调函数（NULL = 无回调）
};

/* --- 菜单实例 --- */
typedef struct {
    const OLED_MenuItem *items;      // 当前菜单数组
    uint8_t count;                   // 当前项数
    uint8_t selection;               // 当前选中 (0-based)
    uint16_t textColor;
    uint16_t selColor;
    uint8_t startLine;
    uint8_t visibleLines;
    uint8_t viewTop;

    // 历史栈
    struct {
        const OLED_MenuItem *items;
        uint8_t count;
        uint8_t selection;
    } history[OLED_MENU_MAX_DEPTH];
    uint8_t historyDepth;
} OLED_Menu;

/* ============================================================
 * API
 * ============================================================ */

void OLED_Menu_Init(OLED_Menu *menu, uint16_t textColor, uint16_t selColor);
void OLED_Menu_SetRoot(OLED_Menu *menu, const OLED_MenuItem *items, uint8_t count);
void OLED_Menu_SetOrientation(OLED_Menu *menu, uint8_t mode);

void OLED_Menu_Draw(const OLED_Menu *menu);
void OLED_Menu_Next(OLED_Menu *menu);
void OLED_Menu_Prev(OLED_Menu *menu);

/**
 * @brief 确认当前选项
 * @return 若选中项有子菜单 → 自动进入并返回 OLED_MENU_ENTER_SUBMENU
 *         若为叶子节点（无子菜单） → 返回选中索引 (0-based)
 */
uint8_t OLED_Menu_Enter(OLED_Menu *menu);

/**
 * @brief 返回上级菜单
 * @return 0 = 已在根菜单, 1 = 成功返回上级
 */
uint8_t OLED_Menu_Back(OLED_Menu *menu);

uint8_t OLED_Menu_GetSelection(const OLED_Menu *menu);
uint8_t OLED_Menu_IsRoot(const OLED_Menu *menu);

#endif
