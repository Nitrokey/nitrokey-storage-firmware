/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 03.04.2012
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
u8 GetSmartCardStatus (typeStick20Configuration_st * Status_st);

const u8 AES_DummyKey_au8[32] = "00000000000000000000000000000000";

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


#define HTML_FILEIO_LUN           0 // RAM Disk

/*******************************************************************************

 Global declarations

*******************************************************************************/

u64 getu64 (u8 * array);

/*******************************************************************************

 External declarations

*******************************************************************************/
void AES_SetNewStorageKey (unsigned char* pcKey);

extern int sd_mmc_mci_test_unit_only_local_access;

/*******************************************************************************

 Local declarations

*******************************************************************************/
// Interface to HID
extern u8 HID_CmdGet_u8;        // Todo: make function
extern u8 HID_String_au8[50];

typeStick20ProductionInfos_st Stick20ProductionInfos_st;

u8 HID_AdminPassword_au8[HID_PASSWORD_LEN + 1];
u8 HID_UserPassword_au8[HID_PASSWORD_LEN + 1];

u32 HID_AdminPasswordEnabled_u32 = FALSE;
u32 HID_UserPasswordEnabled_u32 = FALSE;

u32 HID_NextPasswordIsHiddenPassword_u32 = FALSE;


/*******************************************************************************

  HID_ExcuteCmd



  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void HID_ExcuteCmd (void)
{
    // u8 Text_u8[HTML_INPUTBUFFER_SIZE];
    u8 StorageKey_pu8[32];
    u8 Cmd_u8;
    u8* PasswordMatrix_pu8;
    u8 ret_u8;
    u64 timestamp;
    u32 Len;
    // u32 Ret_u32;

    Cmd_u8 = HID_CmdGet_u8;

    // Check for active CCID command


    // If cmd is active, disable CCID smart card access

    /*
       if (HTML_CMD_NOTHING == cmd) { HID_SmartcardAccess_u8 = TRUE; } */

    // Check for matrix password > if yes convert the matrix to password
    switch (Cmd_u8)
    {
        case HTML_CMD_ENABLE_AES_LUN:
        case HTML_CMD_ENABLE_HIDDEN_AES_LUN:
        case HTML_CMD_DISABLE_HIDDEN_AES_LUN:
        case HTML_CMD_EXPORT_BINARY:
        case HTML_CMD_GENERATE_NEW_KEYS:
        case HTML_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
        case HTML_CMD_CLEAR_NEW_SD_CARD_FOUND:
            switch (HID_String_au8[0])
            {
                case 'P':  // normal password
                    // Do nothing
                    break;
                case 'M':  // matrix password
                    // Convert matrix input to password
                    if (FALSE == ConvertMatrixDataToPassword (&HID_String_au8[1]))
                    {
                        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                        return;
                    }
                    break;
            }
            break;

        case HTML_CMD_FILL_SD_WITH_RANDOM_CHARS:
            switch (HID_String_au8[1])
            {
                case 'P':  // normal password
                    // Do nothing
                    break;
                case 'M':  // matrix password
                    // Convert matrix input to password
                    if (FALSE == ConvertMatrixDataToPassword (&HID_String_au8[2]))
                    {
                        UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
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
            case HTML_CMD_ENABLE_AES_LUN:
                Cmd_u8 = HTML_CMD_ENABLE_HIDDEN_AES_LUN;
                CI_TickLocalPrintf ("Redirect password -%s- to hidden volume\r\n", &HID_String_au8[1]);
                SetSdEncryptedCardEnableState (FALSE);
                SetSdEncryptedHiddenState (FALSE);
                CI_TickLocalPrintf ("Unmount crypted volume\r\n");

                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR, 50);

                vTaskDelay (1000);  // Wait that pc unmount old volume
                break;
        }
    }


    //
    switch (Cmd_u8)
    {
        case HTML_CMD_NOTHING:
            break;

        case HTML_CMD_ENABLE_AES_LUN:
            if (TRUE == IW_SendToSC_PW1 (&HID_String_au8[1]))
            {
                SetSdEncryptedHiddenState (FALSE);
                SetSdEncryptedCardEnableState (FALSE);  // Disable crypted or hidden volume
                vTaskDelay (3000);

                CI_TickLocalPrintf ("Get key for crypted volume\r\n");

                GetStorageKey_u32 (&HID_String_au8[1], StorageKey_pu8);
                AES_SetNewStorageKey (StorageKey_pu8);

                SetSdEncryptedCardEnableState (TRUE);

                DecryptedHiddenVolumeSlotsData ();
                PWS_DecryptedPasswordSafeKey ();

                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                /*
                   HID_NextPasswordIsHiddenPassword_u32 = TRUE; // switch on redirection of aes volume password CI_TickLocalPrintf ("Next AES volume
                   password is used for hidden volume\r\n"); */
                CI_TickLocalPrintf ("SD card AES LUN enabled\r\n");
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                HID_NextPasswordIsHiddenPassword_u32 = FALSE;
                CI_TickLocalPrintf ("*** wrong password ***\r\n");
            }
            break;

        case HTML_CMD_DISABLE_AES_LUN:
            SetSdEncryptedCardEnableState (FALSE);
            SetSdEncryptedHiddenState (FALSE);
            AES_SetNewStorageKey ((u8 *) AES_DummyKey_au8); // Set dummy key
            HID_NextPasswordIsHiddenPassword_u32 = FALSE;
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);

            CI_TickLocalPrintf ("SD card AES LUN disabled\r\n");
            break;

        case HTML_CMD_ENABLE_FIRMWARE_UPDATE:
            CI_TickLocalPrintf ("Firmware update - ");

            if (TRUE == flashc_is_security_bit_active ())
            {
                CI_TickLocalPrintf ("Security bit is active. Update not enabled\r\n");
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE, 0);
                break;
            }

            if (TRUE == CheckUpdatePin (&HID_String_au8[1], strlen ((char*)&HID_String_au8[1])))
            {
                CI_TickLocalPrintf ("good bye\r\n");
                DFU_EnableFirmwareUpdate ();
                DFU_ResetCPU ();
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("wrong password\r\n");
            }
            break;

        case HTML_CMD_FILL_SD_WITH_RANDOM_CHARS:
            if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[2]))
            {
                CI_TickLocalPrintf ("Fill SD card with random numbers - it runs very long, 1 GB ca. 4 minutes\r\n");

                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR, 0);

                // Disable all SD card access via USB
                SetSdUncryptedCardEnableState (FALSE);  // look for >> sd_mmc_mci_test_unit_only_local_access = hard disabel
                SetSdEncryptedCardEnableState (FALSE);
                SetSdEncryptedHiddenState (FALSE);
                vTaskDelay (3000);

                AES_SetNewStorageKey ((u8 *) AES_DummyKey_au8); // Set dummy key

                /*
                   if (STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ALL_VOL == HID_String_au8[0]) { SD_SecureEraseHoleCard (); } else {
                   SD_SecureEraseCryptedVolume (); } */
                SD_SecureEraseCryptedVolume ();

                SetSdCardFilledWithRandomsToFlash ();

                // Enable uncrypted SD card access via USB
                SetSdUncryptedCardEnableState (TRUE);
                // SetSdUncryptedCardReadWriteEnableState (READ_WRITE_ACTIVE);

                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR, 100);
                vTaskDelay (500);
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                vTaskDelay (500);
                // DFU_ResetCPU (); // Reboot to get correct volume data
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("wrong password\r\n");
            }
            break;

        case HTML_CMD_ENABLE_HIDDEN_AES_LUN:
            // SetSdUncryptedCardEnableState (FALSE); // look for >> sd_mmc_mci_test_unit_only_local_access = hard disabel

            // Get w AES key for hidden volume - we use always the password that was send
            memset (StorageKey_pu8, 0, 32);
            ret_u8 = GetHiddenVolumeKeyFromUserpassword ((u8 *) & HID_String_au8[1], StorageKey_pu8);
            switch (ret_u8)
            {
                case HIDDEN_VOLUME_OUTPUT_STATUS_OK:
                    SetSdEncryptedCardEnableState (FALSE);
                    SetSdEncryptedHiddenState (FALSE);
                    AES_SetNewStorageKey ((u8 *) AES_DummyKey_au8); // Set dummy key

                    vTaskDelay (3000);  // Wait 3 sec to send LUN not active

                    AES_SetNewStorageKey (StorageKey_pu8);
                    SetSdEncryptedHiddenState (TRUE);
                    SetSdEncryptedCardEnableState (TRUE);
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                    CI_TickLocalPrintf ("Hidden volume - SD card hidden AES LUN enabled\r\n");
                    break;
                case HIDDEN_VOLUME_OUTPUT_STATUS_WRONG_PASSWORD:
                    CI_TickLocalPrintf ("Hidden volume - wrong password\r\n");
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                    break;
                case HIDDEN_VOLUME_OUTPUT_STATUS_NO_USER_PASSWORD_UNLOCK:
                    CI_TickLocalPrintf ("Hidden volume - no user password\r\n");
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK, 0);
                    break;
                case HIDDEN_VOLUME_OUTPUT_STATUS_SMARTCARD_ERROR:
                    CI_TickLocalPrintf ("Hidden volume - smartcard error\r\n");
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR, 0);
                    break;
                default:
                    break;
            }

            HID_NextPasswordIsHiddenPassword_u32 = FALSE;   // switch off redirection of aes volume password
            break;

        case HTML_CMD_DISABLE_HIDDEN_AES_LUN:
            SetSdEncryptedCardEnableState (FALSE);
            SetSdEncryptedHiddenState (FALSE);
            memset (StorageKey_pu8, 0, 32); // Set dummy key
            AES_SetNewStorageKey ((u8 *) AES_DummyKey_au8); // Set dummy key
            HID_NextPasswordIsHiddenPassword_u32 = FALSE;

            vTaskDelay (3000);  // Wait 3 sec to send LUN not active
            CI_TickLocalPrintf ("SD card hidden AES LUN disabled\r\n");
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;


        case HTML_CMD_EXPORT_BINARY:
            if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[1]))
            {
                CI_TickLocalPrintf ("Export binary\r\n");
                // SetSdUncryptedCardEnableState (FALSE); // todo: Check disable access, don't work here

                FileIO_SaveAppImage_u8 ();

                SetSdUncryptedCardEnableState (FALSE);  // Disable access
                vTaskDelay (3000);
                SetSdUncryptedCardEnableState (TRUE);   // Enable access

                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("*** wrong password ***\r\n");
            }
            break;

        case HTML_CMD_GENERATE_NEW_KEYS:
        {
    u8 CmdOk_u8 = TRUE;
            CI_TickLocalPrintf ("Generate new keys\r\n");
            if (TRUE == BuildStorageKeys_u32 ((u8 *) & HID_String_au8[1]))
            {
                SetSdEncryptedCardEnableState (FALSE);
                SetSdEncryptedHiddenState (FALSE);
                memset (StorageKey_pu8, 0, 32);
                AES_SetNewStorageKey ((u8 *) AES_DummyKey_au8); // Set dummy key
            }
            else
            {
                CmdOk_u8 = FALSE;
            }

            CI_TickLocalPrintf ("Generate new keys for hidden volume slots\r\n");
            if (FALSE == InitHiddenSlots ())
            {
                CmdOk_u8 = FALSE;
            }

            CI_TickLocalPrintf ("Generate new keys for password safe store\r\n");
            if (FALSE == BuildPasswordSafeKey_u32 ())
            {
                CmdOk_u8 = FALSE;
            }

            if (TRUE == CmdOk_u8)
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
            }
        }
            break;

        case HTML_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
            CI_TickLocalPrintf ("Set readonly to unencrypted volume\r\n");
            if (TRUE == IW_SendToSC_PW1 (&HID_String_au8[1]))
            {
                SetSdUncryptedCardEnableState (FALSE);  // Disable access
                SetSdEncryptedCardEnableState (FALSE);
                SetSdUncryptedCardReadWriteEnableState (READ_ONLY_ACTIVE);
                vTaskDelay (6000);
                SetSdUncryptedCardEnableState (TRUE);   // Enable access

                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                CI_TickLocalPrintf ("ok\r\n");
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("*** wrong password ***\r\n");
            }
            break;

        case HTML_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
            CI_TickLocalPrintf ("Set readwrite to unencrypted volume\r\n");
            if (TRUE == IW_SendToSC_PW1 (&HID_String_au8[1]))
            {
                SetSdUncryptedCardEnableState (FALSE);  // Disable access
                SetSdEncryptedCardEnableState (FALSE);
                SetSdUncryptedCardReadWriteEnableState (READ_WRITE_ACTIVE);
                vTaskDelay (6000);
                SetSdUncryptedCardEnableState (TRUE);   // Enable access

                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                CI_TickLocalPrintf ("ok\r\n");
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("*** wrong password ***\r\n");
            }

            break;

        case HTML_CMD_SEND_PASSWORD_MATRIX:
            CI_TickLocalPrintf ("Get SEND_PASSWORD_MATRIX\r\n");

            PasswordMatrix_pu8 = GetCorrectMatrix ();

            Stick20HIDInitSendMatrixData (PasswordMatrix_pu8);

            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY, 0);
            break;

        case HTML_CMD_SEND_PASSWORD_MATRIX_PINDATA:
            CI_TickLocalPrintf ("Get SEND_PASSWORD_MATRIX_PINDATA\r\n");

            CI_TickLocalPrintf ("Data 1 : %s\r\n", HID_String_au8);

            // PasswordMatrix_pu8 = GetCorrectMatrix ();

            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;

        case HTML_CMD_SEND_PASSWORD_MATRIX_SETUP:
            CI_TickLocalPrintf ("Get SEND_PASSWORD_MATRIX_SETUP\r\n");
            CI_TickLocalPrintf ("Data : %s\r\n", HID_String_au8);
            WriteMatrixColumsUserPWToUserPage (&HID_String_au8[1]); // Data is in ASCII
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;


        case HTML_CMD_GET_DEVICE_STATUS:
            CI_TickLocalPrintf ("Get HTML_CMD_GET_DEVICE_STATUS\r\n");

            StickConfiguration_st.FirmwareLocked_u8 = FALSE;
            if (TRUE == flashc_is_security_bit_active ())
            {
                StickConfiguration_st.FirmwareLocked_u8 = TRUE;
            }

            GetSmartCardStatus (&StickConfiguration_st);
            Stick20HIDInitSendConfiguration (STICK20_SEND_STATUS_PIN);
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;

        case HTML_CMD_WRITE_STATUS_DATA:
            CI_TickLocalPrintf ("Get HTML_CMD_WRITE_STATUS_DATA\r\n");
            // GetStickStatusFromHID ((HID_Stick20AccessStatus_est*)HID_String_au8); // Changed
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;


        case HTML_SEND_HIDDEN_VOLUME_SETUP:
            CI_TickLocalPrintf ("Get HTML_SEND_HIDDEN_VOLUME_SETUP\r\n");
            InitRamdomBaseForHiddenKey ();
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;

        case HTML_CMD_SEND_PASSWORD:
            CI_TickLocalPrintf ("Get HTML_CMD_SEND_PASSWORD\r\n");

            HID_AdminPasswordEnabled_u32 = FALSE;
            HID_UserPasswordEnabled_u32 = FALSE;

            switch (HID_String_au8[0])
            {
                case 'P':
                    ret_u8 = IW_SendToSC_PW1 (&HID_String_au8[1]);
                    HID_UserPasswordEnabled_u32 = ret_u8;
                    strncpy ((char *) HID_UserPassword_au8, (char *) &HID_String_au8[1], HID_PASSWORD_LEN);
                    break;
                case 'A':
                    ret_u8 = IW_SendToSC_PW3 (&HID_String_au8[1]);
                    HID_AdminPasswordEnabled_u32 = ret_u8;
                    strncpy ((char *) HID_AdminPassword_au8, (char *) &HID_String_au8[1], HID_PASSWORD_LEN);
                    break;
            }

            if (TRUE == ret_u8)
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                CI_TickLocalPrintf ("password ok\r\n");
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("*** wrong password ***\r\n");
            }
            break;

        case HTML_CMD_SEND_NEW_PASSWORD:
            CI_TickLocalPrintf ("Get HTML_CMD_SEND_NEW_PASSWORD\r\n");
            ret_u8 = FALSE;
            switch (HID_String_au8[0])
            {
                case 'P':
                    if (TRUE == HID_UserPasswordEnabled_u32)
                    {
                        ret_u8 = LA_OpenPGP_V20_Test_ChangeUserPin (HID_UserPassword_au8, &HID_String_au8[1]);
                    }
                    else
                    {
                        CI_TickLocalPrintf ("** User password not present ***\r\n");
                    }
                    break;
                case 'A':
                    if (TRUE == HID_AdminPasswordEnabled_u32)
                    {
                        ret_u8 = LA_OpenPGP_V20_Test_ChangeAdminPin (HID_AdminPassword_au8, &HID_String_au8[1]);
                    }
                    else
                    {
                        CI_TickLocalPrintf ("** Admin password not present ***\r\n");
                    }
                    break;
            }

            if (TRUE == ret_u8)
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                CI_TickLocalPrintf ("password change ok\r\n");
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("*** password NOT changed ***\r\n");
            }

            // Clear password
            HID_AdminPasswordEnabled_u32 = FALSE;
            HID_UserPasswordEnabled_u32 = FALSE;
            memset (HID_UserPassword_au8, 0, HID_PASSWORD_LEN);
            memset (HID_AdminPassword_au8, 0, HID_PASSWORD_LEN);
            break;

        case HTML_CMD_CLEAR_NEW_SD_CARD_FOUND:
            CI_TickLocalPrintf ("Get HTML_CMD_CLEAR_NEW_SD_CARD_FOUND\r\n");
            if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[1]))
            {
                ClearNewSdCardFoundToFlash ();
                SetSdCardFilledWithRandomCharsToFlash ();
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                CI_TickLocalPrintf ("password ok\r\n");
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("*** wrong password ***\r\n");
            }
            break;

        case HTML_CMD_SEND_STARTUP:
            CI_TickLocalPrintf ("Get HTML_CMD_SEND_STARTUP\r\n");
            timestamp = getu64 (&HID_String_au8[0]);
            if (FALSE == CheckSystemtime ((u32) timestamp))
            {
                // todo
            }

            GetSmartCardStatus (&StickConfiguration_st);

            Stick20HIDInitSendConfiguration (STICK20_SEND_STARTUP);
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;
            /*
               case STICK20_CMD_SEND_PASSWORD_RETRY_COUNT : CI_TickLocalPrintf ("Get STICK20_CMD_SEND_PASSWORD_RETRY_COUNT\r\n");

               // Stick20HIDInitSendConfiguration (STICK20_SEND_STARTUP); UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK,0); break; */
        case HTML_CMD_HIDDEN_VOLUME_SETUP:
            CI_TickLocalPrintf ("Get HTML_CMD_HIDDEN_VOLUME_SETUP\r\n");
            ret_u8 = SetupUpHiddenVolumeSlot ((HiddenVolumeSetup_tst *) & HID_String_au8[0]);
            switch (ret_u8)
            {
                case HIDDEN_VOLUME_OUTPUT_STATUS_OK:
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                    break;
                case HIDDEN_VOLUME_OUTPUT_STATUS_WRONG_PASSWORD:
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                    break;
                case HIDDEN_VOLUME_OUTPUT_STATUS_NO_USER_PASSWORD_UNLOCK:
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK, 0);
                    break;
                case HIDDEN_VOLUME_OUTPUT_STATUS_SMARTCARD_ERROR:
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR, 0);
                    break;
                default:
                    break;
            }
            break;

        case HTML_CMD_CLEAR_STICK_KEYS_NOT_INITIATED:
            CI_TickLocalPrintf ("Get HTML_CMD_CLEAR_STICK_KEYS_NOT_INITIATED\r\n");
            ClearStickKeysNotInitatedToFlash ();
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;

        case HTML_CMD_CLEAR_LOCK_STICK_HARDWARE:
            CI_TickLocalPrintf ("Get HTML_CMD_CLEAR_LOCK_STICK_HARDWARE\r\n");
            if (TRUE == IW_SendToSC_PW3 (&HID_String_au8[1]))
            {
                CI_TickLocalPrintf ("Lock firmware\r\n");
                flashc_activate_security_bit ();    // Debugging disabled, only chip erase works (bootloader is save) , AES storage keys and setup
                                                    // are erased
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            }
            else
            {
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                CI_TickLocalPrintf ("*** worng password ***\r\n");
            }
            break;

        case HTML_CMD_PRODUCTION_TEST:
            CI_TickLocalPrintf ("Get HTML_CMD_PRODUCTION_TEST\r\n");
            GetProductionInfos (&Stick20ProductionInfos_st);
            Stick20HIDInitSendConfiguration (STICK20_SEND_PRODUCTION_TEST);
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
            break;

        case HTML_CMD_CHANGE_UPDATE_PIN:
            CI_TickLocalPrintf ("Get HTML_CMD_CHANGE_UPDATE_PIN\r\n");

            Len = strlen ((char*)&HID_String_au8[1]);

            if (TRUE == CheckUpdatePin (&HID_String_au8[1], Len))
            {
                Len = strlen ((char*)&HID_String_au8[1]);
                if (TRUE == StoreNewUpdatePinHashInFlash (&HID_String_au8[16], Len))    // Start of new PW
                {
                    CI_TickLocalPrintf ("Update PIN changed\r\n");
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_OK, 0);
                }
                else
                {
                    CI_TickLocalPrintf ("*** worng password len ***\r\n");
                    UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
                }
            }
            else
            {
                CI_TickLocalPrintf ("*** worng password ***\r\n");
                UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD, 0);
            }
            break;


        default:
            CI_TickLocalPrintf ("HID_ExcuteCmd: Get unknown command: %d \r\n", Cmd_u8);
            UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_IDLE, 0);
            break;
    }

    HID_CmdGet_u8 = HTML_CMD_NOTHING;
    memset (HID_String_au8, 0, 50); // Clear input data, with possible sent passwords

    /*
       HID_SmartcardAccess_u8 = FALSE; */


}


/*******************************************************************************

  GetSmartCardStatus

  Changes
  Date      Author          Info
  02.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 GetSmartCardStatus (typeStick20Configuration_st * Status_st)
{
u8 Text_u8[20];
u32 Ret_u32 = FALSE;

    Status_st->ActiveSmartCardID_u32 = 0;
    Status_st->UserPwRetryCount = 77;
    Status_st->AdminPwRetryCount = 77;

    // Check for active smartcard
    if (FALSE == ISO7816_IsSmartcardUsable ())
    {
        return (FALSE);
    }


    /* Get smartcard ID from AID */
    Ret_u32 = LA_OpenPGP_V20_GetAID ((char *) Text_u8);
    if (TRUE == Ret_u32)
    {
        Status_st->ActiveSmartCardID_u32 = (u32) Text_u8[10];
        Status_st->ActiveSmartCardID_u32 = Status_st->ActiveSmartCardID_u32 << 8;
        Status_st->ActiveSmartCardID_u32 += (u32) Text_u8[11];
        Status_st->ActiveSmartCardID_u32 = Status_st->ActiveSmartCardID_u32 << 8;
        Status_st->ActiveSmartCardID_u32 += (u32) Text_u8[12];
        Status_st->ActiveSmartCardID_u32 = Status_st->ActiveSmartCardID_u32 << 8;
        Status_st->ActiveSmartCardID_u32 += (u32) Text_u8[13];
    }


    /* Get password retry counts */
    Ret_u32 = LA_OpenPGP_V20_GetPasswordstatus ((char *) Text_u8);
    if (TRUE == Ret_u32)
    {
        Status_st->UserPwRetryCount = Text_u8[4];
        Status_st->AdminPwRetryCount = Text_u8[6];
    }
    else
    {
        Status_st->UserPwRetryCount = 88;
        Status_st->AdminPwRetryCount = 88;
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

void GetProductionInfos (typeStick20ProductionInfos_st * Infos_st)
{
typeStick20Configuration_st SC_Status_st;
volatile u32* id_data = (u32 *) 0x80800204; // Place of 120 bit CPU ID
u32 i;
u32 CPU_ID_u32;
cid_t* cid;

    // Clear data field
    memset ((void *) Infos_st, 0, sizeof (Infos_st));

    Infos_st->FirmwareVersion_au8[0] = VERSION_MAJOR;
    Infos_st->FirmwareVersion_au8[1] = VERSION_MINOR;
    Infos_st->FirmwareVersion_au8[2] = 0;   // Build number not used
    Infos_st->FirmwareVersion_au8[3] = 0;   // Build number not used

    // Get smartcard infos
    GetSmartCardStatus (&SC_Status_st);

    // Run the check only if the initial pw is aktive
    if (3 == SC_Status_st.UserPwRetryCount)
    {
        if (FALSE == LA_OpenPGP_V20_Test_SendUserPW2 ((unsigned char *) "123456"))
        {
            CI_TickLocalPrintf ("GetProductionInfos: Intial password is not activ\r\n");
            Infos_st->SC_AdminPwRetryCount = 99;    // Marker for wrong pw
            return;
        }
    }
    else
    {
        CI_TickLocalPrintf ("GetProductionInfos: Password retry count is not 3 : %d\r\n", SC_Status_st.UserPwRetryCount);
        Infos_st->SC_AdminPwRetryCount = 88;    // Marker for wrong pw retry count
        return;
    }


    // Get XORed CPU ID
    CPU_ID_u32 = 0;
    for (i = 0; i < 4; i++)
    {
        CPU_ID_u32 ^= id_data[i];
    }
    Infos_st->CPU_CardID_u32 = CPU_ID_u32;


    // Save smartcard infos
    Infos_st->SC_UserPwRetryCount = SC_Status_st.UserPwRetryCount;
    Infos_st->SC_AdminPwRetryCount = SC_Status_st.AdminPwRetryCount;
    Infos_st->SmartCardID_u32 = SC_Status_st.ActiveSmartCardID_u32;

    // Get SD card infos
    cid = (cid_t *) GetSdCidInfo ();

    Infos_st->SD_CardID_u32 = (cid->psnh << 8) + cid->psnl;
    Infos_st->SD_Card_ManufacturingYear_u8 = cid->mdt / 16;
    Infos_st->SD_Card_ManufacturingMonth_u8 = cid->mdt % 0x0f;
    Infos_st->SD_Card_OEM_u16 = cid->oid;
    Infos_st->SD_Card_Manufacturer_u8 = cid->mid;

    // Get SD card speed
    Infos_st->SD_WriteSpeed_u16 = SD_SpeedTest ();
}

/*******************************************************************************

  LockDevice

  Changes
  Date      Author          Info
  18.18.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void LockDevice (void)
{
    // Disable volume access
    SetSdEncryptedCardEnableState (FALSE);
    SetSdEncryptedHiddenState (FALSE);

    AES_SetNewStorageKey ((u8 *) AES_DummyKey_au8); // Set dummy AES key

    HID_AdminPasswordEnabled_u32 = FALSE;
    HID_UserPasswordEnabled_u32 = FALSE;
    memset (HID_UserPassword_au8, 0, HID_PASSWORD_LEN);
    memset (HID_AdminPassword_au8, 0, HID_PASSWORD_LEN);

    // Disable password safe
    PWS_DisableKey ();

    // Powerdown smartcard
    ISO7816_SetLockCounter (0);
    CCID_SmartcardOff_u8 ();

    // or DFU_ResetCPU ();
}
