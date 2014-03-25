/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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


#ifndef GLOBAL_H
#define GLOBAL_H

//#include "portmacro.h" Don't use is here, system crashes

#define VERSION_MAJOR 0
#define VERSION_MINOR 3


#define TRUE		1
#define FALSE   0

/*
typedef unsigned char  	u8;
typedef unsigned short 	u16;
typedef unsigned long  	u32;
typedef signed char  	s8;
typedef signed short 	s16;
typedef signed long  	s32;
*/

/******************************************************************************

  Defines for debugging

******************************************************************************/
//#define DEBUG_LOG_CCID_DETAIL       // Shows details of smartcard io + set CCID_T1_DebugLevel = 3;

//#define MMC_DEBUG_PRINT            // Shows details of SD card actions


// Define to activate A_Muster specific handling
//to delete #define STICK_20_A_MUSTER           // Activate - to delete

// Define to activate A_Muster specific handling
// Achtung: Bootloader in trampoline.x aktivieren

// *** Activate only for PROD version ***
//#define STICK_20_A_MUSTER_PROD

//#define STICK_20_SEND_DEBUGINFOS_VIA_HID

// Enable the HTML interface via ram disc
//#define HTML_ENABLE_HTML_INTERFACE


#define STICK_20_AES_ENABLE

// Simulate
//#define SIMULATE_USB_CCID_DISPATCH


// Activated AD logging
//#define AD_LOGGING_ACTIV

// Printf only for Windows
#define CCID_NO_PRINTF

// Active time measuring - always on for realtime handling
#define TIME_MEASURING_ENABLE


#ifndef STICK_20_A_MUSTER_PROD
  #define INTERPRETER_ENABLE     // Disable for PROD Version
#endif

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
  #define INTERPRETER_ENABLE     // Enable also for PROD Version
#endif



#define HID_PASSWORD_LEN      20

int CI_LocalPrintf (char *szFormat,...);
int CI_LocalPrintfNoDelay (char *szFormat,...);
int CI_TickLocalPrintf (char *szFormat,...);


#define FMCK_HZ                       60000000
#define FCPU_HZ                       FMCK_HZ
#define FHSB_HZ                       FCPU_HZ
#define FPBB_HZ                       FMCK_HZ
#define FPBA_HZ                       FMCK_HZ


// Led ports
#define TOOL_LED_RED_PIN       AVR32_PIN_PX41
#define TOOL_LED_GREEN_PIN     AVR32_PIN_PX45


#endif // GLOBAL_H
