/* This header file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief SD/MMC configuration file.
 *
 * This file contains the possible external configuration of the SD/MMC.
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

#ifndef _CONF_SD_MMC_MCI_H_
#define _CONF_SD_MMC_MCI_H_


#include "conf_access.h"

#if SD_MMC_MCI_0_MEM == DISABLE && SD_MMC_MCI_1_MEM == DISABLE
  #error conf_sd_mmc_mci.h is #included although SD_MMC_MCI_x_MEM are all disabled
#endif


#include "sd_mmc_mci.h"

#define SD_4_BIT    ENABLE

#define SD_SLOT    0

#define SD_SIZE_UNCRYPTED_PARITION                    (1024*2*128)      // SD Blocks a 512 Bytes = 128 MB
#define SD_MAGIC_NUMBER_HIDDEN_CRYPTED_PARITION       0x23636923        // u32 for secure
#define SD_START_HIDDEN_CRYPTED_PARITION              (1024*2*1024)     // SD Blocks a 512 Bytes = 2048 MB

extern U32 gSdStartUncryptedVolume_u32;
extern U32 gSdEndUncryptedVolume_u32;

extern U32 gSdStartCryptedVolume_u32;
extern U32 gSdEndCryptedVolume_u32;

extern U32 gSdStartHiddenVolume_u32;
extern U32 gSdEndHiddenVolume_u32;


#endif  // _CONF_SD_MMC_MCI_H_
