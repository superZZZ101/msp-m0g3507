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
#include "OLED.h"
#include "delay.h"
#include "Encode.h"
#include "PWM.h"
#include "bsp_IR_io.h" 
int main(void)
{
	int i=0,j,x=0,y=0;
	 uint16_t comp,k=0 ;
  
	SYSCFG_DL_init();
	NVIC_EnableIRQ(encode_INT_IRQN);//开启按键引脚的GPIOB端口中断
	OLED_Init();
	PWM_init();
  OLED_ShowNum(1,1,comp,5);  // 看是不是0
	OLED_ShowString(1,1,"MSPM0G3507");
	while (1) {
		x++;y++;
		delay(1);
		i=Encode_GetspeedL();
		j=Encode_GetspeedR();
//		OLED_ShowSignedNum(2,1,i,5);
//		OLED_ShowSignedNum(3,1,j,5);
		OLED_ShowIRSensors();
		PWM_Run(x,y);

//		DL_GPIO_setPins(LED_PORT,LED_LIGHT_PIN);
//		delay(500);
//		DL_GPIO_clearPins(LED_PORT, LED_LIGHT_PIN);
//		delay(500);

    }
}
//encode
void GROUP1_IRQHandler(void){
    switch( DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1) )
    {
        case DL_INTERRUPT_GROUP1_IIDX_GPIOA:
//            DL_GPIO_togglePins(LED_PORT,LED_LIGHT_PIN);
            encode_getcount();
        break;
    }
}
