/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 11.01.2013
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
 * OTP_Test.c
 *
 *  Created on: 11.01.2013
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
#include "TIME_MEASURING.h"

#include "OTP_test.h"
#include "..\\OTP\\hotp.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

extern void USB_KB_SendDataToUSB (void);
void sendChar (u8 chr);


/*******************************************************************************

 Local declarations

*******************************************************************************/

/*******************************************************************************

  IBN_OTP_Tests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_OTP_Tests (unsigned char nParamsGet_u8, unsigned char CMD_u8,
                    unsigned int Param_u32, unsigned char *String_pu8)
{
    u8 DFU_String_au8[4];
    u32 Ret_u32;
    u32 i;

#ifdef TIME_MEASURING_ENABLE
    u32 Runtime_u32;
#endif
    u8 *Data_pu8;
    u8 slot_no;
    u8 is_programmed;
    u32 result;

    if (0 == nParamsGet_u8)
    {
        CI_LocalPrintf ("OTP test functions\r\n");
        CI_LocalPrintf ("\r\n");
        CI_LocalPrintf ("0   ??\r\n");
        CI_LocalPrintf ("1   Send char\r\n");
        CI_LocalPrintf ("2 x CRC32 Check x=String\r\n");
        CI_LocalPrintf ("3   Show hotp config [slot nr]\r\n");
        CI_LocalPrintf ("4   Get HOTP [slot]\r\n");
        CI_LocalPrintf ("5   Show HOTP counter page [slot]\r\n");
        CI_LocalPrintf ("6   Show TOTP config [slot nr]\r\n");
        CI_LocalPrintf ("7   Get TOTP time\r\n");
        CI_LocalPrintf ("8   Set TOTP time\r\n");
        CI_LocalPrintf ("\r\n");
        return;
    }

    switch (CMD_u8)
    {
        case 0:
            CI_LocalPrintf ("ISP Config 1 word : ");

            check_backups ();

            HexPrint (4, DFU_String_au8);
            CI_LocalPrintf ("\r\n");
            break;

        case 1:
            sendChar ('a');
            sendChar ('b');
            sendChar ('c');

            break;

        case 2:
            // String_pu8 = "123456789";
            CI_LocalPrintf ("CRC32 check : -%s- = ", String_pu8);
#ifdef TIME_MEASURING_ENABLE
            TIME_MEASURING_Start (TIME_MEASURING_TIME_15);
#endif
            CRC_InitCRC32 ();
            CRC_CalcBlockCRC32 (String_pu8, strlen ((char *) String_pu8));
            Ret_u32 = CRC_GetCRC32 ();
#ifdef TIME_MEASURING_ENABLE
            Runtime_u32 = TIME_MEASURING_Stop (TIME_MEASURING_TIME_15);
#endif
            CI_LocalPrintf ("0x%08x\r\n", Ret_u32);
#ifdef TIME_MEASURING_ENABLE
            CI_LocalPrintf ("CRC32 Runtime %ld usec\n\r",
                            Runtime_u32 / TIME_MEASURING_TICKS_IN_USEC);
#endif
            break;

        case 3:    // Print hopt slot header
            /*
             * slot structure: 1b 0x01 if slot is used (programmed) 15b slot
             * name 20b secret 1b configuration flags: MSB [x|x|x|x|x|send
             * token id|send enter after code?|no. of digits 6/8] LSB 12b
             * token id 1b keyboard layout 
             */
            Data_pu8 = get_hotp_slot_addr (Param_u32);
            if (NULL == Data_pu8)
            {
                CI_LocalPrintf ("Wrong hopt slot %d\n\r", Param_u32);
                return;
            }

            if (0x01 == Data_pu8[0])
            {
                CI_LocalPrintf ("slot %d is used\n\r", Param_u32);
            }
            else
            {
                CI_LocalPrintf ("slot %d is not used\n\r", Param_u32);
            }

            CI_LocalPrintf ("slot name -%.15.15s-\n\r", &Data_pu8[1]);
            CI_LocalPrintf ("        : ");
            HexPrint (15, &Data_pu8[1]);
            CI_LocalPrintf ("\n\r");


            CI_LocalPrintf ("Secret  : ");
            HexPrint (20, &Data_pu8[16]);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("Config  : ");
            HexPrint (1, &Data_pu8[36]);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("Token   : ");
            HexPrint (12, &Data_pu8[37]);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("Keyboard: ");
            HexPrint (1, &Data_pu8[39]);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("slot name -%.15.15s-\n\r", &Data_pu8[1]);

            break;

        case 4:    // Get hopt slot
            slot_no = Param_u32;
            if (slot_no < NUMBER_OF_HOTP_SLOTS) // HOTP slot
            {
                is_programmed = *((u8 *) (hotp_slots[slot_no]));
                if (is_programmed == 0x01)
                {
                    result = get_code_from_hotp_slot (slot_no);
                    change_endian_u32 (result);
                    CI_LocalPrintf ("HOPT %ld\n\r", result);
                }
                else
                {
                    CI_LocalPrintf ("Slot %d not programmed\n\r", slot_no);
                }
            }
            else
            {
                CI_LocalPrintf ("Slot %d not a hopt slot\n\r", slot_no);
            }
            break;

        case 5:    // Show hopt counter slot
            slot_no = Param_u32;
            if (slot_no < NUMBER_OF_HOTP_SLOTS) // HOTP slot
            {
                is_programmed = *((u8 *) (hotp_slots[slot_no]));

                Data_pu8 = (u8 *) hotp_slot_counters[slot_no];

                CI_LocalPrintf ("Slot %d\n\r", slot_no);
                CI_LocalPrintf ("Counter %lld\n\r",
                                get_counter_value ((u32) Data_pu8));
                CI_LocalPrintf ("Addr 0x%08x - Page %d\n\r", Data_pu8,
                                ((u32) Data_pu8 -
                                 FLASH_START) / FLASH_PAGE_SIZE);
                CI_LocalPrintf ("Programmed flag = %d (1 = true)\n\r",
                                is_programmed);

                for (i = 0; i < 16; i++)
                {
                    CI_LocalPrintf ("0x%03x ", i * 32);
                    HexPrint (32, &Data_pu8[i * 32]);
                    CI_LocalPrintf ("\n\r");
                }
            }

            break;
        case 6:    // Print topt slot header
            /*
             * slot structure: 1b 0x01 if slot is used (programmed) 15b slot
             * name 20b secret 1b configuration flags: MSB [x|x|x|x|x|send
             * token id|send enter after code?|no. of digits 6/8] LSB 12b
             * token id 1b keyboard layout 
             */
            Data_pu8 = get_totp_slot_addr (Param_u32);
            if (NULL == Data_pu8)
            {
                CI_LocalPrintf ("Wrong topt slot %d\n\r", Param_u32);
                return;
            }

            if (0x01 == Data_pu8[0])
            {
                CI_LocalPrintf ("slot %d is used\n\r", Param_u32);
            }
            else
            {
                CI_LocalPrintf ("slot %d is not used\n\r", Param_u32);
            }

            CI_LocalPrintf ("slot name -%.15.15s-\n\r", &Data_pu8[1]);
            CI_LocalPrintf ("        : ");
            HexPrint (15, &Data_pu8[1]);
            CI_LocalPrintf ("\n\r");


            CI_LocalPrintf ("Secret  : ");
            HexPrint (20, &Data_pu8[16]);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("Config  : ");
            HexPrint (1, &Data_pu8[36]);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("Token   : ");
            HexPrint (12, &Data_pu8[37]);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("Keyboard: ");
            HexPrint (1, &Data_pu8[39]);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("slot name -%.15.15s-\n\r", &Data_pu8[1]);
            break;

        case 7:    // Show topt time
            result = get_flash_time_value ();
            CI_LocalPrintf ("Flash time : 0x%08x - %ld\n\r", result, result);
            break;

        case 8:    // Set topt time
            CI_LocalPrintf ("Set flash time : to 0x%08x - %ld\n\r", 100, 100);
            set_time_value (100);
            result = get_flash_time_value ();
            CI_LocalPrintf ("Flash time : 0x%08x - %ld\n\r", result, result);
            break;


        case 11:
            // parse_report (NULL,NULL);
            break;


    }
}
