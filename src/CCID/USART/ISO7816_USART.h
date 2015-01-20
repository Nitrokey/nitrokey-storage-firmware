/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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
 * ISO7816_USART.h
 *
 *  Created on: 19.06.2010
 *      Author: RB
 */

#ifndef ISO7816_USART_H_
#define ISO7816_USART_H_


// Define for activate ISO7816 task
#define ISO7816_USE_TASK



/*
 * Delay timeout for ISO7816 transfers 
 */
#define ISO7816_CHAR_DELAY_TIME_FIRST_MS         30000  // ca. 1000 ms normal 
                                                        // - generate 3072
                                                        // bit key > 20 sec
#define ISO7816_CHAR_DELAY_TIME_MS                   5  // ca.  5 ms

#define ISO7816_CHAR_DELAY_TIME_FIRST_CHAR_PTS    1000  // ca. 1000 ms -
                                                        // Delay for PTS
                                                        // answer


#define ISO7816_USART_TIMEOUT					10  // see also usart.h for
                                                    // error codes


// #define ISO7816_CHAR_DELAY_COUNTER_1MS 131 // at 12 MHz
#define ISO7816_CHAR_DELAY_COUNTER_1MS      650 // at 60 MHz

// #define FPBA_HZ 60000000

#define CCID_TARGET_PBACLK_FREQ_HZ FPBA_HZ  // FOSC0 // PBA clock target
                                            // frequency, in Hz

/*******************************************************************************

	USART chip defines

*******************************************************************************/

#define ISO7816_USART               	(&AVR32_USART1)
/*
 * #define ISO7816_USART_RX_PIN AVR32_USART1_RXD_0_0_PIN #define
 * ISO7816_USART_RX_FUNCTION AVR32_USART1_RXD_0_0_FUNCTION #define
 * ISO7816_USART_TX_PIN AVR32_USART1_TXD_0_0_PIN // Used for ISO7816 #define
 * ISO7816_USART_TX_FUNCTION AVR32_USART1_TXD_0_0_FUNCTION 
 */
/*
 * Alte SIO
 */
#define ISO7816_USART_RX_PIN        	AVR32_USART1_RXD_0_2_PIN
#define ISO7816_USART_RX_FUNCTION   	AVR32_USART1_RXD_0_2_FUNCTION
#define ISO7816_USART_TX_PIN        	AVR32_USART1_TXD_0_2_PIN
#define ISO7816_USART_TX_FUNCTION   	AVR32_USART1_TXD_0_2_FUNCTION

#define ISO7816_USART_CLK_PIN       	AVR32_USART1_CLK_0_PIN
#define ISO7816_USART_CLK_FUNCTION  	AVR32_USART1_CLK_0_FUNCTION
#define ISO7816_USART_CLOCK_MASK    	AVR32_USART1_CLK_PBA
/*
 * #define EXAMPLE_USART (&AVR32_USART1) # define EXAMPLE_USART_RX_PIN
 * AVR32_USART1_RXD_0_0_PIN # define EXAMPLE_USART_RX_FUNCTION
 * AVR32_USART1_RXD_0_0_FUNCTION # define EXAMPLE_USART_TX_PIN
 * AVR32_USART1_TXD_0_0_PIN # define EXAMPLE_USART_TX_FUNCTION
 * AVR32_USART1_TXD_0_0_FUNCTION # define EXAMPLE_USART_CLOCK_MASK
 * AVR32_USART1_CLK_PBA 
 */

/*
 * Test Reset #define ISO7816_RST_PIN AVR32_PIN_PA01 
 */

#define ISO7816_RST_PIN       			AVR32_PIN_PX04

#define EXAMPLE_PDCA_CLOCK_HSB      AVR32_PDCA_CLK_HSB
#define EXAMPLE_PDCA_CLOCK_PB       AVR32_PDCA_CLK_PBA


/*******************************************************************************

	Prototypes

*******************************************************************************/

int Init_ISO7816_Usart (void);
int Test_ISO7816_Usart_Pins (void);

unsigned char ISO7816_XOR_String (int nLen, unsigned char *szString);

int ISO7816_Is_TX_Ready (int nTimeOutMs);
int ISO7816_SetFiDiRatio (unsigned long nFidiRatio);
void ISO7816_ClearRx (void);
void ISO7816_ClearTx (void);
void usart_reset_tx (volatile avr32_usart_t * usart);
void usart_reset_rx (volatile avr32_usart_t * usart);

int ISO7816_ReadChar (int *nChar, int nTimeOutMs);
int ISO7816_SendChar (int nChar, int nTimeOutMs);

int ISO7816_ReadString (int *nCount, unsigned char *szText);
int ISO7816_SendString (int nCount, unsigned char *szText);

void ISO7816_test (void);
void ISO7816_ResetSC_test (void);

void SmartcardPowerOn (void);
void SmartcardPowerOff (void);

void Smartcard_Reset_on (void);
void Smartcard_Reset_off (void);

#endif /* ISO7816_USART_H_ */
