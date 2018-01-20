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
 * html_io.h
 *
 *  Created on: 03.04.2012
 *      Author: RB
 */

#ifndef HTML_IO_H_
#define HTML_IO_H_


// Userinterface commands
#define HTML_CMD_NOTHING                            0
#define HTML_CMD_ENABLE_AES_LUN                     1
#define HTML_CMD_DISABLE_AES_LUN                    2
#define HTML_CMD_ENABLE_FIRMWARE_UPDATE             3
#define HTML_CMD_FILL_SD_WITH_RANDOM_CHARS          4
// #define HTML_CMD_WRITE_STATUS_DATA 5 // delete
#define HTML_CMD_GENERATE_NEW_KEYS                  6
#define HTML_CMD_ENABLE_READONLY_UNCRYPTED_LUN      7
#define HTML_CMD_ENABLE_READWRITE_UNCRYPTED_LUN     8

#define HTML_CMD_ENABLE_HIDDEN_AES_LUN              10
#define HTML_CMD_DISABLE_HIDDEN_AES_LUN             11

// #define HTML_CMD_INIT_HIDDEN_VOLUME_SLOT 12

#define HTML_CMD_WRITE_STATUS_DATA                  13
#define HTML_CMD_GET_DEVICE_STATUS                  14
#define HTML_CMD_CLEAR_NEW_SD_CARD_FOUND            15

#define HTML_CMD_SEND_STARTUP                       16
#define HTML_CMD_SEND_PASSWORD_RETRY_COUNT          17

#define HTML_CMD_SAVE_HIDDEN_VOLUME_SLOT            18
#define HTML_CMD_READ_HIDDEN_VOLUME_SLOT            19

#define HTML_CMD_EXPORT_BINARY                      20

#define HTML_CMD_SEND_PASSWORD_MATRIX                21
#define HTML_CMD_SEND_PASSWORD_MATRIX_PINDATA        22
#define HTML_CMD_SEND_PASSWORD_MATRIX_SETUP          23

#define HTML_SEND_HIDDEN_VOLUME_SETUP                24 // Old version
#define HTML_CMD_SEND_PASSWORD                       25
#define HTML_CMD_SEND_NEW_PASSWORD                   26

#define HTML_CMD_HIDDEN_VOLUME_SETUP                 27
#define HTML_CMD_CLEAR_STICK_KEYS_NOT_INITIATED      28
#define HTML_CMD_CLEAR_LOCK_STICK_HARDWARE           29
#define HTML_CMD_PRODUCTION_TEST                     30

#define HTML_CMD_CHANGE_UPDATE_PIN                   31
#define HTML_CMD_PRODUCTION_TEST_WITH_WRITE_TEST     32

#define HTML_CMD_ENABLE_ADMIN_READONLY_UNCRYPTED_LUN      33
#define HTML_CMD_ENABLE_ADMIN_READWRITE_UNCRYPTED_LUN     34

#define HTML_CMD_ENABLE_ADMIN_READONLY_ENCRYPTED_LUN      35
#define HTML_CMD_ENABLE_ADMIN_READWRITE_ENCRYPTED_LUN     36

#define HTML_CMD_CHECK_SMARTCARD_USAGE                    37

/* Look for 4 byte alignment of 32 bit values */
typedef struct
{
    u8  FirmwareVersion_au8[2];  // 2 byte // 2
    u8  FirmwareVersionInternal_u8;      // 1 byte // 3
    u8  SD_Card_Size_u8;         // 1 byte // 4  in GB
    u32 CPU_CardID_u32;          // 4 byte // 8
    u32 SmartCardID_u32;         // 4 byte // 12
    u32 SD_CardID_u32;           // 4 byte // 16
    u8  SC_UserPwRetryCount;     // User password retry count 1 byte // 17
    u8  SC_AdminPwRetryCount;    // Admin password retry count 1 byte // 18
    u8  SD_Card_ManufacturingYear_u8;    // 1 byte // 19
    u8  SD_Card_ManufacturingMonth_u8;   // 1 byte // 20
    u16 SD_Card_OEM_u16;         // 2 byte // 22
    u16 SD_WriteSpeed_u16;       // in kbyte / sec 2 byte // 24
    u8  SD_Card_Manufacturer_u8; // 1 byte // 25
} typeStick20ProductionInfos_st; // Sum 25 byte (Max 25 Byte) // not packed

extern typeStick20ProductionInfos_st Stick20ProductionInfos_st;

void GetProductionInfos (typeStick20ProductionInfos_st * Infos_st,u8 WriteTestEnabled);
void LockDevice (void);

u8 HTML_FileIO_Init_u8 (void);

void HID_ExcuteCmd (void);

#endif /* HTML_IO_H_ */
