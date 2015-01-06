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


#ifndef _TIME_MEASURING_H_
#define _TIME_MEASURING_H_

#include "intc.h"

// Define for interrupt measuring
#define TIME_MEASURING_INT_MAX_ENTRYS   (68+1)      // +1 for unknown interrupt
#define TIME_MEASURING_INT_UNKONWN      (TIME_MEASURING_INT_MAX_ENTRYS-1)

extern unsigned char TIME_MEASURING_INT_IntTableOffset[AVR32_INTC_NUM_INT_GRPS];
extern unsigned int  TIME_MEASURING_INT_IntCount[TIME_MEASURING_INT_MAX_ENTRYS];




// Defines for mearsuring slots

#define TIME_MEASURING_TIMER_IDF_10MS    1
#define TIME_MEASURING_TIMER_IW_10MS     2
#define TIME_MEASURING_TIME_3            3
#define TIME_MEASURING_TIME_4         4
#define TIME_MEASURING_TIME_5         5
#define TIME_MEASURING_TIMER_KEY_10MS    6
#define TIME_MEASURING_TIME_SD_ACCESS    7
#define TIME_MEASURING_TIME_8         8
#define TIME_MEASURING_TIME_9         9
#define TIME_MEASURING_TIME_10       10
#define TIME_MEASURING_TIME_CCID_USB_GET    11
#define TIME_MEASURING_TIME_CCID_USB_SEND   12
#define TIME_MEASURING_TIME_MSD_AES_READ    13
#define TIME_MEASURING_TIME_MSD_AES_WRITE   14
#define TIME_MEASURING_TIME_15       15
#define TIME_MEASURING_TIME_16       16
#define TIME_MEASURING_TIME_17       17
#define TIME_MEASURING_TIME_18       18
#define TIME_MEASURING_TIME_FOR_ALL  19
#define TIME_MEASURING_TIME_CMD      20

#define TIME_MEASURING_TICKS_IN_USEC              (FCPU_HZ/1000000ul)

char *TIME_MEASURING_ShortText (u64 TimeInUsec_u64);
u16  TIME_MEASURING_Init (void);
u64  TIME_MEASURING_GetTime (void);
void TIME_MEASURING_Start (u8 Timer);
void TIME_MEASURING_INT_Start (u8 Timer);
u32  TIME_MEASURING_Stop (u8 Timer);
u32  TIME_MEASURING_INT_Stop (u8 Timer);
void TIME_MEASURING_SetRuntimeSliceSize (u32 NewSliceSizeInUsec_u32);
void TIME_MEASURING_PrintRuntimeSlots (u8 Entry);
void TIME_MEASURING_PrintIntRuntimeSlots (u8 Entry);
void TIME_MEASURING_Print (u8 Value);
u32 time_ms(u32* timer);

u64 TIME_MEASURING_GetLastRuntimeU64 (u8 Timer_u8);

void TIME_MEASURING_INT_Infos (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8);
void TIME_MEASURING_INT_SetTableOffsets (void);


#endif  /* ifndef _TIME_MEASURING_H_ */
