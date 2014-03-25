/*
* Author: Copyright (C) Andrzej Surowiec 2012
*												
*
* This file is part of GPF Crypto Stick.
*
* GPF Crypto Stick is free software: you can redistribute it and/or modify
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
* This file contains modifications done by Rudolf Boeddeker
* For the modifications applies:
*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-16
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

//#ifdef NOT_USED

#include "global.h"
#include "tools.h"

#include "keyboard.h"
#include "hotp.h"
#include "report_protocol.h"
#include "string.h"
#include "time.h"


#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"
#include "USB_CCID/USB_CCID.h"

#include "BUFFERED_SIO.h"
#include "Interpreter.h"

#include "HighLevelFunctions/FlashStorage.h"
#include "User_Interface/html_io.h"




/*******************************************************************************

 Local defines

*******************************************************************************/


#define DEBUG_REPORT_PROTOCOL

#ifdef DEBUG_REPORT_PROTOCOL
#else
  #define CI_LocalPrintf(...)
  #define CI_TickLocalPrintf(...)
  #define CI_StringOut(...)
  #define CI_Print8BitValue(...)
  #define HexPrint(...)
#endif

//#define KEYBOARD_FEATURE_COUNT                64

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

u8 HID_SetReport_Value[KEYBOARD_FEATURE_COUNT];
u8 HID_GetReport_Value[KEYBOARD_FEATURE_COUNT];

u8 HID_SetReport_Value_tmp[KEYBOARD_FEATURE_COUNT];
u8 HID_GetReport_Value_tmp[KEYBOARD_FEATURE_COUNT];

u8  OTP_device_status = STATUS_READY;
u8  temp_password[25];
u8  tmp_password_set =0;
u32 authorized_crc   =0xFFFFFFFF;

// For stick 2.0
u8 HID_CmdGet_u8  = HTML_CMD_NOTHING;   // Todo: make function
u8 HID_String_au8[50];

HID_Stick20Status_est   HID_Stick20Status_st;

HID_Stick20SendData_est   HID_Stick20SendData_st;


#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID

/*******************************************************************************

  InitStick20DebugData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void InitStick20DebugData (u8 *output)
{
  HID_Stick20SendData_st.SendCounter_u8     = 0;
  HID_Stick20SendData_st.SendDataType_u8    = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_NONE;
  HID_Stick20SendData_st.FollowBytesFlag_u8 = 0; // False
  HID_Stick20SendData_st.SendSize_u8        = 0;

  memset (&HID_Stick20SendData_st.SendData_u8[0],0,OUTPUT_CMD_STICK20_SEND_DATA_SIZE);

  memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START],(void*)&HID_Stick20SendData_st,sizeof (HID_Stick20SendData_est));
}

/*******************************************************************************

  Stick20HIDSendDebugData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void Stick20HIDSendDebugData (u8 *output)
{
  u8  Size_u8;
  u32 calculated_crc32;

  HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG;

  Size_u8 = BUFFERED_SIO_HID_GetSendChars (&HID_Stick20SendData_st.SendData_u8[0] ,OUTPUT_CMD_STICK20_SEND_DATA_SIZE);

  // Send buffer empty ?
  if (TRUE != BUFFERED_SIO_HID_TxEmpty ())
  {
    HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;   // No
  }
  else
  {
    HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;   // Yes
  }

  HID_Stick20SendData_st.SendCounter_u8  += 1;
  HID_Stick20SendData_st.SendDataType_u8  = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG;
  HID_Stick20SendData_st.SendSize_u8      = Size_u8;

  memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START],(void*)&HID_Stick20SendData_st,sizeof (HID_Stick20SendData_est));

  // Build CRC, can overwritten by new CRC computation
  calculated_crc32 = generateCRC (output);

  output[OUTPUT_CRC_OFFSET]   =  calculated_crc32      & 0xFF;
  output[OUTPUT_CRC_OFFSET+1] = (calculated_crc32>>8)  & 0xFF;
  output[OUTPUT_CRC_OFFSET+2] = (calculated_crc32>>16) & 0xFF;
  output[OUTPUT_CRC_OFFSET+3] = (calculated_crc32>>24) & 0xFF;
}


#endif // STICK_20_SEND_DEBUGINFOS_VIA_HID

/*******************************************************************************

  Stick20HIDInitSendConfoguration

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 Stick20HIDSendConfigurationState_u8      = 0;

u8 Stick20HIDInitSendConfoguration (void)
{
  CI_StringOut ("Init status data\r\n");

  Stick20HIDSendConfigurationState_u8      = 1;

  return (TRUE);
}

/*******************************************************************************

  Stick20HIDSendAccessStatusData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 Stick20HIDSendAccessStatusData (u8 *output)
{
  u32 calculated_crc32;
  typeStick20Configuration_st Stick20Configuration_st;
  u8  text_au8[10];

  SendStickStatusToHID (&Stick20Configuration_st);

  HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_STATUS;

  CI_StringOut ("Send configuration data\r\n");
  HID_Stick20SendData_st.FollowBytesFlag_u8  = 0;     // No
  HID_Stick20SendData_st.SendSize_u8         = 1;

  memcpy (&HID_Stick20SendData_st.SendData_u8[0],&Stick20Configuration_st,sizeof (typeStick20Configuration_st));

  CI_StringOut ("Firmware version  ");

  itoa (Stick20Configuration_st.VersionInfo_au8[0],text_au8);
  CI_StringOut (text_au8);
  CI_StringOut (".");
  itoa (Stick20Configuration_st.VersionInfo_au8[1],text_au8);
  CI_StringOut (text_au8);
  CI_StringOut ("\r\n");

  if (READ_WRITE_ACTIVE == Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8)
  {
    CI_StringOut ("Uncyrpted Volume  READ/WRITE active\r\n");
  }
  else
  {
    CI_StringOut ("Uncyrpted Volume  READ ONLY active\r\n");
  }

  if (0 != (Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_CRYPTED_VOLUME_BIT_PLACE)))
  {
    CI_StringOut ("Crypted volume    active\r\n");
  }
  else
  {
    CI_StringOut ("Crypted volume    not active\r\n");
  }

  if (0 != (Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_HIDDEN_VOLUME_BIT_PLACE)))
  {
    CI_StringOut ("Hidden volume     active\r\n");
  }



  if (0 != (Stick20Configuration_st.NewSDCardFound_u8 & 0x01))
  {
    CI_StringOut ("*** New SD card found - Change Counter:");
  }
  else
  {
    CI_StringOut ("SD card              - Change Counter:");
  }
  itoa ((Stick20Configuration_st.SDFillWithRandomChars_u8 >> 1),text_au8);
  CI_StringOut (text_au8);
  CI_StringOut ("\r\n");

  if (0 == (Stick20Configuration_st.SDFillWithRandomChars_u8 & 0x01))
  {
    CI_StringOut("Not filled with random chars - Change Counter:");
  }
  else
  {
    CI_StringOut("Filled with random    - Change Counter:");
  }
  itoa ((Stick20Configuration_st.SDFillWithRandomChars_u8 >> 1),text_au8);
  CI_StringOut (text_au8);
  CI_StringOut ("\r\n");


  HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;     // No
  HID_Stick20SendData_st.SendSize_u8        = sizeof (typeStick20Configuration_st);

  // Copy send data to transfer struct
  memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START],(void*)&HID_Stick20SendData_st,sizeof (HID_Stick20SendData_est));

  // Build CRC, can overwritten by new CRC computation
  calculated_crc32 = generateCRC (output);

  output[OUTPUT_CRC_OFFSET]   =  calculated_crc32      & 0xFF;
  output[OUTPUT_CRC_OFFSET+1] = (calculated_crc32>>8)  & 0xFF;
  output[OUTPUT_CRC_OFFSET+2] = (calculated_crc32>>16) & 0xFF;
  output[OUTPUT_CRC_OFFSET+3] = (calculated_crc32>>24) & 0xFF;

  Stick20HIDSendConfigurationState_u8              = 0;     // Send status done

  return (TRUE);
}
/*******************************************************************************

  Stick20HIDInitSendMatrixData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8  Stick20HIDSendMatrixState_u8      = 0;
u8 *Stick20HIDSendMatrixDataBlock_au8 = NULL;

u8 Stick20HIDInitSendMatrixData (u8 *PasswordMatrixData_au8)
{
  CI_StringOut ("Init SendMatrix\r\n");

  Stick20HIDSendMatrixState_u8      = 1;
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

u8 Stick20HIDSendMatrixData (u8 *output)
{
  u32 calculated_crc32;

  HID_Stick20SendData_st.SendDataType_u8 = OUTPUT_CMD_STICK20_SEND_DATA_TYPE_PW_DATA;

  switch (Stick20HIDSendMatrixState_u8)
  {
    case 0 :                              // IDLE - wait
      break;
    case 1 :
      CI_StringOut ("SendMatrix S1\r\n");
      HID_Stick20SendData_st.SendData_u8[0]     = 1;     // Set state in sendblock
      HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;     // Yes
      HID_Stick20SendData_st.SendSize_u8        = 1;
      Stick20HIDSendMatrixState_u8              = 2;     // Next state
      break;
    case 2 :
      CI_StringOut ("SendMatrix S2\r\n");
      HID_Stick20SendData_st.SendData_u8[0]     = 2;     // Set state in sendblock
      memcpy (&HID_Stick20SendData_st.SendData_u8[1],&Stick20HIDSendMatrixDataBlock_au8[0],20);
      HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;     // Yes
      HID_Stick20SendData_st.SendSize_u8        = 21;
      Stick20HIDSendMatrixState_u8              = 3;     // Next state
      break;
    case 3 :
      CI_StringOut ("SendMatrix S3\r\n");
      HID_Stick20SendData_st.SendData_u8[0]     = 3;     // Set state in sendblock
      memcpy (&HID_Stick20SendData_st.SendData_u8[1],&Stick20HIDSendMatrixDataBlock_au8[20],20);
      HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;     // Yes
      HID_Stick20SendData_st.SendSize_u8        = 21;
      Stick20HIDSendMatrixState_u8              = 4;     // Next state
      break;
    case 4 :
      CI_StringOut ("SendMatrix S4\r\n");
      HID_Stick20SendData_st.SendData_u8[0]     = 4;     // Set state in sendblock
      memcpy (&HID_Stick20SendData_st.SendData_u8[1],&Stick20HIDSendMatrixDataBlock_au8[40],20);
      HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;     // Yes
      HID_Stick20SendData_st.SendSize_u8        = 21;
      Stick20HIDSendMatrixState_u8              = 5;     // Next state
      break;
    case 5 :
      CI_StringOut ("SendMatrix S5\r\n");
      HID_Stick20SendData_st.SendData_u8[0]     = 5;     // Set state in sendblock
      memcpy (&HID_Stick20SendData_st.SendData_u8[1],&Stick20HIDSendMatrixDataBlock_au8[60],20);
      HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;     // Yes
      HID_Stick20SendData_st.SendSize_u8        = 21;
      Stick20HIDSendMatrixState_u8              = 6;     // Next state
      break;
    case 6 :
      CI_StringOut ("SendMatrix S6\r\n");
      HID_Stick20SendData_st.SendData_u8[0]     = 6;     // Set state in sendblock
      memcpy (&HID_Stick20SendData_st.SendData_u8[1],&Stick20HIDSendMatrixDataBlock_au8[80],20);
      HID_Stick20SendData_st.FollowBytesFlag_u8 = 1;     // Yes
      HID_Stick20SendData_st.SendSize_u8        = 1;
      Stick20HIDSendMatrixState_u8              = 7;     // Next state
      break;
    case 7 :
      CI_StringOut ("SendMatrix S7\r\n");
      HID_Stick20SendData_st.SendData_u8[0]     = 7;     // Set state in sendblock

      // Build CRC over matrix password data
      calculated_crc32 = generateCRC_len (Stick20HIDSendMatrixDataBlock_au8,100/4);
      HID_Stick20SendData_st.SendData_u8[1] =  calculated_crc32      & 0xFF;
      HID_Stick20SendData_st.SendData_u8[2] = (calculated_crc32>>8)  & 0xFF;
      HID_Stick20SendData_st.SendData_u8[3] = (calculated_crc32>>16) & 0xFF;
      HID_Stick20SendData_st.SendData_u8[4] = (calculated_crc32>>24) & 0xFF;

      HID_Stick20SendData_st.FollowBytesFlag_u8 = 0;     // No
      HID_Stick20SendData_st.SendSize_u8        = 5;
      Stick20HIDSendMatrixState_u8              = 0;     // Next state - idle
      break;
    default :
      break;
  }

  HID_Stick20SendData_st.SendCounter_u8  += 1;

  // Copy send data to transfer struct
  memcpy (&output[OUTPUT_CMD_RESULT_STICK20_DATA_START],(void*)&HID_Stick20SendData_st,sizeof (HID_Stick20SendData_est));

  // Build CRC, can overwritten by new CRC computation
  calculated_crc32 = generateCRC (output);

  output[OUTPUT_CRC_OFFSET]   =  calculated_crc32      & 0xFF;
  output[OUTPUT_CRC_OFFSET+1] = (calculated_crc32>>8)  & 0xFF;
  output[OUTPUT_CRC_OFFSET+2] = (calculated_crc32>>16) & 0xFF;
  output[OUTPUT_CRC_OFFSET+3] = (calculated_crc32>>24) & 0xFF;

  return (TRUE);
}


/*******************************************************************************

  parse_report_stick_20

  Reviews
  Date      Reviewer        Info

  In USB context > don't use printf

*******************************************************************************/

u8 parse_report_stick_20 (u8 *report,u8 *output)
{
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

u8 parse_report(u8 *report,u8 *output)
{
	u8 cmd_type=report[CMD_TYPE_OFFSET];
	u32 received_crc32;
	u32 calculated_crc32;
	u8 i;
	u8 not_authorized=0;

  static u8 oldStatus;
  static u8 initOldStatus = FALSE;

  if (FALSE == initOldStatus)
  {
    oldStatus     = output[OUTPUT_CMD_STATUS_OFFSET];
    initOldStatus = TRUE;
  }

// Init output block
	memset(output,0,KEYBOARD_FEATURE_COUNT);

// Set last command value
	output[OUTPUT_LAST_CMD_TYPE_OFFSET]=cmd_type;

// Get CRC from HID report block
  received_crc32 = getu32(report+KEYBOARD_FEATURE_COUNT-4);

// Check data with CRC
  calculated_crc32 = generateCRC (report);

// Store the CRC as the CMD_CRC
	output[OUTPUT_CMD_CRC_OFFSET]  = received_crc32      & 0xFF;
	output[OUTPUT_CMD_CRC_OFFSET+1]=(received_crc32>> 8) & 0xFF;
	output[OUTPUT_CMD_CRC_OFFSET+2]=(received_crc32>>16) & 0xFF;
	output[OUTPUT_CMD_CRC_OFFSET+3]=(received_crc32>>24) & 0xFF;


  if (calculated_crc32 != received_crc32)
  {
    CI_StringOut ("CRC Wrong\r\n");
    CI_StringOut ("Get     ");
    for (i=0;i<4;i++)
    {
        CI_Print8BitValue (output[OUTPUT_CMD_CRC_OFFSET+3-i]);
    }
    CI_StringOut ("\r\n");

    CI_StringOut ("Compute ");
    for (i=0;i<4;i++)
    {
        CI_Print8BitValue (output[OUTPUT_CMD_CRC_OFFSET+3-i]);
    }
    CI_StringOut ("\r\n");
  }


// Stick 1.0 commands
	if (calculated_crc32==received_crc32)
	{
		switch (cmd_type){

      case CMD_GET_STATUS:
        CI_StringOut ("Get CMD_GET_STATUS\r\n");
        cmd_get_status(report,output);
        break;

      case CMD_WRITE_TO_SLOT:
        {
          u8 text[10];
          CI_StringOut ("Get CMD_WRITE_TO_SLOT - slot ");
          itoa ((u8)report[1],text);
          CI_StringOut (text);
          CI_StringOut ("\r\n");
        }
        if (calculated_crc32==authorized_crc)
        {
          cmd_write_to_slot(report,output);
        }
        else
        {
          not_authorized=1;
        }
        break;

      case CMD_READ_SLOT_NAME:
        {
          u8 text[10];
          CI_StringOut ("Get CMD_READ_SLOT_NAME - slot ");
          itoa ((u8)report[1],text);
          CI_StringOut (text);
          CI_StringOut ("\r\n");
        }
        cmd_read_slot_name(report,output);
        break;

      case CMD_READ_SLOT:
        CI_StringOut ("Get CMD_READ_SLOT\r\n");
        if (calculated_crc32==authorized_crc)
        cmd_read_slot(report,output);
        else
        not_authorized=1;
        break;

      case CMD_GET_CODE:
        CI_StringOut ("Get CMD_GET_CODE\r\n");
        cmd_get_code(report,output);
        break;

      case CMD_WRITE_CONFIG:
        CI_StringOut ("Get CMD_WRITE_CONFIG\r\n");
        if (calculated_crc32==authorized_crc)
        cmd_write_config(report,output);
        else
        not_authorized=1;
        break;

      case CMD_ERASE_SLOT:
        CI_StringOut ("Get CMD_ERASE_SLOT\r\n");
        if (calculated_crc32==authorized_crc)
          cmd_erase_slot(report,output);
        else
          not_authorized=1;
        break;

      case CMD_FIRST_AUTHENTICATE:
        CI_StringOut ("Get CMD_FIRST_AUTHENTICATE\r\n");
        cmd_first_authenticate(report,output);
        break;

      case CMD_AUTHORIZE:
        CI_StringOut ("Get CMD_AUTHORIZE\r\n");
        cmd_authorize(report,output);
        break;
    }
    if (not_authorized)
    {
      CI_StringOut ("Get CMD_AUTHORIZE\r\n");
      output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_NOT_AUTHORIZED;
    }

    if (calculated_crc32==authorized_crc)
    {
      authorized_crc=0xFFFFFFFF;
    }
	}
  else
  {
    output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_CRC;
  }

  // Stick 2.0 commands
  if (STICK20_CMD_START_VALUE <=cmd_type)
  {
    if ((calculated_crc32==received_crc32))
    {
      switch (cmd_type){
        case STICK20_CMD_ENABLE_CRYPTED_PARI:
          CI_StringOut ("Get STICK20_CMD_ENABLE_CRYPTED_PARI Password=-");
          CI_StringOut ((char*)&report[1]);
          CI_StringOut ("-\r\n");

          StartStick20Command (STICK20_CMD_ENABLE_CRYPTED_PARI);

          // Tranfer data to other context
          HID_CmdGet_u8  = HTML_CMD_ENABLE_AES_LUN;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_DISABLE_CRYPTED_PARI:
          CI_StringOut ("Get STICK20_CMD_DISABLE_CRYPTED_PARI\r\n");

          StartStick20Command (STICK20_CMD_DISABLE_CRYPTED_PARI);

          // Tranfer data to other context
          HID_CmdGet_u8  = HTML_CMD_DISABLE_AES_LUN;
          memset (HID_String_au8,0,33);
          break;


        case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI:
          CI_StringOut ("Get STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI Password=-");
          CI_StringOut ((char*)&report[1]);
          CI_StringOut ("-\r\n");

          StartStick20Command (STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_ENABLE_HIDDEN_AES_LUN;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI:
          CI_StringOut ("Get STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI\r\n");

          StartStick20Command (STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_DISABLE_HIDDEN_AES_LUN;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_ENABLE_FIRMWARE_UPDATE:
          CI_StringOut ("Get STICK20_CMD_ENABLE_FIRMWARE_UPDATE\r\n");

          StartStick20Command (STICK20_CMD_ENABLE_FIRMWARE_UPDATE);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_ENABLE_FIRMWARE_UPDATE;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE:
          CI_StringOut ("Get STICK20_CMD_EXPORT_FIRMWARE_TO_FILE\r\n");

          StartStick20Command (STICK20_CMD_EXPORT_FIRMWARE_TO_FILE);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_EXPORT_BINARY;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_GENERATE_NEW_KEYS:
          CI_StringOut ("Get STICK20_CMD_GENERATE_NEW_KEYS\r\n");

          StartStick20Command (STICK20_CMD_GENERATE_NEW_KEYS);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_GENERATE_NEW_KEYS;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS:
          CI_StringOut ("Get STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS\r\n");

          StartStick20Command (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_FILL_SD_WITH_RANDOM_CHARS;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_SEND_PASSWORD_MATRIX :
          CI_StringOut ("Get STICK20_CMD_SEND_PASSWORD_MATRIX\r\n");

          StartStick20Command (STICK20_CMD_SEND_PASSWORD_MATRIX);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_SEND_PASSWORD_MATRIX;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA :
          CI_StringOut ("Get STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA\r\n");

          StartStick20Command (STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_SEND_PASSWORD_MATRIX_PINDATA;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP :
          CI_StringOut ("Get STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP\r\n");

          StartStick20Command (STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_SEND_PASSWORD_MATRIX_SETUP;
          memcpy (HID_String_au8,&report[1],33);
          break;


        case STICK20_CMD_GET_DEVICE_STATUS :
          CI_StringOut ("Get STICK20_CMD_GET_DEVICE_STATUS\r\n");

          StartStick20Command (STICK20_CMD_GET_DEVICE_STATUS);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_GET_DEVICE_STATUS;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_SEND_DEVICE_STATUS :
          CI_StringOut ("Get STICK20_CMD_SEND_DEVICE_STATUS\r\n");

          StartStick20Command (STICK20_CMD_SEND_DEVICE_STATUS);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_WRITE_STATUS_DATA;
          memcpy (HID_String_au8,&report[1],33);
          break;

  /*
        case STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD :
          CI_StringOut ("Get STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD\r\n");

          StartStick20Command (STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_SEND_HIDDEN_VOLUME_PASSWORD;
          memcpy (HID_String_au8,&report[1],33);
          break;
  */
        case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP :
          CI_StringOut ("Get STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP\r\n");

          StartStick20Command (STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_SEND_HIDDEN_VOLUME_SETUP;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_SEND_PASSWORD :
          CI_StringOut ("Get STICK20_CMD_SEND_PASSWORD\r\n");

          StartStick20Command (STICK20_CMD_SEND_PASSWORD);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_SEND_PASSWORD;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_SEND_NEW_PASSWORD :
          CI_StringOut ("Get STICK20_CMD_SEND_NEW_PASSWORD\r\n");

          StartStick20Command (STICK20_CMD_SEND_NEW_PASSWORD);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_SEND_NEW_PASSWORD;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN :
          CI_StringOut ("Get STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN\r\n");

          StartStick20Command (STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_ENABLE_READONLY_UNCRYPTED_LUN;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
          CI_StringOut ("Get STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN\r\n");

          StartStick20Command (STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_ENABLE_READWRITE_UNCRYPTED_LUN;
          memcpy (HID_String_au8,&report[1],33);
          break;

        case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND :
          CI_StringOut ("Get STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND\r\n");

          StartStick20Command (STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND);

          // Transfer data to other context
          HID_CmdGet_u8  = HTML_CMD_CLEAR_NEW_SD_CARD_FOUND;
          memcpy (HID_String_au8,&report[1],33);
          break;


        default:
          break;

      }
    }
    else
    {
      output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_CRC;
    }

    // copy Stick 2.0 output status	to HID output
    memcpy (&output[OUTPUT_CMD_RESULT_STICK20_STATUS_START],(void*)&HID_Stick20Status_st,sizeof (HID_Stick20Status_est));

  }


// Stick 20 Send password matrix ?
  if (0 != Stick20HIDSendMatrixState_u8)
  {
    Stick20HIDSendMatrixData (output);
  }
#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
  else   // Send debug data
  {
    Stick20HIDSendDebugData (output);
  }
#endif

  calculated_crc32 = generateCRC (output);

	output[OUTPUT_CRC_OFFSET]   =  calculated_crc32      & 0xFF;
	output[OUTPUT_CRC_OFFSET+1] = (calculated_crc32>>8)  & 0xFF;
	output[OUTPUT_CRC_OFFSET+2] = (calculated_crc32>>16) & 0xFF;
	output[OUTPUT_CRC_OFFSET+3] = (calculated_crc32>>24) & 0xFF;

  if (output[OUTPUT_CMD_STATUS_OFFSET] != oldStatus)
  {
    oldStatus = output[OUTPUT_CMD_STATUS_OFFSET];

// Print the new output status
    switch (oldStatus)
    {
        case CMD_STATUS_OK                  :
          CI_StringOut ("CMD_STATUS_OK\r\n");
          break;
        case CMD_STATUS_WRONG_CRC           :
          CI_StringOut ("CMD_STATUS_WRONG_CRC\r\n");
          break;
        case CMD_STATUS_WRONG_SLOT          :
          CI_StringOut ("CMD_STATUS_WRONG_SLOT\r\n");
          break;
        case CMD_STATUS_SLOT_NOT_PROGRAMMED :
          CI_StringOut ("CMD_STATUS_SLOT_NOT_PROGRAMMED\r\n");
          break;
        case CMD_STATUS_WRONG_PASSWORD      :
          CI_StringOut ("CMD_STATUS_SLOT_NOT_PROGRAMMED\r\n");
          break;
        case CMD_STATUS_NOT_AUTHORIZED      :
          CI_StringOut ("CMD_STATUS_NOT_AUTHORIZED\r\n");
          break;
    }
  }
/* USB Debug
  CI_StringOut ("\n\rSend ");
  for (i=0;i<KEYBOARD_FEATURE_COUNT;i++)
  {
      CI_Print8BitValue (output[i]);
  }
  CI_StringOut ("\n\r");
*/
  return 0;
}
/*******************************************************************************

  cmd_get_status

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_get_status(u8 *report,u8 *output)
{
	output[OUTPUT_CMD_RESULT_OFFSET]    =FIRMWARE_VERSION&0xFF;
	output[OUTPUT_CMD_RESULT_OFFSET+1]  =(FIRMWARE_VERSION>>8)&0xFF;
/*
	output[OUTPUT_CMD_RESULT_OFFSET+2]  =cardSerial&0xFF;
	output[OUTPUT_CMD_RESULT_OFFSET+3]  =(cardSerial>>8)&0xFF;
	output[OUTPUT_CMD_RESULT_OFFSET+4]  =(cardSerial>>16)&0xFF;
	output[OUTPUT_CMD_RESULT_OFFSET+5]  =(cardSerial>>24)&0xFF;
*/
	memcpy(output+OUTPUT_CMD_RESULT_OFFSET+6,(u8 *)SLOTS_ADDRESS+GLOBAL_CONFIG_OFFSET,3);

	return 0;
}

/*******************************************************************************

  cmd_write_to_slot

  Write data from pc into flash

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_write_to_slot(u8 *report,u8 *output)
{

	u8 slot_no=report[CMD_WTS_SLOT_NUMBER_OFFSET];
	u8 slot_tmp[64];//this is will be the new slot contents

	memset(slot_tmp,0,64);

	slot_tmp[0]=0x01; //marks slot as programmed

	memcpy(slot_tmp+1,report+CMD_WTS_SLOT_NAME_OFFSET,49);

	if ((slot_no >= 0x10) && (slot_no < 0x10 + NUMBER_OF_HOTP_SLOTS))        // HOTP slot
	{
		slot_no = slot_no & 0x0F;
		
		u64 counter = getu64(report+CMD_WTS_COUNTER_OFFSET);

		set_counter_value(hotp_slot_counters[slot_no], counter);

		write_to_slot(slot_tmp, hotp_slot_offsets[slot_no], 64);
	}
	else if ((slot_no >= 0x20) && (slot_no < 0x20 + NUMBER_OF_TOTP_SLOTS))   // TOTP slot
	{
		slot_no = slot_no & 0x0F;
		
		write_to_slot(slot_tmp, totp_slot_offsets[slot_no], 64);
	}
	else{
		output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_SLOT;
	}

	return 0;
}

/*******************************************************************************

  cmd_read_slot_name

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_read_slot_name(u8 *report,u8 *output)
{

	u8 slot_no=report[1];


	if ((slot_no >= 0x10) && (slot_no < 0x10 + NUMBER_OF_HOTP_SLOTS))      //HOTP slot
	{
		slot_no = slot_no & 0x0F;
		u8 is_programmed =*((u8 *)(hotp_slots[slot_no]));

		if (is_programmed==0x01)
		{
		  memcpy(output+OUTPUT_CMD_RESULT_OFFSET,(u8 *)(hotp_slots[slot_no]+SLOT_NAME_OFFSET),15);
      CI_StringOut ("Slot name -");
      CI_StringOut (output+OUTPUT_CMD_RESULT_OFFSET);
      CI_StringOut ("-\r\n");
		}
		else
		{
		  output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_SLOT_NOT_PROGRAMMED;
		}
	}
	else if ((slot_no >= 0x20) && (slot_no < 0x20 + NUMBER_OF_TOTP_SLOTS))   //TOTP slot
	{
		slot_no=slot_no&0x0F;
		u8 is_programmed=*((u8 *)(totp_slots[slot_no]));

		if (is_programmed==0x01)
		{
		  memcpy(output+OUTPUT_CMD_RESULT_OFFSET,(u8 *)(totp_slots[slot_no]+SLOT_NAME_OFFSET),15);
      CI_StringOut ("Slot name -");
      CI_StringOut (output+OUTPUT_CMD_RESULT_OFFSET);
      CI_StringOut ("-\r\n");
		}
		else
		{
		  output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_SLOT_NOT_PROGRAMMED;
		}
	}
	else
	{
		output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_SLOT;
	}

	return 0;
}
/*******************************************************************************

  cmd_read_slot

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


u8 cmd_read_slot(u8 *report,u8 *output)
{
	u8 slot_no=report[CMD_RS_SLOT_NUMBER_OFFSET];


	if ((slot_no >= 0x10) && (slot_no < 0x10 + NUMBER_OF_HOTP_SLOTS))    //HOTP slot
	{
		slot_no=slot_no&0x0F;
		u8 is_programmed=*((u8 *)(hotp_slots[slot_no]));

		if (is_programmed==0x01)
		{
			memcpy(output+OUTPUT_CMD_RESULT_OFFSET,(u8 *)(hotp_slots[slot_no]+SECRET_OFFSET),34);
			//memcpy(output+OUTPUT_CMD_RESULT_OFFSET+CMD_RS_OUTPUT_COUNTER_OFFSET,(u8 *)(hotp_slot_counters[slot_no]),8);
			
			u64 counter= get_counter_value(hotp_slot_counters[slot_no]);
			memcpy(output+OUTPUT_CMD_RESULT_OFFSET+CMD_RS_OUTPUT_COUNTER_OFFSET,&counter,8);
		}
		else
		{
      output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_SLOT_NOT_PROGRAMMED;
		}
	}
	else if ((slot_no >= 0x20) && (slot_no < 0x20 + NUMBER_OF_TOTP_SLOTS))     //TOTP slot
	{
		slot_no=slot_no&0x0F;
		u8 is_programmed=*((u8 *)(totp_slots[slot_no]));
		if (is_programmed==0x01){
			memcpy(output+OUTPUT_CMD_RESULT_OFFSET,(u8 *)(totp_slots[slot_no]+SECRET_OFFSET),34);
		}
		else
		{
		  output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_SLOT_NOT_PROGRAMMED;
		}
	}
	else
	{
		output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_SLOT;
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

#define MAX_SYSTEMTIME_TIMEDIFF                           60        // Sec
#define SYSTEMTIME_TIMEDIFF_FOR_WRITING_INTO_FLASH       (60*60)    // Sec = 1 hour


u8 CheckSystemtime (u32 Newtimestamp_u32)
{
  struct tm      tm_st;
  u8             Time_u8[30];
  time_t         now;
  u32            StoredTime_u32;
  s64            Timediff_s64;
  s32            Timediff_s32;

// Get the local time
  time (&now);

  if (60*60*24*365 > now)     // If the local time is before 1971, set systemtime
  {
    CI_LocalPrintf ("Local time is not set - set to %ld = ",Newtimestamp_u32);
    ctime_r (&Newtimestamp_u32,(char*)Time_u8);
    CI_LocalPrintf ("%s\r\n",Time_u8);
    set_time (Newtimestamp_u32);
  }

// Check timediff
  time (&now);                // Get local time
  Timediff_s64 = now - Newtimestamp_u32;
  Timediff_s32 = Timediff_s64;
  if ((Timediff_s32 > MAX_SYSTEMTIME_TIMEDIFF) || (Timediff_s32 < -MAX_SYSTEMTIME_TIMEDIFF))
  {
    CI_LocalPrintf ("New timestamp is not valid\r\n");
    CI_LocalPrintf ("Systemtime:");
    ctime_r (&now,(char*)Time_u8);
    CI_LocalPrintf ("%s",Time_u8);

    CI_LocalPrintf ("Timestamp :");
    ctime_r (&Newtimestamp_u32,(char*)Time_u8);
    CI_LocalPrintf ("%s",Time_u8);
    return (FALSE);
  }

// Check for writing timestamp to flash
  ReadDatetime (&StoredTime_u32);
  Timediff_s64 = now - StoredTime_u32;
  Timediff_s32 = Timediff_s64;

  if (Timediff_s32 < -MAX_SYSTEMTIME_TIMEDIFF)
  {
    CI_LocalPrintf ("ERROR: Systemtime is time before stored time\r\n");
    return (FALSE);     // Timestamp points to a time before the stored time > something is wrong
  }

  if (Timediff_s32 < SYSTEMTIME_TIMEDIFF_FOR_WRITING_INTO_FLASH)
  {
    return (TRUE);
  }

// Save the systemtime into flash
  CI_LocalPrintf ("Write time into flash - ");
  ctime_r (&now,(char*)Time_u8);
  CI_LocalPrintf ("%s\r\n",Time_u8);

  WriteDatetime (now);

  return (TRUE);
}

/*******************************************************************************

  cmd_get_code

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_get_code(u8 *report,u8 *output)
{
	
  u64 challenge = getu64(report + CMD_GC_CHALLENGE_OFFSET);
  u64 timestamp = getu64(report + CMD_GC_TIMESTAMP_OFFSET);
  u8  interval  = report[CMD_GC_INTERVAL_OFFSET];
	u32 result=0;
/*
	CI_LocalPrintf ("challenge:" );
  HexPrint (8,(unsigned char*)report + CMD_GC_CHALLENGE_OFFSET);
  CI_LocalPrintf ("\n\r");
*/

	u8 slot_no=report[CMD_GC_SLOT_NUMBER_OFFSET];
	
	if ((slot_no >= 0x10) && (slot_no < 0x10 + NUMBER_OF_HOTP_SLOTS))    //HOTP slot
	{
		slot_no = slot_no & 0x0F;
		u8 is_programmed = *((u8 *)(hotp_slots[slot_no]));
		if (is_programmed == 0x01)
		{
			result = get_code_from_hotp_slot(slot_no);

// Change endian
      result = change_endian_u32 (result);

			memcpy(output+OUTPUT_CMD_RESULT_OFFSET,&result,4);
			memcpy(output+OUTPUT_CMD_RESULT_OFFSET+4,(u8 *)hotp_slots[slot_no]+CONFIG_OFFSET,14);
		}
		else
		{
		  output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_SLOT_NOT_PROGRAMMED;
		}
	}
	else if ((slot_no >= 0x20) && (slot_no < 0x20 + NUMBER_OF_TOTP_SLOTS))           //TOTP slot
	{
		slot_no          = slot_no & 0x0F;
		u8 is_programmed = *((u8 *)(totp_slots[slot_no]));

		if (FALSE == CheckSystemtime ((u32)timestamp))
		{

		}

		if (is_programmed==0x01)
		{
      result = get_code_from_totp_slot (slot_no,timestamp / (u64) interval);
//      result = get_code_from_totp_slot (slot_no,challenge);

// Change endian
			result = change_endian_u32 (result);

			memcpy(output+OUTPUT_CMD_RESULT_OFFSET,&result,4);
			memcpy(output+OUTPUT_CMD_RESULT_OFFSET+4,(u8 *)totp_slots[slot_no]+CONFIG_OFFSET,14);
		}
		else
		{
		  output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_SLOT_NOT_PROGRAMMED;
		}
	}
	else
	{
		output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_SLOT;
	}

	return 0;
}

/*******************************************************************************

  cmd_write_config

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_write_config(u8 *report,u8 *output)
{
	u8 slot_tmp[3];          //this is will be the new slot contents
	
	memset(slot_tmp,0,3);

	memcpy(slot_tmp,report+1,3);

	write_to_slot(slot_tmp,GLOBAL_CONFIG_OFFSET, 3);
	
	return 0;
}
/*******************************************************************************

  cmd_erase_slot

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


u8 cmd_erase_slot(u8 *report,u8 *output)
{
	u8 slot_no=report[CMD_WTS_SLOT_NUMBER_OFFSET];
	u8 slot_tmp[64];

	memset(slot_tmp,0xFF,64);

	if (slot_no>=0x10&&slot_no<=0x11)          //HOTP slot
	{
		slot_no=slot_no&0x0F;
		write_to_slot(slot_tmp, hotp_slot_offsets[slot_no], 64);
	}
	else if (slot_no>=0x20&&slot_no<=0x23)     //TOTP slot
	{
		slot_no=slot_no&0x0F;	
		write_to_slot(slot_tmp, totp_slot_offsets[slot_no], 64);
	}
	else
	{
		output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_SLOT;
	}

	return 0;
}
/*******************************************************************************

  cmd_first_authenticate

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_first_authenticate(u8 *report,u8 *output)
{
	u8 res = 1;
	u8 card_password[26];
	u32 Ret_u32;
		
	memset (card_password,0,26);
		
	memcpy (card_password,report+1,25);

// Init smartcard
//  CCID_RestartSmartcard_u8 ();
//  ISO7816_T1_InitSendNr ();


// res = cardAuthenticate (card_password); // Check for admin password

  Ret_u32 =  LA_SC_SendVerify (1,(unsigned char *)card_password); // 1 = user pw
//	Ret_u32 = LA_OpenPGP_V20_Test_SendAdminPW ((unsigned char *)card_password);
  if (TRUE == Ret_u32)
  {
    res = 0;
  }
		
  memset (card_password,0,26);    // RB : Clear password ram

  if (res==0)
  {
    memcpy(temp_password,report+26,25);

    tmp_password_set = 1;

//  getAID();

    Ret_u32 = LA_OpenPGP_V20_Test_GetAID ();

    return 0;
  }
  else
  {
    output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_PASSWORD;
    return 1; //wrong card password
  }
}

/*******************************************************************************

  cmd_authorize

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 cmd_authorize(u8 *report,u8 *output)
{
	if (tmp_password_set == 1)
	{
	  if (memcmp(report+5,temp_password,25)==0)
      authorized_crc = getu32(report+1);
	  else
	    output[OUTPUT_CMD_STATUS_OFFSET]=CMD_STATUS_WRONG_PASSWORD;
	}

	return (0);
}
/*******************************************************************************

  CopyUpdateToHIDData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CopyUpdateToHIDData (void)
{
  memcpy (&HID_GetReport_Value_tmp[OUTPUT_CMD_RESULT_STICK20_STATUS_START],(void*)&HID_Stick20Status_st,sizeof (HID_Stick20Status_est));
}

/*******************************************************************************

  UpdateStick20Command

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void UpdateStick20Command (u8 Status_u8,u8 ProgressBarValue_u8)
{
  HID_Stick20Status_st.Status_u8            = Status_u8;
  HID_Stick20Status_st.ProgressBarValue_u8  = ProgressBarValue_u8;

  CopyUpdateToHIDData ();
}

/*******************************************************************************

  StartStick20Command

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void StartStick20Command (u8 Command_u8)
{
  HID_Stick20Status_st.CommandCounter_u8   += 1;
  HID_Stick20Status_st.LastCommand_u8       = Command_u8;
  HID_Stick20Status_st.Status_u8            = OUTPUT_CMD_STICK20_STATUS_BUSY;
  HID_Stick20Status_st.ProgressBarValue_u8  = 0;

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
  HID_Stick20Status_st.CommandCounter_u8    = 0;
  HID_Stick20Status_st.LastCommand_u8       = 0xFF;
  HID_Stick20Status_st.Status_u8            = OUTPUT_CMD_STICK20_STATUS_IDLE;
  HID_Stick20Status_st.ProgressBarValue_u8  = 0;

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


/* Endless loop after USB startup  */
//  while (1)
  {
    if (OTP_device_status == STATUS_RECEIVED_REPORT)
    {
      OTP_device_status = STATUS_BUSY;
      CI_StringOut ("-B-");
      parse_report(HID_SetReport_Value_tmp,HID_GetReport_Value_tmp);
      OTP_device_status = STATUS_READY;
//      CI_StringOut ("-R-");

    }

    if (numLockClicked)
    {
      numLockClicked=0;
      //sendString("NumLock",7);

      u8 slot_number=((u8 *)SLOTS_ADDRESS+GLOBAL_CONFIG_OFFSET)[0];
      if (slot_number<=1)
      {
        u8 programmed=*((u8 *)hotp_slots[slot_number]);

        if (programmed==0x01){
          u32 code= get_code_from_hotp_slot(slot_number);
          u8 config =get_hotp_slot_config(slot_number);

          if (config&(1<<SLOT_CONFIG_TOKENID))
            sendString((char *)(hotp_slots[slot_number]+TOKEN_ID_OFFSET),12);

          if (config&(1<<SLOT_CONFIG_DIGITS))
            sendNumberN(code,8);
          else
            sendNumberN(code,6);

          if (config&(1<<SLOT_CONFIG_ENTER))
            sendEnter();
        }
      }
    }
    if (capsLockClicked)
    {
      capsLockClicked=0;
    //sendString("CapsLock",8);

      u8 slot_number=((u8 *)SLOTS_ADDRESS+GLOBAL_CONFIG_OFFSET)[1];
      if (slot_number<=1)
      {
        u8 programmed=*((u8 *)hotp_slots[slot_number]);

        if (programmed==0x01)
        {
          u32 code= get_code_from_hotp_slot(slot_number);
          u8 config =get_hotp_slot_config(slot_number);

          if (config&(1<<SLOT_CONFIG_TOKENID))
            sendString((char *)(hotp_slots[slot_number]+TOKEN_ID_OFFSET),12);

          if (config&(1<<SLOT_CONFIG_DIGITS))
            sendNumberN(code,8);
          else
            sendNumberN(code,6);

          if (config&(1<<SLOT_CONFIG_ENTER))
            sendEnter();
        }
      }
    }

    if (scrollLockClicked)
    {
      scrollLockClicked=0;
    //sendString("ScrollLock",10);

      u8 slot_number=((u8 *)SLOTS_ADDRESS+GLOBAL_CONFIG_OFFSET)[2];
      if (slot_number<=1)
      {
        u8 programmed = *((u8 *)hotp_slots[slot_number]);

        if (programmed==0x01)
        {
          u32 code= get_code_from_hotp_slot(slot_number);
          u8 config =get_hotp_slot_config(slot_number);

          if (config&(1<<SLOT_CONFIG_TOKENID))
            sendString((char *)(hotp_slots[slot_number]+TOKEN_ID_OFFSET),12);

          if (config&(1<<SLOT_CONFIG_DIGITS))
            sendNumberN(code,8);
          else
            sendNumberN(code,6);

          if (config&(1<<SLOT_CONFIG_ENTER))
            sendEnter();
        }
      }

    }
  }
}



