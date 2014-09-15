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

/*
 * tools.c
 *
 *  Created on: 24.06.2010
 *      Author: RB
 */


#include <avr32/io.h>
#include "compiler.h"
#include "board.h"
#include "gpio.h"
#include "tools.h"
#include "TIME_MEASURING.h"

#include "ctype.h"
#include "string.h"

// Include ISO7816_USE_TASK
#include "CCID/USART/ISO7816_USART.h"

#ifdef ISO7816_USE_TASK
	#include "FreeRTOS.h"
	#include "task.h"
#endif

#include "global.h"

int CI_LocalPrintf (char *szFormat,...);


/*******************************************************************************

 Local defines

*******************************************************************************/

#define TOOL_TEST_PIN     AVR32_PIN_PX10  // = UART2 - RX


// Delay1Ms_Counting

//#define TOOLS_DELAY_1MS_COUNT   910 // at 12 MHz
#define TOOLS_DELAY_1MS_COUNT   (4200) // At 60 MHz


/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

u32 getu32(u8 *array);

/*******************************************************************************

 Local declarations

*******************************************************************************/

/*******************************************************************************

  Delay1Ms_Counting

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void Delay1Ms_Counting (void)
{
	register int i;

	for (i=0;i<TOOLS_DELAY_1MS_COUNT;i++)
	{
	}
}

/*******************************************************************************

  Delay1Ms_Taskdelay

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#ifdef ISO7816_USE_TASK
void Delay1Ms_Taskdelay (void)
{
	vTaskDelay(CCID_TASK_DELAY_1_MS_IN_TICKS);
}
#endif // ISO7816_USE_TASK

/*******************************************************************************

  Delay1Ms

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void Delay1Ms (void)
{
#ifdef ISO7816_USE_TASK
	Delay1Ms_Taskdelay ();
#else
	Delay1Ms_Counting ();
#endif // ISO7816_USE_TASK

}

/*******************************************************************************

  DelayMs

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void DelayMs (int nMs)
{
	int i;

	for (i=0;i<nMs;i++)
	{
		Delay1Ms ();
	}
}
/*******************************************************************************

  DelayCounterTest

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void DelayCounterTest (void)
{
	while (1)
	{
		ToolPinSet ();
		ToolPinClr ();
		ToolPinSet ();
		ToolPinClr ();
		DelayMs (10);
		ToolPinSet ();
		DelayMs (10);
		ToolPinClr ();
		DelayMs (10);
		ToolPinSet ();
		DelayMs (20);
		ToolPinClr ();
		DelayMs (20);
	}
}
/*******************************************************************************

  ToolPinSet

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void ToolPinSet (void)
{
#ifndef STICK_20_A_MUSTER_PROD
	gpio_set_gpio_pin (TOOL_TEST_PIN);
#endif
}

/*******************************************************************************

  ToolPinClr

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void ToolPinClr (void)
{
#ifndef STICK_20_A_MUSTER_PROD
	gpio_clr_gpio_pin (TOOL_TEST_PIN);
#endif
}

/*******************************************************************************

  HexPrint

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void HexPrint (int nNumberChars,unsigned char *sData)
{
  int i;
  unsigned char c;

  for (i=0;i<nNumberChars;i++)
  {
    c = sData[i];
    CI_LocalPrintf ("%02x ",c);
  }
}
/*******************************************************************************

  AsciiHexPrint

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AsciiHexPrint (int nNumberChars,unsigned char sData[])
{
  int i;
  unsigned char c;


  CI_LocalPrintf ("\"");
  for (i=0;i<nNumberChars;i++)
  {
    c = sData[i];
    if (0 != isprint (sData[i]))
    {
      CI_LocalPrintf ("%c",sData[i]);
    }
    else
    {
      CI_LocalPrintf (".");
    }
  }
  CI_LocalPrintf ("\" = ");

  for (i=0;i<nNumberChars;i++)
  {
    c = sData[i];
    CI_LocalPrintf ("%02x ",c);
  }
}
/*******************************************************************************

  CRC 32 calculation

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define CRC_POLYNOM_32_REV      0xEDB88320     /* CRC-32 polynomial, reverse */
static u32 CRC_Reg_u32 = 0xffffffff;

/*******************************************************************************

  CRC_InitCRC32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CRC_InitCRC32 (void)
{
  CRC_Reg_u32 = 0xffffffff;
}
/*******************************************************************************

  CRC_CalcBlockCRC32_1

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CRC_CalcBlockCRC32_1 (u8 *Data_pu8, u32 Len_u32)
{
  u32 n;
  u32 i;
  u32 k;
  u8 output[4];

  for (i=0;i < Len_u32; i++)
  {
    k = getu32((u8*)&Data_pu8[i*4]);
    output[0] =  k      & 0xFF;
    output[1] = (k>>8)  & 0xFF;
    output[2] = (k>>16) & 0xFF;
    output[3] = (k>>24) & 0xFF;

    for (k=0;k<4;k++)
    {
      for (n=0;n<8;n++)
      {
        if ((CRC_Reg_u32 & 1) != ((output[k] >> n) & 1))
        {
          CRC_Reg_u32 = (CRC_Reg_u32 >> 1) ^ CRC_POLYNOM_32_REV;
        }
        else
        {
          CRC_Reg_u32 = CRC_Reg_u32 >> 1;
        }
      }
    }
  }
}

/*******************************************************************************

  CRC_CalcBlockCRC32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CRC_CalcBlockCRC32 (u8 *Data_pu8, u32 Len_u32)
{
  u32 n;
  u32 i;

  for (i=0;i < Len_u32; i++)
  {
    for (n=0;n<8;n++)
    {
      if ((CRC_Reg_u32 & 1) != ((Data_pu8[i] >> n) & 1))
      {
        CRC_Reg_u32 = (CRC_Reg_u32 >> 1) ^ CRC_POLYNOM_32_REV;
      }
      else
      {
        CRC_Reg_u32 = CRC_Reg_u32 >> 1;
      }
    }
  }
}
/*******************************************************************************

  CRC_GetCRC32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 CRC_GetCRC32 (void)
{
  return (CRC_Reg_u32 ^ 0xFFFFFFFF);
}


/*******************************************************************************

  Crc32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 Crc32(u32 Crc, u32 Data)
{
    s32 i;

    Crc = Crc ^ Data;

    for(i=0; i<32; i++)
        if (Crc & 0x80000000)
            Crc = (Crc << 1) ^ 0x04C11DB7; // Polynomial used in STM32
        else
            Crc = (Crc << 1);

    return(Crc);
}
/*******************************************************************************

  generateCRC_len

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define REPORT_SIZE  64
#define COMMAND_SIZE 59

u32 generateCRC_len (u8 *data,u8 len)
{
//    u8 report[REPORT_SIZE+1];
    u32 crc=0xffffffff;
    u32 value;
    s32 i;

//    memset(report,0,sizeof(report));

    for (i=0;i<len;i++)
    {
//      value = ((u32 *)(data))[i];
      value = getu32 (&data[i*4]);
      crc=Crc32(crc,value);
    }

    return (crc);
}

/*******************************************************************************

  generateCRC

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 generateCRC(u8 *data)
{
//    u8 report[REPORT_SIZE+1];
    u32 crc=0xffffffff;
    u32 value;
    s32 i;

//    memset(report,0,sizeof(report));

    for (i=0;i<15;i++)
    {
//      value = ((u32 *)(data))[i];
      value = getu32 (&data[i*4]);
      crc=Crc32(crc,value);
    }

    return (crc);
}
/*******************************************************************************

  generateCRC_org

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 generateCRC_org(u8 commandType,u8 *data)
{
    u8 report[REPORT_SIZE+1];
    u32 crc=0xffffffff;
    s32 i;

    memset(report,0,sizeof(report));
    report[1] = commandType;

    memcpy(&report[2],&data[0],COMMAND_SIZE);

    for (i=0;i<15;i++){
        crc=Crc32(crc,((u32 *)(report+1))[i]);
    }
    ((u32 *)(report+1))[15]=crc;

    return (crc);
}

/*******************************************************************************

  itoa

  Changes
  Date      Reviewer        Info
  06.02.14  RB              Function created
                            Based on K&R2. Page 64

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void itoa(u32 n, u8 *s)
{
    s32 i, sign;

    sign = n;

    i = 0;
    do {
        s[i] = n % 10 + '0';
        i++;
    } while ((n /= 10) > 0);

    s[i] = '\0';
    reverse(s);
}

/*******************************************************************************

  itoa_h

  Changes
  Date      Reviewer        Info
  15.09.14  RB              Function created
                            Based on K&R2. Page 64

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void itoa_h (u32 n, u8 *s)
{
    s32 i;

    i = 0;
    do {
      if (10 > n % 16 )
      {
        s[i] = n % 16 + '0';
      }
      else
      {
        s[i] = n % 16 - 10 + 'A';

      }
      i++;
    } while ((n /= 16) > 0);

    s[i] = '\0';
    reverse(s);
}

/*******************************************************************************

  itoa_s

  Changes
  Date      Reviewer        Info
  06.02.14  RB              Function created
                            Based on K&R2. Page 64

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void itoa_s(s32 n, u8 *s)
{
    s32 i, sign;

    sign = n;

    if (0 > sign)
    {
        n = -n;
    }

    i = 0;
    do {
        s[i] = n % 10 + '0';
        i++;
    } while ((n /= 10) > 0);

    if (sign < 0)
    {
        s[i] = '-';
        i++;
    }

    s[i] = '\0';
    reverse(s);
}

/*******************************************************************************

  reverse

  Changes
  Date      Reviewer        Info
  06.02.14  RB              Function created
                            Based on K&R2. Page 64

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void reverse(char *s)
{
    int  i,j;
    char c;

    j = strlen(s) - 1;

    for (i=0;i<j;i++,j--)
    {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/*******************************************************************************

  change_endian_u32

  Changes
  Date      Reviewer        Info
  14.03.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 change_endian_u32 (u32 x)
{
    return (((x)<<24) | ((x)>>24) | (((x)& 0x0000ff00)<<8) | (((x)& 0x00ff0000)>>8));
}

/*******************************************************************************

  change_endian_u16

  Changes
  Date      Reviewer        Info
  07.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u16 change_endian_u16 (u16 x)
{
    return ( (((x)& 0x00ff)<<8) | (((x)& 0xff00)>>8) );
}

