/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/* This file is prepared for Doxygen automatic documentation generation. */
/* ! \file ****************************************************************** \brief Management of the USB device mass-storage task. This file
   manages the USB device mass-storage task. - Compiler: IAR EWAVR32 and GNU GCC for AVR32 - Supported devices: All AVR32 devices with a USB module
   can be used. - AppNote: \author Atmel Corporation: http://www.atmel.com \n Support and FAQ: http://support.atmel.no/
   ************************************************************************* */

/* Copyright (c) 2009 Atmel Corporation. All rights reserved. Redistribution and use in source and binary forms, with or without modification, are
   permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimer. 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
   the following disclaimer in the documentation and/or other materials provided with the distribution. 3. The name of Atmel may not be used to
   endorse or promote products derived from this software without specific prior written permission. 4. This software may only be redistributed and
   used in connection with an Atmel AVR product. THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE EXPRESSLY AND SPECIFICALLY
   DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE */
/*
 * This file contains modifications done by Rudolf Boeddeker
 * For the modifications applies:
 *
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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


// _____ I N C L U D E S ___________________________________________________

#include "conf_usb.h"


#if USB_DEVICE_FEATURE == ENABLED

#include "board.h"
#ifdef FREERTOS_USED
#include "FreeRTOS.h"
#include "task.h"
#endif
#include "usb_drv.h"
#include "usb_descriptors.h"
#include "usb_standard_request.h"
#include "ctrl_access.h"
#include "scsi_decoder.h"
#include "device_mass_storage_task.h"


unsigned char ReadStickConfigurationFromUserPage (void);
unsigned char FAI_InitLun (unsigned char Lun_u8);

// _____ M A C R O S ________________________________________________________


// _____ D E F I N I T I O N S ______________________________________________

#define TIME_MEASURING_TICKS_IN_USEC              (FCPU_HZ/1000000ul)

#define DEBUG_SCSI_IO

#ifdef DEBUG_SCSI_IO
int CI_StringOut (char* szText);
#else
#define CI_LocalPrintf(...)
#define CI_TickLocalPrintf(...)
#define CI_StringOut(...)
#endif


// _____ D E C L A R A T I O N S ____________________________________________

extern volatile Bool ms_multiple_drive;

static U16 sof_cnt;
static U32 dCBWTag;

U8 usb_LUN;
U8 ms_endpoint;


static void usb_mass_storage_cbw (void);
static void usb_mass_storage_csw (void);


// !
// ! @brief This function initializes the hardware/software resources
// ! required for device mass-storage task.
// !
void device_mass_storage_task_init (void)
{
    g_scsi_ep_ms_in = EP_MS_IN;
    g_scsi_ep_ms_out = EP_MS_OUT;
    sof_cnt = 0;
#ifndef FREERTOS_USED
#if USB_HOST_FEATURE == ENABLED
    // If both device and host features are enabled, check if device mode is engaged
    // (accessing the USB registers of a non-engaged mode, even with load operations,
    // may corrupt USB FIFO data).
    if (Is_usb_device ())
#endif // USB_HOST_FEATURE == ENABLED
        Usb_enable_sof_interrupt ();
#endif // FREERTOS_USED

#ifdef FREERTOS_USED
    xTaskCreate (device_mass_storage_task, configTSK_USB_DMS_NAME, configTSK_USB_DMS_STACK_SIZE, NULL, configTSK_USB_DMS_PRIORITY, NULL);
#endif // FREERTOS_USED
}


// !
// ! @brief Entry point of the device mass-storage task management
// !
// ! This function links the device mass-storage SCSI commands to the USB bus.
// !

unsigned long long TIME_MEASURING_GetTime (void);
unsigned long long LastLunAccessInTick_u64[2] = { 0, 0 };

extern int sd_mmc_mci_test_unit_only_local_access;

#define MAX_TICKS_UNTIL_RESTART_MSD_INTERFACE           (5000ULL *(unsigned long long)TIME_MEASURING_TICKS_IN_USEC * 1000ULL)   // 5 sec
#define MAX_TICKS_STARTUP_UNTIL_RESTART_MSD_INTERFACE   (   0ULL *(unsigned long long)TIME_MEASURING_TICKS_IN_USEC * 1000ULL)   // no delay 0 sec

#ifdef FREERTOS_USED
void device_mass_storage_task (void* pvParameters)
#else
void device_mass_storage_task (void)
#endif
{
    unsigned int TickDelayToRestart = MAX_TICKS_UNTIL_RESTART_MSD_INTERFACE;
    int ErrorFound;
    int i;
    unsigned long long ActualTime_u64;
    static unsigned int LoopCounter_u32 = 0;

#ifdef FREERTOS_USED
    portTickType xLastWakeTime;

    for (i=0;i<2000;i++)
    {
      if (TRUE == ReadConfigurationSuccesfull())
      {
         break;
      }
      vTaskDelay (2);         // Wait max 2000 * 1 ms
    }

    FAI_InitLun (0);

    xLastWakeTime = xTaskGetTickCount ();
    while (TRUE)
    {
        vTaskDelayUntil (&xLastWakeTime, configTSK_USB_DMS_PERIOD);

        // First, check the device enumeration state
        if (!Is_device_enumerated ())
            continue;
#else
    // First, check the device enumeration state
    if (!Is_device_enumerated ())
        return;
#endif // FREERTOS_USED

    // If we receive something in the OUT endpoint, parse it
    if (Is_usb_out_received (EP_MS_OUT))
    {
        usb_mass_storage_cbw ();
        usb_mass_storage_csw ();
    }

    // Check LUN activity

    if (30 <= LoopCounter_u32)
    {
        ActualTime_u64 = TIME_MEASURING_GetTime ();
        if ((FALSE == sd_mmc_mci_test_unit_only_local_access)   // On local access > disable check
            // || (ActualTime_u64 > MAX_TICKS_STARTUP_UNTIL_RESTART_MSD_INTERFACE) // Not check on startup
            )
        {
            // TODO check: potential flaw in calculating of the time difference due to an overflow in watchdog mechanism
            // watchdog seems to be disabled
            ErrorFound = FALSE;
            if (ActualTime_u64 - LastLunAccessInTick_u64[0] > TickDelayToRestart)
            {
                CI_StringOut ("UNCRYPTED LUN 0 - TIMEOUT\r\n");
                ErrorFound = TRUE;
            }
            // Check LUN activity
            if (ActualTime_u64 - LastLunAccessInTick_u64[1] > TickDelayToRestart)
            {
                CI_StringOut ("ENCRYPTED LUN 1 - TIMEOUT\r\n");
                ErrorFound = TRUE;
            }
            /*
               if (TRUE == ErrorFound) { CI_StringOut ("*** RESTART MSD DEVICE TASK ***\r\n"); LastLunAccessInTick_u64[0] = ActualTime_u64;
               LastLunAccessInTick_u64[1] = ActualTime_u64; usb_device_task_delete(); usb_device_task_init(); } */
        }
        else
        {
            LastLunAccessInTick_u64[0] = ActualTime_u64;    // Avoid wrong timeout
            LastLunAccessInTick_u64[1] = ActualTime_u64;
        }
    }
    else
    {
        LoopCounter_u32 = 0;
    }


#ifdef FREERTOS_USED
}
#endif
}


// !
// ! @brief usb_sof_action
// !
// ! This function increments the sof_cnt counter each time
// ! the USB Start-of-Frame interrupt subroutine is executed (1 ms).
// ! Useful to manage time delays
// !
void usb_sof_action (void)
{
    sof_cnt++;
}


// !
// ! @brief USB Command Block Wrapper (CBW) management
// !
// ! This function decodes the CBW command and stores the SCSI command.
// !
static void usb_mass_storage_cbw (void)
{
    Bool cbw_error;

    Usb_reset_endpoint_fifo_access (EP_MS_OUT);

    // ! Check if dCBWSignature is correct
    cbw_error = (Usb_read_endpoint_data (EP_MS_OUT, 32) != *(U32 *) & "USBC");

    // ! Store CBW Tag to be repeated in CSW
    dCBWTag = Usb_read_endpoint_data (EP_MS_OUT, 32);

    g_scsi_data_remaining = Usb_read_endpoint_data (EP_MS_OUT, 32);
    g_scsi_data_remaining = usb_format_usb_to_mcu_data (32, g_scsi_data_remaining);

    /* Show the remaining bytes { U8 Text_u8[20]; CI_StringOut (" - "); itoa ((S32)g_scsi_data_remaining,Text_u8); CI_StringOut (Text_u8);
       CI_StringOut (" - "); } */
    // ! if (bmCBWFlags.bit7 == 1) {direction = IN;}
    if (Usb_read_endpoint_data (EP_MS_OUT, 8))
    {
        ms_endpoint = EP_MS_IN;
        if (cbw_error)
        {
            Usb_ack_out_received_free (EP_MS_OUT);
            Usb_enable_stall_handshake (EP_MS_IN);
            return;
        }
    }
    else
    {
        ms_endpoint = EP_MS_OUT;
        if (cbw_error)
        {
            Usb_enable_stall_handshake (EP_MS_OUT);
            Usb_ack_out_received_free (EP_MS_OUT);
            return;
        }
    }

    usb_LUN = Usb_read_endpoint_data (EP_MS_OUT, 8);

    if (!ms_multiple_drive)
    {
        usb_LUN = get_cur_lun ();
    }

    // ! Dummy CBWCBLength read
    Usb_read_endpoint_data (EP_MS_OUT, 8);

    // ! Store scsi_command
    usb_read_ep_rxpacket (EP_MS_OUT, g_scsi_command, sizeof (g_scsi_command), NULL);

    Usb_ack_out_received_free (EP_MS_OUT);

    if (!scsi_decode_command ())
    {
        Usb_enable_stall_handshake (ms_endpoint);
    }
}


// !
// ! @brief USB Command Status Wrapper (CSW) management
// !
// ! This function sends the status in relation with the last CBW.
// !
static void usb_mass_storage_csw (void)
{
    int i;
    /*
       i = 1; while (1 == i) { i = 0; if (Is_usb_endpoint_stall_requested(EP_MS_IN)) { i = 1; if (Is_usb_setup_received()) { usb_process_request(); }
       }

       if (Is_usb_endpoint_stall_requested(EP_MS_OUT)) { i = 1; if (Is_usb_setup_received()) { usb_process_request(); } } } */


    while (Is_usb_endpoint_stall_requested (EP_MS_IN))
    {
        if (Is_usb_setup_received ())
            usb_process_request ();
    }

    while (Is_usb_endpoint_stall_requested (EP_MS_OUT))
    {
        if (Is_usb_setup_received ())
            usb_process_request ();
    }


    // MSC Compliance - Free BAD out receiv during SCSI command
    while (Is_usb_out_received (EP_MS_OUT))
    {
        Usb_ack_out_received_free (EP_MS_OUT);
    }

    int LoopCounter = 0;
    while (!Is_usb_in_ready (EP_MS_IN))
    {
      LoopCounter++;
      if (100000 < LoopCounter)
      {
        break;
      }
    }

    /*
       // for (i=0;i<100000;i++) // Make break out { if (Is_usb_in_ready(EP_MS_IN)) { break; } } */
    Usb_reset_endpoint_fifo_access (EP_MS_IN);

    // ! Write CSW Signature
    Usb_write_endpoint_data (EP_MS_IN, 32, *(U32 *) & "USBS");

    // ! Write stored CBW Tag
    Usb_write_endpoint_data (EP_MS_IN, 32, dCBWTag);

    // ! Write data residual value
    Usb_write_endpoint_data (EP_MS_IN, 32, usb_format_mcu_to_usb_data (32, g_scsi_data_remaining));

    // ! Write command status
    Usb_write_endpoint_data (EP_MS_IN, 8, g_scsi_status);

    Usb_ack_in_ready_send (EP_MS_IN);

    // MSC Compliance - Wait end of all transmitions on USB line
    for (i = 0; i < 1000; i++)
    {
        if (0 != Usb_nb_busy_bank (EP_MS_IN))
        {
            if (Is_usb_setup_received ())
                usb_process_request ();
        }
        else
        {
            break;
        }
    }
    /*
       while( 0 != Usb_nb_busy_bank(EP_MS_IN) ) { if (Is_usb_setup_received()) usb_process_request(); } */
}


#endif // USB_DEVICE_FEATURE == ENABLED
