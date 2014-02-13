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


#include <avr32/io.h>
#include <stddef.h>
#include "compiler.h"
#include "flashc.h"
#include "string.h"

#include "global.h"
#include "tools.h"

#include "hotp.h"
//#include "memory_ops.h"
//#include "otp_sha1.h"
#include "hmac-sha1.h"



/*******************************************************************************

 Local defines

*******************************************************************************/

u32 hotp_slots[NUMBER_OF_HOTP_SLOTS]         = {SLOTS_ADDRESS + HOTP_SLOT1_OFFSET, SLOTS_ADDRESS + HOTP_SLOT2_OFFSET};
u32 hotp_slot_counters[NUMBER_OF_HOTP_SLOTS] = {SLOT1_COUNTER_ADDRESS, SLOT2_COUNTER_ADDRESS};
u32 hotp_slot_offsets[NUMBER_OF_HOTP_SLOTS]  = {HOTP_SLOT1_OFFSET, HOTP_SLOT2_OFFSET};

u32 totp_slots[NUMBER_OF_TOTP_SLOTS]         = {SLOTS_ADDRESS + TOTP_SLOT1_OFFSET, SLOTS_ADDRESS + TOTP_SLOT2_OFFSET, SLOTS_ADDRESS + TOTP_SLOT3_OFFSET, SLOTS_ADDRESS + TOTP_SLOT4_OFFSET};
u32 totp_slot_offsets[NUMBER_OF_TOTP_SLOTS]  = {TOTP_SLOT1_OFFSET, TOTP_SLOT2_OFFSET, TOTP_SLOT3_OFFSET, TOTP_SLOT4_OFFSET};

u8 page_buffer[SLOT_PAGE_SIZE];

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

extern u8 HID_GetReport_Value[32+1];

/*******************************************************************************

 Local declarations

*******************************************************************************/

/*******************************************************************************

  getu16

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u16 getu16(u8 *array)
{
  u16 result=array[0]+(array[1]<<8);
  return result;
}


/*******************************************************************************

  getu32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 getu32(u8 *array)
{
  u32 result=0;
  s8  i=0;


  for(i=3;i>=0;i--)
  {
    result<<=8;
    result+=array[i];
  }

  return result;
}


/*******************************************************************************

  getu64

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u64 getu64 (u8 *array)
{
  u64 result = 0;
  s8  i      = 0;

  for(i=7;i>=0;i--)
  {
    result <<= 8;
    result += array[i];
  }

  return result;
}


/*******************************************************************************

  endian_swap

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u64 endian_swap (u64 x)
{
	x = (x >> 56) |
	((x << 40) & 0x00FF000000000000LL) |
	((x << 24) & 0x0000FF0000000000LL) |
	((x <<  8) & 0x000000FF00000000LL) |
	((x >>  8) & 0x00000000FF000000LL) |
	((x >> 24) & 0x0000000000FF0000LL) |
	((x >> 40) & 0x000000000000FF00LL) | (x << 56);
	return x;
}



/*******************************************************************************

  dynamic_truncate

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 dynamic_truncate (u8 * hmac_result)
{

	u8 offset = hmac_result[19] & 0xf;
	u32 bin_code = (hmac_result[offset] & 0x7f) << 24
	| (hmac_result[offset + 1] & 0xff) << 16
	| (hmac_result[offset + 2] & 0xff) << 8
	| (hmac_result[offset + 3] & 0xff);

	return bin_code;
}


/*******************************************************************************

  write_data_to_flash

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void write_data_to_flash (u8 *data,u16 len,u32 addr)
{
  flashc_memcpy((void*)addr,data,len,TRUE);
}


/*******************************************************************************

  get_hotp_value

  Get a HOTP/TOTP truncated value
  counter - HOTP/TOTP counter value
  secret - pointer to secret stored in memory
  secret_length - length of the secret
  len - length of the truncated result, 6 or 8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 get_hotp_value(u64 counter,u8 * secret,u8 secret_length,u8 len)
{
	u8 hmac_result[20];
	u64 c = endian_swap(counter);

	hmac_sha1 (hmac_result, secret, secret_length*8, &c, 64);

	u32 hotp_result = dynamic_truncate (hmac_result);

	if (len==6)
	  hotp_result = hotp_result % 1000000;
	else if (len==8)
	  hotp_result = hotp_result % 100000000;
	else
	  return 0;

	return hotp_result;
}


/*******************************************************************************

  get_counter_value

  Get the HOTP counter stored in flash
  addr - counter page address

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u64 get_counter_value(u32 addr)
{
	u16 i;
	u64 counter=0;
	u8 *ptr=(u8 *)addr;

	// for (i=0;i<4;i++){
	// counter+=*ptr<<(8*i);
	// ptr++;
	// }

	counter  = *((u64 *)addr);
	ptr     +=8;

	i=0;

	while(i<1016)
	{
		if (*ptr==0xff)
		{
		  break;
		}
		ptr++;
		counter++;
		i++;
	}

	return counter;
}


/*******************************************************************************

  set_counter_value

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 set_counter_value(u32 addr, u64 counter)
{
//	FLASH_Status err=FLASH_COMPLETE;
/*
	FLASH_Unlock();

	err=FLASH_ErasePage(addr);
	if (err!=FLASH_COMPLETE) return err;

	err=FLASH_ProgramWord(addr, counter&0xffffffff);

	if (err!=FLASH_COMPLETE) return err;

	err=FLASH_ProgramWord(addr+4,  (counter>>32)&0xffffffff);

	if (err!=FLASH_COMPLETE) return err;

	FLASH_Lock();
*/

  flashc_memcpy ((void*)addr,(void*)&counter,8,TRUE);

	return 0;
}



/*******************************************************************************

  increment_counter_page

  Increment the HOTP counter stored in flash

  addr - counter page address

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 increment_counter_page(u32 addr)
{
	u16 i;
  u16 dummy_u16;
  u32 dummy_u32;
	u8 *ptr=(u8 *)addr;
	u64 counter;
	FLASH_Status err = FLASH_COMPLETE;

	if (ptr[1023]==0x00)
	{
		//Entire page is filled, erase cycle
		counter=get_counter_value(addr)+1;


		/* 
err=FLASH_ErasePage(BACKUP_PAGE_ADDRESS);
if (err!=FLASH_COMPLETE) return err;

//write address to backup page
err=FLASH_ProgramHalfWord(BACKUP_PAGE_ADDRESS, addr);
if (err!=FLASH_COMPLETE) return err;

err=FLASH_ProgramWord(BACKUP_PAGE_ADDRESS+4, counter&0xffffffff);
if (err!=FLASH_COMPLETE) return err;
err=FLASH_ProgramWord(BACKUP_PAGE_ADDRESS+8, (counter>>32)&0xffffffff);
if (err!=FLASH_COMPLETE) return err; */


/*
		FLASH_Unlock();
		err=FLASH_ErasePage(BACKUP_PAGE_ADDRESS);
		if (err!=FLASH_COMPLETE) return err;
*/
		// New erase by flash memset
		flashc_memset8 ((void*)BACKUP_PAGE_ADDRESS,0xFF,FLASH_PAGE_SIZE,TRUE);

		//write address to backup page
/*
		err=FLASH_ProgramWord(BACKUP_PAGE_ADDRESS, counter&0xffffffff);
		if (err!=FLASH_COMPLETE) return err;
		err=FLASH_ProgramWord(BACKUP_PAGE_ADDRESS+4, (counter>>32)&0xffffffff);
		if (err!=FLASH_COMPLETE) return err; 
*/
	  flashc_memcpy ((void*)BACKUP_PAGE_ADDRESS,(void*)&counter,8,TRUE);

/*
		err=FLASH_ProgramWord(BACKUP_PAGE_ADDRESS+BACKUP_ADDRESS_OFFSET, addr);
		if (err!=FLASH_COMPLETE) return err;
*/
    flashc_memcpy ((void*)(BACKUP_PAGE_ADDRESS+BACKUP_ADDRESS_OFFSET),(void*)&addr,4,TRUE);

/*
		err=FLASH_ProgramWord(BACKUP_PAGE_ADDRESS+BACKUP_LENGTH_OFFSET, 8);
		if (err!=FLASH_COMPLETE) return err;
*/
    dummy_u32 = 8;
    flashc_memcpy ((void*)(BACKUP_PAGE_ADDRESS+BACKUP_LENGTH_OFFSET),(void*)&dummy_u32,4,TRUE);

/*
		err=FLASH_ErasePage(addr);
		if (err!=FLASH_COMPLETE) return err;
*/
    // New erase by flash memset
    flashc_memset8 ((void*)addr,0xFF,FLASH_PAGE_SIZE,TRUE);

/*
		err=FLASH_ProgramWord(addr, counter&0xffffffff);
		if (err!=FLASH_COMPLETE) return err;
		err=FLASH_ProgramWord(addr+4,  (counter>>32)&0xffffffff);
		if (err!=FLASH_COMPLETE) return err;
*/
    flashc_memcpy ((void*)BACKUP_PAGE_ADDRESS,(void*)&counter,8,TRUE);

/*
		err=FLASH_ProgramHalfWord(BACKUP_PAGE_ADDRESS+BACKUP_OK_OFFSET, 0x4F4B);
		if (err!=FLASH_COMPLETE) return err;
*/
    dummy_u16 = 0x4F4B;
    flashc_memcpy ((void*)(BACKUP_PAGE_ADDRESS+BACKUP_OK_OFFSET),(void*)&dummy_u16,2,TRUE);

/*
		FLASH_Lock();
*/
	}
	else{

		ptr+=8;
		i=0;
		while(i<1016){
			if (*ptr==0xff)
			  break;
			ptr++;
			i++;
		}
/*
		FLASH_Unlock();
*/
		if ((u32)ptr%2){ //odd byte
/*
			err=FLASH_ProgramHalfWord((u32)ptr-1, 0x0000);
*/
	    dummy_u16 = 0x0000;
	    flashc_memcpy ((void*)(ptr-1),(void*)&dummy_u16,2,TRUE);


		}
		else{ //even byte
/*
			err=FLASH_ProgramHalfWord((u32)ptr, 0xff00);
*/
      dummy_u16 = 0xFF00;
      flashc_memcpy ((void*)ptr,(void*)&dummy_u16,2,TRUE);


		}
/*
		FLASH_Lock();
*/
	}

	return err; //no error
}


/*******************************************************************************

  get_code_from_hotp_slot

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 get_code_from_hotp_slot(u8 slot)
{
	u32 result;
	u8 len=6;
	u64 counter;
	FLASH_Status err;
	u8 config=0;

	if (slot>=NUMBER_OF_HOTP_SLOTS) return 0;

	config=get_hotp_slot_config(slot);

	if (config&(1<<SLOT_CONFIG_DIGITS))
	  len=8;

	result=*((u8 *)hotp_slots[slot]);

	if (result==0xFF)//unprogrammed slot
	  return 0;

	counter = get_counter_value(hotp_slot_counters[slot]);

	result = get_hotp_value(counter,(u8 *)(hotp_slots[slot]+SECRET_OFFSET),20,len);

	err=increment_counter_page(hotp_slot_counters[slot]);

	if (err!=FLASH_COMPLETE) return 0;

	return result;
}


/*******************************************************************************

  backup_data

  backup data to the backup page
  data -data to be backed up
  len - length of the data
  addr - original address of the data

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void backup_data(u8 *data,u16 len, u32 addr)
{
  u16 dummy_u16;
  u32 dummy_u32;

/*
	FLASH_Unlock();
	FLASH_ErasePage(BACKUP_PAGE_ADDRESS);
*/
  // New erase by flash memset
  flashc_memset8 ((void*)BACKUP_PAGE_ADDRESS,0xFF,FLASH_PAGE_SIZE,TRUE);

	write_data_to_flash(data,len,BACKUP_PAGE_ADDRESS);
/*
	FLASH_ProgramHalfWord(BACKUP_PAGE_ADDRESS+BACKUP_LENGTH_OFFSET, len);
*/
  dummy_u16 = len;
  flashc_memcpy ((void*)(BACKUP_PAGE_ADDRESS+BACKUP_LENGTH_OFFSET),(void*)&dummy_u16,2,TRUE);

/*
	FLASH_ProgramWord(BACKUP_PAGE_ADDRESS+BACKUP_ADDRESS_OFFSET, addr);
*/
  dummy_u32 = addr;
  flashc_memcpy ((void*)(BACKUP_PAGE_ADDRESS+BACKUP_ADDRESS_OFFSET),(void*)&dummy_u32,4,TRUE);

/*
	FLASH_Lock();
*/
}


/*******************************************************************************

  write_to_slot

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void write_to_slot(u8 *data, u16 offset, u16 len)
{
  u16 dummy_u16;

	//copy entire page to ram
	u8 *page=(u8 *)SLOTS_ADDRESS;
	memcpy(page_buffer,page,SLOT_PAGE_SIZE);

	//make changes to page
	memcpy(page_buffer+offset,data,len);


	//write page to backup location
	backup_data(page_buffer,SLOT_PAGE_SIZE,SLOTS_ADDRESS);

	//write page to regular location
/*
	FLASH_Unlock();
	FLASH_ErasePage(SLOTS_ADDRESS);
*/
  // New erase by flash memset
  flashc_memset8 ((void*)SLOTS_ADDRESS,0xFF,FLASH_PAGE_SIZE,TRUE);

	write_data_to_flash(page_buffer,SLOT_PAGE_SIZE,SLOTS_ADDRESS);

/*
	FLASH_ProgramHalfWord(BACKUP_PAGE_ADDRESS+BACKUP_OK_OFFSET, 0x4F4B);
*/
  dummy_u16 = 0x4F4B;
  flashc_memcpy ((void*)(BACKUP_PAGE_ADDRESS+BACKUP_OK_OFFSET),(void*)&dummy_u16,2,TRUE);


/*
	FLASH_Lock();
*/
}


/*******************************************************************************

  check_backups

  check for any data on the backup page

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 check_backups()
{
	u32 address =getu32((u8 *)BACKUP_PAGE_ADDRESS+BACKUP_ADDRESS_OFFSET);
	u16 ok      =getu16((u8 *)BACKUP_PAGE_ADDRESS+BACKUP_OK_OFFSET);
	u16 length  =getu16((u8 *)BACKUP_PAGE_ADDRESS+BACKUP_LENGTH_OFFSET);

	if (ok == 0x4F4B)//backed up data was correctly written to its destination
	{
	  return 0;
	}
	else
	{

		if ((address != 0xffffffff) && (length <= 1000))
		{
/*
		  FLASH_Unlock();
			FLASH_ErasePage(address);
*/

		  // New erase by flash memset
		  flashc_memset8 ((void*)address,0xFF,FLASH_PAGE_SIZE,TRUE);

			write_data_to_flash((u8 *)BACKUP_PAGE_ADDRESS,length,address);
/*
			FLASH_ErasePage(BACKUP_PAGE_ADDRESS);
*/
      // New erase by flash memset
      flashc_memset8 ((void*)BACKUP_PAGE_ADDRESS,0xFF,FLASH_PAGE_SIZE,TRUE);

/*
			FLASH_Lock();
*/
			return 1; //backed up page restored
		}
		else
		{
		  return 2; //something bad happened, but before the original page was earsed, so we're safe (or there is nothing on the backup page)
		}
	}

}


/*******************************************************************************

  get_hotp_slot_config

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 get_hotp_slot_config(u8 slot_number)
{
	u8 result=0;
	if (slot_number>=NUMBER_OF_HOTP_SLOTS)
	  return 0;
	else
	{
		result=((u8 *)hotp_slots[slot_number])[CONFIG_OFFSET];
	}

	return result;
}

u8 get_totp_slot_config(u8 slot_number)
{
	u8 result=0;
	if (slot_number>=NUMBER_OF_TOTP_SLOTS)
	  return 0;
	else
	{
		result=((u8 *)totp_slots[slot_number])[CONFIG_OFFSET];
	}

	return result;
}


/*******************************************************************************

  get_code_from_totp_slot

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 get_code_from_totp_slot(u8 slot, u64 challenge)
{
	u32 result;
	u8 config=0;
	u8 len=6;

	if (slot>=NUMBER_OF_TOTP_SLOTS) return 0;


	result=*((u8 *)totp_slots[slot]);
	if (result==0xFF)//unprogrammed slot
	  return 0;


	config=get_totp_slot_config(slot);

	if (config&(1<<SLOT_CONFIG_DIGITS))
	  len=8;

	result= get_hotp_value(challenge,(u8 *)(totp_slots[slot]+SECRET_OFFSET),20,len);


	return result;

}

/*******************************************************************************

  get_hotp_slot_addr

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


u8 *get_hotp_slot_addr(u8 slot_number)
{
  u8 *result = NULL;

  if (slot_number>=NUMBER_OF_HOTP_SLOTS)
  {
    return NULL;
  }
  else
  {
    result = (u8 *)hotp_slots[slot_number];
  }

  return result;
}

/*******************************************************************************

  get_totp_slot_addr

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 *get_totp_slot_addr (u8 slot_number)
{
  u8 *result = NULL;

  if (slot_number>=NUMBER_OF_TOTP_SLOTS)
  {
    return NULL;
  }
  else
  {
    result = (u8 *)totp_slots[slot_number];
  }

  return result;
}


#ifdef NOT_USED

#endif // NOT_USED





