/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
 *
 * This file is part of Nitrokey
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


#ifndef GLOBAL_H
#define GLOBAL_H

// #include "portmacro.h" Don't use is here, system crashes

#define VERSION_MAJOR 0 // 255 = debug version
#define VERSION_MINOR 39    // 0 = development

#define INTERNAL_VERSION_NR 0

#define TRUE		1
#define FALSE   0



// Define to activate A_Muster specific handling
// Achtung: Bootloader in trampoline.x aktivieren

// *** Activate only for PROD version ***
#define STICK_20_A_MUSTER_PROD


// #define STICK_20_SEND_DEBUGINFOS_VIA_HID // = Debug version, use ca. 2k flash

#ifdef  STICK_20_SEND_DEBUGINFOS_VIA_HID
#undef VERSION_MAJOR
#define VERSION_MAJOR 255
#endif


#define STICK_20_AES_ENABLE

// Printf only for Windows
#define CCID_NO_PRINTF

// Active time measuring - always on for realtime handling
#define TIME_MEASURING_ENABLE


#ifndef STICK_20_A_MUSTER_PROD
#define INTERPRETER_ENABLE  // Disable for PROD Version
#endif

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
#define INTERPRETER_ENABLE  // Enable also for PROD Version
#endif


/*******************************************************************************

  Enable password matrix calls

*******************************************************************************/

#define HID_PASSWORD_MARTIX_AKTIV

/*******************************************************************************

  Security defines

*******************************************************************************/

#define GENERATE_RANDOM_NUMBER_WITH_2ND_SOURCE

/******************************************************************************

  Defines for debugging

******************************************************************************/
// #define DEBUG_LOG_CCID_DETAIL // Shows details of smartcard io + set CCID_T1_DebugLevel = 3;
// #define DEBUG_USB_CCID_IO_DETAIL // Shows details of CCID USB transfers

// #define MMC_DEBUG_PRINT // Shows details of SD card actions

// #define DEBUG_SCSI_IO // Debug SCSI IO

/*******************************************************************************

  Defines for enabling interpreter test

*******************************************************************************/

// Flash map (Block 0 - 511)
// Bootloader / flash loader block 0 - 16 8 KB
// Programm code block 16 - 497
// Hidden volume setup data block 498 - 499 HV_FLASH_START_PAGE (block 499 is free for extensions)
// OTP data block 500 - 510 OTP_FLASH_START_PAGE
// Password safe data block 510 - 511 PWS_FLASH_START_PAGE
//
// Warning flash data starts at block 498 > space for program code + bootloader is 254976 byte

// #define ENABLE_IBN_PWM_TESTS // ca. 1k - Password matix - Enable the interpreter tests
// #define ENABLE_IBN_HV_TESTS // ca. 1k - Hidden volume - Enable the interpreter tests
// #define ENABLE_IBN_TIME_ACCESS_TESTS // ca. 2k - Time access - Enable the interpreter tests
// #define ENABLE_IBN_FILE_ACCESS_TESTS // ca. 7k - file access - Enable the interpreter tests
// #define DEBUG_DUF_TESTS // ca. 1,5k - DFU tests - Enable the interpreter tests

/*******************************************************************************

  Defines for enabling special debug informations

*******************************************************************************/

// #define DEBUG_SCSI_IO // ca. 3 k - Enable SCSI command output


/*******************************************************************************

   Defines for saving flash memory
   *** Only for debugging ***

   Some defines lower the secure of the stick 2.0

*******************************************************************************/

// #define SAVE_FLASH_MEMORY

#ifdef SAVE_FLASH_MEMORY
#define SAVE_FLASH_MEMORY_NO_PBKDF2 // ca. 12k - Disable the PBKDF2 function: Warning stored data is not secure
#endif


/*********************************************************************************/


#define HID_PASSWORD_LEN      20

// Remove all printf in production version
#ifdef INTERPRETER_ENABLE
int CI_LocalPrintf (char* szFormat, ...);
int CI_LocalPrintfNoDelay (char* szFormat, ...);
int CI_TickLocalPrintf (char* szFormat, ...);
#else
#define CI_LocalPrintf(...)
#define CI_TickLocalPrintf(...)
#define CI_StringOut(...)
#endif



#define FMCK_HZ                       60000000
#define FCPU_HZ                       FMCK_HZ
#define FHSB_HZ                       FCPU_HZ
#define FPBB_HZ                       FMCK_HZ
#define FPBA_HZ                       FMCK_HZ


// Led ports
#define TOOL_LED_RED_PIN       AVR32_PIN_PX45
#define TOOL_LED_GREEN_PIN     AVR32_PIN_PX41


/*********************************************************************************

 Special functions

*********************************************************************************/

// Simulate
// #define SIMULATE_USB_CCID_DISPATCH

// Activated AD logging
// #define AD_LOGGING_ACTIV


#endif // GLOBAL_H
