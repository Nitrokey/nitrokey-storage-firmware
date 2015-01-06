/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief MCI driver for AVR32 UC3.
 *
 * This file contains basic functions for the AVR32 MCI, with support for all
 * modes, settings and clock speeds.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a MCI module can be used.
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



#include "mci.h"

//#include "mci.h"

extern  volatile long xTickCount            ;

//#define MCI_DEBUG

#ifdef  MCI_DEBUG
  #include "global.h"
  #define CI_LocalPrintf  CI_LocalPrintfNoDelay

  int CI_LocalPrintf (char *szFormat,...);
  int CI_TickLocalPrintf (char *szFormat,...);


char *MCI_PrintCMD (int Cmd)
{
  switch (Cmd)
  {
    case SD_MMC_GO_IDLE_STATE_CMD                 : return ("SD_MMC_GO_IDLE_STATE_CMD                ");
    case SD_MMC_INIT_STATE_CMD                    : return ("SD_MMC_INIT_STATE_CMD                   ");
    case SD_MMC_MMC_GO_IDLE_STATE_CMD             : return ("SD_MMC_MMC_GO_IDLE_STATE_CMD            ");
    case SD_MMC_MMC_SEND_OP_COND_CMD              : return ("SD_MMC_MMC_SEND_OP_COND_CMD             ");
    case SD_MMC_ALL_SEND_CID_CMD                  : return ("SD_MMC_ALL_SEND_CID_CMD                 ");
    case SD_MMC_MMC_ALL_SEND_CID_CMD              : return ("SD_MMC_MMC_ALL_SEND_CID_CMD             ");
    case SD_MMC_SET_RELATIVE_ADDR_CMD             : return ("SD_MMC_SET_RELATIVE_ADDR_CMD            ");
    case SD_MMC_MMC_SET_RELATIVE_ADDR_CMD         : return ("SD_MMC_MMC_SET_RELATIVE_ADDR_CMD        ");
//    case MCI_SET_DSR_CMD                          : return ("MCI_SET_DSR_CMD                         ");
    case SD_MMC_SEL_DESEL_CARD_CMD                : return ("SD_MMC_SEL_DESEL_CARD_CMD               ");
    case SD_MMC_SEND_EXT_CSD_CMD                  : return ("SD_MMC_SEND_EXT_CSD_CMD                 ");
    case SD_MMC_SEND_CSD_CMD                      : return ("SD_MMC_SEND_CSD_CMD                     ");
    case SD_MMC_SEND_CID_CMD                      : return ("SD_MMC_SEND_CID_CMD                     ");
    case SD_MMC_MMC_READ_DAT_UNTIL_STOP_CMD       : return ("SD_MMC_MMC_READ_DAT_UNTIL_STOP_CMD      ");
    case SD_MMC_STOP_READ_TRANSMISSION_CMD        : return ("SD_MMC_STOP_READ_TRANSMISSION_CMD       ");
    case SD_MMC_STOP_WRITE_TRANSMISSION_CMD       : return ("SD_MMC_STOP_WRITE_TRANSMISSION_CMD      ");
    case SD_MMC_STOP_TRANSMISSION_SYNC_CMD        : return ("SD_MMC_STOP_TRANSMISSION_SYNC_CMD       ");
//    case SD_MMC_SEND_STATUS_CMD                   : return ("SD_MMC_SEND_STATUS_CMD                  ");
    case SD_MMC_GO_INACTIVE_STATE_CMD             : return ("SD_MMC_GO_INACTIVE_STATE_CMD            ");
    case SD_MMC_SET_BLOCKLEN_CMD                  : return ("SD_MMC_SET_BLOCKLEN_CMD                 ");
    case SD_MMC_READ_SINGLE_BLOCK_CMD             : return ("SD_MMC_READ_SINGLE_BLOCK_CMD            ");
    case SD_MMC_READ_MULTIPLE_BLOCK_CMD           : return ("SD_MMC_READ_MULTIPLE_BLOCK_CMD          ");
    case SD_MMC_MMC_WRITE_DAT_UNTIL_STOP_CMD      : return ("SD_MMC_MMC_WRITE_DAT_UNTIL_STOP_CMD     ");
    case SD_MMC_WRITE_BLOCK_CMD                   : return ("SD_MMC_WRITE_BLOCK_CMD                  ");
    case SD_MMC_WRITE_MULTIPLE_BLOCK_CMD          : return ("SD_MMC_WRITE_MULTIPLE_BLOCK_CMD         ");
    case SD_MMC_PROGRAM_CSD_CMD                   : return ("SD_MMC_PROGRAM_CSD_CMD                  ");
    case SD_MMC_SET_WRITE_PROT_CMD                : return ("SD_MMC_SET_WRITE_PROT_CMD               ");
    case SD_MMC_CLR_WRITE_PROT_CMD                : return ("SD_MMC_CLR_WRITE_PROT_CMD               ");
    case SD_MMC_SEND_WRITE_PROT_CMD               : return ("SD_MMC_SEND_WRITE_PROT_CMD              ");
    case SD_MMC_TAG_SECTOR_START_CMD              : return ("SD_MMC_TAG_SECTOR_START_CMD             ");
    case SD_MMC_TAG_SECTOR_END_CMD                : return ("SD_MMC_TAG_SECTOR_END_CMD               ");
    case SD_MMC_MMC_UNTAG_SECTOR_CMD              : return ("SD_MMC_MMC_UNTAG_SECTOR_CMD             ");
    case SD_MMC_MMC_TAG_ERASE_GROUP_START_CMD     : return ("SD_MMC_MMC_TAG_ERASE_GROUP_START_CMD    ");
    case SD_MMC_MMC_TAG_ERASE_GROUP_END_CMD       : return ("SD_MMC_MMC_TAG_ERASE_GROUP_END_CMD      ");
    case SD_MMC_MMC_UNTAG_ERASE_GROUP_CMD         : return ("SD_MMC_MMC_UNTAG_ERASE_GROUP_CMD        ");
    case SD_MMC_ERASE_CMD                         : return ("SD_MMC_ERASE_CMD                        ");
//    case SD_MMC_LOCK_UNLOCK                       : return ("SD_MMC_LOCK_UNLOCK                      ");
    case SD_MMC_SD_SEND_IF_COND_CMD               : return ("SD_MMC_SD_SEND_IF_COND_CMD              ");
    case SD_MMC_APP_CMD                           : return ("SD_MMC_APP_CMD                          ");
    case SD_MMC_GEN_CMD                           : return ("SD_MMC_GEN_CMD                          ");
    case MMC_SWITCH_CMD                           : return ("MMC_SWITCH_CMD                          ");
    case SD_SWITCH_FUNC                           : return ("SD_SWITCH_FUNC                          ");
    case SD_MMC_SDCARD_SET_BUS_WIDTH_CMD          : return ("SD_MMC_SDCARD_SET_BUS_WIDTH_CMD         ");
//    case SD_MMC_SDCARD_STATUS_CMD                 : return ("SD_MMC_SDCARD_STATUS_CMD                ");
    case SD_MMC_SDCARD_SEND_NUM_WR_BLOCKS_CMD     : return ("SD_MMC_SDCARD_SEND_NUM_WR_BLOCKS_CMD    ");
    case SD_MMC_SDCARD_SET_WR_BLK_ERASE_COUNT_CMD : return ("SD_MMC_SDCARD_SET_WR_BLK_ERASE_COUNT_CMD");
    case SD_MMC_SDCARD_APP_OP_COND_CMD            : return ("SD_MMC_SDCARD_APP_OP_COND_CMD           ");
    case SD_MMC_SDCARD_SET_CLR_CARD_DETECT_CMD    : return ("SD_MMC_SDCARD_SET_CLR_CARD_DETECT_CMD   ");
    case SD_MMC_SDCARD_SEND_SCR_CMD               : return ("SD_MMC_SDCARD_SEND_SCR_CMD              ");
//    case SD_MMC_SDCARD_APP_ALL_CMD                : return ("SD_MMC_SDCARD_APP_ALL_CMD               ");
    case SD_MMC_FAST_IO_CMD                       : return ("SD_MMC_FAST_IO_CMD                      ");
    case SD_MMC_GO_IRQ_STATE_CMD                  : return ("SD_MMC_GO_IRQ_STATE_CMD                 ");
    default : return ("CMD name not found");
  }
}



#else
  #define  CI_LocalPrintf(...)
#endif


//! Global Error Mask
#define MCI_SR_ERROR		        ( AVR32_MCI_SR_UNRE_MASK  |\
                                  AVR32_MCI_SR_OVRE_MASK  |\
                                  AVR32_MCI_SR_DTOE_MASK  |\
                                  AVR32_MCI_SR_DCRCE_MASK |\
                                  AVR32_MCI_SR_RTOE_MASK  |\
                                  AVR32_MCI_SR_RENDE_MASK |\
                                  AVR32_MCI_SR_RCRCE_MASK |\
                                  AVR32_MCI_SR_RDIRE_MASK |\
                                  AVR32_MCI_SR_RINDE_MASK)


int shadow_sr=0;

void mci_reset(volatile avr32_mci_t *mci)
{
  mci->cr = (1<<AVR32_MCI_CR_SWRST);
}

void mci_disable(volatile avr32_mci_t *mci)
{
  // Disable the MCI
  mci->cr = (1<<AVR32_MCI_CR_MCIDIS) | (1<<AVR32_MCI_CR_PWSDIS);
}

void mci_enable(volatile avr32_mci_t *mci)
{
  // Enable the MCI
  mci->cr = (1<<AVR32_MCI_CR_MCIEN) | (1<<AVR32_MCI_CR_PWSEN);
}

void mci_stop(volatile avr32_mci_t *mci)
{
  // Disable all the interrupts
  mci->idr = 0xffffffff;

  // Reset the MCI
  mci_reset(mci);

  // Disable the MCI
  mci_disable(mci);
}

void mci_set_speed(volatile avr32_mci_t *mci,
                   long pbb_hz,
                   long card_speed)
{
  U32 mci_mode_register;
  unsigned short clkdiv;
  unsigned short rest;
  union u_cfg{
    unsigned long     cfg;
    avr32_mci_cfg_t   CFG;
  };
  union u_cfg val;

  // Get the Mode Register
  mci_mode_register = mci->mr;

  if (card_speed > AVR32_MCI_HSDIS_MAX_FREQ)
  { // Use of the High Speed mode of the MCI macro.
    val.cfg = mci->cfg;
    val.CFG.hsmode = 1;
    mci->cfg = val.cfg;
  }

  // Multimedia Card Interface clock (MCCK or MCI_CK) is Master Clock (MCK)
  // divided by (2*(CLKDIV+1))
  if (card_speed > 0)
  {
    clkdiv = pbb_hz / (card_speed * 2);
    rest   = pbb_hz % (card_speed * 2);
    if (rest)
    { // Ensure that the card_speed can not be higher than expected.
      clkdiv += 1;
    }

    if (clkdiv > 0)
    {
      clkdiv -= 1;
    }
  }
  else
  {
    clkdiv = 0;
  }

  // Write new configuration
  mci_mode_register &= ~AVR32_MCI_MR_CLKDIV_MASK; // Clear previous value
  mci_mode_register |= clkdiv; // Set the new one
  mci->mr = mci_mode_register;
}

int mci_init(volatile avr32_mci_t *mci,
              const mci_options_t *mci_opt,
              long pbb_hz)
{
  if (mci_opt->card_slot>MCI_LAST_SLOTS)
    return MCI_INVALID_INPUT;

  // Initialize all bits of the shadow status register.
  shadow_sr=0;

  // Reset the MCI
  mci_reset(mci);

  // Disable the MCI
  mci_disable(mci);

  // Disable all the interrupts
  mci->idr = 0xffffffff;

  // Setup configuration register
  mci->cfg = 0;

  // Clear Mode register
  mci->mr = 0;

  // Set the Data Timeout Register to 1 Mega Cycles
  mci->dtor = (MCI_DEFAULT_DTOREG);

  // Set the Mode Register
  mci_set_speed(mci, pbb_hz, mci_opt->card_speed);
  mci->mr |= ((MCI_DEFAULT_PWSDIV<<AVR32_MCI_MR_PWSDIV) | AVR32_MCI_MR_RDPROOF_MASK | AVR32_MCI_MR_WRPROOF_MASK);

  // Set the SD/MMC Card Register
  mci->sdcr = (mci_opt->bus_width>>AVR32_MCI_SDCR_SDCBUS_OFFSET)|(mci_opt->card_slot>>AVR32_MCI_SDCR_SDCSEL_OFFSET);

  // Enable the MCI and the Power Saving
  mci_enable(mci);

  return MCI_SUCCESS;
}

void mci_set_block_size(volatile avr32_mci_t *mci,
                        unsigned short length)
{
  U32 mci_mode_register;

  mci_mode_register = mci->mr;
  mci_mode_register &= ~AVR32_MCI_MR_BLKLEN_MASK; // Clear previous BLKLEN
  mci_mode_register |= (length<<AVR32_MCI_MR_BLKLEN_OFFSET); // Set the new value

  mci->mr = mci_mode_register;
}

void mci_set_block_count(volatile avr32_mci_t *mci,
                         unsigned short cnt)
{
  union u_blkr{
    unsigned long     blkr;
    avr32_mci_blkr_t  BLKR;
  };
  union u_blkr val;

  val.blkr = mci->blkr;
  val.BLKR.bcnt = cnt;
  mci->blkr = val.blkr;
}

int mci_send_cmd(volatile avr32_mci_t *mci,
                 unsigned int cmd,
                 unsigned int arg)
{
  unsigned int	error;

  CI_LocalPrintf ("%d MCI_CMD:CMD 0x%04x ARG 0x%08x-%s\r\n",xTickCount,cmd,arg,MCI_PrintCMD(cmd));

//  if (0xfffe4000 != mci) CI_LocalPrintf ("1");

  // Send the command
  mci->argr = arg;
  mci->cmdr = cmd;

  // wait for CMDRDY Status flag to read the response
  while( !(mci_cmd_ready(mci)) );

  // Test error  ==> if crc error and response R3 ==> don't check error
  error = mci_get_sr(mci) & MCI_SR_ERROR;
//  if (0xfffe4000 != mci) CI_LocalPrintf ("2");
  if(error != 0 )
  {
    // if the command is SEND_OP_COND the CRC error flag is always present (cf : R3 response)
    if ( (cmd != SD_MMC_SDCARD_APP_OP_COND_CMD) && (cmd != SD_MMC_MMC_SEND_OP_COND_CMD))
    {
      if( error != AVR32_MCI_SR_RTOE_MASK )
      {
        // filter RTOE error which happens when using the HS mode
        CI_LocalPrintf ("%9d MCI_SEND_CMD: ERROR=0x%x\r\n",xTickCount,error);
        return error;
      }
    }
    else
    {
      if (error != AVR32_MCI_SR_RCRCE_MASK)
      {
        CI_LocalPrintf ("%9d MCI_SEND_CMD: ERROR=0x%x\r\n",xTickCount,error);
        return error;
      }
    }
  }
  return MCI_SUCCESS;

}

int mci_set_bus_size(volatile avr32_mci_t *mci,
                     unsigned char busWidth)
{
  U32 mci_sdcr_register;

  if (busWidth > MCI_BUS_SIZE_8_BIT)
    return MCI_INVALID_INPUT;

  mci_sdcr_register = mci->sdcr;
  mci_sdcr_register &= ~AVR32_MCI_SDCR_SDCBUS_MASK; // Clear previous buswidth
  mci_sdcr_register |= (busWidth<<AVR32_MCI_SDCR_SDCBUS_OFFSET);
  mci->sdcr = mci_sdcr_register;

  return MCI_SUCCESS;
}


unsigned int mci_read_response(volatile avr32_mci_t *mci)
{
  return mci->rspr0;
}

void mci_wait_busy_signal(volatile avr32_mci_t *mci)
{
  int i;
  for (i=0;i<1000000;i++)
  {
    if((mci_get_sr(mci)&AVR32_MCI_SR_NOTBUSY_MASK))// todo deadlock
    {
      return;
    }
  }
  return;
}

int mci_select_card(volatile avr32_mci_t *mci,
                    unsigned char card_slot,
                    unsigned char bus_width)
{
  if (card_slot > MCI_LAST_SLOTS)
    return MCI_INVALID_INPUT;

  // Select the card slot and bus width
  mci->sdcr = (bus_width<<AVR32_MCI_SDCR_SDCBUS_OFFSET) | (card_slot<<AVR32_MCI_SDCR_SDCSEL_OFFSET);

  return MCI_SUCCESS;
}
