/* This header file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/* This file is prepared for Doxygen automatic documentation generation. */
/* ! \file ********************************************************************* \brief FreeRTOS demonstration for AVR32 UC3. - Compiler: IAR
   EWAVR32 and GNU GCC for AVR32 - Supported devices: All AVR32 devices can be used. - AppNote: \author Atmel Corporation: http://www.atmel.com \n
   Support and FAQ: http://support.atmel.no/ **************************************************************************** */

/* Copyright (c) 2009 Atmel Corporation. All rights reserved. Redistribution and use in source and binary forms, with or without modification, are
   permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimer. 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
   the following disclaimer in the documentation and/or other materials provided with the distribution. 3. The name of Atmel may not be used to
   endorse or promote products derived from this software without specific prior written permission. 4. This software may only be redistributed and
   used in connection with an Atmel AVR product. THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE EXPRESSLY AND SPECIFICALLY
   DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE */
/*
 * This file contains modifications done by Rudolf Boeddeker
 * For the modifications applies:
 *
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
 *
 * Nitrokey  is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "board.h"
#include "global.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define portREMOVE_STATIC_QUALIFIER              1
#define configGENERATE_RUN_TIME_STATS            1

#ifdef TIME_MEASURING_ENABLE
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS   TIME_MEASURING_Init_Timer
#define portGET_RUN_TIME_COUNTER_VALUE           TIME_MEASURING_GetTime
#else
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS   TIME_MEASURING_Void
#define portGET_RUN_TIME_COUNTER_VALUE           TIME_MEASURING_Void
#endif

#define INCLUDE_uxTaskGetStackHighWaterMark   	 1

#define configUSE_PREEMPTION      1
#define configUSE_IDLE_HOOK       0
#define configUSE_TICK_HOOK       1
// #define configCPU_CLOCK_HZ ( FOSC0 ) /* 12 MHz Hz clk gen */
#define configCPU_CLOCK_HZ        ( FMCK_HZ )   /* 60 MHz Hz clk gen */
#define configPBA_CLOCK_HZ        ( FOSC0 )
// #define configPBA_CLOCK_HZ ( FMCK_HZ )
#define configTICK_RATE_HZ        ( ( portTickType ) 2000)  // 200 )
#define configMAX_PRIORITIES      ( ( unsigned portBASE_TYPE ) 8 )
#define configMINIMAL_STACK_SIZE  ( ( unsigned portSHORT ) 256 )
/* configTOTAL_HEAP_SIZE is not used when heap_3.c is used. */
#define configTOTAL_HEAP_SIZE     ( ( size_t ) ( 1024*25 ) )
#define configMAX_TASK_NAME_LEN   ( 16 )
#define configUSE_TRACE_FACILITY  0
#define configUSE_16_BIT_TICKS    0
#define configIDLE_SHOULD_YIELD   1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES     0
#define configMAX_CO_ROUTINE_PRIORITIES ( 0 )

/* Set the following definitions to 1 to include the API function, or zero to exclude the API function. */

#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskCleanUpResources       0
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xTaskGetCurrentTaskHandle   0
#define INCLUDE_xTaskGetSchedulerState		1

/* configTICK_USE_TC is a boolean indicating whether to use a Timer Counter or the CPU Cycle Counter for the tick generation. Both methods will
   generate an accurate tick. 0: Use of the CPU Cycle Counter. 1: Use of the Timer Counter (configTICK_TC_CHANNEL is the TC channel). */

/*
   #define configTICK_USE_TC 0 #define configTICK_TC_CHANNEL 2 */
// Problems with debugging : timer int at step into...
// http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=104397&start=0
#define configTICK_USE_TC             1
#define configTICK_TC_CHANNEL         1



/* configHEAP_INIT is a boolean indicating whether to initialize the heap with 0xA5 in order to be able to determine the maximal heap consumption. */
#define configHEAP_INIT               1

/* Debug trace configuration. configDBG is a boolean indicating whether to activate the debug trace. */
#if BOARD == EVK1100
#define configDBG                     1
#define configDBG_USART               (&AVR32_USART1)
#define configDBG_USART_RX_PIN        AVR32_USART1_RXD_0_0_PIN
#define configDBG_USART_RX_FUNCTION   AVR32_USART1_RXD_0_0_FUNCTION
#define configDBG_USART_TX_PIN        AVR32_USART1_TXD_0_0_PIN
#define configDBG_USART_TX_FUNCTION   AVR32_USART1_TXD_0_0_FUNCTION
#define configDBG_USART_BAUDRATE      57600
#define serialPORT_USART              (&AVR32_USART1)
#define serialPORT_USART_RX_PIN       AVR32_USART1_RXD_0_0_PIN
#define serialPORT_USART_RX_FUNCTION  AVR32_USART1_RXD_0_0_FUNCTION
#define serialPORT_USART_TX_PIN       AVR32_USART1_TXD_0_0_PIN
#define serialPORT_USART_TX_FUNCTION  AVR32_USART1_TXD_0_0_FUNCTION
#define serialPORT_USART_IRQ          AVR32_USART1_IRQ
#define serialPORT_USART_BAUDRATE     57600
#elif BOARD == EVK1101
#define configDBG                     1
#define configDBG_USART               (&AVR32_USART1)
#define configDBG_USART_RX_PIN        AVR32_USART1_RXD_0_0_PIN
#define configDBG_USART_RX_FUNCTION   AVR32_USART1_RXD_0_0_FUNCTION
#define configDBG_USART_TX_PIN        AVR32_USART1_TXD_0_0_PIN
#define configDBG_USART_TX_FUNCTION   AVR32_USART1_TXD_0_0_FUNCTION
#define configDBG_USART_BAUDRATE      57600
#define serialPORT_USART              (&AVR32_USART1)
#define serialPORT_USART_RX_PIN       AVR32_USART1_RXD_0_0_PIN
#define serialPORT_USART_RX_FUNCTION  AVR32_USART1_RXD_0_0_FUNCTION
#define serialPORT_USART_TX_PIN       AVR32_USART1_TXD_0_0_PIN
#define serialPORT_USART_TX_FUNCTION  AVR32_USART1_TXD_0_0_FUNCTION
#define serialPORT_USART_IRQ          AVR32_USART1_IRQ
#define serialPORT_USART_BAUDRATE     57600
#elif BOARD == EVK1104
#define configDBG                     0
#define configDBG_USART               (&AVR32_USART1)
#define configDBG_USART_RX_PIN        AVR32_USART1_RXD_0_0_PIN
#define configDBG_USART_RX_FUNCTION   AVR32_USART1_RXD_0_0_FUNCTION
#define configDBG_USART_TX_PIN        AVR32_USART1_TXD_0_0_PIN
#define configDBG_USART_TX_FUNCTION   AVR32_USART1_TXD_0_0_FUNCTION
#define configDBG_USART_BAUDRATE      57600
#define serialPORT_USART              (&AVR32_USART1)
#define serialPORT_USART_RX_PIN       AVR32_USART1_RXD_0_0_PIN
#define serialPORT_USART_RX_FUNCTION  AVR32_USART1_RXD_0_0_FUNCTION
#define serialPORT_USART_TX_PIN       AVR32_USART1_TXD_0_0_PIN
#define serialPORT_USART_TX_FUNCTION  AVR32_USART1_TXD_0_0_FUNCTION
#define serialPORT_USART_IRQ          AVR32_USART1_IRQ
#define serialPORT_USART_BAUDRATE     57600
#elif BOARD == EVK1105
#define configDBG                     1
#define configDBG_USART               (&AVR32_USART0)
#define configDBG_USART_RX_PIN        AVR32_USART0_RXD_0_0_PIN
#define configDBG_USART_RX_FUNCTION   AVR32_USART0_RXD_0_0_FUNCTION
#define configDBG_USART_TX_PIN        AVR32_USART0_TXD_0_0_PIN
#define configDBG_USART_TX_FUNCTION   AVR32_USART0_TXD_0_0_FUNCTION
#define configDBG_USART_BAUDRATE      57600
#define serialPORT_USART              (&AVR32_USART0)
#define serialPORT_USART_RX_PIN       AVR32_USART0_RXD_0_0_PIN
#define serialPORT_USART_RX_FUNCTION  AVR32_USART0_RXD_0_0_FUNCTION
#define serialPORT_USART_TX_PIN       AVR32_USART0_TXD_0_0_PIN
#define serialPORT_USART_TX_FUNCTION  AVR32_USART0_TXD_0_0_FUNCTION
#define serialPORT_USART_IRQ          AVR32_USART0_IRQ
#define serialPORT_USART_BAUDRATE     57600
#elif BOARD == UC3C_EK
#define configDBG                     1
#define configDBG_USART               (&AVR32_USART2)
#define configDBG_USART_RX_PIN        AVR32_USART2_RXD_0_1_PIN
#define configDBG_USART_RX_FUNCTION   AVR32_USART2_RXD_0_1_FUNCTION
#define configDBG_USART_TX_PIN        AVR32_USART2_TXD_0_1_PIN
#define configDBG_USART_TX_FUNCTION   AVR32_USART2_TXD_0_1_FUNCTION
#define configDBG_USART_BAUDRATE      57600
#define serialPORT_USART              (&AVR32_USART2)
#define serialPORT_USART_RX_PIN       AVR32_USART2_RXD_0_1_PIN
#define serialPORT_USART_RX_FUNCTION  AVR32_USART2_RXD_0_1_FUNCTION
#define serialPORT_USART_TX_PIN       AVR32_USART2_TXD_0_1_PIN
#define serialPORT_USART_TX_FUNCTION  AVR32_USART2_TXD_0_1_FUNCTION
#define serialPORT_USART_IRQ          AVR32_USART2_IRQ
#define serialPORT_USART_BAUDRATE     57600
#else
#error The board USART configuration should be defined here.
#endif

// Systemtick now 2000 Hz

/* USB task definitions. */
#define configTSK_USB_NAME            ((const signed portCHAR *)"USB")
#define configTSK_USB_STACK_SIZE      256   // 1024 // 256
#define configTSK_USB_PRIORITY        (tskIDLE_PRIORITY + 4)

/* USB device task definitions. */
#define configTSK_USB_DEV_NAME        ((const signed portCHAR *)"USB Device")
#define configTSK_USB_DEV_STACK_SIZE  512   // 1024 // 256
#define configTSK_USB_DEV_PRIORITY    (tskIDLE_PRIORITY + 3)
#define configTSK_USB_DEV_PERIOD      1 // 20

/* USB host task definitions. */
#define configTSK_USB_HST_NAME        ((const signed portCHAR *)"USB Host")
#define configTSK_USB_HST_STACK_SIZE  256
#define configTSK_USB_HST_PRIORITY    (tskIDLE_PRIORITY + 3)
#define configTSK_USB_HST_PERIOD      20

/* USB device mass-storage task definitions. */
#define configTSK_USB_DMS_NAME        ((const signed portCHAR *)"USB Device MSD")
#define configTSK_USB_DMS_STACK_SIZE  1024  // 256
#define configTSK_USB_DMS_PRIORITY    (tskIDLE_PRIORITY + 2)
#define configTSK_USB_DMS_PERIOD      1 // 20

/* USB host mass-storage task definitions. */
#define configTSK_USB_HMS_NAME        ((const signed portCHAR *)"USB Host MSD")
#define configTSK_USB_HMS_STACK_SIZE  256
#define configTSK_USB_HMS_PRIORITY    (tskIDLE_PRIORITY + 2)
#define configTSK_USB_HMS_PERIOD      20

/* uShell task definitions. */
#define configTSK_USHELL_NAME         ((const signed portCHAR *)"uShell")
#define configTSK_USHELL_STACK_SIZE   768
#define configTSK_USHELL_PRIORITY     (tskIDLE_PRIORITY + 1)
#define configTSK_USHELL_PERIOD       50

/* USB device CCID definitions. */
#define configTSK_USB_CCID_NAME        ((const signed portCHAR *)"USB Device CCID")
#define configTSK_USB_CCID_STACK_SIZE  (1024*1)
#define configTSK_USB_CCID_PRIORITY    (tskIDLE_PRIORITY + 2)
#define configTSK_USB_CCID_PERIOD      1    // 20

/* USB device CCID definitions. */
#define configTSK_CCID_TEST_NAME        ((const signed portCHAR *)"CCID Test task")
#define configTSK_CCID_TEST_STACK_SIZE  (1024*2)
#define configTSK_CCID_TEST_PRIORITY    (tskIDLE_PRIORITY + 2)
#define configTSK_CCID_TEST_PERIOD      20

/* IDF definitions. */
#define configTSK_IDF_TEST_NAME        ((const signed portCHAR *)"IDF task")
#define configTSK_IDF_TEST_STACK_SIZE  (1024+512)
#define configTSK_IDF_TEST_PRIORITY    (tskIDLE_PRIORITY + 2)
#define configTSK_IDF_TEST_PERIOD      1    // 10

/* AD definitions. */
#define configTSK_AD_LOG_NAME        	((const signed portCHAR *)"AD task")
#define configTSK_AD_LOG_STACK_SIZE  	(1024)
#define configTSK_AD_LOG_PRIORITY    	(tskIDLE_PRIORITY + 1)
#define configTSK_AD_LOG_PERIOD      	100

/* IW definitions. Used for HID interface */
#define configTSK_IW_TEST_NAME        ((const signed portCHAR *)"IW task")
#define configTSK_IW_TEST_STACK_SIZE  (1024)
#define configTSK_IW_TEST_PRIORITY    (tskIDLE_PRIORITY + 2)
#define configTSK_IW_TEST_PERIOD      20    // 10

/* OTP definitions. */
#define configTSK_OTP_TEST_NAME        ((const signed portCHAR *)"OTP task")
#define configTSK_OTP_TEST_STACK_SIZE  (1024)
#define configTSK_OTP_TEST_PRIORITY    (tskIDLE_PRIORITY + 2)
#define configTSK_OTP_TEST_PERIOD      10


#endif /* FREERTOS_CONFIG_H */
