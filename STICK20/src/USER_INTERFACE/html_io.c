/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 03.04.2012
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
 * html_io.c
 *
 *  Created on: 03.04.2012
 *      Author: RB
 */


#include "board.h"

#ifdef FREERTOS_USED
  #include "FreeRTOS.h"
  #include "task.h"
#endif
#include "string.h"
#include "stdio.h"
#include "print_funcs.h"
#include "gpio.h"
#include "pm.h"
#include "adc.h"
#include "time.h"
#include "flashc.h"

#include "global.h"
#include "tools.h"
#include "Interpreter.h"
#include "BUFFERED_SIO.h"
#include "..\FILE_IO\FileAccessInterface.h"
#include "TIME_MEASURING.h"

#include "CCID/USART/ISO7816_USART.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"

#include "mci.h"
#include "conf_sd_mmc_mci.h"
#include "sd_mmc_mci.h"

/* FAT includes */
#include "file.h"
#include "navigation.h"
#include "ctrl_access.h"
#include "fsaccess.h"

#include "Inbetriebnahme.h"
#include "html_io.h"
#include "INTERNAL_WORK/internal_work.h"
#include "HighLevelFunctions\HandleAesStorageKey.h"
#include "DFU_test.h"
#include "SD_Test.h"
#include "file_io.h"
#include "OTP\report_protocol.h"
#include "..\HighLevelFunctions\MatrixPassword.h"
#include "..\HighLevelFunctions\HiddenVolume.h"


#include "..\HighLevelFunctions\FlashStorage.h"
#include "..\HighLevelFunctions\HiddenVolume.h"
#include "..\HighLevelFunctions\password_safe.h"

extern typeStick20Configuration_st StickConfiguration_st;
u8 GetSmartCardStatus (typeStick20Configuration_st *Status_st);

/*******************************************************************************

 Local defines

*******************************************************************************/

#define DEBUG_HTML_IO

#ifdef DEBUG_HTML_IO
#else
  #define CI_LocalPrintf(...)
  #define CI_TickLocalPrintf(...)
  #define CI_StringOut(...)
  #define CI_Print8BitValue(...)
  #define HexPrint(...)
#endif

//#define HTML_ENABLE_HTML_INTERFACE

#ifdef HTML_ENABLE_HTML_INTERFACE
  #define HTML_FILENAME_STARTPAGE_FF   "startfire.htm"
  #define HTML_FILENAME_STARTPAGE_IE   "start_ie.htm"
  #define HTML_FILENAME_INPUTFILE   "data.txt"
#endif // HTML_ENABLE_HTML_INTERFACE

#define HTML_FILEIO_LUN           0    // RAM Disk

/*******************************************************************************

 Global declarations

*******************************************************************************/

u64 getu64 (u8 *array);

/*******************************************************************************

 External declarations

*******************************************************************************/
void AES_SetNewStorageKey (unsigned char *pcKey);

/*******************************************************************************

 Local declarations

*******************************************************************************/
// Interface to HID
extern u8 HID_CmdGet_u8;   // Todo: make function
extern u8 HID_String_au8[50];

typeStick20ProductionInfos_st Stick20ProductionInfos_st;

u8 HID_AdminPassword_au8[HID_PASSWORD_LEN+1];
u8 HID_UserPassword_au8[HID_PASSWORD_LEN+1];

u32 HID_AdminPasswordEnabled_u32 = FALSE;
u32 HID_UserPasswordEnabled_u32  = FALSE;

u32 HID_NextPasswordIsHiddenPassword_u32 = FALSE;

#ifdef HTML_ENABLE_HTML_INTERFACE

/*******************************************************************************

  HTML_FileIO_Init_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 HTML_FileIO_Init_u8 (void)
{
  u8 Path_u8[20];

  b_fsaccess_init ();

// Move path in RAM , necessary for cd
//  strcpy ((char*)Path_u8,AD_FILEIO_WORKING_DIR);

// Mount RAM DISC drive
  CI_LocalPrintf ("\r\nMount ram disk\r\n");

  if (FALSE == FAI_mount(HTML_FILEIO_LUN))
  {
    return (FALSE);
  }

// Show free space
  CI_LocalPrintf ("\r\n\r\n");
  FAI_free_space (HTML_FILEIO_LUN);

  HTML_GenerateStartPage ();

// Show dirs
  CI_LocalPrintf ("\r\nOld dir\r\n");
  FAI_Dir (0);

// Show new dir
/*
  CI_LocalPrintf ("\r\nNew dir\r\n");
  FAI_Dir (0);
*/
  return (TRUE);
}

/*******************************************************************************

  HTML_FileIO_AppendText_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 HTML_FileIO_AppendText_u8 (u8 *Filename_pu8,u8 *Text_pu8)
{
  s32 FileID_s32;
  s32 Ret_s32;

  FileID_s32 = open ((char*)Filename_pu8,O_CREAT|O_APPEND);
  if (0 > FileID_s32)
  {
    CI_LocalPrintf ("Error open %s - %d - %s\r\n",Filename_pu8,fs_g_status, IBN_FileSystemErrorText(fs_g_status));
    return (FALSE);
  }

  Ret_s32 = write (FileID_s32,Text_pu8,strlen ((char*)Text_pu8));

  Ret_s32 = close (FileID_s32);
  if (-1 == Ret_s32)
  {
    CI_LocalPrintf ("Close  = %d \r\n",Ret_s32);
  }

  return (TRUE);
}

u8 *HTML_Code_Startpage1_pu8 =
{
"<html>"
"<body>"
"<h1>Cryptostick 2.1</h1>"
"<form action='input_button.htm'>"
"<p>Kennwort:<br><input name='kennwort' type='password' size='16' maxlength='16'></p>"
"<p><input type='button' name='Text 2' value='Pin eingeben' onclick="
};


//"    <input type='button' name='Text 2' value='Text 2 anzeigen' onclick='saveFile('data.txt',this.form.kennwort.value)'>"
u8 *HTML_Code_Startpage2_pu8 =
{
    "'saveFile("
    "\"d:\\"HTML_FILENAME_INPUTFILE"\""
    ",\"01 \"+this.form.kennwort.value)'>"
};

u8 *HTML_Code_Startpage3_pu8 =
{
"  </p>"
"</form>"
"<script>"
"function saveFile(fileName, val)"
"{"
"   try {"
"      netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');"
"   } catch (e) {"
"      alert('Permission denied.');"
"   }"
"   var Path = window.location.pathname;"
"   var Drive = Path.substr(1,2);"
"   var pName = fileName;"
//  "   var pName = Drive+'\\\\'+fileName;"
"   var file = Components.classes['@mozilla.org/file/local;1'].createInstance(Components.interfaces.nsILocalFile);"
"   file.initWithPath(pName);"
//     "     alert('Creating file: '+pName );"
"   if (file.exists() == false) {"
//"     alert('Creating file... ' );"
"     file.create( Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 420 );"
"   }"
""
"   var outputStream = Components.classes['@mozilla.org/network/file-output-stream;1']"
"            .createInstance(Components.interfaces.nsIFileOutputStream);"
"   outputStream.init( file, 0x04 | 0x08 | 0x20, 420, 0 );"
"   var result = outputStream.write(val, val.length );"
"  outputStream.close();"
"}"
"</script>"
"</body>"
"</html>"
};


u8 *HTML_Code_Startpage3_IE_pu8 =
{
"  </p>"
"</form>"
"<script>"
"function saveFile(filePath, fileContent) {"
"    try {"
"      var fso = new ActiveXObject('Scripting.FileSystemObject');"
"      var file = fso.OpenTextFile(filePath, 2, true);"
"      file.Write(fileContent);"
"       file.Close();"
"    } catch (e) {"
"    if (e.number == -2146827859) {"
"           alert('Unable to access local files due to browser security settings. ' +"
"               'To overcome this, go to Tools->Internet Options->Security->Custom Level. ' +"
"                'Find the setting for -Initialize and script ActiveX controls not marked as safe- and change it to -Enable- or -Prompt-');"
"          }"
"    }"
"  }"
"</script>"
"</body>"
"</html>"
};
/*******************************************************************************

  HTML_GenerateStartPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void HTML_GenerateStartPage (void)
{
// For firefox
  HTML_FileIO_AppendText_u8 ((u8*)HTML_FILENAME_STARTPAGE_FF,HTML_Code_Startpage1_pu8);
  HTML_FileIO_AppendText_u8 ((u8*)HTML_FILENAME_STARTPAGE_FF,HTML_Code_Startpage2_pu8);
  HTML_FileIO_AppendText_u8 ((u8*)HTML_FILENAME_STARTPAGE_FF,HTML_Code_Startpage3_pu8);


// For IE
  HTML_FileIO_AppendText_u8 ((u8*)HTML_FILENAME_STARTPAGE_IE,HTML_Code_Startpage1_pu8);
  HTML_FileIO_AppendText_u8 ((u8*)HTML_FILENAME_STARTPAGE_IE,HTML_Code_Startpage2_pu8);
  HTML_FileIO_AppendText_u8 ((u8*)HTML_FILENAME_STARTPAGE_IE,HTML_Code_Startpage3_IE_pu8);

  set_virtual_unit_busy ();
//  FAI_UpdateContend (HTML_FILEIO_LUN);
}
/*******************************************************************************

  HTML_CheckInput

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 HTML_CheckInput (u8 *Text_u8)
{
  s32 FileID_s32;
  s32 Ret_s32 = -1;

//CI_LocalPrintf("*");

  if (FALSE == FAI_mount(HTML_FILEIO_LUN))
  {
    CI_TickLocalPrintf ("HTML_CheckInput: Error mounting ram disk - %d - %s\r\n",fs_g_status, IBN_FileSystemErrorText(fs_g_status));
    return (FALSE);
  }

  FAI_UpdateContend (0);
  vTaskDelay (100);

  FileID_s32 = open ((char*)HTML_FILENAME_INPUTFILE,O_RDONLY);
  if (0 > FileID_s32)
  {
//    CI_TickLocalPrintf ("HTML_CheckInput: Error open %s - %d - %s\r\n",HTML_FILENAME_INPUTFILE,fs_g_status, IBN_FileSystemErrorText(fs_g_status));
    return (FALSE);
  }

  Ret_s32 = read (FileID_s32,Text_u8,HTML_INPUTBUFFER_SIZE);
  if (-1 == Ret_s32)
  {
    CI_TickLocalPrintf ("HTML_CheckInput: Error reading %s - %d - %s\r\n",HTML_FILENAME_INPUTFILE,fs_g_status, IBN_FileSystemErrorText(fs_g_status));
  }
  else
  {
    Text_u8 [Ret_s32] = 0;
    CI_TickLocalPrintf ("HTML_CheckInput: Reading %d bytes -%*.*s-\r\n",Ret_s32,Ret_s32,Ret_s32,Text_u8);
  }

  if (-1 == close (FileID_s32))
  {
    CI_TickLocalPrintf ("HTML_CheckInput: Close fail\r\n");
  }

  if (-1 != Ret_s32)
  {
    CI_TickLocalPrintf ("HTML_CheckInput: Delete input file\r\n");
    FAI_DeleteFile(HTML_FILENAME_INPUTFILE);
    vTaskDelay (100);
    FAI_UpdateContend (HTML_FILEIO_LUN);
    vTaskDelay (100);
    set_virtual_unit_busy ();               // Tell the host that the FAT has changed
  }
  return (TRUE);
}
/*******************************************************************************

  HTML_CheckRamDisk

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void HTML_CheckRamDisk (void)
{
  u8  Text_u8[HTML_INPUTBUFFER_SIZE];
  u8  StorageKey_pu8[32];
  s32 HTML_Command_s32;


  HTML_Command_s32 = HTML_CMD_NOTHING;

  if (TRUE == HTML_CheckInput (Text_u8))
  {
    HTML_Command_s32 = atoi (Text_u8);
//    CI_TickLocalPrintf ("HTML_CheckRamDisk: Get command: %d \r\n",HTML_Command_s32);

    switch (HTML_Command_s32)
    {
      case HTML_CMD_NOTHING :
        break;

      case HTML_CMD_ENABLE_AES_LUN :
        if (TRUE == IW_SendToSC_PW1 (&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD]))
        {
          GetStorageKey_u32 (&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD],StorageKey_pu8);
          AES_SetNewStorageKey (StorageKey_pu8);

          SetSdEncryptedHiddenState (FALSE);
          SetSdEncryptedCardEnableState (TRUE);
          CI_TickLocalPrintf ("SD card AES LUN enabled\r\n");
        }
        break;

      case HTML_CMD_DISABLE_AES_LUN :
        SetSdEncryptedCardEnableState (FALSE);
        SetSdEncryptedHiddenState (FALSE);
        AES_SetNewStorageKey ("0000");      // Set dummy key
        CI_TickLocalPrintf ("SD card AES LUN disabled\r\n");
        break;

      case HTML_CMD_ENABLE_FIRMWARE_UPDATE :
        if (TRUE == IW_SendToSC_PW3 (&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD]))
        {
          CI_TickLocalPrintf ("Firmware update enabled - good bye\r\n");
          DFU_EnableFirmwareUpdate ();
        }
        break;

      case HTML_CMD_FILL_SD_WITH_RANDOM_CHARS :
        if (TRUE == IW_SendToSC_PW3 (&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD]))
        {
          CI_TickLocalPrintf ("Fill SD card with random numbers - ist lasts very long, 1 GB ca. 4 minutes\r\n");
          SD_SecureEraseHoleCard ();
        }
        break;

      case HTML_CMD_ENABLE_HIDDEN_AES_LUN :
        if (TRUE == IW_SendToSC_PW1 (&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD]))
        {
          GetStorageKey_u32 (&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD],StorageKey_pu8);
          AES_SetNewStorageKey (StorageKey_pu8);

          SetSdEncryptedHiddenState (TRUE);
          SetSdEncryptedCardEnableState (TRUE);
          CI_TickLocalPrintf ("SD card hidden AES LUN enabled\r\n");
        }
        break;

      case HTML_CMD_DISABLE_HIDDEN_AES_LUN :
        SetSdEncryptedCardEnableState (FALSE);
        SetSdEncryptedHiddenState (FALSE);
        AES_SetNewStorageKey ("0000");      // Set dummy key
        CI_TickLocalPrintf ("SD card hidden AES LUN disabled\r\n");
        break;


      case HTML_CMD_EXPORT_BINARY :
        if (TRUE == IW_SendToSC_PW3 (&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD]))
        {
          CI_TickLocalPrintf ("Export binary\r\n");


          FileIO_SaveAppImage_u8 ();

// Todo : send update to host
          SetSdUncryptedCardEnableState (FALSE);      // Disable access
          vTaskDelay (1000);
          SetSdUncryptedCardEnableState (TRUE);       // Enable access
        }
        break;

      case HTML_CMD_GENERATE_NEW_KEYS :
        CI_TickLocalPrintf ("Generate new keys\r\n");
        BuildStorageKeys_u32 ((u8*)&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD]);
        break;

      case HTML_CMD_WRITE_STATUS_DATA :
        CI_TickLocalPrintf ("Write status data\r\n");
        break;

      case HTML_CMD_ENABLE_READONLY_UNCRYPTED_LUN :
        CI_TickLocalPrintf ("Undo readwrite enable uncrypted lun\r\n");
        SetSdUncryptedCardReadWriteEnableState (READ_ONLY_ACTIVE);
        // Todo : send update to host
        SetSdUncryptedCardEnableState (FALSE);      // Disable access
        vTaskDelay (1000);
        SetSdUncryptedCardEnableState (TRUE);       // Enable access
        break;

      case HTML_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
        CI_TickLocalPrintf ("Enable readwrite uncrypted lun\r\n");
        if (TRUE == IW_SendToSC_PW3 (&Text_u8[HTML_CMD_STARTBYTE_OF_PAYLOAD]))
        {
          SetSdUncryptedCardReadWriteEnableState (READ_WRITE_ACTIVE);
        }
        else
        {
          CI_TickLocalPrintf ("*** worng password ***\r\n");
        }
        // Todo : send update to host
        SetSdUncryptedCardEnableState (FALSE);      // Disable access
        vTaskDelay (1000);
        SetSdUncryptedCardEnableState (TRUE);       // Enable access

        break;

      default:
        CI_TickLocalPrintf ("HTML_CheckRamDisk: Get unknown command: %d \r\n",HTML_Command_s32);
        break;
    }
  }
}

#endif // HTML_ENABLE_HTML_INTERFAC
/*******************************************************************************

  HID_ExcuteCmd



  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void HID_ExcuteCmd (void)
{
//  u8  Text_u8[HTML_INPUTBUFFER_SIZE];
  u8  StorageKey_pu8[32];
  u8  Cmd_u8;
  u8 *PasswordMatrix_pu8;
  u8  ret_u8;
  u64 timestamp;
//  u32 Ret_u32;

  Cmd_u8 = HID_CmdGet_u8;

// Check for active CCID command


// If cmd is active, disable CCID smart card access

/*
  if (HTML_CMD_NOTHING == cmd)
  {
    HID_SmartcardAccess_u8 = TRUE;
  }
*/

  // Check for matrix password > if yes convert the matrix to password
  switch (Cmd_u8)
  {
    case HTML_CMD_ENABLE_AES_LUN :
    case HTML_CMD_ENABLE_FIRMWARE_UPDATE :
    case HTML_CMD_ENABLE_HIDDEN_AES_LUN :
    case HTML_CMD_DISABLE_HIDDEN_AES_LUN :
    case HTML_CMD_EXPORT_BINARY :
    case HTML_CMD_GENERATE_NEW_KEYS :
    case HTML_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
    case HTML_CMD_CLEAR_NEW_SD_CARD_FOUND :
      switch (HID_String_au8[0])
      {
        case 'P' : //  normal password
          // Do nothing
          break;
        case 'M' : //  matrix password
          // Convert matrix input to password
          if (FALSE == ConvertMatrixDataToPassword(&HID_String_au8[1]))
          {
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
            return;
          }
          break;
      }
      break;

    case HTML_CMD_FILL_SD_WITH_RANDOM_CHARS :
      switch (HID_String_au8[1])
      {
        case 'P' : //  normal password
          // Do nothing
          break;
        case 'M' : //  matrix password
          // Convert matrix input to password
          if (FALSE == ConvertMatrixDataToPassword(&HID_String_au8[2]))
          {
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
            return;
          }
          break;
      }
      break;
  }
  // Check for hidden password

  if (TRUE == HID_NextPasswordIsHiddenPassword_u32)
  {
    switch (Cmd_u8)
    {
      case HTML_CMD_ENABLE_AES_LUN :
        Cmd_u8 = HTML_CMD_ENABLE_HIDDEN_AES_LUN;
        CI_TickLocalPrintf ("Redirect password -%s- to hidden volume\r\n",&HID_String_au8[1]);
        SetSdEncryptedCardEnableState (FALSE);
        SetSdEncryptedHiddenState (FALSE);
        CI_TickLocalPrintf ("Unmount crypted volume\r\n");

        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR,50);

        vTaskDelay (1000);     // Wait that pc unmount old volume
        break;
    }
  }


  //
  switch (Cmd_u8)
  {
    case HTML_CMD_NOTHING :
      break;

    case HTML_CMD_ENABLE_AES_LUN :
      if (TRUE == IW_SendToSC_PW1 (&HID_String_au8[1]))
      {
        SetSdEncryptedCardEnableState (FALSE);    // Disable crypted or hidden volume
        vTaskDelay (1000);

        CI_TickLocalPrintf ("Get key for crypted volume\r\n");

        GetStorageKey_u32 (&HID_String_au8[1],StorageKey_pu8);
        AES_SetNewStorageKey (StorageKey_pu8);

        SetSdEncryptedHiddenState (FALSE);
        SetSdEncryptedCardEnableState (TRUE);

        DecryptedHiddenVolumeSlotsData ();
        PWS_DecryptedPasswordSafeKey ();

        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
/*
        HID_NextPasswordIsHiddenPassword_u32 = TRUE;    // switch on redirection of aes volume password
        CI_TickLocalPrintf ("Next AES volume password is used for hidden volume\r\n");
*/
        CI_TickLocalPrintf ("SD card AES LUN enabled\r\n");
      }
      else
      {
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
        HID_NextPasswordIsHiddenPassword_u32 = FALSE;
        CI_TickLocalPrintf ("*** wrong password ***\r\n");
      }
      break;

    case HTML_CMD_DISABLE_AES_LUN :
      SetSdEncryptedCardEnableState (FALSE);
      SetSdEncryptedHiddenState (FALSE);
      AES_SetNewStorageKey ((u8*)"0000");      // Set dummy key
      HID_NextPasswordIsHiddenPassword_u32 = FALSE;
      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);

      CI_TickLocalPrintf ("SD card AES LUN disabled\r\n");
      break;

    case HTML_CMD_ENABLE_FIRMWARE_UPDATE :
      CI_TickLocalPrintf ("Firmware update - ");

      if (TRUE == flashc_is_security_bit_active ())
      {
        CI_TickLocalPrintf ("Security bit is active. Update not enabled\r\n");
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE,0);
        break;
      }
      if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[1]))
      {
        CI_TickLocalPrintf ("good bye\r\n");
        DFU_EnableFirmwareUpdate ();
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      }
      else
      {
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
        CI_TickLocalPrintf ("wrong password\r\n");
      }
      break;

    case HTML_CMD_FILL_SD_WITH_RANDOM_CHARS :
      if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[2]))
      {
        CI_TickLocalPrintf ("Fill SD card with random numbers - it runs very long, 1 GB ca. 4 minutes\r\n");

        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR,0);
        vTaskDelay (500);

        // Disable all SD card access via USB
        SetSdUncryptedCardEnableState (FALSE);    // look for >> sd_mmc_mci_test_unit_only_local_access = hard disabel
        SetSdEncryptedCardEnableState (FALSE);
        SetSdEncryptedHiddenState (FALSE);
        AES_SetNewStorageKey ((u8*)"0000");       // Set dummy key

        vTaskDelay (500);

        if (STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ALL_VOL == HID_String_au8[0])
        {
          SD_SecureEraseHoleCard ();
        }
        else
        {
          SD_SecureEraseCryptedVolume ();
        }

        SetSdCardFilledWithRandomsToFlash ();

        // Enable uncrypted SD card access via USB
        SetSdUncryptedCardEnableState (TRUE);
        SetSdUncryptedCardReadWriteEnableState (READ_WRITE_ACTIVE);

        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR,100);
        vTaskDelay (500);
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      }
      else
      {
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
        CI_TickLocalPrintf ("wrong password\r\n");
      }
      break;

    case HTML_CMD_ENABLE_HIDDEN_AES_LUN :
//      SetSdUncryptedCardEnableState (FALSE);    // look for >> sd_mmc_mci_test_unit_only_local_access = hard disabel
      SetSdEncryptedCardEnableState (FALSE);
      SetSdEncryptedHiddenState (FALSE);

      memset (StorageKey_pu8,0,32);
      AES_SetNewStorageKey ((u8*)"0000");       // Set dummy key

      vTaskDelay (2000);    // Wait 2 sec to send LUN not active

      // Get w AES key for hidden volume - we use always the password that was send
      ret_u8 = GetHiddenVolumeKeyFromUserpassword ((u8*)&HID_String_au8[1],StorageKey_pu8);
      switch (ret_u8)
      {
        case HIDDEN_VOLUME_OUTPUT_STATUS_OK :
          AES_SetNewStorageKey (StorageKey_pu8);
          SetSdEncryptedHiddenState (TRUE);
          SetSdEncryptedCardEnableState (TRUE);
          UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
          CI_TickLocalPrintf ("Hidden volume - SD card hidden AES LUN enabled\r\n");
          break;
        case HIDDEN_VOLUME_OUTPUT_STATUS_WRONG_PASSWORD :
          CI_TickLocalPrintf ("Hidden volume - wrong password\r\n");
          UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
          break;
        case HIDDEN_VOLUME_OUTPUT_STATUS_NO_USER_PASSWORD_UNLOCK :
          CI_TickLocalPrintf ("Hidden volume - no user password\r\n");
          UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK,0);
          break;
        case HIDDEN_VOLUME_OUTPUT_STATUS_SMARTCARD_ERROR :
          CI_TickLocalPrintf ("Hidden volume - smartcard error\r\n");
          UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR,0);
          break;
        default :
          break;
      }

       HID_NextPasswordIsHiddenPassword_u32 = FALSE;       // switch off redirection of aes volume password
       break;

    case HTML_CMD_DISABLE_HIDDEN_AES_LUN :
      SetSdEncryptedCardEnableState (FALSE);
      SetSdEncryptedHiddenState (FALSE);
      memset (StorageKey_pu8,0,32);             // Set dummy key
      AES_SetNewStorageKey ((u8*)"0000");       // Set dummy key
      HID_NextPasswordIsHiddenPassword_u32 = FALSE;

      vTaskDelay (2000);    // Wait 2 sec to send LUN not active
      CI_TickLocalPrintf ("SD card hidden AES LUN disabled\r\n");
      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      break;


    case HTML_CMD_EXPORT_BINARY :
      if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[1]))
      {
        CI_TickLocalPrintf ("Export binary\r\n");


        FileIO_SaveAppImage_u8 ();

// Todo : send update to host
        SetSdUncryptedCardEnableState (FALSE);      // Disable access
        vTaskDelay (1000);
        SetSdUncryptedCardEnableState (TRUE);       // Enable access
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      }
      else
      {
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
        CI_TickLocalPrintf ("*** worng password ***\r\n");
      }
      break;

    case HTML_CMD_GENERATE_NEW_KEYS :
      CI_TickLocalPrintf ("Generate new keys\r\n");
      if (TRUE == BuildStorageKeys_u32 ((u8*)&HID_String_au8[1]))
      {
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      }
      else
      {
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
      }
      break;

    case HTML_CMD_ENABLE_READONLY_UNCRYPTED_LUN :
      CI_TickLocalPrintf ("Set readonly uncrypted lun\r\n");
      SetSdUncryptedCardReadWriteEnableState (READ_ONLY_ACTIVE);
      if (TRUE == IW_SendToSC_PW1 (&HID_String_au8[1]))
      {
        // Todo : send update to host
        SetSdUncryptedCardEnableState (FALSE);      // Disable access
        vTaskDelay (1000);
        SetSdUncryptedCardReadWriteEnableState (READ_ONLY_ACTIVE);
        SetSdUncryptedCardEnableState (TRUE);       // Enable access
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
        CI_TickLocalPrintf ("ok\r\n");
      }
      else
      {
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
        CI_TickLocalPrintf ("*** worng password ***\r\n");
      }
      break;

    case HTML_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
      CI_TickLocalPrintf ("Set readwrite uncrypted lun\r\n");
      if (TRUE == IW_SendToSC_PW1 (&HID_String_au8[1]))
      {
        // Todo : send update to host
        SetSdUncryptedCardEnableState (FALSE);      // Disable access
        vTaskDelay (1000);
        SetSdUncryptedCardReadWriteEnableState (READ_WRITE_ACTIVE);
        SetSdUncryptedCardEnableState (TRUE);       // Enable access

        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
        CI_TickLocalPrintf ("ok\r\n");
      }
      else
      {
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
        CI_TickLocalPrintf ("*** worng password ***\r\n");
      }

      break;

    case HTML_CMD_SEND_PASSWORD_MATRIX :
      CI_TickLocalPrintf ("Get SEND_PASSWORD_MATRIX\r\n");

      PasswordMatrix_pu8 = GetCorrectMatrix ();

      Stick20HIDInitSendMatrixData (PasswordMatrix_pu8);

      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY,0);
      break;

    case HTML_CMD_SEND_PASSWORD_MATRIX_PINDATA :
      CI_TickLocalPrintf ("Get SEND_PASSWORD_MATRIX_PINDATA\r\n");

      CI_TickLocalPrintf ("Data 1 : %s\r\n",HID_String_au8);

//      PasswordMatrix_pu8 = GetCorrectMatrix ();

      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      break;

    case HTML_CMD_SEND_PASSWORD_MATRIX_SETUP :
      CI_TickLocalPrintf ("Get SEND_PASSWORD_MATRIX_SETUP\r\n");
      CI_TickLocalPrintf ("Data : %s\r\n",HID_String_au8);
      WriteMatrixColumsUserPWToUserPage (&HID_String_au8[1]);   // Data is in ASCII
      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      break;


    case HTML_CMD_GET_DEVICE_STATUS :
      CI_TickLocalPrintf ("Get HTML_CMD_GET_DEVICE_STATUS\r\n");
      GetSmartCardStatus (&StickConfiguration_st);
      Stick20HIDInitSendConfiguration (STICK20_SEND_STATUS_PIN);
      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      break;

    case HTML_CMD_WRITE_STATUS_DATA:
      CI_TickLocalPrintf ("Get HTML_CMD_WRITE_STATUS_DATA\r\n");
//      GetStickStatusFromHID ((HID_Stick20AccessStatus_est*)HID_String_au8); // Changed
      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
      break;


    case HTML_SEND_HIDDEN_VOLUME_SETUP:
       CI_TickLocalPrintf ("Get HTML_SEND_HIDDEN_VOLUME_SETUP\r\n");
       InitRamdomBaseForHiddenKey ();
       UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
       break;

    case HTML_CMD_SEND_PASSWORD:
       CI_TickLocalPrintf ("Get HTML_CMD_SEND_PASSWORD\r\n");

       HID_AdminPasswordEnabled_u32 = FALSE;
       HID_UserPasswordEnabled_u32  = FALSE;

       switch (HID_String_au8[0])
       {
         case 'P' :
           ret_u8 = IW_SendToSC_PW1 (&HID_String_au8[1]);
           HID_UserPasswordEnabled_u32  = ret_u8;
           strncpy ((char*)HID_UserPassword_au8,(char*)&HID_String_au8[1],HID_PASSWORD_LEN);
           break;
         case 'A' :
           ret_u8 = IW_SendToSC_PW3 (&HID_String_au8[1]);
           HID_AdminPasswordEnabled_u32 = ret_u8;
           strncpy ((char*)HID_AdminPassword_au8,(char*)&HID_String_au8[1],HID_PASSWORD_LEN);
           break;
       }

       if (TRUE == ret_u8)
       {
         UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
         CI_TickLocalPrintf ("password ok\r\n");
       }
       else
       {
         UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
         CI_TickLocalPrintf ("*** wrong password ***\r\n");
       }
       break;

     case HTML_CMD_SEND_NEW_PASSWORD :
       CI_TickLocalPrintf ("Get HTML_CMD_SEND_NEW_PASSWORD\r\n");
       ret_u8 = FALSE;
       switch (HID_String_au8[0])
       {
         case 'P' :
           if (TRUE == HID_UserPasswordEnabled_u32)
           {
             ret_u8 =  LA_OpenPGP_V20_Test_ChangeUserPin (HID_UserPassword_au8,&HID_String_au8[1]);
           }
           else
           {
             CI_TickLocalPrintf ("** User password not present ***\r\n");
           }
           break;
         case 'A' :
           if (TRUE == HID_AdminPasswordEnabled_u32)
           {
             ret_u8 =  LA_OpenPGP_V20_Test_ChangeAdminPin (HID_AdminPassword_au8,&HID_String_au8[1]);
           }
           else
           {
             CI_TickLocalPrintf ("** Admin password not present ***\r\n");
           }
           break;
       }

       if (TRUE == ret_u8)
       {
         UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
         CI_TickLocalPrintf ("password change ok\r\n");
       }
       else
       {
         UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
         CI_TickLocalPrintf ("*** password NOT changed ***\r\n");
       }

       // Clear password
       HID_AdminPasswordEnabled_u32 = FALSE;
       HID_UserPasswordEnabled_u32  = FALSE;
       memset (HID_UserPassword_au8,0,HID_PASSWORD_LEN);
       memset (HID_AdminPassword_au8,0,HID_PASSWORD_LEN);
       break;

     case HTML_CMD_CLEAR_NEW_SD_CARD_FOUND :
       CI_TickLocalPrintf ("Get HTML_CMD_CLEAR_NEW_SD_CARD_FOUND\r\n");
       if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[1]))
       {
         ClearNewSdCardFoundToFlash ();
         SetSdCardFilledWithRandomCharsToFlash ();
         UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
         CI_TickLocalPrintf ("password ok\r\n");
       }
       else
       {
         UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
         CI_TickLocalPrintf ("*** wrong password ***\r\n");
       }
       break;

     case HTML_CMD_SEND_STARTUP :
       CI_TickLocalPrintf ("Get HTML_CMD_SEND_STARTUP\r\n");
       timestamp = getu64(&HID_String_au8[0]);
       if (FALSE == CheckSystemtime ((u32)timestamp))
       {
          // todo
       }

       GetSmartCardStatus (&StickConfiguration_st);

       Stick20HIDInitSendConfiguration (STICK20_SEND_STARTUP);
       UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
       break;
/*
     case STICK20_CMD_SEND_PASSWORD_RETRY_COUNT :
       CI_TickLocalPrintf ("Get STICK20_CMD_SEND_PASSWORD_RETRY_COUNT\r\n");

//       Stick20HIDInitSendConfiguration (STICK20_SEND_STARTUP);
       UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
       break;
*/
     case HTML_CMD_HIDDEN_VOLUME_SETUP:
        CI_TickLocalPrintf ("Get HTML_CMD_HIDDEN_VOLUME_SETUP\r\n");
        ret_u8 = SetupUpHiddenVolumeSlot ((HiddenVolumeSetup_tst *)&HID_String_au8[0]);
        switch (ret_u8)
        {
          case HIDDEN_VOLUME_OUTPUT_STATUS_OK :
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
            break;
          case HIDDEN_VOLUME_OUTPUT_STATUS_WRONG_PASSWORD :
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
            break;
          case HIDDEN_VOLUME_OUTPUT_STATUS_NO_USER_PASSWORD_UNLOCK :
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK,0);
            break;
          case HIDDEN_VOLUME_OUTPUT_STATUS_SMARTCARD_ERROR :
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR,0);
            break;
          default :
            break;
        }
        break;

      case HTML_CMD_CLEAR_STICK_KEYS_NOT_INITIATED :
        CI_TickLocalPrintf ("Get HTML_CMD_CLEAR_STICK_KEYS_NOT_INITIATED\r\n");
        ClearStickKeysNotInitatedToFlash ();
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
        break;

      case HTML_CMD_CLEAR_LOCK_STICK_HARDWARE :
        CI_TickLocalPrintf ("Get HTML_CMD_CLEAR_LOCK_STICK_HARDWARE\r\n");
        if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[1]))
        {
          CI_TickLocalPrintf ("Lock firmware\r\n");
          flashc_activate_security_bit ();            // Debugging disabled, only chip erase works (bootloader is save) , AES storage keys and setup are erased
          UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
        }
        else
        {
          UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD,0);
          CI_TickLocalPrintf ("*** worng password ***\r\n");
        }
        break;

      case HTML_CMD_PRODUCTION_TEST :
        CI_TickLocalPrintf ("Get HTML_CMD_PRODUCTION_TEST\r\n");
        GetProductionInfos (&Stick20ProductionInfos_st);
        Stick20HIDInitSendConfiguration (STICK20_SEND_PRODUCTION_TEST);
        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0);
        break;


    default:
      CI_TickLocalPrintf ("HID_ExcuteCmd: Get unknown command: %d \r\n",Cmd_u8);
      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_IDLE,0);
      break;
  }

  HID_CmdGet_u8 = HTML_CMD_NOTHING;
  memset (HID_String_au8,0,50);

/*
    HID_SmartcardAccess_u8 = FALSE;
*/


}


/*******************************************************************************

  GetSmartCardStatus

  Changes
  Date      Author          Info
  02.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 GetSmartCardStatus (typeStick20Configuration_st *Status_st)
{
  u8  Text_u8[20];
  u32 Ret_u32 = FALSE;
//  u32 *p_u32;

//  DelayMs (2000);


/* Get smartcard ID from AID */
  Status_st->ActiveSmartCardID_u32 = 0;
  Ret_u32 = LA_OpenPGP_V20_GetAID ((char*)Text_u8);
  if (TRUE == Ret_u32)
  {
    Status_st->ActiveSmartCardID_u32 =  (u32)Text_u8[10];
    Status_st->ActiveSmartCardID_u32 = Status_st->ActiveSmartCardID_u32 << 8;
    Status_st->ActiveSmartCardID_u32 +=  (u32)Text_u8[11];
    Status_st->ActiveSmartCardID_u32 = Status_st->ActiveSmartCardID_u32 << 8;
    Status_st->ActiveSmartCardID_u32 +=  (u32)Text_u8[12];
    Status_st->ActiveSmartCardID_u32 = Status_st->ActiveSmartCardID_u32 << 8;
    Status_st->ActiveSmartCardID_u32 +=  (u32)Text_u8[13];
  }


/* Get password retry counts*/
  Status_st->UserPwRetryCount  = 0;
  Status_st->AdminPwRetryCount = 0;
  Ret_u32 = LA_OpenPGP_V20_GetPasswordstatus ((char*)Text_u8);
  if (TRUE == Ret_u32)
  {
    Status_st->UserPwRetryCount  = Text_u8[4];
    Status_st->AdminPwRetryCount = Text_u8[6];
  }
  else
  {
    Status_st->UserPwRetryCount  = 88;
    Status_st->AdminPwRetryCount = 88;
  }

  Status_st->FirmwareLocked_u8 = FALSE;
  if (TRUE ==  flashc_is_security_bit_active())
  {
    Status_st->FirmwareLocked_u8 = TRUE;
  }

  return (TRUE);
}

/*******************************************************************************

  GetProductionInfos

  Changes
  Date      Author          Info
  06.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void GetProductionInfos (typeStick20ProductionInfos_st *Infos_st)
{
  typeStick20Configuration_st SC_Status_st;
  volatile u32* id_data = (u32*)0x80800204;       // Place of 120 bit CPU ID
  u32           i;
  u32           CPU_ID_u32;
  cid_t        *cid;

// Clear data field
  memset ((void*)Infos_st,0,sizeof (Infos_st));

  Infos_st->FirmwareVersion_au8[0]              = VERSION_MAJOR;
  Infos_st->FirmwareVersion_au8[1]              = VERSION_MINOR;
  Infos_st->FirmwareVersion_au8[2]              = 0;               // Build number not used
  Infos_st->FirmwareVersion_au8[3]              = 0;               // Build number not used

// Get smartcard infos
  GetSmartCardStatus (&SC_Status_st);

// Run the check only if the initial pw is aktive
  if (3 == SC_Status_st.UserPwRetryCount)
  {
    if (FALSE == LA_OpenPGP_V20_Test_SendUserPW2 ((unsigned char*)"123456"))
    {
      CI_TickLocalPrintf ("GetProductionInfos: Intial password is not activ\r\n");
      Infos_st->SC_AdminPwRetryCount = 99;    // Marker for wrong pw
      return;
    }
  }
  else
  {
    CI_TickLocalPrintf ("GetProductionInfos: Password retry count is not 3 : %d\r\n",SC_Status_st.UserPwRetryCount);
    Infos_st->SC_AdminPwRetryCount = 88;    // Marker for wrong pw retry count
    return;
  }


// Get XORed CPU ID
  CPU_ID_u32 = 0;
  for (i=0;i<4;i++)
  {
     CPU_ID_u32 ^= id_data[i];
  }
  Infos_st->CPU_CardID_u32 = CPU_ID_u32;


// Save smartcard infos
  Infos_st->SC_UserPwRetryCount  = SC_Status_st.UserPwRetryCount;
  Infos_st->SC_AdminPwRetryCount = SC_Status_st.AdminPwRetryCount;
  Infos_st->SmartCardID_u32      = SC_Status_st.ActiveSmartCardID_u32;

// Get SD card infos
  cid = (cid_t *) GetSdCidInfo ();

  Infos_st->SD_CardID_u32                 = (cid->psnh << 8) + cid->psnl;
  Infos_st->SD_Card_ManufacturingYear_u8  = cid->mdt/16;
  Infos_st->SD_Card_ManufacturingMonth_u8 = cid->mdt % 0x0f;
  Infos_st->SD_Card_OEM_u16               = cid->oid;
  Infos_st->SD_Card_Manufacturer_u8       = cid->mid;

// Get SD card speed
  Infos_st->SD_WriteSpeed_u16 = SD_SpeedTest ();
}

