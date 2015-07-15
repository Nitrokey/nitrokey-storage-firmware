/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 10.09.2010
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
 * ISO7816_Prot_T0.c
 *
 *  Created on: 10.09.2010
 *      Author: RB
 */

// Only some test functions



#include <avr32/io.h>
#include "stdio.h"
#include "string.h"
#include "compiler.h"
#include "board.h"
#include "power_clocks_lib.h"
#include "gpio.h"
#include "usart.h"

#include "tools.h"
#include "ISO7816_USART.h"
#include "ISO7816_ADPU.h"
#include "ISO7816_Prot_T0.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

#define ISO7861_RESPONSE_STATE_ERROR 		   0
#define ISO7861_RESPONSE_STATE_NO_STATUS 	 1
#define ISO7861_RESPONSE_STATE_ACK 			   2
#define ISO7861_RESPONSE_STATE_STATUS 		 3


/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

/*******************************************************************************

  ISO7861_Check_APDU_Status

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7861_T0_Check_APDU_Status (typeAPDU * tSC)
{
int nData;
int nRet;

    nData = 0;

    nRet = ISO7816_ReadChar (&nData, ISO7816_CHAR_DELAY_TIME_MS);
    if (USART_SUCCESS != nRet)
    {
        tSC->tState.cStatus = ISO7861_RESPONSE_STATE_NO_STATUS;
        return (TRUE);
    }

    // Correct answer ?
    if (((nData & 0xF0) != 0x60) && ((nData & 0xF0) != 0x90))
    {
        tSC->tState.cStatus = ISO7861_RESPONSE_STATE_ERROR; // No
        return (FALSE);
    }

    // Got ACK ?
    if (((nData & 0xFE) == ((~(tSC->tAPDU.cINS)) & 0xFE)) || ((nData & 0xFE) == (tSC->tAPDU.cINS & 0xFE)))
    {
        tSC->tState.cStatus = ISO7861_RESPONSE_STATE_ACK;   // Got ACK
        tSC->tState.cSW1 = nData;   // Save ACK byte
        return (TRUE);
    }

    // SW1 received
    tSC->tState.cSW1 = nData;   // Save ACK byte

    // Get SW2
    nRet = ISO7816_ReadChar (&nData, ISO7816_CHAR_DELAY_TIME_MS);
    if (USART_SUCCESS != nRet)
    {
        tSC->tState.cStatus = ISO7861_RESPONSE_STATE_NO_STATUS;
        return (FALSE);
    }

    tSC->tState.cSW2 = nData;
    return (TRUE);
}
