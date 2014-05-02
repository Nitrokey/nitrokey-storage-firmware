/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-07-12
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

typeStick20Configuration_st StickConfiguration_st;

/*******************************************************************************

 Local defines

*******************************************************************************/

/*

  Userpage layout

   Byte
   0 -  31    AES Storage key
  32 -  51    Matrix columns for user password
  52 -  71    Matrix columns for admin password
  72 - 101    Stick Configuration
  92 - 123    Base for AES key hidden volume (32 byte)
 124 - 128    ID of sd card (4 byte)

*/

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

/*******************************************************************************

  WriteAESStorageKeyToUserPage

  Reviews
  Date      Reviewer        Info
  03.02.14  RB              First review

*******************************************************************************/

u8 WriteAESStorageKeyToUserPage (u8 *data)
{
  flashc_memcpy(AVR32_FLASHC_USER_PAGE,data,32,TRUE);
  return (TRUE);
}

/*******************************************************************************

  ReadAESStorageKeyToUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 ReadAESStorageKeyToUserPage (u8 *data)
{
  memcpy (data,(void*)(AVR32_FLASHC_USER_PAGE),32);
  return (TRUE);
}


/*******************************************************************************

  WriteMatrixColumsUserPWToUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 WriteMatrixColumsUserPWToUserPage (u8 *data)
{
  flashc_memcpy(AVR32_FLASHC_USER_PAGE + 32,data,20,TRUE);
  return (TRUE);
}

/*******************************************************************************

  ReadMatrixColumsUserPWFromUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 ReadMatrixColumsUserPWFromUserPage (u8 *data)
{
  memcpy (data,(void*)(AVR32_FLASHC_USER_PAGE + 32),20);
  return (TRUE);
}

/*******************************************************************************

  WriteMatrixColumsAdminPWFromUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 WriteMatrixColumsAdminPWFromUserPage (u8 *data)
{
  flashc_memcpy(AVR32_FLASHC_USER_PAGE + 52,data,20,TRUE);
  return (TRUE);
}

/*******************************************************************************

  ReadMatrixColumsAdminPWFromUserPage

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 ReadMatrixColumsAdminPWFromUserPage (u8 *data)
{
  memcpy (data,(void*)(AVR32_FLASHC_USER_PAGE + 52),20);
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
// Set actual firmware version
  StickConfiguration_st.VersionInfo_au8[0]              = VERSION_MAJOR;
  StickConfiguration_st.VersionInfo_au8[1]              = VERSION_MINOR;
  StickConfiguration_st.VersionInfo_au8[2]              = 0;               // Build number not used
  StickConfiguration_st.VersionInfo_au8[3]              = 0;               // Build number not used

  flashc_memcpy(AVR32_FLASHC_USER_PAGE + 72,&StickConfiguration_st,30,TRUE);
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
  memcpy (&StickConfiguration_st,(void*)(AVR32_FLASHC_USER_PAGE + 72),sizeof (typeStick20Configuration_st)-8); // Not the retry counter and sc serial no

  if (MAGIC_NUMBER_STICK20_CONFIG != StickConfiguration_st.MagicNumber_StickConfig_u16)
  {
    return (FALSE);
  }
  return (TRUE);
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
  StickConfiguration_st.MagicNumber_StickConfig_u16     = MAGIC_NUMBER_STICK20_CONFIG;
  StickConfiguration_st.ReadWriteFlagUncryptedVolume_u8 = READ_WRITE_ACTIVE;
  StickConfiguration_st.ReadWriteFlagCryptedVolume_u8   = READ_WRITE_ACTIVE;
  StickConfiguration_st.ReadWriteFlagHiddenVolume_u8    = READ_WRITE_ACTIVE;
  StickConfiguration_st.StoredMatrixLength_u8           = 0;
  StickConfiguration_st.ActiveSD_CardID_u32             = 0;               // todo: check endian
  StickConfiguration_st.VersionInfo_au8[0]              = VERSION_MAJOR;
  StickConfiguration_st.VersionInfo_au8[1]              = VERSION_MINOR;
  StickConfiguration_st.VersionInfo_au8[2]              = 0;               // Build number not used
  StickConfiguration_st.VersionInfo_au8[3]              = 0;               // Build number not used
  StickConfiguration_st.NewSDCardFound_u8               = 0;
  StickConfiguration_st.SDFillWithRandomChars_u8        = FALSE;
  StickConfiguration_st.VolumeActiceFlag_u8             = 0;
  StickConfiguration_st.NewSmartCardFound_u8            = 0;
  StickConfiguration_st.ActiveSmartCardID_u32           = 0;


  WriteStickConfigurationToUserPage ();
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

u8 WriteHiddenVolumeSlotKey (u8 *data)
{
  flashc_memcpy((void*)(AVR32_FLASHC_USER_PAGE + 102),data,32,TRUE);
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

u8 ReadHiddenVolumeSlotsKey (u8 *data)
{
  memcpy (data,(void*)(AVR32_FLASHC_USER_PAGE + 102),32);
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

void SendStickStatusToHID (typeStick20Configuration_st *Status_st)
{
  // If configuration not found then init it
  if (FALSE == ReadStickConfigurationFromUserPage ())
  {
    InitStickConfigurationToUserPage_u8 ();
  }

  memcpy (Status_st,&StickConfiguration_st,sizeof (typeStick20Configuration_st)); // Not the retry counter and sc serial no

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

  ReadSdId (&Status_st->ActiveSD_CardID_u32);
}

/*******************************************************************************

  GetStickStatusFromHID

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void GetStickStatusFromHID (typeStick20Configuration_st *Status_st)
{
/*
  HID_Stick20AccessStatus_st->MatrixPasswordUserActiv_u8  = FALSE;
  HID_Stick20AccessStatus_st->MatrixPasswordAdminActiv_u8 = FALSE;
  HID_Stick20AccessStatus_st->ActivPasswordStatus_u8      = FALSE;
  HID_Stick20AccessStatus_st->VolumeStatus_u8             = FALSE;

  sd_mmc_mci_read_capacity_0(&HID_Stick20AccessStatus_st->SD_BlockSize_u32);
*/
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
  // If configuration not found then init it
  if (FALSE == ReadStickConfigurationFromUserPage ())
  {
    InitStickConfigurationToUserPage_u8 ();
  }
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
  // If configuration not found then init it
  if (FALSE == ReadStickConfigurationFromUserPage ())
  {
    InitStickConfigurationToUserPage_u8 ();
  }

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

u8 WriteSdId (u32 *SdId_u32)
{
  flashc_memcpy(AVR32_FLASHC_USER_PAGE + 134,SdId_u32,4,TRUE);

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

u8 ReadSdId (u32 *SdId_u32)
{
  memcpy (SdId_u32,(void*)(AVR32_FLASHC_USER_PAGE + 134),4);

  StickConfiguration_st.ActiveSD_CardID_u32 = *SdId_u32;
  return (TRUE);
}

/*******************************************************************************

  WriteNewSdCardFoundToFlash

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of save new SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 WriteNewSdCardFoundToFlash (u32 *SdId_u32)
{
  // If configuration not found then init it
  if (FALSE == ReadStickConfigurationFromUserPage ())
  {
    InitStickConfigurationToUserPage_u8 ();
  }

  CI_LocalPrintf ("*** New SD card found ***  Serial number 0x%08x\r\n",SdId_u32);

  WriteSdId (SdId_u32);

  StickConfiguration_st.NewSDCardFound_u8 |= 0x01;            // Bit 0 = new card found
  StickConfiguration_st.NewSDCardFound_u8 += 2;               // add change counter +1

  StickConfiguration_st.SDFillWithRandomChars_u8 &= 0xFE;     // Clear the "card with random chars filled"  bit

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
  // If configuration not found then init it
  if (FALSE == ReadStickConfigurationFromUserPage ())
  {
    InitStickConfigurationToUserPage_u8 ();
  }

  CI_LocalPrintf ("SD is filled with random chars\r\n");

  StickConfiguration_st.SDFillWithRandomChars_u8 |= 0x01;     // Set the "SD card filled with randoms"  bit
  StickConfiguration_st.SDFillWithRandomChars_u8 += 2;        // add counter +1

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
  // If configuration not found then init it
  if (FALSE == ReadStickConfigurationFromUserPage ())
  {
    InitStickConfigurationToUserPage_u8 ();
  }

  CI_LocalPrintf ("Clear new SD card found\r\n");

  StickConfiguration_st.NewSDCardFound_u8 &= 0xFE;     // Clear the "new SD card found"  bit

  WriteStickConfigurationToUserPage ();

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
  flashc_memcpy(AVR32_FLASHC_USER_PAGE + 138,&Datetime_u32,4,TRUE);
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

u8 ReadDatetime (u32 *Datetime_u32)
{
  memcpy (Datetime_u32,(void*)(AVR32_FLASHC_USER_PAGE + 138),4);
  return (TRUE);
}
