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

typedef enum
{
  FLASH_BUSY = 1,
  FLASH_ERROR_PG,
  FLASH_ERROR_WRP,
  FLASH_COMPLETE,
  FLASH_TIMEOUT
}FLASH_Status;


#define NUMBER_OF_HOTP_SLOTS 2
#define NUMBER_OF_TOTP_SLOTS 4

//Flash memory pages:
//0x801E800
//0x801EC00 <- slots
//0x801F000 <- slot 1 counter
//0x801F400 <- slot 2 counter
//0x801F800
//0x801FC00 <- backup page


/*
slot structure:
1b	0x01 if slot is used (programmed)
15b slot name
20b secret
1b configuration flags:
	MSB [x|x|x|x|x|send token id|send enter after code?|no. of digits 6/8] LSB
12b token id
1b keyboard layout

*/

#define SLOT_CONFIG_DIGITS 0
#define SLOT_CONFIG_ENTER 1
#define SLOT_CONFIG_TOKENID 2

/*
global config slot:

1b slot sent after numlock 
1b slot sent after caps lock
1b slot sent after scroll lock

*/

#define FLASH_PAGE_SIZE 1024 // RB

#define SLOT_PAGE_SIZE 1000 //less than actual page, so we can copy it to backup page with additional info

#define SLOTS_ADDRESS         (0x80000000 + 245 * FLASH_PAGE_SIZE +    0) // 0x803EC00
#define SLOT1_COUNTER_ADDRESS (0x80000000 + 245 * FLASH_PAGE_SIZE + 1024) // 0x803F000
#define SLOT2_COUNTER_ADDRESS (0x80000000 + 245 * FLASH_PAGE_SIZE + 2048) // 0x803F400
#define BACKUP_PAGE_ADDRESS   (0x80000000 + 245 * FLASH_PAGE_SIZE + 4096) // 0x803FC00

#define BACKUP_ADDRESS_OFFSET 1000
#define BACKUP_LENGTH_OFFSET  1004
#define BACKUP_OK_OFFSET      1006

#define GLOBAL_CONFIG_OFFSET 0

#define HOTP_SLOT1_OFFSET 64
#define HOTP_SLOT2_OFFSET 128

#define TOTP_SLOT1_OFFSET 192
#define TOTP_SLOT2_OFFSET 256
#define TOTP_SLOT3_OFFSET 320
#define TOTP_SLOT4_OFFSET 384

#define SLOT_TYPE_OFFSET 0
#define SLOT_NAME_OFFSET 1
#define SECRET_OFFSET 16
#define CONFIG_OFFSET 36
#define TOKEN_ID_OFFSET 37




extern u32 hotp_slots[NUMBER_OF_HOTP_SLOTS];
extern u32 totp_slots[NUMBER_OF_TOTP_SLOTS];
extern u32 hotp_slot_counters[NUMBER_OF_HOTP_SLOTS];
extern u32 hotp_slot_offsets[NUMBER_OF_HOTP_SLOTS];
extern u32 totp_slot_offsets[NUMBER_OF_TOTP_SLOTS];

extern u8 page_buffer[SLOT_PAGE_SIZE];

u32 getu32(u8 *array);
u64 getu64(u8 *array);

u64 endian_swap (u64 x);

u32 dynamic_truncate (u8 * hmac_result);

void write_data_to_flash(u8 *data,u16 len,u32 addr);
u32 get_hotp_value(u64 counter,u8 * secret,u8 secret_length,u8 len);
u64 get_counter_value(u32 addr);
u8 set_counter_value(u32 addr, u64 counter);
u32 get_code_from_hotp_slot(u8 slot);
u8 increment_counter_page(u32 addr);
void write_to_slot(u8 *data, u16 offset, u16 len);
void backup_data(u8 *data,u16 len, u32 addr);
u8 check_backups(void);
u8 get_hotp_slot_config(u8 slot_number);
u8 get_totp_slot_config(u8 slot_number);
u32 get_code_from_totp_slot(u8 slot, u64 challenge);


u8 *get_hotp_slot_addr(u8 slot_number);
u8 *get_totp_slot_addr (u8 slot_number);


