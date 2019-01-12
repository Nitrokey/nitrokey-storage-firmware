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

#ifndef HOTP_H
#define HOTP_H

typedef enum
{
    FLASH_BUSY = 1,
    FLASH_ERROR_PG,
    FLASH_ERROR_WRP,
    FLASH_COMPLETE,
    FLASH_TIMEOUT
} FLASH_Status;

#define NUMBER_OF_HOTP_SLOTS 3
#define NUMBER_OF_TOTP_SLOTS 15

#define SLOT_CONFIG_DIGITS 0
#define SLOT_CONFIG_ENTER 1
#define SLOT_CONFIG_TOKENID 2
#define SLOT_CONFIG_PASSWORD_USED   3
#define SLOT_CONFIG_PASSWORD_TIMED  4

/*
   global config slot:

   1b slot sent after numlock 1b slot sent after caps lock 1b slot sent after scroll lock

 */

#define FLASH_PAGE_SIZE 512 // AVR

#define SLOT_PAGE_SIZE  500 // less than actual page, so we can copy it to backup page with additional info

/* OTP BLOCK LAYOUT:
Page 500 - 502  : 0x800_3E800 : OTP Slots data, contains the handling structs for each OTP slot
Page 503 - 505  : 0x800_3EE00 : HOTP Slot counters, contains one offset counter per page for each of the HOTP slots
Page 506 - 508  : 0x800_3F400 : Backup pages, used for temporary backup of data to flash memory
Page 509        : 0x800_3FA00 : TOTP time, contains the current system time for TOTP generation 
TODO: Not sure if that's actually the case. Why do we even store the system time in flash?
*/
#define FLASH_START            0x80000000
#define OTP_FLASH_START_PAGE   500
#define SLOTS_ADDRESS         (FLASH_START + OTP_FLASH_START_PAGE * FLASH_PAGE_SIZE + (FLASH_PAGE_SIZE*0))   // 0x8003e800
#define SLOT1_COUNTER_ADDRESS (FLASH_START + OTP_FLASH_START_PAGE * FLASH_PAGE_SIZE + (FLASH_PAGE_SIZE*3))   // 0x8003
#define SLOT2_COUNTER_ADDRESS (FLASH_START + OTP_FLASH_START_PAGE * FLASH_PAGE_SIZE + (FLASH_PAGE_SIZE*4))   // 0x8003
#define SLOT3_COUNTER_ADDRESS (FLASH_START + OTP_FLASH_START_PAGE * FLASH_PAGE_SIZE + (FLASH_PAGE_SIZE*5))   // 0x8003
#define BACKUP_PAGE_ADDRESS   (FLASH_START + OTP_FLASH_START_PAGE * FLASH_PAGE_SIZE + (FLASH_PAGE_SIZE*6))   // (3 Pages) 0x8003
#define TIME_ADDRESS          (FLASH_START + OTP_FLASH_START_PAGE * FLASH_PAGE_SIZE + (FLASH_PAGE_SIZE*9))   //

/* Backup page layout:
0x800_3F400 - 0x800_3F7F4 : 1524 Bytes : Backup Memory
0x800_3F7F4 - 0x800_3F9FA :    4 Bytes : The address, which was backed up
0x800_3F9FA - 0x800_3F9FC :    2 Bytes : Length of the backup
0x800_3F9FC - 0x800_3FA00 :    2 Bytes : Backup OK token, 0x4F4B if successful
*/
#define BACKUP_SIZE           (FLASH_PAGE_SIZE * 3)
#define BACKUP_ADDRESS_OFFSET (BACKUP_SIZE - 12)  // 500 - no flash block addr
#define BACKUP_LENGTH_OFFSET  (BACKUP_SIZE -  8)  // 504 - no flash block addr
#define BACKUP_OK_OFFSET      (BACKUP_SIZE -  6)  // 506 - no flash block addr

#define GLOBAL_CONFIG_OFFSET  0
#define SECRET_LENGTH_DEFINE  20
// TODO: Increase to 40 when everything else works

#define __packed __attribute__((__packed__))
typedef struct {
    u8 type; //'H' - HOTP, 'T' - TOTP, 0xFF - not programmed
    u8 slot_number;
    u8 name[15];
    u8 secret[SECRET_LENGTH_DEFINE];
    u8 config;
    u8 token_id[13];
    u8 interval[2];
} __packed OTP_slot;

extern u32 hotp_slot_counters[NUMBER_OF_HOTP_SLOTS];

#define TIME_OFFSET          4

// How many tokens are stored on one counter page?
#define TOKEN_PER_FLASHPAGE        (FLASH_PAGE_SIZE-8)

extern u8 page_buffer[FLASH_PAGE_SIZE * 3];

extern u64 current_time;

u16 getu16 (u8 * array);
u32 getu32 (u8 * array);
u64 getu64 (u8 * array);

u64 endian_swap (u64 x);

u32 dynamic_truncate (u8 * hmac_result);
void erase_counter (u8 slot);

void write_data_to_flash (u8 * data, u16 len, u32 addr);
u32 get_hotp_value (u64 counter, u8 * secret, u8 secret_length, u8 len);
u64 get_counter_value (u32 addr);
u32 get_flash_time_value (void);
u32 get_time_value (void);
u8 set_time_value (u32 time);
u8 set_counter_value (u32 addr, u64 counter);
u32 get_code_from_hotp_slot (u8 slot);
u8 increment_counter_page (u32 addr);
void write_to_slot (u8 * data, u8 * addr);
void write_to_config (u8 * data, u8 len);
void backup_data (u8 * data, u16 len, u32 addr);
u8 check_backups (void);
u8 get_hotp_slot_config (u8 slot_number);
u8 get_totp_slot_config (u8 slot_number);
u32 get_code_from_totp_slot (u8 slot, u64 challenge);

void send_hotp_slot(u8 slot_no);
u8* get_totp_slot_addr (u8 slot_number);
u8* get_hotp_slot_addr (u8 slot_number);
u32 get_slot_offset(u8 slot_number);

#endif // HOTP_H
