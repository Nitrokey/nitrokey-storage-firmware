/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 16.06.2010
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

/* Parts depend on the AVR framework */

/*
 * USART.c
 *
 *  Created on: 16.06.2010
 *      Author: RB
 */

#include <avr32/io.h>
#include "compiler.h"
#include "mass_storage_example.h"
#include "board.h"
#include "power_clocks_lib.h"
#include "gpio.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "task.h"

#include "global.h"
#include "tools.h"

#include "ISO7816_USART_INT.h"
#include "ISO7816_USART.h"


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

static const gpio_map_t ISO7816_USART_GPIO_MAP =
{
//  {ISO7816_USART_RX_PIN   , ISO7816_USART_RX_FUNCTION},
  {ISO7816_USART_TX_PIN     , ISO7816_USART_TX_FUNCTION},
  {ISO7816_USART_CLK_PIN    , ISO7816_USART_CLK_FUNCTION}
};

// USART options.
static const usart_options_t ISO7816_USART_OPTIONS =
{
  .baudrate     = 57600,
  .charlength   = 8,
  .paritytype   = USART_NO_PARITY,
  .stopbits     = USART_1_STOPBIT,
  .channelmode  = USART_NORMAL_CHMODE
};

/*******************************************************************************

  usart_set_stopbits access

*******************************************************************************/

//! Input parameters when initializing ISO7816 mode.
const usart_iso7816_options_t ISO7816_USART_ISO7816 =
{
  .iso7816_hz     = 4000000,
//  .iso7816_hz     = 3000000,
//  .iso7816_hz     = 8000000,
  .fidi_ratio     = 372, // 417
  .paritytype     = USART_EVEN_PARITY,
  .inhibit_nack   = 0,
  .dis_suc_nack   = 0,
  .max_iterations = 0,
  .bit_order    = 0
};

/*******************************************************************************

  Init_ISO7816_Usart

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int Init_ISO7816_Usart (void)
{
	int nRet;
	int nProtocol = 1;

// Assign GPIO to USART1
	nRet = gpio_enable_module(ISO7816_USART_GPIO_MAP,sizeof(ISO7816_USART_GPIO_MAP) / sizeof(ISO7816_USART_GPIO_MAP[0]));

// Init ISO 7816
	nRet = usart_init_iso7816(ISO7816_USART,&ISO7816_USART_ISO7816, nProtocol, CCID_TARGET_PBACLK_FREQ_HZ);

#ifdef ISO7816_USE_TASK
// Enable usart ISR
	ISO7816_Usart_ISR_Init ();
#endif // ISO7816_USE_TASK

	return (nRet);
}

/*******************************************************************************

  usart_reset_tx

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void usart_reset_tx (volatile avr32_usart_t *usart)
{
  usart->cr |= AVR32_USART_CR_RSTTX_MASK;
}

/*******************************************************************************

  usart_reset_rx

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void usart_reset_rx (volatile avr32_usart_t *usart)
{
  usart->cr |= AVR32_USART_CR_RSTRX_MASK;

}

/*******************************************************************************

  usart_enable_tx

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void usart_enable_tx (volatile avr32_usart_t *usart)
{
  usart->cr |= AVR32_USART_CR_TXEN_MASK;
}

/*******************************************************************************

  usart_disable_tx

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void usart_disable_tx (volatile avr32_usart_t *usart)
{
  usart->cr |= AVR32_USART_CR_TXDIS_MASK;
}

/*******************************************************************************

  usart_enable_rx

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void usart_enable_rx (volatile avr32_usart_t *usart)
{
  usart->cr |= AVR32_USART_CR_RXEN_MASK;
}

/*******************************************************************************

  usart_disable_rx

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void usart_disable_rx (volatile avr32_usart_t *usart)
{
  usart->cr |= AVR32_USART_CR_RXDIS_MASK;
}

/*******************************************************************************

  usart_set_stopbits

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int usart_set_stopbits (volatile avr32_usart_t *usart,int nStopBits)
{
	unsigned int n;
	if ((0 > nStopBits) || (2 < nStopBits))
	{
		return (USART_FAILURE);
	}

	usart->mr &= ~AVR32_USART_MR_NBSTOP_MASK;

	n =  nStopBits << AVR32_USART_MR_NBSTOP_OFFSET;
	usart->mr |= n;

	n = usart->mr;

	return (USART_SUCCESS);
}

/*******************************************************************************

  usart_set_mode9

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int usart_set_mode9 (volatile avr32_usart_t *usart,int nMode)
{
	if ((0 != nMode) && (1 != nMode))
	{
		return (USART_FAILURE);
	}

	usart->mr &= ~AVR32_USART_MODE9_MASK;
	usart->mr |= nMode << AVR32_USART_MODE9_OFFSET;;

	return (USART_SUCCESS);
}

/*******************************************************************************

  usart_set_fidi_ratio

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int usart_set_fidi_ratio (volatile avr32_usart_t *usart,unsigned long nFidiRatio)
{
	usart->fidi = (unsigned long)nFidiRatio;

	return (USART_SUCCESS);
}

/*******************************************************************************

  ISO7816_XOR_String

	Build the checksum of a string
	Return: Checksum

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

unsigned char ISO7816_XOR_String (int nLen,unsigned char *szString)
{
	int i;
	int nXOR;

	nXOR = 0;
	for (i=0;i<nLen;i++)
	{
		nXOR = nXOR ^ szString[i];
	}

	return (nXOR);
}

/*******************************************************************************

  ISO7816_SetFiDiRatio

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_SetFiDiRatio (unsigned long nFidiRatio)
{
	return (usart_set_fidi_ratio (ISO7816_USART,nFidiRatio));
}

/*******************************************************************************

  ISO7816_Is_TX_ReadyISR

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_Is_TX_ReadyISR (int nTimeOutMs)
{
	int i;

	for (i=0;i<nTimeOutMs;i++)
	{
		if (1 == usart_tx_ready(ISO7816_USART))
		{
			return (TRUE);
		}
		vTaskDelay (CCID_TASK_DELAY_1_MS_IN_TICKS);		// 1 ms
	}
	return (FALSE);
}

/*******************************************************************************

  ISO7816_Is_TX_ReadyNoISR

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_Is_TX_ReadyNoISR (int nTimeOutMs)
{
	int i;
	int nDelayCounter;

	nDelayCounter =  nTimeOutMs * ISO7816_CHAR_DELAY_COUNTER_1MS;

	for (i=0;i<nDelayCounter;i++)
	{
		if (1 == usart_tx_ready(ISO7816_USART))
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


/*******************************************************************************

  ISO7816_Is_TX_Ready

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_Is_TX_Ready (int nTimeOutMs)
{
#ifdef ISO7816_USE_TASK
	return (ISO7816_Is_TX_ReadyNoISR (nTimeOutMs));
#else
	return (ISO7816_Is_TX_ReadyNoISR (nTimeOutMs));
#endif // ISO7816_USE_TASK
}


/*******************************************************************************

  ISO7816_Is_RX_Rready

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

/*
inline int ISO7816_Is_RX_Rready (void)
{
//	return (usart_rx_ready(ISO7816_USART));
}
*/
/*******************************************************************************

  ISO7816_SendCharNoISR

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_SendCharNoISR (int nChar,int nTimeOutMs)
{
	int i;
	int nRet;
	int nDelayCounter;

//	usart_disable_rx (ISO7816_USART);
//	usart_enable_tx (ISO7816_USART);


	nDelayCounter =  nTimeOutMs * ISO7816_CHAR_DELAY_COUNTER_1MS;

	ToolPinSet();
	for (i=0;i<nDelayCounter;i++)
	{
		nRet = usart_write_char (ISO7816_USART, nChar);
		switch (nRet)
		{
			case USART_TX_BUSY :
				break;
			case USART_SUCCESS :
				ToolPinClr();
				return (USART_SUCCESS);
			default :
				i = nDelayCounter;		// break for
				break;
		}
	}
	ToolPinClr();
	return (nRet);
}

/*******************************************************************************

  ISO7816_SendChar

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_SendChar (int nChar,int nTimeOutMs)
{
#ifdef ISO7816_USE_TASK
	return (ISO7816_SendCharISR (nChar,nTimeOutMs));
#else
	return (ISO7816_SendCharNoISR (nChar,nTimeOutMs));
#endif // ISO7816_USE_TASK
}

/*******************************************************************************

  ISO7816_ReadCharNoISR

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_ReadCharNoISR (int *nChar,int nTimeOutMs)
{
	int i;
	int nRet;
	int nDelayCounter;

	nDelayCounter =  (nTimeOutMs * ISO7816_CHAR_DELAY_COUNTER_1MS) + 1;  // + 1 = for no timeout
ToolPinSet();

	for (i=0;i<nDelayCounter;i++)
	{
		nRet = usart_read_char(ISO7816_USART, nChar);
		switch (nRet)
		{
			case USART_RX_EMPTY :
				break;
			case USART_SUCCESS :
ToolPinClr();
				return (USART_SUCCESS);
			default :					// RX error
ToolPinClr();
				return (nRet);

				break;
		}
	}
ToolPinClr();

	return (ISO7816_USART_TIMEOUT);
}

/*******************************************************************************

  ISO7816_ReadChar

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_ReadChar (int *nChar,int nTimeOutMs)
{
#ifdef ISO7816_USE_TASK
	return (ISO7816_ReadCharISR ((u32*)nChar,nTimeOutMs));
#else
	return (ISO7816_ReadCharNoISR (nChar,nTimeOutMs));
#endif // ISO7816_USE_TASK

}
/*******************************************************************************

  ISO7816_ClearTx

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_ClearTx (void)
{
	ISO7816_Is_TX_Ready (5);			// timeout 5 ms

//	usart_reset_tx (ISO7816_USART);
//	usart_reset_rx (ISO7816_USART);
	usart_disable_rx (ISO7816_USART);
	usart_enable_tx (ISO7816_USART);
}

/*******************************************************************************

  ISO7816_ClearRx

  AT32UC3A3256 has a problem with switching between RX and TX
  you had to wait until all bytes are send (RX empty shows not the internal
  2 chip buffers) then reset the TX port
  See errata in spec.

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_ClearRx (void)
{
//	int nChar;

// is all transmitted ?
	if (FALSE == ISO7816_Is_TX_Ready (5))		// timeout 5 ms
	{
		usart_reset_tx (ISO7816_USART);			// Reset all
		usart_reset_rx (ISO7816_USART);
	}

//usart_reset_rx (ISO7816_USART);
	ISO7816_Usart_ISR_InitRxBuffer ();

	usart_reset_tx (ISO7816_USART);				// reset the tx port to free the port for receiving
												// (Chip bug)


// 	switch usart direction
//	usart_disable_tx (ISO7816_USART);
//	usart_enable_rx (ISO7816_USART);

/*
	while (USART_SUCCESS == ISO7816_ReadChar (&nChar,0))
	{
	}
*/
}

/*******************************************************************************

  ISO7816_ReadString

  Sendet einen String über ISO7816

  Achtung: Gibt in der Regel einen Timeoutfehler zurück
           da in der Regel nicht bekannt ist wieviele Zeichen kommen

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_ReadString (int *nCount,unsigned char *cString)
{
	int i;
	int nRet;
	int nChar;
	int nTimeOutMs;

  nTimeOutMs = ISO7816_CHAR_DELAY_TIME_FIRST_MS;

//  nTimeOutMs = 1000;  // For fast debugging

	for (i=0;i<*nCount;i++)
	{
		nRet = ISO7816_ReadChar (&nChar,nTimeOutMs);
		if (USART_SUCCESS == nRet)
		{
			cString[i] = (unsigned char)nChar;
		}
		else
		{
			*nCount = i;
			return (nRet);
		}
		nTimeOutMs = ISO7816_CHAR_DELAY_TIME_MS;
	}
	return (USART_SUCCESS);
}

/*******************************************************************************

  ISO7816_SendString

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_SendString (int nCount,unsigned char *cString)
{
	int i;
	int nRet;
	int nPos;
  int nTimeOutMs;
  int nTimeOutCounter;

	nTimeOutMs = ISO7816_CHAR_DELAY_TIME_FIRST_MS;
	nPos = 0;

	ToolPinClr();
	ToolPinSet();
	ToolPinClr();
	ToolPinSet();
	ToolPinClr();

	usart_enable_tx (ISO7816_USART);

// Wait until last byte had leaved the transmitter buffer
  while (TRUE != usart_txTransmitter_ready(ISO7816_USART))
  {
  }

	for (i=0;i<nCount;i++)
	{
		// wait tx ready
		nRet = ISO7816_SendChar ((int)cString[nPos],nTimeOutMs);

		if (USART_SUCCESS == nRet)
		{
			nPos++;
			if (nPos >= nCount)
			{
				break;			// end of transmission
			}
		}

		nTimeOutMs = ISO7816_CHAR_DELAY_TIME_MS;		// next char > short delay
	}

// Wait until last byte had leaved the transmitter buffer
	nTimeOutCounter = 10000;
	while (TRUE != usart_txTransmitter_ready(ISO7816_USART))
	{
	  nTimeOutCounter--;
	  if (0 == nTimeOutCounter)
	  {
	    break;
	  }
	}

//  CI_LocalPrintf ("C %5d\n\r",10000-nTimeOutCounter);

	return (nRet);
}


/*******************************************************************************

  ISO7816_ResetSC_test

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_ResetSC_test (void)
{
	int i;
	int nReadCount;
	unsigned char szText[30];

	ToolPinClr();

	Smartcard_Reset_off ();

  SmartcardPowerOff ();


  DelayMs (1);


	i = Init_ISO7816_Usart ();

//	usart_set_stopbits (ISO7816_USART,AVR32_USART_MR_NBSTOP_1);
//	usart_set_mode9 (ISO7816_USART,1);

//	usart_enable_tx (ISO7816_USART);
	//usart_disable_tx (ISO7816_USART);
//	usart_enable_rx (ISO7816_USART);

//	usart_reset_tx (ISO7816_USART);				// reset the tx port to free the port for receiving

	while (1)
	{
	  SmartcardPowerOn ();
    DelayMs(100);
		Smartcard_Reset_on ();

//		ToolPinSet();

		nReadCount = 30;
		ISO7816_ReadString (&nReadCount,szText);

//		ToolPinClr();

		DelayMs(100);

		Smartcard_Reset_off ();

	  SmartcardPowerOff ();

		DelayMs(100);
	}
}

/*******************************************************************************

  ISO7816_test

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_test (void)
{
	int i;

//	usart_reset(ISO7816_USART);

	Init_ISO7816_Usart ();


	Smartcard_Reset_on ();

	usart_enable_tx (ISO7816_USART);

	while (1)
	{
		i = usart_write_char (ISO7816_USART,0xa5);
		for (i=0;i<100000;i++) ;
	}
}

/*******************************************************************************

  Test_ISO7816_Usart_Pins

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int Test_ISO7816_Usart_Pins (void)
{
	int i;

	gpio_enable_gpio_pin (ISO7816_RST_PIN);				    // PX04
	gpio_enable_gpio_pin (ISO7816_USART_TX_PIN); 		  // PX05
	gpio_enable_gpio_pin (ISO7816_USART_CLK_PIN);     // PA07
	gpio_enable_gpio_pin (AVR32_PIN_PA05);            // VDD
	gpio_enable_gpio_pin (AVR32_PIN_PA06);            // VDD

//	gpio_enable_pin_pull_up(ISO7816_USART_CLK_PIN);

	for (i=0;i<1000000000;i++)
	{
		if (0 == i % 200)
		{
			gpio_set_gpio_pin (ISO7816_RST_PIN);				   // PX04
			gpio_set_gpio_pin (ISO7816_USART_TX_PIN); 		 // PX05
      gpio_set_gpio_pin (ISO7816_USART_CLK_PIN);     // PA07

      SmartcardPowerOn ();
//      gpio_set_gpio_pin (AVR32_PIN_PA05);          // VDD
//      gpio_set_gpio_pin (AVR32_PIN_PA06);          // VDD

//    gpio_set_gpio_pin (AVR32_PIN_PX22);            // PA07

		}
    if (100 == i % 200)
		{
			gpio_clr_gpio_pin (ISO7816_RST_PIN);				// PX04
			gpio_clr_gpio_pin (ISO7816_USART_TX_PIN); 			// PX05
      gpio_clr_gpio_pin (ISO7816_USART_CLK_PIN);      // PA07

      SmartcardPowerOff ();
//      gpio_clr_gpio_pin (AVR32_PIN_PA05);    // VDD
//      gpio_clr_gpio_pin (AVR32_PIN_PA06);    // VDD
//      gpio_clr_gpio_pin (AVR32_PIN_PX22);      // PA07
		}
	}

	return (true);
}



/*******************************************************************************

	Smartcard access

*******************************************************************************/

/*******************************************************************************

  SmartcardPowerOn

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void SmartcardPowerOn (void)
{
  // Init Vcc for smartcard
#ifdef STICK_20_A_MUSTER_PROD
  gpio_set_gpio_pin (AVR32_PIN_PA01);
  gpio_set_gpio_pin (AVR32_PIN_PB05);
#else
  gpio_set_gpio_pin (AVR32_PIN_PA05);
  gpio_set_gpio_pin (AVR32_PIN_PA06);
#endif // STICK_20_A_MUSTER_PROD
}

/*******************************************************************************

  SmartcardPowerOff

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void SmartcardPowerOff (void)
{
// Reset the uart
  usart_reset(ISO7816_USART);

  // Init Vcc for smartcard
#ifdef STICK_20_A_MUSTER_PROD
  gpio_set_gpio_open_drain_pin (AVR32_PIN_PA01);
  gpio_set_gpio_open_drain_pin (AVR32_PIN_PB05);
  gpio_clr_gpio_pin (AVR32_PIN_PA01);
  gpio_clr_gpio_pin (AVR32_PIN_PB05);
#else
  gpio_set_gpio_open_drain_pin (AVR32_PIN_PA05);
  gpio_set_gpio_open_drain_pin (AVR32_PIN_PA06);
  gpio_clr_gpio_pin (AVR32_PIN_PA05);
  gpio_clr_gpio_pin (AVR32_PIN_PA06);
#endif // STICK_20_A_MUSTER_PROD
}

/*******************************************************************************

  Smartcard_Reset_on

  Smart card set active

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void Smartcard_Reset_on (void)
{
	gpio_set_gpio_pin (ISO7816_RST_PIN);
}

/*******************************************************************************

  Smartcard_Reset_off

  Smart card set in reset mode

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void Smartcard_Reset_off (void)
{
	gpio_clr_gpio_pin (ISO7816_RST_PIN);
}




