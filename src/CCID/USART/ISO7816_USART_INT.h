/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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
    ISO7816_USART_INT.h

	USART with interrupt
 */

#ifndef ISO7816_USART_INT_H_
#define ISO7816_USART_INT_H_

u32 ISO7816_ReadCharISR (u32 *nChar_pu32,u32 nTimeOutMs_u32);
u32 ISO7816_SendCharISR (u32 nChar_u32,u32 nTimeOutMs_u32);

void ISO7816_Usart_ISR_Init (void);
void ISO7816_Usart_ISR_InitRxBuffer (void);
void USART_Int_Test (void);

#endif /* ISO7816_USART_INT_H_ */
