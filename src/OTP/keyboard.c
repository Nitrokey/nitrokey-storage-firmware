/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *
 *
 * This file is part of Nitrokey.
 *
 * Nitrokey is free software: you can redistribute it and/or modify
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
 * This file contains modifications done by Rudolf Boeddeker
 * For the modifications applies:
 *
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-16
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
// #ifdef NOT_USED

#include "compiler.h"
#include "preprocessor.h"
#include "board.h"
#include "gpio.h"
#include "ctrl_access.h"
#include "flashc.h"
#include "string.h"

#include "conf_usb.h"
#include "usb_drv.h"
#include "usb_descriptors.h"
#include "usb_standard_request.h"

#include "global.h"
#include "tools.h"

#include "keyboard.h"
#include "hotp.h"


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

u8 keyboardBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

u8 PrevXferComplete = 1;

/*******************************************************************************

  sendChar

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void sendChar (u8 chr)
{
    keyboardBuffer[0] = 0;
    keyboardBuffer[3] = 0;

    if (chr >= 'A' && chr <= 'Z')
    {
        keyboardBuffer[0] = MOD_SHIFT_LEFT;
        keyboardBuffer[3] = chr - 61;
    }
    else if (chr >= 'a' && chr <= 'z')
    {
        keyboardBuffer[3] = chr - 93;
    }
    else if (chr >= '0' && chr <= '9')
    {
        if (chr == '0')
            keyboardBuffer[3] = 39;
        else
            keyboardBuffer[3] = chr - 19;
    }
    else if (chr == ' ')
    {
        keyboardBuffer[3] = KEY_SPACE;
    }
    sendKeys (keyboardBuffer);

    keyboardBuffer[0] = 0;
    keyboardBuffer[3] = 0;

    sendKeys (keyboardBuffer);

}

/*******************************************************************************

  sendEnter

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void sendEnter ()
{
    keyboardBuffer[3] = KEY_RETURN;
    sendKeys (keyboardBuffer);

    keyboardBuffer[0] = 0;
    keyboardBuffer[3] = 0;
    sendKeys (keyboardBuffer);
}

/*******************************************************************************

  sendTab

  Changes
  Date      Reviewer        Info
  01.08.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void sendTab (void)
{
    keyboardBuffer[3] = KEY_TAB;
    sendKeys (keyboardBuffer);

    keyboardBuffer[0] = 0;
    keyboardBuffer[3] = 0;
    sendKeys (keyboardBuffer);
}


/*******************************************************************************

  sendKeys

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void sendKeys (u8 * buffer)
{
u32 i;

#ifdef NOT_USED
    if (bDeviceState == CONFIGURED)
    {
        while (!PrevXferComplete);

        PrevXferComplete = 0;
        /*
         * Use the memory interface function to write to the selected
         * endpoint 
         */
        UserToPMABufferCopy (buffer, ENDP4_TXADDR, 8);

        /*
         * Update the data length in the control register 
         */
        SetEPTxCount (ENDP4, 8);
        SetEPTxStatus (ENDP4, EP_TX_VALID);

    }
#endif

    while (Is_usb_endpoint_stall_requested (EP_KB_IN))
    {
        if (Is_usb_setup_received ())
        {
            usb_process_request ();
        }
    }

    // MSC Compliance - Free BAD out receive during SCSI command
    while (Is_usb_out_received (EP_CCID_OUT))
    {
        Usb_ack_out_received_free (EP_CCID_OUT);
    }

    while (!Is_usb_in_ready (EP_KB_IN))
    {
        if (!Is_usb_endpoint_enabled (EP_KB_IN))
        {
            i = 0;  // todo USB Reset
        }
    }

    Usb_reset_endpoint_fifo_access (EP_KB_IN);
    /*
     * Usb_write_endpoint_data(EP_KB_IN, 8, 'D');
     * Usb_write_endpoint_data(EP_KB_IN, 8, 'D');
     * Usb_write_endpoint_data(EP_KB_IN, 8, 'D');
     * Usb_write_endpoint_data(EP_KB_IN, 8, 'D'); 
     */
    usb_write_ep_txpacket (EP_KB_IN, buffer, 8, NULL);
    // Usb_send_in(EP_CONTROL);

    Usb_ack_in_ready_send (EP_KB_IN);


    // MSC Compliance - Wait end of all transmitions on USB line
    while (0 != Usb_nb_busy_bank (EP_KB_IN))
    {
        if (Is_usb_setup_received ())
            usb_process_request ();
    }

}

/*******************************************************************************

  sendNumber

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


void sendNumber (u32 number)
{
u8 result[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
u32 tmp_number = number;
u8 len = 0;
u8 i;

    do
    {
        len++;
        tmp_number /= 10;
    } while (tmp_number > 0);


    for (i = len; i > 0; i--)
    {
        result[i - 1] = number % 10;
        number /= 10;
    }

    for (i = 0; i < len; i++)
        sendChar (result[i] + '0');

}

/*******************************************************************************

  sendNumberN

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void sendNumberN (u32 number, u8 len)
{
u8 result[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
u8 i;

    if (len <= 10)
    {

        for (i = len; i > 0; i--)
        {
            result[i - 1] = number % 10;
            number /= 10;
        }

        for (i = 0; i < len; i++)
            sendChar (result[i] + '0');

    }
}

/*******************************************************************************

  sendString

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void sendString (char *string, u8 len)
{
    u8 i;

    for (i = 0; i < len; i++)
        sendChar (string[i]);

}
