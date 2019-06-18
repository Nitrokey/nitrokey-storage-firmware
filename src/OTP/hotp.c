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


#include <avr32/io.h>
#include <stddef.h>
#include "compiler.h"
#include "flashc.h"
#include "string.h"
#include "time.h"

#include "global.h"
#include "tools.h"

#include "hotp.h"
// #include "memory_ops.h"
// #include "otp_sha1.h"
#include "hmac-sha1.h"
#include "LED_test.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

// #define DEBUG_HOTP
/*
 */

#ifdef DEBUG_HOTP
#else
#define CI_LocalPrintf(...)
#define CI_TickLocalPrintf(...)
#define CI_StringOut(...)
#define CI_Print8BitValue(...)
#define HexPrint(...)
#endif

// #define LITTLE_ENDIAN

#define BIG_ENDIAN  // AVR is BIG_ENDIAN

/*

   Flash storage description OTP

   Stick 2.x


   SLOT PAGE LAYOUT:
   Slot             Start   End     Description
   GLOBAL_CONFIG    0       2       Holds the configuration data for the auto-type functionality
   free             3       63      not in use
   HOTP 1-3         64      282     Configuration data for 3 HOTP slots
   TOTP 1-16        283     1450    Configuration data for 16 TOTP slots

   OTP SLOT LAYOUT:
   Name             Start   Length      Description
   type             0       1           Slot type Description: 'T' when programmed TOTP, 'H' when programmed TOTP, 0xFF when empty
   slot_number      1       1           Configured slot number
   name             2       15          Slot name as String
   secret           17      20          Slot secret
   config           37      1           Configuration bits for auto-type functionality
   token_id         38      13          OATH token Identifier
   interval         51      2           TOTP interval (unused for HOTP slots)

   OTP counter storage slot

   This field is used for storing the actual counter value. Because of the limitation of flash erase accesses (1000 for a Stick 1.4 flash page (1024
   byte), 100000 for a Stick 2.0 flash page (512 byte)). it is necessary to reduce the erase access of the flash. This is done by using a greater
   area of the flash. The limitation of flash accesses is only in the change of the bits to 1 in a flash page. Only all bit in a hole flash page can
   set to 1 in a single process (this is the erase access). The setting to 0 of a bit in the flash page is independent to other bits.

   The implementation: The first 8 byte of the slot contains the base counter of stored value as an unsigned 64 bit value. The remaining page stored
   a token per flash byte for a used number. When all tokens in a slot are used, the base counter is raised and the tokens are reseted to 0xff

   Flash page layout

   Entry Position Counter base 0 - 7 8 byte, unsigned 64 bit Use flags 8 - 1023 1016 byte marked a used value (for Stick 1.4) Use flags 8 - 511 504
   byte marked a used value (for Stick 2.0)

   Flash page byte order 0 1 2 3 4 End of flash page 01234567890123456789012345678901234567890123456789.... X
   VVVVVVVVFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF.... .

   V = 64 bit counter base value F = token for a used value, 0xFF = unused, 0x00 = used

   The actual counter is the sum of the counter base value and the number of tokens with a 0 in the slot.

   Example:

   Flash page byte order 0 1 2 3 4 End of flash page 01234567890123456789012345678901234567890123456789.... X xHi
   0000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF.... . xLo 0000001000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF.... .

   V = 0x0100 = 256 Token 0-7 are marked as used = 8 tokens are marked

   Counter value = 256 + 8 = 264


   Slot size 1024 byte Stick 1.4 Slot size 512 byte Stick 2.0


   OTP backup slot Backup of the OTP parameter block


 */

u32 hotp_slot_counters[NUMBER_OF_HOTP_SLOTS] = {
    SLOT1_COUNTER_ADDRESS,
    SLOT2_COUNTER_ADDRESS,
    SLOT3_COUNTER_ADDRESS,
    SLOT4_COUNTER_ADDRESS,
};

u8 page_buffer[FLASH_PAGE_SIZE * 3];

u64 current_time = 0x0;

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

#define KEYBOARD_FEATURE_COUNT 64

extern u8 HID_GetReport_Value[KEYBOARD_FEATURE_COUNT];

/*******************************************************************************

 Local declarations

*******************************************************************************/

/*******************************************************************************

  getu16

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u16 getu16 (u8 * array)
{
u16 result = array[0] + (array[1] << 8);
    return result;
}


/*******************************************************************************

  getu32

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 getu32 (u8 * array)
{
u32 result = 0;
s8 i = 0;


    for (i = 3; i >= 0; i--)
    {
        result <<= 8;
        result += array[i];
    }

    return result;
}


/*******************************************************************************

  getu64

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u64 getu64 (u8 * array)
{
u64 result = 0;
s8 i = 0;

    for (i = 7; i >= 0; i--)
    {
        result <<= 8;
        result += array[i];
    }

    return result;
}


/*******************************************************************************

  endian_swap

  only for little endian systems

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u64 endian_swap (u64 val)
{
    val =       ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val =       ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}



/*******************************************************************************

  dynamic_truncate

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 dynamic_truncate (u8 * hmac_result)
{
u8 offset = hmac_result[19] & 0xf;

u32 bin_code = (hmac_result[offset] & 0x7f) << 24
        | (hmac_result[offset + 1] & 0xff) << 16 | (hmac_result[offset + 2] & 0xff) << 8 | (hmac_result[offset + 3] & 0xff);

    return bin_code;
}

/*******************************************************************************

  crc_STM32

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit dde179d5d24bd3d06cab73848ba1ab7e5efda898
                              Time in firmware Test #74 Test #105

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

u32 crc_STM32 (u32 time)
{

int i;
u32 value = time << 8;
    // u32 crc;

    for (i = 0; i < 24; i++)
    {
        if (value & 0x80000000)
        {
            value = (value) ^ 0x98800000;   // Polynomial used in STM32
        }
        value = value << 1;
    }

    time = (time << 8) + (value >> 24);

    return time;
}

/*******************************************************************************

  write_data_to_flash

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void write_data_to_flash (u8 * data, u16 len, u32 addr)
{
    flashc_memcpy ((void *) addr, data, len, TRUE);
}


/*******************************************************************************

  get_hotp_value

  Get a HOTP/TOTP truncated value
  counter - HOTP/TOTP counter value
  secret - pointer to secret stored in memory
  secret_length - length of the secret
  len - length of the truncated result, 6 or 8

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 get_hotp_value (u64 counter, u8 * secret, u8 secret_length, u8 len)
{
u8 hmac_result[20];
u64 c = counter;

    LED_GreenOn ();


#ifdef DEBUG_HOTP
    {
u8 text[20];
        CI_LocalPrintf ("output len  :");
        itoa ((u32) len, text);
        CI_LocalPrintf ((s8 *) text);
        CI_LocalPrintf ("\n\r");

        CI_LocalPrintf ("secret len  :");
        itoa ((u32) secret_length, text);
        CI_LocalPrintf (text);
        CI_LocalPrintf ("\n\r");

        CI_LocalPrintf ("counter     :");
        itoa ((u32) counter, text);
        CI_LocalPrintf (text);
        CI_LocalPrintf (" - ");
        HexPrint (8, (u8 *) & counter);
        CI_LocalPrintf ("\n\r");

        CI_LocalPrintf ("secret      :");
        HexPrint (secret_length, secret);
        CI_LocalPrintf ("\n\r");

        CI_LocalPrintf ("c           :");
        HexPrint (8, (u8 *) & c);
        CI_LocalPrintf ("\n\r");
    }
#endif

    hmac_sha1 (hmac_result, secret, secret_length * 8, &c, 64);

#ifdef DEBUG_HOTP
    {
        CI_LocalPrintf ("hmac_result :");
        HexPrint (20, hmac_result);
        CI_LocalPrintf ("\n\r");
    }
#endif

u32 hotp_result = dynamic_truncate (hmac_result);

#ifdef DEBUG_HOTP
    {
u8 text[20];

        CI_LocalPrintf ("hotp_result 1:");
        HexPrint (4, (u8 *) & hotp_result);
        CI_LocalPrintf (" - ");
        itoa (hotp_result, text);
        CI_LocalPrintf (text);


        CI_LocalPrintf ("\n\r");
    }
#endif

    LED_GreenOff ();

    if (len == 6)
        hotp_result = hotp_result % 1000000;
    else if (len == 8)
        hotp_result = hotp_result % 100000000;
    else
        return 0;


#ifdef DEBUG_HOTP
    {
u8 text[20];
        CI_LocalPrintf ("hotp_result 2:");
        HexPrint (4, (u8 *) & hotp_result);
        CI_LocalPrintf (" - ");
        itoa (hotp_result, text);
        CI_LocalPrintf (text);
        CI_LocalPrintf ("\n\r");
    }
#endif

    return hotp_result;
}


/*******************************************************************************

  get_counter_value

  Get the HOTP counter stored in flash
  addr - counter page address

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u64 get_counter_value (u32 addr)
{
u16 i;
u64 counter;
u8* ptr;
u64* ptr_u64;


    ptr_u64 = (u64 *) addr;
    counter = *ptr_u64; // Set the counter base value


    ptr = (u8 *) addr;  // Start of counter storage page
    ptr += 8;   // Start of token area

    i = 0;

    while (i < TOKEN_PER_FLASHPAGE)
    {
        if (*ptr == 0xff)
        {
            break;  // A free token entry found
        }
        ptr++;
        counter++;
        i++;
    }

    return counter;
}


/*******************************************************************************

  get_flash_time_value

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit dde179d5d24bd3d06cab73848ba1ab7e5efda898
                              Time in firmware Test #74 Test #105

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 get_flash_time_value (void)
{
int i, flag = 0;
u32 time = 0;
u32* p;

    if (getu32 (TIME_ADDRESS) == 0xffffffff)
    {
        return 0xffffffff;
    }

    for (i = 1; i < 32; i++)
    {
        if (getu32 (TIME_ADDRESS + TIME_OFFSET * i) == 0xffffffff)
        {
            p = (TIME_ADDRESS + TIME_OFFSET * (i - 1));
            time = *p;
            flag = 1;
            break;
        }
    }

    if (0 == flag)
    {
        p = TIME_ADDRESS + TIME_OFFSET * 31;
        time = *p;
    }
    /*
       { u8 text[30];

       CI_StringOut ("get_time_value: time "); itoa (time,text); CI_StringOut ((char*)text);

       CI_StringOut (" = "); itoa (crc_STM32 (time>>8),text); CI_StringOut ((char*)text); CI_StringOut ("\r\n"); } */
    return (time);
}


/*******************************************************************************

  get_time_value

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit dde179d5d24bd3d06cab73848ba1ab7e5efda898
                              Time in firmware Test #74 Test #105

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 get_time_value (void)
{
u32 time = 0;

    time = get_flash_time_value ();

    if (time == 0xffffffff)
    {
        return 0xffffffff;
    }

    if (time != crc_STM32 (time >> 8))
    {
        return (0);
    }
    // uint8_t *ptr=(uint8_t *)TIME_ADDRESS;

    // for (i=0;i<4;i++){
    // time+=*ptr<<(8*i);
    // ptr++;
    // }

    return (time >> 8);
}

/*******************************************************************************

  set_time_value

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit dde179d5d24bd3d06cab73848ba1ab7e5efda898
                              Time in firmware Test #74 Test #105

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 set_time_value (u32 time)
{
int i, flag = 0;
u32* flash_value;
s32 page_s32;
u32 time_1 = time;

    LED_GreenOn ();

    // Add crc to time
    time = crc_STM32 (time);

    // Find free slot
    for (i = 0; i < 32; i++)
    {
        flash_value = (u32 *) (TIME_ADDRESS + TIME_OFFSET * i);
        if (getu32 ((u8 *) flash_value) == 0xffffffff)  // Is slot free ?
        {
            flashc_memcpy ((void *) (TIME_ADDRESS + TIME_OFFSET * i), (void *) &time, 4, TRUE);
            flag = 1;
            break;
        }
    }

    if (0 == flag)  // Clear page when no free slot was found, an write time
    {
        page_s32 = (TIME_ADDRESS - FLASH_START) / FLASH_PAGE_SIZE;
        flashc_erase_page (page_s32, TRUE);

        flashc_memcpy ((void *) TIME_ADDRESS, (void *) &time, 4, TRUE);
    }

    LED_GreenOff ();

    {
u8 text[30];

        CI_StringOut ("set_time_value: time ");
        itoa (time_1, text);
        CI_StringOut ((char *) text);
        CI_StringOut (" = ");
        itoa (crc_STM32 (time_1), text);
        CI_StringOut ((char *) text);
        CI_StringOut ("\r\n");
    }

    return 0;
}


/*******************************************************************************

  set_counter_value

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 set_counter_value (u32 addr, u64 counter)
{
s32 page_s32;

    page_s32 = (addr - FLASH_START) / FLASH_PAGE_SIZE;

    flashc_erase_page (page_s32, TRUE); // clear hole page

    flashc_memcpy ((void *) addr, (void *) &counter, 8, TRUE);  // set the counter base value

    return 0;
}



/*******************************************************************************

  increment_counter_page

  Increment the HOTP counter stored in flash

  addr - counter page address

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 increment_counter_page (u32 addr)
{
u16 i;
u8 n;
u16 dummy_u16;
u32 dummy_u32;
u8* ptr;
u64 counter;
FLASH_Status err = FLASH_COMPLETE;

    LED_GreenOn ();

    ptr = (u8 *) addr;  // Set counter page slot

    if (ptr[FLASH_PAGE_SIZE - 1] == 0x00)   // Are all token used ?
    {
        // Entire page is filled, erase cycle
        counter = get_counter_value (addr);

        // Clear the backup page
        flashc_memset8 ((void *) BACKUP_PAGE_ADDRESS, 0xFF, FLASH_PAGE_SIZE, TRUE);

        // write address to backup page
        flashc_memcpy ((void *) BACKUP_PAGE_ADDRESS, (void *) &counter, 8, FALSE);  // Area is erased

        // Write page addr to backup page
        flashc_memcpy ((void *) (BACKUP_PAGE_ADDRESS + BACKUP_ADDRESS_OFFSET), (void *) &addr, 4, FALSE);   // Area is erased


        dummy_u32 = 8;
        flashc_memcpy ((void *) (BACKUP_PAGE_ADDRESS + BACKUP_LENGTH_OFFSET), (void *) &dummy_u32, 4, FALSE);   // Area is erased

        // New erase by flash memset
        flashc_memset8 ((void *) addr, 0xFF, FLASH_PAGE_SIZE, TRUE);    // Erase counter page

        // Write counter page value
        flashc_memcpy ((void *) addr, (void *) &counter, 8, FALSE); // Area is erased

        // Write valid token to backup page
        dummy_u16 = 0x4F4B;
        flashc_memcpy ((void *) (BACKUP_PAGE_ADDRESS + BACKUP_OK_OFFSET), (void *) &dummy_u16, 2, FALSE);   // Area is erased

    }
    else
    {
        ptr += 8;   // Start of token area
        i = 0;

        while (i < TOKEN_PER_FLASHPAGE)
        {
            n = *ptr;
            if (n == 0xff)
            {
                break;  // Token is free
            }
            ptr++;
            i++;
        }

#ifdef DEBUG_HOTP
        {
u8 text[20];

            CI_TickLocalPrintf ("Mark token:");
            itoa ((u32) i, text);
            CI_LocalPrintf (text);
            CI_LocalPrintf (" - Counter:");
            itoa ((u32) get_counter_value (addr), text);
            CI_LocalPrintf (text);
            CI_LocalPrintf ("\n\r");

        }
#endif

        // Mark token as used
        flashc_memset8 ((void *) ptr, 0x00, 1, FALSE);  // Area is erased
        n = *ptr;
        /*
           { u8 text[20];

           CI_LocalPrintf ("After change0:" ); itoa ((u32)n,text); CI_LocalPrintf (text); CI_LocalPrintf ("\n\r"); } */
        if (0 != n) // If token is not erased
        {
            // Mark token as used with erasing page
            flashc_memset8 ((void *) ptr, 0x00, 1, TRUE);
#ifdef DEBUG_HOTP
            {
u8 text[20];
                n = *ptr;
                CI_LocalPrintf ("Token after erase: (0 = ok)");
                itoa ((u32) n, text);
                CI_LocalPrintf (text);
                CI_LocalPrintf ("\n\r");
            }
#endif
        }



    }

    LED_GreenOff ();

    return err; // no error
}

/*******************************************************************************

  validate_code_from_hotp_slot

  Changes
  Date      Reviewer        Info
  02.02.19  ET              Integration from NK Pro

*******************************************************************************/

s8 validate_code_from_hotp_slot(u8 slot_number, u32 code_to_verify) {
    const s32 RET_GENERAL_ERROR = -1;
    const s32 RET_CODE_NOT_VALID = -2;

    u8 generated_hotp_code_length = 6;
    FLASH_Status err;
    u32 calculated_code;

    if (slot_number >= NUMBER_OF_HOTP_SLOTS)
        return RET_GENERAL_ERROR;

    OTP_slot * hotp_slot = ((OTP_slot *) get_hotp_slot_addr(slot_number));
    if (hotp_slot->type != 'H') // unprogrammed slot_number
        return RET_GENERAL_ERROR;

    if ((hotp_slot->config & (1 << SLOT_CONFIG_DIGITS)) == 1)
        generated_hotp_code_length = 8;

    const u64 counter = get_counter_value (hotp_slot_counters[slot_number]);

    u8 counter_offset = 0;
    const u8 calculate_ahead_values = 10;
    for (counter_offset = 0; counter_offset < calculate_ahead_values; counter_offset++){
        calculated_code = get_hotp_value (counter+counter_offset, hotp_slot->secret, SECRET_LENGTH_DEFINE, generated_hotp_code_length);
        if (calculated_code == code_to_verify) break;
    }

    const u8 code_found = counter_offset < calculate_ahead_values;
    if(TRUE == code_found){
        //increment the counter for the lacking values, plus one to be ready for next validation
        u8 i;
        for (i = 0; i < counter_offset + 1; i++){
            err = (FLASH_Status) increment_counter_page (hotp_slot_counters[slot_number]);
            if (err != FLASH_COMPLETE) return RET_GENERAL_ERROR;
        }
        return counter_offset;
    }

    return RET_CODE_NOT_VALID;
}

/*******************************************************************************

  get_code_from_hotp_slot

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 get_code_from_hotp_slot (u8 slot_no)
{
u32 result;
u8 len = 6;
u64 counter;
FLASH_Status err;

    if (slot_no >= NUMBER_OF_HOTP_SLOTS)
        return 0;

    OTP_slot *slot = (OTP_slot *) get_hotp_slot_addr(slot_no);

    if ((slot->config & (1 << SLOT_CONFIG_DIGITS)) == 1)
        len = 8;

    if (slot->type != 'H') // unprogrammed or TOTP slot
        return 0;

    LED_GreenOn ();

    counter = get_counter_value (hotp_slot_counters[slot_no]);
    /*
       For a counter test without flash { static u32 co = 0; counter = co; co++; } */
#ifdef DEBUG_HOTP
    {
u8 text[20];
u32 c;

        DelayMs (10);
        CI_LocalPrintf ("HOTP counter:");
        itoa ((u32) counter, text);
        CI_LocalPrintf (text);
        CI_LocalPrintf ("\n\r");

        CI_LocalPrintf ("secret addr :");
        c = hotp_slots[slot] + SECRET_OFFSET;
        HexPrint (4, &c);
        CI_LocalPrintf ("\n\r");
    }
#endif

    result = get_hotp_value (counter, slot->secret, SECRET_LENGTH_DEFINE, len);


    err = increment_counter_page (hotp_slot_counters[slot_no]);

#ifdef DEBUG_HOTP
    {
u8 text[20];

        counter = get_counter_value (hotp_slot_counters[slot]);

        CI_LocalPrintf ("HOTP counter after inc:");
        itoa ((u32) counter, text);
        CI_LocalPrintf (text);
        CI_LocalPrintf ("\n\r");
    }
#endif

    LED_GreenOff ();

    if (err != FLASH_COMPLETE)
        return 0;

    return result;
}


/*******************************************************************************

  backup_data

  backup data to the backup page
  data - data to be backed up
  len  - length of the data
  addr - original address of the data

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void backup_data (u8 * data, u16 len, u32 addr)
{
u16 dummy_u16;
u32 dummy_u32;

    // New erase by flash memset
    flashc_memset8 ((void *) BACKUP_PAGE_ADDRESS, 0xFF, FLASH_PAGE_SIZE, TRUE);

    write_data_to_flash (data, len, BACKUP_PAGE_ADDRESS);

    dummy_u16 = len;
    flashc_memcpy ((void *) (BACKUP_PAGE_ADDRESS + BACKUP_LENGTH_OFFSET), (void *) &dummy_u16, 2, FALSE);   // Area is erased

    dummy_u32 = addr;
    flashc_memcpy ((void *) (BACKUP_PAGE_ADDRESS + BACKUP_ADDRESS_OFFSET), (void *) &dummy_u32, 4, FALSE);  // Area is erased

}

/*******************************************************************************

  erase_counter

  Clear HOTP slot (512 byte)

  Changes
  Date      Reviewer        Info
  14.08.14  RB              Integration from Stick 1.4
                              Commit 3170acd1e68c618aaffd16b2722108c7fc7d5725
                              Secret not overwriten when the Tool sends and empty secret
  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void erase_counter (u8 slot)
{
    LED_GreenOn ();

    flashc_erase_page (hotp_slot_counters[slot], TRUE); // clear hole page

    LED_GreenOff ();

}

/*******************************************************************************

  write_to_slot

  Write a paramter slot to the parameter page

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4
  14.08.14  RB              Integration from Stick 1.4
                              Commit 3170acd1e68c618aaffd16b2722108c7fc7d5725
                              Secret not overwriten when the Tool sends and empty secret
  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void write_to_slot (u8 * data, u8 * addr)
{
    u16 dummy_u16;
    u8  i;
    u8  Found;
    const u16 offset = (u16) addr - SLOTS_ADDRESS;

    LED_GreenOn ();

    // copy all slot data from Flash to RAM
    memcpy (page_buffer, (u8*) SLOTS_ADDRESS, FLASH_PAGE_SIZE * 3);

    OTP_slot *input_slot    = (OTP_slot *) data;
    OTP_slot *buffer_slot   = (OTP_slot *) (page_buffer + offset);

    // Check if the secret from the tool is empty and if it is use the old secret
    // Secret could begin with 0x00, so checking the whole secret before keeping the old one in mandatory
    Found = FALSE;
    for (i = 0; i < SECRET_LENGTH_DEFINE; i++)
    {
      if (0 != input_slot->secret[i])
      {
        Found = TRUE;
        break;
      }
    }

    if (FALSE == Found)
    {
        // Input secret is empty. Keep the secret that is currently in the buffer.
        memcpy (input_slot->secret, buffer_slot->secret, SECRET_LENGTH_DEFINE);
    }

    // write changes in input slot to RAM buffer
    memcpy (buffer_slot, input_slot, sizeof(OTP_slot));

    // write page to backup location
    backup_data (page_buffer, FLASH_PAGE_SIZE * 3, SLOTS_ADDRESS);

    // Clear flash mem
    // flashc_memset8 ((void*)SLOTS_ADDRESS,0xFF,FLASH_PAGE_SIZE*3,TRUE);

    // write page to regular location
    write_data_to_flash (page_buffer, FLASH_PAGE_SIZE * 3, SLOTS_ADDRESS);

    // Init backup block
    dummy_u16 = 0x4F4B;
    flashc_memcpy ((void *) (BACKUP_PAGE_ADDRESS + BACKUP_OK_OFFSET), (void *) &dummy_u16, 2, TRUE);

    LED_GreenOff ();

}

/*******************************************************************************

  write_to_config

  Write a new configuration to config section

  Changes
  Date      Reviewer        Info
  07.01.19  ET              Add function

*******************************************************************************/

void write_to_config (u8 * data, u8 len)
{
    u16 dummy_u16;

    LED_GreenOn ();

    // copy all slot data from Flash to RAM
    memcpy (page_buffer, (u8*) SLOTS_ADDRESS, FLASH_PAGE_SIZE * 3);

    // write changes in input slot to RAM buffer
    memcpy (page_buffer + GLOBAL_CONFIG_OFFSET, data, len);

    // write page to backup location
    backup_data (page_buffer, FLASH_PAGE_SIZE * 3, SLOTS_ADDRESS);

    // write page to regular location
    write_data_to_flash (page_buffer, FLASH_PAGE_SIZE * 3, SLOTS_ADDRESS);

    // Init backup block
    dummy_u16 = 0x4F4B;
    flashc_memcpy ((void *) (BACKUP_PAGE_ADDRESS + BACKUP_OK_OFFSET), (void *) &dummy_u16, 2, TRUE);

    LED_GreenOff ();

}


/*******************************************************************************

  check_backups

  check for any data on the backup page

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 check_backups ()
{
u32 address = getu32 ((u8 *) BACKUP_PAGE_ADDRESS + BACKUP_ADDRESS_OFFSET);
u16 ok = getu16 ((u8 *) BACKUP_PAGE_ADDRESS + BACKUP_OK_OFFSET);
u16 length = getu16 ((u8 *) BACKUP_PAGE_ADDRESS + BACKUP_LENGTH_OFFSET);

    if (ok == 0x4F4B)   // backed up data was correctly written to its destination
    {
        return 0;
    }
    else
    {

        if ((address != 0xffffffff) && (length <= 1000))    // todo 1000 > define
        {
            // New erase by flash memset
            flashc_memset8 ((void *) address, 0xFF, FLASH_PAGE_SIZE, TRUE);

            write_data_to_flash ((u8 *) BACKUP_PAGE_ADDRESS, length, address);

            // New erase by flash memset
            flashc_memset8 ((void *) BACKUP_PAGE_ADDRESS, 0xFF, FLASH_PAGE_SIZE, TRUE);

            return 1;   // backed up page restored
        }
        else
        {
            return 2;   // something bad happened, but before the original page was earsed, so we're safe (or there is nothing on the backup page)
        }
    }

}

/*******************************************************************************

  get_code_from_totp_slot

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 get_code_from_totp_slot (u8 slot_no, u64 challenge)
{
u64 time_min;
u32 result;
u8 len = 6;
time_t now;

    // Get the local ATMEL time
    time (&now);

    if (slot_no >= NUMBER_OF_TOTP_SLOTS)
        return 0;

    OTP_slot *slot = (OTP_slot *) get_totp_slot_addr(slot_no);

    // Add last time stamp from app and elapsed seconds since last set_time operation
    time_min = (current_time + now) / slot->interval_or_counter;

    if (slot->type != 'T') // unprogrammed slot
        return 0;

    if ((slot->config & (1 << SLOT_CONFIG_DIGITS)) == 1)
        len = 8;

    result = get_hotp_value (time_min, slot->secret, SECRET_LENGTH_DEFINE, len);

    return result;

}

/*******************************************************************************

  get_hotp_slot_addr

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


u8* get_hotp_slot_addr (u8 slot_number)
{
    return (u8*)(SLOTS_ADDRESS + get_slot_offset(slot_number));
}

/*******************************************************************************

  get_totp_slot_addr

  Changes
  Date      Reviewer        Info
  24.03.14  RB              Integration from Stick 1.4

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8* get_totp_slot_addr (u8 slot_number)
{
    return (u8*)(SLOTS_ADDRESS + get_slot_offset(NUMBER_OF_HOTP_SLOTS + slot_number));
}


u32 get_slot_offset(u8 slot_number)
{
    const u32 global_config_offset = 64;
    u32 slot_offset = sizeof(OTP_slot) * slot_number + global_config_offset;

    //FIXME: There is no way of communicating a failure/invalid slot number now
    if(slot_offset > (2 * FLASH_PAGE_SIZE + SLOT_PAGE_SIZE - sizeof(OTP_slot))) slot_offset = global_config_offset;

    return slot_offset;
}
