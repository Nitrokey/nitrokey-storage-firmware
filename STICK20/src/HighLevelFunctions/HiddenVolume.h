/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-07-14
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
 * HiddenVolume.h
 *
 *  Created on: 14.07.2013
 *      Author: RB
 */

#ifndef HIDDENVOLUME_H_
#define HIDDENVOLUME_H_

u8 *BuildAESKeyFromUserpassword_u8 (u8 *Userpassword_pu8,u8 *AESKey_au8);
u8 InitRamdomBaseForHiddenKey_u8 (void);

/* Storage description */

#define FLASH_PAGE_SIZE       512                   // AVR
#define FLASH_START           0x80000000            // AVR

#define HV_FLASH_START_PAGE   498
#define HV_SLOT_START_ADDRESS (FLASH_START + HV_FLASH_START_PAGE * FLASH_PAGE_SIZE) // 0x8003e400
#define HV_SLOT_SIZE          64                    // Byte
#define HV_SLOT_COUNT         8                     // 8 Slots a 64 byte = 512 byte


#define HV_MAGIC_NUMBER_SLOT_ENTRY        0x26f29c02

typedef struct {
  u32 MagicNumber_u32;          //
  u32 StartBlock_u32;           //
  u32 EndBlock_u32;             //
  u8  AesKey_au8[32];           //
  u32 Crc_u32;                  // This had to be the last entry in struct
} HiddenVolumeKeySlot_tst;      // 48 byte = 4+4+4+32+4 - must multiple from 4 for CRC32

/* Interface description */

#define STICK_20_HV_COMAND_GET_SLOT_DATA       0
#define STICK_20_HV_COMAND_INIT_SLOT           1
#define STICK_20_HV_COMAND_SEND_SLOT_DATA      2

#define STICK_20_HV_STATUS_ERROR               0
#define STICK_20_HV_STATUS_OK                  1
#define STICK_20_HV_SLOT_NOT_USED              2

#define STICK_20_HV_COMAND_DATA_LENGTH        30

typedef struct {
  u8  Comand_u8;                                  //
  u8  Status_u8;                                  //
  u8  Slot_u8;                                    //
  u8  dummy_u8;                                   // for 4 byte alignment
  u32 StartBlock_u32;                             //
  u32 EndBlock_u32;                               //
  u8  Data_au8[STICK_20_HV_COMAND_DATA_LENGTH];   //
} HiddenVolumeTransferData_tst;                   // 42 byte = 1+1+1+1+4+4+30



void IBN_HV_Tests (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8);


#endif /* HIDDENVOLUME_H_ */
