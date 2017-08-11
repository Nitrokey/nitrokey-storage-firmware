/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/* This file is prepared for Doxygen automatic documentation generation. */
/* ! \file ********************************************************************* \brief Management of the SCSI decoding. This file manages the SCSI
   decoding. - Compiler: IAR EWAVR32 and GNU GCC for AVR32 - Supported devices: All AVR32 devices with a USB module can be used. - AppNote: \author
   Atmel Corporation: http://www.atmel.com \n Support and FAQ: http://support.atmel.no/
   **************************************************************************** */

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

// _____ I N C L U D E S ___________________________________________________

#include "conf_usb.h"


#if USB_DEVICE_FEATURE == ENABLED

#include "scsi_decoder.h"
#include "usb_drv.h"
#include "ctrl_access.h"

#include "sd_mmc_mci.h"
#include "tools.h"

// _____ D E C L A R A T I O N S ____________________________________________

extern U8 usb_LUN;
extern U8 ms_endpoint;


/* ! \brief Writes the header of the MODE SENSE (6) and (10) commands. \param b_sense_10 Boolean indicating whether the (10) version of the command
   is requested: \arg \c TRUE to specify a MODE SENSE (10) command; \arg \c FALSE to specify a MODE SENSE (6) command. \param u8_data_length Data
   length in bytes. */
static void sbc_header_mode_sense (Bool b_sense_10, U8 u8_data_length);

/* ! \brief Writes the Error Recovery mode page. \param length Allocated data length in bytes. */
static void send_read_write_error_recovery_page (U8 length);

/* ! \brief Writes the Informational Exceptions Control mode page. */
static void send_informational_exceptions_page (void);


// _____ D E F I N I T I O N S ______________________________________________

U8 g_scsi_command[16];
U8 g_scsi_status;
U32 g_scsi_data_remaining;

s_scsi_sense g_scsi_sense[2];

U8 g_scsi_ep_ms_in;
U8 g_scsi_ep_ms_out;

// ! INQUIRY data.
static const sbc_st_std_inquiry_data sbc_std_inquiry_data = {
    // Byte 0: 0x00.
    0,  // PeripheralQualifier: Currently connected.
    0,  // DeviceType: Direct-access device.

    // Byte 1: 0x80.
    1,  // RMB: Medium is removable (this bit must be at 1, else the medium isn't seen by Windows).
    0,  // Reserved1.

    // Byte 2: 0x00.
    0,  // Version: Device not compliant to any standard.

    // Byte 3: 0x02.
    0,  // AERC.
    0,  // Obsolete0.
    0,  // NormACA.
    2,  // Response data format.

    // Byte 4: 0x1F.
    // Byte 5: 0x00.
    // Byte 6: 0x00.
    // Reserved4[3].
    {
     0x1F,  // Additional Length (n - 4).
     0, // SCCS: SCC supported.
     0},

    // Byte 7: 0x00.
    0,  // RelativeAddressing.
    0,  // Wide32Bit.
    0,  // Wide16Bit.
    0,  // Synchronous.
    0,  // LinkedCommands.
    0,  // Reserved5.
    0,  // CommandQueue.
    0,  // SoftReset.

    SBC_VENDOR_ID,

    SBC_PRODUCT_ID,

    SBC_REVISION_ID
};

extern int sd_mmc_mci_test_unit_only_local_access;

extern S32 xTickCount;

#include "global.h"

// #define DEBUG_SCSI_IO

#ifdef DEBUG_SCSI_IO
int CI_StringOut (char* szText);
#else
#define CI_LocalPrintf(...)
#define CI_TickLocalPrintf(...)
#define CI_StringOut(...)
#endif



void Sbc_build_sense (U8 lun, U8 skey, U8 sasc, U8 sascq)
{
    switch (lun)
    {
        case 0:    // Uncrypted volume
            g_scsi_sense[0].key = skey;
            g_scsi_sense[0].asc = sasc;
            g_scsi_sense[0].ascq = sascq;
            break;
        case 1:    // Encrypted or hidden volume
            g_scsi_sense[1].key = skey;
            g_scsi_sense[1].asc = sasc;
            g_scsi_sense[1].ascq = sascq;
            break;
    }
}

/*******************************************************************************

  DebugScsiCommand

*******************************************************************************/


#ifdef DEBUG_SCSI_IO


void DebugScsiCommand (U8 * ScsiCommand_pu8)
{
static U8 LastScsiCommand_u8 = 255;
static U8 LastLun_u8 = 255;
U8 Text_u8[20];


    if ((ScsiCommand_pu8[0] != LastScsiCommand_u8) || (LastLun_u8 != usb_LUN))
    {
        itoa ((S32) xTickCount / 2, Text_u8);   // Tick = 0,5 ms
        CI_StringOut (Text_u8);
        CI_StringOut (" - ");

        switch (ScsiCommand_pu8[0]) // Check received command.
        {
            case SBC_CMD_TEST_UNIT_READY:  // 0x00 - Mandatory.
                CI_StringOut ("SCSI: TEST_UNIT_READY");
                break;
            case SBC_CMD_REQUEST_SENSE:    // 0x03 - Mandatory.
                CI_StringOut ("SCSI: REQUEST_SENSE");
                break;
            case SBC_CMD_INQUIRY:  // 0x12 - Mandatory.
                CI_StringOut ("SCSI: INQUIRY");
                break;
            case SBC_CMD_MODE_SENSE_6: // 0x1A - Optional.
                CI_StringOut ("SCSI: MODE_SENSE_6");
                break;
            case SBC_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL: // 0x1E - Optional.
                CI_StringOut ("SCSI: PREVENT_ALLOW_MEDIUM_REMOVAL");
                break;
            case SBC_CMD_READFORMAT_CAPACITIES:    // 0x23 - Optional.
                CI_StringOut ("SCSI: SBC_CMD_READFORMAT_CAPACITIES");
                break;
            case SBC_CMD_READ_CAPACITY_10: // 0x25 - Mandatory.
                CI_StringOut ("SCSI: READ_CAPACITY_10");
                break;
            case SBC_CMD_READ_10:  // 0x28 - Mandatory.
                CI_StringOut ("SCSI: READ_10");
                break;
            case SBC_CMD_WRITE_10: // 0x2A - Optional.
                CI_StringOut ("SCSI: WRITE_10");
                break;
            case SBC_CMD_VERIFY_10:    // 0x2F - Optional.
                CI_StringOut ("SCSI: VERIFY_10");
                break;
            case SBC_CMD_MODE_SENSE_10:    // 0x5A - Optional.
                CI_StringOut ("SCSI: MODE_SENSE_10");
                break;
            case SBC_CMD_START_STOP_UNIT:  // 0x1B - Ignored. This command is used by the Linux 2.4 kernel,
                CI_StringOut ("SCSI: START_STOP_UNIT");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_FORMAT_UNIT:  // 0x04 - Mandatory.
                CI_StringOut ("SCSI: FORMAT_UNIT");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_REASSIGN_BLOCKS:  // 0x07 - Optional.
                CI_StringOut ("SCSI: REASSIGN_BLOCKS");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_READ_6:   // 0x08 - Mandatory.
                CI_StringOut ("SCSI: READ_6");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_6:  // 0x0A - Optional.
                CI_StringOut ("SCSI: WRITE_6");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_MODE_SELECT_6:    // 0x15 - Optional.
                CI_StringOut ("SCSI: MODE_SELECT_6");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_RECEIVE_DIAGNOSTIC_RESULTS:   // 0x1C - Optional.
                CI_StringOut ("SCSI: RECEIVE_DIAGNOSTIC_RESULTS");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_SEND_DIAGNOSTIC:  // 0x1D - Mandatory.
                CI_StringOut ("SCSI: SEND_DIAGNOSTIC");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_AND_VERIFY_10:  // 0x2E - Optional.
                CI_StringOut ("SCSI: WRITE_AND_VERIFY_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_PREFETCH_10:  // 0x34 - Optional.
                CI_StringOut ("SCSI: PREFETCH_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_SYNCHRONIZE_CACHE_10: // 0x35 - Optional.
                CI_StringOut ("SCSI: SYNCHRONIZE_CACHE_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_READ_DEFECT_DATA_10:  // 0x37 - Optional.
                CI_StringOut ("SCSI: READ_DEFECT_DATA_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_BUFFER: // 0x3B - Optional.
                CI_StringOut ("SCSI: WRITE_BUFFER");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_READ_BUFFER:  // 0x3C - Optional.
                CI_StringOut ("SCSI: READ_BUFFER");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_READ_LONG_10: // 0x3E - Optional.
                CI_StringOut ("SCSI: READ_LONG_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_LONG_10:    // 0x3F - Optional.
                CI_StringOut ("SCSI: WRITE_LONG_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_SAME_10:    // 0x41 - Optional.
                CI_StringOut ("SCSI: WRITE_SAME_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_LOG_SELECT:   // 0x4C - Optional.
                CI_StringOut ("SCSI: LOG_SELECT");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_LOG_SENSE:    // 0x4D - Optional.
                CI_StringOut ("SCSI: LOG_SENSE");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_XDWRITE_10:   // 0x50 - Optional.
                CI_StringOut ("SCSI: XDWRITE_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_XPWRITE_10:   // 0x51 - Optional.
                CI_StringOut ("SCSI: XPWRITE_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_XDREAD_10:    // 0x52 - Optional.
                CI_StringOut ("SCSI: XDREAD_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_XDWRITEREAD_10:   // 0x53 - Optional.
                CI_StringOut ("SCSI: XDWRITEREAD_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_MODE_SELECT_10:   // 0x55 - Optional.
                CI_StringOut ("SCSI: MODE_SELECT_10");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_PERSISTENT_RESERVE_IN:    // 0x5E - Optional.
                CI_StringOut ("SCSI: PERSISTENT_RESERVE_IN");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_PERSISTENT_RESERVE_OUT:   // 0x5F - Optional.
                CI_StringOut ("SCSI: PERSISTENT_RESERVE_OUT");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_EXTENDED_COPY:    // 0x83 - Optional.
                CI_StringOut ("SCSI: EXTENDED_COPY");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_RECEIVE_COPY_RESULTS: // 0x84 - Optional.
                CI_StringOut ("SCSI: RECEIVE_COPY_RESULTS");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_ACCESS_CONTROL_IN:    // 0x86 - Optional.
                CI_StringOut ("SCSI: ACCESS_CONTROL_IN");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_ACCESS_CONTROL_OUT:   // 0x87 - Optional.
                CI_StringOut ("SCSI: ACCESS_CONTROL_OUT");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_READ_16:  // 0x88 - Optional.
                CI_StringOut ("SCSI: READ_16");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_16: // 0x8A - Optional.
                CI_StringOut ("SCSI: WRITE_16");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_READ_ATTRIBUTE:   // 0x8C - Optional.
                CI_StringOut ("SCSI: READ_ATTRIBUTE");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_ATTRIBUTE:  // 0x8D - Optional.
                CI_StringOut ("SCSI: WRITE_ATTRIBUTE");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_AND_VERIFY_16:  // 0x8E - Optional.
                CI_StringOut ("SCSI: WRITE_AND_VERIFY_16");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_VERIFY_16:    // 0x8F - Optional.
                CI_StringOut ("SCSI: VERIFY_16");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_PREFETCH_16:  // 0x90 - Optional.
                CI_StringOut ("SCSI: PREFETCH_16");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_SYNCHRONIZE_CACHE_16: // 0x91 - Optional.
                CI_StringOut ("SCSI: SYNCHRONIZE_CACHE_16");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_SAME_16:    // 0x93 - Optional.
                CI_StringOut ("SCSI: WRITE_SAME_16");
                break;
            case SBC_CMD_SERVICE_ACTION_IN_16: // 0x9E
                CI_StringOut ("SCSI: SBC_CMD_SERVICE_ACTION_IN_16");
                break;
            case SBC_CMD_REPORT_LUNS:  // 0xA0 - Mandator.
                CI_StringOut ("SCSI: REPORT_LUNS");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_ATA_PASS_THROUGH: // 0xA1
                CI_StringOut ("SCSI: SBC_CMD_ATA_PASS_THROUGH");
                break;
            case SBC_CMD_READ_12:  // 0xA8 - Optional.
                CI_StringOut ("SCSI: READ_12");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_12: // 0xAA - Optional.
                CI_StringOut ("SCSI: WRITE_12");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_WRITE_AND_VERIFY_12:  // 0xAE - Optional.
                CI_StringOut ("SCSI: WRITE_AND_VERIFY_12");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_VERIFY_12:    // 0xAF - Optional.
                CI_StringOut ("SCSI: VERIFY_12");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            case SBC_CMD_READ_DEFECT_DATA_12:  // 0xB7 - Optional.
                CI_StringOut ("SCSI: READ_DEFECT_DATA_12");
                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
            default:
                CI_StringOut ("SCSI: *** UNKOWN *** ");
                itoa ((U32) ScsiCommand_pu8[0], Text_u8);
                CI_StringOut (Text_u8);
                CI_StringOut (" ");

                break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.
        }
        switch (usb_LUN)
        {
            case 0:
                CI_StringOut (" - UNCYP L0\r\n");
                break;
            case 1:
                CI_StringOut (" - ENCYP L1\r\n");
                break;
            default:
                CI_StringOut (" - *** ERROR LUN ***\r\n");
                break;
        }
        /*
           CI_StringOut (" - Lun "); Text_u8[0] = '0' + usb_LUN; Text_u8[1] = 0; CI_StringOut (Text_u8); CI_StringOut ("\r\n"); */
        DelayMs (10);
    }
    else
    {
        CI_StringOut ("*");
    }

    LastScsiCommand_u8 = ScsiCommand_pu8[0];
    LastLun_u8 = usb_LUN;
}
#else
#define DebugScsiCommand(...)
#endif // DEBUG_SCSI_IO

extern unsigned long long LastLunAccessInTick_u64[2];
unsigned long long TIME_MEASURING_GetTime (void);

Bool scsi_decode_command (void)
{
    Bool status = FALSE;

    if (TRUE == sd_mmc_mci_test_unit_only_local_access)
    {
      switch (g_scsi_command[0])
      {
        case SBC_CMD_READ_10:       // 0x28 - Mandatory.
        case SBC_CMD_WRITE_10:      // 0x2A - Optional.
        case SBC_CMD_VERIFY_10:     // 0x2F - Optional.
          CI_StringOut ("SCSI: no external access\r\n");
          sbc_lun_status_is_not_present ();
          return (FALSE);
      }
    }

    // Log LUN activity
    if (2 > usb_LUN)
    {
        LastLunAccessInTick_u64[usb_LUN] = TIME_MEASURING_GetTime ();
    }

    DebugScsiCommand (g_scsi_command);

    switch (g_scsi_command[0])  // Check received command.
    {
        case SBC_CMD_TEST_UNIT_READY:  // 0x00 - Mandatory.
            DelayMs (2);    // To avoid USB host error
            status = sbc_test_unit_ready ();
            break;

        case SBC_CMD_REQUEST_SENSE:    // 0x03 - Mandatory.
            status = sbc_request_sense ();
            break;

        case SBC_CMD_INQUIRY:  // 0x12 - Mandatory.
            status = sbc_inquiry ();
            break;

        case SBC_CMD_MODE_SENSE_6: // 0x1A - Optional.
            status = sbc_mode_sense (FALSE);
            break;

        case SBC_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL: // 0x1E - Optional.
            status = sbc_prevent_allow_medium_removal ();
            break;

        case SBC_CMD_READFORMAT_CAPACITIES:    // 0x23 - Optional.
            status = sbc_readformat_capacity ();
            break;

        case SBC_CMD_READ_CAPACITY_10: // 0x25 - Mandatory.
            status = sbc_read_capacity ();
            break;

        case SBC_CMD_READ_10:  // 0x28 - Mandatory.
            Scsi_start_read_action ();
            status = sbc_read_10 ();
            Scsi_stop_read_action ();
            break;

        case SBC_CMD_WRITE_10: // 0x2A - Optional.
            Scsi_start_write_action ();
            status = sbc_write_10 ();
            Scsi_stop_write_action ();
            break;

        case SBC_CMD_VERIFY_10:    // 0x2F - Optional.
            status = TRUE;
            sbc_lun_status_is_good ();
            break;
            /*
               case SBC_CMD_MODE_SENSE_10: // 0x5A - Optional. status = sbc_mode_sense(TRUE); break; */
        case SBC_CMD_START_STOP_UNIT:  // 0x1B - Ignored. This command is used by the Linux 2.4 kernel,
            status = TRUE;

            sbc_lun_status_is_good ();
            {
    U8 Text[10];
                CI_StringOut ("SCSI: START_STOP_UNIT - LUN ");
                Text[0] = usb_LUN + '0';
                Text[1] = 0;
                CI_StringOut (Text);
                CI_StringOut (" - Start bit ");
                Text[0] = (g_scsi_command[4] & 0x01) + '0';
                CI_StringOut (Text);
                CI_StringOut ("\r\n");
            }
/*
            if (0 == (g_scsi_command[4] & 0x01))
            {
                SetSdUncryptedCardEnableState (FALSE);  // Close all volumes
                SetSdEncryptedCardEnableState (FALSE);
                SetSdEncryptedHiddenState (FALSE);
            }
            else
            {
                SetSdUncryptedCardEnableState (TRUE);   // Open only the unencrypted volume
            }
*/
            break;  // for which we can not reply INVALID COMMAND, otherwise the disk will not mount.

        case SBC_CMD_SYNCHRONIZE_CACHE_10: // 0x35 - Optional.
            status = TRUE;
            sbc_lun_status_is_good ();
            break;

        case SBC_CMD_FORMAT_UNIT:  // 0x04 - Mandatory.
        case SBC_CMD_REASSIGN_BLOCKS:  // 0x07 - Optional.
        case SBC_CMD_READ_6:   // 0x08 - Mandatory.
        case SBC_CMD_WRITE_6:  // 0x0A - Optional.
        case SBC_CMD_MODE_SELECT_6:    // 0x15 - Optional.
        case SBC_CMD_RECEIVE_DIAGNOSTIC_RESULTS:   // 0x1C - Optional.
        case SBC_CMD_SEND_DIAGNOSTIC:  // 0x1D - Mandatory.
        case SBC_CMD_WRITE_AND_VERIFY_10:  // 0x2E - Optional.
        case SBC_CMD_PREFETCH_10:  // 0x34 - Optional.
//        case SBC_CMD_SYNCHRONIZE_CACHE_10: // 0x35 - Optional.
        case SBC_CMD_READ_DEFECT_DATA_10:  // 0x37 - Optional.
        case SBC_CMD_WRITE_BUFFER: // 0x3B - Optional.
        case SBC_CMD_READ_BUFFER:  // 0x3C - Optional.
        case SBC_CMD_READ_LONG_10: // 0x3E - Optional.
        case SBC_CMD_WRITE_LONG_10:    // 0x3F - Optional.
        case SBC_CMD_WRITE_SAME_10:    // 0x41 - Optional.
        case SBC_CMD_LOG_SELECT:   // 0x4C - Optional.
        case SBC_CMD_LOG_SENSE:    // 0x4D - Optional.
        case SBC_CMD_XDWRITE_10:   // 0x50 - Optional.
        case SBC_CMD_XPWRITE_10:   // 0x51 - Optional.
        case SBC_CMD_XDREAD_10:    // 0x52 - Optional.
        case SBC_CMD_XDWRITEREAD_10:   // 0x53 - Optional.
        case SBC_CMD_MODE_SELECT_10:   // 0x55 - Optional.
        case SBC_CMD_PERSISTENT_RESERVE_IN:    // 0x5E - Optional.
        case SBC_CMD_PERSISTENT_RESERVE_OUT:   // 0x5F - Optional.
        case SBC_CMD_EXTENDED_COPY:    // 0x83 - Optional.
        case SBC_CMD_RECEIVE_COPY_RESULTS: // 0x84 - Optional.
        case SBC_CMD_ACCESS_CONTROL_IN:    // 0x86 - Optional.
        case SBC_CMD_ACCESS_CONTROL_OUT:   // 0x87 - Optional.
        case SBC_CMD_READ_16:  // 0x88 - Optional.
        case SBC_CMD_WRITE_16: // 0x8A - Optional.
        case SBC_CMD_READ_ATTRIBUTE:   // 0x8C - Optional.
        case SBC_CMD_WRITE_ATTRIBUTE:  // 0x8D - Optional.
        case SBC_CMD_WRITE_AND_VERIFY_16:  // 0x8E - Optional.
        case SBC_CMD_VERIFY_16:    // 0x8F - Optional.
        case SBC_CMD_PREFETCH_16:  // 0x90 - Optional.
        case SBC_CMD_SYNCHRONIZE_CACHE_16: // 0x91 - Optional.
        case SBC_CMD_WRITE_SAME_16:    // 0x93 - Optional.
        case SBC_CMD_REPORT_LUNS:  // 0xA0 - Mandator.
        case SBC_CMD_READ_12:  // 0xA8 - Optional.
        case SBC_CMD_WRITE_12: // 0xAA - Optional.
        case SBC_CMD_WRITE_AND_VERIFY_12:  // 0xAE - Optional.
        case SBC_CMD_VERIFY_12:    // 0xAF - Optional.
        case SBC_CMD_READ_DEFECT_DATA_12:  // 0xB7 - Optional.
        default:
            // Command not supported.
            Sbc_send_failed ();
            Sbc_build_sense (usb_LUN, SBC_SENSE_KEY_ILLEGAL_REQUEST, SBC_ASC_INVALID_COMMAND_OPERATION_CODE, 0x00);
            break;
    }

    return status;
}


Bool sbc_test_unit_ready (void)
{
    switch (mem_test_unit_ready (usb_LUN))
    {
        case CTRL_GOOD:
            sbc_lun_status_is_good ();
            break;

        case CTRL_NO_PRESENT:
            sbc_lun_status_is_not_present ();
            break;

        case CTRL_BUSY:
            sbc_lun_status_is_busy_or_change ();
            break;

        case CTRL_FAIL:
        default:
            sbc_lun_status_is_fail ();
            break;
    }

    return TRUE;
}


Bool sbc_request_sense (void)
{
    // U32 i;
U8 allocation_length;
U8 request_sense_output[18];    // The maximal size of request is 17.


    allocation_length = min (g_scsi_command[4], sizeof (request_sense_output));

    if ((allocation_length != 0) && (2 > usb_LUN))
    {
        // Initialize the request sense data.
        request_sense_output[0] = SBC_RESPONSE_CODE_SENSE;  // 0x70.
        request_sense_output[1] = 0x00; // Obsolete.
        request_sense_output[2] = g_scsi_sense[usb_LUN].key;

        request_sense_output[3] = 0x00; // For direct access media, Information field.
        request_sense_output[4] = 0x00; // Give the unsigned logical block.
        request_sense_output[5] = 0x00; // Address associated with the sense key.
        request_sense_output[6] = 0x00;

        request_sense_output[7] = SBC_ADDITIONAL_SENSE_LENGTH;  // Device shall not adjust the additional sense length to reflect truncation.
        request_sense_output[8] = SBC_COMMAND_SPECIFIC_INFORMATION_3;
        request_sense_output[9] = SBC_COMMAND_SPECIFIC_INFORMATION_2;
        request_sense_output[10] = SBC_COMMAND_SPECIFIC_INFORMATION_1;
        request_sense_output[11] = SBC_COMMAND_SPECIFIC_INFORMATION_0;

        request_sense_output[12] = g_scsi_sense[usb_LUN].asc;
        request_sense_output[13] = g_scsi_sense[usb_LUN].ascq;

        request_sense_output[14] = SBC_FIELD_REPLACEABLE_UNIT_CODE;
        request_sense_output[15] = SBC_SENSE_KEY_SPECIFIC_2;
        request_sense_output[16] = SBC_SENSE_KEY_SPECIFIC_1;
        request_sense_output[17] = SBC_SENSE_KEY_SPECIFIC_0;

        // Send the request data.
        Usb_reset_endpoint_fifo_access (g_scsi_ep_ms_in);
        usb_write_ep_txpacket (g_scsi_ep_ms_in, request_sense_output, allocation_length, NULL);
        Sbc_valid_write_usb (allocation_length);
        // MSC Compliance - Wait end of all transmitions on USB line, because a stall may be send after data
        int LoopCounter = 0;
        while (0 != Usb_nb_busy_bank (EP_MS_IN))
        {
          LoopCounter++;
          if (1000000 < LoopCounter)
          {
            break;
          }
        }
    }

    sbc_lun_status_is_good ();

    return (allocation_length <= g_scsi_command[4]);
}


Bool sbc_inquiry (void)
{
U8 allocation_length;

    DelayMs (2);

    // CMDT or EPVD bit is not 0 or PAGE or OPERATION CODE fields != 0x00.
    if ((g_scsi_command[1] & 0x03) || g_scsi_command[2])
    {
        sbc_lun_status_is_cdb_field ();
        return FALSE;
    }

    // Send standard INQUIRY data (bytes 0 to (allocation_length - 1)).
    allocation_length = min (g_scsi_command[4], sizeof (sbc_st_std_inquiry_data));
    if (allocation_length != 0)
    {
        while (!Is_usb_in_ready (g_scsi_ep_ms_in))
        {
            if (!Is_usb_endpoint_enabled (g_scsi_ep_ms_in))
                return FALSE;   // USB Reset
        }
        Usb_reset_endpoint_fifo_access (g_scsi_ep_ms_in);
        usb_write_ep_txpacket (g_scsi_ep_ms_in, &sbc_std_inquiry_data, allocation_length, NULL);
        Sbc_valid_write_usb (allocation_length);

        // MSC Compliance - Wait end of all transmitions on USB line, because a stall may be send after data
        int LoopCounter = 0;
        while (0 != Usb_nb_busy_bank (EP_MS_IN))
        {
          LoopCounter++;
          if (100000 < LoopCounter)
          {
            break;
          }
        }
    }
    sbc_lun_status_is_good ();

    if (allocation_length > g_scsi_command[4])  // Not enough space
    {
        return (FALSE);
    }

    return (TRUE);
}


Bool sbc_mode_sense (Bool b_sense_10)
{
U8 allocation_length = (b_sense_10) ? g_scsi_command[8] : g_scsi_command[4];

    // Switch for page code.
    switch (g_scsi_command[2] & SBC_MSK_PAGE_CODE)
    {
        case SBC_PAGE_CODE_READ_WRITE_ERROR_RECOVERY:
            sbc_header_mode_sense (b_sense_10, SBC_MODE_DATA_LENGTH_READ_WRITE_ERROR_RECOVERY);
            send_read_write_error_recovery_page (allocation_length);
            Sbc_valid_write_usb (SBC_MODE_DATA_LENGTH_READ_WRITE_ERROR_RECOVERY + 1);
            break;

            // Page Code: Informational Exceptions Control mode page.
        case SBC_PAGE_CODE_INFORMATIONAL_EXCEPTIONS:
            sbc_header_mode_sense (b_sense_10, SBC_MODE_DATA_LENGTH_INFORMATIONAL_EXCEPTIONS);
            send_informational_exceptions_page ();
            Sbc_valid_write_usb (SBC_MODE_DATA_LENGTH_INFORMATIONAL_EXCEPTIONS + 1);
            break;

        case SBC_PAGE_CODE_ALL:
            if (b_sense_10)
            {
                sbc_header_mode_sense (b_sense_10,
                                       (allocation_length <
                                        (SBC_MODE_DATA_LENGTH_CODE_ALL + 2)) ? (allocation_length - 2) : SBC_MODE_DATA_LENGTH_CODE_ALL);
            }
            else
            {
                sbc_header_mode_sense (b_sense_10,
                                       (allocation_length <
                                        (SBC_MODE_DATA_LENGTH_CODE_ALL + 1)) ? (allocation_length - 1) : SBC_MODE_DATA_LENGTH_CODE_ALL);
            }
            if (allocation_length == ((b_sense_10) ? 8 : 4))
            {
                Sbc_valid_write_usb (allocation_length);
                break;
            }
            // Send page by ascending order code.
            send_read_write_error_recovery_page (allocation_length);    // 12 bytes.
            if (allocation_length > 12)
            {
                send_informational_exceptions_page ();  // 12 bytes.
                Sbc_valid_write_usb (SBC_MODE_DATA_LENGTH_CODE_ALL + 1);
            }
            else
            {
                Sbc_valid_write_usb (allocation_length);
            }
            break;

        default:
            sbc_lun_status_is_cdb_field ();
            return FALSE;
    }

    sbc_lun_status_is_good ();

    return TRUE;
}


Bool sbc_prevent_allow_medium_removal (void)
{
    sbc_lun_status_is_good ();
    return TRUE;
}



Bool sbc_readformat_capacity (void)
{
  U32 mem_size_nb_sector;
  switch (mem_read_capacity (usb_LUN, &mem_size_nb_sector))
  {
    case CTRL_GOOD:
        Usb_reset_endpoint_fifo_access (g_scsi_ep_ms_in);

        // Capacity List Header                         4 byte
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 32, sbc_format_mcu_to_scsi_data (32, 0x08 + 0x08));

        // *** Maximum Capacity Descriptor ***          8 byte
        // Return nb block.
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 32, sbc_format_mcu_to_scsi_data (32, mem_size_nb_sector));

        // Return Descriptor Code
        Usb_write_endpoint_data (g_scsi_ep_ms_in,  8, 0x02);    // Formatted Media - Current media capacity

        // Return block size (= 512 bytes)
        Usb_write_endpoint_data (g_scsi_ep_ms_in,  8, 0x00);
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 16, Sbc_format_mcu_to_scsi_data (16, 512));

        // *** Formattable Capacity Descriptor ***      8 byte
        // Return nb block.
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 32, sbc_format_mcu_to_scsi_data (32, mem_size_nb_sector));

        // Return block size (= 512 bytes)
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 32, Sbc_format_mcu_to_scsi_data (32, 512));


        Sbc_valid_write_usb (0x04 + 0x08 + 0x08);
        sbc_lun_status_is_good ();
        return TRUE;

    case CTRL_NO_PRESENT:
        sbc_lun_status_is_not_present ();
        break;

    case CTRL_BUSY:
        sbc_lun_status_is_busy_or_change ();
        break;

    case CTRL_FAIL:
    default:
        sbc_lun_status_is_fail ();
        break;
  }
  return FALSE;
}

Bool sbc_read_capacity (void)
{
  U32 mem_size_nb_sector;

    switch (mem_read_capacity (usb_LUN, &mem_size_nb_sector))
    {
        case CTRL_GOOD:
            Usb_reset_endpoint_fifo_access (g_scsi_ep_ms_in);

            // Return nb block.
            Usb_write_endpoint_data (g_scsi_ep_ms_in, 32, sbc_format_mcu_to_scsi_data (32, mem_size_nb_sector));

            // Return block size (= 512 bytes).
            Usb_write_endpoint_data (g_scsi_ep_ms_in, 32, Sbc_format_mcu_to_scsi_data (32, 512));

            Sbc_valid_write_usb (SBC_READ_CAPACITY_LENGTH);
            sbc_lun_status_is_good ();
            return TRUE;

        case CTRL_NO_PRESENT:
            sbc_lun_status_is_not_present ();
            break;

        case CTRL_BUSY:
            sbc_lun_status_is_busy_or_change ();
            break;

        case CTRL_FAIL:
        default:
            sbc_lun_status_is_fail ();
            break;
    }
    return FALSE;
}


Bool sbc_read_10 (void)
{
U32 mass_addr;                  // Read/write block address.
U16 mass_size;                  // Read/write number of blocks.

    // Read address.
    MSB0W (mass_addr) = g_scsi_command[2];
    MSB1W (mass_addr) = g_scsi_command[3];
    MSB2W (mass_addr) = g_scsi_command[4];
    MSB3W (mass_addr) = g_scsi_command[5];

    // Read size.
    MSB (mass_size) = g_scsi_command[7];
    LSB (mass_size) = g_scsi_command[8];

    if (ms_endpoint == EP_MS_OUT)
    {
        sbc_lun_status_is_good ();
        return FALSE;
    }

    // No data to transfer.
    if (0 == g_scsi_data_remaining)
    {
        if (mass_size == (g_scsi_data_remaining / 512))
        {
            sbc_lun_status_is_good ();
        }
        else
        {
            sbc_lun_status_is_cdb_field ();
        }
        return TRUE;
    }

    switch (memory_2_usb (usb_LUN, mass_addr, g_scsi_data_remaining / 512))
    {
        case CTRL_GOOD:
            if (mass_size == (g_scsi_data_remaining / 512))
            {
                sbc_lun_status_is_good ();
            }
            else
            {
                sbc_lun_status_is_cdb_field ();
            }
            g_scsi_data_remaining = 0;
            return TRUE;

        case CTRL_NO_PRESENT:
            sbc_lun_status_is_not_present ();
            break;

        case CTRL_BUSY:
            sbc_lun_status_is_busy_or_change ();
            break;

        case CTRL_FAIL:
        default:
            sbc_lun_status_is_fail ();
            break;
    }

    return FALSE;
}


Bool sbc_write_10 (void)
{
U32 mass_addr;                  // Read/write block address.
U16 mass_size;                  // Read/write number of blocks.

    // Read address.
    MSB0W (mass_addr) = g_scsi_command[2];
    MSB1W (mass_addr) = g_scsi_command[3];
    MSB2W (mass_addr) = g_scsi_command[4];
    MSB3W (mass_addr) = g_scsi_command[5];

    // Read size.
    MSB (mass_size) = g_scsi_command[7];
    LSB (mass_size) = g_scsi_command[8];

    if (ms_endpoint == EP_MS_IN)
    {
        // Error in command field
        sbc_lun_status_is_cdb_field ();
        return FALSE;
    }

    // No data to transfer.
    if (0 == g_scsi_data_remaining)
    {
        if (mass_size == (g_scsi_data_remaining / 512))
        {
            sbc_lun_status_is_good ();
        }
        else
        {
            sbc_lun_status_is_cdb_field ();
        }
        return TRUE;
    }

    if (mem_wr_protect (usb_LUN))
    {
        sbc_lun_status_is_protected ();
        return TRUE;    // FALSE;
    }

    switch (usb_2_memory (usb_LUN, mass_addr, g_scsi_data_remaining / 512))
    {
        case CTRL_GOOD:
            if (mass_size == (g_scsi_data_remaining / 512))
            {
                sbc_lun_status_is_good ();
            }
            else
            {
                sbc_lun_status_is_cdb_field ();
            }
            g_scsi_data_remaining = 0;
            return TRUE;

        case CTRL_NO_PRESENT:
            sbc_lun_status_is_not_present ();
            break;

        case CTRL_BUSY:
            sbc_lun_status_is_busy_or_change ();
            break;

        case CTRL_FAIL:
        default:
            sbc_lun_status_is_fail ();
            break;
    }
    return FALSE;
}


static void sbc_header_mode_sense (Bool b_sense_10, U8 u8_data_length)
{
    Usb_reset_endpoint_fifo_access (g_scsi_ep_ms_in);

    // Send data length.
    if (b_sense_10)
    {
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0);
    }
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, u8_data_length);

    // Send device type.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_MEDIUM_TYPE);

    // Write protect status.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, (mem_wr_protect (usb_LUN)) ? SBC_DEV_SPEC_PARAM_WR_PROTECT : SBC_DEV_SPEC_PARAM_WR_ENABLE);

    if (b_sense_10)
    {
        // Reserved.
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 16, 0);
    }

    // Send block descriptor length.
    if (b_sense_10)
    {
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0);
    }
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_BLOCK_DESCRIPTOR_LENGTH);
}


static void send_read_write_error_recovery_page (U8 length)
{
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_PAGE_CODE_READ_WRITE_ERROR_RECOVERY);

    // Page Length.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_PAGE_LENGTH_READ_WRITE_ERROR_RECOVERY);
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0x80);
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_READ_RETRY_COUNT);
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0x00);
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0x00);
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0x00);
    // Reserved.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0x00);

    if (length > 12)
    {
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_WRITE_RETRY_COUNT);
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0x00);
        Usb_write_endpoint_data (g_scsi_ep_ms_in, 16, Sbc_format_mcu_to_scsi_data (16, SBC_RECOVERY_TIME_LIMIT));
    }
}


static void send_informational_exceptions_page (void)
{
    // Page Code: Informational Exceptions Control mode page.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_PAGE_CODE_INFORMATIONAL_EXCEPTIONS);

    // Page Length.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_PAGE_LENGTH_INFORMATIONAL_EXCEPTIONS);
    // ..., test bit = 0, ...
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, 0x00);
    // MRIE = 0x05.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 8, SBC_MRIE_GENERATE_NO_SENSE);
    // Interval Timer.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 32, Sbc_format_mcu_to_scsi_data (32, 0x00000000));
    // Report Count.
    Usb_write_endpoint_data (g_scsi_ep_ms_in, 32, Sbc_format_mcu_to_scsi_data (32, 0x00000001));
}


void sbc_lun_status_is_good (void)
{
    Sbc_send_good ();
    Sbc_build_sense (usb_LUN, SBC_SENSE_KEY_NO_SENSE, SBC_ASC_NO_ADDITIONAL_SENSE_INFORMATION, 0x00);
}


void sbc_lun_status_is_not_present (void)
{
    Sbc_send_failed ();
    Sbc_build_sense (usb_LUN, SBC_SENSE_KEY_NOT_READY, SBC_ASC_MEDIUM_NOT_PRESENT, 0x00);
}


void sbc_lun_status_is_busy_or_change (void)
{
    Sbc_send_failed ();
    Sbc_build_sense (usb_LUN, SBC_SENSE_KEY_UNIT_ATTENTION, SBC_ASC_NOT_READY_TO_READY_CHANGE, 0x00);
}


void sbc_lun_status_is_fail (void)
{
    Sbc_send_failed ();
    Sbc_build_sense (usb_LUN, SBC_SENSE_KEY_HARDWARE_ERROR, SBC_ASC_NO_ADDITIONAL_SENSE_INFORMATION, 0x00);
}


void sbc_lun_status_is_protected (void)
{
    Sbc_send_failed ();
    Sbc_build_sense (usb_LUN, SBC_SENSE_KEY_DATA_PROTECT, SBC_ASC_WRITE_PROTECTED, 0x00);
}


void sbc_lun_status_is_cdb_field (void)
{
    Sbc_send_failed ();
    Sbc_build_sense (usb_LUN, SBC_SENSE_KEY_ILLEGAL_REQUEST, SBC_ASC_INVALID_FIELD_IN_CDB, 0x00);
}


#endif // USB_DEVICE_FEATURE == ENABLED
