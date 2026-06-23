#include "oled_menu.h"
#include "OLED.h"       /* OLED_ShowString, OLED_Clear, etc. */

/* ============================================================
 * 局部辅助
 * ============================================================ */

/**
 * @brief 计算视口可滚动的最大起始索引（分页模式下）
 *        当 count <= 4 时返回 0
 */
static uint8_t OLED_Menu_MaxViewTop(const OLED_Menu *menu)
{
    if (menu->count <= OLED_MENU_VISIBLE_LINES)
        return 0;
    return menu->count - OLED_MENU_VISIBLE_LINES;
}

/**
 * @brief 翻页调整 — 在循环滚动到边界时翻页，普通跨页也翻页
 */
static void OLED_Menu_AdjustView(OLED_Menu *menu, uint8_t oldSel)
{
    uint8_t maxTop = OLED_Menu_MaxViewTop(menu);

    /* 从末尾循环到开头：翻到第一页 */
    if (oldSel == menu->count - 1 && menu->selection == 0)
    {
        menu->viewTop = 0;
        return;
    }

    /* 从开头循环到末尾：翻到最后一页 */
    if (oldSel == 0 && menu->selection == menu->count - 1)
    {
        menu->viewTop = maxTop;
        return;
    }

    /* 选中项跑出视口下方 → 向下翻一行 */
    if (menu->selection >= menu->viewTop + OLED_MENU_VISIBLE_LINES)
    {
        menu->viewTop++;
        if (menu->viewTop > maxTop)
            menu->viewTop = maxTop;
        return;
    }

    /* 选中项跑出视口上方 → 向上翻一行 */
    if (menu->selection < menu->viewTop)
    {
        if (menu->viewTop > 0)
            menu->viewTop--;
    }
}

/* ============================================================
 * API 实现
 * ============================================================ */

void OLED_Menu_Init(OLED_Menu *menu)
{
    menu->items        = NULL;
    menu->count        = 0;
    menu->selection    = 0;
    menu->viewTop      = 0;
    menu->historyDepth = 0;
}

void OLED_Menu_SetRoot(OLED_Menu *menu, const OLED_MenuItem *items, uint8_t count)
{
    menu->items        = items;
    menu->count        = count;
    menu->selection    = 0;
    menu->viewTop      = 0;
    menu->historyDepth = 0;
}

/* ============================================================
 * Draw —— 全量刷新
 *
 * 布局示例 (128×64, 4行, 每行最多16字符):
 *
 *   > Item 1              ← 选中项前加 '>'
 *     Item 2              ← 空格占位
 *     Item 3              ← ...
 *     Item 4
 *
 * 当 count <= 4 时不翻页；超过时 viewTop 控制可见窗口。
 * ============================================================ */
void OLED_Menu_Draw(const OLED_Menu *menu)
{
    uint8_t i;
    char buf[17];  /* 16 字符 + '\0' */

    if (menu->items == NULL) return;

    OLED_Clear();

    uint8_t end = menu->viewTop + OLED_MENU_VISIBLE_LINES;
    if (end > menu->count)
        end = menu->count;

    for (i = menu->viewTop; i < end; i++)
    {
        uint8_t line = (i - menu->viewTop) + 1;  /* Line: 1..4 */
        uint8_t isSel = (i == menu->selection);

        /* 构造显示文本: "> Item" 或 "  Item" */
        buf[0] = isSel ? '>' : ' ';
        buf[1] = ' ';

        /* 复制菜单文本，最多 14 字符（因为前两格用来放光标）*/
        {
            uint8_t j;
            for (j = 0; j < 14 && menu->items[i].text[j] != '\0'; j++)
            {
                buf[2 + j] = menu->items[i].text[j];
            }
            buf[2 + j] = '\0';
        }

        OLED_ShowString(line, 1, buf);
    }
}

/* ============================================================
 * 选择移动
 * ============================================================ */
void OLED_Menu_Prev(OLED_Menu *menu)
{
    if (menu->count == 0) return;

    uint8_t oldSel = menu->selection;

    /* 循环上移 */
    if (menu->selection == 0)
        menu->selection = menu->count - 1;
    else
        menu->selection--;

    OLED_Menu_AdjustView(menu, oldSel);
    OLED_Menu_Draw(menu);
}

void OLED_Menu_Next(OLED_Menu *menu)
{
    if (menu->count == 0) return;

    uint8_t oldSel = menu->selection;

    /* 循环下移 */
    if (menu->selection >= menu->count - 1)
        menu->selection = 0;
    else
        menu->selection++;

    OLED_Menu_AdjustView(menu, oldSel);
    OLED_Menu_Draw(menu);
}

/* ============================================================
 * Enter —— 确认 / 进入子菜单
 * ============================================================ */
uint8_t OLED_Menu_Enter(OLED_Menu *menu)
{
    if (menu->count == 0) return 0;

    const OLED_MenuItem *item = &menu->items[menu->selection];

    /* 有子菜单 → 进入子菜单 */
    if (item->submenu != NULL && item->submenuCount > 0)
    {
        if (menu->historyDepth < OLED_MENU_MAX_DEPTH)
        {
            menu->history[menu->historyDepth].items     = menu->items;
            menu->history[menu->historyDepth].count     = menu->count;
            menu->history[menu->historyDepth].selection = menu->selection;
            menu->historyDepth++;
        }

        menu->items     = item->submenu;
        menu->count     = item->submenuCount;
        menu->selection = 0;
        menu->viewTop   = 0;
        OLED_Menu_Draw(menu);
        return OLED_MENU_ENTER_SUBMENU;
    }

    /* 叶子节点 → 执行回调（若有）并返回选中索引 */
    if (item->action != NULL)
        item->action();

    return menu->selection;
}

/* ============================================================
 * Back —— 返回上级菜单
 * ============================================================ */
uint8_t OLED_Menu_Back(OLED_Menu *menu)
{
    if (menu->historyDepth == 0)
        return 0;  /* 已在根菜单 */

    menu->historyDepth--;
    menu->items     = menu->history[menu->historyDepth].items;
    menu->count     = menu->history[menu->historyDepth].count;
    menu->selection = menu->history[menu->historyDepth].selection;
    menu->viewTop   = 0;
    OLED_Menu_Draw(menu);
    return 1;
}

/* ============================================================
 * 辅助查询
 * ============================================================ */
uint8_t OLED_Menu_GetSelection(const OLED_Menu *menu)
{
    return menu->selection;
}

uint8_t OLED_Menu_IsRoot(const OLED_Menu *menu)
{
    return (menu->historyDepth == 0) ? 1 : 0;
}
