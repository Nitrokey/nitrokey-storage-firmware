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
 * FlashStorage.h
 *
 *  Created on: 12.07.2013
 *      Author: RB
 */

#ifndef FLASHSTORAGE_H_
#define FLASHSTORAGE_H_

u8 WriteAESStorageKeyToUserPage (u8 *data);
u8 ReadAESStorageKeyToUserPage (u8 *data);
u8 WriteMatrixColumsUserPWToUserPage (u8 *data);
u8 ReadMatrixColumsUserPWFromUserPage (u8 *data);
u8 WriteMatrixColumsAdminPWFromUserPage (u8 *data);
u8 ReadMatrixColumsAdminPWFromUserPage (u8 *data);
u8 WriteStickConfigurationToUserPage (void);
u8 ReadStickConfigurationFromUserPage (void);
u8 InitStickConfigurationToUserPage_u8 (void);

u8 WriteBaseForHiddenVolumeKey (u8 *data);
u8 ReadBaseForHiddenVolumeKey (u8 *data);

u8 ReadMatrixColumsUserPWFromUserPage (u8 *data);


u8 Read_ReadWriteStatusUncryptedVolume_u8 (void);
u8 Write_ReadWriteStatusUncryptedVolume_u8 (u8 NewStatus_u8);
u8 WriteDatetime (u32 Datetime_u32);
u8 ReadDatetime (u32 *Datetime_u32);


/***************************************************************************************

  Structure is also send to GUI

***************************************************************************************/

#define MAGIC_NUMBER_STICK20_CONFIG     0x1809

#define READ_WRITE_ACTIVE             0
#define READ_ONLY_ACTIVE              1

#define SD_UNCRYPTED_VOLUME_BIT_PLACE   0
#define SD_CRYPTED_VOLUME_BIT_PLACE     1
#define SD_HIDDEN_VOLUME_BIT_PLACE      2


typedef struct {
  u16 MagicNumber_StickConfig_u16;          // Shows that the structure is valid                  2 byte
  u8  ReadWriteFlagUncryptedVolume_u8;      // Flag stores the read/write flag in the CPU flash   1 byte
  u8  ReadWriteFlagCryptedVolume_u8;        // Flag stores the read/write flag in the CPU flash   1 byte
  u8  ReadWriteFlagHiddenVolume_u8;         // Flag stores the read/write flag in the CPU flash   1 byte
  u8  StoredMatrixLength_u8;                // Not used                                           1 byte
  u32 ActiveSD_CardID_u32;                  // Not used                                           4 byte  // 10
  u8  VersionInfo_au8[4];                   //                                                    4 byte
  u8  NewSDCardFound_u8;                    // Bit 0 new card found, bit 1-7 change counter       1 byte
  u8  SDFillWithRandomChars_u8;             // Bit 0 = 1 = filled, bit 1-7 change counter         1 byte
  u8  VolumeActiceFlag_u8;                  //                                                    1 byte
} typeStick20Configuration_st;                                                          // Sum   17 byte

u8 ReadStickConfigurationFromUserPage (void);
void SendStickStatusToHID (typeStick20Configuration_st *Status_st);

u8 WriteSdId (u32 *SdId_u32);
u8 ReadSdId (u32 *SdId_u32);

u8 WriteNewSdCardFoundToFlash (u32 *SdId_u32);
u8 SetSdCardFilledWithRandomsToFlash (void);
u8 ClearNewSdCardFoundToFlash (void);

#endif /* FLASHSTORAGE_H_ */
