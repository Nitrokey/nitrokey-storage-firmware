/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 22.11.2012
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
 * SD_Test.c
 *
 *  Created on: 22.11.2012
 *      Author: RB
 */


#include "global.h"
#include "tools.h"
#include "FreeRTOS.h"
#include "task.h"
#include "malloc.h"
#include "string.h"

#include "conf_explorer.h"
/* FAT includes */
#include "file.h"
#include "navigation.h"
#include "ctrl_access.h"
#include "fsaccess.h"
#include "ctrl_access.h"

#include "conf_sd_mmc_mci.h"
#include "sd_mmc_mci.h"
#include "sd_mmc_mci_mem.h"


#include "time.h"

#include "TIME_MEASURING.h"
#include "Inbetriebnahme.h"
#include "..\FILE_IO\FileAccessInterface.h"
#include "CCID/USART/ISO7816_USART.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"

#include "OTP\report_protocol.h"

#include "SD_Test.h"
#include "LED_test.h"
#include "aes.h"



/*******************************************************************************

 Local defines

*******************************************************************************/

#define SD_SLOT    0

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

void sd_mmc_mci_resources_init(void);

extern U8 sector_buf_0[SD_MMC_SECTOR_SIZE];
extern U8 sector_buf_1[SD_MMC_SECTOR_SIZE];


#define SD_BLOCK_SIZE     SD_MMC_SECTOR_SIZE  // = 512
#define SC_RANDOM_SIZE                    16


/*******************************************************************************

 Local declarations

*******************************************************************************/

U32 SD_ProgressInPercent = 0;

/*******************************************************************************

  SD_PrintSDBlock

  Used the usb buffers

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void SD_PrintSDBlock (u32 Block_u32)
{
  u16 i;

// Read AES SD Block = Stored in sector_buf_0

  SetSdEncryptedCardEnableState (TRUE);

//  sd_mmc_mci_usb_read_10_0 (Block_u32, 1);
  if (FALSE == GetSdEncryptedCardEnableState ())    // Flag for enabling the sd card
  {
    CI_TickLocalPrintf ("SD card not enabled\r\n");
    return;
  }

  sd_mmc_mci_test_unit_ready (SD_SLOT);

  if( !sd_mmc_mci_mem_check(SD_SLOT) )
  {
    CI_TickLocalPrintf ("SD card not ready\r\n");
    return;
  }

  if( !sd_mmc_mci_dma_read_open(SD_SLOT, Block_u32, NULL, 1) )
  {
    CI_TickLocalPrintf ("SD block open fails\r\n");
    return;
  }

  dma_mci_2_ram(&sector_buf_0, SD_MMC_SECTOR_SIZE,Block_u32);
/*
  if( !sd_mmc_mci_read_multiple_sector(SD_SLOT, 1, sector_buf_0) )
  {
    CI_TickLocalPrintf ("SD block read fails\r\n");
    return;
  }
*/
  if( !sd_mmc_mci_read_close(SD_SLOT) )
  {
    CI_TickLocalPrintf ("SD block close fails\r\n");
    return;
  }

// Print it
  CI_TickLocalPrintf ("\r\nSD Block %d = \r\n",Block_u32);
  for (i=0;i<SD_BLOCK_SIZE/SC_RANDOM_SIZE;i++)
  {
    CI_LocalPrintf ("0x%03x : ",i*SC_RANDOM_SIZE);
    AsciiHexPrint (SC_RANDOM_SIZE,&sector_buf_0[i*SC_RANDOM_SIZE]);
    CI_LocalPrintf ("\r\n");
  }
}

/*******************************************************************************

  SD_WriteMultipleBlocksWithRandoms

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void SD_WriteMultipleBlocksWithRandoms (u8 *Addr_u8,u32 Block_u32,u32 Count_u32,u8 CryptionFlag_u8)
{
  u32 i;
#ifdef TIME_MEASURING_ENABLE
  u64 Runtime_u64;
#endif

  CI_TickLocalPrintf ("SD_WriteMultipleBlocksWithRandoms - Addr %x Block %d Count %d \r\n",Addr_u8,Block_u32,Count_u32);

// Write SD Block
  SetSdEncryptedCardEnableState (TRUE);

  if (FALSE == GetSdEncryptedCardEnableState ())    // Flag for enabling the sd card
  {
    CI_TickLocalPrintf ("SD card not enabled\r\n");
    return;
  }

  sd_mmc_mci_test_unit_ready (SD_SLOT);

  if( !sd_mmc_mci_mem_check(SD_SLOT) )
  {
    CI_TickLocalPrintf ("SD card not ready\r\n");
    return;
  }

#ifdef TIME_MEASURING_ENABLE
  TIME_MEASURING_Start (TIME_MEASURING_TIME_15);
#endif

//
  if (TRUE == CryptionFlag_u8)
  {
    AES_SetNewStorageKey (Addr_u8);   // Set key from input data
  }

  SD_ProgressInPercent = 0;

// Write the data
  for (i=0;i<Count_u32;i++)
  {
//    Addr_u8[0] = (u8)i;
    if(FALSE == sd_mmc_mci_dma_write_open(SD_SLOT, Block_u32, Addr_u8, 1) )
    {
      CI_TickLocalPrintf ("SD block open fails. Block %d\r\n",i);
      return;
    }
    if (TRUE == CryptionFlag_u8)
    {
      // Use the crypted function to randomise the data
      dma_ram_2_mci_crypted (Addr_u8, SD_MMC_SECTOR_SIZE,Block_u32+i);
    }
    else
    {
      dma_ram_2_mci(Addr_u8, SD_MMC_SECTOR_SIZE,Block_u32+i);
    }

    if (i % 1000 == 0)
    {
      // Change the key every 1000 blocks
      if (TRUE == CryptionFlag_u8)
      {
        AES_SetNewStorageKey ((u8*)&Addr_u8[i%(256-16)]);   // Get key from input data
      }

      SD_ProgressInPercent = i / (Count_u32 / 100);
      if (100 < SD_ProgressInPercent)
      {
        SD_ProgressInPercent = 100;
      }
      LED_GreenToggle ();
    }

    if (0 == i % (Count_u32/100))
    {
      UpdateStick20Command (OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR,(u8)(i / (Count_u32 /100)));
    }

// Generate new random chars
    if (0 == i % (Count_u32/127))
    {
      SD_GetRandomBlock (Addr_u8);
    }

#ifdef TIME_MEASURING_ENABLE
    if (i % 10000 == 0)
    {
      if (0 != i)
      {
        TIME_MEASURING_Stop (TIME_MEASURING_TIME_15);
        Runtime_u64 = TIME_MEASURING_GetLastRuntimeU64 (TIME_MEASURING_TIME_15);
        CI_LocalPrintf ("%8d Blocks : SD runtime %ld sec = %6.0f kb/sec\n\r",i,(u32)(Runtime_u64/(u64)TIME_MEASURING_TICKS_IN_USEC/(u64)1000000L),(float)((u64)i*(u64)SD_MMC_SECTOR_SIZE) /(float)(Runtime_u64/(u64)TIME_MEASURING_TICKS_IN_USEC)*1000.0 );
      }
    }
#endif
  }

  LED_GreenOff ();

  if( !sd_mmc_mci_read_close(SD_SLOT) )
  {
    CI_TickLocalPrintf ("SD block close fails\r\n");
    return;
  }
#ifdef TIME_MEASURING_ENABLE
  TIME_MEASURING_Stop (TIME_MEASURING_TIME_15);
  Runtime_u64 = TIME_MEASURING_GetLastRuntimeU64 (TIME_MEASURING_TIME_15);
  CI_LocalPrintf ("SD runtime %ld sec = %6.0f kb/sec\n\r",(u32)(Runtime_u64/(u64)TIME_MEASURING_TICKS_IN_USEC/(u64)1000000L),(float)((u64)Count_u32*(u64)SD_MMC_SECTOR_SIZE) /(float)(Runtime_u64/(u64)TIME_MEASURING_TICKS_IN_USEC)*1000.0 );
#endif
}

/*******************************************************************************

  SD_FillBlocks

  Used the usb buffers

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void SD_FillBlocks (u8 Pattern_u8,u32 Block_u32,u32 Count_u32)
{
  u32 i;

  for (i=0;i<SD_BLOCK_SIZE;i++)
  {
    sector_buf_0[i] = Pattern_u8;
  }
  SD_WriteMultipleBlocksWithRandoms (sector_buf_0,Block_u32,Count_u32,0);
}

/*******************************************************************************

  SD_WriteBlock

  Used the usb buffers

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void SD_WriteBlock (u32 Block_u32)
{
//  u8  WriteBlock_u8[SD_BLOCK_SIZE];
  u16 i;

  CI_TickLocalPrintf ("Build SD start block\r\n");
  for (i=0;i<SD_BLOCK_SIZE/SC_RANDOM_SIZE;i++)
  {
    CI_TickLocalPrintf ("GetRandomNumber %d\n\r",i);
    if (FALSE == GetRandomNumber_u32 (SC_RANDOM_SIZE,&sector_buf_0[i*SC_RANDOM_SIZE]))
    {
      CI_TickLocalPrintf ("fails\n\r");
      return;
    }
  }

  CI_TickLocalPrintf ("\r\nSD Block = \r\n");
  for (i=0;i<SD_BLOCK_SIZE/SC_RANDOM_SIZE;i++)
  {
    CI_LocalPrintf ("0x%03x : ",i*SC_RANDOM_SIZE);
    HexPrint (SC_RANDOM_SIZE,&sector_buf_0[i*SC_RANDOM_SIZE]);
    CI_LocalPrintf ("\r\n");
  }

// Copy USB Buffer 0 to 1
  memcpy (sector_buf_1,sector_buf_0,SD_MMC_SECTOR_SIZE);

// Write SD Block
  SetSdEncryptedCardEnableState (TRUE);

  if (FALSE == GetSdEncryptedCardEnableState ())    // Flag for enabling the sd card
  {
    CI_TickLocalPrintf ("SD card not enabled\r\n");
    return;
  }

  sd_mmc_mci_test_unit_ready (SD_SLOT);

  if( !sd_mmc_mci_mem_check(SD_SLOT) )
  {
    CI_TickLocalPrintf ("SD card not ready\r\n");
    return;
  }

  if( !sd_mmc_mci_dma_write_open(SD_SLOT, Block_u32, sector_buf_0, 1) )
  {
    CI_TickLocalPrintf ("SD block open fails\r\n");
    return;
  }

  // Use the crypted function to randomise the data
//  dma_ram_2_mci_crypted (sector_buf_0, SD_MMC_SECTOR_SIZE,Block_u32);

  dma_ram_2_mci(&sector_buf_0, SD_MMC_SECTOR_SIZE,Block_u32);

  if( !sd_mmc_mci_read_close(SD_SLOT) )
  {
    CI_TickLocalPrintf ("SD block close fails\r\n");
    return;
  }
}

/*******************************************************************************

  SD_GetRandomBlock

  Changes
  Date      Author          Info
  03.06.14  RB              Creation of function

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 RandomBuffer_u8[SD_BLOCK_SIZE];

u8 SD_GetRandomBlock (u8 *RandomData_u8)
{
  u32 i;

  for (i=0;i<SD_BLOCK_SIZE/SC_RANDOM_SIZE;i++)
  {
  //    CI_TickLocalPrintf ("GetRandomNumber %d\n\r",i);
    if (FALSE == GetRandomNumber_u32 (SC_RANDOM_SIZE,&RandomData_u8[i*SC_RANDOM_SIZE]))
    {
      CI_TickLocalPrintf ("SD_GetRandomBlock: fails\n\r");
      return (FALSE);
    }
  }
/*
  CI_TickLocalPrintf ("\r\nRandom Block = \r\n");
  for (i=0;i<SD_BLOCK_SIZE/SC_RANDOM_SIZE;i++)
  {
    CI_LocalPrintf ("0x%03x : ",i*SC_RANDOM_SIZE);
    HexPrint (SC_RANDOM_SIZE,&RandomData_u8[i*SC_RANDOM_SIZE]);
    CI_LocalPrintf ("\r\n");
  }
*/
  return (TRUE);
}

/*******************************************************************************

  SD_WriteBlocks

  Changes
  Date      Author          Info
  03.06.14  RB              Own buffer for random chars

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

extern int sd_mmc_mci_test_unit_only_local_access;
extern U32 Check_Sd_mmc_mci_access_signal_on (void);

void SD_WriteBlocks (u32 Block_u32,u32 Count_u32,u8 CryptionFlag_u8)
{
  u16 i;

  CI_TickLocalPrintf ("Build SD start block -");

  SD_GetRandomBlock (RandomBuffer_u8);

  CI_TickLocalPrintf (" ok\n\r");

// Lock SD card for local access
  sd_mmc_mci_test_unit_only_local_access = TRUE;

  // Wait until no access
  while (TRUE == Check_Sd_mmc_mci_access_signal_on ())
  {
  }
 // Write Data
  SD_WriteMultipleBlocksWithRandoms (RandomBuffer_u8,Block_u32,Count_u32,CryptionFlag_u8);

  sd_mmc_mci_test_unit_only_local_access = FALSE;
}

/*******************************************************************************

  SD_SecureEraseHoleCard

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 SD_SecureEraseHoleCard (void)
{
  u32         Blockcount_u32;

  sd_mmc_mci_read_capacity (SD_SLOT,(U32 *)&Blockcount_u32);

//  Blockcount_u32 = 1000000; // for testing

  CI_LocalPrintf ("Erase SD: %d blocks\r\n",Blockcount_u32);

  SD_WriteBlocks (0,Blockcount_u32,1);
  return (TRUE);
}

/*******************************************************************************

  SD_SecureEraseCryptedVolume

  Write random chars in volume 1, the unencrypted volume is untouched

  Changes
  Date      Author          Info
  07.02.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 SD_SecureEraseCryptedVolume (void)
{
  u32         Blockcount_u32;

  sd_mmc_mci_read_capacity (SD_SLOT,(U32 *)&Blockcount_u32);

  CI_LocalPrintf ("Erase SD: blocks from %d to %d\r\n",GetStartCryptedVolume_u32 (),Blockcount_u32);

  SD_WriteBlocks (GetStartCryptedVolume_u32 (),Blockcount_u32,1);
  return (TRUE);
}

/*******************************************************************************

  IBN_SD_Tests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_SD_Tests (u8 nParamsGet_u8,u8 CMD_u8,u32 Param_u32,u32 Param_1_u32,u32 Param_2_u32)
{
//  volatile avr32_mci_t *mci = &AVR32_MCI;
  Ctrl_status Ret_e;
  u32         Runtime_u32;
  u32         Blockcount_u32;
  const cid_t *cid;

  if (0 == nParamsGet_u8)
  {
    CI_LocalPrintf ("SD test functions\r\n");
    CI_LocalPrintf ("\r\n");
    CI_LocalPrintf ("0   Show SD stat\r\n");
    CI_LocalPrintf ("1   Init SD driver\r\n");
    CI_LocalPrintf ("2 X Test unit ready [X=slot]\r\n");
    CI_LocalPrintf ("3 X 0 = Stop SD card, 1 = Start\r\n");
    CI_LocalPrintf ("4 X Read SD [X=Block]\r\n");
    CI_LocalPrintf ("5 X Write SD [X=Block]\r\n");
    CI_LocalPrintf ("6 X Write SD start = 0[X=Blockcount]\r\n");
    CI_LocalPrintf ("7 X fill SD start = 0[X=fill char]\r\n");
    CI_LocalPrintf ("8   Show SD block count\r\n");
    CI_LocalPrintf ("9   Erase encrypted volume with random numbers via AES\r\n");
    CI_LocalPrintf ("10  Lock USB access\r\n");
    CI_LocalPrintf ("11  Unlock USB access\r\n");
    CI_LocalPrintf ("14  Get CID from sd card\r\n");
    CI_LocalPrintf ("\r\n");
    return;
  }

  switch (CMD_u8)
  {
    case 0 :
      CI_LocalPrintf ("MCI SR status register - deactivated\r\n\r\n");
/*
       CI_LocalPrintf ("MCI SR status register\r\n\r\n");
       CI_LocalPrintf ("unre        %x\r\n",mci->SR.unre        );
       CI_LocalPrintf ("ovre        %x\r\n",mci->SR.ovre        );
       CI_LocalPrintf ("xfrdone     %x\r\n",mci->SR.xfrdone     );
       CI_LocalPrintf ("fifoempty   %x\r\n",mci->SR.fifoempty   );
       CI_LocalPrintf ("dmadone     %x\r\n",mci->SR.dmadone     );
       CI_LocalPrintf ("blkovre     %x\r\n",mci->SR.blkovre     );
       CI_LocalPrintf ("cstoe       %x\r\n",mci->SR.cstoe       );
       CI_LocalPrintf ("dtoe        %x\r\n",mci->SR.dtoe        );
       CI_LocalPrintf ("dcrce       %x\r\n",mci->SR.dcrce       );
       CI_LocalPrintf ("rtoe        %x\r\n",mci->SR.rtoe        );
       CI_LocalPrintf ("rende       %x\r\n",mci->SR.rende       );
       CI_LocalPrintf ("rcrce       %x\r\n",mci->SR.rcrce       );
       CI_LocalPrintf ("rdire       %x\r\n",mci->SR.rdire       );
       CI_LocalPrintf ("rinde       %x\r\n",mci->SR.rinde       );
       CI_LocalPrintf ("txbufe      %x\r\n",mci->SR.txbufe      );
       CI_LocalPrintf ("rxbuff      %x\r\n",mci->SR.rxbuff      );
       CI_LocalPrintf ("csrcv       %x\r\n",mci->SR.csrcv       );
       CI_LocalPrintf ("sdiowait    %x\r\n",mci->SR.sdiowait    );
       CI_LocalPrintf ("sdioirqd    %x\r\n",mci->SR.sdioirqd    );
       CI_LocalPrintf ("sdioirqc    %x\r\n",mci->SR.sdioirqc    );
       CI_LocalPrintf ("sdioirqb    %x\r\n",mci->SR.sdioirqb    );
       CI_LocalPrintf ("sdioirqa    %x\r\n",mci->SR.sdioirqa    );
       CI_LocalPrintf ("endtx       %x\r\n",mci->SR.endtx       );
       CI_LocalPrintf ("endrx       %x\r\n",mci->SR.endrx       );
       CI_LocalPrintf ("notbusy     %x\r\n",mci->SR.notbusy     );
       CI_LocalPrintf ("dtip        %x\r\n",mci->SR.dtip        );
       CI_LocalPrintf ("blke        %x\r\n",mci->SR.blke        );
       CI_LocalPrintf ("txrdy       %x\r\n",mci->SR.txrdy       );
       CI_LocalPrintf ("rxrdy       %x\r\n",mci->SR.rxrdy       );
       CI_LocalPrintf ("cmdrdy      %x\r\n",mci->SR.cmdrdy      );
*/
       break;

    case 1:
      sd_mmc_mci_resources_init();
      break;

    case 2:
#ifdef TIME_MEASURING_ENABLE
      TIME_MEASURING_Start (TIME_MEASURING_TIME_FOR_ALL);
#endif
      Ret_e = sd_mmc_mci_test_unit_ready(Param_u32);
#ifdef TIME_MEASURING_ENABLE
      Runtime_u32 = TIME_MEASURING_Stop (TIME_MEASURING_TIME_FOR_ALL);
#endif
      CI_LocalPrintf ("TEST_UNIT_READY  slot %d = %d - %s\r\n",Param_u32,Ret_e,IBN_SdRetValueString(Ret_e));
      CI_LocalPrintf ("Function runtime %d usec\r\n",Runtime_u32);
      break;

    case 3 :
      switch (Param_u32)
      {
        case 0 :
          SetSdEncryptedCardEnableState (FALSE);
          CI_LocalPrintf ("Crypted volume disabled\r\n");
          break;
        case 1 :
          SetSdEncryptedCardEnableState (TRUE);
          CI_LocalPrintf ("Crypted volume enabled\r\n");
          break;
      }
      break;

      case 4:
        SD_PrintSDBlock (Param_u32);
        break;

      case 5 :
        CI_LocalPrintf ("SD - Write block with random AES\r\n");
        SD_WriteBlock (Param_u32);
        break;
      case 6 :
        CI_LocalPrintf ("SD - Write %d blocks - Crypted %d\r\n",Param_u32,Param_1_u32);
        SD_WriteBlocks (0,Param_u32,Param_1_u32);
        break;
      case 7 :
        CI_LocalPrintf ("SD - Fill block with 0\r\n");
        SD_FillBlocks (0,0,Param_u32);
        break;
      case 8 :
        sd_mmc_mci_read_capacity (SD_SLOT,(unsigned long int *)&Blockcount_u32);
        CI_LocalPrintf ("SD - %d blocks a 512 byte = %d MB\r\n",Blockcount_u32,Blockcount_u32/2000);
        break;
      case 9 :
        SD_SecureEraseCryptedVolume ();
        break;
      case 10 :
        SetSdUncryptedCardEnableState (FALSE);      // Disable access
        SetSdEncryptedCardEnableState (FALSE);
        vTaskDelay (1000);
//        sd_mmc_mci_test_unit_only_local_access = TRUE;
        break;
      case 11 :
//        sd_mmc_mci_test_unit_only_local_access = FALSE;
        SetSdEncryptedCardEnableState (TRUE);
        SetSdUncryptedCardEnableState (TRUE);       // Enable access
        break;

      case 12 :
        sd_mmc_mci_test_unit_only_local_access = TRUE;
        break;
      case 13 :
        sd_mmc_mci_test_unit_only_local_access = FALSE;
        break;
      case 14 :
        cid = GetSdCidInfo ();
        CI_LocalPrintf ("SD - CID info\r\n");
        CI_LocalPrintf ("Manufacturer ID     0x%04x\r\n",cid->mid);
        CI_LocalPrintf ("Product name        %c%c%c%c%c\r\n",cid->pnmh,(cid->pnml>>24)&0xff,(cid->pnml>>16)&0xff,(cid->pnml>>8)&0xff,(cid->pnml>>0)&0xff);
        CI_LocalPrintf ("Serial number       0x%06x%02x\r\n",cid->psnh,cid->psnl);
        CI_LocalPrintf ("Manufacturing date  %d %02d\r\n",2000+(cid->mdt/16),cid->mdt % 0x0f);

        break;
  }
}

