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

/*
 * tools.h
 *
 *  Created on: 24.06.2010
 *      Author: RB
 */

#ifndef TOOLS_H_
#define TOOLS_H_

#define CCID_TASK_DELAY_1_MS_IN_TICKS			1 * (configTICK_RATE_HZ/1000)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef unsigned long long u64;
typedef signed long long s64;

void Delay1Ms (void);
void DelayMs (int nMs);
void DelayCounterTest (void);

void ToolPinSet (void);
void ToolPinClr (void);

void HexPrint (int nNumberChars, unsigned char* sData);
void AsciiHexPrint (int nNumberChars, unsigned char sData[]);

void atoi_reverse (u8 * String_pu8, u32 * Value_u32, u8 MaxChars_u8);
void itoa (u32 n, u8 * s);
void itoa_s (s32 n, u8 * s);
void itoa_h (u32 n, u8 * s);
void reverse (char* s);


void CRC_InitCRC32 (void);
void CRC_CalcBlockCRC32 (u8 * Data_pu8, u32 Len_u32);
void CRC_CalcBlockCRC32_8 (u32 * Data_pu8, u32 Len_u32);
u32 CRC_GetCRC32 (void);
u32 generateCRC_len (u8 * data, u8 len);
u32 generateCRC (u8 * data);
u32 change_endian_u32 (u32 x);
u16 change_endian_u16 (u16 x);

void UpdateMsdLastAccessTimer (u32 NewTime);
void MSD_AccessManager100ms (void);

void memset_safe(void *const pnt, unsigned char val, const u32 len);

typedef struct
{
    u32 MSD_ReadCalls_u32;
    u32 MSD_BytesRead_u32;

    u32 MSD_WriteCalls_u32;
    u32 MSD_BytesWrite_u32;

    u32 MSD_LastReadAccess_u32;
    u32 MSD_LastWriteAccess_u32;

    u32 CCID_ReadCalls_u32;
    u32 CCID_BytesRead_u32;
    u32 CCID_WriteCalls_u32;
    u32 CCID_BytesWrite_u32;
} USB_Log_tst;

extern USB_Log_tst USB_Log_st;


#endif /* TOOLS_H_ */
