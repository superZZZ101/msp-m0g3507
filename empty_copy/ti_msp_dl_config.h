/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
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

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     32000000



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMA0)
#define TIMER_0_INST_IRQHandler                                 TIMA0_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMA0_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                             (0U)




/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOB)

/* Defines for LIGHT: GPIOB.22 with pinCMx 50 on package pin 21 */
#define LED_LIGHT_PIN                                           (DL_GPIO_PIN_22)
#define LED_LIGHT_IOMUX                                          (IOMUX_PINCM50)
/* Defines for SCL: GPIOA.0 with pinCMx 1 on package pin 33 */
#define OLED_SPI_SCL_PORT                                                (GPIOA)
#define OLED_SPI_SCL_PIN                                         (DL_GPIO_PIN_0)
#define OLED_SPI_SCL_IOMUX                                        (IOMUX_PINCM1)
/* Defines for SDA: GPIOA.1 with pinCMx 2 on package pin 34 */
#define OLED_SPI_SDA_PORT                                                (GPIOA)
#define OLED_SPI_SDA_PIN                                         (DL_GPIO_PIN_1)
#define OLED_SPI_SDA_IOMUX                                        (IOMUX_PINCM2)
/* Defines for RES: GPIOA.8 with pinCMx 19 on package pin 54 */
#define OLED_SPI_RES_PORT                                                (GPIOA)
#define OLED_SPI_RES_PIN                                         (DL_GPIO_PIN_8)
#define OLED_SPI_RES_IOMUX                                       (IOMUX_PINCM19)
/* Defines for DC: GPIOA.9 with pinCMx 20 on package pin 55 */
#define OLED_SPI_DC_PORT                                                 (GPIOA)
#define OLED_SPI_DC_PIN                                          (DL_GPIO_PIN_9)
#define OLED_SPI_DC_IOMUX                                        (IOMUX_PINCM20)
/* Defines for CS: GPIOB.15 with pinCMx 32 on package pin 3 */
#define OLED_SPI_CS_PORT                                                 (GPIOB)
#define OLED_SPI_CS_PIN                                         (DL_GPIO_PIN_15)
#define OLED_SPI_CS_IOMUX                                        (IOMUX_PINCM32)
/* Defines for BLK: GPIOB.16 with pinCMx 33 on package pin 4 */
#define OLED_SPI_BLK_PORT                                                (GPIOB)
#define OLED_SPI_BLK_PIN                                        (DL_GPIO_PIN_16)
#define OLED_SPI_BLK_IOMUX                                       (IOMUX_PINCM33)
/* Defines for UP: GPIOB.27 with pinCMx 58 on package pin 29 */
#define KEY_UP_PORT                                                      (GPIOB)
#define KEY_UP_PIN                                              (DL_GPIO_PIN_27)
#define KEY_UP_IOMUX                                             (IOMUX_PINCM58)
/* Defines for DOWN: GPIOB.23 with pinCMx 51 on package pin 22 */
#define KEY_DOWN_PORT                                                    (GPIOB)
#define KEY_DOWN_PIN                                            (DL_GPIO_PIN_23)
#define KEY_DOWN_IOMUX                                           (IOMUX_PINCM51)
/* Defines for ENTER: GPIOA.12 with pinCMx 34 on package pin 5 */
#define KEY_ENTER_PORT                                                   (GPIOA)
#define KEY_ENTER_PIN                                           (DL_GPIO_PIN_12)
#define KEY_ENTER_IOMUX                                          (IOMUX_PINCM34)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_TIMER_0_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
