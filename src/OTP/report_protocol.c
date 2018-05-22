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

#include "global.h"
#include "tools.h"

#include "keyboard.h"
#include "hotp.h"
#include "report_protocol.h"
#include "string.h"
#include "time.h"
#include "stdlib.h"

#include "polarssl/timing.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"
#include "USB_CCID/USB_CCID.h"
#include "Tools/DFU_test.h"

#include "BUFFERED_SIO.h"
#include "Interpreter.h"

#include "HighLevelFunctions/FlashStorage.h"
#include "USER_INTERFACE/html_io.h"
#include "HighLevelFunctions/password_safe.h"
#include "hotp.h"


/*******************************************************************************

 Local defines

*******************************************************************************/


#define DEBUG_REPORT_PROTOCOL
// #define DEBUG_HID_USB_DATA

#ifdef DEBUG_REPORT_PROTOCOL
#else
#define CI_LocalPrintf(...)
#define CI_TickLocalPrintf(...)
#define CI_StringOut(...)
#define CI_Print8BitValue(...)
#define HexPrint(...)
#endif

// #define KEYBOARD_FEATURE_COUNT 64

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/
u8 sd_mmc_mci_read_capacity (u8 slot, u32 * u32_nb_sector);

/*******************************************************************************

 Local declarations

*******************************************************************************/

u8 HID_SetReport_Value[KEYBOARD_FEATURE_COUNT];
u8 HID_GetReport_Value[KEYBOARD_FEATURE_COUNT];

u8 HID_SetReport_Value_tmp[KEYBOARD_FEATURE_COUNT];
u8 HID_GetReport_Value_tmp[KEYBOARD_FEATURE_COUNT];

u8 OTP_device_status = STATUS_READY;
u8 temp_password[25];
u8 tmp_password_set = 0;
u32 authorized_crc = 0xFFFFFFFF;
u8 temp_user_password[25];
u8 tmp_user_password_set = 0;
u32 authorized_user_crc = 0xFFFFFFFF;
u32 authorized_user_crc_set=0;

#define HID_STRING_LEN      50

// For stick 2.0
u8 HID_CmdGet_u8 = HTML_CMD_NOTHING;
u8 HID_String_au8[HID_STRING_LEN];

HID_Stick20Status_est HID_Stick20Status_st;

HID_Stick20SendData_est HID_Stick20SendData_st;


#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID

/*******************************************************************************

  InitStick20DebugData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void InitStick20DebugData (u8 * output)
{
    HID_Stick20SendData_st.SendCounter_u8 = 0;
    HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_NONE;
    HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;  // False
    HID_Stick20SendData_st.SendSize_u8 = 0;

    memset (&HID_Stick20SendData_st.SendData_u8[0], 0, OUTPUT_CMD_STICK20_SEND_DATA_SIZE);

    memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START], (void *) &HID_Stick20SendData_st, sizeof (HID_Stick20SendData_est));
}

/*******************************************************************************

  Stick20HIDSendDebugData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void Stick20HIDSendDebugData (u8 * output)
{
u8 Size_u8;
u32 calculated_crc32;

    HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG;

    Size_u8 = BUFFERED_SIO_HID_GetSendChars (&HID_Stick20SendData_st.SendData_u8[0], OUTPUT_CMD_STICK20_SEND_DATA_SIZE);

    // Send buffer empty ?
    if (TRUE != BUFFERED_SIO_HID_TxEmpty ())
    {
        HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;  // No
    }
    else
    {
        HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;  // Yes
    }

    HID_Stick20SendData_st.SendCounter_u8 += 1;
    HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG;
    HID_Stick20SendData_st.SendSize_u8 = Size_u8;
    /*
       { u8 text_au8[20]; CI_StringOut ("<"); itoa (HID_Stick20SendData_st.SendCounter_u8,text_au8); CI_StringOut ((char*)text_au8); CI_StringOut
       (">"); } */
    memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START], (void *) &HID_Stick20SendData_st, sizeof (HID_Stick20SendData_est));

    // Build CRC, can overwritten by new CRC computation
    calculated_crc32 = generateCRC (output);

    output[OUTPUT_CRC_OFFSET] = calculated_crc32 & 0xFF;
    output[OUTPUT_CRC_OFFSET + 1] = (calculated_crc32 >> 8) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 2] = (calculated_crc32 >> 16) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 3] = (calculated_crc32 >> 24) & 0xFF;
}


#endif // STICK_20_SEND_DEBUGINFOS_VIA_HID

/*******************************************************************************

  Stick20HIDInitSendConfiguration

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

//extern volatile long xTickCount;

u8 Stick20HIDSendConfigurationState_u8 = STICK20_SEND_STATUS_IDLE;

u8 Stick20HIDInitSendConfiguration (u8 state_u8)
{
//  { u8 text_au8[20]; itoa (xTickCount/2,text_au8); CI_StringOut ((char*)text_au8); }
    CI_StringOut ("Send HID data\r\n");

    Stick20HIDSendConfigurationState_u8 = state_u8;

    return (TRUE);
}

/*******************************************************************************

  Stick20HIDSendAccessStatusData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 Stick20HIDSendAccessStatusData (u8 * output)
{
u32 calculated_crc32;
typeStick20Configuration_st Config_st;
u8 text_au8[10];

    SendStickStatusToHID (&Config_st);

    HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_STATUS;

    CI_StringOut ("Send configuration data\r\n");
    HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;  // No
    HID_Stick20SendData_st.SendSize_u8 = 1;


    CI_StringOut ("Firmware version  ");

    itoa (Config_st.VersionInfo_au8[0], text_au8);
    CI_StringOut ((char *) text_au8);
    CI_StringOut (".");
    itoa (Config_st.VersionInfo_au8[1], text_au8);
    CI_StringOut ((char *) text_au8);
    CI_StringOut ("\r\n");

    if (FALSE == Config_st.FirmwareLocked_u8)
    {
        CI_StringOut ("Firmware not locked\r\n");
    }
    else
    {
        CI_StringOut ("*** Firmware locked ***\r\n");
    }

    if (READ_WRITE_ACTIVE == Config_st.ReadWriteFlagUncryptedVolume_u8)
    {
        CI_StringOut ("Uncyrpted Volume  READ/WRITE active\r\n");
    }
    else
    {
        CI_StringOut ("Uncyrpted Volume  READ ONLY active\r\n");
    }

    if (STICK20_SEND_STATUS_PIN == Stick20HIDSendConfigurationState_u8) // This information only with pin
    {
        if (0 != (Config_st.VolumeActiceFlag_u8 & (1 << SD_CRYPTED_VOLUME_BIT_PLACE)))
        {
            CI_StringOut ("Crypted volume    active\r\n");
        }
        else
        {
            CI_StringOut ("Crypted volume    not active\r\n");
        }

        if (0 != (Config_st.VolumeActiceFlag_u8 & (1 << SD_HIDDEN_VOLUME_BIT_PLACE)))
        {
            CI_StringOut ("Hidden volume     active\r\n");
        }
    }


    if (0 != (Config_st.NewSDCardFound_u8 & 0x01))
    {
        CI_StringOut ("*** New SD card found - Change Counter:");
    }
    else
    {
        CI_StringOut ("SD card              - Change Counter:");
    }
    itoa ((Config_st.NewSDCardFound_u8 >> 1), text_au8);
    CI_StringOut ((char *) text_au8);
    CI_StringOut ("\r\n");

    if (0 == (Config_st.SDFillWithRandomChars_u8 & 0x01))
    {
        CI_StringOut ("Not filled with random chars - Change Counter:");
    }
    else
    {
        CI_StringOut ("Filled with random   - Change Counter:");
    }
    itoa ((Config_st.SDFillWithRandomChars_u8 >> 1), text_au8);
    CI_StringOut ((char *) text_au8);
    CI_StringOut ("\r\n");

    if (TRUE == Config_st.StickKeysNotInitiated_u8)
    {
        CI_StringOut ("*** Stick not initiated ***\r\n");
    }
    else
    {
        CI_StringOut ("Stick initiated\r\n");
    }

    CI_StringOut ("Password retry count - User ");
    itoa (Config_st.UserPwRetryCount, text_au8);
    CI_StringOut ((char *) text_au8);
    CI_StringOut (" - Admin ");
    itoa (Config_st.AdminPwRetryCount, text_au8);
    CI_StringOut ((char *) text_au8);
    CI_StringOut ("\r\n");

    CI_StringOut ("Smartcard serial nr: ");
    itoa (Config_st.ActiveSmartCardID_u32, text_au8);
    CI_StringOut ((char *) text_au8);
    CI_StringOut ("\r\n");

    CI_StringOut ("SD card serial nr: 0x");
    itoa_h (Config_st.ActiveSD_CardID_u32, text_au8);
    CI_StringOut ((char *) text_au8);
    CI_StringOut ("\r\n");

    // Set endian
    Config_st.ActiveSmartCardID_u32 = change_endian_u32 (Config_st.ActiveSmartCardID_u32);
    Config_st.ActiveSD_CardID_u32 = change_endian_u32 (Config_st.ActiveSD_CardID_u32);

    memcpy (&HID_Stick20SendData_st.SendData_u8[0], &Config_st, sizeof (typeStick20Configuration_st));


    HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;  // No next data
    HID_Stick20SendData_st.SendSize_u8 = sizeof (typeStick20Configuration_st);

    // Copy send data to transfer struct
    memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START], (void *) &HID_Stick20SendData_st, sizeof (HID_Stick20SendData_est));

    // Build CRC, can overwritten by new CRC computation
    calculated_crc32 = generateCRC (output);

    output[OUTPUT_CRC_OFFSET] = calculated_crc32 & 0xFF;
    output[OUTPUT_CRC_OFFSET + 1] = (calculated_crc32 >> 8) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 2] = (calculated_crc32 >> 16) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 3] = (calculated_crc32 >> 24) & 0xFF;

    Stick20HIDSendConfigurationState_u8 = STICK20_SEND_STATUS_IDLE; // Send status done

    return (TRUE);
}


/*******************************************************************************

  Stick20HIDSendProductionInfos

  Changes
  Date      Author          Info
  07.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 Stick20HIDSendProductionInfos (u8 * output)
{
u32 calculated_crc32;
typeStick20ProductionInfos_st Infos_st;

    HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_PROD_INFO;

    CI_StringOut ("Send production infos\r\n");
    HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;  // No
    HID_Stick20SendData_st.SendSize_u8 = 1;

    Infos_st = Stick20ProductionInfos_st;

    // Set endian
    Infos_st.CPU_CardID_u32 = change_endian_u32 (Infos_st.CPU_CardID_u32);
    Infos_st.SmartCardID_u32 = change_endian_u32 (Infos_st.SmartCardID_u32);
    Infos_st.SD_CardID_u32 = change_endian_u32 (Infos_st.SD_CardID_u32);

    Infos_st.SD_Card_OEM_u16 = change_endian_u16 (Infos_st.SD_Card_OEM_u16);
    Infos_st.SD_WriteSpeed_u16 = change_endian_u16 (Infos_st.SD_WriteSpeed_u16);


    memcpy (&HID_Stick20SendData_st.SendData_u8[0], &Infos_st, sizeof (typeStick20ProductionInfos_st));


    HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;  // No next data
    HID_Stick20SendData_st.SendSize_u8 = sizeof (typeStick20ProductionInfos_st);

    // Copy send data to transfer struct
    memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START], (void *) &HID_Stick20SendData_st, sizeof (HID_Stick20SendData_est));

    // Build CRC, can overwritten by new CRC computation
    calculated_crc32 = generateCRC (output);

    output[OUTPUT_CRC_OFFSET] = calculated_crc32 & 0xFF;
    output[OUTPUT_CRC_OFFSET + 1] = (calculated_crc32 >> 8) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 2] = (calculated_crc32 >> 16) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 3] = (calculated_crc32 >> 24) & 0xFF;

    Stick20HIDSendConfigurationState_u8 = STICK20_SEND_STATUS_IDLE; // Send status done

    return (TRUE);
}

/*******************************************************************************

  Stick20HIDInitSendMatrixData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 Stick20HIDSendMatrixState_u8 = 0;
u8* Stick20HIDSendMatrixDataBlock_au8 = NULL;

u8 Stick20HIDInitSendMatrixData (u8 * PasswordMatrixData_au8)
{
    CI_StringOut ("Init SendMatrix\r\n");

    Stick20HIDSendMatrixState_u8 = 1;
    Stick20HIDSendMatrixDataBlock_au8 = PasswordMatrixData_au8;

    return (TRUE);
}

/*******************************************************************************

  Stick20HIDSendMatrixData

  Password matrix HID Block

  Byte
  0       Status
          0 - IDEL
          1 - Start new Block
          2 - Data  0-19
          3 - Data 20-39
          4 - Data 40-59
          5 - Data 60-79
          6 - Data 80-99
          7 - All Data send, CRC
  1-20    Data or CRC

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 Stick20HIDSendMatrixData (u8 * output)
{
u32 calculated_crc32;

    HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_PW_DATA;

    switch (Stick20HIDSendMatrixState_u8)
    {
        case 0:    // IDLE - wait
            break;
        case 1:
            CI_StringOut ("SendMatrix S1\r\n");
            HID_Stick20SendData_st.SendData_u8[0] = 1;  // Set state in sendblock
            HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;  // Yes
            HID_Stick20SendData_st.SendSize_u8 = 1;
            Stick20HIDSendMatrixState_u8 = 2;   // Next state
            break;
        case 2:
            CI_StringOut ("SendMatrix S2\r\n");
            HID_Stick20SendData_st.SendData_u8[0] = 2;  // Set state in sendblock
            memcpy (&HID_Stick20SendData_st.SendData_u8[1], &Stick20HIDSendMatrixDataBlock_au8[0], 20);
            HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;  // Yes
            HID_Stick20SendData_st.SendSize_u8 = 21;
            Stick20HIDSendMatrixState_u8 = 3;   // Next state
            break;
        case 3:
            CI_StringOut ("SendMatrix S3\r\n");
            HID_Stick20SendData_st.SendData_u8[0] = 3;  // Set state in sendblock
            memcpy (&HID_Stick20SendData_st.SendData_u8[1], &Stick20HIDSendMatrixDataBlock_au8[20], 20);
            HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;  // Yes
            HID_Stick20SendData_st.SendSize_u8 = 21;
            Stick20HIDSendMatrixState_u8 = 4;   // Next state
            break;
        case 4:
            CI_StringOut ("SendMatrix S4\r\n");
            HID_Stick20SendData_st.SendData_u8[0] = 4;  // Set state in sendblock
            memcpy (&HID_Stick20SendData_st.SendData_u8[1], &Stick20HIDSendMatrixDataBlock_au8[40], 20);
            HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;  // Yes
            HID_Stick20SendData_st.SendSize_u8 = 21;
            Stick20HIDSendMatrixState_u8 = 5;   // Next state
            break;
        case 5:
            CI_StringOut ("SendMatrix S5\r\n");
            HID_Stick20SendData_st.SendData_u8[0] = 5;  // Set state in sendblock
            memcpy (&HID_Stick20SendData_st.SendData_u8[1], &Stick20HIDSendMatrixDataBlock_au8[60], 20);
            HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;  // Yes
            HID_Stick20SendData_st.SendSize_u8 = 21;
            Stick20HIDSendMatrixState_u8 = 6;   // Next state
            break;
        case 6:
            CI_StringOut ("SendMatrix S6\r\n");
            HID_Stick20SendData_st.SendData_u8[0] = 6;  // Set state in sendblock
            memcpy (&HID_Stick20SendData_st.SendData_u8[1], &Stick20HIDSendMatrixDataBlock_au8[80], 20);
            HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;  // Yes
            HID_Stick20SendData_st.SendSize_u8 = 1;
            Stick20HIDSendMatrixState_u8 = 7;   // Next state
            break;
        case 7:
            CI_StringOut ("SendMatrix S7\r\n");
            HID_Stick20SendData_st.SendData_u8[0] = 7;  // Set state in sendblock

            // Build CRC over matrix password data
            calculated_crc32 = generateCRC_len (Stick20HIDSendMatrixDataBlock_au8, 100 / 4);
            HID_Stick20SendData_st.SendData_u8[1] = calculated_crc32 & 0xFF;
            HID_Stick20SendData_st.SendData_u8[2] = (calculated_crc32 >> 8) & 0xFF;
            HID_Stick20SendData_st.SendData_u8[3] = (calculated_crc32 >> 16) & 0xFF;
            HID_Stick20SendData_st.SendData_u8[4] = (calculated_crc32 >> 24) & 0xFF;

            HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;  // No
            HID_Stick20SendData_st.SendSize_u8 = 5;
            Stick20HIDSendMatrixState_u8 = 0;   // Next state - idle
            break;
        default:
            break;
    }

    HID_Stick20SendData_st.SendCounter_u8 += 1;

    // Copy send data to transfer struct
    memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START], (void *) &HID_Stick20SendData_st, sizeof (HID_Stick20SendData_est));

    // Build CRC, can overwritten by new CRC computation
    calculated_crc32 = generateCRC (output);

    output[OUTPUT_CRC_OFFSET] = calculated_crc32 & 0xFF;
    output[OUTPUT_CRC_OFFSET + 1] = (calculated_crc32 >> 8) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 2] = (calculated_crc32 >> 16) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 3] = (calculated_crc32 >> 24) & 0xFF;

    return (TRUE);
}

/*******************************************************************************

  parse_report

  Changes
  Date      Author          Info
  11.02.14  RB              Implementation: New SD card found

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

  In USB context > don't use printf

*******************************************************************************/

void parse_report (u8 * report, u8 * output)
{
u8 cmd_type = report[CMD_TYPE_OFFSET];
u32 received_crc32;
u32 calculated_crc32;
u8 i;
u8 not_authorized = 0;

static u8 oldStatus;
static u8 initOldStatus = FALSE;

    if (FALSE == initOldStatus)
    {
        oldStatus = output[OUTPUT_CMD_STATUS_OFFSET];
        initOldStatus = TRUE;
    }

    // Init output block
    memset (output, 0, KEYBOARD_FEATURE_COUNT);

    // Set last command value
    output[OUTPUT_LAST_CMD_TYPE_OFFSET] = cmd_type;

    // Get CRC from HID report block
    received_crc32 = getu32 (report + KEYBOARD_FEATURE_COUNT - 4);

    // Check data with CRC
    calculated_crc32 = generateCRC (report);

    // Store the CRC as the CMD_CRC
    output[OUTPUT_CMD_CRC_OFFSET] = received_crc32 & 0xFF;
    output[OUTPUT_CMD_CRC_OFFSET + 1] = (received_crc32 >> 8) & 0xFF;
    output[OUTPUT_CMD_CRC_OFFSET + 2] = (received_crc32 >> 16) & 0xFF;
    output[OUTPUT_CMD_CRC_OFFSET + 3] = (received_crc32 >> 24) & 0xFF;


    if (calculated_crc32 != received_crc32)
    {
        CI_StringOut ("CRC Wrong\r\n");
        CI_StringOut ("Get     ");
        for (i = 0; i < 4; i++)
        {
            CI_Print8BitValue (output[OUTPUT_CMD_CRC_OFFSET + 3 - i]);
        }
        CI_StringOut ("\r\n");

        CI_StringOut ("Compute ");
        for (i = 0; i < 4; i++)
        {
            CI_Print8BitValue (output[OUTPUT_CMD_CRC_OFFSET + 3 - i]);
        }
        CI_StringOut ("\r\n");
    }


    // Stick 1.0 commands
    if (calculated_crc32 == received_crc32)
    {
        switch (cmd_type)
        {

            case CMD_GET_STATUS:
                CI_StringOut ("Get CMD_GET_STATUS\r\n");
                cmd_get_status (report, output);
                break;

            case CMD_WRITE_TO_SLOT:
            {
u8 text[10];
                CI_StringOut ("Get CMD_WRITE_TO_SLOT - slot ");
                itoa ((u8) report[1], text);
                CI_StringOut ((char *) text);
                CI_StringOut ("\r\n");
            }
                if (calculated_crc32 == authorized_crc)
                {
                    cmd_write_to_slot (report, output);
                }
                else
                {
                    not_authorized = 1;
                }
                break;

            case CMD_READ_SLOT_NAME:
            {
u8 text[10];
                CI_StringOut ("Get CMD_READ_SLOT_NAME - slot ");
                itoa ((u8) report[1], text);
                CI_StringOut ((char *) text);
                CI_StringOut ("\r\n");
            }
                cmd_read_slot_name (report, output);
                break;

            case CMD_READ_SLOT:
            {
u8 text[10];
                CI_StringOut ("Get CMD_READ_SLOT - slot ");
                itoa ((u8) report[1], text);
                CI_StringOut ((char *) text);
                CI_StringOut ("\r\n");
            }

                cmd_read_slot (report, output);
                /*
                   if (calculated_crc32==authorized_crc) cmd_read_slot(report,output); else not_authorized=1; */
                break;

            case CMD_GET_CODE:
                CI_StringOut ("Get CMD_GET_CODE\r\n");
                if( ((1 == authorized_user_crc_set) && (calculated_crc32 == authorized_user_crc)) ||
                              (*((u8 *) (SLOTS_ADDRESS + GLOBAL_CONFIG_OFFSET + 3)) != 1))
                {
                    authorized_user_crc     = 0xFFFFFFFF;
                    authorized_user_crc_set = 0;
                    cmd_get_code (report, output);
                }
                else
                {
                    not_authorized = 1;
                }
                break;

            case CMD_WRITE_CONFIG:
                CI_StringOut ("Get CMD_WRITE_CONFIG\r\n");
                if (calculated_crc32 == authorized_crc)
                    cmd_write_config (report, output);
                else
                    not_authorized = 1;
                break;

            case CMD_ERASE_SLOT:
                CI_StringOut ("Get CMD_ERASE_SLOT\r\n");
                if (calculated_crc32 == authorized_crc)
                    cmd_erase_slot (report, output);
                else
                    not_authorized = 1;
                break;

            case CMD_FIRST_AUTHENTICATE:
                CI_StringOut ("Get CMD_FIRST_AUTHENTICATE\r\n");
                cmd_first_authenticate (report, output);
                break;

            case CMD_AUTHORIZE:
                CI_StringOut ("Get CMD_AUTHORIZE\r\n");
                cmd_authorize (report, output);
                break;

            case CMD_UNLOCK_USER_PASSWORD:
                CI_StringOut ("Get CMD_UNLOCK_USER_PASSWORD\r\n");
                cmd_unlock_userpassword (report, output);
                break;

            case CMD_LOCK_DEVICE:
                CI_StringOut ("Get CMD_LOCK_DEVICE\r\n");
                cmd_lock_device (report, output);
                break;

            case CMD_USER_AUTHORIZE:
                CI_StringOut ("Get CMD_USER_AUTHORIZE\r\n");
                cmd_user_authorize (report, output);
                break;

            case CMD_GET_PASSWORD_RETRY_COUNT:
                CI_StringOut ("Get CMD_GET_PASSWORD_RETRY_COUNT\r\n");
                // cmd_getPasswordCount(report,output);
                cmd_get_password_retry_count (report, output);
                output[OUTPUT_CMD_RESULT_STICK20_STATUS_START] = 0xaa;
                break;

            case CMD_GET_USER_PASSWORD_RETRY_COUNT:
                CI_StringOut ("Get CMD_GET_USER_PASSWORD_RETRY_COUNT\r\n");
                cmd_get_user_password_retry_count (report, output);
                break;

            case CMD_USER_AUTHENTICATE:
                CI_StringOut ("Get CMD_USER_AUTHENTICATE\r\n");
                cmd_user_authenticate (report, output);
                break;

            case CMD_SET_TIME:
                CI_StringOut ("Get CMD_SET_TIME\r\n");
                cmd_set_time (report, output);
                break;

            case CMD_TEST_COUNTER:
                CI_StringOut ("Get CMD_TEST_COUNTER\r\n");
                cmd_test_counter (report, output);
                break;

            case CMD_TEST_TIME:
                CI_StringOut ("Get CMD_TEST_TIME\r\n");
                cmd_test_time (report, output);
                break;

            case CMD_GET_PW_SAFE_SLOT_STATUS:
                CI_StringOut ("Get CMD_GET_PW_SAFE_SLOT_STATUS\r\n");
                cmd_getPasswordSafeStatus (report, output);
                break;

            case CMD_GET_PW_SAFE_SLOT_NAME:
                CI_StringOut ("Get CMD_GET_PW_SAFE_SLOT_NAME\r\n");
                cmd_getPasswordSafeSlotName (report, output);
                break;

            case CMD_GET_PW_SAFE_SLOT_PASSWORD:
                CI_StringOut ("Get CMD_GET_PW_SAFE_SLOT_PASSWORD\r\n");
                cmd_getPasswordSafeSlotPassword (report, output);
                break;

            case CMD_GET_PW_SAFE_SLOT_LOGINNAME:
                CI_StringOut ("Get CMD_GET_PW_SAFE_SLOT_LOGINNAME\r\n");
                cmd_getPasswordSafeSlotLoginName (report, output);
                break;

            case CMD_SET_PW_SAFE_SLOT_DATA_1:
                CI_StringOut ("Get CMD_GET_PW_SAFE_SET_SLOT_DATA_1\r\n");
                cmd_getPasswordSafeSetSlotData_1 (report, output);
                break;

            case CMD_SET_PW_SAFE_SLOT_DATA_2:
                CI_StringOut ("Get CMD_GET_PW_SAFE_SET_SLOT_DATA_2\r\n");
                cmd_getPasswordSafeSetSlotData_2 (report, output);
                break;

            case CMD_PW_SAFE_ERASE_SLOT:
                CI_StringOut ("Get CMD_GET_PW_SAFE_ERASE_SLOT\r\n");
                cmd_getPasswordSafeEraseSlot (report, output);
                break;

            case CMD_PW_SAFE_ENABLE:
                CI_StringOut ("Get CMD_PW_SAFE_ENABLE\r\n");
                cmd_getPasswordSafeEnable (report, output);
                break;

            case CMD_PW_SAFE_INIT_KEY:
                CI_StringOut ("Get CMD_PW_SAFE_INIT_KEY\r\n");
                cmd_getPasswordSafeInitKey (report, output);
                break;

            case CMD_PW_SAFE_SEND_DATA:
                CI_StringOut ("Get CMD_PW_SAFE_SEND_DATA\r\n");
                cmd_getPasswordSafeSendData (report, output);
                break;

            case CMD_SD_CARD_HIGH_WATERMARK:
                CI_StringOut ("Get CMD_SD_CARD_HIGH_WATERMARK\r\n");
                cmd_getSdCardHighWaterMark (report, output);
                {
u8 text[10];
                    CI_StringOut ("Send high water mark: write ");
                    itoa (output[OUTPUT_CMD_RESULT_OFFSET + 0], text);
                    CI_StringOut ((char *) text);
                    CI_StringOut (" % - read ");
                    itoa (output[OUTPUT_CMD_RESULT_OFFSET + 1], text);
                    CI_StringOut ((char *) text);
                    CI_StringOut (" %\r\n");
                }
                break;

            case CMD_FACTORY_RESET:
                CI_StringOut ("Get CMD_FACTORY_RESET\r\n");
                cmd_getFactoryReset (report, output);
                break;
            case CMD_RESET_STICK :
                CI_StringOut ("Get CMD_RESET_STICK\r\n");
//                cmd_getResetStick (report, output);
                break;


            default:
                if ((STICK20_CMD_START_VALUE > cmd_type) || (STICK20_CMD_END_VALUE < cmd_type))
                {
u8 text[10];
                    CI_StringOut ("Get unknown cmd ");
                    itoa (cmd_type, text);
                    CI_StringOut ((char *) text);
                    CI_StringOut ("\r\n");
                }
                break;
        }
        if (not_authorized)
        {
            CI_StringOut ("*** NOT AUTHORIZED ***\r\n");
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
        }

        if (calculated_crc32 == authorized_crc)
        {
            authorized_crc = 0xFFFFFFFF;
        }
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_CRC;
    }

    // Stick 2.0 commands
    if ((STICK20_CMD_START_VALUE <= cmd_type) && (STICK20_CMD_END_VALUE > cmd_type))
    {
        if ((calculated_crc32 == received_crc32))
        {
            switch (cmd_type)
            {
                case STICK20_CMD_ENABLE_CRYPTED_PARI:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_CRYPTED_PARI Password=-");
                    CI_StringOut ((char *) &report[1]);
                    CI_StringOut ("-\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_CRYPTED_PARI);

                    // Tranfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_AES_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_DISABLE_CRYPTED_PARI:
                    CI_StringOut ("Get STICK20_CMD_DISABLE_CRYPTED_PARI\r\n");

                    StartStick20Command (STICK20_CMD_DISABLE_CRYPTED_PARI);

                    // Tranfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_DISABLE_AES_LUN;
                    memset (HID_String_au8, 0, 33);
                    break;


                case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI Password=-");
                    CI_StringOut ((char *) &report[1]);
                    CI_StringOut ("-\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_HIDDEN_AES_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI:
                    CI_StringOut ("Get STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI\r\n");

                    StartStick20Command (STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_DISABLE_HIDDEN_AES_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_ENABLE_FIRMWARE_UPDATE:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_FIRMWARE_UPDATE\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_FIRMWARE_UPDATE);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_FIRMWARE_UPDATE;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE:
                    CI_StringOut ("Get STICK20_CMD_EXPORT_FIRMWARE_TO_FILE\r\n");

                    StartStick20Command (STICK20_CMD_EXPORT_FIRMWARE_TO_FILE);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_EXPORT_BINARY;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_GENERATE_NEW_KEYS:
                    CI_StringOut ("Get STICK20_CMD_GENERATE_NEW_KEYS\r\n");

                    StartStick20Command (STICK20_CMD_GENERATE_NEW_KEYS);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_GENERATE_NEW_KEYS;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS:
                    CI_StringOut ("Get STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS\r\n");

                    StartStick20Command (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_FILL_SD_WITH_RANDOM_CHARS;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

#ifdef HID_PASSWORD_MARTIX_AKTIV
                case STICK20_CMD_SEND_PASSWORD_MATRIX:
                    CI_StringOut ("Get STICK20_CMD_SEND_PASSWORD_MATRIX\r\n");

                    StartStick20Command (STICK20_CMD_SEND_PASSWORD_MATRIX);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_SEND_PASSWORD_MATRIX;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA:
                    CI_StringOut ("Get STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA\r\n");

                    StartStick20Command (STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_SEND_PASSWORD_MATRIX_PINDATA;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP:
                    CI_StringOut ("Get STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP\r\n");

                    StartStick20Command (STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_SEND_PASSWORD_MATRIX_SETUP;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;
#endif

                case STICK20_CMD_GET_DEVICE_STATUS:
                    CI_StringOut ("Get STICK20_CMD_GET_DEVICE_STATUS\r\n");

                    StartStick20Command (STICK20_CMD_GET_DEVICE_STATUS);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_GET_DEVICE_STATUS;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_SEND_DEVICE_STATUS:
                    CI_StringOut ("Get STICK20_CMD_SEND_DEVICE_STATUS\r\n");

                    StartStick20Command (STICK20_CMD_SEND_DEVICE_STATUS);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_WRITE_STATUS_DATA;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                    /*
                       case STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD : CI_StringOut ("Get STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD\r\n");

                       StartStick20Command (STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD);

                       // Transfer data to other context HID_CmdGet_u8 = HTML_SEND_HIDDEN_VOLUME_PASSWORD; memcpy (HID_String_au8,&report[1],33);
                       break; */
                    /*
                       case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP : CI_StringOut ("Get STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP\r\n");

                       StartStick20Command (STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP);

                       // Transfer data to other context HID_CmdGet_u8 = HTML_SEND_HIDDEN_VOLUME_SETUP; memcpy (HID_String_au8,&report[1],33); break; */
                case STICK20_CMD_SEND_PASSWORD:
                    CI_StringOut ("Get STICK20_CMD_SEND_PASSWORD\r\n");

                    StartStick20Command (STICK20_CMD_SEND_PASSWORD);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_SEND_PASSWORD;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_SEND_NEW_PASSWORD:
                    CI_StringOut ("Get STICK20_CMD_SEND_NEW_PASSWORD\r\n");

                    StartStick20Command (STICK20_CMD_SEND_NEW_PASSWORD);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_SEND_NEW_PASSWORD;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_READONLY_UNCRYPTED_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_READWRITE_UNCRYPTED_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND:
                    CI_StringOut ("Get STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND\r\n");

                    StartStick20Command (STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_CLEAR_NEW_SD_CARD_FOUND;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_SEND_STARTUP:
                    CI_StringOut ("Get STICK20_CMD_SEND_STARTUP\r\n");

                    StartStick20Command (STICK20_CMD_SEND_STARTUP);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_SEND_STARTUP;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP:
                    CI_StringOut ("Get STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP\r\n");

                    StartStick20Command (STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_HIDDEN_VOLUME_SETUP;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED:
                    CI_StringOut ("Get STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED\r\n");

                    StartStick20Command (STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_CLEAR_STICK_KEYS_NOT_INITIATED;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_SEND_LOCK_STICK_HARDWARE:
                    CI_StringOut ("Get STICK20_CMD_SEND_LOCK_STICK_HARDWARE\r\n");

                    StartStick20Command (STICK20_CMD_SEND_LOCK_STICK_HARDWARE);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_CLEAR_LOCK_STICK_HARDWARE;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_PRODUCTION_TEST:
                    CI_StringOut ("Get STICK20_CMD_PRODUCTION_TEST\r\n");

                    StartStick20Command (STICK20_CMD_PRODUCTION_TEST);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_PRODUCTION_TEST;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_PRODUCTION_TEST_WITH_WRITE_TEST:
                    CI_StringOut ("Get HTML_CMD_PRODUCTION_TEST_WITH_WRITE_TEST\r\n");
                    StartStick20Command (HTML_CMD_PRODUCTION_TEST_WITH_WRITE_TEST);
                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_PRODUCTION_TEST_WITH_WRITE_TEST;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;



                case STICK20_CMD_CHANGE_UPDATE_PIN:
                    CI_StringOut ("Get STICK20_CMD_CHANGE_UPDATE_PIN\r\n");
                    StartStick20Command (STICK20_CMD_CHANGE_UPDATE_PIN);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_CHANGE_UPDATE_PIN;
                    memcpy (HID_String_au8, &report[1], HID_STRING_LEN);
                    break;

                case STICK20_CMD_ENABLE_ADMIN_READONLY_UNCRYPTED_LUN:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_ADMIN_READONLY_UNCRYPTED_LUN\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_ADMIN_READONLY_UNCRYPTED_LUN);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_ADMIN_READONLY_UNCRYPTED_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_ENABLE_ADMIN_READWRITE_UNCRYPTED_LUN:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_ADMIN_READWRITE_UNCRYPTED_LUN\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_ADMIN_READWRITE_UNCRYPTED_LUN);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_ADMIN_READWRITE_UNCRYPTED_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

/*
                case STICK20_CMD_ENABLE_ADMIN_READONLY_ENCRYPTED_LUN:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_ADMIN_READONLY_ENCRYPTED_LUN\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_ADMIN_READONLY_ENCRYPTED_LUN);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_ADMIN_READONLY_ENCRYPTED_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

                case STICK20_CMD_ENABLE_ADMIN_READWRITE_ENCRYPTED_LUN:
                    CI_StringOut ("Get STICK20_CMD_ENABLE_ADMIN_READWRITE_ENCRYPTED_LUN\r\n");

                    StartStick20Command (STICK20_CMD_ENABLE_ADMIN_READWRITE_ENCRYPTED_LUN);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_ENABLE_ADMIN_READWRITE_ENCRYPTED_LUN;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;
*/

                case STICK20_CMD_CHECK_SMARTCARD_USAGE:
                    CI_StringOut ("Get STICK20_CMD_CHECK_SMARTCARD_USAGE\r\n");

                    StartStick20Command (STICK20_CMD_CHECK_SMARTCARD_USAGE);

                    // Transfer data to other context
                    HID_CmdGet_u8 = HTML_CMD_CHECK_SMARTCARD_USAGE;
                    memcpy (HID_String_au8, &report[1], 33);
                    break;

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
                case STICK20_CMD_SEND_DEBUG_DATA:
                    // CI_StringOut ("Get STICK20_CMD_SEND_DEBUG_DATA\r\n");
                    Stick20HIDSendDebugData (output);
                    DelayMs (1);
                    break;
#endif

                default:
                    break;

            }
        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_CRC;
        }

        // copy Stick 2.0 output status to HID output
        memcpy (&output[OUTPUT_CMD_RESULT_STICK20_STATUS_START], (void *) &HID_Stick20Status_st, sizeof (HID_Stick20Status_est));

    }

    // Stick 20 Send password matrix ?
    if (0 != Stick20HIDSendMatrixState_u8)
    {
        Stick20HIDSendMatrixData (output);
    }


    calculated_crc32 = generateCRC (output);

    output[OUTPUT_CRC_OFFSET] = calculated_crc32 & 0xFF;
    output[OUTPUT_CRC_OFFSET + 1] = (calculated_crc32 >> 8) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 2] = (calculated_crc32 >> 16) & 0xFF;
    output[OUTPUT_CRC_OFFSET + 3] = (calculated_crc32 >> 24) & 0xFF;

    if (output[OUTPUT_CMD_STATUS_OFFSET] != oldStatus)
    {
        oldStatus = output[OUTPUT_CMD_STATUS_OFFSET];

        // Print the new output status
        switch (oldStatus)
        {
            case CMD_STATUS_OK:
                CI_StringOut ("CMD_STATUS_OK\r\n");
                break;
            case CMD_STATUS_WRONG_CRC:
                CI_StringOut ("CMD_STATUS_WRONG_CRC\r\n");
                break;
            case CMD_STATUS_WRONG_SLOT:
                CI_StringOut ("CMD_STATUS_WRONG_SLOT\r\n");
                break;
            case CMD_STATUS_SLOT_NOT_PROGRAMMED:
                CI_StringOut ("CMD_STATUS_SLOT_NOT_PROGRAMMED\r\n");
                break;
            case CMD_STATUS_WRONG_PASSWORD:
                CI_StringOut ("CMD_STATUS_SLOT_NOT_PROGRAMMED\r\n");
                break;
            case CMD_STATUS_NOT_AUTHORIZED:
                CI_StringOut ("CMD_STATUS_NOT_AUTHORIZED\r\n");
                break;
        }
    }
    /* USB Debug */
#ifdef DEBUG_HID_USB_DATA
    CI_StringOut ("\n\rSend ");
    for (i = 0; i < KEYBOARD_FEATURE_COUNT; i++)
    {
        CI_Print8BitValue (output[i]);
    }
    CI_StringOut ("\n\r");
#endif

}

/*******************************************************************************

  cmd_get_status

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 426797e7c2598b6befc0b43e3dd33393195ddbf1
                              Improvements to OTP and About Window see #75 #62 #96 #108
  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void cmd_get_status (u8 * report, u8 * output)
{
    output[OUTPUT_CMD_RESULT_OFFSET] = FIRMWARE_VERSION & 0xFF;
    output[OUTPUT_CMD_RESULT_OFFSET + 1] = (FIRMWARE_VERSION >> 8) & 0xFF;
    /*
       output[OUTPUT_CMD_RESULT_OFFSET+2] =cardSerial&0xFF; output[OUTPUT_CMD_RESULT_OFFSET+3] =(cardSerial>>8)&0xFF;
       output[OUTPUT_CMD_RESULT_OFFSET+4] =(cardSerial>>16)&0xFF; output[OUTPUT_CMD_RESULT_OFFSET+5] =(cardSerial>>24)&0xFF; */
    memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 6, (u8 *) SLOTS_ADDRESS + GLOBAL_CONFIG_OFFSET, 3);
    memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 9, (u8 *) SLOTS_ADDRESS + GLOBAL_CONFIG_OFFSET + 3, 2);

    {
u8 text_au8[10];
u32 i;

        CI_StringOut ("Send config: ");
        for (i = 0; i < 5; i++)
        {
            itoa (output[OUTPUT_CMD_RESULT_OFFSET + 6 + i], text_au8);
            CI_StringOut ((char *) text_au8);
            CI_StringOut (" ");
        }
        CI_StringOut ("\r\n");
    }
}

/*******************************************************************************

  cmd_get_password_retry_count

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 426797e7c2598b6befc0b43e3dd33393195ddbf1
                              Improvements to OTP and About Window see #75 #62 #96 #108
  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void cmd_get_password_retry_count (u8 * report, u8 * output)
{
    output[OUTPUT_CMD_RESULT_OFFSET] = StickConfiguration_st.AdminPwRetryCount;
}

/*******************************************************************************

  cmd_get_user_password_retry_count

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 426797e7c2598b6befc0b43e3dd33393195ddbf1
                              Improvements to OTP and About Window see #75 #62 #96 #108
  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_get_user_password_retry_count (u8 * report, u8 * output)
{
    output[OUTPUT_CMD_RESULT_OFFSET] = StickConfiguration_st.UserPwRetryCount;
    return 0;
}

/*******************************************************************************

  cmd_write_to_slot

  Write data from pc into flash

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 9afc711a5365fc920c6c412b612b200954ab11eb
                              Mandatory slot name #109

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_write_to_slot (u8 * report, u8 * output)
{

u8 slot_no = report[CMD_WTS_SLOT_NUMBER_OFFSET];
u8 slot_tmp[64];                // this is will be the new slot contents
u8 slot_name[15];
u32 Counter_u32;
u64 counter;

    memset (slot_tmp, 0, 64);

    slot_tmp[0] = 0x01; // marks slot as programmed

    memcpy (slot_tmp + 1, report + CMD_WTS_SLOT_NAME_OFFSET, 51);
    memcpy (slot_name, report + CMD_WTS_SLOT_NAME_OFFSET, 15);

    if (slot_name[0] == 0)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NO_NAME_ERROR;
        return 1;
    }


    if ((slot_no >= 0x10) && (slot_no < 0x10 + NUMBER_OF_HOTP_SLOTS))   // HOTP slot
    {
        slot_no = slot_no & 0x0F;

        // u64 counter = getu64 (report+CMD_WTS_COUNTER_OFFSET);
        // u64 counter = atoi (report+CMD_WTS_COUNTER_OFFSET);

        Counter_u32 = atoi ((char *)&report[CMD_WTS_COUNTER_OFFSET]);
        counter = Counter_u32;
        {
u8 text[20];
            CI_StringOut ("Write HOTP Slot ");
            itoa (slot_no, text);
            CI_StringOut ((char *) text);
            CI_StringOut (" counter -");
            itoa (counter, text);
            CI_StringOut ((char *) text);
            CI_StringOut (" -");
            memcpy (text, (char *) report + CMD_WTS_COUNTER_OFFSET, 8);
            text[8] = 0;
            CI_StringOut ((char *) text);
            CI_StringOut ("-\r\n");
        }

        set_counter_value (hotp_slot_counters[slot_no], counter);

        write_to_slot (slot_tmp, hotp_slot_offsets[slot_no], 64);
    }
    else if ((slot_no >= 0x20) && (slot_no < 0x20 + NUMBER_OF_TOTP_SLOTS))  // TOTP slot
    {
        slot_no = slot_no & 0x0F;

        write_to_slot (slot_tmp, totp_slot_offsets[slot_no], 64);
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_SLOT;
        return 1;
    }

    return 0;
}

/*******************************************************************************

  cmd_read_slot_name

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_read_slot_name (u8 * report, u8 * output)
{
u8 text[10];
u8 slot_no = report[1];


    if ((slot_no >= 0x10) && (slot_no < 0x10 + NUMBER_OF_HOTP_SLOTS))   // HOTP slot
    {
        slot_no = slot_no & 0x0F;
u8 is_programmed = *((u8 *) (hotp_slots[slot_no]));

        if (is_programmed == 0x01)
        {
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET, (u8 *) (hotp_slots[slot_no] + SLOT_NAME_OFFSET), 15);

            CI_StringOut ("HOTP Slot ");
            itoa (slot_no, text);
            CI_StringOut ((char *) text);
            CI_StringOut (" name -");
            CI_StringOut ((char *) output + OUTPUT_CMD_RESULT_OFFSET);
            CI_StringOut ("-\r\n");
        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_SLOT_NOT_PROGRAMMED;
            return 1;
        }
    }
    else if ((slot_no >= 0x20) && (slot_no < 0x20 + NUMBER_OF_TOTP_SLOTS))  // TOTP slot
    {
        slot_no = slot_no & 0x0F;
u8 is_programmed = *((u8 *) (totp_slots[slot_no]));

        if (is_programmed == 0x01)
        {
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET, (u8 *) (totp_slots[slot_no] + SLOT_NAME_OFFSET), 15);
            CI_StringOut ("TOTP Slot ");
            itoa (slot_no, text);
            CI_StringOut ((char *) text);
            CI_StringOut (" name -");
            CI_StringOut ((char *) output + OUTPUT_CMD_RESULT_OFFSET);
            CI_StringOut ("-\r\n");
        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_SLOT_NOT_PROGRAMMED;
            return 1;
        }
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_SLOT;
    }

    return 0;
}

/*******************************************************************************

  cmd_read_slot

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_read_slot (u8 * report, u8 * output)
{
u8 slot_no = report[CMD_RS_SLOT_NUMBER_OFFSET];
u64 counter;

    if ((slot_no >= 0x10) && (slot_no < 0x10 + NUMBER_OF_HOTP_SLOTS))   // HOTP slot
    {
        slot_no = slot_no & 0x0F;
u8 is_programmed = *((u8 *) (hotp_slots[slot_no]));

        if (is_programmed == 0x01)
        {
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET, (u8 *) (hotp_slots[slot_no] + SLOT_NAME_OFFSET), 15);
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 15, (u8 *) (hotp_slots[slot_no] + CONFIG_OFFSET), 1);
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 16, (u8 *) (hotp_slots[slot_no] + TOKEN_ID_OFFSET), 13);

            counter = get_counter_value (hotp_slot_counters[slot_no]);
            itoa ((u32) counter, &output[OUTPUT_CMD_RESULT_OFFSET + 29]);   // Bytes after OUTPUT_CMD_RESULT_OFFSET+29+8 is unused

            {
u8 text[20];
                CI_StringOut ("Read HOTP Slot ");
                itoa (slot_no, text);
                CI_StringOut ((char *) text);
                CI_StringOut (" counter -");
                itoa (counter, text);
                CI_StringOut ((char *) text);
                CI_StringOut ("- -");
                memcpy (text, (char *) &output[OUTPUT_CMD_RESULT_OFFSET + 29], 8);
                text[8] = 0;
                CI_StringOut ((char *) text);
                CI_StringOut ("-\r\n");
            }

        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_SLOT_NOT_PROGRAMMED;
            return 1;
        }
    }
    else if ((slot_no >= 0x20) && (slot_no < 0x20 + NUMBER_OF_TOTP_SLOTS))  // TOTP slot
    {
        slot_no = slot_no & 0x0F;
u8 is_programmed = *((u8 *) (totp_slots[slot_no]));
        if (is_programmed == 0x01)
        {
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET, (u8 *) (totp_slots[slot_no] + SLOT_NAME_OFFSET), 15);
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 15, (u8 *) (totp_slots[slot_no] + CONFIG_OFFSET), 1);
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 16, (u8 *) (totp_slots[slot_no] + TOKEN_ID_OFFSET), 13);
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 29, (u8 *) (totp_slots[slot_no] + INTERVAL_OFFSET), 2);
        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_SLOT_NOT_PROGRAMMED;
            return 1;
        }
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_SLOT;
        return 1;
    }

    return 0;
}

/*******************************************************************************

  CheckSystemtime


  Changes
  Date      Author          Info
  18.03.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

#define MAX_SYSTEMTIME_TIMEDIFF                           60    // Sec
#define SYSTEMTIME_TIMEDIFF_FOR_WRITING_INTO_FLASH       (60*60)    // Sec = 1 hour


u8 CheckSystemtime (u32 Newtimestamp_u32)
{
    // struct tm tm_st;
u8 Time_u8[30];
time_t now;
u32 StoredTime_u32;
s64 Timediff_s64;
s32 Timediff_s32;

    // Get the local time
    time (&now);

    if (60 * 60 * 24 * 365 > now)   // If the local time is before 1971, set systemtime
    {
        CI_LocalPrintf ("Local time is not set - set to %ld = ", Newtimestamp_u32);
        ctime_r ((time_t *) & Newtimestamp_u32, (char *) Time_u8);
        CI_LocalPrintf ("%s\r\n", Time_u8);
        set_time ((time_t) Newtimestamp_u32);

        // Update MSD readwrite timer
        UpdateMsdLastAccessTimer (Newtimestamp_u32);
    }

    // Check timediff
    time (&now);    // Get local time
    Timediff_s64 = now - Newtimestamp_u32;
    Timediff_s32 = Timediff_s64;
    if ((Timediff_s32 > MAX_SYSTEMTIME_TIMEDIFF) || (Timediff_s32 < -MAX_SYSTEMTIME_TIMEDIFF))
    {
        CI_LocalPrintf ("New timestamp is not valid\r\n");
        CI_LocalPrintf ("Systemtime:");
        ctime_r (&now, (char *) Time_u8);
        CI_LocalPrintf ("%s", Time_u8);

        CI_LocalPrintf ("Timestamp :");
        ctime_r ((time_t *) & Newtimestamp_u32, (char *) Time_u8);
        CI_LocalPrintf ("%s", Time_u8);
        return (FALSE);
    }

    // Check for writing timestamp to flash
    ReadDatetime (&StoredTime_u32);
    Timediff_s64 = now - StoredTime_u32;
    Timediff_s32 = Timediff_s64;

    if (Timediff_s32 < -MAX_SYSTEMTIME_TIMEDIFF)
    {
        CI_LocalPrintf ("ERROR: Systemtime is time before stored time\r\n");
        return (FALSE); // Timestamp points to a time before the stored time > something is wrong
    }

    if (Timediff_s32 < SYSTEMTIME_TIMEDIFF_FOR_WRITING_INTO_FLASH)
    {
        return (TRUE);
    }

    // Save the systemtime into flash
    CI_LocalPrintf ("Write time into flash - ");
    ctime_r (&now, (char *) Time_u8);
    CI_LocalPrintf ("%s\r\n", Time_u8);

    WriteDatetime (now);

    return (TRUE);
}

/*******************************************************************************

  cmd_get_code

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_get_code (u8 * report, u8 * output)
{
    // u64 challenge = getu64(report + CMD_GC_CHALLENGE_OFFSET);
u64 timestamp = getu64 (report + CMD_GC_TIMESTAMP_OFFSET);
u8 interval = report[CMD_GC_INTERVAL_OFFSET];
u32 result = 0;
    /*
       CI_LocalPrintf ("challenge:" ); HexPrint (8,(unsigned char*)report + CMD_GC_CHALLENGE_OFFSET); CI_LocalPrintf ("\n\r"); */

u8 slot_no = report[CMD_GC_SLOT_NUMBER_OFFSET];

    if ((slot_no >= 0x10) && (slot_no < 0x10 + NUMBER_OF_HOTP_SLOTS))   // HOTP slot
    {
        slot_no = slot_no & 0x0F;
u8 is_programmed = *((u8 *) (hotp_slots[slot_no]));
        if (is_programmed == 0x01)
        {
            result = get_code_from_hotp_slot (slot_no);

            // Change endian
            result = change_endian_u32 (result);

            memcpy (output + OUTPUT_CMD_RESULT_OFFSET, &result, 4);
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 4, (u8 *) hotp_slots[slot_no] + CONFIG_OFFSET, 14);
        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_SLOT_NOT_PROGRAMMED;
        }
    }
    else if ((slot_no >= 0x20) && (slot_no < 0x20 + NUMBER_OF_TOTP_SLOTS))  // TOTP slot
    {
        slot_no = slot_no & 0x0F;
u8 is_programmed = *((u8 *) (totp_slots[slot_no]));

/* Improvement for future, check USB timestamp against local timestamp
        if (FALSE == CheckSystemtime ((u32) timestamp))
        {

        }
*/
        if (is_programmed == 0x01)
        {
            result = get_code_from_totp_slot (slot_no, timestamp / (u64) interval);
            // result = get_code_from_totp_slot (slot_no,challenge);

            // Change endian
            result = change_endian_u32 (result);

            memcpy (output + OUTPUT_CMD_RESULT_OFFSET, &result, 4);
            memcpy (output + OUTPUT_CMD_RESULT_OFFSET + 4, (u8 *) totp_slots[slot_no] + CONFIG_OFFSET, 14);
        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_SLOT_NOT_PROGRAMMED;
        }
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_SLOT;
    }

    return 0;
}

/*******************************************************************************

  cmd_write_config

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_write_config (u8 * report, u8 * output)
{
u8 slot_tmp[64];                // this is will be the new slot contents

    memset (slot_tmp, 0, 5);

    memcpy (slot_tmp, report + 1, 5);

    {
u8 text_au8[10];
u32 i;

        CI_StringOut ("Write config: ");
        for (i = 0; i < 5; i++)
        {
            itoa (slot_tmp[i], text_au8);
            CI_StringOut ((char *) text_au8);
            CI_StringOut (" ");
        }
        CI_StringOut ("\r\n");
    }

    write_to_slot (slot_tmp, GLOBAL_CONFIG_OFFSET, 5);

    return 0;
}

/*******************************************************************************

  cmd_erase_slot

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 3170acd1e68c618aaffd16b2722108c7fc7d5725
                              Secret not overwriten when the Tool sends and empty secret

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


u8 cmd_erase_slot (u8 * report, u8 * output)
{
u8 slot_no = report[CMD_WTS_SLOT_NUMBER_OFFSET];
u8 slot_tmp[64];
u8 text_au8[10];

    memset (slot_tmp, 0xFF, 64);

    if ((slot_no >= 0x10) && (slot_no <= 0x10 + NUMBER_OF_HOTP_SLOTS))  // HOTP slot
    {
        slot_no = slot_no & 0x0F;

        CI_StringOut ("Erase slot HOTP ");
        itoa (slot_no, text_au8);
        CI_StringOut ((char *) text_au8);
        CI_StringOut ("\r\n");

        write_to_slot (slot_tmp, hotp_slot_offsets[slot_no], 64);
        erase_counter (slot_no);
    }
    else if ((slot_no >= 0x20) && (slot_no <= 0x20 + NUMBER_OF_TOTP_SLOTS)) // TOTP slot
    {
        slot_no = slot_no & 0x0F;

        CI_StringOut ("Erase slot TOTP ");
        itoa (slot_no, text_au8);
        CI_StringOut ((char *) text_au8);
        CI_StringOut ("\r\n");


        write_to_slot (slot_tmp, totp_slot_offsets[slot_no], 64);
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_SLOT;
    }

    return 0;
}

/*******************************************************************************

  cmd_first_authenticate

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_first_authenticate (u8 * report, u8 * output)
{
u8 res = 1;
u8 card_password[26];
u32 Ret_u32;
u8 cMainVersion_u8;
u8 cSecVersion_u8;

    memset (card_password, 0, 26);

    memcpy (card_password, report + 1, 25);

    // Init smartcard
    // CCID_RestartSmartcard_u8 ();
    // ISO7816_T1_InitSendNr ();


    // res = cardAuthenticate (card_password); // Check for admin password

    // Ret_u32 = LA_SC_SendVerify (1,(unsigned char *)card_password); // 1 = user pw
    Ret_u32 = LA_OpenPGP_V20_Test_SendAdminPW ((unsigned char *) card_password);
    if (TRUE == Ret_u32)
    {
        res = 0;
    }

    memset (card_password, 0, 26);  // RB : Clear password ram
    memset (temp_password, 0, 25);  // RB : Clear password ram
    tmp_password_set = 0;

    if (res == 0)
    {
        memcpy (temp_password, report + 26, 25);

        tmp_password_set = 1;

        // getAID();

        Ret_u32 = LA_OpenPGP_V20_Test_GetAID (&cMainVersion_u8, &cSecVersion_u8);

        PWS_DecryptedPasswordSafeKey ();

        return 0;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
        return 1;   // wrong card password
    }
}

/*******************************************************************************

  cmd_user_authenticate

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 426797e7c2598b6befc0b43e3dd33393195ddbf1
                              Improvements to OTP and About Window see #75 #62 #96 #108
  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_user_authenticate (u8 * report, u8 * output)
{
    u8 res = 1;
    u8 user_password[26];
    u32 Ret_u32;

    memset (user_password, 0, 26);
    memset (temp_user_password, 0, 25);
    memcpy (user_password, report + 1, 25);
    tmp_user_password_set = 0;

    Ret_u32 = LA_OpenPGP_V20_Test_SendUserPW2 ((unsigned char *) user_password);
    if (TRUE == Ret_u32)
    {
        res = 0;
    }

    if (res == 0)
    {
        memcpy (temp_user_password, report + 26, 25);
        tmp_user_password_set = 1;
        return 0;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
        return 1;   // wrong card password
    }
}

/*******************************************************************************

  cmd_unlock_userpassword

  Changes
  Date      Reviewer        Info
  02.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_unlock_userpassword (u8 * report, u8 * output)
{
u8 res = 1;
u8 newUserPassword_au8[40];
u32 Ret_u32;

    memset (newUserPassword_au8, 0, 40);
    memcpy (newUserPassword_au8, report + 2, 30);

    Ret_u32 = LA_OpenPGP_V20_Test_ResetRetryCounter ((unsigned char *) newUserPassword_au8);
    if (TRUE == Ret_u32)
    {
        res = 0;
    }

    memset (newUserPassword_au8, 0, 40);

    if (0 == res)
    {
        return (0);
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
        return (1); // wrong password
    }
}

/*******************************************************************************

  cmd_lock_device

  Changes
  Date      Reviewer        Info
  18.10.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_lock_device (u8 * report, u8 * output)
{
    LockDevice ();
    return (0);
}

/*******************************************************************************

  cmd_authorize

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_authorize (u8 * report, u8 * output)
{
    if (tmp_password_set == 1)
    {
        if (memcmp (report + 5, temp_password, 25) == 0)
        {
            authorized_crc = getu32 (report + 1);
            return (0);
        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
            return (1);
        }
    }

    return (0);
}

/*******************************************************************************

  cmd_authorize

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 426797e7c2598b6befc0b43e3dd33393195ddbf1
                              Improvements to OTP and About Window see #75 #62 #96 #108
  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_user_authorize (u8 * report, u8 * output)
{
    if (tmp_user_password_set == 1)
    {
        if (memcmp (report + 5, temp_user_password, 25) == 0)
        {
            authorized_user_crc     = getu32 (report + 1);
            authorized_user_crc_set = 1;
            return 0;
        }
        else
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
            return 1;
        }
    }
    return 1;
}


/*******************************************************************************

  cmd_getPasswordCount

  Changes
  Date      Author          Info
  20.08.14  RB              Add admin retry count

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordCount (u8 * report, u8 * output)
{
u8 Data_u8[20];
u32 Ret_u32;

    Ret_u32 = LA_OpenPGP_V20_GetPasswordstatus ((char *) Data_u8);
    if (TRUE == Ret_u32)
    {
        // output[OUTPUT_CMD_RESULT_OFFSET] = Data_u8[4];
        output[OUTPUT_CMD_RESULT_OFFSET] = Data_u8[6];  // Admin retry counter
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_RESULT_OFFSET] = 88;  // Admin retry counter
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
    }

    return (0);
}




/*******************************************************************************

  cmd_set_time

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit dde179d5d24bd3d06cab73848ba1ab7e5efda898
                              Time in firmware Test #74 Test #105

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_set_time (u8 * report, u8 * output)
{
int err;
u64 new_time = (getu64 (report + CMD_DATA_OFFSET + 1));
u32 old_time = get_time_value ();
u32 new_time_minutes = (new_time - 1388534400) / 60;    // 1388534400 = 01.01.2014 00:00
    /*
       { u8 text[30];

       CI_StringOut ("cmd_set_time: ");

       CI_StringOut ("new_time = "); itoa (new_time,text); CI_StringOut ((char*)text);

       CI_StringOut (" = "); ctime_r ((time_t*)&new_time,(char*)text); CI_StringOut ((char*)text); CI_StringOut ("\r\n"); } */

    if (0 == report[CMD_DATA_OFFSET])   // Check valid time only at check time
    {
        if (old_time == 0)
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_TIMESTAMP_WARNING;
            return 1;
        }
    }

    // if((old_time <= new_time_minutes) || (old_time == 0xffffffff) || (*((u8 *)(report + CMD_DATA_OFFSET)) == 1))
    if ((old_time <= new_time_minutes) || (old_time == 0xffffffff) || (report[CMD_DATA_OFFSET] == 1))
    {
        /*
           { u8 text[30];

           CI_StringOut ("cmd_set_time: ");

           CI_StringOut ("Local time set to = "); itoa (new_time,text); CI_StringOut ((char*)text);

           CI_StringOut (" = "); ctime_r ((time_t*)&new_time,(char*)text); CI_StringOut ((char*)text); CI_StringOut ("\r\n"); } */
        set_time (new_time);

        current_time = new_time;
        err = set_time_value (new_time_minutes);
        if (err)
        {
            output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_TIMESTAMP_WARNING;
            return 1;
        }
        return 0;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_TIMESTAMP_WARNING;
        return 1;
    }

}


/*******************************************************************************

  cmd_test_counter

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 7bac1a8ad3c805d52a7a1b0d327b921cebbc8d72
                              OTP Test Scripts Test #80

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_test_counter (u8 * report, u8 * output)
{
int i;
u8 slot_no = report[CMD_DATA_OFFSET];
u16 tests_number = getu16 (report + CMD_DATA_OFFSET + 1);
u16 results = 0;
u64 counter = get_counter_value (hotp_slot_counters[slot_no]);
u64 counter_new = 0;

    for (i = 0; i < tests_number; i++)
    {
        set_counter_value (hotp_slot_counters[slot_no], counter);
        counter_new = get_counter_value (hotp_slot_counters[slot_no]);
        if (counter == counter_new)
        {
            results++;
        }
    }

    memcpy (output + OUTPUT_CMD_RESULT_OFFSET, &results, 2);

    return 0;
}

/*******************************************************************************

  cmd_test_time

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 7bac1a8ad3c805d52a7a1b0d327b921cebbc8d72
                              OTP Test Scripts Test #80

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_test_time (u8 * report, u8 * output)
{
int i, err;
u32 time_min;
u32 read_time;
u32 tests_number = getu16 (report + CMD_DATA_OFFSET + 1);
u16 results = 0;
time_t now;

    // Get the local ATMEL time
    time (&now);
    current_time = now;

    for (i = 0; i < tests_number; i++)
    {
        time_min = (current_time - 1388534400) / 60;    // time = minutes after 01.01.2014 00:00
        err = set_time_value (time_min);

        read_time = get_time_value ();

        if (!err && (read_time == time_min))
        {
            results++;
        }
    }

    memcpy (output + OUTPUT_CMD_RESULT_OFFSET, &results, 2);

    return 0;
}







/*******************************************************************************

  cmd_getPasswordSafeStatus

  Changes
  Date      Reviewer        Info
  30.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeStatus (u8 * report, u8 * output)
{
u8 Data_u8[PWS_SLOT_COUNT];
u32 Ret_u32;

    Ret_u32 = PWS_GetAllSlotStatus (Data_u8);
    if (TRUE == Ret_u32)
    {
        memcpy (&output[OUTPUT_CMD_RESULT_OFFSET], Data_u8, PWS_SLOT_COUNT);
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}


/*******************************************************************************

  cmd_getPasswordSafeSlotName

  Changes
  Date      Reviewer        Info
  30.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeSlotName (u8 * report, u8 * output)
{
u32 Ret_u32;

    Ret_u32 = PWS_GetSlotName (report[1], &output[OUTPUT_CMD_RESULT_OFFSET]);
    if (TRUE == Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}

/*******************************************************************************

  cmd_getPasswordSafeSlotPassword

  Changes
  Date      Reviewer        Info
  30.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeSlotPassword (u8 * report, u8 * output)
{
u32 Ret_u32;

    Ret_u32 = PWS_GetSlotPassword (report[1], &output[OUTPUT_CMD_RESULT_OFFSET]);
    if (TRUE == Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}

/*******************************************************************************

  cmd_getPasswordSafeSlotLoginName

  Changes
  Date      Reviewer        Info
  30.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeSlotLoginName (u8 * report, u8 * output)
{
u32 Ret_u32;

    Ret_u32 = PWS_GetSlotLoginName (report[1], &output[OUTPUT_CMD_RESULT_OFFSET]);
    if (TRUE == Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}

/*******************************************************************************

  cmd_getPasswordSafeSetSlotData_1

  Changes
  Date      Reviewer        Info
  30.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeSetSlotData_1 (u8 * report, u8 * output)
{
u32 Ret_u32;

    // Slot name, Slot password. Don't write it into flash
    Ret_u32 = PWS_WriteSlotData_1 (report[1], &report[2], &report[2 + PWS_SLOTNAME_LENGTH]);

    if (TRUE == Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}

/*******************************************************************************

  cmd_getPasswordSafeSetSlotData_2

  Changes
  Date      Reviewer        Info
  30.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeSetSlotData_2 (u8 * report, u8 * output)
{
u32 Ret_u32;

    // Slot login name and write to flash
    Ret_u32 = PWS_WriteSlotData_2 (report[1], &report[2]);

    if (TRUE == Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}

/*******************************************************************************

  cmd_getPasswordSafeEraseSlot

  Changes
  Date      Reviewer        Info
  30.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeEraseSlot (u8 * report, u8 * output)
{
u32 Ret_u32;

    Ret_u32 = PWS_EraseSlot (report[1]);
    if (TRUE == Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}

/*******************************************************************************

  cmd_getPasswordSafeEnable

  Changes
  Date      Reviewer        Info
  01.08.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeEnable (u8 * report, u8 * output)
{
    u8 Ret_u8;

    Ret_u8 = PWS_EnableAccess (&report[1]);
    if (PWS_RETURN_SUCCESS == Ret_u8)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else if (PWS_RETURN_AES_ERROR == Ret_u8){
//      output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_AES_DEC_FAILED; //FIXME enable in later releases
      output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_UNKNOWN_ERROR;
    }
    else if (PWS_RETURN_WRONG_PASSWORD == Ret_u8) {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
    }
    else {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_UNKNOWN_ERROR;
    }

    return (0);
}

/*******************************************************************************

  cmd_getPasswordSafeInitKey

  Changes
  Date      Reviewer        Info
  30.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeInitKey (u8 * report, u8 * output)
{
u32 Ret_u32;

    Ret_u32 = PWS_InitKey ();
    if (TRUE == Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}

/*******************************************************************************

  cmd_getPasswordSafeSendData

  Changes
  Date      Reviewer        Info
  01.08.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getPasswordSafeSendData (u8 * report, u8 * output)
{
u32 Ret_u32;

    Ret_u32 = PWS_SendData (report[1], report[2]);
    if (TRUE == Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;
    }
    else
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_NOT_AUTHORIZED;
    }

    return (0);
}

/*******************************************************************************

  cmd_getSdCardHighWaterMark

  Changes
  Date      Reviewer        Info
  08.10.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

extern u32 sd_MaxAccessedBlockReadMin_u32;
extern u32 sd_MaxAccessedBlockReadMax_u32;
extern u32 sd_MaxAccessedBlockWriteMin_u32;
extern u32 sd_MaxAccessedBlockWriteMax_u32;

#define SD_SLOT    0

#define DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MIN         0
#define DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MAX         1
#define DEVICE_HIGH_WATERMARK_SD_CARD_READ_MIN          2
#define DEVICE_HIGH_WATERMARK_SD_CARD_READ_MAX          3

u8 cmd_getSdCardHighWaterMark (u8 * report, u8 * output)
{
u32 Blockcount_u32;

    sd_mmc_mci_read_capacity (SD_SLOT, (u32 *) & Blockcount_u32);

    // +-1 needed by rounding cuts
    output[OUTPUT_CMD_RESULT_OFFSET + DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MIN] = (u8) ((u64) sd_MaxAccessedBlockWriteMin_u32 * (u64) 100 / (u64) Blockcount_u32) + 1;   // In
                                                                                                                                                                        // %
                                                                                                                                                                        // of
                                                                                                                                                                        // sd
                                                                                                                                                                        // size
    output[OUTPUT_CMD_RESULT_OFFSET + DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MAX] = (u8) ((u64) sd_MaxAccessedBlockWriteMax_u32 * (u64) 100 / (u64) Blockcount_u32) - 1;   // In
                                                                                                                                                                        // %
                                                                                                                                                                        // of
                                                                                                                                                                        // sd
                                                                                                                                                                        // size
    output[OUTPUT_CMD_RESULT_OFFSET + DEVICE_HIGH_WATERMARK_SD_CARD_READ_MIN] = (u8) ((u64) sd_MaxAccessedBlockReadMin_u32 * (u64) 100 / (u64) Blockcount_u32) + 1; // In
                                                                                                                                                                    // %
                                                                                                                                                                    // of
                                                                                                                                                                    // sd
                                                                                                                                                                    // size
    output[OUTPUT_CMD_RESULT_OFFSET + DEVICE_HIGH_WATERMARK_SD_CARD_READ_MAX] = (u8) ((u64) sd_MaxAccessedBlockReadMax_u32 * (u64) 100 / (u64) Blockcount_u32) - 1; // In
                                                                                                                                                                    // %
                                                                                                                                                                    // of
                                                                                                                                                                    // sd
                                                                                                                                                                    // size

    output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_OK;

    return (0);
}


/*******************************************************************************

  cmd_getFactoryReset

  Changes
  Date      Reviewer        Info
  21.11.14  RB              Creation of function

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getFactoryReset (u8 * report, u8 * output)
{
u8 admin_password[26];
u32 Ret_u32;

    memset (admin_password, 0, 26);
    memcpy (admin_password, report + 1, 25);

    Ret_u32 = LA_OpenPGP_V20_Test_SendAdminPW ((unsigned char *) admin_password);
    if (TRUE != Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
        return (FALSE); // wrong card password
    }

    LA_OpenPGP_V20_ResetCard ();    // Factory reset smartcard

    EraseLocalFlashKeyValues_u32 ();    // Factory reset local flash

//    InitUpdatePinHashInFlash ();    // Don't remove old password

    return (TRUE);
}

/*******************************************************************************

  cmd_getResetStick

  Changes
  Date      Reviewer        Info
  13.06.16  RB              Creation of function

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 cmd_getResetStick (u8 * report, u8 * output)
{
    u8 user_password[26];
    u32 Ret_u32;

    memset (user_password, 0, 26);
    memcpy (user_password, report + 1, 25);

    Ret_u32 = LA_OpenPGP_V20_Test_SendUserPW2 ((unsigned char *) user_password);
    if (TRUE != Ret_u32)
    {
        output[OUTPUT_CMD_STATUS_OFFSET] = CMD_STATUS_WRONG_PASSWORD;
        return (FALSE); // wrong card password
    }

    DelayMs (500);

    DFU_ResetCPU ();

    return (TRUE);
}


/*******************************************************************************

  CopyUpdateToHIDData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CopyUpdateToHIDData (void)
{
    memcpy (&HID_GetReport_Value_tmp[OUTPUT_CMD_RESULT_STICK20_STATUS_START], (void *) &HID_Stick20Status_st, sizeof (HID_Stick20Status_est));
}

/*******************************************************************************

  UpdateStick20Command

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void UpdateStick20Command (u8 Status_u8, u8 ProgressBarValue_u8)
{
    HID_Stick20Status_st.Status_u8 = Status_u8;
    HID_Stick20Status_st.ProgressBarValue_u8 = ProgressBarValue_u8;

    CopyUpdateToHIDData ();

    u32 calculated_crc32;

    calculated_crc32 = generateCRC(HID_GetReport_Value_tmp);
    HID_GetReport_Value_tmp[OUTPUT_CRC_OFFSET] = calculated_crc32 & 0xFF;
    HID_GetReport_Value_tmp[OUTPUT_CRC_OFFSET + 1] = (calculated_crc32 >> 8) & 0xFF;
    HID_GetReport_Value_tmp[OUTPUT_CRC_OFFSET + 2] = (calculated_crc32 >> 16) & 0xFF;
    HID_GetReport_Value_tmp[OUTPUT_CRC_OFFSET + 3] = (calculated_crc32 >> 24) & 0xFF;
}

/*******************************************************************************

  StartStick20Command

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void StartStick20Command (u8 Command_u8)
{
    HID_Stick20Status_st.CommandCounter_u8 += 1;
    HID_Stick20Status_st.LastCommand_u8 = Command_u8;
    HID_Stick20Status_st.Status_u8 = OUTPUT_CMD_STICK20_STATUS_BUSY;
    HID_Stick20Status_st.ProgressBarValue_u8 = 0;

    CopyUpdateToHIDData ();
}

/*******************************************************************************

  InitStick20Interface

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void InitStick20Interface (void)
{
    HID_Stick20Status_st.CommandCounter_u8 = 0;
    HID_Stick20Status_st.LastCommand_u8 = 0xFF;
    HID_Stick20Status_st.Status_u8 = OUTPUT_CMD_STICK20_STATUS_IDLE;
    HID_Stick20Status_st.ProgressBarValue_u8 = 0;

    CopyUpdateToHIDData ();
}

/*******************************************************************************

  OTP_main

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void OTP_main (void)
{
    static u8 InitFlag_u8 = 0;

    if (0 == InitFlag_u8)
    {
        InitFlag_u8 = 1;
        InitStick20Interface ();
#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
        InitStick20DebugData (HID_GetReport_Value_tmp);
#endif
    }


    /* Endless loop after USB startup */
    // while (1)
    {
        if (OTP_device_status == STATUS_RECEIVED_REPORT)
        {
            OTP_device_status = STATUS_BUSY;
            // CI_StringOut ("-B-");
            parse_report (HID_SetReport_Value_tmp, HID_GetReport_Value_tmp);
            OTP_device_status = STATUS_READY;
            // CI_StringOut ("-R-");

        }

        if (numLockClicked)
        {
            numLockClicked = 0;
            // sendString("NumLock",7);

    u8 slot_number = ((u8 *) SLOTS_ADDRESS + GLOBAL_CONFIG_OFFSET)[0];
            if (slot_number <= 1)
            {
    u8 programmed = *((u8 *) hotp_slots[slot_number]);

                if (programmed == 0x01)
                {
    u32 code = get_code_from_hotp_slot (slot_number);
    u8 config = get_hotp_slot_config (slot_number);

                    if (config & (1 << SLOT_CONFIG_TOKENID))
                        sendString ((char *) (hotp_slots[slot_number] + TOKEN_ID_OFFSET), 12);

                    if (config & (1 << SLOT_CONFIG_DIGITS))
                        sendNumberN (code, 8);
                    else
                        sendNumberN (code, 6);

                    if (config & (1 << SLOT_CONFIG_ENTER))
                        sendEnter ();
                }
            }
        }
        if (capsLockClicked)
        {
            capsLockClicked = 0;
            // sendString("CapsLock",8);

    u8 slot_number = ((u8 *) SLOTS_ADDRESS + GLOBAL_CONFIG_OFFSET)[1];
            if (slot_number <= 1)
            {
    u8 programmed = *((u8 *) hotp_slots[slot_number]);

                if (programmed == 0x01)
                {
    u32 code = get_code_from_hotp_slot (slot_number);
    u8 config = get_hotp_slot_config (slot_number);

                    if (config & (1 << SLOT_CONFIG_TOKENID))
                        sendString ((char *) (hotp_slots[slot_number] + TOKEN_ID_OFFSET), 12);

                    if (config & (1 << SLOT_CONFIG_DIGITS))
                        sendNumberN (code, 8);
                    else
                        sendNumberN (code, 6);

                    if (config & (1 << SLOT_CONFIG_ENTER))
                        sendEnter ();
                }
            }
        }

        if (scrollLockClicked)
        {
            scrollLockClicked = 0;
            // sendString("ScrollLock",10);

    u8 slot_number = ((u8 *) SLOTS_ADDRESS + GLOBAL_CONFIG_OFFSET)[2];
            if (slot_number <= 1)
            {
    u8 programmed = *((u8 *) hotp_slots[slot_number]);

                if (programmed == 0x01)
                {
    u32 code = get_code_from_hotp_slot (slot_number);
    u8 config = get_hotp_slot_config (slot_number);

                    if (config & (1 << SLOT_CONFIG_TOKENID))
                        sendString ((char *) (hotp_slots[slot_number] + TOKEN_ID_OFFSET), 12);

                    if (config & (1 << SLOT_CONFIG_DIGITS))
                        sendNumberN (code, 8);
                    else
                        sendNumberN (code, 6);

                    if (config & (1 << SLOT_CONFIG_ENTER))
                        sendEnter ();
                }
            }

        }
    }
}
