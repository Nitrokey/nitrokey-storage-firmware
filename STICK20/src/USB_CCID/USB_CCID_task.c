/*
 * Based on device_mass_storage_task.c
 *
 * Author: R.Böddeker
 */


/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Management of the USB device mass-storage task.
 *
 * This file manages the USB device mass-storage task.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a USB module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ***************************************************************************/

/* Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 */
/*
* This file contains modifications done by Rudolf Boeddeker
* For the modifications applies:
*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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



//_____  I N C L U D E S ___________________________________________________

#include "conf_usb.h"


#if USB_DEVICE_FEATURE == ENABLED
#include <stdio.h>

#include "board.h"
#include "print_funcs.h"

#ifdef FREERTOS_USED
#include "FreeRTOS.h"
#include "task.h"
#endif
#include "usb_drv.h"
#include "usb_descriptors.h"
#include "usb_standard_request.h"
#include "ctrl_access.h"
//#include "scsi_decoder.h"
#include "USB_CCID_task.h"

#include "global.h"
#include "tools.h"
#include "usart.h"
#include "CCID\\USART\\ISO7816_USART.h"
#include "CCID\\USART\\ISO7816_ADPU.h"
#include "CCID\\USART\\ISO7816_Prot_T1.h"
#include "USB_CCID.h"

#include "task.h"
#include "TOOLS\\TIME_MEASURING.h"
#include "LED_test.h"



/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/


extern volatile Bool ms_multiple_drive;

//static U32  dCBWTag;


/*******************************************************************************

 Local declarations

*******************************************************************************/

U8 usb_LUN;
U8 ms_endpoint;

// For testing
t_USB_CCID_data_st USB_CCID_data_st;

static void USB_CCID_GetDataFromUSB (void);
static void USB_CCID_SendDataToUSB(void);

/*******************************************************************************

  USB_CCID_send_INT_Message

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void USB_CCID_send_INT_Message (void)
{
	 unsigned short Message_u16;
	 int i,i1;
/* todo
	  while (Is_usb_endpoint_stall_requested(EP_CCID_IN))
	  {
	    if (Is_usb_setup_received()) usb_process_request();
	  }

	  while (Is_usb_endpoint_stall_requested(EP_CCID_OUT))
	  {
	    if (Is_usb_setup_received()) usb_process_request();
	  }

	  // MSC Compliance - Free BAD out receive during SCSI command
	  while( Is_usb_out_received(EP_CCID_OUT) ) {
	    Usb_ack_out_received_free(EP_CCID_OUT);
	  }
*/
	while (!Is_usb_in_ready(EP_CCID_INT));

	Usb_reset_endpoint_fifo_access(EP_CCID_INT);

	Message_u16 = 0x5003;

	i = Usb_byte_count(EP_CCID_INT);

	Usb_write_endpoint_data(EP_CCID_INT, 16, Message_u16);

	i1 = Usb_byte_count(EP_CCID_INT);

	Usb_ack_in_ready_send(EP_CCID_INT);

	// MSC Compliance - Wait end of all transmitions on USB line
	for (i=0;i<100;i++)
	{
    if( 0 != Usb_nb_busy_bank(EP_CCID_INT) )
    {
      if (Is_usb_setup_received()) usb_process_request();
    }
    else
    {
      break;
    }
	}

}

/*******************************************************************************

  USB_CCID_GetDataFromUSB

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

static void USB_CCID_GetDataFromUSB (void)
{
	int i;
	int USB_Datalen_s32;
//	Bool cbw_error;

	Usb_reset_endpoint_fifo_access(EP_CCID_OUT);

	USB_Datalen_s32 = Usb_byte_count (EP_CCID_OUT);

  USB_Log_st.CCID_WriteCalls_u32 ++;
  USB_Log_st.CCID_BytesWrite_u32 += USB_Datalen_s32;

	for (i=0;i<USB_Datalen_s32;i++)
	{
		USB_CCID_data_st.USB_data[i] = Usb_read_endpoint_data(EP_CCID_OUT, 8);
	}

  LED_RedOn ();

	USB_to_CRD_DispatchUSBMessage_v (&USB_CCID_data_st);

  LED_RedOff ();

	Usb_ack_out_received_free(EP_CCID_OUT);

}

/*******************************************************************************

  USB_CCID_SendDataToUSB

  @brief USB Command Status Wrapper (CSW) management

  This function sends the status in relation with the last CBW.

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

static void USB_CCID_SendDataToUSB(void)
{
	int USB_Datalen_s32;
	int i;

	while (Is_usb_endpoint_stall_requested(EP_CCID_IN))
	{
		if (Is_usb_setup_received()) usb_process_request();
	}

	while (Is_usb_endpoint_stall_requested(EP_CCID_OUT))
	{
		if (Is_usb_setup_received()) usb_process_request();
	}

	// MSC Compliance - Free BAD out receive during SCSI command

	while( Is_usb_out_received(EP_CCID_OUT) ) {
		Usb_ack_out_received_free(EP_CCID_OUT);
	}

	while (!Is_usb_in_ready(EP_CCID_IN))
	{
		 if(!Is_usb_endpoint_enabled(EP_CCID_IN))
		 {
			 i = 0; // todo USB Reset
		 }
	}

	Usb_reset_endpoint_fifo_access(EP_CCID_IN);

	USB_Datalen_s32 = USB_CCID_data_st.CCID_datalen + 10;

	for (i=0;i<USB_Datalen_s32;i++)
    {
		Usb_write_endpoint_data(EP_CCID_IN, 8, USB_CCID_data_st.USB_data[i]);
	}

	Usb_ack_in_ready_send(EP_CCID_IN);
  
  USB_Log_st.CCID_ReadCalls_u32 ++;
  USB_Log_st.CCID_BytesRead_u32 += USB_Datalen_s32;


	// MSC Compliance - Wait end of all transmitions on USB line
	while( 0 != Usb_nb_busy_bank(EP_CCID_IN) )
	{
		if (Is_usb_setup_received()) usb_process_request();
	}
}


/*******************************************************************************

  USB_CCID_task_init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void USB_CCID_task_init(void)
{
  xTaskCreate(USB_CCID_task,
              configTSK_USB_CCID_NAME,
              configTSK_USB_CCID_STACK_SIZE,
              NULL,
              configTSK_USB_CCID_PRIORITY,
              NULL);
}

/*******************************************************************************

  USB_CCID_task

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void USB_CCID_task(void *pvParameters)
{
	unsigned char Startup_b = TRUE;
	portTickType xLastWakeTime;

	ISO7816_InitSC ();
/*
	CI_LocalPrintf ("USB_CCID : USB CCID raise IN ready - %d\n",xTaskGetTickCount());
*/
// Start CCID interface
  while( 0 != Usb_nb_busy_bank(EP_CCID_IN) )        // MSC Compliance - Wait end of all transmitions on USB line
  {
    if (Is_usb_setup_received()) usb_process_request();
  }
	Usb_raise_in_ready (EP_CCID_IN);

// Start keyboard interface
  while( 0 != Usb_nb_busy_bank(EP_KB_IN) )        // MSC Compliance - Wait end of all transmitions on USB line
  {
    if (Is_usb_setup_received()) usb_process_request();
  }
  Usb_raise_in_ready (EP_KB_IN);


	xLastWakeTime = xTaskGetTickCount();

	while (TRUE)
	{
		vTaskDelayUntil(&xLastWakeTime, configTSK_USB_CCID_PERIOD);

		// First, check the device enumeration state
		if (!Is_device_enumerated()) continue;

		// If smartcard is ready send it over usb
		if (TRUE == Startup_b)
		{
			Startup_b = FALSE;
//CI_LocalPrintf ("USB_CCID : USB CCID started - %d\n",xTaskGetTickCount());
			DelayMs (100);						// Wait 100 ms
			USB_CCID_send_INT_Message ();			// We are always online
		}

		// If we receive something in the OUT endpoint, parse it
		if (Is_usb_out_received(EP_CCID_OUT))
		{
#ifdef TIME_MEASURING_ENABLE
			TIME_MEASURING_Start (TIME_MEASURING_TIME_CCID_USB_GET);
#endif
			USB_CCID_GetDataFromUSB ();
#ifdef TIME_MEASURING_ENABLE
			TIME_MEASURING_Stop (TIME_MEASURING_TIME_CCID_USB_GET);
			TIME_MEASURING_Start (TIME_MEASURING_TIME_CCID_USB_SEND);
#endif
			USB_CCID_SendDataToUSB ();
#ifdef TIME_MEASURING_ENABLE
			TIME_MEASURING_Stop (TIME_MEASURING_TIME_CCID_USB_SEND);
#endif
		}
	}
}


#endif  // USB_DEVICE_FEATURE == ENABLED
