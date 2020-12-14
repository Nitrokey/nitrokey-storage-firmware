/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/* This file has been prepared for Doxygen automatic documentation generation. */
/* ! \file ********************************************************************* \brief DMACA RAM -> AES -> RAM evaluation for AVR32 UC3. -
   Compiler: IAR EWAVR32 and GNU GCC for AVR32 - Supported devices: All AVR32 devices with an AES and a DMACA - AppNote: \author Atmel Corporation:
   http://www.atmel.com \n Support and FAQ: http://support.atmel.no/ *************************************************************************** */
/* ! \page License Copyright (c) 2009 Atmel Corporation. All rights reserved. Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer. 2. Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 3. The name of Atmel
   may not be used to endorse or promote products derived from this software without specific prior written permission. 4. This software may only be
   redistributed and used in connection with an Atmel AVR product. THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE EXPRESSLY AND
   SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
   OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE */
 /* ! \mainpage * \section intro Introduction * This is the documentation for the data structures, functions, variables, defines, enums, and *
    typedefs for the DMACA driver. <BR>It also gives an example of the usage of the * DMACA module, eg: <BR> * - [on EVK1104S * * \section files Main
    Files * - dmaca.h : DMACA header file * - ram_aes_ram_example.c : DMACA code example wich benchmark transfer rates of the following path: * -
    CPUSRAM to AES to CPUSRAM * - HSBSRAM0 to AES to HSBSRAM1 * * \section compinfo Compilation Info * This software was written for the GNU GCC for
    AVR32 and IAR Systems compiler * for AVR UC3. Other compilers may or may not work. * * \section deviceinfo Device Info * All AVR UC3 devices with
    a ADC module can be used. This example has been tested * with the following setup:<BR> * - EVK1104 evaluation kit with AT32UC3A3256S. * *
    \section setupinfo Setup Information * <BR>CPU speed: <i> DMACA_AES_EVAL_CPU_FREQ MHz </i> * DMACA_AES_EVAL_CPU_FREQ is given in the confg.mk,
    the default value is 12000000, but it can * be set to 66000000 to see the impact on the frequency on transfer rate * * - [on EVK1104 only]
    Connect a PC USB cable to the USB VCP plug (the USB plug * on the right) of the EVK1104. The PC is used as a power source. The UC3A3256S * USART1
    is connected to the UC3B USART1. The UC3B holds a firmware that acts as * a USART to USB gateway. On the USB side, the UC3B firmware implements a
    USB * CDC class: when connected to a PC, it will enumerate as a Virtual Com Port. * Once the UC3B USB is correctly installed on Windows, to
    communicate on this * port, open a HyperTerminal configured with the following settings: 57600 bps, * 8 data bits, no parity bit, 1 stop bit, no
    flow control. * * \section contactinfo Contact Information * For further information, visit * <A
    href="http://www.atmel.com/products/AVR32/">Atmel AVR UC3</A>.\n * Support and FAQ: http://support.atmel.no/ */

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


#include <string.h>

#include "board.h"
#include "global.h"
#include "print_funcs.h"
#include "gpio.h"
#include "pm.h"
#include "intc.h"
#include "usart.h"
#include "aes.h"
#include "xxHash.h"
#include "tools.h"
#include "HighLevelFunctions/FlashStorage.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* AES */
#define AVR32_AES_ADDRESS                  0xFFFD0000
#define AVR32_AES                          (*((volatile avr32_aes_t*)AVR32_AES_ADDRESS))
#define AVR32_AES_IRQ                      672

#include "avr32/aes_1231.h"


Bool is_dma_usb_2_ram_complete (void);
Bool is_dma_mci_2_ram_complete (void);
Bool is_dma_ram_2_usb_complete (void);
Bool is_dma_ram_2_mci_complete (void);

extern volatile xSemaphoreHandle AES_semphr;     // AES semaphore

/* ! \name Board-Related Example Settings */
// ! @{
#if BOARD == EVK1104
#  define DMACA_AES_EVAL_USART               (&AVR32_USART1)
#  define DMACA_AES_EVAL_USART_RX_PIN        AVR32_USART1_RXD_0_0_PIN
#  define DMACA_AES_EVAL_USART_RX_FUNCTION   AVR32_USART1_RXD_0_0_FUNCTION
#  define DMACA_AES_EVAL_USART_TX_PIN        AVR32_USART1_TXD_0_0_PIN
#  define DMACA_AES_EVAL_USART_TX_FUNCTION   AVR32_USART1_TXD_0_0_FUNCTION
#  define DMACA_AES_EVAL_USART_BAUDRATE      57600
#  define DMACA_AES_EVAL_LED1                LED0_GPIO
#  define DMACA_AES_EVAL_LED2                LED1_GPIO
#  define DMACA_AES_EVAL_LED3                LED2_GPIO
#endif

#if !defined(DMACA_AES_EVAL_USART)             || \
    !defined(DMACA_AES_EVAL_USART_RX_PIN)      || \
    !defined(DMACA_AES_EVAL_USART_RX_FUNCTION) || \
    !defined(DMACA_AES_EVAL_USART_TX_PIN)      || \
    !defined(DMACA_AES_EVAL_USART_TX_FUNCTION) || \
    !defined(DMACA_AES_EVAL_USART_BAUDRATE)    || \
    !defined(DMACA_AES_EVAL_LED1)              || \
    !defined(DMACA_AES_EVAL_LED2)              || \
    !defined(DMACA_AES_EVAL_LED3)
#  error The USART and LEDs configuration to use for debug on your board is missing
#endif
// ! @}

// Global AES Enable switch
#ifdef STICK_20_AES_ENABLE

#define   DMACA_AES_EVAL_BUF_SIZE             256

#define   DMACA_AES_EVAL_REFBUF_SIZE          16

// Reference input data.
const unsigned int RefInputData[DMACA_AES_EVAL_REFBUF_SIZE] = {
    0x6bc1bee2,
    0x2e409f96,
    0xe93d7e11,
    0x7393172a,
    0xae2d8a57,
    0x1e03ac9c,
    0x9eb76fac,
    0x45af8e51,
    0x30c81c46,
    0xa35ce411,
    0xe5fbc119,
    0x1a0a52ef,
    0xf69f2445,
    0xdf4f9b17,
    0xad2b417b,
    0xe66c3710
};

// CipherKey array
// the_key = 256'h603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4;
unsigned int CipherKey[8] = {
    0x603deb10,
    0x15ca71be,
    0x2b73aef0,
    0x857d7781,
    0x1f352c07,
    0x3b6108d7,
    0x2d9810a3,
    0x0914dff4
};

// InitVector array
// Initial Value 128'h000102030405060708090a0b0c0d0e0f
const unsigned int InitVector[4] = {
    0x00010203,
    0x04050607,
    0x08090a0b,
    0x0c0d0e0f
};

// InitVector array
unsigned int InitVectorSD[4] = {
    0,
    0,
    0,
    0
};


// Reference output data.
const unsigned int RefOutputData[DMACA_AES_EVAL_REFBUF_SIZE] = {
    0xf58c4c04,
    0xd6e5f1ba,
    0x779eabfb,
    0x5f7bfbd6,
    0x9cfc4e96,
    0x7edb808d,
    0x679f777b,
    0xc6702c7d,
    0x39f23369,
    0xa9d9bacf,
    0xa530e263,
    0x04231461,
    0xb2eb05e2,
    0xc39be9fc,
    0xda6c1907,
    0x8c6a9d1b
};

unsigned int InputData[DMACA_AES_EVAL_BUF_SIZE];    // Input data array
volatile unsigned int OutputData[DMACA_AES_EVAL_BUF_SIZE];  // Output data array

unsigned int* pSrcData_HsbSram; // Src data array in HSBSRAM
volatile unsigned int* pDstData_HsbSram;    // Dst data array in HSBSRAM

volatile unsigned int ccountt0, ccountt1;   // Performance evaluation variables

#define DMACA_AES_EVAL_CPU_FREQ 60000000

pm_freq_param_t pm_freq_param_dma = {
    .cpu_f = DMACA_AES_EVAL_CPU_FREQ,
    .pba_f = DMACA_AES_EVAL_CPU_FREQ,
    .osc0_f = FOSC0,
    .osc0_startup = OSC0_STARTUP
};

// ! 1) Configure two DMACA channels:
// ! - RAM -> AES
// ! - AES -> RAM
// ! 2) Set the AES cryptographic key and init vector.
// ! 3) Start the process
// ! 4) Check the result on the first 16 Words.



#define STICK20_RAM_TO_AES_TO_MCI				0
#define STICK20_MCI_TO_AES_TO_RAM				1


void STICK20_ram_aes_ram (unsigned char cMode, unsigned short int u16BufferSize, unsigned int* pSrcBuf, unsigned int* pDstBuf)
{
    // unsigned int i;
    // unsigned char TestResult = TRUE;
    aes_config_t AesConf;       // AES config structure

    // Wait for the semaphore
    while (pdTRUE != xSemaphoreTake (AES_semphr, 1))
    {
//      CI_StringOut ("�1");
    }

    taskENTER_CRITICAL ();

    // ====================
    // Configure the AES.
    // ====================
    switch (cMode)
    {
        case STICK20_RAM_TO_AES_TO_MCI:
            AesConf.ProcessingMode = AES_PMODE_CIPHER;
            break;
        case STICK20_MCI_TO_AES_TO_RAM:
            AesConf.ProcessingMode = AES_PMODE_DECIPHER;
            break;
    }
    AesConf.ProcessingDelay = 0;    // No delay: best performance
    AesConf.StartMode = AES_START_MODE_DMA; // DMA mode
    AesConf.KeySize = AES_KEY_SIZE_256; // 256bit cryptographic key
    AesConf.OpMode = AES_CBC_MODE;  // CBC cipher mode
    AesConf.LodMode = 0;    // LODMODE == 0 : the end of the
    // encryption is notified by the DMACA transfer complete interrupt. The output
    // is available in the OutputData[] buffer.
    AesConf.CFBSize = 0;    // Don't-care because we're using the CBC mode.
    AesConf.CounterMeasureMask = 0; // Disable all counter measures.
    aes_configure (&AVR32_AES, &AesConf);



    // ====================
    // Configure the DMACA.
    // ====================
    // Enable the DMACA
    AVR32_DMACA.dmacfgreg = 1 << AVR32_DMACA_DMACFGREG_DMA_EN_OFFSET;

    // *
    // * Configure the DMA RAM -> AES channel.
    // *
    // ------------+--------+------+------------+--------+-------+----------------
    // Transfer | Source | Dest | Flow | Width | Chunk | Buffer
    // type | | | controller | (bits) | size | Size
    // ------------+--------+------+------------+--------+-------+----------------
    // | | | | | |
    // Mem-to-Per | RAM | AES | DMACA | 32 | 4 | u16BufferSize
    // | | | | | |
    // ------------+--------+------+------------+--------+-------+----------------
    // NOTE: We arbitrarily choose to use channel 0 for this datapath

    // Src Address: the InputData[] array
    AVR32_DMACA.sar0 = (unsigned long) pSrcBuf;

    // Dst Address: the AES_IDATAXR registers.
    AVR32_DMACA.dar0 = (AVR32_AES_ADDRESS | AVR32_AES_IDATA1R);

    // Linked list ptrs: not used.
    AVR32_DMACA.llp0 = 0x00000000;

    // Channel 0 Ctrl register low
    AVR32_DMACA.ctl0l = (0 << AVR32_DMACA_CTL0L_INT_EN_OFFSET) |    // Do not enable interrupts
        (2 << AVR32_DMACA_CTL0L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
        (2 << AVR32_DMACA_CTL0L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
        (2 << AVR32_DMACA_CTL0L_DINC_OFFSET) |  // Dst address increment: none
        (0 << AVR32_DMACA_CTL0L_SINC_OFFSET) |  // Src address increment: increment
        (1 << AVR32_DMACA_CTL0L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 4 data items
        (1 << AVR32_DMACA_CTL0L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 4 data items
        (0 << AVR32_DMACA_CTL0L_S_GATH_EN_OFFSET) | // Source gather: disabled
        (0 << AVR32_DMACA_CTL0L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
        (1 << AVR32_DMACA_CTL0L_TT_FC_OFFSET) | // transfer type:M2P, flow controller: DMACA
        (0 << AVR32_DMACA_CTL0L_DMS_OFFSET) |   // Dest master: HSB master 1
        (1 << AVR32_DMACA_CTL0L_SMS_OFFSET) |   // Source master: HSB master 2
        (0 << AVR32_DMACA_CTL0L_LLP_D_EN_OFFSET) |  // Not used
        (0 << AVR32_DMACA_CTL0L_LLP_S_EN_OFFSET)    // Not used
        ;



    // Channel 0 Ctrl register high
    AVR32_DMACA.ctl0h = ((u16BufferSize / 4) << AVR32_DMACA_CTL0H_BLOCK_TS_OFFSET) |    // Block transfer size
        (0 << AVR32_DMACA_CTL0H_DONE_OFFSET)    // Not done
        ;

    // Channel 0 Config register low
    AVR32_DMACA.cfg0l = (0 << AVR32_DMACA_CFG0L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking
        (0 << AVR32_DMACA_CFG0L_HS_SEL_SRC_OFFSET)  // Source handshaking: ignored because the src is memory.
        ;   // All other bits set to 0.

    // Channel 0 Config register high
    AVR32_DMACA.cfg0h = (AVR32_DMACA_CH_AES_TX << AVR32_DMACA_CFG0H_DEST_PER_OFFSET) |  // Dest hw handshaking itf:
        (0 << AVR32_DMACA_CFG0H_SRC_PER_OFFSET) // Source hw handshaking itf: ignored because the src is memory.
        ;   // All other bits set to 0.

    // *
    // * Configure the DMA AES -> RAM channel.
    // *
    // ------------+--------+------+------------+--------+-------+----------------
    // Transfer | Source | Dest | Flow | Width | Chunk | Buffer
    // type | | | controller | (bits) | size | Size
    // ------------+--------+------+------------+--------+-------+----------------
    // | | | | | |
    // Per-to-Mem | AES | RAM | DMACA | 32 | 4 | u16BufferSize
    // | | | | | |
    // ------------+--------+------+------------+--------+-------+----------------
    // NOTE: We arbitrarily choose to use channel 1 for this datapath

    // Src Address: the AES_ODATAXR registers.
    AVR32_DMACA.sar1 = (AVR32_AES_ADDRESS | AVR32_AES_ODATA1R);

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.dar1 = (unsigned long) pDstBuf;

    // Linked list ptrs: not used.
    AVR32_DMACA.llp1 = 0x00000000;

    // Channel 1 Ctrl register low
    AVR32_DMACA.ctl1l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) |    // Do not enable interrupts
        (2 << AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
        (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
        (0 << AVR32_DMACA_CTL1L_DINC_OFFSET) |  // Dst address increment: increment
        (2 << AVR32_DMACA_CTL1L_SINC_OFFSET) |  // Src address increment: none
        (1 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 4 data items
        (1 << AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 4 data items
        (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source gather: disabled
        (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
        (2 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | // transfer type:P2M, flow controller: DMACA
        (1 << AVR32_DMACA_CTL1L_DMS_OFFSET) |   // Dest master: HSB master 2
        (0 << AVR32_DMACA_CTL1L_SMS_OFFSET) |   // Source master: HSB master 1
        (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) |  // Not used
        (0 << AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET)    // Not used
        ;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((u16BufferSize / 4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET) |    // Block transfer size
        (0 << AVR32_DMACA_CTL1H_DONE_OFFSET)    // Not done
        ;

    // Channel 1 Config register low
    AVR32_DMACA.cfg1l = (0 << AVR32_DMACA_CFG1L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking
        (0 << AVR32_DMACA_CFG1L_HS_SEL_SRC_OFFSET)  // Source handshaking: ignored because the src is memory.
        ;   // All other bits set to 0.

    // Channel 1 Config register high
    AVR32_DMACA.cfg1h = (0 << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) |  // Dest hw handshaking itf: ignored because the dst is memory.
        (AVR32_DMACA_CH_AES_RX << AVR32_DMACA_CFG1H_SRC_PER_OFFSET) // Source hw handshaking itf:
        ;   // All other bits set to 0.



    // *
    // * Set the AES cryptographic key and init vector.
    // *
    // Set the cryptographic key.
    aes_set_key (&AVR32_AES, CipherKey);

    // Set the initialization vector.
    aes_set_initvector (&AVR32_AES, InitVector);

    // *
    // * Start the process
    // *
    // ccountt0 = Get_system_register(AVR32_COUNT);

    // Enable Channel 0 & 1 : start the process.
    AVR32_DMACA.chenreg = ((3 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (3 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

    taskEXIT_CRITICAL ();

    // Wait for the end of the AES->RAM transfer (channel 1).
    while (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET));

    taskENTER_CRITICAL ();
    xSemaphoreGive (AES_semphr);
    taskEXIT_CRITICAL ();

    // ccountt1 = Get_system_register(AVR32_COUNT);

    // Check the results of the encryption.
    /*
       CI_LocalPrintf("Nb cycles: "); print_ulong(DMACA_AES_EVAL_USART, ccountt1 - ccountt0); CI_LocalPrintf("OK!!! Nb cycles: ");
       print_ulong(DMACA_AES_EVAL_USART, ccountt1 - ccountt0); */
}



void STICK20_mci_aes (unsigned char cMode, unsigned short int u16BufferSize, unsigned int* pIOBuffer, unsigned int BlockNr_u32)
{
    unsigned int i;
    // unsigned char TestResult = TRUE;
    aes_config_t AesConf;       // AES config structure

    taskENTER_CRITICAL ();

    // ====================
    // Configure the AES.
    // ====================
    switch (cMode)
    {
        case STICK20_RAM_TO_AES_TO_MCI:
            AesConf.ProcessingMode = AES_PMODE_CIPHER;
            break;
        case STICK20_MCI_TO_AES_TO_RAM:
            AesConf.ProcessingMode = AES_PMODE_DECIPHER;
            break;
    }
    AesConf.ProcessingDelay = 0;    // No delay: best performance
    AesConf.StartMode = AES_START_MODE_DMA; // DMA mode
    AesConf.KeySize = AES_KEY_SIZE_256; // 256bit cryptographic key
    AesConf.OpMode = AES_CBC_MODE;  // CBC cipher mode
    AesConf.LodMode = 0;    // LODMODE == 0 : the end of the
    // encryption is notified by the DMACA transfer complete interrupt. The output
    // is available in the OutputData[] buffer.
    AesConf.CFBSize = 0;    // Don't-care because we're using the CBC mode.
    AesConf.CounterMeasureMask = 0; // Disable all counter measures.
    aes_configure (&AVR32_AES, &AesConf);

    // ====================
    // Configure the DMACA.
    // ====================
    // Enable the DMACA
    AVR32_DMACA.dmacfgreg = 1 << AVR32_DMACA_DMACFGREG_DMA_EN_OFFSET;

    // *
    // * Configure the DMA channel 0
    // *
    // ------------+--------+------+------------+--------+-------+----------------
    // Transfer | Source | Dest | Flow | Width | Chunk | Buffer
    // type | | | controller | (bits) | size | Size
    // ------------+--------+------+------------+--------+-------+----------------
    // | | | | | |
    //
    // STICK20_MCI_TO_AES_TO_RAM - READ from MCI
    // PER-to-Per | MCI | AES | DMACA | 32 | 4 | u16BufferSize
    //
    // STICK20_RAM_TO_AES_TO_MCI - READ from RAM -
    // Mem-to-Per | RAM | AES | DMACA | 32 | 4 | u16BufferSize
    // | | | | | |
    // ------------+--------+------+------------+--------+-------+----------------
    // NOTE: We arbitrarily choose to use channel 0 for this datapath

    // STICK20_RAM_TO_AES_TO_MCI : Part 1 : RAM > AES (Cipher)
    // STICK20_MCI_TO_AES_TO_RAM : Part 1 : MCI > AES (Decipher)
    switch (cMode)
    {
        case STICK20_RAM_TO_AES_TO_MCI:
            // RAM > AES
            AVR32_DMACA.sar0 = (unsigned long) pIOBuffer;   // Src Address: RAM
            break;

        case STICK20_MCI_TO_AES_TO_RAM:
            // MCI > AES
            AVR32_DMACA.sar0 = (unsigned long) &AVR32_MCI.fifo; // Src Address: MCI
            break;
    }

    // Dst Address: the AES_IDATAXR registers.
    AVR32_DMACA.dar0 = (AVR32_AES_ADDRESS | AVR32_AES_IDATA1R);

    // Linked list ptrs: not used.
    AVR32_DMACA.llp0 = 0x00000000;

    switch (cMode)
    {
        case STICK20_RAM_TO_AES_TO_MCI:
            // RAM > AES
            AVR32_DMACA.ctl0l = (0 << AVR32_DMACA_CTL0L_INT_EN_OFFSET) |    // Do not enable interrupts
                (2 << AVR32_DMACA_CTL0L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
                (2 << AVR32_DMACA_CTL0L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
                (2 << AVR32_DMACA_CTL0L_DINC_OFFSET) |  // Dst address increment: none
                (0 << AVR32_DMACA_CTL0L_SINC_OFFSET) |  // Src address increment: increment
                (1 << AVR32_DMACA_CTL0L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 4 data items
                (1 << AVR32_DMACA_CTL0L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 4 data items
                (0 << AVR32_DMACA_CTL0L_S_GATH_EN_OFFSET) | // Source gather: disabled
                (0 << AVR32_DMACA_CTL0L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
                (1 << AVR32_DMACA_CTL0L_TT_FC_OFFSET) | // transfer type:M2P, flow controller: DMACA
                (0 << AVR32_DMACA_CTL0L_DMS_OFFSET) |   // Dest master: HSB master 1 - AES
                (1 << AVR32_DMACA_CTL0L_SMS_OFFSET) |   // Source master: HSB master 2 - RAM
                (0 << AVR32_DMACA_CTL0L_LLP_D_EN_OFFSET) |  // Not used
                (0 << AVR32_DMACA_CTL0L_LLP_S_EN_OFFSET)    // Not used
                ;
            break;

        case STICK20_MCI_TO_AES_TO_RAM:
            // MCI > AES
            AVR32_DMACA.ctl0l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) |    // Do not enable interrupts
                (2 << AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
                (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
                (2 << AVR32_DMACA_CTL1L_DINC_OFFSET) |  // Dst address increment: none
                (0 << AVR32_DMACA_CTL1L_SINC_OFFSET) |  // Src address increment: increment
                (1 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // 1 Dst burst transaction len: 4 data items
                (3 << AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // 1 Src burst transaction len: 16 data items
                (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source gather: disabled
                (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
                (3 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | // transfer type:P2P, flow controller: DMACA
                (0 << AVR32_DMACA_CTL1L_DMS_OFFSET) |   // 0 Dest master: HSB master 1 - AES
                (0 << AVR32_DMACA_CTL1L_SMS_OFFSET) |   // 0 Source master: HSB master 1 - MCI
                (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) |  // Not used
                (0 << AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET)    // Not used
                ;
            break;
    }


    // Channel 0 Ctrl register high
    AVR32_DMACA.ctl0h = (u16BufferSize / 4 << AVR32_DMACA_CTL0H_BLOCK_TS_OFFSET) |  // Block transfer size, 32 bit
        (0 << AVR32_DMACA_CTL0H_DONE_OFFSET)    // Not done
        ;

    // Channel 0 Config register low
    AVR32_DMACA.cfg0l = (0 << AVR32_DMACA_CFG0L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking or ignored because memory.
        (0 << AVR32_DMACA_CFG0L_HS_SEL_SRC_OFFSET)  // Source handshaking: hw handshaking.
        ;   // All other bits set to 0.

    // Channel 0 Config register high
    switch (cMode)
    {
        case STICK20_RAM_TO_AES_TO_MCI:
            // RAM > AES
            AVR32_DMACA.cfg0h = (AVR32_DMACA_CH_AES_TX << AVR32_DMACA_CFG0H_DEST_PER_OFFSET) |  // Dest hw handshaking itf:
                (0 << AVR32_DMACA_CFG0H_SRC_PER_OFFSET) // Source hw handshaking itf:
                ;   // All other bits set to 0.
            break;

        case STICK20_MCI_TO_AES_TO_RAM:
            // MCI > AES
            AVR32_DMACA.cfg0h = (AVR32_DMACA_CH_AES_TX << AVR32_DMACA_CFG0H_DEST_PER_OFFSET) |  // AVR32_DMACA_CH_AES_TX, Dest hw handshaking itf:
                (AVR32_DMACA_CH_MMCI_RX << AVR32_DMACA_CFG0H_SRC_PER_OFFSET)    // Source hw handshaking itf:
                ;   // All other bits set to 0.
            break;
    }


    // *
    // * Configure the DMA channel 1
    // *
    // ------------+--------+------+------------+--------+-------+----------------
    // Transfer | Source | Dest | Flow | Width | Chunk | Buffer
    // type | | | controller | (bits) | size | Size
    // ------------+--------+------+------------+--------+-------+----------------
    // | | | | | |
    // STICK20_MCI_TO_AES_TO_RAM - READ from MCI
    // PER-to-Per | AES | RAM | DMACA | 32 | 4 | u16BufferSize
    //
    // STICK20_RAM_TO_AES_TO_MCI - READ from RAM -
    // Mem-to-Per | AES | MCI | DMACA | 32 | 4 | u16BufferSize
    // | | | | | |
    // ------------+--------+------+------------+--------+-------+----------------

    // NOTE: We arbitrarily choose to use channel 1 for this datapath

    // STICK20_RAM_TO_AES_TO_MCI : Part 2 : AES (Ciphered) > MCI
    // STICK20_MCI_TO_AES_TO_RAM : Part 2 : AES (Deciphered) > RAM

    // Src Address: the AES_ODATAXR registers.
    AVR32_DMACA.sar1 = (AVR32_AES_ADDRESS | AVR32_AES_ODATA1R);

    // Dst Address
    switch (cMode)
    {
        case STICK20_RAM_TO_AES_TO_MCI:
            AVR32_DMACA.dar1 = (unsigned long) &AVR32_MCI.fifo; // Dst Address: RAM
            break;

        case STICK20_MCI_TO_AES_TO_RAM:
            AVR32_DMACA.dar1 = (unsigned long) pIOBuffer;   // Dst Address: RAM
            break;
    }

    // Linked list ptrs: not used.
    AVR32_DMACA.llp1 = 0x00000000;

    // Channel 1 Ctrl register low
    switch (cMode)
    {
        case STICK20_RAM_TO_AES_TO_MCI:
            // AES > MCI
            AVR32_DMACA.ctl1l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) |    // Do not enable interrupts
                (2 << AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
                (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
                (0 << AVR32_DMACA_CTL1L_DINC_OFFSET) |  // Dst address increment: increment (MCI)
                (2 << AVR32_DMACA_CTL1L_SINC_OFFSET) |  // Src address increment: none
                (3 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // 1 Dst burst transaction len: 4 data items
                (1 << AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // 1 Src burst transaction len: 4 data items
                (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source gather: disabled
                (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
                (3 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | // transfer type:P2P, flow controller: DMACA
                (0 << AVR32_DMACA_CTL1L_DMS_OFFSET) |   // Dest master: HSB master 1 - per
                (0 << AVR32_DMACA_CTL1L_SMS_OFFSET) |   // Source master: HSB master 1 - per
                (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) |  // Not used
                (0 << AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET)    // Not used
                ;
            break;

        case STICK20_MCI_TO_AES_TO_RAM:
            // AES > RAM
            AVR32_DMACA.ctl1l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) |    // Do not enable interrupts
                (2 << AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
                (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
                (0 << AVR32_DMACA_CTL1L_DINC_OFFSET) |  // Dst address increment: increment
                (2 << AVR32_DMACA_CTL1L_SINC_OFFSET) |  // Src address increment: none
                (1 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 4 data items
                (1 << AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 4 data items
                (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source gather: disabled
                (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
                (2 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | // transfer type:P2M, flow controller: DMACA
                (1 << AVR32_DMACA_CTL1L_DMS_OFFSET) |   // 1 Dest master: HSB master 2 - mem
                (0 << AVR32_DMACA_CTL1L_SMS_OFFSET) |   // 0 Source master: HSB master 1 - per
                (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) |  // Not used
                (0 << AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET)    // Not used
                ;
            break;
    }



    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((u16BufferSize / 4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET) |    // Block transfer size, 32 bit
        (0 << AVR32_DMACA_CTL1H_DONE_OFFSET)    // Not done
        ;

    // Channel 1 Config register low
    AVR32_DMACA.cfg1l = (0 << AVR32_DMACA_CFG1L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking or ignored because memory.
        (0 << AVR32_DMACA_CFG1L_HS_SEL_SRC_OFFSET)  // Source handshaking: hw handshaking
        ;   // All other bits set to 0.

    // Channel 1 Config register high
    AVR32_DMACA.cfg1h = (0 << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) |  // Dest hw handshaking itf: ignored because the dst is memory.
        (AVR32_DMACA_CH_AES_RX << AVR32_DMACA_CFG1H_SRC_PER_OFFSET) // Source hw handshaking itf:
        ;   // All other bits set to 0.

    // Channel 1 Config register high
    switch (cMode)
    {
        case STICK20_RAM_TO_AES_TO_MCI:
            // AES > MCI
            AVR32_DMACA.cfg1h = (AVR32_DMACA_CH_MMCI_TX << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) | // Dest hw handshaking itf:
                (AVR32_DMACA_CH_AES_RX << AVR32_DMACA_CFG1H_SRC_PER_OFFSET) // Source hw handshaking itf:
                ;   // All other bits set to 0.
            break;

        case STICK20_MCI_TO_AES_TO_RAM:
            // AES > RAM
            AVR32_DMACA.cfg1h = (0 << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) |  // Dest hw handshaking itf: ignored because the dst is memory.
                (AVR32_DMACA_CH_AES_RX << AVR32_DMACA_CFG1H_SRC_PER_OFFSET) // Source hw handshaking itf:
                ;   // All other bits set to 0.
            break;
    }


    // *
    // * Set the AES cryptographic key and init vector.
    // *
    // Set the cryptographic key.
    aes_set_key (&AVR32_AES, CipherKey);


    // TODO move to ESSIV instead of using raw block no as IV
    // Set the initialization vector.
    for (i = 0; i < 4; i++)
    {
        InitVectorSD[i] = BlockNr_u32;
    }
    aes_set_initvector (&AVR32_AES, InitVectorSD);

    taskEXIT_CRITICAL ();
    // *
    // * Start the process
    // *
    // ccountt0 = Get_system_register(AVR32_COUNT);
    /*
       // Enable Channel 0 & 1 : start the process. AVR32_DMACA.chenreg = ((3<<AVR32_DMACA_CHENREG_CH_EN_OFFSET) |
       (3<<AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

       // Wait for the end of the AES->RAM transfer (channel 1). while(AVR32_DMACA.chenreg & (2<<AVR32_DMACA_CHENREG_CH_EN_OFFSET)); */
    // ccountt1 = Get_system_register(AVR32_COUNT);

    // Check the results of the encryption.
    /*
       CI_LocalPrintf("Nb cycles: "); print_ulong(DMACA_AES_EVAL_USART, ccountt1 - ccountt0); CI_LocalPrintf("OK!!! Nb cycles: ");
       print_ulong(DMACA_AES_EVAL_USART, ccountt1 - ccountt0); */
}

/*****************************************************************************************************/
// ! 1) Configure two DMACA channels:
// ! - RAM -> AES
// ! - AES -> RAM
// ! 2) Set the AES cryptographic key and init vector.
// ! 3) Start the process
// ! 4) Check the result on the first 16 Words.
void test_ram_aes_ram (unsigned short int u16BufferSize, unsigned int* pSrcBuf, unsigned int* pDstBuf)
{
    unsigned int i;
    unsigned char TestResult = TRUE;


    // ====================
    // Configure the DMACA.
    // ====================
    // Enable the DMACA
    AVR32_DMACA.dmacfgreg = 1 << AVR32_DMACA_DMACFGREG_DMA_EN_OFFSET;

    // *
    // * Configure the DMA RAM -> AES channel.
    // *
    // ------------+--------+------+------------+--------+-------+----------------
    // Transfer | Source | Dest | Flow | Width | Chunk | Buffer
    // type | | | controller | (bits) | size | Size
    // ------------+--------+------+------------+--------+-------+----------------
    // | | | | | |
    // Mem-to-Per | RAM | AES | DMACA | 32 | 4 | u16BufferSize
    // | | | | | |
    // ------------+--------+------+------------+--------+-------+----------------
    // NOTE: We arbitrarily choose to use channel 0 for this datapath

    // Src Address: the InputData[] array
    AVR32_DMACA.sar0 = (unsigned long) pSrcBuf;

    // Dst Address: the AES_IDATAXR registers.
    AVR32_DMACA.dar0 = (AVR32_AES_ADDRESS | AVR32_AES_IDATA1R);

    // Linked list ptrs: not used.
    AVR32_DMACA.llp0 = 0x00000000;

    // Channel 0 Ctrl register low
    AVR32_DMACA.ctl0l = (0 << AVR32_DMACA_CTL0L_INT_EN_OFFSET) |    // Do not enable interrupts
        (2 << AVR32_DMACA_CTL0L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
        (2 << AVR32_DMACA_CTL0L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
        (2 << AVR32_DMACA_CTL0L_DINC_OFFSET) |  // Dst address increment: none
        (0 << AVR32_DMACA_CTL0L_SINC_OFFSET) |  // Src address increment: increment
        (1 << AVR32_DMACA_CTL0L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 4 data items
        (1 << AVR32_DMACA_CTL0L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 4 data items
        (0 << AVR32_DMACA_CTL0L_S_GATH_EN_OFFSET) | // Source gather: disabled
        (0 << AVR32_DMACA_CTL0L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
        (1 << AVR32_DMACA_CTL0L_TT_FC_OFFSET) | // transfer type:M2P, flow controller: DMACA
        (0 << AVR32_DMACA_CTL0L_DMS_OFFSET) |   // Dest master: HSB master 1
        (1 << AVR32_DMACA_CTL0L_SMS_OFFSET) |   // Source master: HSB master 2
        (0 << AVR32_DMACA_CTL0L_LLP_D_EN_OFFSET) |  // Not used
        (0 << AVR32_DMACA_CTL0L_LLP_S_EN_OFFSET)    // Not used
        ;

    // Channel 0 Ctrl register high
    AVR32_DMACA.ctl0h = ((u16BufferSize / 4) << AVR32_DMACA_CTL0H_BLOCK_TS_OFFSET) |    // Block transfer size
        (0 << AVR32_DMACA_CTL0H_DONE_OFFSET)    // Not done
        ;

    // Channel 0 Config register low
    AVR32_DMACA.cfg0l = (0 << AVR32_DMACA_CFG0L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking
        (0 << AVR32_DMACA_CFG0L_HS_SEL_SRC_OFFSET)  // Source handshaking: ignored because the src is memory.
        ;   // All other bits set to 0.

    // Channel 0 Config register high
    AVR32_DMACA.cfg0h = (AVR32_DMACA_CH_AES_TX << AVR32_DMACA_CFG0H_DEST_PER_OFFSET) |  // Dest hw handshaking itf:
        (0 << AVR32_DMACA_CFG0H_SRC_PER_OFFSET) // Source hw handshaking itf: ignored because the src is memory.
        ;   // All other bits set to 0.


    // *
    // * Configure the DMA AES -> RAM channel.
    // *
    // ------------+--------+------+------------+--------+-------+----------------
    // Transfer | Source | Dest | Flow | Width | Chunk | Buffer
    // type | | | controller | (bits) | size | Size
    // ------------+--------+------+------------+--------+-------+----------------
    // | | | | | |
    // Per-to-Mem | AES | RAM | DMACA | 32 | 4 | u16BufferSize
    // | | | | | |
    // ------------+--------+------+------------+--------+-------+----------------
    // NOTE: We arbitrarily choose to use channel 1 for this datapath

    // Src Address: the AES_ODATAXR registers.
    AVR32_DMACA.sar1 = (AVR32_AES_ADDRESS | AVR32_AES_ODATA1R);

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.dar1 = (unsigned long) pDstBuf;

    // Linked list ptrs: not used.
    AVR32_DMACA.llp1 = 0x00000000;

    // Channel 1 Ctrl register low
    AVR32_DMACA.ctl1l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) |    // Do not enable interrupts
        (2 << AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
        (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
        (0 << AVR32_DMACA_CTL1L_DINC_OFFSET) |  // Dst address increment: increment
        (2 << AVR32_DMACA_CTL1L_SINC_OFFSET) |  // Src address increment: none
        (1 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 4 data items
        (1 << AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 4 data items
        (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source gather: disabled
        (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
        (2 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | // transfer type:P2M, flow controller: DMACA
        (1 << AVR32_DMACA_CTL1L_DMS_OFFSET) |   // Dest master: HSB master 2
        (0 << AVR32_DMACA_CTL1L_SMS_OFFSET) |   // Source master: HSB master 1
        (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) |  // Not used
        (0 << AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET)    // Not used
        ;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((u16BufferSize / 4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET) |    // Block transfer size
        (0 << AVR32_DMACA_CTL1H_DONE_OFFSET)    // Not done
        ;

    // Channel 1 Config register low
    AVR32_DMACA.cfg1l = (0 << AVR32_DMACA_CFG1L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking
        (0 << AVR32_DMACA_CFG1L_HS_SEL_SRC_OFFSET)  // Source handshaking: ignored because the src is memory.
        ;   // All other bits set to 0.

    // Channel 1 Config register high
    AVR32_DMACA.cfg1h = (0 << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) |  // Dest hw handshaking itf: ignored because the dst is memory.
        (AVR32_DMACA_CH_AES_RX << AVR32_DMACA_CFG1H_SRC_PER_OFFSET) // Source hw handshaking itf:
        ;   // All other bits set to 0.


    // *
    // * Set the AES cryptographic key and init vector.
    // *
    // Set the cryptographic key.
    aes_set_key (&AVR32_AES, CipherKey);

    // Set the initialization vector.
    aes_set_initvector (&AVR32_AES, InitVector);

    // *
    // * Start the process
    // *
    ccountt0 = Get_system_register (AVR32_COUNT);

    // Enable Channel 0 & 1 : start the process.
    AVR32_DMACA.chenreg = ((3 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (3 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

    // Wait for the end of the AES->RAM transfer (channel 1).
    while (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET));

    ccountt1 = Get_system_register (AVR32_COUNT);

    // Check the results of the encryption.
    for (i = 0; i < DMACA_AES_EVAL_REFBUF_SIZE; i++)
    {
        if (OutputData[i] != RefOutputData[i])
        {
            TestResult = FALSE;
            break;
        }
    }
    if (FALSE == TestResult)
        CI_LocalPrintf ("KO!!!\n");
    else
    {
        CI_LocalPrintf ("OK!!! Nb cycles: %d", ccountt1 - ccountt0);
    }
}


/*****************************************************************************************************/
// ! 1) Configure two DMACA channels:
// ! - RAM -> AES
// ! - AES -> RAM
// ! 2) Set the AES cryptographic key and init vector.
// ! 3) Start the process
// ! 4) Check the result on the first 16 Words.
void AES_Encryption (unsigned short int u16BufferSize, unsigned int* pSrcBuf, unsigned int* pDstBuf, unsigned int* pKey,
                     unsigned int* pLocalInitVector)
{
    // unsigned int i;
    // unsigned char TestResult = TRUE;

  if (DMACA_AES_EVAL_BUF_SIZE < u16BufferSize)
  {
    u16BufferSize = u16BufferSize;
  }

  taskENTER_CRITICAL ();

  // Wait for end of DMA transfers
  busy_wait (is_dma_mci_2_ram_complete);
  busy_wait (is_dma_ram_2_usb_complete);
  busy_wait (is_dma_usb_2_ram_complete);
  busy_wait (is_dma_ram_2_mci_complete);

    // ====================
    // Configure the DMACA.
    // ====================
    // Enable the DMACA
    AVR32_DMACA.dmacfgreg = 1 << AVR32_DMACA_DMACFGREG_DMA_EN_OFFSET;

    // *
    // * Configure the DMA RAM -> AES channel.
    // *
    // ------------+--------+------+------------+--------+-------+----------------
    // Transfer | Source | Dest | Flow | Width | Chunk | Buffer
    // type | | | controller | (bits) | size | Size
    // ------------+--------+------+------------+--------+-------+----------------
    // | | | | | |
    // Mem-to-Per | RAM | AES | DMACA | 32 | 4 | u16BufferSize
    // | | | | | |
    // ------------+--------+------+------------+--------+-------+----------------
    // NOTE: We arbitrarily choose to use channel 0 for this datapath

    // Src Address: the InputData[] array
    AVR32_DMACA.sar0 = (unsigned long) pSrcBuf;

    // Dst Address: the AES_IDATAXR registers.
    AVR32_DMACA.dar0 = (AVR32_AES_ADDRESS | AVR32_AES_IDATA1R);

    // Linked list ptrs: not used.
    AVR32_DMACA.llp0 = 0x00000000;

    // Channel 0 Ctrl register low
    AVR32_DMACA.ctl0l = (0 << AVR32_DMACA_CTL0L_INT_EN_OFFSET) |    // Do not enable interrupts
        (2 << AVR32_DMACA_CTL0L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
        (2 << AVR32_DMACA_CTL0L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
        (2 << AVR32_DMACA_CTL0L_DINC_OFFSET) |  // Dst address increment: none
        (0 << AVR32_DMACA_CTL0L_SINC_OFFSET) |  // Src address increment: increment
        (1 << AVR32_DMACA_CTL0L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 4 data items
        (1 << AVR32_DMACA_CTL0L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 4 data items
        (0 << AVR32_DMACA_CTL0L_S_GATH_EN_OFFSET) | // Source gather: disabled
        (0 << AVR32_DMACA_CTL0L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
        (1 << AVR32_DMACA_CTL0L_TT_FC_OFFSET) | // transfer type:M2P, flow controller: DMACA
        (0 << AVR32_DMACA_CTL0L_DMS_OFFSET) |   // Dest master: HSB master 1
        (1 << AVR32_DMACA_CTL0L_SMS_OFFSET) |   // Source master: HSB master 2
        (0 << AVR32_DMACA_CTL0L_LLP_D_EN_OFFSET) |  // Not used
        (0 << AVR32_DMACA_CTL0L_LLP_S_EN_OFFSET)    // Not used
        ;

    // Channel 0 Ctrl register high
    AVR32_DMACA.ctl0h = ((u16BufferSize / 4) << AVR32_DMACA_CTL0H_BLOCK_TS_OFFSET) |    // Block transfer size
        (0 << AVR32_DMACA_CTL0H_DONE_OFFSET)    // Not done
        ;

    // Channel 0 Config register low
    AVR32_DMACA.cfg0l = (0 << AVR32_DMACA_CFG0L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking
        (0 << AVR32_DMACA_CFG0L_HS_SEL_SRC_OFFSET)  // Source handshaking: ignored because the src is memory.
        ;   // All other bits set to 0.

    // Channel 0 Config register high
    AVR32_DMACA.cfg0h = (AVR32_DMACA_CH_AES_TX << AVR32_DMACA_CFG0H_DEST_PER_OFFSET) |  // Dest hw handshaking itf:
        (0 << AVR32_DMACA_CFG0H_SRC_PER_OFFSET) // Source hw handshaking itf: ignored because the src is memory.
        ;   // All other bits set to 0.


    // *
    // * Configure the DMA AES -> RAM channel.
    // *
    // ------------+--------+------+------------+--------+-------+----------------
    // Transfer | Source | Dest | Flow | Width | Chunk | Buffer
    // type | | | controller | (bits) | size | Size
    // ------------+--------+------+------------+--------+-------+----------------
    // | | | | | |
    // Per-to-Mem | AES | RAM | DMACA | 32 | 4 | u16BufferSize
    // | | | | | |
    // ------------+--------+------+------------+--------+-------+----------------
    // NOTE: We arbitrarily choose to use channel 1 for this datapath

    // Src Address: the AES_ODATAXR registers.
    AVR32_DMACA.sar1 = (AVR32_AES_ADDRESS | AVR32_AES_ODATA1R);

    // Dst Address: the OutputData[] array.
    AVR32_DMACA.dar1 = (unsigned long) pDstBuf;

    // Linked list ptrs: not used.
    AVR32_DMACA.llp1 = 0x00000000;

    // Channel 1 Ctrl register low
    AVR32_DMACA.ctl1l = (0 << AVR32_DMACA_CTL1L_INT_EN_OFFSET) |    // Do not enable interrupts
        (2 << AVR32_DMACA_CTL1L_DST_TR_WIDTH_OFFSET) |  // Dst transfer width: 32 bits
        (2 << AVR32_DMACA_CTL1L_SRC_TR_WIDTH_OFFSET) |  // Src transfer width: 32 bits
        (0 << AVR32_DMACA_CTL1L_DINC_OFFSET) |  // Dst address increment: increment
        (2 << AVR32_DMACA_CTL1L_SINC_OFFSET) |  // Src address increment: none
        (1 << AVR32_DMACA_CTL1L_DST_MSIZE_OFFSET) | // Dst burst transaction len: 4 data items
        (1 << AVR32_DMACA_CTL1L_SRC_MSIZE_OFFSET) | // Src burst transaction len: 4 data items
        (0 << AVR32_DMACA_CTL1L_S_GATH_EN_OFFSET) | // Source gather: disabled
        (0 << AVR32_DMACA_CTL1L_D_SCAT_EN_OFFSET) | // Destination scatter: disabled
        (2 << AVR32_DMACA_CTL1L_TT_FC_OFFSET) | // transfer type:P2M, flow controller: DMACA
        (1 << AVR32_DMACA_CTL1L_DMS_OFFSET) |   // Dest master: HSB master 2
        (0 << AVR32_DMACA_CTL1L_SMS_OFFSET) |   // Source master: HSB master 1
        (0 << AVR32_DMACA_CTL1L_LLP_D_EN_OFFSET) |  // Not used
        (0 << AVR32_DMACA_CTL1L_LLP_S_EN_OFFSET)    // Not used
        ;

    // Channel 1 Ctrl register high
    AVR32_DMACA.ctl1h = ((u16BufferSize / 4) << AVR32_DMACA_CTL1H_BLOCK_TS_OFFSET) |    // Block transfer size
        (0 << AVR32_DMACA_CTL1H_DONE_OFFSET)    // Not done
        ;

    // Channel 1 Config register low
    AVR32_DMACA.cfg1l = (0 << AVR32_DMACA_CFG1L_HS_SEL_DST_OFFSET) |    // Destination handshaking: hw handshaking
        (0 << AVR32_DMACA_CFG1L_HS_SEL_SRC_OFFSET)  // Source handshaking: ignored because the src is memory.
        ;   // All other bits set to 0.

    // Channel 1 Config register high
    AVR32_DMACA.cfg1h = (0 << AVR32_DMACA_CFG1H_DEST_PER_OFFSET) |  // Dest hw handshaking itf: ignored because the dst is memory.
        (AVR32_DMACA_CH_AES_RX << AVR32_DMACA_CFG1H_SRC_PER_OFFSET) // Source hw handshaking itf:
        ;   // All other bits set to 0.


    // *
    // * Set the AES cryptographic key and init vector.
    // *
    // Set the cryptographic key.
    aes_set_key (&AVR32_AES, pKey);

    // Set the initialization vector.
    // aes_set_initvector(&AVR32_AES, InitVector);
    aes_set_initvector (&AVR32_AES, pLocalInitVector);

    // *
    // * Start the process
    // *
    ccountt0 = Get_system_register (AVR32_COUNT);


    // Enable Channel 0 & 1 : start the process.
    AVR32_DMACA.chenreg = ((3 << AVR32_DMACA_CHENREG_CH_EN_OFFSET) | (3 << AVR32_DMACA_CHENREG_CH_EN_WE_OFFSET));

    taskEXIT_CRITICAL ();

    // Wait for the end of the AES->RAM transfer (channel 1).
    while (AVR32_DMACA.chenreg & (2 << AVR32_DMACA_CHENREG_CH_EN_OFFSET));

    ccountt1 = Get_system_register (AVR32_COUNT);

    // CI_LocalPrintf("Nb cycles: %d",ccountt1 - ccountt0);
}


/*******************************************************************************

  AES_StorageKeyEncryption

  Only 256 bit Keys // Max 256 byte

*******************************************************************************/

int AES_StorageKeyEncryption (unsigned int nLength, unsigned char* cData, unsigned char* cKeyData, unsigned char cMode)
{
    aes_config_t AesConf;       // AES config structure
    // int i;
    // ****************************************************************************
    // CIPHER IN DMA MODE: RAM -> AES -> RAM
    // - 256bit cryptographic key
    // - EBC cipher mode
    // - No counter measures
    // ****************************************************************************

    // Wait for the semaphore
    while (pdTRUE != xSemaphoreTake (AES_semphr, 1))
    {
//      CI_StringOut ("�3");
    }

    // Wait for end of DMA transfers
    busy_wait(is_dma_mci_2_ram_complete);
    busy_wait(is_dma_ram_2_usb_complete);
    busy_wait(is_dma_usb_2_ram_complete);
    busy_wait(is_dma_ram_2_mci_complete);

    memcpy (InputData, cData, nLength);

    // ====================
    // Configure the AES.
    // ====================
    // AesConf.ProcessingMode = AES_PMODE_CIPHER; // Cipher
    // AesConf.ProcessingMode = AES_PMODE_DECIPHER; // Cipher
    AesConf.ProcessingMode = cMode;
    AesConf.ProcessingDelay = 0;    // No delay: best performance
    AesConf.StartMode = AES_START_MODE_DMA; // DMA mode
    AesConf.KeySize = AES_KEY_SIZE_256; // 256bit cryptographic key
    AesConf.OpMode = AES_ECB_MODE;  // ECB cipher mode
    AesConf.LodMode = 0;    // LODMODE == 0 : the end of the

    // encryption is notified by the DMACA transfer complete interrupt. The output
    // is available in the OutputData[] buffer.

    AesConf.CFBSize = 0;    // Don't-care because we're using the ECB mode.
    AesConf.CounterMeasureMask = 0; // Disable all counter measures.

    aes_configure (&AVR32_AES, &AesConf);

    AES_Encryption (nLength, (unsigned int *) InputData, (unsigned int *) OutputData, (unsigned int *) cKeyData, (unsigned int *) InitVector);

    memcpy (cData, (void *) OutputData, nLength);

    taskENTER_CRITICAL ();
    xSemaphoreGive (AES_semphr);
    taskEXIT_CRITICAL ();

    return (0);
}

#define AES_KEYSIZE_256_BIT     32  // 32 * 8 = 256

void xxHashInitVector_v (unsigned int* InitVector_au32, unsigned int BlockNr)
{
    static unsigned char XorKey_au8[AES_KEYSIZE_256_BIT];
    unsigned int i;

    ReadXorPatternFromFlash (XorKey_au8);

    for (i = 0; i < 4; i++)
    {
        InitVector_au32[i] = XXH_fast32 ((void*)XorKey_au8, AES_KEYSIZE_256_BIT, BlockNr + i);
    }
}


/*******************************************************************************

  AES_StorageKeyEncryption

  Only 256 bit Keys // Max 256 byte

*******************************************************************************/

int AES_KeyEncryption (unsigned int nLength, unsigned char* cData, unsigned char* cKeyData, unsigned char cMode, unsigned int BlockNr_u32)
{
    aes_config_t AesConf;       // AES config structure
    // int i;
    // ****************************************************************************
    // CIPHER IN DMA MODE: RAM -> AES -> RAM
    // - 256bit cryptographic key
    // - EBC cipher mode
    // - No counter measures
    // ****************************************************************************

    if (DMACA_AES_EVAL_BUF_SIZE < nLength)
    {
      nLength = DMACA_AES_EVAL_BUF_SIZE;
    }

    // Wait for the semaphore
    while (pdTRUE != xSemaphoreTake (AES_semphr, 1))
    {
//      CI_StringOut ("�4");
    }

    // Wait for end of DMA transfers
    busy_wait(is_dma_mci_2_ram_complete);
    busy_wait(is_dma_ram_2_usb_complete);
    busy_wait(is_dma_usb_2_ram_complete);
    busy_wait(is_dma_ram_2_mci_complete);

    memcpy (InputData, cData, nLength);

    // ====================
    // Configure the AES.
    // ====================
    // AesConf.ProcessingMode = AES_PMODE_CIPHER; // Cipher
    // AesConf.ProcessingMode = AES_PMODE_DECIPHER; // Cipher
    AesConf.ProcessingMode = cMode;
    AesConf.ProcessingDelay = 0;    // No delay: best performance
    AesConf.StartMode = AES_START_MODE_DMA; // DMA mode
    AesConf.KeySize = AES_KEY_SIZE_256; // 256bit cryptographic key
    AesConf.OpMode = AES_CBC_MODE;  // CBC cipher mode
    AesConf.LodMode = 0;    // LODMODE == 0 : the end of the

    // encryption is notified by the DMACA transfer complete interrupt. The output
    // is available in the OutputData[] buffer.

    AesConf.CFBSize = 0;    // Don't-care because we're using the CBC mode.
    AesConf.CounterMeasureMask = 0; // Disable all counter measures.

    aes_configure (&AVR32_AES, &AesConf);

    // Set the initialization vector.
    /*
       for (i=0;i<4;i++) { InitVectorSD[i] = BlockNr_u32; } */
    xxHashInitVector_v (InitVectorSD, BlockNr_u32);

    if (DMACA_AES_EVAL_BUF_SIZE < nLength)
    {
      nLength = DMACA_AES_EVAL_BUF_SIZE;
    }


    AES_Encryption (nLength, (unsigned int *) InputData, (unsigned int *) OutputData, (unsigned int *) cKeyData, (unsigned int *) InitVectorSD);

    memcpy (cData, (void *) OutputData, nLength);

    taskENTER_CRITICAL ();
    xSemaphoreGive (AES_semphr);
    taskEXIT_CRITICAL ();

    return (0);
}


int AES_local_test (void)
{
    unsigned char acKey[32 + 1] = "01234567890123456789012345678901";
    unsigned char acData[32 + 1] = "11111111111111111111111111111111";
    unsigned char acInputData[32];


    memcpy (acInputData, acData, 32);

    CI_LocalPrintf ("Unverschl�sselter Key : ");
    HexPrint (16, acInputData);
    CI_LocalPrintf ("\r\n");

    AES_KeyEncryption (32, acInputData, acKey, AES_PMODE_CIPHER, 0);
    // AES_StorageKeyEncryption (32, acInputData, acKey, AES_PMODE_CIPHER);

    CI_LocalPrintf ("Verschl�sselter Key   : ");
    HexPrint (16, acInputData);
    CI_LocalPrintf ("\r\n");

    AES_KeyEncryption (32, acInputData, acKey, AES_PMODE_DECIPHER, 0);
    // AES_StorageKeyEncryption (32, acInputData, acKey, AES_PMODE_DECIPHER);

    CI_LocalPrintf ("Entschl�sselter Key   : ");
    HexPrint (16, acInputData);
    CI_LocalPrintf ("\r\n");

    return (0);
}


int AES_local_test_org (void)
{
    aes_config_t AesConf;       // AES config structure
    int i;
    // ****************************************************************************
    // CIPHER IN DMA MODE: RAM -> AES -> RAM
    // - 256bit cryptographic key
    // - CBC cipher mode
    // - No counter measures
    // ****************************************************************************

    // Wait for the semaphore
    while (pdTRUE != xSemaphoreTake (AES_semphr, 1))
    {
//      CI_StringOut ("�5");
    }

    // Init the input array.
    for (i = 0; i < DMACA_AES_EVAL_BUF_SIZE; i += DMACA_AES_EVAL_REFBUF_SIZE)
    {
        memcpy (InputData + i, RefInputData, DMACA_AES_EVAL_REFBUF_SIZE * sizeof (unsigned int));
    }

    // ====================
    // Configure the AES.
    // ====================
    AesConf.ProcessingMode = AES_PMODE_CIPHER;  // Cipher
    AesConf.ProcessingDelay = 0;    // No delay: best performance
    AesConf.StartMode = AES_START_MODE_DMA; // DMA mode
    AesConf.KeySize = AES_KEY_SIZE_256; // 256bit cryptographic key
    AesConf.OpMode = AES_CBC_MODE;  // CBC cipher mode
    AesConf.LodMode = 0;    // LODMODE == 0 : the end of the
    // encryption is notified by the DMACA transfer complete interrupt. The output
    // is available in the OutputData[] buffer.
    AesConf.CFBSize = 0;    // Don't-care because we're using the CBC mode.
    AesConf.CounterMeasureMask = 0; // Disable all counter measures.
    aes_configure (&AVR32_AES, &AesConf);


    // ****************************************************************************
    // - input of 16 32bit words in CPUSRAM
    // - output of 16 32bit words in CPUSRAM
    // ****************************************************************************
    CI_LocalPrintf ("\n---------------------------------------------------\n");
    CI_LocalPrintf ("       Cipher in DMA Mode: CPUSRAM -> AES -> CPUSRAM ------\n");
    CI_LocalPrintf ("       - 256bit cryptographic key\n");
    CI_LocalPrintf ("       - CBC cipher mode\n");
    CI_LocalPrintf ("       - No counter measures\n");
    CI_LocalPrintf ("       - input of 16 32bit words in CPUSRAM\n");
    CI_LocalPrintf ("       - output of 16 32bit words in CPUSRAM\n");

    test_ram_aes_ram (16, (unsigned int *) InputData, (unsigned int *) OutputData);

    // ****************************************************************************
    // - input of 256 32bit words in CPUSRAM
    // - output of 256 32bit words in CPUSRAM
    // ****************************************************************************
    CI_LocalPrintf ("\n---------------------------------------------------\n");
    CI_LocalPrintf ("------ Cipher in DMA Mode: CPUSRAM -> AES -> CPUSRAM ------\n");
    CI_LocalPrintf ("       - 256bit cryptographic key\n");
    CI_LocalPrintf ("       - CBC cipher mode\n");
    CI_LocalPrintf ("       - No counter measures\n");
    CI_LocalPrintf ("       - input of 256 32bit words in CPUSRAM\n");
    CI_LocalPrintf ("       - output of 256 32bit words in CPUSRAM\n");

    test_ram_aes_ram (256, (unsigned int *) InputData, (unsigned int *) OutputData);

    // ****************************************************************************
    // - input of 256 32bit words in HSBSRAM0
    // - output of 256 32bit words in HSBSRAM1
    // ****************************************************************************
    CI_LocalPrintf ("\n---------------------------------------------------\n");
    CI_LocalPrintf ("------ Cipher in DMA Mode: HSBSRAM0 -> AES -> HSBSRAM1 ------\n");
    CI_LocalPrintf ("       - 256bit cryptographic key\n");
    CI_LocalPrintf ("       - CBC cipher mode\n");
    CI_LocalPrintf ("       - No counter measures\n");
    CI_LocalPrintf ("       - input of 256 32bit words in HSBSRAM0\n");
    CI_LocalPrintf ("       - output of 256 32bit words in HSBSRAM1\n");

    // Set the Src and Dst array addresses to respectively HSBSRAM0 & HSBRAM1.
    pSrcData_HsbSram = (unsigned int *) AVR32_INTRAM0_ADDRESS;
    pDstData_HsbSram = (unsigned int *) AVR32_INTRAM1_ADDRESS;

    // Init the input array.
    for (i = 0; i < DMACA_AES_EVAL_BUF_SIZE; i += DMACA_AES_EVAL_REFBUF_SIZE)
    {
        memcpy (pSrcData_HsbSram + i, RefInputData, DMACA_AES_EVAL_REFBUF_SIZE * sizeof (unsigned int));
    }

    test_ram_aes_ram (256, pSrcData_HsbSram, (unsigned int *) pDstData_HsbSram);
    // gpio_clr_gpio_pin(DMACA_AES_EVAL_LED3);

    taskENTER_CRITICAL ();
    xSemaphoreGive (AES_semphr);
    taskEXIT_CRITICAL ();

    CI_LocalPrintf ("\r\nDone!");

    // End of tests: go to sleep.
    // SLEEP(AVR32_PM_SMODE_STATIC);
    // while (TRUE);
    return (0);
}

/*******************************************************************************

  AES_SetNewStorageKey

*******************************************************************************/

void AES_SetNewStorageKey (unsigned char* pcKey)
{
    int i;
    unsigned int n;

    for (i = 0; i < 8; i++)
    {
        n = (pcKey[i * 4] << 24) + (pcKey[i * 4 + 1] << 16) + (pcKey[i * 4 + 2] << 8) + pcKey[i * 4 + 3];
        CipherKey[i] = n;
    }
}

#ifdef NOT_USED
/* ! \brief Initializes the HSB bus matrix. */
static void init_hmatrix (void)
{
    // Set flashc master type to last default to save one cycle for
    // each branch.
    avr32_hmatrix_scfg_t scfg;

    scfg = AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_FLASH];
    scfg.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_LAST_DEFAULT;
    AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_FLASH] = scfg;

    scfg = AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_AES];
    scfg.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_LAST_DEFAULT;
    AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_AES] = scfg;

    scfg = AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_SRAM];
    scfg.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_LAST_DEFAULT;
    AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_SRAM] = scfg;

    scfg = AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_EMBEDDED_SYS_SRAM_0];
    scfg.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_LAST_DEFAULT;
    AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_EMBEDDED_SYS_SRAM_0] = scfg;

    scfg = AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_EMBEDDED_SYS_SRAM_1];
    scfg.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_LAST_DEFAULT;
    AVR32_HMATRIX.SCFG[AVR32_HMATRIX_SLAVE_EMBEDDED_SYS_SRAM_1] = scfg;
}


/* ! \brief The main function. It sets up the USART module on EXAMPLE_USART. The terminal settings are 57600 8N1. Then it sets up the interrupt
   handler and waits for a USART interrupt to trigger. */
int main_aes (void)
{
    aes_config_t AesConf;       // AES config structure
    int i;
    static const gpio_map_t USART_GPIO_MAP =    // USART GPIO map
    {
        {DMACA_AES_EVAL_USART_RX_PIN, DMACA_AES_EVAL_USART_RX_FUNCTION},
        {DMACA_AES_EVAL_USART_TX_PIN, DMACA_AES_EVAL_USART_TX_FUNCTION}
    };
    static const usart_options_t USART_OPTIONS =    // USART options.
    {
        .baudrate = DMACA_AES_EVAL_USART_BAUDRATE,
        .charlength = 8,
        .paritytype = USART_NO_PARITY,
        .stopbits = USART_1_STOPBIT,
        .channelmode = USART_NORMAL_CHMODE
    };


#if BOARD == EVK1104
    if (PM_FREQ_STATUS_FAIL == pm_configure_clocks (&pm_freq_param_dma))
        while (1);
#endif

    init_hmatrix ();

    // Assign GPIO to USART.
    gpio_enable_module (USART_GPIO_MAP, sizeof (USART_GPIO_MAP) / sizeof (USART_GPIO_MAP[0]));

    // Initialize USART in RS232 mode.
    usart_init_rs232 (DMACA_AES_EVAL_USART, &USART_OPTIONS, DMACA_AES_EVAL_CPU_FREQ);
    CI_LocalPrintf ("\x1B[2J\x1B[H.: Using the AES with the DMACA at ");
    print_ulong (DMACA_AES_EVAL_USART, DMACA_AES_EVAL_CPU_FREQ);
    CI_LocalPrintf ("Hz :.\n\n");

    AES_local_test ();
}

#endif // NOT_USED

#endif // STICK_20_AES_ENABLE
