/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 09.04.2011
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
 * Inbetriebnahme.c
 *
 *  Created on: 09.04.2011
 *      Author: RB
 */

#ifndef _BUFFERED_SIO_H_
#define _BUFFERED_SIO_H_


// BUFFERED_SIO Ports
#define BUFFERED_SIO_USART               (&AVR32_USART0)
#define BUFFERED_SIO_USART_RX_PIN        AVR32_USART0_RXD_0_0_PIN
#define BUFFERED_SIO_USART_RX_FUNCTION   AVR32_USART0_RXD_0_0_FUNCTION
#define BUFFERED_SIO_USART_TX_PIN        AVR32_USART0_TXD_0_0_PIN
#define BUFFERED_SIO_USART_TX_FUNCTION   AVR32_USART0_TXD_0_0_FUNCTION
#define BUFFERED_SIO_USART_IRQ           AVR32_USART0_IRQ


#ifndef STICK_20_SEND_DEBUGINFOS_VIA_HID
  #define BUFFERED_SIO_TX_BUFFER_SIZE     5000
#else
  #define BUFFERED_SIO_TX_BUFFER_SIZE     6000    // More buffer for slow HID communication
#endif

#define BUFFERED_SIO_RX_BUFFER_SIZE      200


u8  BUFFERED_SIO_WriteString(u16 Len,u8 *String);
void BUFFERED_SIO_SendHandler(void);
u8  BUFFERED_SIO_GetByte (u8 *Data);
u8 BUFFERED_SIO_ByteReceived (void);
u8 BUFFERED_SIO_TxEmpty (void);
u16  BUFFERED_SIO_Init (void);
u8 BUFFERED_SIO_HID_TxEmpty (void);

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
u8 BUFFERED_SIO_HID_GetSendChars (u8 *Data_pu8,u8 MaxSendChars_u8);
#endif


#endif  /* ifndef _BUFFERED_SIO_H_ */
