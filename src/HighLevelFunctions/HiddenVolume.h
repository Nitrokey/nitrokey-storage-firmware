/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-07-14
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
 * HiddenVolume.h
 *
 *  Created on: 14.07.2013
 *      Author: RB
 */

#ifndef HIDDENVOLUME_H_
#define HIDDENVOLUME_H_


/* Storage description */

#define FLASH_PAGE_SIZE       512   // AVR
#define FLASH_START           0x80000000    // AVR



#define USER_DATA_START         (497-2) // add 2 pages of spare space
#define HV_FLASH_START_PAGE     497
#define HV_MAGIC_NUMBER_SIZE    4
#define HV_SALT_SIZE            32
#define HV_MAGIC_NUMBER_ADDRESS (FLASH_START             + HV_FLASH_START_PAGE * FLASH_PAGE_SIZE)   // 0x8003e400
#define HV_SALT_START_ADDRESS   (HV_MAGIC_NUMBER_ADDRESS + HV_MAGIC_NUMBER_SIZE)    // 0x8003e404
#define HV_SLOT_START_ADDRESS   (HV_SALT_START_ADDRESS   + HV_SALT_SIZE)    // 0x8003e424
#define HV_SLOT_SIZE            64  // Byte
#define HV_SLOT_COUNT           4   // 4 Slots a 128 byte = 512 byte

#define HV_MAGIC_NUMBER_INIT              0x42bc8f13
// #define HV_MAGIC_NUMBER_SLOT_ENTRY 0x26f29c02 // Removed, because it is fix number, it could make it easier to attack the encrypted slot


#define HIDDEN_VOLUME_OUTPUT_STATUS_FAIL                      0
#define HIDDEN_VOLUME_OUTPUT_STATUS_OK                        1
#define HIDDEN_VOLUME_OUTPUT_STATUS_WRONG_PASSWORD            2
#define HIDDEN_VOLUME_OUTPUT_STATUS_NO_USER_PASSWORD_UNLOCK   3
#define HIDDEN_VOLUME_OUTPUT_STATUS_SMARTCARD_ERROR           4


typedef struct
{
    // u32 MagicNumber_u32; // Entry removed, because it is fix number, it could make it easier to attack the encrypted slot
    u8 AesKey_au8[32];          // Move random chars in front, to harden struct against attacks
    u32 StartBlock_u32;         //
    u32 EndBlock_u32;           //
    u32 Crc_u32;                // This had to be the last entry in struct
} HiddenVolumeKeySlot_tst;      // 44 byte = 4+4+32+4 - must multiple from 4 for CRC32


/* GUI interface description */

#define STICK_20_HV_COMAND_DATA_LENGTH        30
#define MAX_HIDDEN_VOLUME_PASSOWORD_SIZE      20

typedef struct
{
    u8 SlotNr_u8;
    u8 StartBlockPercent_u8;
    u8 EndBlockPercent_u8;
    u8 HiddenVolumePassword_au8[MAX_HIDDEN_VOLUME_PASSOWORD_SIZE + 1];
} HiddenVolumeSetup_tst;


u8 SetupUpHiddenVolumeSlot (HiddenVolumeSetup_tst * HV_Setup_st);

u8 InitRamdomBaseForHiddenKey (void);
u8 DecryptedHiddenVolumeSlotsData (void);
u8 GetHiddenVolumeSlotKey (u8 * HiddenVolumeKey_pu8, u8 * Password_pu8, u32 PasswordLen_u32, u8 * Salt_pu8, u32 SaltLen_u32);
u8 SetupUpHiddenVolumeSlot (HiddenVolumeSetup_tst * HV_Setup_st);
u8 GetHiddenVolumeKeyFromUserpassword (u8 * Userpassword_pu8, u8 * DecryptedHiddenVolumeKey_au8);
u8 GetDecryptedHiddenVolumeSlotsKey (u8 ** Key_pa8);
void IBN_HV_Tests (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8);
u8 HV_CheckHiddenVolumeSlotKey_u8 (void);
u8 InitHiddenSlots (void);

#endif /* HIDDENVOLUME_H_ */
