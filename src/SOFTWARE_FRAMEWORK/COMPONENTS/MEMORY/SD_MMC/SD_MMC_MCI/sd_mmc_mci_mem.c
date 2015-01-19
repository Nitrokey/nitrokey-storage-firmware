/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief CTRL_ACCESS interface for SD/MMC card.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with an MCI module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

/* Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 */
/*
* This file contains modifications done by Rudolf Boeddeker
* For the modifications applies:
*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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



//_____  I N C L U D E S ___________________________________________________

#include "conf_access.h"


#if (SD_MMC_MCI_0_MEM == ENABLE) || (SD_MMC_MCI_1_MEM == ENABLE)

#include "conf_sd_mmc_mci.h"
#include "sd_mmc_mci.h"
#include "sd_mmc_mci_mem.h"


#include "global.h"

#include "LED_test.h"
#include "tools.h"
#include "..\HighLevelFunctions\FlashStorage.h"

void TIME_MEASURING_Start (unsigned char Timer);
void TIME_MEASURING_INT_Start (unsigned char Timer);
unsigned int  TIME_MEASURING_Stop (unsigned char Timer);
#define TIME_MEASURING_TIME_SD_ACCESS    7

//_____ D E F I N I T I O N S ______________________________________________
/*
#define Sd_mmc_mci_access_signal_on()
#define Sd_mmc_mci_access_signal_off()
*/
//! Initialization sequence status per Slot.
extern  Bool sd_mmc_mci_init_done[MCI_NR_SLOTS];
//! SD/MMC Card Size per Slot.
extern  U32  g_u32_card_size[MCI_NR_SLOTS];
//! SD/MMC Card Presence status per Slot.
U8   sd_mmc_mci_presence_status[MCI_NR_SLOTS] = {SD_MMC_REMOVED, SD_MMC_REMOVED}; // {SD_MMC_INSERTED, SD_MMC_INSERTED};

/*! \name Control Interface
 */
//! @{


U32 gSdStartUncryptedVolume_u32 = 0;
U32 gSdEndUncryptedVolume_u32   = SD_SIZE_UNCRYPTED_PARITION - 1;

U32 gSdStartCryptedVolume_u32   = SD_SIZE_UNCRYPTED_PARITION;
U32 gSdEndCryptedVolume_u32     = (2*1024*2048) - 1;

U32 gSdStartHiddenVolume_u32    = SD_SIZE_UNCRYPTED_PARITION + (2*1024*2048);
U32 gSdEndHiddenVolume_u32      = (2*1024*2048) + (2*1024*2048) - 1;

U32 gSdEndOfCard_u32            = 0;    // 0 = SD size is not present


U32 sd_FlagHiddenLun_u32         = 0;
U32 sd_MaxAccessedBlockReadMin_u32  = 0;
U32 sd_MaxAccessedBlockReadMax_u32  = 4000000000UL;
U32 sd_MaxAccessedBlockWriteMin_u32 = 0;
U32 sd_MaxAccessedBlockWriteMax_u32 = 4000000000UL;

U32 sd_mmc_mci_access_signal_on_flag_u32 = FALSE;

void  Sd_mmc_mci_access_signal_on(void)
{
  sd_mmc_mci_access_signal_on_flag_u32 = TRUE;

#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Start (TIME_MEASURING_TIME_SD_ACCESS);
#endif
}

void Sd_mmc_mci_access_signal_off(void)
{
  sd_mmc_mci_access_signal_on_flag_u32 = FALSE;
#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Stop (TIME_MEASURING_TIME_SD_ACCESS);
#endif
}

U32 Check_Sd_mmc_mci_access_signal_on (void)
{
  return (sd_mmc_mci_access_signal_on_flag_u32);
}


int sd_mmc_mci_test_unit_only_local_access = FALSE;


/*******************************************************************************

  sd_mmc_mci_test_unit_ready

  Unit 0 = LUN 0 = Encrypted volume or hidden volume
  Unit 1 = LUN 1 = Uncrypted volume

  Changes
  Date      Author          Info

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


Ctrl_status sd_mmc_mci_test_unit_ready(U8 slot)
{
//  static U8 LastStateTestUnitReady[MCI_NR_SLOTS] = { 0 ,0 };

  if (slot > MCI_LAST_SLOTS) return CTRL_FAIL;

// If sd lock for local access
  if (TRUE == sd_mmc_mci_test_unit_only_local_access)
  {
    sd_mmc_mci_mem_check(slot); // Dummy read
    return (CTRL_NO_PRESENT);
  }

  if (0 == slot)    // slot = 0 > encrypted or hidden volume
  {
    if (FALSE == GetSdEncryptedCardEnableState ())    // Flag for enabling the encrypted sd card lun
    {
      return CTRL_NO_PRESENT;
    }
  }

  if (1 == slot)    // slot = 1 > uncrypted volume
  {
    if (FALSE == GetSdUncryptedCardEnableState ())    // Flag for enabling the uncrypted sd card lun
    {
      return CTRL_NO_PRESENT;
    }
  }

  slot = 0;   // In real there is only 1 sd card

  Sd_mmc_mci_access_signal_on();
  switch (sd_mmc_mci_presence_status[slot])
  {
    case SD_MMC_REMOVED:
      sd_mmc_mci_init_done[slot] = FALSE;
      if (OK == sd_mmc_mci_mem_check(slot))
      {
        sd_mmc_mci_presence_status[slot] = SD_MMC_INSERTED;
        Sd_mmc_mci_access_signal_off();
        return CTRL_BUSY;
      }
      Sd_mmc_mci_access_signal_off();
      return CTRL_NO_PRESENT;

    case SD_MMC_INSERTED:
      if (OK != sd_mmc_mci_mem_check(slot))
      {
        sd_mmc_mci_presence_status[slot] = SD_MMC_REMOVING;
        sd_mmc_mci_init_done[slot] = FALSE;
        Sd_mmc_mci_access_signal_off();
        return CTRL_BUSY;
      }
      Sd_mmc_mci_access_signal_off();
      return CTRL_GOOD;

    case SD_MMC_REMOVING:
      sd_mmc_mci_presence_status[slot] = SD_MMC_REMOVED;
      Sd_mmc_mci_access_signal_off();
      return CTRL_NO_PRESENT;

    default:
      sd_mmc_mci_presence_status[slot] = SD_MMC_REMOVED;
      Sd_mmc_mci_access_signal_off();
      return CTRL_BUSY;
  }
}


Ctrl_status sd_mmc_mci_unit_state_e = CTRL_GOOD;

/*******************************************************************************

  sd_mmc_mci_test_unit_ready_0

  Unit 0 = LUN 0 = Encrypted volume or hidden volume

  Changes
  Date      Author          Info

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Ctrl_status sd_mmc_mci_test_unit_ready_0(void)
{
//return sd_mmc_mci_test_unit_ready(0);

// If sd lock for local access
  if (TRUE == sd_mmc_mci_test_unit_only_local_access)
  {
    return (CTRL_NO_PRESENT);
  }

  return sd_mmc_mci_test_unit_ready(0);
/*
	if (CTRL_GOOD == sd_mmc_mci_unit_state_e)
	{
		return sd_mmc_mci_test_unit_ready(0);
	}
	else
	{
		return (sd_mmc_mci_unit_state_e);		// Simulate local state
	}
*/
}

/*******************************************************************************

  sd_mmc_mci_test_unit_ready_1

  Unit 1 = LUN 1 = Uncrypted volume

  Changes
  Date      Author          Info

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


Ctrl_status sd_mmc_mci_test_unit_ready_1(void)
{
// If sd lock for local access
  if (TRUE == sd_mmc_mci_test_unit_only_local_access)
  {
    return (CTRL_NO_PRESENT);
  }

   return sd_mmc_mci_test_unit_ready(1);    // RB
}


Ctrl_status sd_mmc_mci_read_capacity(U8 slot, U32 *u32_nb_sector)
{
   Sd_mmc_mci_access_signal_on();

   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }
   *u32_nb_sector = g_u32_card_size[slot]-1;
   Sd_mmc_mci_access_signal_off();
   return CTRL_GOOD;
}

/*******************************************************************************

  sd_mmc_mci_read_capacity_0

  Unit 0 = LUN 0 = Encrypted volume or hidden volume

  Size of the encrypted or hidden volume

  Changes
  Date      Author    Info
  08.02.14  RB        Implementation of dynamic sizes of volumes


  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Ctrl_status sd_mmc_mci_read_capacity_0(U32 *u32_nb_sector)
{
  *u32_nb_sector = GetSizeCryptedVolume_u32 ();

//  Used for the hidden lun
  if (SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION == sd_FlagHiddenLun_u32)
  {
    *u32_nb_sector = GetSizeHiddenVolume_u32 ();
  }

  return (CTRL_GOOD);

}

/*******************************************************************************

  sd_mmc_mci_read_capacity_1

  Size of volume 1, the uncrypted volume

  Changes
  Date      Author    Info
  08.02.14  RB        Implementation of dynamic sizes of volumes

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


Ctrl_status sd_mmc_mci_read_capacity_1(U32 *u32_nb_sector)
{
  *u32_nb_sector = GetSizeUncryptedVolume_u32 ();

  return (CTRL_GOOD);

}


Bool sd_mmc_mci_wr_protect(U8 slot)
{
  return is_sd_mmc_mci_card_protected(slot);
}


Bool sd_mmc_mci_wr_protect_0(void)
{
  return sd_mmc_mci_wr_protect(0);
}


Bool sd_mmc_mci_wr_protect_1(void)    // Uncrypted partition
{
  if (READ_WRITE_ACTIVE == GetSdUncryptedCardReadWriteEnableState ())    // TRUE = Write enable
  {
    return (FALSE);
  }
  return (TRUE);
}


Bool sd_mmc_mci_removal(U8 slot)
{
  return FALSE;
}


Bool sd_mmc_mci_removal_0(void)
{
  return sd_mmc_mci_removal(0);
}


Bool sd_mmc_mci_removal_1(void)   // Uncrypted partition
{
  return sd_mmc_mci_removal(0);
}

//! @}

#if ACCESS_USB == ENABLED

#include "usb_drv.h"
#include "scsi_decoder.h"

#  define DMACA_AES_EVAL_USART               (&AVR32_USART1)



/*! \name MEM <-> USB Interface
 */
//! @{


Ctrl_status sd_mmc_mci_usb_read_10(U8 slot, U32 addr, U16 nb_sector)
{
	unsigned int   c0, c1; // Performance evaluation variables
	unsigned int   i;

   c0 = Get_system_register(AVR32_COUNT);
/*
   for (i=0;i<nb_sector;i++)
   {
     Sd_mmc_mci_access_signal_on();

     if( !sd_mmc_mci_mem_check(slot) )
     {
       Sd_mmc_mci_access_signal_off();
       return CTRL_NO_PRESENT;
     }

     if( !sd_mmc_mci_dma_read_open(slot, addr+i, NULL, 1) )
       return CTRL_FAIL;

     if( !sd_mmc_mci_read_multiple_sector(slot, 1, addr+i) )
       return CTRL_FAIL;

     if( !sd_mmc_mci_read_close(slot) )
       return CTRL_FAIL;

     Sd_mmc_mci_access_signal_off();
   }
*/


   Sd_mmc_mci_access_signal_on();

   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   if( !sd_mmc_mci_dma_read_open(slot, addr, NULL, nb_sector) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_read_multiple_sector(slot, nb_sector, addr) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_read_close(slot) )
     return CTRL_FAIL;


   Sd_mmc_mci_access_signal_off();

   c1 = Get_system_register(AVR32_COUNT);

/*
     CI_LocalPrintf("AES R : %ld %ld",addr,nb_sector);
     CI_LocalPrintf(" - ");
     CI_LocalPrintf("%ld ticks\n",c1 - c0);

     CI_LocalPrintf("AES R : ");
   print_ulong(DMACA_AES_EVAL_USART, nb_sector*SD_MMC_SECTOR_SIZE);
     CI_LocalPrintf(" - ");
   print_ulong(DMACA_AES_EVAL_USART,c1 - c0);
     CI_LocalPrintf(" ticks\n");
*/
/*
	// AES Test
	if (STICK20_AES_BUFFER_SIZE >= nb_sector*SD_MMC_SECTOR_SIZE)
	{
		STICK20_ram_aes_ram(nb_sector*SD_MMC_SECTOR_SIZE,(unsigned int *)addr, pSTICK20_AES_BUFFER);
	}
	else
	{
		  CI_LocalPrintf("AES Size to big: ");
		print_ulong(DMACA_AES_EVAL_USART, nb_sector*SD_MMC_SECTOR_SIZE);
	}
*/
   return CTRL_GOOD;
}

/*******************************************************************************

  sd_mmc_mci_usb_read_10_0

  Read data from of volume 0, the crypted or hidden volume

  Changes
  Date      Author    Info
  08.02.14  RB        Implementation of dynamic sizes of volumes

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Ctrl_status sd_mmc_mci_usb_read_10_0(U32 addr, U16 nb_sector)
{
  Ctrl_status Ret;

  LED_RedGreenOn ();

// Add offset for the crypted or hidden volume
  if (SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION == sd_FlagHiddenLun_u32)
  {
    addr += GetStartHiddenVolume_u32 ();     // hidden volume
  }
  else
  {
    addr += GetStartCryptedVolume_u32 ();     // crypted volume
  }

  if ((addr > sd_MaxAccessedBlockReadMin_u32) && (addr < gSdEndOfCard_u32/2))    // Min is only in the low half
  {
    sd_MaxAccessedBlockReadMin_u32 = addr;
  }

  if (addr > sd_MaxAccessedBlockReadMin_u32)    // Don't log min values
  {
    if (addr < sd_MaxAccessedBlockReadMax_u32)
    {
      sd_MaxAccessedBlockReadMax_u32 = addr;
    }
  }

  Ret = sd_mmc_mci_usb_read_10(0, addr, nb_sector);

  LED_RedGreenOff ();

  return (Ret);
}


Ctrl_status sd_mmc_mci_usb_read_10_1(U32 addr, U16 nb_sector)   // RB
{
  if ((addr > sd_MaxAccessedBlockReadMin_u32) && (addr < gSdEndOfCard_u32/2))    // Min is only in the low half
  {
    sd_MaxAccessedBlockReadMin_u32 = addr;
  }
  if (addr > sd_MaxAccessedBlockReadMin_u32)      // Don't log min values
  {
    if (addr < sd_MaxAccessedBlockReadMax_u32)
    {
      sd_MaxAccessedBlockReadMax_u32 = addr;
    }
  }

  return sd_mmc_mci_usb_read_10(0, addr, nb_sector);    // uncrypted volume
}


void sd_mmc_mci_read_multiple_sector_callback(const void *psector)
{
  U16 data_to_transfer = SD_MMC_SECTOR_SIZE;

  // Transfer read sector to the USB interface.
  while (data_to_transfer)
  {
    while (!Is_usb_in_ready(g_scsi_ep_ms_in))
    {
      if(!Is_usb_endpoint_enabled(g_scsi_ep_ms_in))
         return; // USB Reset
    }         

    Usb_reset_endpoint_fifo_access(g_scsi_ep_ms_in);
    data_to_transfer = usb_write_ep_txpacket(g_scsi_ep_ms_in, psector,
                                             data_to_transfer, &psector);
    Usb_ack_in_ready_send(g_scsi_ep_ms_in);
  }
}


Ctrl_status sd_mmc_mci_usb_write_10(U8 slot,U32 addr, U16 nb_sector)
{
	unsigned int   c0, c1; // Performance evaluation variables
  unsigned int   i;

   c0 = Get_system_register(AVR32_COUNT);

   LED_RedGreenOn ();

/*
   for (i=0;i<nb_sector;i++)
   {
     Sd_mmc_mci_access_signal_on();

     if( !sd_mmc_mci_mem_check(slot) )
     {
       Sd_mmc_mci_access_signal_off();
       return CTRL_NO_PRESENT;
     }

     if( !sd_mmc_mci_dma_write_open(slot, addr+i, NULL, 1) )
       return CTRL_FAIL;

     if( !sd_mmc_mci_write_multiple_sector(slot, 1,addr+i) )
       return CTRL_FAIL;

     if( !sd_mmc_mci_write_close(slot) )
       return CTRL_FAIL;

     Sd_mmc_mci_access_signal_off();
   }
*/

   Sd_mmc_mci_access_signal_on();

   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }
   if( !sd_mmc_mci_dma_write_open(slot, addr, NULL, nb_sector) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_write_multiple_sector(slot, nb_sector,addr) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_write_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();

   LED_RedGreenOff ();


   c1 = Get_system_register(AVR32_COUNT);

/*
     CI_LocalPrintf("AES W : %ld",nb_sector);
     CI_LocalPrintf(" - ");
     CI_LocalPrintf("%ld ticks\n",c1 - c0);
*/
   return CTRL_GOOD;

}

/*******************************************************************************

  sd_mmc_mci_usb_write_10_0

  Write data for volume 0

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of dynamic volume sizes

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Ctrl_status sd_mmc_mci_usb_write_10_0(U32 addr, U16 nb_sector)
{
// Add offset for the crypted or hidden volume
  if (SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION == sd_FlagHiddenLun_u32)
  {
    addr += GetStartHiddenVolume_u32 ();     // hidden volume
  }
  else
  {
    addr += GetStartCryptedVolume_u32 ();     // crypted volume
  }

  if ((addr > sd_MaxAccessedBlockWriteMin_u32) && (addr < gSdEndOfCard_u32/2))    // Min is only in the low half
  {
    sd_MaxAccessedBlockWriteMin_u32 = addr;
  }
  if (addr > sd_MaxAccessedBlockWriteMin_u32)      // Don't log min values
  {
    if (addr < sd_MaxAccessedBlockWriteMax_u32)
    {
      sd_MaxAccessedBlockWriteMax_u32 = addr;
    }
  }

  return sd_mmc_mci_usb_write_10(0, addr, nb_sector);
}


Ctrl_status sd_mmc_mci_usb_write_10_1(U32 addr, U16 nb_sector)
{
if ((addr > sd_MaxAccessedBlockWriteMin_u32) && (addr < gSdEndOfCard_u32/2))    // Min is only in the low half
  {
    sd_MaxAccessedBlockWriteMin_u32 = addr;
  }
  if (addr > sd_MaxAccessedBlockWriteMin_u32)      // Don't log min values
  {
    if (addr < sd_MaxAccessedBlockWriteMax_u32)    // Real SD Block
    {
      sd_MaxAccessedBlockWriteMax_u32 = addr;
    }
  }
  return sd_mmc_mci_usb_write_10(0, addr, nb_sector);
}


void sd_mmc_mci_write_multiple_sector_callback(void *psector)
{
  U16 data_to_transfer = SD_MMC_SECTOR_SIZE;

  // Transfer sector to write from the USB interface.
  while (data_to_transfer)
  {
    while (!Is_usb_out_received(g_scsi_ep_ms_out))
    {
      if(!Is_usb_endpoint_enabled(g_scsi_ep_ms_out))
         return; // USB Reset
    }         

    Usb_reset_endpoint_fifo_access(g_scsi_ep_ms_out);
    data_to_transfer = usb_read_ep_rxpacket(g_scsi_ep_ms_out, psector,
                                            data_to_transfer, &psector);
    Usb_ack_out_received_free(g_scsi_ep_ms_out);
  }
}


//! @}

#endif  // ACCESS_USB == ENABLED


#if ACCESS_MEM_TO_RAM == ENABLED

/*! \name MEM <-> RAM Interface
 */
//! @{


Ctrl_status sd_mmc_mci_mem_2_ram(U8 slot, U32 addr, void *ram)
{
   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   if( !sd_mmc_mci_read_open(slot, addr, 1) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_read_sector_2_ram(slot, ram) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_read_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();
   return CTRL_GOOD;

}


Ctrl_status sd_mmc_mci_dma_mem_2_ram(U8 slot, U32 addr, void *ram)
{
   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   if( !sd_mmc_mci_dma_read_open(slot, addr, ram, 1) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_dma_read_sector_2_ram(slot,ram) )
     return CTRL_FAIL;

   if(! sd_mmc_mci_read_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();
   return CTRL_GOOD;

}

/*******************************************************************************

  sd_mmc_mci_mem_2_ram_0

  Write data from sd card to ram for volume 0

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of dynamic volume sizes

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Ctrl_status sd_mmc_mci_mem_2_ram_0(U32 addr, void *ram)
{

// Add offset for the crypted or hidden volume
  if (SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION == sd_FlagHiddenLun_u32)
  {
    addr += GetStartHiddenVolume_u32 ();     // hidden volume
  }
  else
  {
    addr += GetStartCryptedVolume_u32 ();     // crypted volume
  }

  if ((addr > sd_MaxAccessedBlockReadMin_u32) && (addr < gSdEndOfCard_u32/2))    // Min is only in the low half
  {
    sd_MaxAccessedBlockReadMin_u32 = addr;
  }
  if (addr > sd_MaxAccessedBlockReadMin_u32)      // Don't log min values
  {
    if (addr < sd_MaxAccessedBlockReadMax_u32)     // Real SD block
    {
      sd_MaxAccessedBlockReadMax_u32 = addr;
    }
  }

  return sd_mmc_mci_mem_2_ram(0, addr, ram);
}


Ctrl_status sd_mmc_mci_mem_2_ram_1(U32 addr, void *ram)
{
  return sd_mmc_mci_mem_2_ram(0, addr, ram);    // RB
}


Ctrl_status sd_mmc_mci_multiple_mem_2_ram(U8 slot, U32 addr, void *ram, U32 nb_sectors)
{
   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   if( !sd_mmc_mci_read_open(slot, addr, nb_sectors) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_read_multiple_sector_2_ram(slot, ram, nb_sectors) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_read_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();
   return CTRL_GOOD;
}


Ctrl_status sd_mmc_mci_dma_multiple_mem_2_ram(U8 slot, U32 addr, void *ram, U32 nb_sectors)
{
   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   if( !sd_mmc_mci_dma_read_open(slot, addr, ram, nb_sectors ) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_dma_read_multiple_sector_2_ram(slot, ram, nb_sectors) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_read_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();
   return CTRL_GOOD;
}

/*******************************************************************************

  sd_mmc_mci_multiple_mem_2_ram_0

  Write data from sd card to ram for volume 0

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of dynamic volume sizes

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Ctrl_status sd_mmc_mci_multiple_mem_2_ram_0(U32 addr, void *ram, U32 nb_sectors)
{

// Add offset for the crypted or hidden volume
  if (SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION == sd_FlagHiddenLun_u32)
  {
    addr += GetStartHiddenVolume_u32 ();     // hidden volume
  }
  else
  {
    addr += GetStartCryptedVolume_u32 ();     // crypted volume
  }

  if ((addr > sd_MaxAccessedBlockReadMin_u32) && (addr < gSdEndOfCard_u32/2))    // Min is only in the low half
  {
    sd_MaxAccessedBlockReadMin_u32 = addr;
  }
  if (addr > sd_MaxAccessedBlockReadMin_u32)      // Don't log min values
  {
    if (addr < sd_MaxAccessedBlockReadMax_u32)     // Real SD block
    {
      sd_MaxAccessedBlockReadMax_u32 = addr;
    }
  }
  return sd_mmc_mci_multiple_mem_2_ram(0, addr, ram, nb_sectors);
}


Ctrl_status sd_mmc_mci_multiple_mem_2_ram_1(U32 addr, void *ram, U32 nb_sectors)
{
  return sd_mmc_mci_multiple_mem_2_ram(1, addr, ram, nb_sectors);
}


Ctrl_status sd_mmc_mci_ram_2_mem(U8 slot, U32 addr, const void *ram)
{
   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   if( !sd_mmc_mci_write_open(slot, addr, 1) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_write_sector_from_ram(slot,ram) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_write_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();
   return CTRL_GOOD;
}


Ctrl_status sd_mmc_mci_dma_ram_2_mem(U8 slot, U32 addr, const void *ram)
{
   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   if( !sd_mmc_mci_dma_write_open(slot, addr, ram, 1) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_dma_write_sector_from_ram(slot, ram) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_write_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();
   return CTRL_GOOD;
}

/*******************************************************************************

  sd_mmc_mci_ram_2_mem_0

  Write data from ram to sd card for volume 0

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of dynamic volume sizes

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Ctrl_status sd_mmc_mci_ram_2_mem_0(U32 addr, const void *ram)
{
// Add offset for the crypted or hidden volume
  if (SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION == sd_FlagHiddenLun_u32)
  {
    addr += GetStartHiddenVolume_u32 ();     // hidden volume
  }
  else
  {
    addr += GetStartCryptedVolume_u32 ();     // crypted volume
  }

  if ((addr > sd_MaxAccessedBlockWriteMin_u32) && (addr < gSdEndOfCard_u32/2))    // Min is only in the low half
  {
    sd_MaxAccessedBlockWriteMin_u32 = addr;
  }
  if (addr > sd_MaxAccessedBlockWriteMin_u32)      // Don't log min values
  {
    if (addr < sd_MaxAccessedBlockWriteMax_u32)    // Real SD Block
    {
      sd_MaxAccessedBlockWriteMax_u32 = addr;
    }
  }

  return sd_mmc_mci_ram_2_mem(0, addr, ram);
}


Ctrl_status sd_mmc_mci_ram_2_mem_1(U32 addr, const void *ram)
{
  return sd_mmc_mci_ram_2_mem(0, addr, ram);    // RB
}


Ctrl_status sd_mmc_mci_multiple_ram_2_mem(U8 slot, U32 addr, const void *ram, U32 nb_sectors)
{
   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   LED_RedGreenOn ();

   if( !sd_mmc_mci_write_open(slot, addr, nb_sectors) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_write_multiple_sector_from_ram(slot, ram, nb_sectors) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_write_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();

   LED_RedGreenOff ();

   return CTRL_GOOD;
}


Ctrl_status sd_mmc_mci_dma_multiple_ram_2_mem(U8 slot, U32 addr, const void *ram, U32 nb_sectors)
{
   if( !sd_mmc_mci_mem_check(slot) )
   {
     Sd_mmc_mci_access_signal_off();
     return CTRL_NO_PRESENT;
   }

   if( !sd_mmc_mci_dma_write_open(slot, addr, ram, nb_sectors) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_dma_write_multiple_sector_from_ram(slot, ram, nb_sectors) )
     return CTRL_FAIL;

   if( !sd_mmc_mci_write_close(slot) )
     return CTRL_FAIL;

   Sd_mmc_mci_access_signal_off();
   return CTRL_GOOD;
}

/*******************************************************************************

  sd_mmc_mci_multiple_ram_2_mem_0

  Write data from ram to sd card for volume 0

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation of dynamic volume sizes

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Ctrl_status sd_mmc_mci_multiple_ram_2_mem_0(U32 addr, const void *ram, U32 nb_sectors)
{
// Add offset for the crypted or hidden volume
  if (SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION == sd_FlagHiddenLun_u32)
  {
    addr += GetStartHiddenVolume_u32 ();     // hidden volume
  }
  else
  {
    addr += GetStartCryptedVolume_u32 ();     // crypted volume
  }

  if ((addr > sd_MaxAccessedBlockWriteMin_u32) && (addr < gSdEndOfCard_u32/2))    // Min is only in the low half
  {
    sd_MaxAccessedBlockWriteMin_u32 = addr;
  }
  if (addr > sd_MaxAccessedBlockWriteMin_u32)      // Don't log min values
  {
    if (addr < sd_MaxAccessedBlockWriteMax_u32)    // Real SD Block
    {
      sd_MaxAccessedBlockWriteMax_u32 = addr;
    }
  }

  return sd_mmc_mci_multiple_ram_2_mem(0, addr, ram, nb_sectors);
}


Ctrl_status sd_mmc_mci_multiple_ram_2_mem_1(U32 addr, const void *ram, U32 nb_sectors)
{
  return sd_mmc_mci_multiple_ram_2_mem(1, addr, ram, nb_sectors);
}


U32 GetSizeUncryptedVolume_u32 (void)
{
  return (gSdEndUncryptedVolume_u32 - gSdStartUncryptedVolume_u32);
}

U32 GetStartUncryptedVolume_u32 (void)
{
  return (gSdStartUncryptedVolume_u32);
}

U32 GetEndUncryptedVolume_u32 (void)
{
  return (gSdEndUncryptedVolume_u32);
}

U32 GetSizeCryptedVolume_u32 (void)
{
  return (gSdEndCryptedVolume_u32 - gSdStartCryptedVolume_u32);

}
U32 GetStartCryptedVolume_u32 (void)
{
  return (gSdStartCryptedVolume_u32);

}
U32 GetEndCryptedVolume_u32 (void)
{
  return (gSdEndCryptedVolume_u32);

}

U32 GetSizeHiddenVolume_u32 (void)
{
  return (gSdEndHiddenVolume_u32 - gSdStartHiddenVolume_u32);
}

U32 GetStartHiddenVolume_u32(void)
{
  return (gSdStartHiddenVolume_u32);
}

U32 GetEndHiddenVolume_u32 (void)
{
  return (gSdEndHiddenVolume_u32);
}

U32 InitSDVolumeSizes_u32 (void)
{
  U32 EndBlockOfSdCard_u32;

  sd_mmc_mci_read_capacity (SD_SLOT,(U32 *)&EndBlockOfSdCard_u32);
  EndBlockOfSdCard_u32 -= 1;                                    // Last block nr

  gSdEndOfCard_u32            = EndBlockOfSdCard_u32;

  gSdStartUncryptedVolume_u32 = 0;
  gSdEndUncryptedVolume_u32   = SD_SIZE_UNCRYPTED_PARITION - 1;

  gSdStartCryptedVolume_u32   = SD_SIZE_UNCRYPTED_PARITION;
  gSdEndCryptedVolume_u32     = EndBlockOfSdCard_u32;

  gSdStartHiddenVolume_u32    = SD_START_HIDDEN_CRYPTED_PARITION + SD_SIZE_UNCRYPTED_PARITION;
  gSdEndHiddenVolume_u32      = EndBlockOfSdCard_u32;

  sd_MaxAccessedBlockReadMin_u32  = 0;
  sd_MaxAccessedBlockReadMax_u32  = EndBlockOfSdCard_u32+1;
  sd_MaxAccessedBlockWriteMin_u32 = 0;
  sd_MaxAccessedBlockWriteMax_u32 = EndBlockOfSdCard_u32+1;

  CI_LocalPrintf ("SD card size %d MB - %d blocks\r\n",(EndBlockOfSdCard_u32+1)/2000,EndBlockOfSdCard_u32+1);

  return (TRUE);
}

void SetHiddenVolumeSizes_u32 (U32 StartBlock_u32, U32 EndBlock_u32)
{
  gSdStartHiddenVolume_u32    = StartBlock_u32;
  gSdEndHiddenVolume_u32      = EndBlock_u32;
}


//! @}

// Debug functions
#ifdef MMC_DEBUG
#endif // MMC_DEBUG


#endif  // ACCESS_MEM_TO_RAM == ENABLED


#endif  // SD_MMC_MCI_0_MEM == ENABLE || SD_MMC_MCI_1_MEM == ENABLE
