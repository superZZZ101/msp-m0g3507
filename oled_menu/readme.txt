/**
 * empty.c ??MSPM0G3507 ?????? + ??????
 */

#include "w25q64.h"
#include "font_receiver.h"
#include "ti_msp_dl_config.h"
#include "led.h"
#include "delay.h"
#include "uart.h"
int main(void)
{
    SYSCFG_DL_init();
    W25Q64_Init();

    uart_putc(0x55);  // 告诉 PC：我准备好了

    FontReceiver_Run(0);

    while (1)
    {
        if (!DL_UART_isRXFIFOEmpty(UART0_INST))
        {
            uint8_t cmd = DL_UART_receiveData(UART0_INST);
            if (cmd == 'V')
                FontReceiver_VerifyFlash();
        }
    }
}
