/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 13.08.2012
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


/*
 * DFU_test.c
 *
 *  Created on: 13.08.2012
 *      Author: RB
 */



#include "compiler.h"
#include "preprocessor.h"
#include "board.h"
#include "gpio.h"
#include "ctrl_access.h"
#include "flashc.h"
#include "string.h"

#include "conf_usb.h"
#include "usb_drv.h"

#include "global.h"
#include "tools.h"

#include "DFU_test.h"


#ifdef DEBUG_DUF_TESTS
int CI_LocalPrintf (char* szFormat, ...);
int CI_TickLocalPrintf (char* szFormat, ...);
int CI_StringOut (char* szText);
#else
#define CI_LocalPrintf(...)
#define CI_TickLocalPrintf(...)
#define CI_StringOut(...)
#endif

/*******************************************************************************

 Local defines

*******************************************************************************/

#define TOOL_DFU_ISP_CONFIG_ADDR_1 (AVR32_FLASHC_USER_PAGE + 0x1FC)


#define TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0      (0x929E2A9E)
#define TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0_0 (0x92)
#define TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0_1 (0x9E)
#define TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0_2 (0x2A)
#define TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0_3 (0x9E)

/*
   //#define TOOL_DFU_ISP_CONFIG_ADDR_1 (AVR32_FLASHC_USER_PAGE + 0x1F8) #define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0 (0x929E0E62) #define
   TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0_0 (0x92) #define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0_1 (0x9E) #define
   TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0_2 (0x0E) #define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0_3 (0x62)

   #define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_1 (0xE11EFCDE) #define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_1_0 (0xE1) #define
   TOOL_DFU_ISP_CONFIG_WORD_START_APPL_1_1 (0x1E) #define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_1_2 (0xFC) #define
   TOOL_DFU_ISP_CONFIG_WORD_START_APPL_1_3 (0xDE) */

/* For DFU 1.1.0 */
#define TOOL_DFU_ISP_CONFIG_WORD_START_APPL   (0xE11EFCDE)
#define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0 (0xE1)
#define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_1 (0x1E)
#define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_2 (0xFC)
#define TOOL_DFU_ISP_CONFIG_WORD_START_APPL_3 (0xDE)



#define TOOL_DFU_ISP_CONFIG_WORD_START_DFU    (0xE11EFED0)
#define TOOL_DFU_ISP_CONFIG_WORD_START_DFU_0  (0xE1)
#define TOOL_DFU_ISP_CONFIG_WORD_START_DFU_1  (0x1E)
#define TOOL_DFU_ISP_CONFIG_WORD_START_DFU_2  (0xFE)
#define TOOL_DFU_ISP_CONFIG_WORD_START_DFU_3  (0xD0)

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/



/*******************************************************************************

  DFU_ForceIsp

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define ISP_GPFB_FORCE                31
#define ISP_GPFB_FORCE_MASK           0x80000000
#define ISP_GPFB_FORCE_OFFSET         31
#define ISP_GPFB_FORCE_SIZE           1

void DFU_ForceIsp (Bool force)
{
    flashc_set_gp_fuse_bit (ISP_GPFB_FORCE_OFFSET, force);
}

/*******************************************************************************

  DFU_UsbStop

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void DFU_UsbStop (void)
{
    Disable_global_interrupt ();
    Usb_disable ();
    (void) Is_usb_enabled ();
    Enable_global_interrupt ();
}

/*******************************************************************************

  DFU_DisableFirmwareUpdate

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void DFU_DisableFirmwareUpdate (void)
{
    u8 DFU_String_au8[4];

    DFU_String_au8[0] = TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0;
    DFU_String_au8[1] = TOOL_DFU_ISP_CONFIG_WORD_START_APPL_1;
    DFU_String_au8[2] = TOOL_DFU_ISP_CONFIG_WORD_START_APPL_2;
    DFU_String_au8[3] = TOOL_DFU_ISP_CONFIG_WORD_START_APPL_3;

    flashc_memcpy (TOOL_DFU_ISP_CONFIG_ADDR_1, DFU_String_au8, 4, TRUE);

    flashc_write_gp_fuse_bit (30, 0);   // Set SP_IO_COND_EN = 0 for DUF 1.0.3
    flashc_write_gp_fuse_bit (31, 0);   // Set to 0 to start application for DUF 1.0.3

}

/*******************************************************************************

  DFU_ResetCPU

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

void DFU_ResetCPU (void)
{
    DFU_UsbStop ();
    Disable_global_interrupt ();
    vTaskEndScheduler ();

    __builtin_mtdr (AVR32_DC, AVR32_DC_DBE_MASK);
    __builtin_mtdr (AVR32_DC, AVR32_DC_RES_MASK);

    while (1);
}

/*******************************************************************************

  DFU_EnableFirmwareUpdate

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void DFU_EnableFirmwareUpdate (void)
{
    u8 DFU_String_au8[4];

    DFU_String_au8[0] = TOOL_DFU_ISP_CONFIG_WORD_START_DFU_0;
    DFU_String_au8[1] = TOOL_DFU_ISP_CONFIG_WORD_START_DFU_1;
    DFU_String_au8[2] = TOOL_DFU_ISP_CONFIG_WORD_START_DFU_2;
    DFU_String_au8[3] = TOOL_DFU_ISP_CONFIG_WORD_START_DFU_3;

    flashc_memcpy (TOOL_DFU_ISP_CONFIG_ADDR_1, DFU_String_au8, 4, TRUE);

    flashc_erase_gp_fuse_bit (30, 0);   // Set SP_IO_COND_EN = 1 for DUF 1.0.3
    flashc_erase_gp_fuse_bit (31, 0);   // Set to 1 to start bootloader for DUF 1.0.3

    flashc_write_gp_fuse_bit (30, 1);   // Set SP_IO_COND_EN = 1 for DUF 1.0.3
    flashc_write_gp_fuse_bit (31, 1);   // Set to 1 to start application for DUF 1.0.3
}

/*******************************************************************************

  DFU_FirmwareResetUserpage

  Changes
  Date      Author          Info
  28.10.14  RB              Creation of function

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

void DFU_FirmwareResetUserpage (void)
{
    u8 DFU_String_au8[4];

    /*
       flashc_erase_all_gp_fuses (TRUE);

       flashc_write_gp_fuse_bit FC07FFFF

       flashc_set_all_gp_fuses (); */

    flashc_write_gp_fuse_bit (30, 0);   // Set SP_IO_COND_EN = 0 for DUF 1.0.3
    flashc_write_gp_fuse_bit (31, 0);   // Set to 0 to start application for DUF 1.0.3



    flashc_erase_user_page (TRUE);

    DFU_String_au8[0] = TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0_0;
    DFU_String_au8[1] = TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0_1;
    DFU_String_au8[2] = TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0_2;
    DFU_String_au8[3] = TOOL_DFU_ISP_CONFIG_WORD_DEFAULT_0_3;
    /*
       DFU_String_au8[0] = TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0_0; DFU_String_au8[1] = TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0_1; DFU_String_au8[2] =
       TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0_2; DFU_String_au8[3] = TOOL_DFU_ISP_CONFIG_WORD_START_APPL_0_3; */

    flashc_memcpy (TOOL_DFU_ISP_CONFIG_ADDR_1, DFU_String_au8, 4, TRUE);
}


/*******************************************************************************

  IBN_DFU_Tests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
#ifdef DEBUG_DUF_TESTS
void IBN_DFU_Tests (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8)
{
    u8 DFU_String_au8[4];

    if (0 == nParamsGet_u8)
    {
        CI_LocalPrintf ("DFU test functions\r\n");
        CI_LocalPrintf ("\r\n");
        CI_LocalPrintf ("0   Show ISP Config 1 word\r\n");
        CI_LocalPrintf ("1   Enable DFU at next start\r\n");
        CI_LocalPrintf ("2   Disable DFU at next start\r\n");
        CI_LocalPrintf ("3   Show security bit\r\n");
        CI_LocalPrintf ("4   Set security bit\r\n");
        CI_LocalPrintf ("5   Show bootloader protected size\r\n");
        CI_LocalPrintf ("6   Set bootloader protected size = 0x2000\r\n");
        CI_LocalPrintf ("7   Reset system\r\n");
        CI_LocalPrintf ("\r\n");
        return;
    }

    switch (CMD_u8)
    {
        case 0:
            memcpy (DFU_String_au8, (void *) TOOL_DFU_ISP_CONFIG_ADDR_1, 4);
            CI_LocalPrintf ("ISP Config 1 word : ");
            HexPrint (4, DFU_String_au8);
            CI_LocalPrintf ("\r\n");
            break;

        case 1:
            CI_LocalPrintf ("Enable DFU\r\n");
            DFU_EnableFirmwareUpdate ();
            break;

        case 2:
            CI_LocalPrintf ("Disable DFU\r\n");
            DFU_DisableFirmwareUpdate ();
            break;

        case 3:
            CI_LocalPrintf ("Security bit : %d\r\n", flashc_is_security_bit_active ());
            break;

        case 4:
            CI_LocalPrintf ("Activate security bit\r\n");
            flashc_activate_security_bit ();
            break;

        case 5:
            CI_LocalPrintf ("Bootloader protected : 0x%04x\r\n", flashc_get_bootloader_protected_size ());
            break;

        case 6:
            CI_LocalPrintf ("Set bootloader protected 0x2000\r\n");
            flashc_set_bootloader_protected_size (0x2000);
            break;

        case 7:
            DFU_ResetCPU ();
            break;

    }
}
#endif
