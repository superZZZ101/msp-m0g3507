#ifndef __UART_H__
#define __UART_H__
#include "ti_msp_dl_config.h"
#include "stdint.h"
uint8_t uart_getc(void);
void uart_putc(uint8_t c);
#endif