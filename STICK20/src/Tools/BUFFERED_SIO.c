/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 09.04.2011
*
* This file is part of GPF Crypto Stick 2
*
* GPF Crypto Stick 2  is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* GPF Crypto Stick is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GPF Crypto Stick. If not, see <http://www.gnu.org/licenses/>.
*/


/*
 * Inbetriebnahme.c
 *
 *  Created on: 09.04.2011
 *      Author: RB
 */

#include <stdio.h>
#include "global.h"
#include "tools.h"

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
#include "..\CCID\USART\ISO7816_USART.h"

#include "print_funcs.h"
#include "intc.h"
#include "usart.h"
#include "BUFFERED_SIO.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

u8 BUFFERED_SIO_TxBuffer[BUFFERED_SIO_TX_BUFFER_SIZE];
u8 BUFFERED_SIO_RxBuffer[BUFFERED_SIO_RX_BUFFER_SIZE];

u16 BUFFERED_SIO_TxBuffer_StartPointer = 0;
u16 BUFFERED_SIO_TxBuffer_EndPointer   = 0;
u16 BUFFERED_SIO_TxActiv_Sendbytes     = 0;

u16 BUFFERED_SIO_RxBuffer_StartPointer = 0;
u16 BUFFERED_SIO_RxBuffer_EndPointer   = 0;

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
  u16 BUFFERED_SIO_HID_TxBuffer_StartPointer = 0;
  u16 BUFFERED_SIO_HID_TxBuffer_EndPointer   = 0;
  u16 BUFFERED_SIO_HID_TxActiv_Sendbytes     = 0;
#endif

/*******************************************************************************

  BUFFERED_SIO_WriteString

  Write string to sendbuffer

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 BUFFERED_SIO_WriteString (u16 Len,u8 *String_pu8)
{
    u16 Value;

    // Save end pointer 
    Value = BUFFERED_SIO_TxBuffer_EndPointer;

    // For each byte
    while (0 < Len)
    {
        Value++;

        // End of buffer ?
        if (BUFFERED_SIO_TX_BUFFER_SIZE <= Value)
        {
            // Set pointer to start of buffer
            Value = 0;
        }

        // Buffer full ?
        if (Value == BUFFERED_SIO_TxBuffer_StartPointer)
        {
            // Abort sending
            return (FALSE);
        }

        // Write byte into buffer
        BUFFERED_SIO_TxBuffer_EndPointer = Value;

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
        BUFFERED_SIO_HID_TxBuffer_EndPointer = Value;
#endif

        BUFFERED_SIO_TxBuffer[Value]  = *String_pu8;
// Todo ISR Lock start
        // Pointers to next byte
        String_pu8++;
        Len--;
// Todo ISR Lock end
    }

    return (TRUE);
}

/*******************************************************************************

  BUFFERED_SIO_SendHandler

  In ISR: Send 1 char to SIO

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void BUFFERED_SIO_SendHandler(void)
{
    // Something to send ?
    if (BUFFERED_SIO_TxBuffer_StartPointer == BUFFERED_SIO_TxBuffer_EndPointer)
    {
        return;
    }

    BUFFERED_SIO_TxBuffer_StartPointer++;

    // Rollover ?
    if (BUFFERED_SIO_TX_BUFFER_SIZE <= BUFFERED_SIO_TxBuffer_StartPointer)
    {
    	BUFFERED_SIO_TxBuffer_StartPointer = 0;
    }

    // Send char
    usart_putchar (BUFFERED_SIO_USART,BUFFERED_SIO_TxBuffer[BUFFERED_SIO_TxBuffer_StartPointer]);


	// Enable USART Tx interrupt for transmitting all chars
	Disable_global_interrupt();
	BUFFERED_SIO_USART->IER.txempty = 1;
	Enable_global_interrupt();
}

/*******************************************************************************

  BUFFERED_SIO_TxEmpty

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 BUFFERED_SIO_TxEmpty (void)
{
    if (BUFFERED_SIO_TxBuffer_StartPointer == BUFFERED_SIO_TxBuffer_EndPointer)
    {
        return (TRUE);
    }
    return (FALSE);
}

/*******************************************************************************

  BUFFERED_SIO_RxIntHandler

  In ISR: Write recieved char to buffer
  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void BUFFERED_SIO_RxIntHandler (s16 data_s16)
{
    u16 Value;

    Value = BUFFERED_SIO_RxBuffer_EndPointer + 1;

    if (BUFFERED_SIO_RX_BUFFER_SIZE <= Value)
    {
        Value = 0;
    }

    if (Value == BUFFERED_SIO_RxBuffer_StartPointer)
    {
        return;
    }

    BUFFERED_SIO_RxBuffer[Value] = (u8)data_s16;

   BUFFERED_SIO_RxBuffer_EndPointer = Value;
}

/*******************************************************************************

  BUFFERED_SIO_TxIntHandler

  In ISR: Check for tx data and set SIO flags

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void BUFFERED_SIO_TxIntHandler (void)
{
	if (FALSE == BUFFERED_SIO_TxEmpty ())
	{
		BUFFERED_SIO_SendHandler ();
	}
	else
	{
		// Disable USART Tx interrupt.
		Disable_global_interrupt();
		BUFFERED_SIO_USART->IDR.txempty = 1;
		Enable_global_interrupt();
	}
}

/*******************************************************************************

  BUFFERED_SIO_GetByte

  Get recevied char from SIO inputbuffer (Non ISR)

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 BUFFERED_SIO_GetByte (u8 *Data_pu8)
{
    if (BUFFERED_SIO_RxBuffer_StartPointer == BUFFERED_SIO_RxBuffer_EndPointer)
    {
        return (FALSE);
    }

    if (BUFFERED_SIO_RX_BUFFER_SIZE >= BUFFERED_SIO_RxBuffer_StartPointer+1)
    {
      *Data_pu8 = BUFFERED_SIO_RxBuffer[BUFFERED_SIO_RxBuffer_StartPointer+1];
    }
    else
    {
      *Data_pu8 = BUFFERED_SIO_RxBuffer[0];
    }

    BUFFERED_SIO_RxBuffer_StartPointer++;

    if (BUFFERED_SIO_RX_BUFFER_SIZE < BUFFERED_SIO_RxBuffer_StartPointer)
    {
        BUFFERED_SIO_RxBuffer_StartPointer = 0;
    }

    return (TRUE);
}

/*******************************************************************************

  BUFFERED_SIO_ByteReceived

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 BUFFERED_SIO_ByteReceived (void)
{
    if (BUFFERED_SIO_RxBuffer_StartPointer == BUFFERED_SIO_RxBuffer_EndPointer)
    {
        return (FALSE);
    }
    return (TRUE);
}

/*******************************************************************************

  BUFFERED_SIO_Usart_ISR

  ISR for receiving and sending chars

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#if (defined __GNUC__)
__attribute__((__interrupt__))
#elif (defined __ICCAVR32__)
__interrupt
#endif
void BUFFERED_SIO_Usart_ISR (void)
{
	static int nRxBytes  = 0;
	static int nTxBytes  = 0;
	static int nIntCalls = 0;
	static int nError    = 0;
	int c;
	int nRet;

	// clearing interrupt request
	c = BUFFERED_SIO_USART->cr;
	BUFFERED_SIO_USART->cr = c;
	c = BUFFERED_SIO_USART->cr;

	// Test for rx int
	if (1 == BUFFERED_SIO_USART->CSR.rxrdy)
	{
		// Get Data
		nRet = usart_read_char(BUFFERED_SIO_USART, &c);

		if (USART_RX_ERROR == nRet)
		{
			usart_reset_status (BUFFERED_SIO_USART);
			nError++;
		}
		else
		{
			BUFFERED_SIO_RxIntHandler (c);
			nRxBytes++;
		}
	}

	// Test for tx empty int
	if (1 == BUFFERED_SIO_USART->CSR.txempty)
	{
		BUFFERED_SIO_TxIntHandler ();
		nTxBytes++;

	}

	nIntCalls++;
}

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID

/*******************************************************************************

  BUFFERED_SIO_HID_TxEmpty

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 BUFFERED_SIO_HID_TxEmpty (void)
{
    if (BUFFERED_SIO_HID_TxBuffer_StartPointer == BUFFERED_SIO_HID_TxBuffer_EndPointer)
    {
        return (TRUE);
    }
    return (FALSE);
}


/*******************************************************************************

  BUFFERED_SIO_HID_GetSendChars

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 BUFFERED_SIO_HID_GetSendChars (u8 *Data_pu8,u8 MaxSendChars_u8)
{
  u8 SendChars_u8;

  SendChars_u8 = 0;
  while (SendChars_u8 < MaxSendChars_u8)
  {
      // Something to send ?
      if (TRUE == BUFFERED_SIO_HID_TxEmpty ())
      {
        break;  // No, stop
      }

      // Pointer to next byte
      BUFFERED_SIO_HID_TxBuffer_StartPointer++;

      // Rollover ?
      if (BUFFERED_SIO_TX_BUFFER_SIZE <= BUFFERED_SIO_HID_TxBuffer_StartPointer)
      {
        BUFFERED_SIO_HID_TxBuffer_StartPointer = 0;
      }

      // Copy data
      Data_pu8[SendChars_u8] = BUFFERED_SIO_TxBuffer[BUFFERED_SIO_HID_TxBuffer_StartPointer];

      SendChars_u8++;
  }

  return (SendChars_u8);
}
#endif

/*******************************************************************************

  BUFFERED_SIO_Init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u16 BUFFERED_SIO_Init (void)
{
// IO mapping
    static const gpio_map_t USART_GPIO_MAP =
    {
      {BUFFERED_SIO_USART_RX_PIN, BUFFERED_SIO_USART_RX_FUNCTION},
      {BUFFERED_SIO_USART_TX_PIN, BUFFERED_SIO_USART_TX_FUNCTION}
    };

// USART options.
    static const usart_options_t USART_OPTIONS =
    {
      .baudrate     = 57600,
      .charlength   = 8,
      .paritytype   = USART_NO_PARITY,
      .stopbits     = USART_1_STOPBIT,
      .channelmode  = USART_NORMAL_CHMODE
    };

// Reset vars
    BUFFERED_SIO_TxBuffer_StartPointer = 0;
    BUFFERED_SIO_TxBuffer_EndPointer   = 0;
    BUFFERED_SIO_TxActiv_Sendbytes     = 0;

    BUFFERED_SIO_RxBuffer_StartPointer = 0;
    BUFFERED_SIO_RxBuffer_EndPointer   = 0;

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
    BUFFERED_SIO_HID_TxBuffer_StartPointer = 0;
    BUFFERED_SIO_HID_TxBuffer_EndPointer   = 0;
    BUFFERED_SIO_HID_TxActiv_Sendbytes     = 0;
#endif

// Assign GPIO to USART.
    gpio_enable_module(USART_GPIO_MAP, sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

// Initialize USART in RS232 mode.
    usart_init_rs232(BUFFERED_SIO_USART, &USART_OPTIONS, FPBA_HZ);

// Disable all interrupts.
    Disable_global_interrupt();

// Set ISR
    INTC_register_interrupt(&BUFFERED_SIO_Usart_ISR, BUFFERED_SIO_USART_IRQ, AVR32_INTC_INT0);

// Enable USART Rx interrupt.
    BUFFERED_SIO_USART->IER.rxrdy   = 1;

// Enable USART Tx interrupt.
//	BUFFERED_SIO_USART->IER.txempty   = 1;

// Enable all interrupts.
    Enable_global_interrupt();

    return ( TRUE );
}

