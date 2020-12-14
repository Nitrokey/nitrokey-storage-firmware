/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/* This file is prepared for Doxygen automatic documentation generation. */
/* ! \file ********************************************************************* \brief SD/MMC card driver using MCI interface. - Compiler: IAR
   EWAVR32 and GNU GCC for AVR32 - Supported devices: All AVR32 devices with an MCI module can be used. - AppNote: \author Atmel Corporation:
   http://www.atmel.com \n Support and FAQ: http://support.atmel.no/ **************************************************************************** */

/* Copyright (c) 2009 Atmel Corporation. All rights reserved. Redistribution and use in source and binary forms, with or without modification, are
   permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimer. 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
   the following disclaimer in the documentation and/or other materials provided with the distribution. 3. The name of Atmel may not be used to
   endorse or promote products derived from this software without specific prior written permission. 4. This software may only be redistributed and
   used in connection with an Atmel AVR product. THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE EXPRESSLY AND SPECIFICALLY
   DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE */
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



// _____ I N C L U D E S ____________________________________________________

#include "conf_access.h"
#include <stdio.h>


#if SD_MMC_MCI_0_MEM == ENABLE || SD_MMC_MCI_1_MEM == ENABLE

#include "gpio.h"
#include "mci.h"
#include "conf_sd_mmc_mci.h"
#include "sd_mmc_mci.h"
#include "sd_mmc_mci_mem.h"
#include "cycle_counter.h"

#include "global.h"
#include "tools.h"
#include "TIME_MEASURING.h"
#include "LED_test.h"

#include "HighLevelFunctions/FlashStorage.h"

#include "portmacro.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

extern volatile portTickType xTickCount;
extern volatile xSemaphoreHandle AES_semphr;     // AES semaphore

// _____ P R I V A T E D E C L A R A T I O N S ____________________________

// Variable to manage the specification of the card
Bool sd_mmc_mci_init_done[MCI_NR_SLOTS] = { FALSE, FALSE };

U8 g_u8_card_type[MCI_NR_SLOTS];    // Global Card Type
U32 g_u32_card_rca[MCI_NR_SLOTS];   // Global RCA
U32 g_u32_card_size[MCI_NR_SLOTS];  // Global Card Size
U16 g_u16_card_freq[MCI_NR_SLOTS];  // Global Card Frequency
U8 g_u8_card_bus_width[MCI_NR_SLOTS];   // Global Card Bus Width

cid_t g_sd_card_cid = { 0, 0, 0, 0 };   // Stores the CID data from the SD card

#if 1
  // These buffers should be dynamically allocated; may be in the sd_mmc driver initialization.
#if (defined __GNUC__) && (defined __AVR32__)
__attribute__ ((__aligned__ (4)))
#elif (defined __ICCAVR32__)
#pragma data_alignment = 4
#endif
U8 sector_buf_0[SD_MMC_SECTOR_SIZE];

#if (defined __GNUC__) && (defined __AVR32__)
__attribute__ ((__aligned__ (4)))
#elif (defined __ICCAVR32__)
#pragma data_alignment = 4
#endif
U8 sector_buf_1[SD_MMC_SECTOR_SIZE];

#else
#define EXTERNAL_RAM_0 _Pragma("location=AVR32_INTRAM0_ADDRESS") __no_init
#define EXTERNAL_RAM_1 _Pragma("location=AVR32_INTRAM1_ADDRESS") __no_init

EXTERNAL_RAM_0 U8 sector_buf_0[SD_MMC_SECTOR_SIZE];
EXTERNAL_RAM_1 U8 sector_buf_1[SD_MMC_SECTOR_SIZE];
#endif


// Global Pbb Speed in case of reinitialization Launch
U32 g_pbb_hz = 0;

// Global Cpu speed, needed by the cpu_delay() function
U32 g_cpu_hz = 0;

// Variables to manage a standby/restart access
volatile U32 gl_ptr_mem[MCI_NR_SLOTS] = { 0, 0 };

// MCI Instance
volatile avr32_mci_t* mci = &AVR32_MCI;

#if ACCESS_USB == ENABLED

Bool is_dma_usb_2_ram_complete (void);
// static Bool is_dma_ram_2_mci_complete( void );
static void dma_usb_2_ram (void* ram, size_t size);
// static void dma_ram_2_mci(const void *ram, size_t size, unsigned int BlockNr_u32);

Bool is_dma_mci_2_ram_complete (void);
Bool is_dma_ram_2_usb_complete (void);
// static void dma_mci_2_ram(void *ram, size_t size, unsigned int BlockNr_u32);
static void dma_ram_2_usb (const void* ram, size_t size);

#endif

void sd_mmc_mci_dma_read_init (void);
void sd_mmc_mci_dma_write_init (void);

unsigned char cSdEncryptedCardEnabledByUser = FALSE;
unsigned char cSdEncryptedCardReadWriteEnabled = READ_WRITE_ACTIVE;


unsigned char cSdUncryptedCardEnabledByUser = TRUE;
unsigned char cSdUncryptedCardReadWriteEnabled = READ_WRITE_ACTIVE;

// #define MMC_DEBUG_PRINT // set in global.h

#ifdef MMC_DEBUG_PRINT
int CI_LocalPrintf (char* szFormat, ...);
int CI_TickLocalPrintf (char* szFormat, ...);
#else
#define CI_LocalPrintf(...)
#define CI_TickLocalPrintf(...)
#endif


// _____ D E F I N I T I O N S ______________________________________________

void sd_mmc_mci_resources_init (void);

Bool is_sd_mmc_mci_card_present (U8 slot)
{
#ifdef MMC_DEBUG_PRINT
static U8 LastStateTestCardPresent[MCI_NR_SLOTS] = { 0, 0 };
#endif // MMC_DEBUG_PRINT
    // static U8 StartupFlag = 0;
U8 Value = 0;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;

    switch (slot)
    {
        case MCI_SLOT_B:
#ifdef MMC_DEBUG_PRINT
            Value = (gpio_get_pin_value (SD_SLOT_4BITS_CARD_DETECT) == SD_SLOT_4BITS_CARD_DETECT_VALUE);
            if (LastStateTestCardPresent[MCI_SLOT_B] != Value)
            {
                CI_LocalPrintf ("%9d MMC SLOT B 4-Bit present = %d\n\r", xTickCount, Value);
                LastStateTestCardPresent[MCI_SLOT_B] = Value;
            }
#endif // MMC_DEBUG_PRINT
            return (gpio_get_pin_value (SD_SLOT_4BITS_CARD_DETECT) == SD_SLOT_4BITS_CARD_DETECT_VALUE);

        case MCI_SLOT_A:
            // Todo: Check real sd card detect
#ifdef MMC_DEBUG_PRINT

#define SD_SLOT_8BITS_CARD_DETECT_STICK20          AVR32_PIN_PB00

            // Value = (gpio_get_pin_value(SD_SLOT_8BITS_CARD_DETECT)==SD_SLOT_8BITS_CARD_DETECT_VALUE);
            // Value = (gpio_get_pin_value(SD_SLOT_8BITS_CARD_DETECT_STICK20)==1);


            if (LastStateTestCardPresent[MCI_SLOT_A] != Value)
            {
                CI_LocalPrintf ("%9d MMC SLOT A 8-Bit present = %d\n\r", xTickCount, Value);
                LastStateTestCardPresent[MCI_SLOT_A] = Value;
            }
#endif // MMC_DEBUG_PRINT
            Value = 1;
            return (Value);
            // return (gpio_get_pin_value(SD_SLOT_8BITS_CARD_DETECT)==SD_SLOT_8BITS_CARD_DETECT_VALUE);
    }

    return FALSE;
}

Bool is_sd_mmc_mci_card_protected (U8 slot)
{
#ifdef MMC_DEBUG_PRINT
static U8 LastStateTestCardProtected[MCI_NR_SLOTS] = { 0, 0 };
U8 Value;
#endif // MMC_DEBUG_PRINT

    if (slot > MCI_LAST_SLOTS)
        return FALSE;

    switch (slot)
    {
        case MCI_SLOT_B:
#ifdef MMC_DEBUG_PRINT
            Value = (gpio_get_pin_value (SD_SLOT_4BITS_WRITE_PROTECT) == SD_SLOT_4BITS_WRITE_PROTECT);
            if (LastStateTestCardProtected[MCI_SLOT_B] != Value)
            {
                CI_LocalPrintf ("%9d MMC SLOT B 4-Bit protected = %d\n\r", xTickCount, Value);
                LastStateTestCardProtected[MCI_SLOT_B] = Value;
            }
#endif // MMC_DEBUG_PRINT
            return (gpio_get_pin_value (SD_SLOT_4BITS_WRITE_PROTECT) == SD_SLOT_4BITS_WRITE_PROTECT_VALUE);
        case MCI_SLOT_A:
#ifdef MMC_DEBUG_PRINT
            Value = (gpio_get_pin_value (SD_SLOT_8BITS_WRITE_PROTECT) == SD_SLOT_8BITS_WRITE_PROTECT);
            if (LastStateTestCardProtected[MCI_SLOT_A] != Value)
            {
                CI_LocalPrintf ("%9d MMC SLOT A 8-Bit protected = %d\n\r", xTickCount, Value);
                LastStateTestCardProtected[MCI_SLOT_A] = Value;
            }
#endif // MMC_DEBUG_PRINT
            return (0);
            // return (gpio_get_pin_value(SD_SLOT_8BITS_WRITE_PROTECT)==SD_SLOT_8BITS_WRITE_PROTECT_VALUE);
    }

    return FALSE;
}



Bool sd_mmc_mci_get_csd (U8 slot)
{
csd_t csd;

U32 max_Read_DataBlock_Length;
U32 mult;
U32 blocknr;
U8 tmp;
const U16 freq_unit[4] = { 10, 100, 1000, 10000 };
const U8 mult_fact[16] = { 0, 10, 12, 13, 15, 20, 26, 30, 35, 40, 45, 52, 55, 60, 70, 80 }; // MMC tabs...

    if (slot > MCI_LAST_SLOTS)
        return FALSE;
    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // -- (CMD9)
    if (mci_send_cmd (mci, SD_MMC_SEND_CSD_CMD, g_u32_card_rca[slot]) != MCI_SUCCESS)
        return FALSE;


    csd.csd[0] = mci_read_response (mci);
    csd.csd[1] = mci_read_response (mci);
    csd.csd[2] = mci_read_response (mci);
    csd.csd[3] = mci_read_response (mci);

    // -- Read "System specification version", only available on MMC card
    // field: SPEC_VERS (only on MMC)
    if (MMC_CARD & g_u8_card_type[slot])    // TO BE ADDED
    {
        if (CSD_SPEC_VER_4_0 == (MSB0 (csd.csd[0]) & CSD_MSK_SPEC_VER))
        {
            g_u8_card_type[slot] |= MMC_CARD_V4;
        }
    }

    // -- Compute MMC/SD speed
    // field: TRAN_SPEED (CSD V1 & V2 are the same)
    g_u16_card_freq[slot] = mult_fact[csd.csd_v1.tranSpeed >> 3];   // Get Multiplier factor
    if (SD_CARD & g_u8_card_type[slot])
    {
        // SD card don't have same frequency that MMC card
        if (26 == g_u16_card_freq[slot])
        {
            g_u16_card_freq[slot] = 25;
        }
        else if (52 == g_u16_card_freq[slot])
        {
            g_u16_card_freq[slot] = 50;
        }
    }
    g_u16_card_freq[slot] *= freq_unit[csd.csd_v1.tranSpeed & 0x07];    // Get transfer rate unit

    // -- Compute card size in number of block
    // field: WRITE_BL_LEN, READ_BL_LEN, C_SIZE (CSD V1 & V2 are not the same)
    if (SD_CARD_HC & g_u8_card_type[slot])
    {
        g_u32_card_size[slot] = (csd.csd_v2.deviceSizeH << 16) + (csd.csd_v2.deviceSizeL & 0xFFff);

        // memory capacity = (C_SIZE+1) * 1K sector
        g_u32_card_size[slot] = (g_u32_card_size[slot] + 1) << 10;  // unit 512B
    }
    else
    {
        // Check block size
        tmp = csd.csd_v1.writeBlLen;    // WRITE_BL_LEN
        if (tmp < CSD_BLEN_512)
            return FALSE;   // block size < 512B not supported by firmware

        tmp = csd.csd_v1.readBlLen; // READ_BL_LEN
        if (tmp < CSD_BLEN_512)
            return FALSE;   // block size < 512B not supported by firmware

        // // Compute Memory Capacity
        // compute MULT
        mult = 1 << (csd.csd_v1.cSizeMult + 2);
        max_Read_DataBlock_Length = 1 << csd.csd_v1.readBlLen;
        // compute MSB of C_SIZE
        blocknr = csd.csd_v1.deviceSizeH << 2;
        // compute MULT * (LSB of C-SIZE + MSB already computed + 1) = BLOCKNR
        blocknr = mult * (blocknr + csd.csd_v1.deviceSizeL + 1);
        g_u32_card_size[slot] = ((max_Read_DataBlock_Length * blocknr) / 512);
    }

    return TRUE;
}

Bool sd_mmc_get_ext_csd (U8 slot)
{
U8 i;
U32 val;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;
    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    mci_set_block_size (mci, SD_MMC_SECTOR_SIZE);   // Ext CSD = 512B size
    mci_set_block_count (mci, 1);

    // ** (CMD8)
    // read the extended CSD
    if (mci_send_cmd (mci, SD_MMC_SEND_EXT_CSD_CMD, 0) != MCI_SUCCESS)
        return FALSE;

    // READ_EXT_CSD // discard bytes not used
    for (i = (512L / 8); i != 0; i--)
    {
        while (!(mci_rx_ready (mci)));
        mci_rd_data (mci);
        while (!(mci_rx_ready (mci)));
        if (((64 - 26) == i) && (g_u8_card_type[slot] & MMC_CARD_HC))
        {
            // If MMC HC then read Sector Count
            g_u32_card_size[slot] = mci_rd_data (mci);
        }
        else
        {
            val = mci_rd_data (mci);
            if ((64 - 24) == i)
            {   // Read byte at offset 196
                if (MSB0 (val) & 0x02)
                    g_u16_card_freq[slot] = 52 * 1000;
                else
                    g_u16_card_freq[slot] = 26 * 1000;
            }
        }
    }

    return TRUE;
}

Bool sd_mmc_set_block_len (U8 slot, U16 length)
{
    if (slot > MCI_LAST_SLOTS)
        return FALSE;
    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    if (mci_send_cmd (mci, SD_MMC_SET_BLOCKLEN_CMD, length) != MCI_SUCCESS)
        return FALSE;

    // check response, card must be in TRAN state
    if ((mci_read_response (mci) & MMC_TRAN_STATE_MSK) != MMC_TRAN_STATE)
        return FALSE;

    mci_set_block_size (mci, length);
    mci_set_block_count (mci, 1);
    return TRUE;
}

Bool sd_mmc_mci_init (const mci_options_t * mci_opt, long pbb_hz, long cpu_hz)
{
union
{
    unsigned long mcfg;
    avr32_hmatrix_mcfg_t MCFG;
} u_avr32_hmatrix_mcfg;

union
{
    unsigned long scfg;
    avr32_hmatrix_scfg_t SCFG;
} u_avr32_hmatrix_scfg;

    // For the USBB DMA HMATRIX master, use infinite length burst.
    u_avr32_hmatrix_mcfg.mcfg = AVR32_HMATRIX.mcfg[AVR32_HMATRIX_MASTER_USBB_DMA];
    u_avr32_hmatrix_mcfg.MCFG.ulbt = AVR32_HMATRIX_ULBT_INFINITE;
    AVR32_HMATRIX.mcfg[AVR32_HMATRIX_MASTER_USBB_DMA] = u_avr32_hmatrix_mcfg.mcfg;

    // For the USBB DPRAM HMATRIX slave, use the USBB DMA as fixed default master.
    u_avr32_hmatrix_scfg.scfg = AVR32_HMATRIX.scfg[AVR32_HMATRIX_SLAVE_USBB_DPRAM];
    u_avr32_hmatrix_scfg.SCFG.fixed_defmstr = AVR32_HMATRIX_MASTER_USBB_DMA;
    u_avr32_hmatrix_scfg.SCFG.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_FIXED_DEFAULT;
    AVR32_HMATRIX.scfg[AVR32_HMATRIX_SLAVE_USBB_DPRAM] = u_avr32_hmatrix_scfg.scfg;

    g_pbb_hz = pbb_hz;
    g_cpu_hz = cpu_hz;

    // Init MCI controler
    if (mci_init (mci, mci_opt, pbb_hz) != MCI_SUCCESS)
    {
#ifdef MMC_DEBUG_PRINT
        CI_LocalPrintf ("%9d sd_mmc_mci_init: *** mci_init fail *** \n\r", xTickCount);
#endif // MMC_DEBUG_PRINT
        return FALSE;
    }

    // Default card initialization. This can be removed since the card init will
    // automatically be done when needed.
    sd_mmc_mci_card_init (mci_opt->card_slot);



    return TRUE;
}

Bool sd_mmc_mci_card_init (U8 slot)
{
mci_options_t mci_opt;
U32 u32_response;

    if (TRUE == sd_mmc_mci_init_done[slot])
        return TRUE;

    // Default card is not known.
    g_u8_card_type[slot] = UNKNOWN_CARD;

    // Default bus size is 1 bit.
    g_u8_card_bus_width[slot] = MCI_BUS_SIZE_1_BIT;

    // Default card speed and disable High Speed mode.
    mci_opt.card_speed = 400000;
    mci_opt.card_slot = slot;
    mci_init (mci, &mci_opt, g_pbb_hz);

    // Wait for 1ms, then wait for 74 more clock cycles (see MMC norms)
    if (mci_send_cmd (mci, SD_MMC_INIT_STATE_CMD, 0xFFFFFFFF) != MCI_SUCCESS)
        return FALSE;

    // -- (CMD0)
    // Set card in idle state
    if (mci_send_cmd (mci, SD_MMC_GO_IDLE_STATE_CMD, 0xFFFFFFFF) != MCI_SUCCESS)
        return FALSE;

  sd_mmc_init_step1:
    // (CMD1)
    // To send its Operating Conditions Register contents command only supported by MMC card
    if (mci_send_cmd (mci, SD_MMC_MMC_SEND_OP_COND_CMD, OCR_MSK_BUSY | OCR_MSK_VOLTAGE_ALL | OCR_MSK_HC) == MCI_SUCCESS)
    {
        // MMC cards always respond to MMC_SEND_OP_COND
        g_u8_card_type[slot] = MMC_CARD;
        u32_response = mci_read_response (mci);
        if (!(u32_response & OCR_MSK_BUSY))
        {
            // here card busy, it did not completed its initialization process
            // resend command MMC_SEND_OP_COND
            goto sd_mmc_init_step1; // loop until it is ready
        }
        if (0 != (u32_response & OCR_MSK_HC))
        {
            g_u8_card_type[slot] |= MMC_CARD_HC;
        }
    }
    else
    {
        // SD cards do not respond to MMC_SEND_OP_COND
        g_u8_card_type[slot] = SD_CARD;

        // -- (CMD8)
        // enables to expand new functionality to some existing commands supported only by SD HC card
        if (mci_send_cmd (mci, SD_MMC_SD_SEND_IF_COND_CMD, 0x000001AA) == MCI_SUCCESS)
        {
            // It is a SD HC
            if (0x000001AA == mci_read_response (mci))
            {
                g_u8_card_type[slot] |= SD_CARD_V2;
            }
        }

      sd_mmc_init_step2:
        // -- (CMD55)
        // Indicates to the card that the next command is an application specific command rather than a standard command
        // CMD55 shall always precede ACMD41
        if (mci_send_cmd (mci, SD_MMC_APP_CMD, 0) != MCI_SUCCESS)
            return FALSE;

        // -- (ACMD41)
        // Sends host OCR register
        if (SD_CARD_V2 & g_u8_card_type[slot])
        {
            // Sends host capacity support information (HCS)
            if (mci_send_cmd (mci, SD_MMC_SDCARD_APP_OP_COND_CMD, OCR_MSK_BUSY | OCR_MSK_VOLTAGE_3_2V_3_3V | OCR_MSK_HC) != MCI_SUCCESS)
                return FALSE;
        }
        else
        {
            if (mci_send_cmd (mci, SD_MMC_SDCARD_APP_OP_COND_CMD, OCR_MSK_BUSY | OCR_MSK_VOLTAGE_3_2V_3_3V) != MCI_SUCCESS)
                return FALSE;
        }
        u32_response = mci_read_response (mci);

        if (!(u32_response & OCR_MSK_BUSY))
        {
            // Card Busy, resend ACMD41 precede of CMD55
            goto sd_mmc_init_step2;
        }
        // Card read then check HC type
        if (u32_response & OCR_MSK_HC)
        {
            g_u8_card_type[slot] |= SD_CARD_HC; // Card SD High Capacity
        }
    }


    // Here card ready and type (MMC <V4, MMC V4, MMC HC, SD V1, SD V2 HC) detected

    // -- (CMD2)
    // Send CID
    if (mci_send_cmd (mci, SD_MMC_ALL_SEND_CID_CMD, 0) != MCI_SUCCESS)
        return FALSE;

    g_sd_card_cid.cid[0] = mci_read_response (mci);
    g_sd_card_cid.cid[1] = mci_read_response (mci);
    g_sd_card_cid.cid[2] = mci_read_response (mci);
    g_sd_card_cid.cid[3] = mci_read_response (mci);

    // -- (CMD3)
    // Set relative address
    if (MMC_CARD & g_u8_card_type[slot])
    {
        // For MMC card, you send an address to card
        g_u32_card_rca[slot] = RCA_DEFAULT_ADR;
        if (mci_send_cmd (mci, SD_MMC_SET_RELATIVE_ADDR_CMD, RCA_DEFAULT_ADR) != MCI_SUCCESS)
            return FALSE;
    }
    else
    {
        // For SD card, you ask an address to card
        if (mci_send_cmd (mci, SD_MMC_SET_RELATIVE_ADDR_CMD, RCA_RESERVE_ADR) != MCI_SUCCESS)
            return FALSE;
    }
    if (SD_CARD & g_u8_card_type[slot])
    {
        // For SD card, you receiv address of card
        g_u32_card_rca[slot] = mci_read_response (mci) & RCA_MSK_ADR;
    }


    // -- (CMD9)
    // Read & analyse CSD register
    if (sd_mmc_mci_get_csd (slot) != TRUE)
        return FALSE;


    // -- (CMD7)-R1b
    // select card
    if (mci_send_cmd (mci, SD_MMC_SEL_DESEL_CARD_CMD, g_u32_card_rca[slot]) != MCI_SUCCESS)
        return FALSE;

    // Wait end of busy
    mci_wait_busy_signal (mci); // read busy state on DAT0

    // Get clock by checking the extended CSD register
    if (MMC_CARD_V4 & g_u8_card_type[slot])
    {
        // Get clock (MMC V4) and size (MMC V4 HC) by checking the extended CSD register
        // -- (CMD8)
        if (sd_mmc_get_ext_csd (slot) != TRUE)
            return FALSE;
    }


#if (SD_4_BIT == ENABLE)
    // set bus size
    if (SD_CARD & g_u8_card_type[slot])
    {
        // set 4-bit bus for SD Card
        // -- (CMD55)
        if (mci_send_cmd (mci, SD_MMC_APP_CMD, g_u32_card_rca[slot]) != MCI_SUCCESS)
            return FALSE;

        // -- (CMD6)
        if (mci_send_cmd (mci, SD_MMC_SDCARD_SET_BUS_WIDTH_CMD, SD_BUS_4_BIT) != MCI_SUCCESS)
            return FALSE;

        g_u8_card_bus_width[slot] = MCI_BUS_SIZE_4_BIT;
        if (mci_set_bus_size (mci, MCI_BUS_SIZE_4_BIT) != MCI_SUCCESS)
            return FALSE;
    }
    else    // MMC bus width management
    {
        // set 8-bit bus for MMC Card
        if (MMC_CARD_V4 & g_u8_card_type[slot])
        {
            // -- (CMD6)-R1b
            // set 8-bit bus width (appeared from V4.0 specification)
            if (mci_send_cmd (mci,
                              MMC_SWITCH_CMD,
                              ((U32) MMC_SWITCH_WRITE << 24) |
                              ((U32) MMC_SWITCH_BUS_WIDTH << 16) | ((U32) MMC_SWITCH_VAL_8BIT << 8) | ((U32) MMC_SWITCH_CMD_SET)) != MCI_SUCCESS)
            {
                return FALSE;
            }
            // Wait end of busy
            mci_wait_busy_signal (mci); // read busy state on DAT0
            g_u8_card_bus_width[slot] = MCI_BUS_SIZE_8_BIT;
            if (mci_set_bus_size (mci, MCI_BUS_SIZE_8_BIT) != MCI_SUCCESS)
                return FALSE;
        }
    }
#endif

    if (MMC_CARD_V4 & g_u8_card_type[slot])
    {
        // -- (CMD6)-R1b
        // set high speed interface timing
        if (mci_send_cmd (mci,
                          MMC_SWITCH_CMD,
                          ((U32) MMC_SWITCH_WRITE << 24) |
                          ((U32) MMC_SWITCH_HIGH_SPEED << 16) | ((U32) MMC_SWITCH_VAL_HS << 8) | ((U32) MMC_SWITCH_CMD_SET)) != MCI_SUCCESS)
        {
            return FALSE;
        }
        // Wait end of busy
        mci_wait_busy_signal (mci);
    }


    if (SD_CARD_V2 & g_u8_card_type[slot])
    {
/** \brief Switch func argument definitions */
#define SDMMC_SWITCH_FUNC_MODE_CHECK  (0 << 31)   /**< Check function */
#define SDMMC_SWITCH_FUNC_MODE_SWITCH (1 << 31)   /**< Switch function */
#define SDMMC_SWITCH_FUNC_HIGH_SPEED  (1 << 0)    /**< High Speed */
#define SDMMC_SWITCH_FUNC_G1_KEEP     (0xf << 0)  /**< Group 1 No influence */
#define SDMMC_SWITCH_FUNC_G2_KEEP     (0xf << 4)  /**< Group 2 No influence */
#define SDMMC_SWITCH_FUNC_G3_KEEP     (0xf << 8)  /**< Group 3 No influence */
#define SDMMC_SWITCH_FUNC_G4_KEEP     (0xf << 12) /**< Group 4 No influence */
#define SDMMC_SWITCH_FUNC_G5_KEEP     (0xf << 16) /**< Group 5 No influence */
#define SDMMC_SWITCH_FUNC_G6_KEEP     (0xf << 20) /**< Group 6 No influence */

        mci_set_block_size (mci, 512 / 8);  // CMD6 512 bits status
        mci_set_block_count (mci, 1);

        // -- (CMD6)
        // Check if we can enter into the High Speed mode.
        if (mci_send_cmd (mci, SD_SWITCH_FUNC, SDMMC_SWITCH_FUNC_MODE_CHECK | SDMMC_SWITCH_FUNC_HIGH_SPEED) != MCI_SUCCESS)
        {
            return FALSE;
        }
        // Wait end of busy
        mci_wait_busy_signal (mci); // read busy state on DAT0

Bool b_hs_supported = FALSE;
        {
U8 i;
            for (i = 0; i < (512L / 8); i += 4)
            {
volatile U32 data;
                while (!(mci_rx_ready (mci)));
                data = mci_rd_data (mci);
                if (i == 16)
                {
                    if (((data >> 24) & 0xf) == 1)
                        b_hs_supported = TRUE;
                    break;
                }
            }
        }

        if (b_hs_supported == FALSE)
            goto sd_mmc_init_step3;

        if (mci_send_cmd (mci, SD_SWITCH_FUNC, SDMMC_SWITCH_FUNC_MODE_SWITCH
                          | SDMMC_SWITCH_FUNC_G6_KEEP
                          | SDMMC_SWITCH_FUNC_G5_KEEP
                          | SDMMC_SWITCH_FUNC_G4_KEEP
                          | SDMMC_SWITCH_FUNC_G3_KEEP | SDMMC_SWITCH_FUNC_G2_KEEP | SDMMC_SWITCH_FUNC_HIGH_SPEED) != MCI_SUCCESS)
        {
            return FALSE;
        }
        {
U8 i;
            for (i = 0; i < (512L / 8); i += 4)
            {
volatile U32 data;
                while (!(mci_rx_ready (mci)));
                data = mci_rd_data (mci);
            }
        }

        /* A 8 cycle delay is required after switch command @ 200KHz clock this should be 40 uS, but we use 80 to handle unprecise clock setting. */
        cpu_delay_us (80, g_cpu_hz);

union u_cfg
{
    unsigned long cfg;
    avr32_mci_cfg_t CFG;
};
union u_cfg val;
        val.cfg = mci->cfg;
        val.CFG.hsmode = 1;
        mci->cfg = val.cfg;

        // deselect card
        if (mci_send_cmd (mci, SD_MMC_SEL_DESEL_CARD_CMD, 0) != MCI_SUCCESS)
            return FALSE;

        // Wait end of busy
        mci_wait_busy_signal (mci); // read busy state on DAT0

        // -- (CMD9)
        // Read & analyse CSD register
        if (sd_mmc_mci_get_csd (slot) != TRUE)
            return FALSE;

        // select card
        if (mci_send_cmd (mci, SD_MMC_SEL_DESEL_CARD_CMD, g_u32_card_rca[slot]) != MCI_SUCCESS)
            return FALSE;

        // Wait end of busy
        mci_wait_busy_signal (mci); // read busy state on DAT0
    }

  sd_mmc_init_step3:
    // Set clock
    mci_set_speed (mci, g_pbb_hz, g_u16_card_freq[slot] * 1000);

    // -- (CMD13)
    // Check if card is ready, the card must be in TRAN state
    if (sd_mmc_mci_cmd_send_status (slot) != TRUE)
        return FALSE;

    if ((mci_read_response (mci) & MMC_TRAN_STATE_MSK) != MMC_TRAN_STATE)
        return FALSE;

    // -- (CMD16)
    // Set the card block length to 512B
    if (sd_mmc_set_block_len (slot, SD_MMC_SECTOR_SIZE) != TRUE)
        return FALSE;

    // USB Test Unit Attention requires a state "busy" between "not present" and "ready" state
    // otherwise never report card change
    sd_mmc_mci_init_done[slot] = TRUE;

    // Set the volume sizes for uncrypted, crypted and hidden volume
    InitSDVolumeSizes_u32 ();

    CheckForNewSdCard ();
    return TRUE;
}

Bool sd_mmc_mci_read_sector_2_ram (U8 slot, void* ram)
{
U32 wordsToRead;
int* pRam = ram;

    // Read data
    wordsToRead = (SD_MMC_SECTOR_SIZE / sizeof (*pRam));
    while (wordsToRead > 0)
    {
        while (!(mci_rx_ready (mci)));
        *pRam++ = mci_rd_data (mci);
        while (!(mci_rx_ready (mci)));
        *pRam++ = mci_rd_data (mci);
        while (!(mci_rx_ready (mci)));
        *pRam++ = mci_rd_data (mci);
        while (!(mci_rx_ready (mci)));
        *pRam++ = mci_rd_data (mci);
        while (!(mci_rx_ready (mci)));
        *pRam++ = mci_rd_data (mci);
        while (!(mci_rx_ready (mci)));
        *pRam++ = mci_rd_data (mci);
        while (!(mci_rx_ready (mci)));
        *pRam++ = mci_rd_data (mci);
        while (!(mci_rx_ready (mci)));
        *pRam++ = mci_rd_data (mci);
        wordsToRead -= 8;
    }

    return TRUE;
}

Bool sd_mmc_mci_dma_read_sector_2_ram (U8 slot, void* ram)
{
int* pRam = ram;

    // Src Address: the MCI registers.
    AVR32_DMACA.sar1 = (U32) & AVR32_MCI.fifo;

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.dar1 = (unsigned long) pRam;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((SD_MMC_SECTOR_SIZE / 4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET) // Block transfer size
        ;

    // Enable Channel 1 : start the process.
    AVR32_DMACA.chenreg = ((2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (2 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

    // Wait for the end of the AES->RAM transfer (channel 1).
    while (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET));

    return TRUE;
}

Bool sd_mmc_mci_read_multiple_sector_2_ram (U8 slot, void* ram, U32 nb_sector)
{
U32 wordsToRead;
int* pRam = ram;

    // Read data
    while (nb_sector > 0)
    {
        wordsToRead = (SD_MMC_SECTOR_SIZE / sizeof (*pRam));
        while (wordsToRead > 0)
        {
            while (!(mci_rx_ready (mci)));
            *pRam++ = mci_rd_data (mci);
            while (!(mci_rx_ready (mci)));
            *pRam++ = mci_rd_data (mci);
            while (!(mci_rx_ready (mci)));
            *pRam++ = mci_rd_data (mci);
            while (!(mci_rx_ready (mci)));
            *pRam++ = mci_rd_data (mci);
            while (!(mci_rx_ready (mci)));
            *pRam++ = mci_rd_data (mci);
            while (!(mci_rx_ready (mci)));
            *pRam++ = mci_rd_data (mci);
            while (!(mci_rx_ready (mci)));
            *pRam++ = mci_rd_data (mci);
            while (!(mci_rx_ready (mci)));
            *pRam++ = mci_rd_data (mci);
            wordsToRead -= 8;
        }
        nb_sector--;
    }

    return TRUE;
}

#if ACCESS_USB == ENABLED

#include "usb_drv.h"
#include "scsi_decoder.h"
static void dma_ram_2_usb (const void* ram, size_t size)
{
    AVR32_USBB_UDDMAX_nextdesc (g_scsi_ep_ms_in) = (U32) NULL;
    AVR32_USBB_UDDMAX_addr (g_scsi_ep_ms_in) = (U32) ram;
    AVR32_USBB_UDDMAX_control (g_scsi_ep_ms_in) = ((size << AVR32_USBB_UXDMAX_CONTROL_CH_BYTE_LENGTH_OFFSET)
                                                   & AVR32_USBB_UXDMAX_CONTROL_CH_BYTE_LENGTH_MASK)
        // | AVR32_USBB_UXDMAX_CONTROL_BURST_LOCK_EN_MASK
        // | AVR32_USBB_UXDMAX_CONTROL_DMAEND_EN_MASK
        // | AVR32_USBB_UXDMAX_CONTROL_BUFF_CLOSE_IN_EN_MASK
        | AVR32_USBB_UXDMAX_CONTROL_CH_EN_MASK;

    // Workaround for bug 3501.
    (void) AVR32_USBB_UDDMAX_control (g_scsi_ep_ms_in);
}

Bool is_dma_ram_2_usb_complete (void)
{
    if (AVR32_USBB_UDDMAX_status (g_scsi_ep_ms_in) & AVR32_USBB_UXDMAX_STATUS_CH_EN_MASK)
        return FALSE;
    else
        return TRUE;
}


#define STICK20_RAM_TO_AES_TO_MCI				0
#define STICK20_MCI_TO_AES_TO_RAM				1

/******************************************************************************

  This is SD transfer function with AES encryption

  SD > AES > RAM

******************************************************************************/

void STICK20_mci_aes (unsigned char cMode, unsigned short int u16BufferSize, unsigned int* pIOBuffer, unsigned int BlockNr_u32);

static void stop_dma(){
	taskENTER_CRITICAL ();
    AVR32_DMACA.chenreg = (~(3 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | ~(3 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));
    AVR32_DMACA.DMACFGREG.dma_en = 0;
    while(!is_dma_ram_2_mci_complete());
	while(!is_dma_ram_2_usb_complete());
	while(!is_dma_usb_2_ram_complete());
	while(!is_dma_mci_2_ram_complete());
	taskEXIT_CRITICAL();
}

static void dma_mci_2_ram_crypted (void* ram, size_t size, unsigned int BlockNr_u32)
{
    // int *pRam = ram;

    STICK20_mci_aes (STICK20_MCI_TO_AES_TO_RAM, size, (unsigned int *) ram, BlockNr_u32);

    // Enable Channel 0 & 1 : start the process.
    AVR32_DMACA.chenreg = ((3 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (3 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

}

/******************************************************************************

  This is SD transfer function without AES encryption

  SD > RAM

******************************************************************************/

static void dma_mci_2_ram_uncrypted (void* ram, size_t size, unsigned int BlockNr_u32)
{
    int* pRam = ram;

    sd_mmc_mci_dma_read_init ();

    // Src Address: the MCI registers.
    AVR32_DMACA.sar1 = (U32) & AVR32_MCI.fifo;

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.dar1 = (unsigned long) pRam;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((size / 4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET)   // Block transfer size
        ;

    // Enable Channel 1 : start the process.
    AVR32_DMACA.chenreg = ((2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (2 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));
}

Bool is_dma_mci_2_ram_complete (void)
{
    // Wait for the end of the AES->RAM transfer (channel 1).
    if (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET))
        return FALSE;

    return TRUE;
}
#endif

void dma_mci_2_ram (void* ram, size_t size, unsigned int BlockNr_u32)
{
    u32 StartOfParition_u32;

    StartOfParition_u32 = SD_SIZE_UNCRYPTED_PARITION;
    // StartOfParition_u32 = GetStartUncryptedVolume_u32 ();

    if (StartOfParition_u32 <= BlockNr_u32)
    {
        // Used the crypted IO - volume 1
        dma_mci_2_ram_crypted (ram, size, BlockNr_u32);
    }
    else
    {
        // Used the uncrypted IO - volume 0
        dma_mci_2_ram_uncrypted (ram, size, BlockNr_u32);
    }
}

Bool sd_mmc_mci_dma_read_multiple_sector_2_ram (U8 slot, void* ram, U32 nb_sector)
{
    int* pRam = ram;

    // Src Address: the MCI registers.
    AVR32_DMACA.sar1 = (U32) & AVR32_MCI.fifo;

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.dar1 = (unsigned long) pRam;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((nb_sector * (SD_MMC_SECTOR_SIZE / 4)) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET)   // Block transfer size
        ;

    // Enable Channel 1 : start the process.
    AVR32_DMACA.chenreg = ((2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (2 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

    // Wait for the end of the AES->RAM transfer (channel 1).
    while (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET));

    return TRUE;
}

Bool sd_mmc_mci_write_sector_from_ram (U8 slot, const void* ram)
{
U32 wordsToWrite;
const unsigned int* pRam = ram;

    // Write Data
    wordsToWrite = (SD_MMC_SECTOR_SIZE / sizeof (*pRam));
    while (wordsToWrite > 0)
    {
        while (!mci_tx_ready (mci));
        mci_wr_data (mci, *pRam++);
        while (!mci_tx_ready (mci));
        mci_wr_data (mci, *pRam++);
        while (!mci_tx_ready (mci));
        mci_wr_data (mci, *pRam++);
        while (!mci_tx_ready (mci));
        mci_wr_data (mci, *pRam++);
        while (!mci_tx_ready (mci));
        mci_wr_data (mci, *pRam++);
        while (!mci_tx_ready (mci));
        mci_wr_data (mci, *pRam++);
        while (!mci_tx_ready (mci));
        mci_wr_data (mci, *pRam++);
        while (!mci_tx_ready (mci));
        mci_wr_data (mci, *pRam++);
        wordsToWrite -= 8;
    }

    return TRUE;
}

Bool sd_mmc_mci_dma_write_sector_from_ram (U8 slot, const void* ram)
{
const unsigned int* pRam = ram;

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.sar1 = (unsigned long) pRam;

    // Src Address: the MCI registers.
    AVR32_DMACA.dar1 = (U32) & AVR32_MCI.fifo;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((SD_MMC_SECTOR_SIZE / 4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET) // Block transfer size
        ;

    // Enable Channel 1 : start the process.
    AVR32_DMACA.chenreg = ((2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (2 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

    // Wait for the end of the AES->RAM transfer (channel 1).
    while (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET));

    return TRUE;
}

Bool sd_mmc_mci_write_multiple_sector_from_ram (U8 slot, const void* ram, U32 nb_sector)
{
U32 wordsToWrite;
const unsigned int* pRam = ram;

    // Write Data
    while (nb_sector > 0)
    {
        wordsToWrite = (SD_MMC_SECTOR_SIZE / sizeof (*pRam));
        while (wordsToWrite > 0)
        {
            while (!mci_tx_ready (mci));
            mci_wr_data (mci, *pRam++);
            while (!mci_tx_ready (mci));
            mci_wr_data (mci, *pRam++);
            while (!mci_tx_ready (mci));
            mci_wr_data (mci, *pRam++);
            while (!mci_tx_ready (mci));
            mci_wr_data (mci, *pRam++);
            while (!mci_tx_ready (mci));
            mci_wr_data (mci, *pRam++);
            while (!mci_tx_ready (mci));
            mci_wr_data (mci, *pRam++);
            while (!mci_tx_ready (mci));
            mci_wr_data (mci, *pRam++);
            while (!mci_tx_ready (mci));
            mci_wr_data (mci, *pRam++);
            wordsToWrite -= 8;
        }
        nb_sector--;
    }

    return TRUE;
}

#if ACCESS_USB == ENABLED

#include "usb_drv.h"
#include "scsi_decoder.h"
static void dma_usb_2_ram (void* ram, size_t size)
{
    AVR32_USBB_UDDMAX_nextdesc (g_scsi_ep_ms_out) = (U32) NULL;
    AVR32_USBB_UDDMAX_addr (g_scsi_ep_ms_out) = (U32) ram;
    AVR32_USBB_UDDMAX_control (g_scsi_ep_ms_out) = ((size << AVR32_USBB_UXDMAX_CONTROL_CH_BYTE_LENGTH_OFFSET)
                                                    & AVR32_USBB_UXDMAX_CONTROL_CH_BYTE_LENGTH_MASK)
        // | AVR32_USBB_UXDMAX_CONTROL_BURST_LOCK_EN_MASK
        // | AVR32_USBB_UXDMAX_CONTROL_DMAEND_EN_MASK
        // | AVR32_USBB_UXDMAX_CONTROL_BUFF_CLOSE_IN_EN_MASK
        | AVR32_USBB_UXDMAX_CONTROL_CH_EN_MASK;

    // Workaround for bug 3501.
    (void) AVR32_USBB_UDDMAX_control (g_scsi_ep_ms_out);
}

Bool is_dma_usb_2_ram_complete (void)
{
    if (AVR32_USBB_UDDMAX_status (g_scsi_ep_ms_out) & AVR32_USBB_UXDMAX_STATUS_CH_EN_MASK)
        return FALSE;
    else
        return TRUE;
}

/******************************************************************************

  This is SD transfer function with AES encryption

  RAM > AES > SD

******************************************************************************/


void dma_ram_2_mci_crypted (const void* ram, size_t size, unsigned int BlockNr_u32)
{
    // const unsigned int *pRam = ram;

    STICK20_mci_aes (STICK20_RAM_TO_AES_TO_MCI, size, (unsigned int *) ram, BlockNr_u32);

    /*
       sd_mmc_mci_dma_write_init ();

       // Dst Address: the OutputData[] array. AVR32_DMACA.sar1 = (unsigned long)pRam;

       // Src Address: the MCI registers. AVR32_DMACA.dar1 = (U32)&AVR32_MCI.fifo;

       // Channel 1 Ctrl register high AVR32_DMACA.ctl1h = ( (size/4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET) // Block transfer size ; */

    // Enable Channel 0 & 1 : start the process.
    AVR32_DMACA.chenreg = ((3 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (3 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

    /*
       // Enable Channel 1 : start the process. AVR32_DMACA.chenreg = ((2<<AVR32_DMACA_CHENREG_CH_EN_OFFSET) |
       (2<<AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET)); */
}

/******************************************************************************

  This is SD transfer function without AES encryption

  RAM > SD

******************************************************************************/


void dma_ram_2_mci_uncrypted (const void* ram, size_t size, unsigned int BlockNr_u32)
{
    const unsigned int* pRam = ram;

    sd_mmc_mci_dma_write_init ();

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.sar1 = (unsigned long) pRam;

    // Src Address: the MCI registers.
    AVR32_DMACA.dar1 = (U32) & AVR32_MCI.fifo;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((size / 4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET)   // Block transfer size
        ;

    // Enable Channel 1 : start the process.
    AVR32_DMACA.chenreg = ((2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (2 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));
}


Bool is_dma_ram_2_mci_complete (void)
{
    // Wait for the end of the AES->RAM transfer (channel 1).
    if (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET))
        return FALSE;

    return TRUE;
}

#endif

void dma_ram_2_mci (const void* ram, size_t size, unsigned int BlockNr_u32)
{
    u32 StartOfParition_u32;

    StartOfParition_u32 = SD_SIZE_UNCRYPTED_PARITION;
    // StartOfParition_u32 = GetStartUncryptedVolume_u32 ();

    if (StartOfParition_u32 <= BlockNr_u32)
    {
        // Used the crypted IO - volume 1
        dma_ram_2_mci_crypted (ram, size, BlockNr_u32);
    }
    else
    {
        // Used the uncrypted IO - volume 0
        dma_ram_2_mci_uncrypted (ram, size, BlockNr_u32);
    }
}


Bool sd_mmc_mci_dma_write_multiple_sector_from_ram (U8 slot, const void* ram, U32 nb_sector)
{
    const unsigned int* pRam = ram;

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.sar1 = (unsigned long) pRam;

    // Src Address: the MCI registers.
    AVR32_DMACA.dar1 = (U32) & AVR32_MCI.fifo;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((nb_sector * (SD_MMC_SECTOR_SIZE / 4)) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET)   // Block transfer size
        ;

    // Enable Channel 1 : start the process.
    AVR32_DMACA.chenreg = ((2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (2 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

    // Wait for the end of the AES->RAM transfer (channel 1).
    while (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET));

    return TRUE;
}

Bool sd_mmc_mci_mem_check (U8 slot)
{
U8 timeout_init = 0;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;

    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // Check card presence
    if (is_sd_mmc_mci_card_present (slot) == FALSE)
    {
        sd_mmc_mci_init_done[slot] = FALSE;
        return FALSE;
    }

    if (sd_mmc_mci_init_done[slot] == FALSE)
    {
        while (sd_mmc_mci_card_init (slot) != TRUE)
        {
            timeout_init++;
            if (timeout_init > 10)
                return FALSE;
        }
    }
    if (sd_mmc_mci_init_done[slot] == TRUE)
    {
        LED_SdCardOnline_v ();
        return TRUE;
    }
    else
        return FALSE;
}

Bool sd_mmc_mci_read_open (U8 slot, U32 pos, U16 nb_sector)
{
U32 addr;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;

    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // Set the global memory ptr at a Byte address.
    gl_ptr_mem[slot] = pos;

    // wait for MMC not busy
    mci_wait_busy_signal (mci);

    addr = gl_ptr_mem[slot];
    // send command
    if ((!(SD_CARD_HC & g_u8_card_type[slot])) && (!(MMC_CARD_HC & g_u8_card_type[slot])))
    {
        addr <<= 9; // For NO HC card the address must be translate in byte address
    }

    // ** (CMD13)
    // Necessary to clear flag error "ADDRESS_OUT_OF_RANGE" (ID LABO = MMC15)
    if (mci_send_cmd (mci, SD_MMC_SEND_STATUS_CMD, g_u32_card_rca[slot]) != MCI_SUCCESS)
    {
        return FALSE;
    }

    // Request Block Length
    mci_set_block_size (mci, SD_MMC_SECTOR_SIZE);

    // Set Block Count
    mci_set_block_count (mci, nb_sector);

    // ** (CMD17)
    if (mci_send_cmd (mci, SD_MMC_READ_MULTIPLE_BLOCK_CMD, addr) != MCI_SUCCESS)
    {
        return FALSE;
    }

    // check response
    if ((mci_read_response (mci) & CS_FLAGERROR_RD_WR) != 0)
    {
        return FALSE;
    }

    return TRUE;
}


void sd_mmc_mci_dma_read_init (void)
{
    // Enable the DMACA
    AVR32_DMACA.dmacfgreg = 1 << AVR32_DMACA_DMACFGREG_DMA_EN_OFFSET;

    // Linked list ptrs: not used.
    AVR32_DMACA.llp1 = 0x00000000;

    // Channel 1 Ctrl register low
    AVR32_DMACA.ctl1l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) |    // Do not enable interrupts
        (2 << AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
        (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
        (0 << AVR32_DMACA_CTL1L_DINC_OFFSET) |  // Dst address increment: increment
        (0 << AVR32_DMACA_CTL1L_SINC_OFFSET) |  // Src address increment: increment
        (3 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 16 data items
        (3 << AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 16 data items
        (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source gather: disabled
        (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
        (2 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | // transfer type:P2M, flow controller: DMACA
        (1 << AVR32_DMACA_CTL1L_DMS_OFFSET) |   // Dest master: HSB master 2
        (0 << AVR32_DMACA_CTL1L_SMS_OFFSET) |   // Source master: HSB master 1
        (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) |  // Not used
        (0 << AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET)    // Not used
        ;

    // Channel 1 Config register low
    AVR32_DMACA.cfg1l = (0 << AVR32_DMACA_CFG1L_HS_SEL_DST_OFFSET) |    // Destination handshaking: ignored because the dst is memory.
        (0 << AVR32_DMACA_CFG1L_HS_SEL_SRC_OFFSET)  // Source handshaking: hw handshaking
        ;   // All other bits set to 0.

    // Channel 1 Config register high
    AVR32_DMACA.cfg1h = (0 << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) |  // Dest hw handshaking itf: ignored because the dst is memory.
        (AVR32_DMACA_CH_MMCI_RX << AVR32_DMACA_CFG1H_SRC_PER_OFFSET)    // Source hw handshaking itf:
        ;   // All other bits set to 0.

}

Bool sd_mmc_mci_dma_read_open (U8 slot, U32 pos, void* ram, U16 nb_sector)
{
    U32 addr;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;

    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // Set the global memory ptr at a Byte address.
    gl_ptr_mem[slot] = pos;

    // wait for MMC not busy
    mci_wait_busy_signal (mci);

    addr = gl_ptr_mem[slot];
    // send command
    if ((!(SD_CARD_HC & g_u8_card_type[slot])) && (!(MMC_CARD_HC & g_u8_card_type[slot])))
    {
        addr <<= 9; // For NO HC card the address must be translate in byte address
    }

    // ** (CMD13)
    // Necessary to clear flag error "ADDRESS_OUT_OF_RANGE" (ID LABO = MMC15)
    if (mci_send_cmd (mci, SD_MMC_SEND_STATUS_CMD, g_u32_card_rca[slot]) != MCI_SUCCESS)
    {
        return FALSE;
    }

    // Request Block Length
    mci_set_block_size (mci, SD_MMC_SECTOR_SIZE);

    // Set Block Count
    mci_set_block_count (mci, nb_sector);

    // sd_mmc_mci_dma_read_init ();

    AVR32_MCI.dma = 0;


    // Setup MCI DMA register
    AVR32_MCI.dma = AVR32_MCI_DMA_DMAEN_MASK | (AVR32_MCI_DMA_CHKSIZE_16_BYTES << AVR32_MCI_DMA_CHKSIZE_OFFSET);

    // ** (CMD17)
    if (mci_send_cmd (mci, SD_MMC_READ_MULTIPLE_BLOCK_CMD, addr) != MCI_SUCCESS)
    {
        return FALSE;
    }

    // check response
    if ((mci_read_response (mci) & CS_FLAGERROR_RD_WR) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

Bool sd_mmc_mci_read_close (U8 slot)
{
    if ((mci_crc_error (mci)))
    {
        return FALSE;   // An CRC error has been seen
    }

    mci_wait_busy_signal (mci);

    if (mci_send_cmd (mci, SD_MMC_STOP_READ_TRANSMISSION_CMD, 0xffffffff) != MCI_SUCCESS)
        return FALSE;

    /*
       if( (mci_overrun_error(mci)) ) { return FALSE; }

       if( (mci_underrun_error(mci)) ) { return FALSE; } */

    return TRUE;
}

Bool sd_mmc_mci_write_open (U8 slot, U32 pos, U16 nb_sector)
{
U32 addr;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;

    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // Set the global memory ptr at a Byte address.
    gl_ptr_mem[slot] = pos;

    // wait for MMC not busy
    mci_wait_busy_signal (mci);

    /*
       // (CMD13) // Necessary to clear flag error "ADDRESS_OUT_OF_RANGE" (ID LABO = MMC15) if( !mmc_drv_send_cmd( MMC_SEND_STATUS, g_u32_card_rca,
       MMC_RESP_R1 ) ) { return FALSE; } mmc_drv_read_response(); */

    addr = gl_ptr_mem[slot];
    // send command
    if ((!(SD_CARD_HC & g_u8_card_type[slot])) && (!(MMC_CARD_HC & g_u8_card_type[slot])))
    {
        addr <<= 9; // For NO HC card the address must be translate in byte address
    }

    // Set Block Length
    mci_set_block_size (mci, SD_MMC_SECTOR_SIZE);

    // Set Block Count
    mci_set_block_count (mci, nb_sector);

    // ** (CMD24)
    if (mci_send_cmd (mci, SD_MMC_WRITE_MULTIPLE_BLOCK_CMD, addr) != MCI_SUCCESS)
    {
        return FALSE;
    }

    // check response
    if ((mci_read_response (mci) & CS_FLAGERROR_RD_WR) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

void sd_mmc_mci_dma_write_init (void)
{

    // Enable the DMACA
    AVR32_DMACA.dmacfgreg = 1 << AVR32_DMACA_DMACFGREG_DMA_EN_OFFSET;

    // AVR32_MCI.dma = 0;

    // Linked list ptrs: not used.
    AVR32_DMACA.llp1 = 0x00000000;

    // Channel 1 Ctrl register low
    AVR32_DMACA.ctl1l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) |    // Do not enable interrupts
        (2 << AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
        (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
        (0 << AVR32_DMACA_CTL1L_DINC_OFFSET) |  // Dst address increment: increment
        (0 << AVR32_DMACA_CTL1L_SINC_OFFSET) |  // Src address increment: increment
        (3 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 16 data items
        (3 << AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 16 data items
        (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source gather: disabled
        (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
        (1 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | // transfer type:M2P, flow controller: DMACA
        (0 << AVR32_DMACA_CTL1L_DMS_OFFSET) |   // Dest master: HSB master 1
        (1 << AVR32_DMACA_CTL1L_SMS_OFFSET) |   // Source master: HSB master 2
        (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) |  // Not used
        (0 << AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET)    // Not used
        ;

    // Channel 1 Config register low
    AVR32_DMACA.cfg1l = (0 << AVR32_DMACA_CFG1L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking
        (0 << AVR32_DMACA_CFG1L_HS_SEL_SRC_OFFSET)  // Source handshaking: ignored because the dst is memory.
        ;   // All other bits set to 0.

    // Channel 1 Config register high
    AVR32_DMACA.cfg1h = (AVR32_DMACA_CH_MMCI_TX << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) | // Dest hw handshaking itf:
        (0 << AVR32_DMACA_CFG1H_SRC_PER_OFFSET) // Source hw handshaking itf: ignored because the dst is memory.
        ;   // All other bits set to 0.

    // Setup MCI DMA register
    // AVR32_MCI.dma = AVR32_MCI_DMA_DMAEN_MASK | (AVR32_MCI_DMA_CHKSIZE_16_BYTES << AVR32_MCI_DMA_CHKSIZE_OFFSET);
}

Bool sd_mmc_mci_dma_write_open (U8 slot, U32 pos, const void* ram, U16 nb_sector)
{
    U32 addr;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;

    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // Set the global memory ptr at a Byte address.
    gl_ptr_mem[slot] = pos;

    // wait for MMC not busy
    mci_wait_busy_signal (mci);

    /*
       // (CMD13) // Necessary to clear flag error "ADDRESS_OUT_OF_RANGE" (ID LABO = MMC15) if( !mmc_drv_send_cmd( MMC_SEND_STATUS, g_u32_card_rca,
       MMC_RESP_R1 ) ) { return FALSE; } mmc_drv_read_response(); */

    addr = gl_ptr_mem[slot];
    // send command
    if ((!(SD_CARD_HC & g_u8_card_type[slot])) && (!(MMC_CARD_HC & g_u8_card_type[slot])))
    {
        addr <<= 9; // For NO HC card the address must be translate in byte address
    }

    // Set Block Length
    mci_set_block_size (mci, SD_MMC_SECTOR_SIZE);

    // Set Block Count
    mci_set_block_count (mci, nb_sector);

    AVR32_MCI.dma = 0;
    // sd_mmc_mci_dma_write_init ();
    /*
       // Enable the DMACA AVR32_DMACA.dmacfgreg = 1 << AVR32_DMACA_DMACFGREG_DMA_EN_OFFSET;


       // Linked list ptrs: not used. AVR32_DMACA.llp1 = 0x00000000;

       // Channel 1 Ctrl register low AVR32_DMACA.ctl1l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) | // Do not enable interrupts (2 <<
       AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) | // Dst transfer width: 32 bits (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) | // Src transfer width:
       32 bits (0 << AVR32_DMACA_CTL1L_DINC_OFFSET) | // Dst address increment: increment (0 << AVR32_DMACA_CTL1L_SINC_OFFSET) | // Src address
       increment: increment (3 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 16 data items (3 <<
       AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 16 data items (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source
       gather: disabled (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled (1 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | //
       transfer type:M2P, flow controller: DMACA (0 << AVR32_DMACA_CTL1L_DMS_OFFSET) | // Dest master: HSB master 1 (1 <<
       AVR32_DMACA_CTL1L_SMS_OFFSET) | // Source master: HSB master 2 (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) | // Not used (0 <<
       AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET) // Not used ;

       // Channel 1 Config register low AVR32_DMACA.cfg1l = (0 << AVR32_DMACA_CFG1L_HS_SEL_DST_OFFSET) | // Destination handshaking: hw handshaking
       (0 << AVR32_DMACA_CFG1L_HS_SEL_SRC_OFFSET) // Source handshaking: ignored because the dst is memory. ; // All other bits set to 0.

       // Channel 1 Config register high AVR32_DMACA.cfg1h = (AVR32_DMACA_CH_MMCI_TX << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) | // Dest hw handshaking
       itf: (0 << AVR32_DMACA_CFG1H_SRC_PER_OFFSET) // Source hw handshaking itf: ignored because the dst is memory. ; // All other bits set to 0.

     */
    // Setup MCI DMA register
    AVR32_MCI.dma = AVR32_MCI_DMA_DMAEN_MASK | (AVR32_MCI_DMA_CHKSIZE_16_BYTES << AVR32_MCI_DMA_CHKSIZE_OFFSET);

    // ** (CMD24)
    if (mci_send_cmd (mci, SD_MMC_WRITE_MULTIPLE_BLOCK_CMD, addr) != MCI_SUCCESS)
    {
        return FALSE;
    }

    // check response
    if ((mci_read_response (mci) & CS_FLAGERROR_RD_WR) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

Bool sd_mmc_mci_write_close (U8 slot)
{
    if ((mci_crc_error (mci)))
    {
        return FALSE;   // An CRC error has been seen
    }

    while (!(mci_data_block_ended (mci)));

    if (mci_send_cmd (mci, SD_MMC_STOP_WRITE_TRANSMISSION_CMD, 0xffffffff) != MCI_SUCCESS)
    {
        return FALSE;
    }

    /*
       if( (mci_overrun_error(mci)) ) { return FALSE; }

       if( (mci_underrun_error(mci)) ) { return FALSE; } */

    if (slot == SD_SLOT_4BITS)
    {
        gpio_enable_gpio_pin (SD_SLOT_4BITS_DATA0_PIN); // Set D0 line as input.
        while (!(gpio_get_pin_value (SD_SLOT_4BITS_DATA0_PIN)));    // Wait until the card is ready.
        gpio_enable_module_pin (SD_SLOT_4BITS_DATA0_PIN, SD_SLOT_4BITS_DATA0_FUNCTION); // Restore initial D0 pin.
    }
    else
    {
        gpio_enable_gpio_pin (SD_SLOT_8BITS_DATA0_PIN); // Set D0 line as input.
        while (!(gpio_get_pin_value (SD_SLOT_8BITS_DATA0_PIN)));    // Wait until the card is ready.
        gpio_enable_module_pin (SD_SLOT_8BITS_DATA0_PIN, SD_SLOT_8BITS_DATA0_FUNCTION); // Restore initial D0 pin.
    }
    return TRUE;
}

#if ACCESS_USB == ENABLED

#include "usb_drv.h"
#include "scsi_decoder.h"

// ! 1) Configure two DMACA channels:
// ! - RAM -> AES
// ! - AES -> RAM
// ! 2) Set the AES cryptographic key and init vector.
// ! 3) Start the process
// ! 4) Check the result on the first 16 Words.
#  define DMACA_AES_EVAL_USART               (&AVR32_USART1)
/*
   #define STICK20_AES_BUFFER_SIZE (1024) unsigned int pSTICK20_AES_BUFFER[(STICK20_AES_BUFFER_SIZE/4)]; */
#define STICK20_RAM_TO_AES_TO_MCI				0
#define STICK20_MCI_TO_AES_TO_RAM				1


void STICK20_ram_aes_ram (unsigned char cMode, unsigned short int u16BufferSize, unsigned int* pSrcBuf, unsigned int* pDstBuf);


Bool sd_mmc_mci_read_multiple_sector (U8 slot, U16 nb_sector, U32 StartBlockNr_u32)
{
Bool b_last_state_full = FALSE;
U8 buffer_id = 0;
    // U32 ccountt0;
    // U32 ccountt1;
U32 BlockNr_u32;

    CI_TickLocalPrintf ("MCI Read: Start %6d Size %5d\n\r", StartBlockNr_u32, nb_sector);

    BlockNr_u32 = StartBlockNr_u32;

    // Turn on features used by the DMA-USB.
    Usb_enable_endpoint_bank_autoswitch (g_scsi_ep_ms_in);

    // Pipeline the 2 DMA transfer in order to speed-up the performances:
    // DMA MCI -> RAM
    // DMA RAM -> USB
    //

    /*
       CI_LocalPrintf("-"); print_ulong(DMACA_AES_EVAL_USART, nb_sector);

       ccountt0 = Get_system_register(AVR32_COUNT); */

    while (nb_sector--)
    {
        // (re)load first stage.
#ifdef TIME_MEASURING_ENABLE
        TIME_MEASURING_Start (TIME_MEASURING_TIME_MSD_AES_READ);
#endif
        // Wait for the AES semaphore
        while (pdTRUE != xSemaphoreTake (AES_semphr, 1))
        {
//          CI_StringOut ("�2");
        }

        if (buffer_id == 0)
        {
            dma_mci_2_ram (&sector_buf_0, SD_MMC_SECTOR_SIZE, BlockNr_u32);
            buffer_id = 1;
        }
        else
        {
            dma_mci_2_ram (&sector_buf_1, SD_MMC_SECTOR_SIZE, BlockNr_u32);
            buffer_id = 0;
        }


        BlockNr_u32++;

        // (re)load second stage.
        if (b_last_state_full)
        {
            if (buffer_id == 0)
            {
                dma_ram_2_usb (&sector_buf_0, SD_MMC_SECTOR_SIZE);
            }
            else
            {
                dma_ram_2_usb (&sector_buf_1, SD_MMC_SECTOR_SIZE);
            }
            // Wait completion of both stages.
			Bool res1 = busy_wait(is_dma_mci_2_ram_complete);
			Bool res2 = busy_wait(is_dma_ram_2_usb_complete);
            if (!( res1 && res2)){
                stop_dma();
                BlockNr_u32--;
                nb_sector++;
                buffer_id = (buffer_id+1)%2;
				taskENTER_CRITICAL ();
				xSemaphoreGive (AES_semphr);
				taskEXIT_CRITICAL ();
                continue;
            }

        }
        else
        {
            b_last_state_full = TRUE;
            // Wait completion of first stage only.
            //while (!is_dma_mci_2_ram_complete ());			// FIXME got stuck here on after 1 hour of using data 
			/** data for the failed case:
			b_last_state_full	1	Bool@0x923e ([R7]-6)
			buffer_id	1	U8@0x923f ([R7]-5)
			BlockNr_u32	6086273	U32@0x9240 ([R7]-4)
			slot	0	U8@0x9238 ([R7]-12)
			nb_sector	127	U16@0x9234 ([R7]-16)
			StartBlockNr_u32	6086272	U32@0x9230 ([R7]-20)
			*/
			//busy_wait(is_dma_mci_2_ram_complete);
			
			 if (!busy_wait(is_dma_mci_2_ram_complete)){
                stop_dma();
                BlockNr_u32--;
                nb_sector++;
                buffer_id = (buffer_id+1)%2;
				taskENTER_CRITICAL ();
				xSemaphoreGive (AES_semphr);
				taskEXIT_CRITICAL ();
                continue;
            }
        }

        taskENTER_CRITICAL ();
        xSemaphoreGive (AES_semphr);
        taskEXIT_CRITICAL ();

#ifdef TIME_MEASURING_ENABLE
        TIME_MEASURING_Stop (TIME_MEASURING_TIME_MSD_AES_READ);
#endif
    }
    /*
       ccountt1 = Get_system_register(AVR32_COUNT); CI_LocalPrintf(" ");

       print_ulong(DMACA_AES_EVAL_USART, ccountt1 - ccountt0);

       if (4000000000 > ccountt1 - ccountt0) { print_ulong(DMACA_AES_EVAL_USART, ccountt1 - ccountt0); } else { print_ulong(DMACA_AES_EVAL_USART,
       ccountt0 - ccountt1);

       } */
    // Complete execution of the last transfer (which is in the pipe).
    // Wait for the AES semaphore
    while (pdTRUE != xSemaphoreTake (AES_semphr, 1))
    {
//      CI_StringOut ("�8");
    }

    if (buffer_id == 0)
    {
        dma_ram_2_usb (&sector_buf_1, SD_MMC_SECTOR_SIZE);
    }
    else
    {
        dma_ram_2_usb (&sector_buf_0, SD_MMC_SECTOR_SIZE);
    }

    busy_wait(is_dma_ram_2_usb_complete);

    taskENTER_CRITICAL ();
    xSemaphoreGive (AES_semphr);
    taskEXIT_CRITICAL ();

    // Wait until the USB RAM is empty before disabling the autoswitch feature.
    while (Usb_nb_busy_bank (g_scsi_ep_ms_in) != 0);

    // Turn off exotic USB features that may not work for other devices (at45dbx...)
    Usb_disable_endpoint_bank_autoswitch (g_scsi_ep_ms_in);
    return TRUE;
}

Bool sd_mmc_mci_write_multiple_sector (U8 slot, U16 nb_sector, U32 StartBlockNr_u32)
{
Bool b_last_state_full = FALSE;
U8 buffer_id = 0;
U32 BlockNr_u32;

    CI_TickLocalPrintf ("MCI Write: Start %6d Size %5d\n\r", StartBlockNr_u32, nb_sector);

    BlockNr_u32 = StartBlockNr_u32;

    // Turn on features used by the DMA-USB.
    Usb_enable_endpoint_bank_autoswitch (g_scsi_ep_ms_out);

    // Pipeline the 2 DMA transfer in order to speed-up the performances:
    // DMA USB -> RAM
    // DMA RAM -> MCI
    //
    while (nb_sector--)
    {
#ifdef TIME_MEASURING_ENABLE
        TIME_MEASURING_Start (TIME_MEASURING_TIME_MSD_AES_WRITE);
#endif
        // Wait for the AES semaphore
        while (pdTRUE != xSemaphoreTake (AES_semphr, 1))
        {
//          CI_StringOut ("�6");
        }

        // (re)load first stage.
        if (buffer_id == 0)
        {
            dma_usb_2_ram (&sector_buf_0, SD_MMC_SECTOR_SIZE);
            buffer_id = 1;
        }
        else
        {
            dma_usb_2_ram (&sector_buf_1, SD_MMC_SECTOR_SIZE);
            buffer_id = 0;
        }


        // (re)load second stage.
        if (b_last_state_full)
        {
            if (buffer_id == 0)
            {
                dma_ram_2_mci (&sector_buf_0, SD_MMC_SECTOR_SIZE, BlockNr_u32);
            }
            else
            {
                dma_ram_2_mci (&sector_buf_1, SD_MMC_SECTOR_SIZE, BlockNr_u32);
            }

            BlockNr_u32++;

            // Wait completion of both stages.
            if (!(busy_wait(is_dma_usb_2_ram_complete) && busy_wait(is_dma_ram_2_mci_complete) )){
                stop_dma();
                BlockNr_u32--;
                nb_sector++;
                buffer_id = (buffer_id+1)%2;
                continue;
            }

            // AES Test
            // STICK20_ram_aes_ram(STICK20_RAM_TO_AES_TO_MCI,SD_MMC_SECTOR_SIZE/4,(unsigned int *)sector_buf_0, pSTICK20_AES_BUFFER);
            // CI_LocalPrintf("-");

        }
        else
        {
            b_last_state_full = TRUE;
            // Wait completion of the first stage only.
            busy_wait (is_dma_usb_2_ram_complete);
        }

        taskENTER_CRITICAL ();
        xSemaphoreGive (AES_semphr);
        taskEXIT_CRITICAL ();

#ifdef TIME_MEASURING_ENABLE
        TIME_MEASURING_Stop (TIME_MEASURING_TIME_MSD_AES_WRITE);
#endif
    }

    // Wait for the AES semaphore
    while (pdTRUE != xSemaphoreTake (AES_semphr, 1))
    {
//      CI_StringOut ("�7");
    }

    // Complete execution of the last transfer (which is in the pipe).
    if (buffer_id == 0)
    {
        dma_ram_2_mci (&sector_buf_1, SD_MMC_SECTOR_SIZE, BlockNr_u32);
    }
    else
    {
        dma_ram_2_mci (&sector_buf_0, SD_MMC_SECTOR_SIZE, BlockNr_u32);
    }

    busy_wait(is_dma_ram_2_mci_complete);

    taskENTER_CRITICAL ();
    xSemaphoreGive (AES_semphr);
    taskEXIT_CRITICAL ();

    // Wait until the USB RAM is empty before disabling the autoswitch feature.
    while (Usb_nb_busy_bank (g_scsi_ep_ms_out) != 0);

    // Turn off exotic USB features that may not work for other devices (at45dbx...)
    Usb_disable_endpoint_bank_autoswitch (g_scsi_ep_ms_out);
    return TRUE;
}
#endif

#if (defined MMC_CARD_SECU_FUNC) && (MMC_CARD_SECU_FUNC == ENABLE)

Bool sd_mmc_mci_lock_unlock (U8 slot, U8 cmd, U8 pwd_len, U8 * password)    // password length in Bytes (possible values : 2,6,14)
{
U8 block_count;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;

    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // Wait Busy Signal
    mci_wait_busy_signal (mci);

    if (FALSE == sd_mmc_set_block_len (slot, (U8) pwd_len + 2))
    {
        return FALSE;
    }

    // Send Lock/Unlock Command
    mci_send_cmd (mci, SD_MMC_LOCK_UNLOCK, 0);

    // check response
    if ((mci_read_response (mci) & MMC_TRAN_STATE_MSK) != MMC_TRAN_STATE)
    {
        return FALSE;
    }

    // Sends the command
    mci_wr_data (mci, cmd);
    // Sends the data
    if (cmd != CMD_FULL_ERASE)
    {
        mci_wr_data (mci, pwd_len); // PWD_LENGTH
    }
    for (block_count = 0; block_count < pwd_len; block_count++) // PWD
    {
        mci_wr_data (mci, *(password + block_count));
    }

    sd_mmc_set_block_len (slot, SD_MMC_SECTOR_SIZE);

    return TRUE;
}

Bool is_sd_mmc_mci_locked (U8 slot)
{
U32 response;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;
    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // ask status
    if (sd_mmc_mci_cmd_send_status (slot) == FALSE)
        return TRUE;
    response = mci_read_response (mci);
    if ((((U8) (response >> 24)) & 0x02) != 0x00)
    {   // Then it is locked
        return TRUE;
    }
    return FALSE;
}

Bool sd_mmc_mci_lock_unlock_failed (U8 slot)
{
U32 response;

    if (slot > MCI_LAST_SLOTS)
        return FALSE;
    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    // ask status
    if (sd_mmc_mci_cmd_send_status (slot) == FALSE)
        return TRUE;
    response = mci_read_response (mci);
    if ((((Byte) (response >> 24)) & 0x01) != 0x00)
    {   // Then it failed
        return FALSE;
    }
    return TRUE;
}
#endif // end FUNC_MMC_CARD_SECU

Bool sd_mmc_mci_cmd_send_status (U8 slot)
{
    if (slot > MCI_LAST_SLOTS)
        return FALSE;
    // Select Slot card before any other command.
    mci_select_card (mci, slot, g_u8_card_bus_width[slot]);

    if (mci_send_cmd (mci, SD_MMC_SEND_STATUS_CMD, g_u32_card_rca[slot]) != MCI_SUCCESS)
        return FALSE;

    return TRUE;
}

/******************************************************************************

  SetSdEncryptedCardEnableState

******************************************************************************/

void SetSdEncryptedCardEnableState (unsigned char cState)
{
    cSdEncryptedCardEnabledByUser = FALSE;

    if (TRUE == cState)
    {
        cSdEncryptedCardEnabledByUser = TRUE;
    }
}

/******************************************************************************

  GetSdEncryptedCardEnableState

******************************************************************************/


unsigned char GetSdEncryptedCardEnableState (void)
{
    return (cSdEncryptedCardEnabledByUser);
}

/******************************************************************************

  SetSdEncryptedCardReadWriteEnableState

******************************************************************************/

void SetSdEncryptedCardReadWriteEnableState (unsigned char cState)
{
    cSdEncryptedCardReadWriteEnabled = READ_ONLY_ACTIVE;

    if (READ_WRITE_ACTIVE == cState)
    {
        cSdEncryptedCardReadWriteEnabled = READ_WRITE_ACTIVE;
    }

    Write_ReadWriteStatusEncryptedVolume_u8 (cSdEncryptedCardReadWriteEnabled);
}

/******************************************************************************

  GetSdEncryptedCardReadWriteEnableState

******************************************************************************/

unsigned char GetSdEncryptedCardReadWriteEnableState (void)
{
    cSdEncryptedCardReadWriteEnabled = Read_ReadWriteStatusEncryptedVolume_u8 ();

    return (cSdEncryptedCardReadWriteEnabled);
}


/******************************************************************************

  SetSdUncryptedCardEnableState

******************************************************************************/

void SetSdUncryptedCardEnableState (unsigned char cState)
{
    cSdUncryptedCardEnabledByUser = FALSE;

    if (TRUE == cState)
    {
        cSdUncryptedCardEnabledByUser = TRUE;
    }
}

/******************************************************************************

  GetSdUncryptedCardEnableState

******************************************************************************/

unsigned char GetSdUncryptedCardEnableState (void)
{
    return (cSdUncryptedCardEnabledByUser);
}

/******************************************************************************

  SetSdUncryptedCardReadWriteEnableState

******************************************************************************/

void SetSdUncryptedCardReadWriteEnableState (unsigned char cState)
{
    cSdUncryptedCardReadWriteEnabled = READ_ONLY_ACTIVE;

    if (READ_WRITE_ACTIVE == cState)
    {
        cSdUncryptedCardReadWriteEnabled = READ_WRITE_ACTIVE;
    }

    Write_ReadWriteStatusUncryptedVolume_u8 (cSdUncryptedCardReadWriteEnabled);
}

/******************************************************************************

  GetSdUncryptedCardReadWriteEnableState

******************************************************************************/

unsigned char GetSdUncryptedCardReadWriteEnableState (void)
{
    cSdUncryptedCardReadWriteEnabled = Read_ReadWriteStatusUncryptedVolume_u8 ();

    return (cSdUncryptedCardReadWriteEnabled);
}



/******************************************************************************

  SetSdEncryptedHiddenState

******************************************************************************/
extern U32 sd_FlagHiddenLun_u32;

void SetSdEncryptedHiddenState (unsigned char cState)
{
    sd_FlagHiddenLun_u32 = FALSE;

    if (TRUE == cState)
    {
        sd_FlagHiddenLun_u32 = SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION;
    }
}

/******************************************************************************

  GetSdEncryptedHiddenState

******************************************************************************/

unsigned char GetSdEncryptedHiddenState (void)
{
    if (SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION == sd_FlagHiddenLun_u32)
    {
        return (TRUE);
    }
    return (FALSE);
}

/******************************************************************************

  GetSdCidInfo

  Changes
  Date      Author          Info
  07.02.14  RB              Implementation: Get CID

  Reviews
  Date      Reviewer        Info

******************************************************************************/

const cid_t* GetSdCidInfo (void)
{
    return (&g_sd_card_cid);
}

/******************************************************************************

  CheckForNewSdCard

  Return
    TRUE  - New SD card found
    FALSE - SD Card is not changed

  Changes
  Date      Author          Info
  08.02.14  RB              Implementation: Check for new SD card

  Reviews
  Date      Reviewer        Info

******************************************************************************/

unsigned char CheckForNewSdCard (void)
{
    u32 OldSdID_u32;
    u32 NewSdID_u32;

    ReadSdId (&OldSdID_u32);

    NewSdID_u32 = (g_sd_card_cid.psnh << 8) + g_sd_card_cid.psnl;

    if (NewSdID_u32 != OldSdID_u32)
    {
        WriteNewSdCardFoundToFlash (&NewSdID_u32);  // Write new SD id into flash
        return (TRUE);
    }

    return (FALSE);
}



#endif // SD_MMC_MCI_0_MEM == ENABLE || SD_MMC_MCI_1_MEM == ENABLE
