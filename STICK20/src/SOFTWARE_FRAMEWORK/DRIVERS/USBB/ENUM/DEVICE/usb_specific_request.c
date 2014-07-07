/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Processing of USB device specific enumeration requests.
 *
 * This file contains the specific request decoding for enumeration process.
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


//_____ I N C L U D E S ____________________________________________________

#include "global.h"
#include "tools.h"

#include "conf_usb.h"

#ifdef FREERTOS_USED
  #include "FreeRTOS.h"
  #include "task.h"
#endif

#if USB_DEVICE_FEATURE == ENABLED

#include "usb_drv.h"
#include "usb_descriptors.h"
#include "usb_standard_request.h"
#include "usb_specific_request.h"
#include "ctrl_access.h"

#include "OTP/report_protocol.h"
#include "Interpreter.h"
#include "string.h"

//_____ M A C R O S ________________________________________________________


//_____ D E F I N I T I O N S ______________________________________________

volatile Bool ms_multiple_drive;


//_____ P R I V A T E   D E C L A R A T I O N S ____________________________

extern volatile U8    usb_hid_report_mouse[];
U8 g_u8_report_rate=0;

extern const  void *pbuffer;
extern        U16   data_to_transfer;

static void hid_get_descriptor(U8 size_of_report, const U8* p_usb_hid_report);

//_____ D E C L A R A T I O N S ____________________________________________


#define DEBUG_USB_IO

#ifdef DEBUG_USB_IO
#else
  #define CI_LocalPrintf(...)
  #define CI_TickLocalPrintf(...)
  #define CI_StringOut(...)
  #define CI_Print8BitValue(...)
#endif

u8 Stick20HIDSendMatrixData (u8 *output);
extern u8 Stick20HIDSendConfigurationState_u8;

#define STICK20_SEND_STATUS_IDLE     0
#define STICK20_SEND_STATUS_PIN      1
#define STICK20_SEND_STARTUP         2
#define STICK20_SEND_PRODUCTION_TEST 3

/*******************************************************************************
* Function Name  : CCID_Status_In
* Description    : CCID Storage Status IN routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/

u8 lastLEDState=0;
u64 lastNumLockChange=0;
u64 lastCapsLockChange=0;
u64 lastScrollLockChange=0;
//extern u64 currentTime;
volatile u8 numLockClicked    = 0;
volatile u8 capsLockClicked   = 0;
volatile u8 scrollLockClicked = 0;

#define NUM_LOCK_LED  (1<<0)
#define CAPS_LOCK_LED (1<<1)
#define SCROLL_LOCK_LED (1<<2)

#define DOUBLE_CLICK_TIME 500

void Keyboard_SetReport_Output (void)
{
  u32 BytesIn_u32;
  u32 i;
  u16 Index_u16;
  u16 Length_u16;


  Index_u16 = Usb_read_endpoint_data(EP_CONTROL, 16);
  Index_u16 = usb_format_usb_to_mcu_data(16, Index_u16);
  Length_u16 = Usb_read_endpoint_data(EP_CONTROL, 16);
  Length_u16 = usb_format_usb_to_mcu_data(16, Length_u16);


  Usb_ack_setup_received_free();
  while (!Is_usb_control_out_received());

  Usb_reset_endpoint_fifo_access(EP_CONTROL);

  // Get number of transferd bytes
  BytesIn_u32 = Usb_read_endpoint_data(EP_CONTROL, 8);

  if (KEYBOARD_FEATURE_COUNT < BytesIn_u32)
  {
    BytesIn_u32 = KEYBOARD_FEATURE_COUNT; //!< send only requested number of data bytes
  }

  // Get the payload
  usb_read_ep_rxpacket(EP_CONTROL, HID_SetReport_Value, BytesIn_u32, NULL);

  Usb_ack_control_out_received_free();
  Usb_ack_control_in_ready_send();
  while (!Is_usb_control_in_ready());

  CI_Print8BitValue (BytesIn_u32);
  CI_StringOut (" -");
  for (i=0;i<BytesIn_u32;i++)
  {
      CI_Print8BitValue (HID_SetReport_Value[i]);
  }
  CI_StringOut ("-\n\r");


  u8 LEDState = HID_SetReport_Value[0];

  if ((lastLEDState&NUM_LOCK_LED)!=(LEDState & NUM_LOCK_LED))
  {//num lock changed
    if ((xTaskGetTickCount ()  - lastNumLockChange) < DOUBLE_CLICK_TIME)
    {
      numLockClicked = 1;
    }
    lastNumLockChange = xTaskGetTickCount ();
  }

  if ((lastLEDState&CAPS_LOCK_LED)!=(LEDState&CAPS_LOCK_LED))
  {//caps lock changed
    if ((xTaskGetTickCount ()-lastCapsLockChange)<DOUBLE_CLICK_TIME)
    {
      capsLockClicked=1;
    }
    lastCapsLockChange=xTaskGetTickCount ();
  }

  if ((lastLEDState&SCROLL_LOCK_LED)!=(LEDState&SCROLL_LOCK_LED))
  {//numlock changed
    if ((xTaskGetTickCount ()-lastScrollLockChange)<DOUBLE_CLICK_TIME)
    {
        scrollLockClicked=1;
    }
    lastScrollLockChange=xTaskGetTickCount ();
  }

  lastLEDState=LEDState;

  return;
}


void Stick20HIDSendDebugData (u8 *output);
extern u8  Stick20HIDSendMatrixState_u8;

u8 *Keyboard_GetReport_Feature(u16 Length)
{
  if (Length == 0)
  {
//    pInformation->Ctrl_Info.Usb_wLength = KEYBOARD_FEATURE_COUNT;   // USB send bytes
    return (NULL);
  }
  else {
//    u32 i;

    memcpy((void*)HID_GetReport_Value,HID_GetReport_Value_tmp,KEYBOARD_FEATURE_COUNT);
//    HID_GetReport_Value[0] = OTP_device_status;                         // Todo RB usage of OTP_device_status

// Send password matrix ?
    if (0 != Stick20HIDSendMatrixState_u8)
    {
      Stick20HIDSendMatrixData (HID_GetReport_Value);
    }
    else if (STICK20_SEND_STATUS_PIN == Stick20HIDSendConfigurationState_u8)
    {
      Stick20HIDSendAccessStatusData (HID_GetReport_Value);
    }
    else if (STICK20_SEND_STARTUP == Stick20HIDSendConfigurationState_u8)
    {
      Stick20HIDSendAccessStatusData (HID_GetReport_Value);
    }
    else if (STICK20_SEND_PRODUCTION_TEST == Stick20HIDSendConfigurationState_u8)
    {
      Stick20HIDSendProductionInfos (HID_GetReport_Value);
    }

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
    else   // Send debug data
    {
      Stick20HIDSendDebugData (HID_GetReport_Value);
    }
#endif
/*
#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
    Stick20HIDSendDebugData (HID_GetReport_Value);
#endif
*/
    hid_get_descriptor(KEYBOARD_FEATURE_COUNT, (const U8*) HID_GetReport_Value);

/*
for (i=0;i<10;i++)
{
    CI_Print8BitValue (HID_GetReport_Value[i]);
}
CI_StringOut ("\n\r");
*/

    if (0 != HID_GetReport_Value[0])
    {
      HID_GetReport_Value[0] = 1;
    }
    return (HID_GetReport_Value);
  }
}
#ifdef NOT_USED
#endif // NOT_USED

u8 *Keyboard_SetReport_Feature(u16 Length)
{
  u32 BytesIn_u32;
  u32 i;
  u16 Index_u16;
  u16 Length_u16;

  Index_u16 = Usb_read_endpoint_data(EP_CONTROL, 16);
  Index_u16 = usb_format_usb_to_mcu_data(16, Index_u16);
  Length_u16 = Usb_read_endpoint_data(EP_CONTROL, 16);
  Length_u16 = usb_format_usb_to_mcu_data(16, Length_u16);


  Usb_ack_setup_received_free();
  while (!Is_usb_control_out_received());

  Usb_reset_endpoint_fifo_access(EP_CONTROL);

  // Get number of transferd bytes
  BytesIn_u32 = Length_u16; //Usb_read_endpoint_data(EP_CONTROL, 8);

  if (KEYBOARD_FEATURE_COUNT < BytesIn_u32)
  {
    BytesIn_u32 = KEYBOARD_FEATURE_COUNT; //!< send only requested number of data bytes
  }

  // Get the payload
  usb_read_ep_rxpacket(EP_CONTROL, HID_SetReport_Value, BytesIn_u32, NULL);


  memcpy(HID_SetReport_Value_tmp,HID_SetReport_Value,KEYBOARD_FEATURE_COUNT);
  OTP_device_status=STATUS_RECEIVED_REPORT;
/* USB Debug
  CI_StringOut ("\n\r Get ");
  for (i=0;i<BytesIn_u32;i++)
  {
      CI_Print8BitValue (HID_SetReport_Value[i]);
  }
  CI_StringOut ("\n\r");
*/
  Usb_ack_control_out_received_free();
  Usb_ack_control_in_ready_send();
  while (!Is_usb_control_in_ready());

  return (NULL);
}
/*
 * Keyboard_SetReport_Output
u8 *Keyboard_SetReport_Output(u16 Length)
{
  if (Length == 0)
  {
//    pInformation->Ctrl_Info.Usb_wLength = 1;                       // USB send bytes
    return (NULL);
  }
  else {
    return (HID_SetReport_Value);
  }
}
*/

//! @brief This function manages hid set idle request.
//!
//! @param u8_report_id    0 the idle rate applies to all input reports, else only applies to the Report ID
//! @param u8_duration     When the upper byte of wValue is 0 (zero), the duration is indefinite else from 0.004 to 1.020 seconds

//!
void usb_hid_set_idle (U8 u8_report_id, U8 u8_duration )
{
   U16 wInterface;

   // Get interface number to put in idle mode
   wInterface=Usb_read_endpoint_data(EP_CONTROL, 16);
   Usb_ack_setup_received_free();

#ifdef KB_INTERFACE_NB
   if( wInterface == KB_INTERFACE_NB )
     g_u8_report_rate = u8_duration;
#endif

   Usb_ack_control_in_ready_send();
   while (!Is_usb_control_in_ready());
}


//! @brief This function manages hid get idle request.
//!
//! @param u8_report_id    0 the idle rate applies to all input reports, else only applies to the Report ID
//!
void usb_hid_get_idle (U8 u8_report_id)
{
   U16 wLength;
   U16 wInterface;

   // Get interface number to put in idle mode
   wInterface=Usb_read_endpoint_data(EP_CONTROL, 16);
   wLength   =Usb_read_endpoint_data(EP_CONTROL, 16);
   Usb_ack_setup_received_free();

#ifdef KB_INTERFACE_NB
   if( (wLength != 0) && (wInterface == KB_INTERFACE_NB) )
   {
      Usb_write_endpoint_data(EP_CONTROL, 8, g_u8_report_rate);
      Usb_ack_control_in_ready_send();
   }
#endif

   while (!Is_usb_control_out_received());
   Usb_ack_control_out_received_free();
}


//! This function manages the HID Get_Descriptor request.
//!
static void hid_get_descriptor(U8 size_of_report, const U8* p_usb_hid_report)
{
  Bool  zlp;
  U16   wIndex;
  U16   wLength;
  U32   Timeout_u32;

  zlp = FALSE;                                              /* no zero length packet */

  data_to_transfer = size_of_report;
  pbuffer          = p_usb_hid_report;

  wIndex = Usb_read_endpoint_data(EP_CONTROL, 16);
  wIndex = usb_format_usb_to_mcu_data(16, wIndex);
  wLength = Usb_read_endpoint_data(EP_CONTROL, 16);
  wLength = usb_format_usb_to_mcu_data(16, wLength);
  Usb_ack_setup_received_free();                          //!< clear the setup received flag

  if (wLength > data_to_transfer)
  {
    zlp = !(data_to_transfer % EP_CONTROL_LENGTH);  //!< zero length packet condition
  }
  else
  {
    data_to_transfer = wLength; //!< send only requested number of data bytes
  }

  Usb_ack_nak_out(EP_CONTROL);

  while (data_to_transfer && (!Is_usb_nak_out(EP_CONTROL)))
  {
    while( !Is_usb_control_in_ready() && !Is_usb_nak_out(EP_CONTROL) );

    if( Is_usb_nak_out(EP_CONTROL) )
       break;    // don't clear the flag now, it will be cleared after

    Usb_reset_endpoint_fifo_access(EP_CONTROL);
    data_to_transfer = usb_write_ep_txpacket(EP_CONTROL, pbuffer,
                                             data_to_transfer, &pbuffer);
    if( Is_usb_nak_out(EP_CONTROL) )
       break;
    else
       Usb_ack_control_in_ready_send();  //!< Send data until necessary
  }

  if ( zlp && (!Is_usb_nak_out(EP_CONTROL)) )
  {
    while (!Is_usb_control_in_ready());
    Usb_ack_control_in_ready_send();
  }

//  while (!(Is_usb_nak_out(EP_CONTROL)));

  Timeout_u32 = 0;
  while (!(Is_usb_nak_out(EP_CONTROL)))
  {
    Timeout_u32++;
    if (100000 < Timeout_u32)
    {
      break;
    }
  }


  Usb_ack_nak_out(EP_CONTROL);
//  while (!Is_usb_control_out_received());


  Timeout_u32 = 0;
  while (!Is_usb_control_out_received())
  {
    Timeout_u32++;
    if (100000 < Timeout_u32)
    {
      break;
    }
  }
  Usb_ack_control_out_received_free();

}





//! @brief This function configures the endpoints of the device application.
//! This function is called when the set configuration request has been received.
//!
void usb_user_endpoint_init(U8 conf_nb)
{
  ms_multiple_drive = FALSE;

#if (USB_HIGH_SPEED_SUPPORT==ENABLED)
   if( !Is_usb_full_speed_mode() )
   {
#ifdef USB_MSD
     (void)Usb_configure_endpoint(EP_MS_IN,
                                  EP_ATTRIBUTES_1,
                                  DIRECTION_IN,
                                  EP_SIZE_1_HS,
                                  DOUBLE_BANK);
   
     (void)Usb_configure_endpoint(EP_MS_OUT,
                                  EP_ATTRIBUTES_2,
                                  DIRECTION_OUT,
                                  EP_SIZE_2_HS,
                                  DOUBLE_BANK);
#endif //  USB_MSD
#ifdef USB_CCID
	  (void)Usb_configure_endpoint(EP_CCID_INT,
								   EP_ATTRIBUTES_3,
								   DIRECTION_IN,
								   EP_SIZE_3_HS,
								   SINGLE_BANK);

	  (void)Usb_configure_endpoint(EP_CCID_OUT,
								   EP_ATTRIBUTES_4,
								   DIRECTION_OUT,
								   EP_SIZE_4_HS,
								   SINGLE_BANK);

	  (void)Usb_configure_endpoint(EP_CCID_IN,
								   EP_ATTRIBUTES_5,
								   DIRECTION_IN,
								   EP_SIZE_5_HS,
								   SINGLE_BANK);
#endif //  USB_CCID
#ifdef USB_KB
    (void)Usb_configure_endpoint(EP_KB_IN,
                   EP_ATTRIBUTES_6,
                   DIRECTION_IN,
                   EP_SIZE_6_HS,
                   SINGLE_BANK);
#endif // USB_KB
      return;
   }
#endif

#ifdef USB_MSD
  (void)Usb_configure_endpoint(EP_MS_IN,
                               EP_ATTRIBUTES_1,
                               DIRECTION_IN,
                               EP_SIZE_1_FS,
                               DOUBLE_BANK);

  (void)Usb_configure_endpoint(EP_MS_OUT,
                               EP_ATTRIBUTES_2,
                               DIRECTION_OUT,
                               EP_SIZE_2_FS,
                               DOUBLE_BANK);
#endif //  USB_MSD

#ifdef USB_CCID
  (void)Usb_configure_endpoint(EP_CCID_INT,
                               EP_ATTRIBUTES_3,
                               DIRECTION_IN,
                               EP_SIZE_3_FS,
                               SINGLE_BANK);

  (void)Usb_configure_endpoint(EP_CCID_OUT,
                               EP_ATTRIBUTES_4,
                               DIRECTION_OUT,
                               EP_SIZE_4_FS,
                               SINGLE_BANK);

  (void)Usb_configure_endpoint(EP_CCID_IN,
                               EP_ATTRIBUTES_5,
                               DIRECTION_IN,
                               EP_SIZE_5_FS,
                               SINGLE_BANK);
#endif //  USB_CCID
#ifdef USB_KB
  (void)Usb_configure_endpoint(EP_KB_IN,
                               EP_ATTRIBUTES_6,
                               DIRECTION_IN,
                               EP_SIZE_6_FS,
                               SINGLE_BANK);
#endif // USB_KB
}


//! This function is called by the standard USB read request function when
//! the USB request is not supported. This function returns TRUE when the
//! request is processed. This function returns FALSE if the request is not
//! supported. In this case, a STALL handshake will be automatically
//! sent by the standard USB read request function.
//!
Bool usb_user_read_request(U8 type, U8 request)
{
   U16 wInterface;
   U8 wValue_msb;
   U8 wValue_lsb;
   U8 text[20];

   wValue_lsb = Usb_read_endpoint_data(EP_CONTROL, 8);
   wValue_msb = Usb_read_endpoint_data(EP_CONTROL, 8);
   
   //** Specific request from Class HID
   if(USB_SETUP_GET_STAND_INTERFACE == type)   //  0x81 = USB_SETUP_GET_STAND_INTERFACE
   {
      switch( request )
      {
         case GET_DESCRIPTOR:
           switch( wValue_msb ) // Descriptor ID
           {
  #if (USB_HIGH_SPEED_SUPPORT==DISABLED)
    #ifdef KB_INTERFACE_NB
              case HID_DESCRIPTOR:
                CI_LocalPrintf ("Get GET_HID_DESCRIPTOR\r\n");
                hid_get_descriptor(sizeof(usb_conf_desc_fs.hid_kb),(const U8*)&usb_conf_desc_fs.hid_kb);
                return TRUE;
    #endif
  #else
    #ifdef KB_INTERFACE_NB
              case HID_DESCRIPTOR:
                CI_LocalPrintf ("Get GET_HID_DESCRIPTOR\r\n");
                if( Is_usb_full_speed_mode() )
                {
                   hid_get_descriptor(sizeof(usb_conf_desc_fs.hid_kb),  (const U8*)&usb_conf_desc_fs.hid_kb);
                }else
                {
                   hid_get_descriptor(sizeof(usb_conf_desc_hs.hid_kb),  (const U8*)&usb_conf_desc_hs.hid_kb);
                }
                return TRUE;

    #endif
  #endif

              case HID_REPORT_DESCRIPTOR:
                CI_StringOut ("Get GET_HID_REPORT_DESCRIPTOR - Size = "); //,sizeof(usb_hid_report_descriptor_keyboard));
                itoa (sizeof(usb_hid_report_descriptor_keyboard),text);
                CI_StringOut (text);
                CI_StringOut ("\r\n");
                hid_get_descriptor(sizeof(usb_hid_report_descriptor_keyboard),usb_hid_report_descriptor_keyboard);
                return TRUE;

              case HID_PHYSICAL_DESCRIPTOR:
                CI_StringOut ("Get GET_HID_PHYSICAL_DESCRIPTOR\r\n");
                // TODO
                break;
              default :
                CI_StringOut ("Get GET_DESCRIPTOR unknown\r\n");
                break;
           }
         break;
      }
   }


   //** Specific request from Class MassStorage
   if( USB_SETUP_SET_CLASS_INTER == type )
   {
      switch( request )
      {
         case MASS_STORAGE_RESET:
         // wValue must be 0
         // wIndex = Interface
         if( (0!=wValue_lsb) || (0!=wValue_msb) )
            break;
         wInterface=Usb_read_endpoint_data(EP_CONTROL, 16);
#ifdef USB_MSD
         if( INTERFACE_NB != wInterface )
#endif //  USB_MSD
            break;
         Usb_ack_setup_received_free();
         Usb_ack_control_in_ready_send();
         return TRUE;
// HID specific
/*
      case SET_REPORT:
        switch( pInformation->USBwValue1 )          // report type USBwValue1 byte from 1,0
        {
          case HID_FEATURE:
            CopyRoutine = Keyboard_SetReport_Feature;
            Request = SET_REPORT;
            RequestType = HID_FEATURE;
            break;
          case HID_OUTPUT:
            CopyRoutine = Keyboard_SetReport_Output;
            Request = SET_REPORT;
            RequestType = HID_OUTPUT;
            break;
        }
        break;
 */
         case HID_SET_REPORT:
           // The MSB wValue field specifies the Report Type
           // The LSB wValue field specifies the Report ID
           switch (wValue_msb)
           {
              case HID_REPORT_INPUT:
                CI_StringOut ("Get HID_SET_REPORT_INPUT\r\n");                 // TODO
                break;


              case HID_REPORT_OUTPUT:
                CI_StringOut ("Get HID_SET_REPORT_OUTPUT\r\n");
//                CI_StringOut ("A");
                Keyboard_SetReport_Output ();
                CI_StringOut ("Get HID_SET_REPORT_OUTPUT - DONE\r\n");
/*
                {
                  s32 USB_Datalen_s32 = Usb_byte_count (EP_CONTROL);
                  u32 data = 0;

                  CI_StringOut ("Get HID_SET_REPORT_OUTPUT\r\n");
  //                CI_StringOut ("A");

                  data = Usb_read_endpoint_data(EP_CONTROL, 32);
                  USB_Datalen_s32 = Usb_byte_count (EP_CONTROL);

                  Usb_ack_setup_received_free();
                  while (!Is_usb_control_out_received());
                  Usb_ack_control_out_received_free();
                }
                Usb_ack_control_in_ready_send();
                while (!Is_usb_control_in_ready());

*/
/*
                if (OTP_device_status==STATUS_RECEIVED_REPORT)
                {
                  OTP_device_status=STATUS_BUSY;
                  parse_report(HID_SetReport_Value_tmp,HID_GetReport_Value_tmp);
                  OTP_device_status=STATUS_READY;
                }
*/
//                CI_StringOut ("B");

                return TRUE;

              case HID_REPORT_FEATURE:
                CI_StringOut ("Get HID_SET_REPORT_FEATURE\r\n");
                Keyboard_SetReport_Feature(0);    // RB  Length todo
                break;
              default:
                CI_StringOut ("Get HID_GET_REPORT unknown\r\n");
                break;
           }
           break;

         case HID_SET_IDLE:

           CI_StringOut ("Get HID_SET_IDLE - report ");
           itoa (wValue_lsb,text);
           CI_StringOut (text);
           CI_StringOut (" - duration ");
           itoa (wValue_msb,text);
           CI_StringOut (text);
           CI_StringOut ("\r\n");
//           CI_StringOut ("C");
           usb_hid_set_idle(wValue_lsb, wValue_msb);
           return TRUE;

         case HID_SET_PROTOCOL:
           CI_StringOut ("Get HID_SET_REPORT\r\n");
//           CI_StringOut ("D");
           // TODO
           break;
      }
   }

   if( USB_SETUP_GET_CLASS_INTER == type )
   {
      switch( request )
      {
         case GET_MAX_LUN:
           // wValue must be 0
           // wIndex = Interface
           if( (0!=wValue_lsb) || (0!=wValue_msb) )
              break;
           wInterface=Usb_read_endpoint_data(EP_CONTROL, 16);
  #ifdef USB_MSD
           if( INTERFACE_NB != wInterface )
  #endif //  USB_MSD
              break;

           Usb_ack_setup_received_free();
           Usb_reset_endpoint_fifo_access(EP_CONTROL);
           Usb_write_endpoint_data(EP_CONTROL, 8, get_nb_lun() - 1);
           Usb_ack_control_in_ready_send();
           while (!Is_usb_control_in_ready());

           while(!Is_usb_control_out_received());
           Usb_ack_control_out_received_free();

           ms_multiple_drive = TRUE;
           return TRUE;

// HID
         case HID_GET_REPORT:
           // TODO
           switch (wValue_msb)
           {
             case HID_REPORT_FEATURE:
//               CI_StringOut ("Get HID_GET_REPORT_FEATURE\r\n");
//               CI_StringOut ("F");
               Keyboard_GetReport_Feature (KEYBOARD_FEATURE_COUNT);
               break;
             case HID_REPORT_INPUT:
               CI_StringOut ("Get HID_GET_REPORT_INPUT\r\n");
               {
                 U8 b_usb_report[10];

                 b_usb_report[0] = 0;
                 hid_get_descriptor (1, b_usb_report);
               }
               break;
             default:
               CI_StringOut ("Get HID_GET_REPORT unknown\r\n");
               break;
           }
           break;
         case HID_GET_IDLE:
           CI_StringOut ("Get HID_GET_IDLE\r\n");
           usb_hid_get_idle(wValue_lsb);
           return TRUE;
/*
      case GET_PROTOCOL:
        CopyRoutine = Keyboard_GetProtocolValue;
        break;

 */
         case HID_GET_PROTOCOL:
           CI_StringOut ("Get HID_GET_PROTOCOL\r\n");
           // TODO
           break;
      }
      
   }
   
   return FALSE;
}


//! This function returns the size and the pointer on a user information
//! structure
//!
Bool usb_user_get_descriptor(U8 type, U8 string)
{
  pbuffer = NULL;

  switch (type)
  {
  case STRING_DESCRIPTOR:
    switch (string)
    {
    case LANG_ID:
      data_to_transfer = sizeof(usb_user_language_id);
      pbuffer = &usb_user_language_id;
      break;

    case MAN_INDEX:
      data_to_transfer = sizeof(usb_user_manufacturer_string_descriptor);
      pbuffer = &usb_user_manufacturer_string_descriptor;
      break;

    case PROD_INDEX:
      data_to_transfer = sizeof(usb_user_product_string_descriptor);
      pbuffer = &usb_user_product_string_descriptor;
      break;

    case SN_INDEX:
      data_to_transfer = sizeof(usb_user_serial_number);
      pbuffer = &usb_user_serial_number;
      break;

    default:
      break;
    }
    break;

  default:
    break;
  }

  return pbuffer != NULL;
}

void USB_KB_SendDataToUSB(void)
{
  int i;

  while (Is_usb_endpoint_stall_requested(EP_KB_IN))
  {
    if (Is_usb_setup_received()) usb_process_request();
  }

  // MSC Compliance - Free BAD out receive during SCSI command
  while( Is_usb_out_received(EP_CCID_OUT) ) {
    Usb_ack_out_received_free(EP_CCID_OUT);
  }

  while (!Is_usb_in_ready(EP_KB_IN))
  {
     if(!Is_usb_endpoint_enabled(EP_KB_IN))
     {
       i = 0; // todo USB Reset
     }
  }

  Usb_reset_endpoint_fifo_access(EP_KB_IN);

  Usb_write_endpoint_data(EP_KB_IN, 8, 'D');
  Usb_write_endpoint_data(EP_KB_IN, 8, 'D');
  Usb_write_endpoint_data(EP_KB_IN, 8, 'D');
  Usb_write_endpoint_data(EP_KB_IN, 8, 'D');

  Usb_ack_in_ready_send(EP_KB_IN);


  // MSC Compliance - Wait end of all transmitions on USB line
  while( 0 != Usb_nb_busy_bank(EP_KB_IN) )
  {
    if (Is_usb_setup_received()) usb_process_request();
  }
}

/*
void usb_hid_set_report_feature_new(void)
{

   Usb_ack_setup_received_free();
   Usb_ack_control_in_ready_send();// send a ZLP

   while(!Is_usb_control_out_received());

   if(Usb_read_endpoint_data(EP_CONTROL, 8)==0x55)
      if(Usb_read_endpoint_data(EP_CONTROL, 8)==0xAA)
         if(Usb_read_endpoint_data(EP_CONTROL, 8)==0x55)
            if(Usb_read_endpoint_data(EP_CONTROL, 8)==0xAA)
            {
               jump_bootloader=1; // Specific Request with 0x55AA55AA code
            }
   Usb_ack_control_out_received_free();
   Usb_ack_control_in_ready_send();    //!< send a ZLP for STATUS phase
   while(!Is_usb_control_in_ready());
}
*/
#endif  // USB_DEVICE_FEATURE == ENABLED
