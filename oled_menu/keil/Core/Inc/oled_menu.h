#ifndef __OLED_MENU_H
#define __OLED_MENU_H

#include "stdint.h"
#include "stdio.h"
/* ============================================================
 * SSD1306 I2C OLED 简易菜单模块
 *
 * 适配 128×64 单色 OLED，8×16 字体，4 行 × 16 列
 *
 * 特性:
 *   - 多级菜单嵌套（子菜单支持）
 *   - 光标选择 ( `>` 指示 )
 *   - 项目数超过 4 时自动翻页
 *   - 叶子节点回调函数
 *
 * 用法:
 *
 *   // 1. 定义菜单项，子菜单先于父菜单
 *   static const OLED_MenuItem subItems[] = {
 *       {"Set Freq", NULL, 0, NULL},
 *       {"Set Duty", NULL, 0, NULL},
 *       {"< Back",   mainItems, 3, NULL},
 *   };
 *
 *   static const OLED_MenuItem mainItems[] = {
 *       {"SUB MENU", subItems, OLED_MENU_COUNT(subItems), NULL},
 *       {"ITEM 2",   NULL, 0, NULL},
 *       {"ITEM 3",   NULL, 0, myAction},
 *   };
 *
 *   // 2. 初始化、设置根菜单
 *   OLED_Menu menu;
 *   OLED_Menu_Init(&menu);
 *   OLED_Menu_SetRoot(&menu, mainItems, OLED_MENU_COUNT(mainItems));
 *   OLED_Menu_Draw(&menu);
 *
 *   // 3. 主循环中响应按键
 *   switch(key) {
 *       case KEY_UP:    OLED_Menu_Prev(&menu); break;
 *       case KEY_DOWN:  OLED_Menu_Next(&menu); break;
 *       case KEY_ENTER: OLED_Menu_Enter(&menu); break;
 *   }
 *
 *   // 4. 返回上级: OLED_Menu_Back(&menu)
 * ============================================================ */

/* 工具宏 */
#define OLED_MENU_COUNT(arr)       (sizeof(arr) / sizeof((arr)[0]))
#define OLED_MENU_MAX_DEPTH        4
#define OLED_MENU_ENTER_SUBMENU   0xFF

/* 可见行数 (8×16 字体, 64px 高) */
#define OLED_MENU_VISIBLE_LINES    4

/* 叶子节点回调 */
typedef void (*MenuAction)(void);

/* ── 菜单项 ── */
typedef struct OLED_MenuItem OLED_MenuItem;

struct OLED_MenuItem {
    const char *text;                 /* 显示文本（最多16字符）*/
    const OLED_MenuItem *submenu;     /* 子菜单数组 (NULL=无) */
    uint8_t submenuCount;            /* 子菜单项数 (0=无) */
    MenuAction action;                /* 叶子回调 (NULL=无) */
};

/* ── 菜单实例 ── */
typedef struct {
    const OLED_MenuItem *items;       /* 当前菜单数组 */
    uint8_t count;                    /* 当前项数 */
    uint8_t selection;                /* 当前选中 (0-based) */
    uint8_t viewTop;                  /* 视口起始索引 */

    /* 历史栈 */
    struct {
        const OLED_MenuItem *items;
        uint8_t count;
        uint8_t selection;
    } history[OLED_MENU_MAX_DEPTH];

    uint8_t historyDepth;
} OLED_Menu;

/* ============================================================
 * API 函数
 * ============================================================ */

/** @brief 初始化菜单实例 */
void OLED_Menu_Init(OLED_Menu *menu);

/** @brief 设置根菜单 */
void OLED_Menu_SetRoot(OLED_Menu *menu, const OLED_MenuItem *items, uint8_t count);

/** @brief 全量刷新当前菜单页面 */
void OLED_Menu_Draw(const OLED_Menu *menu);

/** @brief 选中上移（循环）*/
void OLED_Menu_Prev(OLED_Menu *menu);

/** @brief 选中下移（循环）*/
void OLED_Menu_Next(OLED_Menu *menu);

/**
 * @brief 确认/进入
 * @return OLED_MENU_ENTER_SUBMENU (0xFF) = 进入了子菜单
 *         0..count-1 = 叶子节点被选中
 *         若动作回调非 NULL 则自动执行
 */
uint8_t OLED_Menu_Enter(OLED_Menu *menu);

/**
 * @brief 返回上级菜单
 * @return 0 = 已在根菜单, 1 = 成功返回
 */
uint8_t OLED_Menu_Back(OLED_Menu *menu);

/** @brief 获取当前选中索引 */
uint8_t OLED_Menu_GetSelection(const OLED_Menu *menu);

/** @brief 判断是否在根菜单 */
uint8_t OLED_Menu_IsRoot(const OLED_Menu *menu);

#endif
