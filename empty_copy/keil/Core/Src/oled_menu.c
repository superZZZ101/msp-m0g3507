#include "oled_menu.h"
#include "oled_spi.h"

/* ============================================================
 * 页式翻页
 * ============================================================ */
static uint8_t OLED_Menu_LastPage(const OLED_Menu *menu) {
    uint8_t ps = menu->visibleLines;
    if (menu->count == 0) return 0;
    return ((menu->count - 1) / ps) * ps;
}

static void OLED_Menu_AdjustView(OLED_Menu *menu, uint8_t oldSel) {
    if (menu->count <= menu->visibleLines) {
        menu->viewTop = 0;
        return;
    }

    uint8_t ps = menu->visibleLines;

    if (oldSel == menu->count - 1 && menu->selection == 0)
        menu->viewTop = 0;
    else if (oldSel == 0 && menu->selection == menu->count - 1)
        menu->viewTop = OLED_Menu_LastPage(menu);
    else if (menu->selection >= menu->viewTop + ps)
        menu->viewTop += ps;
    else if (menu->selection < menu->viewTop)
        menu->viewTop -= ps;
}

/* ============================================================
 * 初始化
 * ============================================================ */
void OLED_Menu_Init(OLED_Menu *menu, uint16_t textColor, uint16_t selColor) {
    menu->items        = NULL;
    menu->count        = 0;
    menu->selection    = 0;
    menu->textColor    = textColor;
    menu->selColor     = selColor;
    menu->startLine    = 1;
    menu->visibleLines = OLED_Height / 16;
    menu->viewTop      = 0;
    menu->historyDepth = 0;
}

void OLED_Menu_SetRoot(OLED_Menu *menu, const OLED_MenuItem *items, uint8_t count) {
    menu->items     = items;
    menu->count     = count;
    menu->selection = 0;
    menu->viewTop   = 0;
    menu->historyDepth = 0;
}

/* ============================================================
 * SetOrientation
 * ============================================================ */
void OLED_Menu_SetOrientation(OLED_Menu *menu, uint8_t mode) {
    OLED_SetOrientation(mode);
    menu->visibleLines = OLED_Height / 16;
    OLED_Menu_Draw(menu);
}

/* ============================================================
 * Draw — 全量刷新可见窗口
 * ============================================================ */
void OLED_Menu_Draw(const OLED_Menu *menu) {
    if (menu->items == NULL) return;

    OLED_Clear();

    uint8_t end = menu->viewTop + menu->visibleLines;
    if (end > menu->count) end = menu->count;

    for (uint8_t i = menu->viewTop; i < end; i++) {
        OLED_MenuRow(menu->startLine + (i - menu->viewTop),
                     menu->items[i].text,
                     menu->textColor,
                     menu->selColor,
                     i == menu->selection);
    }
}

/* ============================================================
 * 选择移动
 * ============================================================ */
static void OLED_Menu_Move(OLED_Menu *menu, uint8_t newSel) {
    if (menu->count == 0) return;

    uint8_t old = menu->selection;
    uint8_t oldTop = menu->viewTop;
    menu->selection = newSel;
    OLED_Menu_AdjustView(menu, old);

    if (menu->viewTop != oldTop) {
        OLED_Menu_Draw(menu);
    } else {
        OLED_MenuRow(menu->startLine + (old - menu->viewTop),
                     menu->items[old].text,
                     menu->textColor,
                     menu->selColor, 0);
        OLED_MenuRow(menu->startLine + (menu->selection - menu->viewTop),
                     menu->items[menu->selection].text,
                     menu->textColor,
                     menu->selColor, 1);
    }
}

void OLED_Menu_Next(OLED_Menu *menu) {
    if (menu->count == 0) return;
    uint8_t next = (menu->selection >= menu->count - 1) ? 0 : (menu->selection + 1);
    OLED_Menu_Move(menu, next);
}

void OLED_Menu_Prev(OLED_Menu *menu) {
    if (menu->count == 0) return;
    uint8_t prev = (menu->selection == 0) ? (menu->count - 1) : (menu->selection - 1);
    OLED_Menu_Move(menu, prev);
}

/* ============================================================
 * Enter — 确认 / 进入子菜单
 * ============================================================ */
uint8_t OLED_Menu_Enter(OLED_Menu *menu) {
    if (menu->count == 0) return 0;

    const OLED_MenuItem *item = &menu->items[menu->selection];

    if (item->submenu != NULL && item->submenuCount > 0) {
        // 记住当前菜单
        if (menu->historyDepth < OLED_MENU_MAX_DEPTH) {
            menu->history[menu->historyDepth].items     = menu->items;
            menu->history[menu->historyDepth].count     = menu->count;
            menu->history[menu->historyDepth].selection = menu->selection;
            menu->historyDepth++;
        }
        // 切换到子菜单
        menu->items     = item->submenu;
        menu->count     = item->submenuCount;
        menu->selection = 0;
        menu->viewTop   = 0;
        OLED_Menu_Draw(menu);
        return OLED_MENU_ENTER_SUBMENU;
    }

    // 叶子节点有回调 → 自动执行
    if (item->action != NULL)
        item->action();

    return menu->selection;
}

/* ============================================================
 * Back — 返回上级菜单
 * ============================================================ */
uint8_t OLED_Menu_Back(OLED_Menu *menu) {
    if (menu->historyDepth == 0) return 0;

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
uint8_t OLED_Menu_GetSelection(const OLED_Menu *menu) {
    return menu->selection;
}

uint8_t OLED_Menu_IsRoot(const OLED_Menu *menu) {
    return (menu->historyDepth == 0) ? 1 : 0;
}
