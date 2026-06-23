#include "uart.h"

void uart_putc(uint8_t c)
{
    DL_UART_transmitDataBlocking(UART0_INST, c);
}

uint8_t uart_getc(void)
{
    return DL_UART_receiveDataBlocking(UART0_INST);
}