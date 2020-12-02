/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-07-12
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
 * FlashStorage.c
 *
 *  Created on: 12.07.2013
 *      Author: RB
 */


#include <avr32/io.h>
#include <stddef.h>
#include "compiler.h"
#include "flashc.h"
#include "string.h"

#include "conf_access.h"
#include "conf_sd_mmc_mci.h"
#include "sd_mmc_mci.h"
#include "sd_mmc_mci_mem.h"


#include "global.h"
#include "tools.h"
#include "OTP/report_protocol.h"
#include "FlashStorage.h"
#include "HiddenVolume.h"
#include "OTP/hotp.h"
#include "password_safe.h"
#include "DFU_test.h"
#include "CCID/USART/ISO7816_USART.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"

#ifdef FREERTOS_USED
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

typeStick20Configuration_st StickConfiguration_st;


/*******************************************************************************

 Local defines

*******************************************************************************/

/*

   Userpage layout

   Byte 
   0 - 31 AES Storage key
   32 - 51 Matrix columns for user password
   52 - 71 Matrix columns for admin password 
   72 - 101 Stick Configuration 
   102 - 133 Base for AES key hidden volume (32 byte) 
   134 - 137 ID of sd card (4 byte) 
   138 - 141 Last stored real timestamp (4 byte) 
   142 - 145 ID of sc card (4 byte) 
   146 - 177 XOR mask for sc tranfered keys (32 byte) 
   178 - 209 Password safe key (32 byte) 
   210 - 241 Update PIN (32 byte) 
   242 - 251 Update PIN SALT (10 byte)
 */


typedef struct {
    u8 AES_key[32];
    u8 matrix_user[20];
    u8 matrix_admin[20];
    u8 stick_configuration[30];
    u8 hidden_volume_AES_key_base[32];
    u8 sdcard_id[4];
    u8 last_timestamp_real[4];
    u8 sccard_id[4];
    u8 xor_mask_for_SC_keys[32];
    u8 password_safe_AES_key[32];
    u8 update_PIN[32];
    u8 update_PIN_salt[10];
} UserPage;

UserPage * user_page = (UserPage * )AVR32_FLASHC_USER_PAGE;

#define TOKENPASTE(a, b) a ## b // "##" is the "Token Pasting Operator"
#define TOKENPASTE2(a,b) TOKENPASTE(a, b) // expand then paste
#define static_assert(x, msg) enum { TOKENPASTE2(ASSERT_line_,__LINE__) \
    = 1 / (msg && (x)) }


static_assert(sizeof(UserPage) == 252, "size of conf struct invalid");
static_assert(sizeof(UserPage) <= 512, "size of conf struct is too big");
static_assert(sizeof(user_page->stick_configuration) >= sizeof(typeStick20Configuration_st), "size of conf struct is too big");


/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

u8 read_configuration_or_init_u8(void);

#define LOCAL_DEBUG

#define AES_KEYSIZE_256_BIT     32  // 32 * 8 = 256
#define UPDATE_PIN_MAX_SIZE     20
#define UPDATE_PIN_SALT_SIZE    10

/*******************************************************************************

  WriteAESStorageKeyToUserPage

  Reviews
  Date      Reviewer        Info
  03.02.14  RB              First review

*******************************************************************************/

u8 WriteAESStorageKeyToUserPage (u8 * data)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE, data, 32, TRUE);
    return (TRUE);
}

/*******************************************************************************

  ReadAESStorageKeyToUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 ReadAESStorageKeyToUserPage (u8 * data)
{
    memcpy (data, (void *) (AVR32_FLASHC_USER_PAGE), 32);
    return (TRUE);
}


/*******************************************************************************

  WriteMatrixColumsUserPWToUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 WriteMatrixColumsUserPWToUserPage (u8 * data)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 32, data, 20, TRUE);
    return (TRUE);
}

/*******************************************************************************

  ReadMatrixColumsUserPWFromUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 ReadMatrixColumsUserPWFromUserPage (u8 * data)
{
    memcpy (data, (void *) (AVR32_FLASHC_USER_PAGE + 32), 20);
    return (TRUE);
}

/*******************************************************************************

  WriteMatrixColumsAdminPWFromUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 WriteMatrixColumsAdminPWFromUserPage (u8 * data)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 52, data, 20, TRUE);
    return (TRUE);
}

/*******************************************************************************

  ReadMatrixColumsAdminPWFromUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 ReadMatrixColumsAdminPWFromUserPage (u8 * data)
{
    memcpy (data, (void *) (AVR32_FLASHC_USER_PAGE + 52), 20);
    return (TRUE);
}

/*******************************************************************************

  WriteStickConfigurationToUserPage

  Changes
  Date      Author          Info
  03.02.14  RB              Change to struct

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 WriteStickConfigurationToUserPage (void)
{
    taskENTER_CRITICAL();
    // Set actual firmware version
    StickConfiguration_st.VersionInfo_au8[0] = VERSION_MAJOR;
    StickConfiguration_st.VersionInfo_au8[1] = VERSION_MINOR;
    StickConfiguration_st.VersionInfo_au8[2] = 0;
    StickConfiguration_st.VersionInfo_au8[3] = INTERNAL_VERSION_NR;   // Internal version nr

    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 72, &StickConfiguration_st, 30, TRUE);
    taskEXIT_CRITICAL();
    
    return (TRUE);
}


u8 ReadConfigurationSuccesfull(void) {
    if (MAGIC_NUMBER_STICK20_CONFIG != StickConfiguration_st.MagicNumber_StickConfig_u16)
    {
        return (FALSE);
    }
    return (TRUE);
}

/*******************************************************************************

  ReadStickConfigurationFromUserPage

  Changes
  Date      Author          Info
  03.02.14  RB              Change to struct

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 ReadStickConfigurationFromUserPage (void)
{
    // TODO should be mutexed with InitStickConfigurationToUserPage_u8

    u8 UserPwRetryCount;
    u8 AdminPwRetryCount;
    u32 ActiveSmartCardID_u32;

    taskENTER_CRITICAL ();
    // Save dynamic data
    UserPwRetryCount = StickConfiguration_st.UserPwRetryCount;
    AdminPwRetryCount = StickConfiguration_st.AdminPwRetryCount;
    ActiveSmartCardID_u32 = StickConfiguration_st.ActiveSmartCardID_u32;

    memcpy (&StickConfiguration_st, (void *) (AVR32_FLASHC_USER_PAGE + 72), sizeof (typeStick20Configuration_st));

    // Write actual version info
    StickConfiguration_st.VersionInfo_au8[0] = VERSION_MAJOR;
    StickConfiguration_st.VersionInfo_au8[1] = VERSION_MINOR;
    StickConfiguration_st.VersionInfo_au8[2] = 0;   // Build number not used
    StickConfiguration_st.VersionInfo_au8[3] = INTERNAL_VERSION_NR;   // Internal version nr

    // Restore dynamic data
    StickConfiguration_st.UserPwRetryCount = UserPwRetryCount;
    StickConfiguration_st.AdminPwRetryCount = AdminPwRetryCount;
    StickConfiguration_st.ActiveSmartCardID_u32 = ActiveSmartCardID_u32;
    taskEXIT_CRITICAL();

    return ReadConfigurationSuccesfull();
}

/*******************************************************************************

  InitStickConfigurationToUserPage_u8

  Changes
  Date      Author          Info
  03.02.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 InitStickConfigurationToUserPage_u8 (void)
{
    // TODO should be mutexed with ReadStickConfigurationFromUserPage

    taskENTER_CRITICAL ();

    StickConfiguration_st.MagicNumber_StickConfig_u16 = MAGIC_NUMBER_STICK20_CONFIG;
    StickConfiguration_st.ReadWriteFlagUncryptedVolume_u8 = READ_ONLY_ACTIVE;
    StickConfiguration_st.ReadWriteFlagCryptedVolume_u8 = READ_WRITE_ACTIVE;
    StickConfiguration_st.ReadWriteFlagHiddenVolume_u8 = READ_WRITE_ACTIVE;
    StickConfiguration_st.FirmwareLocked_u8 = 0;
    StickConfiguration_st.ActiveSD_CardID_u32 = 0;
    StickConfiguration_st.VersionInfo_au8[0] = VERSION_MAJOR;
    StickConfiguration_st.VersionInfo_au8[1] = VERSION_MINOR;
    StickConfiguration_st.VersionInfo_au8[2] = 0;   // Build number not used
    StickConfiguration_st.VersionInfo_au8[3] = INTERNAL_VERSION_NR;   // Internal version nr
    StickConfiguration_st.NewSDCardFound_u8 = 0;
    StickConfiguration_st.SDFillWithRandomChars_u8 = FALSE;
    StickConfiguration_st.VolumeActiceFlag_u8 = 0;
    StickConfiguration_st.NewSmartCardFound_u8 = 0;
    StickConfiguration_st.ActiveSmartCardID_u32 = 0;
    StickConfiguration_st.StickKeysNotInitiated_u8 = TRUE;

    WriteStickConfigurationToUserPage ();
    InitUpdatePinHashInFlash ();

    taskEXIT_CRITICAL();

    return (TRUE);
}

/*******************************************************************************

  WriteHiddenVolumeSlotKey

  Stores the encrypted hidden volume slot key

  Byte
  Len = 32 Byte

  Changes
  Date      Author          Info
  14.04.14  RB              Renaming

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 WriteHiddenVolumeSlotKey (u8 * data)
{
    flashc_memcpy ((void *) (AVR32_FLASHC_USER_PAGE + 102), data, 32, TRUE);
    return (TRUE);
}

/*******************************************************************************

  ReadHiddenVolumeSlotsKey

  Read the encrypted hidden volume slot key

  Changes
  Date      Author          Info
  14.04.14  RB              Renaming


  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 ReadHiddenVolumeSlotsKey (u8 * data)
{
    memcpy (data, (void *) (AVR32_FLASHC_USER_PAGE + 102), 32);
    return (TRUE);
}




/*******************************************************************************

  SendStickStatusToHID

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of sending volume status

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void SendStickStatusToHID (typeStick20Configuration_st * Status_st)
{
cid_t* cid;

    read_configuration_or_init_u8();

    memcpy (Status_st, &StickConfiguration_st, sizeof (typeStick20Configuration_st));   // Not the retry counter and sc serial no

    // Set the actual volume status
    Status_st->VolumeActiceFlag_u8 = 0;

    if (TRUE == GetSdUncryptedCardEnableState ())
    {
        Status_st->VolumeActiceFlag_u8 |= (1 << SD_UNCRYPTED_VOLUME_BIT_PLACE);
    }

    // Only 1 cypted volume could be active
    if (TRUE == GetSdEncryptedCardEnableState ())
    {
        if (TRUE == GetSdEncryptedHiddenState ())
        {
            Status_st->VolumeActiceFlag_u8 |= (1 << SD_HIDDEN_VOLUME_BIT_PLACE);
        }
        else
        {
            Status_st->VolumeActiceFlag_u8 |= (1 << SD_CRYPTED_VOLUME_BIT_PLACE);
        }
    }

    // Read actual SD id
    cid = (cid_t *) GetSdCidInfo ();
    Status_st->ActiveSD_CardID_u32 = (cid->psnh << 8) + cid->psnl;
    // ReadSdId (&Status_st->ActiveSD_CardID_u32);

    Status_st->FirmwareLocked_u8 = FALSE;
    if (TRUE == flashc_is_security_bit_active ())
    {
        Status_st->FirmwareLocked_u8 = TRUE;
    }
}


/*******************************************************************************

  Read_ReadWriteStatusEncryptedVolume_u8

  Changes
  Date      Author          Info
  11.12.17  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 read_configuration_or_init_u8(void){
    taskENTER_CRITICAL();
    // If configuration not found then init it
    if (FALSE == ReadStickConfigurationFromUserPage ())
    {
        InitStickConfigurationToUserPage_u8 ();
    }
    taskEXIT_CRITICAL();
    
    return 0;
}

u8 Read_ReadWriteStatusEncryptedVolume_u8 (void)
{
    read_configuration_or_init_u8();
    return (StickConfiguration_st.ReadWriteFlagCryptedVolume_u8);
}

/*******************************************************************************

  Write_ReadWriteStatusEncryptedVolume_u8

  Changes
  Date      Author          Info
  11.12.17  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 Write_ReadWriteStatusEncryptedVolume_u8 (u8 NewStatus_u8)
{
    read_configuration_or_init_u8();
    StickConfiguration_st.ReadWriteFlagCryptedVolume_u8 = NewStatus_u8;
    WriteStickConfigurationToUserPage ();

    return (TRUE);
}

/*******************************************************************************

  Read_ReadWriteStatusUncryptedVolume_u8

  Changes
  Date      Author          Info
  03.02.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 Read_ReadWriteStatusUncryptedVolume_u8 (void)
{
    read_configuration_or_init_u8();
    return (StickConfiguration_st.ReadWriteFlagUncryptedVolume_u8);
}

/*******************************************************************************

  Write_ReadWriteStatusUncryptedVolume_u8

  Changes
  Date      Author          Info
  03.02.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 Write_ReadWriteStatusUncryptedVolume_u8 (u8 NewStatus_u8)
{
    read_configuration_or_init_u8();
    StickConfiguration_st.ReadWriteFlagUncryptedVolume_u8 = NewStatus_u8;
    WriteStickConfigurationToUserPage ();

    return (TRUE);
}

/*******************************************************************************

  WriteSdId

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of save SD id in CPU flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WriteSdId (u32 * SdId_u32)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 134, SdId_u32, 4, TRUE);

    StickConfiguration_st.ActiveSD_CardID_u32 = *SdId_u32;
    return (TRUE);
}

/*******************************************************************************

  ReadSdId

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of save SD id in CPU flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ReadSdId (u32 * SdId_u32)
{
    memcpy (SdId_u32, (void *) (AVR32_FLASHC_USER_PAGE + 134), 4);

    StickConfiguration_st.ActiveSD_CardID_u32 = *SdId_u32;
    return (TRUE);
}



/*******************************************************************************

  WriteNewSdCardFoundToFlash

  NewSDCardFound_u8
  Bit 0 = 0   New SD card found
  Bit 0 = 1   Previous SD card found


  SDFillWithRandomChars_u8
  Bit 0 = 0   SD card is *** not *** filled with random chars
  Bit 0 = 1   SD card is filled with random chars

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of save new SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WriteNewSdCardFoundToFlash (u32 * SdId_u32)
{
    read_configuration_or_init_u8();

    CI_LocalPrintf ("*** New SD card found ***  Serial number 0x%08x\r\n", *SdId_u32);

    WriteSdId (SdId_u32);

    StickConfiguration_st.NewSDCardFound_u8 |= 0x01;    // Bit 0 = new card found
    // StickConfiguration_st.NewSDCardFound_u8 += 2; // add change counter +1

    StickConfiguration_st.SDFillWithRandomChars_u8 &= 0xFE; // Clear the "card with random chars filled" bit

    WriteStickConfigurationToUserPage ();

    return (TRUE);
}

/*******************************************************************************

  SetSdCardFilledWithRandomsToFlash

  Changes
  Date      Author          Info
  10.02.14  RB              Implementation: New SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 SetSdCardFilledWithRandomsToFlash (void)
{
    read_configuration_or_init_u8();

    CI_LocalPrintf ("SD is filled with random chars\r\n");

    StickConfiguration_st.SDFillWithRandomChars_u8 |= 0x01; // Set the "SD card filled with randoms" bit
    // StickConfiguration_st.SDFillWithRandomChars_u8 += 2; // add counter +1

    WriteStickConfigurationToUserPage ();

    return (TRUE);
}

/*******************************************************************************

  ClearNewSdCardFoundToFlash

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of clear new SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ClearNewSdCardFoundToFlash (void)
{
    read_configuration_or_init_u8();

    CI_LocalPrintf ("Clear new SD card found\r\n");

    StickConfiguration_st.NewSDCardFound_u8 &= 0xFE;    // Clear the "new SD card found" bit

    WriteStickConfigurationToUserPage ();

    return (TRUE);
}

/*******************************************************************************

  SetSdCardFilledWithRandomCharsToFlash

  Changes
  Date      Author          Info
  06.05.14  RB              Implementation of clear new SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 SetSdCardFilledWithRandomCharsToFlash (void)
{
    read_configuration_or_init_u8();

    CI_LocalPrintf ("Set new SD card filled with random chars\r\n");

    StickConfiguration_st.SDFillWithRandomChars_u8 |= 0x01; // Set the "SD card filled with randoms" bit

    WriteStickConfigurationToUserPage ();

    return (TRUE);
}

/*******************************************************************************

  SetSdCardNotFilledWithRandomCharsToFlash

  Bit 0 = 0   SD card is *** not *** filled with random chars
  Bit 0 = 1   SD card is filled with random chars

  Changes
  Date      Author          Info
  06.05.14  RB              Implementation of clear new SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 SetSdCardNotFilledWithRandomCharsToFlash (void)
{
    read_configuration_or_init_u8();

    CI_LocalPrintf ("Set new SD card *** not *** filled with random chars\r\n");

    StickConfiguration_st.SDFillWithRandomChars_u8 &= 0xFE; // Clear the "card with random chars filled" bit

    WriteStickConfigurationToUserPage ();

    return (TRUE);
}


/*******************************************************************************

  SetStickKeysNotInitatedToFlash

  Changes
  Date      Author          Info
  05.05.14  RB              Implementation of clear new SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 SetStickKeysNotInitatedToFlash (void)
{
    read_configuration_or_init_u8();

    CI_LocalPrintf ("Set stick keys not initiated\r\n");

    StickConfiguration_st.StickKeysNotInitiated_u8 = TRUE;

    WriteStickConfigurationToUserPage ();

    return (TRUE);
}

/*******************************************************************************

  ClearStickKeysNotInitatedToFlash

  Changes
  Date      Author          Info
  05.05.14  RB              Implementation of clear new SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ClearStickKeysNotInitatedToFlash (void)
{
    read_configuration_or_init_u8();

    CI_LocalPrintf ("Clear: Stick keys not initiated\r\n");

    StickConfiguration_st.StickKeysNotInitiated_u8 = FALSE;

    WriteStickConfigurationToUserPage ();

    return (TRUE);
}

/*******************************************************************************

  CheckForNewFirmwareVersion

  Changes
  Date      Author          Info
  05.07.14  RB              Implementation

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 CheckForNewFirmwareVersion (void)
{
static u8 UpdateFlag_u8 = FALSE;
u8 update_u8;

    if (TRUE == UpdateFlag_u8)
    {
        return (TRUE);
    }

    UpdateFlag_u8 = TRUE;   // Run only once

    read_configuration_or_init_u8();

    update_u8 = FALSE;
    if (VERSION_MAJOR != StickConfiguration_st.VersionInfo_au8[0])
    {
        update_u8 = TRUE;
    }

    if (VERSION_MINOR != StickConfiguration_st.VersionInfo_au8[1])
    {
        update_u8 = TRUE;
    }

    /*
       StickConfiguration_st.VersionInfo_au8[2] = 0; // Build number not used StickConfiguration_st.VersionInfo_au8[3] = 0; // Build number not used */

    if (TRUE == update_u8)
    {
        WriteStickConfigurationToUserPage ();
    }

    return (TRUE);
}

/*******************************************************************************

  WriteDatetime

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of save datetime in flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WriteDatetime (u32 Datetime_u32)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 138, &Datetime_u32, 4, TRUE);
    return (TRUE);
}

/*******************************************************************************

  ReadDatetime

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of read datetime in flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ReadDatetime (u32 * Datetime_u32)
{
    memcpy (Datetime_u32, (void *) (AVR32_FLASHC_USER_PAGE + 138), 4);
    return (TRUE);
}


/*******************************************************************************

  WriteScId

  Changes
  Date      Author          Info
  20.05.14  RB              Implementation of save SC id in CPU flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WriteScId (u32 * ScId_u32)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 142, ScId_u32, 4, TRUE);

    StickConfiguration_st.ActiveSmartCardID_u32 = *ScId_u32;
    return (TRUE);
}

/*******************************************************************************

  ReadSdId

  Changes
  Date      Author          Info
  20.05.14  RB              Implementation of save SC id in CPU flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ReadScId (u32 * ScId_u32)
{
    memcpy (ScId_u32, (void *) (AVR32_FLASHC_USER_PAGE + 142), 4);

    StickConfiguration_st.ActiveSmartCardID_u32 = *ScId_u32;
    return (TRUE);
}


/*******************************************************************************

  WriteXorPatternToFlash

  Changes
  Date      Author          Info
  20.05.14  RB              Implementation of save xor pattern to flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WriteXorPatternToFlash (u8 * XorPattern_pu8)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 146, XorPattern_pu8, 32, TRUE);

    return (TRUE);
}

/*******************************************************************************

  ReadXorPatternFromFlash

  Changes
  Date      Author          Info
  20.05.14  RB              Implementation of read xor pattern from flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ReadXorPatternFromFlash (u8 * XorPattern_pu8)
{
    memcpy (XorPattern_pu8, (void *) (AVR32_FLASHC_USER_PAGE + 146), 32);

    return (TRUE);
}


/*******************************************************************************

  WritePasswordSafeKey

  Stores the encrypted password safe key

  Byte
  Len = 32 Byte

  Changes
  Date      Author          Info
  01.08.14  RB              Renaming

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WritePasswordSafeKey (u8 * data)
{
    flashc_memcpy ((void *) (AVR32_FLASHC_USER_PAGE + 178), data, 32, TRUE);
    return (TRUE);
}

/*******************************************************************************

  ReadPasswordSafeKey

  Read the encrypted password safe key

  Changes
  Date      Author          Info
  01.08.14  RB              Renaming


  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ReadPasswordSafeKey (u8 * data)
{
    memcpy (data, (void *) (AVR32_FLASHC_USER_PAGE + 178), 32);
    return (TRUE);
}

/*******************************************************************************

  WriteUpdatePinHashToFlash

  Changes
  Date      Author          Info
  20.06.15  RB              Implementation of write update pin to flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WriteUpdatePinHashToFlash (u8 * PIN_Hash_pu8)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 210, PIN_Hash_pu8, 32, TRUE);

    return (TRUE);
}

/*******************************************************************************

  ReadUpdatePinHashFromFlash

  Changes
  Date      Author          Info
  20.06.15  RB              Implementation of read update pin from flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ReadUpdatePinHashFromFlash (u8 * PIN_Hash_pu8)
{
    memcpy (PIN_Hash_pu8, (void *) (AVR32_FLASHC_USER_PAGE + 210), 32);

    return (TRUE);
}

/*******************************************************************************

  WriteUpdatePinSaltToFlash

  Changes
  Date      Author          Info
  20.06.15  RB              Implementation of write update pin salt to flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WriteUpdatePinSaltToFlash (u8 * PIN_pu8)
{
    flashc_memcpy (AVR32_FLASHC_USER_PAGE + 242, PIN_pu8, 10, TRUE);

    return (TRUE);
}

/*******************************************************************************

  ReadUpdatePinSaltFromFlash

  Changes
  Date      Author          Info
  20.06.15  RB              Implementation of read update pin salt from flash

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 ReadUpdatePinSaltFromFlash (u8 * PIN_pu8)
{
    memcpy (PIN_pu8, (void *) (AVR32_FLASHC_USER_PAGE + 242), 10);

    return (TRUE);
}


/*******************************************************************************

  EraseLocalFlashKeyValues_u32

  Changes
  Date      Author          Info
  10.09.14  RB              Implementation


  Reviews
  Date      Reviewer        Info

*******************************************************************************/

#if (defined __GNUC__) && (defined __AVR32__)
__attribute__ ((__aligned__ (4)))
#elif (defined __ICCAVR32__)
#pragma data_alignment = 4
#endif
u8 EraseStoreData_au8[256];

u32 EraseLocalFlashKeyValues_u32 (void)
{
    u32 i;
    u32 i1;

    // Save data for  firmware password
    u8 UpdatePinSalt_u8[UPDATE_PIN_SALT_SIZE];
    u8 UpdatePinHash_u8[AES_KEYSIZE_256_BIT];

    ReadUpdatePinSaltFromFlash (UpdatePinSalt_u8);
    ReadUpdatePinHashFromFlash (UpdatePinHash_u8);


    // Clear user page
    for (i1 = 0; i1 < 5; i1++)
    {
        for (i = 0; i < 256; i++)
        {
            EraseStoreData_au8[i] = (u8) (rand () % 256);
        }
        flashc_memcpy ((void *) AVR32_FLASHC_USER_PAGE, EraseStoreData_au8, 256, TRUE);
    }

//    flashc_erase_user_page (TRUE);
    DFU_FirmwareResetUserpage ();

    // Set default values
    InitStickConfigurationToUserPage_u8 ();

    DFU_DisableFirmwareUpdate ();   // Stick always starts in application mode
    CheckForNewSdCard ();   // Get SD ID

    // Clear password safe
    for (i1 = 0; i1 < 5; i1++)
    {
        for (i = 0; i < 256; i++)
        {
            EraseStoreData_au8[i] = (u8) (rand () % 256);
        }
        flashc_memcpy ((void *) (PWS_FLASH_START_ADDRESS), EraseStoreData_au8, 256, TRUE);
        flashc_memcpy ((void *) (PWS_FLASH_START_ADDRESS + 256), EraseStoreData_au8, 256, TRUE);
    }

    flashc_erase_page (PWS_FLASH_START_PAGE, TRUE);

    // Clear OTP
    for (i1 = 0; i1 < 5; i1++)
    {
        for (i = 0; i < 256; i++)
        {
            EraseStoreData_au8[i] = (u8) (rand () % 256);
        }
        for (i = 0; i < 10; i++)
        {
            flashc_memcpy ((void *) (SLOTS_ADDRESS + i * 512), EraseStoreData_au8, 256, TRUE);
            flashc_memcpy ((void *) (SLOTS_ADDRESS + i * 512 + 256), EraseStoreData_au8, 256, TRUE);
        }
    }

    for (i = 0; i < 10; i++)
    {
        flashc_erase_page (OTP_FLASH_START_PAGE + i, TRUE);
    }

    // Clear hidden volumes
    for (i1 = 0; i1 < 5; i1++)
    {
        for (i = 0; i < 256; i++)
        {
            EraseStoreData_au8[i] = (u8) (rand () % 256);
        }
        for (i = 0; i < 2; i++)
        {
            flashc_memcpy ((void *) (HV_MAGIC_NUMBER_ADDRESS + i * 512), EraseStoreData_au8, 256, TRUE);
            flashc_memcpy ((void *) (HV_MAGIC_NUMBER_ADDRESS + i * 512 + 256), EraseStoreData_au8, 256, TRUE);
        }
    }
    for (i = 0; i < 10; i++)
    {
        flashc_erase_page (HV_FLASH_START_PAGE + i, TRUE);
    }

// Save update pin data
    WriteUpdatePinSaltToFlash (UpdatePinSalt_u8);
    WriteUpdatePinHashToFlash (UpdatePinHash_u8);

    return (TRUE);
}

/*******************************************************************************

  CheckUpdatePin

  Changes
  Date      Reviewer        Info
  20.06.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
void pbkdf2 (u8 * output, const u8 * password, const u32 password_length, const u8 * salt, const u32 salt_length);

u8 CheckUpdatePin (u8 * Password_pu8, u32 PasswordLen_u32)
{
    u8 i;
    u8 UpdateSaltInit;
    u8 output_au8[64];
    u8 UpdatePinSalt_u8[UPDATE_PIN_SALT_SIZE];
    u8 UpdatePinHash_u8[AES_KEYSIZE_256_BIT];

    ReadUpdatePinSaltFromFlash (UpdatePinSalt_u8);

// To avoid bug when a update pin is not initiated after firmware flash
    UpdateSaltInit = FALSE;
    for (i=0;i<UPDATE_PIN_SALT_SIZE;i++)
    {
      if (0xFF != UpdatePinSalt_u8[i])
      {
        UpdateSaltInit = TRUE;
      }
    }
    if (FALSE == UpdateSaltInit)
    {
      StoreNewUpdatePinHashInFlash ((u8 *) "12345678", 8);    // Init the update pin with the default
      ReadUpdatePinSaltFromFlash (UpdatePinSalt_u8);
    }

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("CheckUpdatePin\r\n");
    CI_LocalPrintf ("Password_pu8 : Len %2d -%s-\r\n", PasswordLen_u32, Password_pu8);
    CI_LocalPrintf ("Salt         : ");
    HexPrint (UPDATE_PIN_SALT_SIZE, UpdatePinSalt_u8);
    CI_LocalPrintf ("\r\n");
#endif

    if (UPDATE_PIN_MAX_SIZE < PasswordLen_u32)
    {
        return (FALSE);
    }


#ifndef SAVE_FLASH_MEMORY_NO_PBKDF2
    pbkdf2 (output_au8, Password_pu8, PasswordLen_u32, UpdatePinSalt_u8, UPDATE_PIN_SALT_SIZE);
#else
    CI_LocalPrintf ("*** WARNING low security for password pin ***\r\n");
    memset (output_au8, 0, AES_KEYSIZE_256_BIT);
    memcpy (output_au8, Password_pu8, PasswordLen_u32)
        // use the base key as the key
#endif
    ReadUpdatePinHashFromFlash (UpdatePinHash_u8);

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("Hashed PIN   : ");
    HexPrint (AES_KEYSIZE_256_BIT, UpdatePinHash_u8);
    CI_LocalPrintf ("\r\n");
#endif

    if (0 != memcmp (UpdatePinHash_u8, output_au8, AES_KEYSIZE_256_BIT))
    {
#ifdef LOCAL_DEBUG
        CI_LocalPrintf ("Hashed PIN PW: ");
        HexPrint (AES_KEYSIZE_256_BIT, output_au8);
        CI_LocalPrintf ("\r\n");
        CI_LocalPrintf ("Wrong PIN\r\n");
#endif
        return (FALSE);
    }

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("PIN OK\r\n");
#endif
    DelayMs (100);
    return (TRUE);
}

/*******************************************************************************

  ShowUpdatePinUserPageData

  Changes
  Date      Reviewer        Info
  15.07.16  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void ShowUpdatePinUserPageData (void)
{
#ifdef LOCAL_DEBUG
    u8 UpdatePinSalt_u8[UPDATE_PIN_SALT_SIZE];
    u8 UpdatePinHash_u8[AES_KEYSIZE_256_BIT];

    ReadUpdatePinSaltFromFlash (UpdatePinSalt_u8);

    CI_LocalPrintf ("ShowUpdatePin - UserPageData\r\n");
    CI_LocalPrintf ("Salt         : ");
    HexPrint (UPDATE_PIN_SALT_SIZE, UpdatePinSalt_u8);
    CI_LocalPrintf ("\r\n");

    ReadUpdatePinHashFromFlash (UpdatePinHash_u8);
    CI_LocalPrintf ("Hashed PIN   : ");
    HexPrint (AES_KEYSIZE_256_BIT, UpdatePinHash_u8);
    CI_LocalPrintf ("\r\n");
#endif
}


/*******************************************************************************

  StoreNewUpdatePinHashInFlash

  Changes
  Date      Reviewer        Info
  20.06.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 StoreNewUpdatePinHashInFlash (u8 * Password_pu8, u32 PasswordLen_u32)
{
  u8 output_au8[64];
  u8 UpdatePinSalt_u8[UPDATE_PIN_SALT_SIZE];

    if (UPDATE_PIN_MAX_SIZE < PasswordLen_u32)
    {
        return (FALSE);
    }

    // Generate new salt
    GetRandomNumber_u32 (UPDATE_PIN_SALT_SIZE,UpdatePinSalt_u8);

    WriteUpdatePinSaltToFlash (UpdatePinSalt_u8);

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("StoreNewUpdatePinHashInFlash\r\n");
    CI_LocalPrintf ("Password_pu8 : Len %2d -%s-\r\n", PasswordLen_u32, Password_pu8);
    CI_LocalPrintf ("Salt         : ");
    HexPrint (UPDATE_PIN_SALT_SIZE, UpdatePinSalt_u8);
    CI_LocalPrintf ("\r\n");
#endif

#ifndef SAVE_FLASH_MEMORY_NO_PBKDF2
    pbkdf2 (output_au8, Password_pu8, PasswordLen_u32, UpdatePinSalt_u8, UPDATE_PIN_SALT_SIZE);
#else
    CI_LocalPrintf ("*** WARNING low security for password pin ***\r\n");
    // use the base key as the key
    memset (output_au8, 0, AES_KEYSIZE_256_BIT);
    memcpy (output_au8, Password_pu8, PasswordLen_u32)
#endif
    WriteUpdatePinHashToFlash (output_au8);

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("Hashed PIN   : ");
    HexPrint (AES_KEYSIZE_256_BIT, output_au8);
    CI_LocalPrintf ("\r\n");
#endif

    return (TRUE);
}

/*******************************************************************************

  InitUpdatePinHashInFlash

  Changes
  Date      Reviewer        Info
  21.06.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void InitUpdatePinHashInFlash (void)
{
    StoreNewUpdatePinHashInFlash ((u8 *) "12345678", 8);
}

/*******************************************************************************

  TestUpdatePin

  Changes
  Date      Reviewer        Info
  21.06.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void TestUpdatePin (void)
{
    CheckUpdatePin ((u8 *) "12345678", 8);
    CheckUpdatePin ((u8 *) "1234", 4);
}
