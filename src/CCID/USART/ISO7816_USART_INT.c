/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 16.06.2010
 *
 * This file is part of Nitrokey
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

/*
 * USART.c
 *
 *  Created on: 16.06.2010
 *      Author: RB
 */

#include <avr32/io.h>
#include "preprocessor.h"
#include "compiler.h"
#include "board.h"
#include "power_clocks_lib.h"
#include "gpio.h"
#include "usart.h"
#include "tools.h"
#include "FreeRTOS.h"
#include "task.h"

#include "print_funcs.h"
#include "intc.h"

#include "usart.h"
#include "ISO7816_USART.h"
#include "ISO7816_USART_INT.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

/* UART Int test */

#define ISO7816_USART_INT   // Use the smartcard interface

#ifdef ISO7816_USART_INT
    // Use the smartcard interface
#define EXAMPLE_INT_USART                 ISO7816_USART
#define EXAMPLE_INT_USART_RX_PIN          ISO7816_USART_RX_PIN
#define EXAMPLE_INT_USART_RX_FUNCTION     ISO7816_USART_RX_FUNCTION
#define EXAMPLE_INT_USART_TX_PIN          ISO7816_USART_TX_PIN
#define EXAMPLE_INT_USART_TX_FUNCTION     ISO7816_USART_TX_FUNCTION
#define EXAMPLE_INT_USART_IRQ             AVR32_USART1_IRQ
#define EXAMPLE_INT_USART_BAUDRATE        57600
#define EXAMPLE_INT_TARGET_PBACLK_FREQ_HZ FOSC0 // PBA clock target frequency, in Hz
#else
    // Used the vitual sio of EVK1104
#define EXAMPLE_INT_USART                 (&AVR32_USART1)
#define EXAMPLE_INT_USART_RX_PIN          AVR32_USART1_RXD_0_0_PIN
#define EXAMPLE_INT_USART_RX_FUNCTION     AVR32_USART1_RXD_0_0_FUNCTION
#define EXAMPLE_INT_USART_TX_PIN          AVR32_USART1_TXD_0_0_PIN
#define EXAMPLE_INT_USART_TX_FUNCTION     AVR32_USART1_TXD_0_0_FUNCTION
#define EXAMPLE_INT_USART_IRQ             AVR32_USART1_IRQ
#define EXAMPLE_INT_USART_BAUDRATE        57600
#define EXAMPLE_INT_TARGET_PBACLK_FREQ_HZ FOSC0 // PBA clock target frequency, in Hz
#endif


typedef enum
{
    ISO7816_USART_INT_STATUS_OK = 0,
    ISO7816_USART_INT_STATUS_OVERFLOW
} ISO7816_USART_INT_Status;


#define ISO7816_RX_BUFFER_SIZE    30

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/
static ISO7816_USART_INT_Status ISO7816_UsartRxBufferStatus_e = ISO7816_USART_INT_STATUS_OK;

static u8 ISO7816_UsartRxBuffer_u8[ISO7816_RX_BUFFER_SIZE];
static u16 ISO7816_UsartRxBufferPointerStart_u16 = 0;
static u16 ISO7816_UsartRxBufferPointerEnd_u16 = 0;


/*******************************************************************************

  ISO7816_Usart_ISR

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#if (defined __GNUC__)
__attribute__ ((__interrupt__))
#elif (defined __ICCAVR32__)
__interrupt
#endif
static void ISO7816_Usart_ISR (void)
{
    static int n = 0;
    static int nError = 0;
    int c;
    int nRet;
    u16 SaveBufferPointer_u16;

    // Test for rx int
    if (1 == EXAMPLE_INT_USART->CSR.rxrdy)
    {
        // Get Data
        nRet = usart_read_char (EXAMPLE_INT_USART, &c);

        if (USART_RX_ERROR == nRet)
        {
            usart_reset_tx (ISO7816_USART); // Reset all
            usart_reset_rx (ISO7816_USART);
            nError++;
            return;
        }
        // Save old end pointer
        SaveBufferPointer_u16 = ISO7816_UsartRxBufferPointerEnd_u16;

        // Set next buffer position
        SaveBufferPointer_u16++;

        // End of buffer reached ?
        if (ISO7816_RX_BUFFER_SIZE <= SaveBufferPointer_u16)
        {
            // Yes
            SaveBufferPointer_u16 = 0;
        }

        // Buffer full ?
        if (ISO7816_UsartRxBufferPointerEnd_u16 == SaveBufferPointer_u16)
        {
            // Buffer full
            ISO7816_UsartRxBufferStatus_e = ISO7816_USART_INT_STATUS_OVERFLOW;
        }
        else
        {
            // Store char
            ISO7816_UsartRxBuffer_u8[ISO7816_UsartRxBufferPointerEnd_u16] = c;
            // Set external var to new buffer end
            ISO7816_UsartRxBufferPointerEnd_u16 = SaveBufferPointer_u16;
        }
    }

    n++;
}

/*******************************************************************************

  ISO7816_ReadCharISR

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

u32 ISO7816_ReadCharISR (u32 * nChar_pu32, u32 nTimeOutMs_u32)
{
u32 i;
u16 SaveBufferPointer_u16;

    ToolPinSet ();

    for (i = 0; i < nTimeOutMs_u32; i++)
    {
        // Something received ?
        if (ISO7816_UsartRxBufferPointerEnd_u16 != ISO7816_UsartRxBufferPointerStart_u16)
        {
            // Yes
            *nChar_pu32 = ISO7816_UsartRxBuffer_u8[ISO7816_UsartRxBufferPointerStart_u16];

            // Used save variable to avoid problems, (same as task lock)
            SaveBufferPointer_u16 = ISO7816_UsartRxBufferPointerStart_u16;

            // Set next buffer position
            SaveBufferPointer_u16++;

            // End of buffer reached ?
            if (ISO7816_RX_BUFFER_SIZE <= SaveBufferPointer_u16)
            {
                // Yes
                SaveBufferPointer_u16 = 0;
            }

            ISO7816_UsartRxBufferPointerStart_u16 = SaveBufferPointer_u16;
            ToolPinClr ();
            return (USART_SUCCESS);
        }
        // Wait 1 ms
        vTaskDelay (CCID_TASK_DELAY_1_MS_IN_TICKS);
    }
    ToolPinClr ();

    return (ISO7816_USART_TIMEOUT);
}

/*******************************************************************************

  ISO7816_SendCharISR

  The tx interrupt has a problem, so the chars are send directly

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

u32 ISO7816_SendCharISR (u32 nChar_u32, u32 nTimeOutMs_u32)
{
int i;

    ToolPinSet ();

    for (i = 0; i < nTimeOutMs_u32; i++)
    {
        // Tx free ?
        if (TRUE == usart_tx_ready (EXAMPLE_INT_USART))
        {
            // Tx transmitter buffer ?
            // if (TRUE == usart_txTransmitter_ready(EXAMPLE_INT_USART))
            {
                // Yes, send char
                usart_write_char (EXAMPLE_INT_USART, nChar_u32);
                ToolPinClr ();
                return USART_SUCCESS;
            }
        }

        // Wait 1 ms
        vTaskDelay (CCID_TASK_DELAY_1_MS_IN_TICKS);
    }

    ToolPinClr ();

    return (USART_TX_BUSY);
}

/*******************************************************************************

  ISO7816_Usart_ISR_InitRxBuffer

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_Usart_ISR_InitRxBuffer (void)
{
    ISO7816_UsartRxBufferStatus_e = ISO7816_USART_INT_STATUS_OK;
    ISO7816_UsartRxBufferPointerStart_u16 = 0;
    ISO7816_UsartRxBufferPointerEnd_u16 = 0;
}

/*******************************************************************************

  ISO7816_Usart_ISR_Init

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_Usart_ISR_Init (void)
{
    // Disable all interrupts.
    Disable_global_interrupt ();

    // Reset buffers
    ISO7816_Usart_ISR_InitRxBuffer ();

    // Initialize interrupt vectors.
    // INTC_init_interrupts();

    // Set ISR
    INTC_register_interrupt (&ISO7816_Usart_ISR, EXAMPLE_INT_USART_IRQ, AVR32_INTC_INT0);

    // Enable USART Rx interrupt.
    EXAMPLE_INT_USART->IER.rxrdy = 1;

    // Enable all interrupts.
    Enable_global_interrupt ();
}

/*******************************************************************************

  ISO7816_Usart_ISR_Test

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_Usart_ISR_Test (void)
{
    static int n = 0;
    static int c;
    // static int x;

    // Test for tx int
    // Tx int enabled ?
    if (1 == EXAMPLE_INT_USART->IMR.txempty)
    {
        // Yes
        // Data send ?
        if (1 == EXAMPLE_INT_USART->CSR.txempty)
        {
            // Data send
            EXAMPLE_INT_USART->IDR.txempty = 1;
            usart_reset_tx (EXAMPLE_INT_USART);

            /*
               // Disable tx empty interrupt Disable_global_interrupt(); EXAMPLE_INT_USART->IDR.txempty = 1; Enable_global_interrupt(); while (1 ==
               EXAMPLE_INT_USART->IMR.txempty) ; */
            /*
               // clearing interrupt request x = EXAMPLE_INT_USART->cr; EXAMPLE_INT_USART->cr = x; x = EXAMPLE_INT_USART->cr; */
        }
    }
    else
    {
        n++;
    }

    // Test for rx int
    if (1 == EXAMPLE_INT_USART->CSR.rxrdy)
    {
        // Data is ready
        usart_read_char (EXAMPLE_INT_USART, &c);
        // EXAMPLE_INT_USART->IER.txempty = 1;

        // Send data
        // usart_write_char(EXAMPLE_INT_USART, c);

        // Enable tx int
        Disable_global_interrupt ();
        EXAMPLE_INT_USART->IER.txempty = 1;
        Enable_global_interrupt ();
        while (0 == EXAMPLE_INT_USART->IMR.txempty);

    }

    /*
       usart_read_char(EXAMPLE_INT_USART, &c); usart_write_char(EXAMPLE_INT_USART, c); */

    n++;
}

/*******************************************************************************

  USART_Int_Test

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void USART_Int_Test (void)
{
    int i;

    static const gpio_map_t USART_GPIO_MAP = {
        {EXAMPLE_INT_USART_RX_PIN, EXAMPLE_INT_USART_RX_FUNCTION},
        {EXAMPLE_INT_USART_TX_PIN, EXAMPLE_INT_USART_TX_FUNCTION}
    };

    // USART options.
    static const usart_options_t USART_OPTIONS = {
        .baudrate = EXAMPLE_INT_USART_BAUDRATE,
        .charlength = 8,
        .paritytype = USART_NO_PARITY,
        .stopbits = USART_1_STOPBIT,
        .channelmode = USART_NORMAL_CHMODE
    };

    pcl_switch_to_osc (PCL_OSC0, FOSC0, OSC0_STARTUP);

    // Assign GPIO to USART.
    gpio_enable_module (USART_GPIO_MAP, sizeof (USART_GPIO_MAP) / sizeof (USART_GPIO_MAP[0]));

    // Initialize USART in RS232 mode.
    usart_init_rs232 (EXAMPLE_INT_USART, &USART_OPTIONS, FOSC0);    // EXAMPLE_TARGET_PBACLK_FREQ_HZ);

    // Disable all interrupts.
    Disable_global_interrupt ();

    // Initialize interrupt vectors.
    INTC_init_interrupts ();

    // Register the USART interrupt handler to the interrupt controller.
    // usart_int_handler is the interrupt handler to register.
    // EXAMPLE_USART_IRQ is the IRQ of the interrupt handler to register.
    // AVR32_INTC_INT0 is the interrupt priority level to assign to the group of
    // this IRQ.
    // void INTC_register_interrupt(__int_handler handler, unsigned int irq, unsigned int int_level);
    INTC_register_interrupt (&ISO7816_Usart_ISR, EXAMPLE_INT_USART_IRQ, AVR32_INTC_INT0);

    // Enable USART Rx interrupt.
    // EXAMPLE_INT_USART->ier = AVR32_USART_IER_RXRDY_MASK;
    EXAMPLE_INT_USART->IER.rxrdy = 1;
    // EXAMPLE_INT_USART->IER.txempty = 1;
    //

    // Enable all interrupts.
    Enable_global_interrupt ();

    // We have nothing left to do in the main, so we may switch to a device sleep
    // mode: we just need to be sure that the USART module will be still be active
    // in the chosen sleep mode. The sleep mode to use is the FROZEN sleep mode:
    // in this mode the PB clocks are still active (so the USART module which is
    // on the Peripheral Bus will still be active while the CPU and HSB will be
    // stopped).
    // --
    // Modules communicating with external circuits should normally be disabled
    // before entering a sleep mode that will stop the module operation: this is not
    // the case for the FROZEN sleep mode.
    // --
    // When the USART interrupt occurs, this will wake the CPU up which will then
    // execute the interrupt handler code then come back to the while(1) loop below
    // to execute the sleep instruction again.

    while (1)
    {
        for (i = 0; i < 100000; i++);

        usart_write_char (EXAMPLE_INT_USART, '*');
        EXAMPLE_INT_USART->IER.txempty = 1;
         /**/
            // If there is a chance that any PB write operations are incomplete, the CPU
            // should perform a read operation from any register on the PB bus before
            // executing the sleep instruction.
            // AVR32_INTC.ipr[0]; // Dummy read
            // Go to FROZEN sleep mode.
            // SLEEP(AVR32_PM_SMODE_FROZEN);
            // When the device wakes up due to an interrupt, once the interrupt is serviced,
            // go back into FROZEN sleep mode.
    }
}
