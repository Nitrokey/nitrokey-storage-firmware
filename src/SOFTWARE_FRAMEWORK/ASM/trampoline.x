/* This file is part of the ATMEL AVR32-SoftwareFramework-AT32UC3A-1.4.0 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief AVR32 UC3 ISP trampoline.
 *
 * In order to be able to program a project with both BatchISP and JTAGICE mkII
 * without having to take the general-purpose fuses into consideration, add this
 * file to the project and change the program entry point to _trampoline.
 *
 * The pre-programmed ISP will be erased if JTAGICE mkII is used.
 *
 * - Compiler:           GNU GCC for AVR32
 * - Supported devices:  All AVR32UC devices can be used.
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

/* Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
* This file contains modifications done by Rudolf Boeddeker
* For the modifications applies:
* 
* Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
*
* Nitrokey is free software: you can redistribute it and/or modify
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



#include "conf_isp.h"

// The DUF file
//#define ISP_BIN             "../src/SOFTWARE_FRAMEWORK/UTILS/1.bin"
//#define ISP_BIN             "../src/SOFTWARE_FRAMEWORK/UTILS/at32uc3a3-isp.bin"
#define ISP_BIN             "../src/SOFTWARE_FRAMEWORK/UTILS/DFU_1.0.3.bin"
//#define ISP_BIN             "../src/SOFTWARE_FRAMEWORK/UTILS/at32uc3a-isp.bin"
  
// To enable Application execute
// 
// Default 
// avr32program writefuses -finternal@0x80000000,512Kb gp=0xFC07FFFF  
//
// 0xF    C    0    7    FFFF
//   1111 1100 0000 0111 FFFF
//   1                           1  = Start DFU without testing any other condition
//    1                          1  = Check I/O condition for starting DFU
//     1                         1  = ISP enable BOD by software
//      1 1                      3  = BOD enabled with reset by the ISP using the BODLEVEL and BODHYST settings from the GP fuses
//         1                     1  = BOD hysteresis enabled
//          00 0000              0  = BODLEVEL (set to highest voltage, ok ?)
//                  011          3  = BOOTPROT > Bootprotection size is 8K
//                     1         1  = EPFL > external privileged fetch is unlocked
//                       FFFF    Unlock all flash regions 

// To start application 
// avr32program writefuses -finternal@0x80000000,512Kb gp=0x3C07FFFF  
//
// 0x3    C    0    F    FFFF
//   0011 1100 0000 1111 FFFF
//   0                           1  = Start DFU without testing any other condition
//    0                          1  = Check I/O condition for starting DFU
//     1                         1  = ISP enable BOD by software
//      1 1                      3  = BOD enabled with reset by the ISP using the BODLEVEL and BODHYST settings from the GP fuses
//         1                     1  = BOD hysteresis enabled
//          00 0000              0  = BODLEVEL (set to highest voltage, ok ?)
//                  111          3  = BOOTPROT > Bootprotection size is off
//                     1         1  = EPFL > external privileged fetch is unlocked
//                       FFFF    Unlock all flash regions 


// To start application 
// DFU Configuration word 1 = 0xE11EFCDE
// ISP_FORCE      = 0
// ISP_IO_COND_EN = 0
// 
// With batchisp
// batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FC 0x1FC fillbuffer 0xE1 program
// batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FD 0x1FD fillbuffer 0x1E program
// batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FE 0x1FE fillbuffer 0xFC program
// batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FF 0x1FF fillbuffer 0xDE program
//
// With
// avr32program program -finternal@0x80000000,512Kb -cxtal -e -v -O0x808001FC -Fbin isp_mycfg_w1.bin





//! @{
//! \verbatim



  // This must be linked @ 0x80000000 if it is to be run upon reset.
  .section  .reset, "ax", @progbits


  .global _trampoline
  .type _trampoline, @function
_trampoline:
  // Jump to program start.
//  rjmp    program_start
 
  // Include the ISP raw binary image.
  .incbin ISP_BIN

  .org  PROGRAM_START_OFFSET
program_start:
  // Jump to the C runtime startup routine.
  lda.w   pc, _stext


//! \endverbatim
//! @}
