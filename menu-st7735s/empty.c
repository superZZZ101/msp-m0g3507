/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "oled_spi.h"
#include "delay.h"
#include "key.h"
#include "oled_menu.h"
//声明
static const OLED_MenuItem mainItems[];
void PWM_Freq(void);
void PWM_Duty(void);
void PID_Kp(void);
void PID_Ki(void);
void PID_Kd(void);
// ── 先定义子菜单 ──
static const OLED_MenuItem pwmItems[] = {
    {"Set Freq", NULL, 0},
    {"Set Duty", NULL, 0},
    {"< Back",   mainItems, 3},
};

static const OLED_MenuItem pidItems[] = {
    {"Kp",   NULL, 0,PID_Kp},
    {"Ki",   NULL,0, PID_Ki},
    {"Kd",   NULL, 0,PID_Kd},
    {"< Back",   mainItems, 3},
};

// ── 再定义主菜单──
static const OLED_MenuItem mainItems[] = {
    {"PWM",     pwmItems, OLED_MENU_COUNT(pwmItems)},
    {"PID",     pidItems, OLED_MENU_COUNT(pidItems)},
    {"ENCODER", NULL, 0},
};


OLED_Menu menu;

int main() {
	uint8_t count=0,flag=0;
    // ... 初始化 OLED ...
	SYSCFG_DL_init();
	OLED_Init();
	OLED_SetOrientation(OLED_ORIENTATION_LANDSCAPE);
	OLED_Menu_Init(&menu, COLOR_GREEN, COLOR_BLUE);
	OLED_Menu_SetRoot(&menu, mainItems, OLED_MENU_COUNT(mainItems));
	OLED_Menu_Draw(&menu);
//DL_GPIO_setPins(LED_PORT,LED_LIGHT_PIN);
	while (1) {
		KeyEvent key = KEY_Scan();
	
		switch (key) {
				case KEY_UP:    OLED_Menu_Prev(&menu); break;
				case KEY_DOWN:  OLED_Menu_Next(&menu); break;
				case KEY_ENTER: OLED_Menu_Enter(&menu); break;
				default: break;
		}

	}
	return 0;
	}
void PID_Kp(void){
	DL_GPIO_setPins(LED_PORT,LED_LIGHT_PIN);
}
void PID_Ki(void){
	DL_GPIO_clearPins(LED_PORT,LED_LIGHT_PIN);
}
void PID_Kd(void){}