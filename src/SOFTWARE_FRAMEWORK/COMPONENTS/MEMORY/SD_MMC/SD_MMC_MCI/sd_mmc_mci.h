/* This header file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief SD/MMC card driver using an MCI interface.
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



#ifndef _SD_MMC_MCI_H_
#define _SD_MMC_MCI_H_


/*_____ I N C L U D E S ____________________________________________________*/

#include "conf_access.h"

#if SD_MMC_MCI_0_MEM == DISABLE && SD_MMC_MCI_1_MEM == DISABLE
  #error sd_mmc_mci.h is #included although SD_MMC_MCI_x_MEM are all disabled
#endif


#include "compiler.h"
#include "mci.h"
#include "sd_mmc_cmd.h"


/*_____ M A C R O S ________________________________________________________*/

//! Number of bits for addresses within sectors.
#define SD_MMC_SECTOR_BITS     9

//! Sector size in bytes.
#define SD_MMC_SECTOR_SIZE     (1 << SD_MMC_SECTOR_BITS)

/*_____ D E F I N I T I O N ________________________________________________*/

//! Card identification
#define UNKNOWN_CARD                   0x00
#define MMC_CARD                       0x01
#define SD_CARD                        0x02
#define MMC_CARD_V4                    0x04
#define SD_CARD_V2                     0x08
#define SD_CARD_HC                     0x10
#define MMC_CARD_HC                    0x20

//! Card state
#define CARD_UNINSTALLED                  0
#define CARD_INSTALLED                    1

//! OCR register
#define OCR_MSK_BUSY              0x80000000 // Busy flag
#define OCR_MSK_HC                0x40000000 // High Capacity flag
#define OCR_MSK_VOLTAGE_3_2V_3_3V 0x00100000 // Voltage 3.2V to 3.3V flag
#define OCR_MSK_VOLTAGE_ALL       0x00FF8000 // All Voltage flag

//! RCA register
#define RCA_RESERVE_ADR           0x00000000
#define RCA_MSK_ADR               0xFFFF0000
#define RCA_DEFAULT_ADR           0x0001FFFF // It can be changed

//! CSD register
#define CSD_REG_SIZE              0x10
#define CSD_STRUCT_1_0            0x00
#define CSD_STRUCT_1_1            0x40
#define CSD_STRUCT_1_2            0x80
#define CSD_STRUCT_SUP            0xC0
#define CSD_MSK_STRUCT            0xC0
#define CSD_SPEC_VER_1_0          0x00
#define CSD_SPEC_VER_1_4          0x04
#define CSD_SPEC_VER_2_0          0x08
#define CSD_SPEC_VER_3_1          0x0C
#define CSD_SPEC_VER_4_0          0x10
#define CSD_MSK_SPEC_VER          0x3C
#define CSD_MSK_RBP               0x80
#define CSD_MSK_WBM               0x40
#define CSD_MSK_RBM               0x20
#define CSD_MSK_RBL               0x0F
#define CSD_MSK_CSH               0x03
#define CSD_MSK_CSL               0xC0
#define CSD_MSK_CSM               0x07
#define CSD_MSK_CSMH              0x03
#define CSD_MSK_CSML              0x80
#define CSD_MSK_WBL               0x03
#define CSD_MSK_WBH               0xC0
#define CSD_MSK_WBP               0x20
#define CSD_BLEN_2048             11
#define CSD_BLEN_512              9

//! MMC Status Mask
#define MMC_STBY_STATE_MSK        ((U32)0x01D81E00)
#define MMC_STBY_STATE            ((U32)0x00000600)  // stby state
#define MMC_DATA_STATE_MSK        ((U32)0xE0040E00)
#define MMC_DATA_STATE            ((U32)0x00000A00)  // data state
#define MMC_RCV_STATE_MSK         ((U32)0xE0020E00)
#define MMC_RCV_STATE             ((U32)0x00000A00)  // rcv state
#define MMC_TRAN_STATE_MSK        ((U32)0xE0020E00)
#define MMC_TRAN_STATE            ((U32)0x00000800)  // tran state

//! Flag error of "Card Status" in R1
#define CS_FLAGERROR_RD_WR  (CS_ADR_OUT_OF_RANGE|CS_ADR_MISALIGN|CS_BLOCK_LEN_ERROR|CS_ERASE_SEQ_ERROR|CS_ILLEGAL_COMMAND|CS_CARD_ERROR)
#define CS_ADR_OUT_OF_RANGE (1<<31)
#define CS_ADR_MISALIGN     (1<<30)
#define CS_BLOCK_LEN_ERROR  (1<<29)
#define CS_ERASE_SEQ_ERROR  (1<<28)
#define CS_ERASE_PARAM      (1<<27)
#define CS_WP_VIOLATION     (1<<26)
#define CS_CARD_IS_LOCKED   (1<<25)
#define CS_LOCK_UNLOCK_     (1<<24)
#define CS_COM_CRC_ERROR    (1<<23)
#define CS_ILLEGAL_COMMAND  (1<<22)
#define CS_CARD_ECC_FAILED  (1<<21)
#define CS_CARD_ERROR       (1<<20)
#define CS_EXEC_ERROR       (1<<19)
#define CS_UNDERRUN         (1<<18)
#define CS_OVERRUN          (1<<17)
#define CS_CIDCSD_OVERWRITE (1<<16)
#define CS_WP_ERASE_SKIP    (1<<15)
#define CS_ERASE_RESET      (1<<13)
#define CS_READY_FOR_DATA   (1<<8)
#define CS_SWITCH_ERROR     (1<<7)
#define CS_APP_CMD          (1<<5)

//! SD bus width
#define SD_BUS_1_BIT      MCI_BUS_SIZE_1_BIT
#define SD_BUS_4_BIT      MCI_BUS_SIZE_4_BIT
#define SD_BUS_8_BIT      MCI_BUS_SIZE_8_BIT

// MMC commands (taken from MMC reference)
#define MMC_GO_IDLE_STATE                 0    ///< initialize card to SPI-type access
#define MMC_SEND_OP_COND                  1    ///< set card operational mode
#define MMC_CMD2                          2    ///< illegal in SPI mode !
#define MMC_SEND_CSD                      9    ///< get card's CSD
#define MMC_SEND_CID                      10    ///< get card's CID
#define MMC_SEND_STATUS                   13
#define MMC_SET_BLOCKLEN                  16    ///< Set number of bytes to transfer per block
#define MMC_READ_SINGLE_BLOCK             17    ///< read a block
#define MMC_WRITE_BLOCK                   24    ///< write a block
#define MMC_PROGRAM_CSD                   27
#define MMC_SET_WRITE_PROT                28
#define MMC_CLR_WRITE_PROT                29
#define MMC_SEND_WRITE_PROT               30
#define SD_TAG_WR_ERASE_GROUP_START       32
#define SD_TAG_WR_ERASE_GROUP_END         33
#define MMC_TAG_SECTOR_START              32
#define MMC_TAG_SECTOR_END                33
#define MMC_UNTAG_SECTOR                  34
#define MMC_TAG_ERASE_GROUP_START         35    ///< Sets beginning of erase group (mass erase)
#define MMC_TAG_ERASE_GROUP_END           36    ///< Sets end of erase group (mass erase)
#define MMC_UNTAG_ERASE_GROUP             37    ///< Untag (unset) erase group (mass erase)
#define MMC_ERASE                         38    ///< Perform block/mass erase
#define SD_SEND_OP_COND_ACMD              41    ///< Same as MMC_SEND_OP_COND but specific to SD (must be preceeded by CMD55)
#define MMC_LOCK_UNLOCK                   42    ///< To start a lock/unlock/pwd operation
#define SD_APP_CMD55                      55    ///< Use before any specific command (type ACMD)
#define MMC_CRC_ON_OFF                    59    ///< Turns CRC check on/off
// R1 Response bit-defines
#define MMC_R1_BUSY                       0x80  ///< R1 response: bit indicates card is busy
#define MMC_R1_PARAMETER                  0x40
#define MMC_R1_ADDRESS                    0x20
#define MMC_R1_ERASE_SEQ                  0x10
#define MMC_R1_COM_CRC                    0x08
#define MMC_R1_ILLEGAL_COM                0x04
#define MMC_R1_ERASE_RESET                0x02
#define MMC_R1_IDLE_STATE                 0x01
// Data Start tokens
#define MMC_STARTBLOCK_READ               0xFE  ///< when received from card, indicates that a block of data will follow
#define MMC_STARTBLOCK_WRITE              0xFE  ///< when sent to card, indicates that a block of data will follow
#define MMC_STARTBLOCK_MWRITE             0xFC
// Data Stop tokens
#define MMC_STOPTRAN_WRITE                0xFD
// Data Error Token values
#define MMC_DE_MASK                       0x1F
#define MMC_DE_ERROR                      0x01
#define MMC_DE_CC_ERROR                   0x02
#define MMC_DE_ECC_FAIL                   0x04
#define MMC_DE_OUT_OF_RANGE               0x04
#define MMC_DE_CARD_LOCKED                0x04
// Data Response Token values
#define MMC_DR_MASK                       0x1F
#define MMC_DR_ACCEPT                     0x05
#define MMC_DR_REJECT_CRC                 0x0B
#define MMC_DR_REJECT_WRITE_ERROR         0x0D
// Arguments of MMC_SWITCH command
#define MMC_SWITCH_WRITE          ((U8)03)
#define MMC_SWITCH_BUS_WIDTH      ((U8)183)
#define MMC_SWITCH_HIGH_SPEED     ((U8)185)
#define MMC_SWITCH_CMD_SET        ((U8)03)
#define MMC_SWITCH_VAL_LS         ((U8)00)
#define MMC_SWITCH_VAL_HS         ((U8)01)
#define MMC_SWITCH_VAL_1BIT       ((U8)00)
#define MMC_SWITCH_VAL_4BIT       ((U8)01)
#define MMC_SWITCH_VAL_8BIT       ((U8)02)
// Arguments of MMC_LOCK_UNLOCK command
#define CMD_FULL_ERASE           0x08
#define CMD_UNLOCK               0x00
#define CMD_CLEAR                0x02
#define CMD_LOCK                 0x01


/*_____ D E C L A R A T I O N ______________________________________________*/

typedef struct
{
  union
  {
    unsigned int cardStatus;
    struct
    {
      unsigned int outOfRange       :1;
      unsigned int addressserror    :1;
      unsigned int blockLenError    :1;
      unsigned int eraseSeqErro     :1;
      unsigned int eraseParam       :1;
      unsigned int wpViolation      :1;
      unsigned int cardIsLocked     :1;
      unsigned int lockUnlockFailed :1;
      unsigned int comCrcError      :1;
      unsigned int illegalCommand   :1;
      unsigned int cardEccFailed    :1;
      unsigned int ccError          :1;
      unsigned int error            :1;
      unsigned int                  :2;
      unsigned int csdOverwrite     :1;
      unsigned int wpEraseSkip      :1;
      unsigned int cardEccDisabled  :1;
      unsigned int eraseReset       :1;
      unsigned int currentState     :4;
      unsigned int readyForData     :1;
      unsigned int                  :2;
      unsigned int appCmd           :1;
      unsigned int                  :1;
      unsigned int akeSeqError      :1;
      unsigned int                  :3;
    };
  };
}card_status_t;


typedef struct
{
  union
  {
    unsigned int ocr;
    struct
    {
      unsigned int busy   :1;
      unsigned int ccs    :1;
      unsigned int        :6;
      unsigned int vRange :9;
      unsigned int        :15;
    };
  };
}ocr_t;

typedef struct
{
  //CDS[0]
  unsigned int csdStructure       :2;
  unsigned int                    :6;
  unsigned int taac               :8;
  unsigned int nsac               :8;
  unsigned int tranSpeed          :8;
  //CSD[1]
  unsigned int ccc                :12;
  unsigned int readBlLen          :4;
  unsigned int readBlePartial     :1;
  unsigned int writeBlkMisalign   :1;
  unsigned int readBlkMisalign    :1;
  unsigned int dsrImp             :1;
  unsigned int                    :2;
  unsigned int deviceSizeH        :10;
  //CSD[2]
  unsigned int deviceSizeL        :2;
  unsigned int vddRCurrMin        :3;
  unsigned int vddRCurrMax        :3;
  unsigned int vddWCurrMin        :3;
  unsigned int vddWCurrMax        :3;
  unsigned int cSizeMult          :3;
  unsigned int eraseBlkEn         :1;
  unsigned int sectorSize         :7;
  unsigned int wpGrpSize          :7;
  //CSD[3]
  unsigned int wpGrpEnable        :1;
  unsigned int                    :2;
  unsigned int r2wFactor          :3;
  unsigned int writeBlLen         :4;
  unsigned int writeBlPartial     :1;
  unsigned int                    :5;
  unsigned int fileFormatGrp      :1;
  unsigned int copy               :1;
  unsigned int permWriteProtect   :1;
  unsigned int tmpWriteProtect    :1;
  unsigned int fileFormat         :2;
  unsigned int                    :2;
  unsigned int crc                :7;
  unsigned int                    :1;
}csd_v1_t;


typedef struct
{
  //CDS[0]
  unsigned int csdStructure       :2;
  unsigned int                    :6;
  unsigned int taac               :8;
  unsigned int nsac               :8;
  unsigned int tranSpeed          :8;
  //CSD[1]
  unsigned int ccc                :12;
  unsigned int readBlLen          :4;
  unsigned int readBlePartial     :1;
  unsigned int writeBlkMisalign   :1;
  unsigned int readBlkMisalign    :1;
  unsigned int dsrImp             :1;
  unsigned int                    :6;
  unsigned int deviceSizeH        :6;
  //CSD[2]
  unsigned int deviceSizeL        :16;
  unsigned int                    :1;
  unsigned int eraseBlkEn         :1;
  unsigned int sectorSize         :7;
  unsigned int wpGrpSize          :7;
  //CSD[3]
  unsigned int wpGrpEnable        :1;
  unsigned int                    :2;
  unsigned int r2wFactor          :3;
  unsigned int writeBlLen         :4;
  unsigned int writeBlPartial     :1;
  unsigned int                    :5;
  unsigned int fileFormatGrp      :1;
  unsigned int copy               :1;
  unsigned int permWriteProtect   :1;
  unsigned int tmpWriteProtect    :1;
  unsigned int fileFormat         :2;
  unsigned int                    :2;
  unsigned int crc                :7;
  unsigned int                    :1;
}csd_v2_t;


typedef struct
{
  union
  {
    unsigned int csd[4];
    union
    {
      csd_v1_t csd_v1;
      csd_v2_t csd_v2;
    };
  };
}csd_t;

typedef struct
{
  union
  {
    unsigned int cid[4];
    struct
    {
      unsigned int mid  :8; // Manufacturer ID
      unsigned int oid  :16; // OEM/Application ID
      unsigned int pnmh :8; // Product name high bits
      unsigned int pnml :32;  // Product name low bits
      unsigned int prv  :8; // Product revision
      unsigned int psnh :24; // Product serial number
      unsigned int psnl :8; // Product serial number
      unsigned int      :4;
      unsigned int mdt  :12; // Manufacturing date
      unsigned int crc  :7; // CRC7 Checksum
      unsigned int      :1;
    };
  };
}cid_t;



typedef struct
{
  union
  {
    unsigned int  rep1[2];
    struct
    {
    unsigned int startBit   :1;
    unsigned int transBit   :1;
    unsigned int cmdIndex   :6;
    card_status_t cardStatus;
    unsigned int crc7       :6;
    unsigned int endBit     :1;
    unsigned int pad        :16;
    };
  };
}rep1_t;


typedef struct
{
  union
  {
    unsigned int  rep2[5];
    struct
    {
    union
    {
      cid_t cid;
      csd_t csd;
    };
    unsigned int endBit   :1;
    unsigned int pad      :32;
    };
  };
}rep2_t;


typedef struct
{
  union
  {
    unsigned int  rep3[2];
    struct
    {
    unsigned int startBit :1;
    unsigned int transBit :1;
    unsigned int          :6;
    ocr_t ocr;
    unsigned int          :7;
    unsigned int endBit   :1;
    unsigned int pad      :16;
    };
  };
}rep3_t;


//_____ D E C L A R A T I O N S ____________________________________________

/*! \name Control Functions
 */
//! @{

/*! \brief Initializes the MCI driver.
 *
 * \param mci_opt     Initialization options of the MCI.
 * \param pbb_hz      MCI module input clock frequency (PBA clock, Hz).
 * \param cpu_hz      CPU clock, Hz.
 *
 * \retval OK Success.
 * \retval KO Failure.
 */
extern Bool sd_mmc_mci_init(const mci_options_t *mci_opt, long pbb_hz, long cpu_hz);

/*! \brief Initializes the SD/MMC card.
 *
 * \param slot        Slot number.
 *
 * \retval OK Success.
 * \retval KO Failure.
 */
extern Bool sd_mmc_mci_card_init(U8 slot);

/*! \brief Performs a card check presence.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \retval TRUE Card Present
 * \retval FALSE Card Unpresent.
 */
extern Bool is_sd_mmc_mci_card_present(U8 slot);

/*! \brief Performs a card check protection.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \retval TRUE Card Protected
 * \retval FALSE Card Unprotected.
 */
extern Bool is_sd_mmc_mci_card_protected(U8 slot);

/*! \brief Performs a memory check.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \retval OK Success.
 * \retval KO Failure.
 */
extern Bool sd_mmc_mci_mem_check(U8 slot);

/*! \brief Opens a DF memory in read mode at a given sector.
 *
 * \param slot       SD/MMC Slot Card Selected.
 * \param sector     Start sector.
 * \param nb_sector  Number of sector.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 * \note Sector may be page-unaligned (depending on the DF page size).
 */
extern Bool sd_mmc_mci_read_open(U8 slot, U32 sector, U16 nb_sector);

/*! \brief Opens a DF memory in read mode at a given sector. DMA will be used.
 *
 * \param slot       SD/MMC Slot Card Selected.
 * \param sector     Start sector.
 * \param ram        pointer on ram buffer.
 * \param nb_sector  Number of sector.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 * \note Sector may be page-unaligned (depending on the DF page size).
 */
extern Bool sd_mmc_mci_dma_read_open(U8 slot, U32 sector, void* ram, U16 nb_sector );

/*! \brief Unselects the current DF memory.
 * \param slot SD/MMC Slot Card Selected.
 *
 * \retval OK Success.
 * \retval KO Failure.
 */
extern Bool sd_mmc_mci_read_close(U8 slot);

/*! \brief This function opens a DF memory in write mode at a given sector.
 *
 * \param slot       SD/MMC Slot Card Selected.
 * \param sector     Start sector.
 * \param nb_sector  Number of sector.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 * \note Sector may be page-unaligned (depending on the SD/MMC page size).
 *
 */
extern Bool sd_mmc_mci_write_open(U8 slot, U32 sector, U16 nb_sector);

/*! \brief This function opens a DF memory in write mode at a given sector. DMA will be used.
 *
 * \param slot       SD/MMC Slot Card Selected.
 * \param sector     Start sector.
 * \param nb_sector  Number of sector.
 * \param ram        pointer on ram buffer.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 * \note Sector may be page-unaligned (depending on the SD/MMC page size).
 *
 */
extern Bool sd_mmc_mci_dma_write_open(U8 slot, U32 sector, const void* ram, U16 nb_sector);

/*! \brief Fills the end of the current logical sector and launches page programming.
 * \param slot SD/MMC Slot Card Selected.
 *
 * \retval OK Success.
 * \retval KO Failure.
 */
extern Bool sd_mmc_mci_write_close(U8 slot);

//! @}


/*! \name Multiple-Sector Access Functions
 */
//! @{

/*! \brief Reads \a nb_sector sectors from DF memory.
 *
 * Data flow is: SD/MMC -> callback.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param nb_sector Number of contiguous sectors to read.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 * \note First call must be preceded by a call to the \ref sd_mmc_mci_read_open
 *       function.
 *
 */
extern Bool sd_mmc_mci_read_multiple_sector(U8 slot, U16 nb_sector,U32 StartBlockNr_u32);

/*! \brief Callback function invoked after each sector read during
 *         \ref sd_mmc_mci_read_multiple_sector.
 *
 * \param psector Pointer to read sector.
 */
extern void sd_mmc_mci_read_multiple_sector_callback(const void *psector);

/*! \brief Writes \a nb_sector sectors to SD/MMC memory.
 *
 * Data flow is: callback -> SD/MMC.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param nb_sector Number of contiguous sectors to write.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 * \note First call must be preceded by a call to the \ref sd_mmc_mci_write_open
 *       function.
 *
 */
extern Bool sd_mmc_mci_write_multiple_sector(U8 slot, U16 nb_sector,U32 StartBlockNr_u32);

/*! \brief Callback function invoked before each sector write during
 *         \ref sd_mmc_mci_write_multiple_sector.
 *
 * \param psector Pointer to sector to write.
 */
extern void sd_mmc_mci_write_multiple_sector_callback(void *psector);

/*! \brief Reads nb_sector SD/MMC sector to a RAM buffer.
 *
 * Data flow is: SD/MMC -> RAM.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param ram Pointer to RAM buffer.
 * \param nb_sector Number of sector to read.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 */
extern Bool sd_mmc_mci_read_multiple_sector_2_ram(U8 slot, void *ram, U32 nb_sector);

/*! \brief Reads nb_sector SD/MMC sector to a RAM buffer, using the DMA.
 *
 * Data flow is: SD/MMC -> RAM.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param ram Pointer to RAM buffer.
 * \param nb_sector Number of sector to read.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 */
extern Bool sd_mmc_mci_dma_read_multiple_sector_2_ram(U8 slot, void *ram, U32 nb_sector);

/*! \brief Writes nb_sector SD/MMC sector from a RAM buffer.
 *
 * Data flow is: RAM -> SD/MMC.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param ram Pointer to RAM buffer.
 * \param nb_sector Number of sector to write.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 */
extern Bool sd_mmc_mci_write_multiple_sector_from_ram(U8 slot, const void *ram, U32 nb_sector);

/*! \brief Writes nb_sector SD/MMC sector from a RAM buffer, using the DMA.
 *
 * Data flow is: RAM -> SD/MMC.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param ram Pointer to RAM buffer.
 * \param nb_sector Number of sector to write.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 */
extern Bool sd_mmc_mci_dma_write_multiple_sector_from_ram(U8 slot, const void *ram, U32 nb_sector);

//! @}


/*! \name Single-Sector Access Functions
 */
//! @{

/*! \brief Reads 1 SD/MMC sector to a RAM buffer.
 *
 * Data flow is: SD/MMC -> RAM.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param ram Pointer to RAM buffer.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 */
extern Bool sd_mmc_mci_read_sector_2_ram(U8 slot, void *ram);

/*! \brief Reads 1 SD/MMC sector to a RAM buffer, using the DMA.
 *
 * Data flow is: SD/MMC -> RAM.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param ram Pointer to RAM buffer.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 */
extern Bool sd_mmc_mci_dma_read_sector_2_ram(U8 slot, void *ram);

/*! \brief Writes 1 SD/MMC sector from a RAM buffer.
 *
 * Data flow is: RAM -> SD/MMC.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param ram Pointer to RAM buffer.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 */
extern Bool sd_mmc_mci_write_sector_from_ram(U8 slot, const void *ram);

/*! \brief Writes 1 SD/MMC sector from a RAM buffer, using the DMA.
 *
 * Data flow is: RAM -> SD/MMC.
 *
 * \param slot SD/MMC Slot Card Selected.
 * \param ram Pointer to RAM buffer.
 *
 * \retval OK Success.
 * \retval KO Failure.
 *
 */
extern Bool sd_mmc_mci_dma_write_sector_from_ram(U8 slot, const void *ram);
//! @}

/*! \name Protection Access Functions
 */
//! @{

/*! \brief This function sends lock/unlock commands for sd or mmc.
 *
 * \param slot          SD/MMC Slot Card Selected.
 * \param cmd           CMD_FULL_ERASE ; CMD_LOCK ; CMD_UNLOCK..
 * \param pwd_len       password length in bytes (limited to 253).
 * \param password      the password content.
 *
 *
 * \retval TRUE Command successfull.
 * \retval FALSE Command failed.
 *
 */
extern Bool sd_mmc_mci_lock_unlock (U8 slot, U8 cmd, U8 pwd_len, U8 * password);

/*! \brief Get sd status register and look if card is locked by a password.
 *
 * \param slot SD/MMC Slot Card Selected.
 *
 * \retval TRUE Card is locked.
 * \retval FALSE Card is unlocked.
 *
 */
extern Bool is_sd_mmc_mci_locked(U8 slot);

/*! \brief Get sd status register and look if the lock/unlock command was ok.
 *
 * \param slot SD/MMC Slot Card Selected.
 *
 * \retval TRUE Lock/Unlock failed.
 * \retval FALSE Lock/Unlock was successfull.
 *
 */
extern Bool sd_mmc_mci_lock_unlock_failed(U8 slot);

/*! \brief ask mmc status register.
 *
 * \param slot SD/MMC Slot Card Selected.
 *
 * \retval TRUE Success.
 * \retval FALSE Failure.
 *
 */
extern Bool  sd_mmc_mci_cmd_send_status(U8 slot);
//! @}


extern void SetSdEncryptedCardEnableState (unsigned char cState);
extern unsigned char GetSdEncryptedCardEnableState (void);

void SetSdUncryptedCardEnableState (unsigned char cState);
unsigned char GetSdUncryptedCardEnableState (void);

void SetSdUncryptedCardReadWriteEnableState (unsigned char cState);
unsigned char GetSdUncryptedCardReadWriteEnableState (void);

void SetSdEncryptedHiddenState (unsigned char cState);

void dma_mci_2_ram(void *ram, size_t size, unsigned int BlockNr_u32);
void dma_ram_2_mci(const void *ram, size_t size, unsigned int BlockNr_u32);
void dma_ram_2_mci_crypted (const void *ram, size_t size, unsigned int BlockNr_u32);
void dma_ram_2_mci_uncrypted (const void *ram, size_t size, unsigned int BlockNr_u32);
Bool is_dma_ram_2_mci_complete( void );

const cid_t *GetSdCidInfo (void);
unsigned char CheckForNewSdCard (void);
unsigned char GetSdEncryptedHiddenState (void);

#endif  // _SD_MMC_MCI_H_
