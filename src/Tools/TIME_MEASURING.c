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



#include <stdio.h>
#include "cycle_counter.h"
#include "FreeRTOS.h"
#include "task.h"
#include "intc.h"
#include "global.h"
#include "tools.h"
#include "TIME_MEASURING.h"


// #define TIME_MEASURING_ENABLE


#ifdef TIME_MEASURING_ENABLE


/*******************************************************************************

 Local defines

*******************************************************************************/


#define TIME_MEASURING_INT_ENABLE

#define TIME_MEASURING_MAX_TIME_SLICES          10

#define TIME_MEASURING_RUNTIMESLICE_IN_USEC     1000ul  // usec
#define TIME_MEASURING_RUNTIMESLICE_IN_TICKS    (TIME_MEASURING_RUNTIMESLICE_IN_USEC*TIME_MEASURING_TICKS_IN_USEC)

#define TIME_MEASURING_STOPTIMESLICE_IN_USEC    10000ul // usec
#define TIME_MEASURING_STOPTIMESLICE_IN_TICKS   (TIME_MEASURING_STOPTIMESLICE_IN_USEC*TIME_MEASURING_TICKS_IN_USEC)

#ifdef TIME_MEASURING_INT_ENABLE
#define TIME_MEASURING_INT_RUNTIMESLICE_IN_USEC     5ul // usec
#define TIME_MEASURING_INT_RUNTIMESLICE_IN_TICKS    (TIME_MEASURING_INT_RUNTIMESLICE_IN_USEC*TIME_MEASURING_TICKS_IN_USEC)

#define TIME_MEASURING_INT_STOPTIMESLICE_IN_USEC    50ul    // usec
#define TIME_MEASURING_INT_STOPTIMESLICE_IN_TICKS   (TIME_MEASURING_INT_STOPTIMESLICE_IN_USEC*TIME_MEASURING_TICKS_IN_USEC)
#endif // TIME_MEASURING_INT_ENABLE

#define TIME_MEASURING_MAX_ZEITSCHEIBEN (TIME_MEASURING_TIME_CMD+1)


/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/


typedef struct TIME_MEASURING_st
{
    u32 Events_u32;
    u64 LastStart_u64;
    u64 LastStop_u64;
    u32 LastRuntime_u32;
    u64 LastRuntime_u64;
    u32 LastStoptime_u32;
    u32 LastRuntimeMin_u32;
    u32 LastRuntimeMax_u32;
    u32 LastStoptimeMin_u32;
    u32 LastStoptimeMax_u32;
    u64 SumRuntime_u64;
    u32 RuntimeSliceCount_u32[TIME_MEASURING_MAX_TIME_SLICES];
} TIME_MEASURING_tst;


static char* TIME_MEASURING_TimerNames_u8[] = {
    "TIME  0 ",
    "IDF_10MS",
    "HTML10MS",
    "AD DATA ",
    "AD DATA1",
    "AD FILE ",
    "HID KEY ",
    "SD ACTIV",
    "TIME  8 ",
    "TIME  9 ",
    "TIME 10 ",
    "CCID UG ",
    "CCID US ",
    "MSD READ",
    "MSD WRIT",
    "TIME 15 ",
    "TIME 16 ",
    "TIME 17 ",
    "TIME 18 ",
    "TIME 19 ",
    "TIME 20 "
};


u32 TIME_MEASURING_RuntimeSlice_in_ticks_u32 = TIME_MEASURING_RUNTIMESLICE_IN_TICKS;
u32 TIME_MEASURING_StoptimeSlice_in_ticks_u32 = TIME_MEASURING_STOPTIMESLICE_IN_TICKS;

TIME_MEASURING_tst TIME_MEASURING_st[TIME_MEASURING_MAX_ZEITSCHEIBEN];

u8 NoTimereset_u8 = 0;          // Flag for CPU load output

#ifdef TIME_MEASURING_INT_ENABLE

static u8* TIME_MEASURING_IntTimerNames_u8[] = {
};

#define INT_TIME_MEASURING_MAX_ZEITSCHEIBEN 69

u32 TIME_MEASURING_INT_RuntimeSlice_in_ticks_u32 = TIME_MEASURING_INT_RUNTIMESLICE_IN_TICKS;
u32 TIME_MEASURING_INT_StoptimeSlice_in_ticks_u32 = TIME_MEASURING_INT_STOPTIMESLICE_IN_TICKS;

TIME_MEASURING_tst INT_TIME_MEASURING_st[INT_TIME_MEASURING_MAX_ZEITSCHEIBEN];

#endif // TIME_MEASURING_INT_ENABLE



extern const struct
{
    unsigned int num_irqs;
    volatile __int_handler* _int_line_handler_table;
} _int_handler_table[AVR32_INTC_NUM_INT_GRPS];

unsigned char TIME_MEASURING_INT_IntTableOffset[AVR32_INTC_NUM_INT_GRPS];

unsigned int TIME_MEASURING_INT_IntCount[TIME_MEASURING_INT_MAX_ENTRYS];


static char* INT_NameTable[] = {
    " 0, 0 CPU",    // CPU with optional MPU and optional OCD
    " 1, 0 INT 0",  // External Interrupt Controller EIC 0
    " 1, 1 INT 1",  // External Interrupt Controller EIC 1
    " 1, 2 INT 2",  // External Interrupt Controller EIC 2
    " 1, 3 INT 3",  // External Interrupt Controller EIC 3
    " 1, 4 INT 4",  // External Interrupt Controller EIC 4
    " 1, 5 INT 5",  // External Interrupt Controller EIC 5
    " 1, 6 INT 6",  // External Interrupt Controller EIC 6
    " 1, 7 INT 7",  // External Interrupt Controller EIC 7
    " 1, 8 RTC",    // Real Time Counter RTC
    " 1, 9 PM", // Power Manager PM
    " 1,10 UNKNOWN",    // Unbekannt AES ?
    " 2, 0 GPIO 0", // General Purpose Input/Output Controller GPIO 0
    " 2, 1 GPIO 1", // General Purpose Input/Output Controller GPIO 1
    " 2, 2 GPIO 2", // General Purpose Input/Output Controller GPIO 2
    " 2, 3 GPIO 3", // General Purpose Input/Output Controller GPIO 3
    " 2, 4 GPIO 4", // General Purpose Input/Output Controller GPIO 4
    " 2, 5 GPIO 5", // General Purpose Input/Output Controller GPIO 5
    " 2, 6 GPIO 6", // General Purpose Input/Output Controller GPIO 6
    " 2, 7 GPIO 7", // General Purpose Input/Output Controller GPIO 7
    " 2, 8 GPIO 8", // General Purpose Input/Output Controller GPIO 8
    " 2, 9 GPIO 9", // General Purpose Input/Output Controller GPIO 9
    " 2,10 GPIO 10",    // General Purpose Input/Output Controller GPIO 10
    " 2,11 GPIO 11",    // General Purpose Input/Output Controller GPIO 11
    " 2,12 GPIO 12",    // General Purpose Input/Output Controller GPIO 12
    " 2,13 GPIO 13",    // General Purpose Input/Output Controller GPIO 13
    " 3, 0 DMA 0",  // Peripheral DMA Controller PDCA 0
    " 3, 1 DMA 1",  // Peripheral DMA Controller PDCA 1
    " 3, 2 DMA 2",  // Peripheral DMA Controller PDCA 2
    " 3, 3 DMA 3",  // Peripheral DMA Controller PDCA 3
    " 3, 4 DMA 4",  // Peripheral DMA Controller PDCA 4
    " 3, 5 DMA 5",  // Peripheral DMA Controller PDCA 5
    " 3, 6 DMA 6",  // Peripheral DMA Controller PDCA 6
    " 3, 7 DMA 7",  // Peripheral DMA Controller PDCA 7
    " 4, 0 FLASHC", // Flash Controller FLASHC
    " 5, 0 USART0", // USART0
    " 6, 0 USART1", // USART1
    " 7, 0 USART2", // USART2
    " 8, 0 USART3", // USART3
    " 9, 0 SPI0",   // SPI0
    "10, 0 SPI1",   // SPI1
    "11, 0 TWIM0",  // Two-wire Master Interface 0
    "12, 0 TWIM1",  // Two-wire Master Interface 1
    "13, 0 SSC",    // Synchronous Serial Controller SSC
    "14, 0 TC00",   // Timer/Counter TC00
    "14, 1 TC01",   // Timer/Counter TC01
    "14, 2 TC02",   // Timer/Counter TC02
    "15, 0 ADC",    // Analog to Digital Converter ADC
    "16, 0 TC10",   // Timer/Counter TC10
    "16, 1 TC11",   // Timer/Counter TC11
    "16, 2 TC12",   // Timer/Counter TC12
    "17, 0 USBB",   // USB 2.0 OTG Interface USBB
    "18, 0 SDRAM",  // SDRAM Controller SDRAMC
    "19, 0 DAC",    // Audio Bitstream DAC ABDAC
    "20, 0 MCI",    // Mulitmedia Card Interface MCI
    "21, 0 AES",    // Advanced Encryption Standard AES
    "22, 0 DMA BLOCK",  // DMA Controller DMACA BLOCK
    "22, 1 DMA DSTT",   // DMA Controller DMACA DSTT
    "22, 2 DMA ERR",    // DMA Controller DMACA ERR
    "22, 3 DMA SRCT",   // DMA Controller DMACA SRCT
    "22, 4 DMA TFR",    // DMA Controller DMACA TFR
    "23, 0 UNKNOWN",    // Unbekannt
    "24, 0 UNKNOWN",    // Unbekannt
    "25, 0 UNKNOWN",    // Unbekannt
    "26, 0 MSI",    // Memory Stick Interface MSI
    "27, 0 TWIS0",  // Two-wire Slave Interface TWIS0
    "28, 0 TWIS1",  // Two-wire Slave Interface TWIS1
    "29, 0 ECCHRS", // Error code corrector Hamming and Reed Solomon
    "      UNKNOWN",    // Unknown
};


/*******************************************************************************

  TIME_MEASURING_ShortText

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

char* TIME_MEASURING_ShortText (u64 TimeInUsec_u64)
{
static char String_u8[15];

    if (10000 > TimeInUsec_u64)
    {
        sprintf (String_u8, "%6u us", (u32) TimeInUsec_u64);
        return (String_u8);
    }
    if (1000 * 1000 > TimeInUsec_u64)
    {
        sprintf (String_u8, "%6.2f ms", (float) TimeInUsec_u64 / 1000.0);
        return (String_u8);
    }
    if (1000ull * 1000ull * 1000ull > TimeInUsec_u64)
    {
        sprintf (String_u8, "%6.2f  s", (float) TimeInUsec_u64 / 1000000.0);
        return (String_u8);
    }
    if (1000ull * 1000ull * 1000ull * 1000ull > TimeInUsec_u64)
    {
        sprintf (String_u8, "%6.2f ks", (float) TimeInUsec_u64 / 1000000000.0);
        return (String_u8);
    }

    sprintf (String_u8, "<999ks");
    return (String_u8);
}

/*******************************************************************************

  TIME_MEASURING_Init_Timer

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_Init_Timer (void)
{
    Set_sys_compare (0);
    Set_sys_count (0);
}

/*******************************************************************************

  TIME_MEASURING_Init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u16 TIME_MEASURING_Init (void)
{
    u8 i;
    u8 i1;

    for (i = 0; i < TIME_MEASURING_MAX_ZEITSCHEIBEN; i++)
    {
        TIME_MEASURING_st[i].Events_u32 = 0;
        TIME_MEASURING_st[i].LastStart_u64 = 0;
        TIME_MEASURING_st[i].LastStop_u64 = 0;
        TIME_MEASURING_st[i].LastRuntime_u32 = 0;
        TIME_MEASURING_st[i].LastRuntimeMin_u32 = 0xFFFFFFFF;
        TIME_MEASURING_st[i].LastRuntimeMax_u32 = 0;
        TIME_MEASURING_st[i].LastStoptimeMin_u32 = 0xFFFFFFFF;
        TIME_MEASURING_st[i].LastStoptimeMax_u32 = 0;
        TIME_MEASURING_st[i].SumRuntime_u64 = 0;

        for (i1 = 0; i1 < TIME_MEASURING_MAX_TIME_SLICES; i1++)
        {
            TIME_MEASURING_st[i].RuntimeSliceCount_u32[i1] = 0;
        }
    }

#ifdef TIME_MEASURING_INT_ENABLE
    TIME_MEASURING_INT_SetTableOffsets ();

    for (i = 0; i < INT_TIME_MEASURING_MAX_ZEITSCHEIBEN; i++)
    {
        INT_TIME_MEASURING_st[i].Events_u32 = 0;
        INT_TIME_MEASURING_st[i].LastStart_u64 = 0;
        INT_TIME_MEASURING_st[i].LastStop_u64 = 0;
        INT_TIME_MEASURING_st[i].LastRuntime_u32 = 0;
        INT_TIME_MEASURING_st[i].LastRuntimeMin_u32 = 0xFFFFFFFF;
        INT_TIME_MEASURING_st[i].LastRuntimeMax_u32 = 0;
        INT_TIME_MEASURING_st[i].LastStoptimeMin_u32 = 0xFFFFFFFF;
        INT_TIME_MEASURING_st[i].LastStoptimeMax_u32 = 0;

        for (i1 = 0; i1 < TIME_MEASURING_MAX_TIME_SLICES; i1++)
        {
            INT_TIME_MEASURING_st[i].RuntimeSliceCount_u32[i1] = 0;
        }
    }
#endif // TIME_MEASURING_INT_ENABLE

    TIME_MEASURING_Init_Timer ();

    NoTimereset_u8++;

    return (TRUE);
}

/*******************************************************************************

  TIME_MEASURING_LogEvent

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_LogEvent (u8 Timer_u8)
{
    if (TIME_MEASURING_MAX_ZEITSCHEIBEN < Timer_u8)
    {
        return;
    }
    TIME_MEASURING_st[Timer_u8].Events_u32++;
}

/*******************************************************************************

  TIME_MEASURING_GetTime

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u64 TIME_MEASURING_GetTime (void)
{
static u32 CycelCounter_u32;
static u32 LastTime_u32;
static u32 TimeErrors_u32 = 0;
u32 NewTime_u32;
u32 NewTime1_u32;

    NewTime_u32 = Get_sys_count ();

    if (NewTime_u32 < LastTime_u32)
    {
        NewTime1_u32 = Get_sys_count ();
        if (NewTime1_u32 < LastTime_u32)
        {
            CycelCounter_u32++;
        }
        else
        {
            NewTime_u32 = NewTime1_u32;
            TimeErrors_u32++;   // A CPU bug ?
        }
    }

    LastTime_u32 = NewTime_u32;
    return ((u64) ((u64) CycelCounter_u32 << 32) + (u64) NewTime_u32);
}

/*******************************************************************************

  TIME_MEASURING_GetDiffTime

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u64 TIME_MEASURING_GetDiffTime (u64 StartTime_u64, u64 EndTime_u64)
{
    return (EndTime_u64 - StartTime_u64);
}

/*******************************************************************************

  TIME_MEASURING_Start_st

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_Start_st (TIME_MEASURING_tst * Time_st)
{

    Time_st->Events_u32++;

    Time_st->LastStart_u64 = TIME_MEASURING_GetTime ();

    if (0 != Time_st->LastStop_u64)
    {
        Time_st->LastStoptime_u32 = TIME_MEASURING_GetDiffTime (Time_st->LastStop_u64, Time_st->LastStart_u64);

        if (Time_st->LastStoptimeMin_u32 > Time_st->LastStoptime_u32)
        {
            Time_st->LastStoptimeMin_u32 = Time_st->LastStoptime_u32;
        }

        if (Time_st->LastStoptimeMax_u32 < Time_st->LastStoptime_u32)
        {
            Time_st->LastStoptimeMax_u32 = Time_st->LastStoptime_u32;
        }
    }
}

/*******************************************************************************

  TIME_MEASURING_Start

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_Start (u8 Timer_u8)
{
    if (TIME_MEASURING_MAX_ZEITSCHEIBEN < Timer_u8)
    {
        return;
    }

    TIME_MEASURING_Start_st (&TIME_MEASURING_st[Timer_u8]);
}

/*******************************************************************************

  TIME_MEASURING_Stop_st

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 TIME_MEASURING_Stop_st (TIME_MEASURING_tst * Time_st, u32 SlotSize_u32)
{
u32 Value_u32;

    Time_st->LastStop_u64 = TIME_MEASURING_GetTime ();
    Time_st->LastRuntime_u64 = TIME_MEASURING_GetDiffTime (Time_st->LastStart_u64, Time_st->LastStop_u64);
    Time_st->LastRuntime_u32 = Time_st->LastRuntime_u64;

    if (Time_st->LastRuntimeMin_u32 > Time_st->LastRuntime_u32)
    {
        Time_st->LastRuntimeMin_u32 = Time_st->LastRuntime_u32;
    }

    if (Time_st->LastRuntimeMax_u32 < Time_st->LastRuntime_u32)
    {
        Time_st->LastRuntimeMax_u32 = Time_st->LastRuntime_u32;
    }

    Value_u32 = Time_st->LastRuntime_u32 / SlotSize_u32;

    if (TIME_MEASURING_MAX_TIME_SLICES - 1 < Value_u32)
    {
        Value_u32 = TIME_MEASURING_MAX_TIME_SLICES - 1;
    }
    Time_st->RuntimeSliceCount_u32[Value_u32]++;

    if (1000000000ull > (u64) Time_st->LastRuntime_u32) // log only runtime < 1000/60 sec
    {
        Time_st->SumRuntime_u64 += (u64) Time_st->LastRuntime_u32;
    }

    return (Time_st->LastRuntime_u32);
}

/*******************************************************************************

  TIME_MEASURING_Stop

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 TIME_MEASURING_Stop (u8 Timer_u8)
{
    if (TIME_MEASURING_MAX_ZEITSCHEIBEN < Timer_u8)
    {
        return (0);
    }
    return (TIME_MEASURING_Stop_st (&TIME_MEASURING_st[Timer_u8], TIME_MEASURING_RuntimeSlice_in_ticks_u32));
}


/*******************************************************************************

  TIME_MEASURING_GetLastRuntimeU64

  Changes
  Date      Author          Info
  06.02.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u64 TIME_MEASURING_GetLastRuntimeU64 (u8 Timer_u8)
{
    return (TIME_MEASURING_st[Timer_u8].LastRuntime_u64);
}


/*******************************************************************************

  TIME_MEASURING_SetRuntimeSliceSize

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_SetRuntimeSliceSize (u32 NewSliceSizeInUsec_u32)
{
u32 i;
u32 i1;

    TIME_MEASURING_RuntimeSlice_in_ticks_u32 = NewSliceSizeInUsec_u32 * TIME_MEASURING_TICKS_IN_USEC;

    for (i = 0; i < TIME_MEASURING_MAX_ZEITSCHEIBEN; i++)
    {
        for (i1 = 0; i1 < TIME_MEASURING_MAX_TIME_SLICES; i1++)
        {
            TIME_MEASURING_st[i].RuntimeSliceCount_u32[i1] = 0;
        }
    }
}

/*******************************************************************************

  TIME_MEASURING_SetStoptimeSliceSize

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
/*
   void TIME_MEASURING_SetStoptimeSliceSize (u32 NewSliceSizeInUsec_u32) { u32 i; u32 i1;

   TIME_MEASURING_StoptimeSlice_in_ticks_u32 = NewSliceSizeInUsec_u32 * TIME_MEASURING_TICKS_IN_USEC;

   for (i=0;i<TIME_MEASURING_MAX_ZEITSCHEIBEN;i++) { for (i1=0;i1<TIME_MEASURING_MAX_TIME_SLICES;i1++) {
   TIME_MEASURING_st[i].RuntimeSliceCount_u32[i1] = 0; } } }

 */


/*******************************************************************************

  TIME_MEASURING_PrintLine

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_PrintLine (u8 Entry_u8)
{

    CI_LocalPrintf (" %s : %9ld   ", TIME_MEASURING_TimerNames_u8[Entry_u8], TIME_MEASURING_st[Entry_u8].Events_u32);

    if (0xFFFFFFFF != TIME_MEASURING_st[Entry_u8].LastRuntimeMin_u32)
    {
        CI_LocalPrintf ("%9ld ", TIME_MEASURING_st[Entry_u8].LastRuntimeMin_u32 / TIME_MEASURING_TICKS_IN_USEC);
    }
    else
    {
        CI_LocalPrintf ("          ");
    }
    CI_LocalPrintf ("%9ld  ", TIME_MEASURING_st[Entry_u8].LastRuntimeMax_u32 / TIME_MEASURING_TICKS_IN_USEC);

    if (0xFFFFFFFF != TIME_MEASURING_st[Entry_u8].LastStoptimeMin_u32)
    {
        CI_LocalPrintf ("%9ld ", TIME_MEASURING_st[Entry_u8].LastStoptimeMin_u32 / TIME_MEASURING_TICKS_IN_USEC);
    }
    else
    {
        CI_LocalPrintf ("          ");
    }
    CI_LocalPrintf ("%9ld ", TIME_MEASURING_st[Entry_u8].LastStoptimeMax_u32 / TIME_MEASURING_TICKS_IN_USEC);
    /*
       CI_LocalPrintf ("\n\r%9ld ",TIME_MEASURING_st[Entry_u8].SumRuntime_u32/TIME_MEASURING_TICKS_IN_USEC); CI_LocalPrintf (" %9ld ",OS_Time); */
    if (1 >= NoTimereset_u8)
    {
        CI_LocalPrintf ("%6.3f%%\n\r",
                        (float) (TIME_MEASURING_st[Entry_u8].SumRuntime_u64 / TIME_MEASURING_TICKS_IN_USEC) / (float) xTaskGetTickCount () / 10.0);
    }
    else
    {
        CI_LocalPrintf ("\n\r");
    }

}



/*******************************************************************************

  TIME_MEASURING_INT_show_interrupt_table

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_INT_show_interrupt_table (void)
{
    unsigned int int_grp, int_req, IntNamePointer;

    IntNamePointer = 0;

    // For all interrupt groups,
    CI_LocalPrintf ("Grp Nr   Addr\n\r");
    for (int_grp = 0; int_grp < AVR32_INTC_NUM_INT_GRPS; int_grp++)
    {
        // For all interrupt request lines of each group,
        for (int_req = 0; int_req < _int_handler_table[int_grp].num_irqs; int_req++)
        {
            if (&_unhandled_interrupt == _int_handler_table[int_grp]._int_line_handler_table[int_req])
            {
                CI_LocalPrintf ("%3d %3d %4d %-15s : %9d UNHANDLED\n\r", IntNamePointer, int_grp, int_req, INT_NameTable[IntNamePointer],
                                TIME_MEASURING_INT_IntCount[IntNamePointer]);
            }
            else
            {
                CI_LocalPrintf ("%3d %3d %4d %-15s : %9d 0x%08x\n\r", IntNamePointer, int_grp, int_req, INT_NameTable[IntNamePointer],
                                TIME_MEASURING_INT_IntCount[IntNamePointer], _int_handler_table[int_grp]._int_line_handler_table[int_req]);
            }
            IntNamePointer++;
            DelayMs (50);
        }
    }
    CI_LocalPrintf ("             %-15s : %9d\n\r", INT_NameTable[IntNamePointer], TIME_MEASURING_INT_IntCount[IntNamePointer]);
}

/*******************************************************************************

  TIME_MEASURING_INT_show_interrupt_count

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_INT_show_interrupt_count (void)
{
    unsigned int int_grp, int_req, IntNamePointer;

    IntNamePointer = 0;

    // For all interrupt groups,
    CI_LocalPrintf ("Grp Nr   Addr\n\r");
    for (int_grp = 0; int_grp < AVR32_INTC_NUM_INT_GRPS; int_grp++)
    {
        // For all interrupt request lines of each group,
        for (int_req = 0; int_req < _int_handler_table[int_grp].num_irqs; int_req++)
        {

            if (0 != TIME_MEASURING_INT_IntCount[IntNamePointer])
            {
                if ((__int_handler *) & _unhandled_interrupt == (__int_handler *) _int_handler_table[int_grp]._int_line_handler_table[int_req])
                {
                    CI_LocalPrintf ("%3d %4d %-15s : %9d UNHANDLED\n\r", int_grp, int_req, INT_NameTable[IntNamePointer],
                                    TIME_MEASURING_INT_IntCount[IntNamePointer]);
                }
                else
                {
                    CI_LocalPrintf ("%3d %4d %-15s : %9d 0x%08x\n\r", int_grp, int_req, INT_NameTable[IntNamePointer],
                                    TIME_MEASURING_INT_IntCount[IntNamePointer], _int_handler_table[int_grp]._int_line_handler_table[int_req]);
                }
            }
            IntNamePointer++;
        }
    }
    if (0 != TIME_MEASURING_INT_IntCount[TIME_MEASURING_INT_UNKONWN])
    {
        CI_LocalPrintf ("             %-15s : %9d\n\r", INT_NameTable[TIME_MEASURING_INT_UNKONWN],
                        TIME_MEASURING_INT_IntCount[TIME_MEASURING_INT_UNKONWN]);
    }
}

/*******************************************************************************

  TIME_MEASURING_INT_SetTableOffsets

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_INT_SetTableOffsets (void)
{
    /*
       unsigned int int_req; */
    unsigned int int_grp, IntNamePointer;
    IntNamePointer = 0;

    // For all interrupt groups,
    for (int_grp = 0; int_grp < AVR32_INTC_NUM_INT_GRPS; int_grp++)
    {
        /*
           // For all interrupt request lines of each group, for (int_req = 0; int_req < _int_handler_table[int_grp].num_irqs; int_req++) {
           IntNamePointer++; } */
        TIME_MEASURING_INT_IntTableOffset[int_grp] = IntNamePointer;
        IntNamePointer += _int_handler_table[int_grp].num_irqs;
    }
}

/*******************************************************************************

  TIME_MEASURING_INT_Infos

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_INT_Infos (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8)
{
    if (0 == nParamsGet_u8)
    {
        CI_LocalPrintf ("Interrupt infos\r\n");
        CI_LocalPrintf ("\r\n");
        CI_LocalPrintf ("0          Show Interrupttable\r\n");
        CI_LocalPrintf ("\r\n");
        return;
    }
    switch (CMD_u8)
    {
        case 0:
            TIME_MEASURING_INT_show_interrupt_table ();
            break;

        case 1:
            TIME_MEASURING_INT_show_interrupt_count ();
            break;

        case 5:
            if (0 == Param_u32)
            {
            }
            else
            {
            }
            break;

    }
}

/*******************************************************************************

  TIME_MEASURING_INT_Start

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_INT_Start (u8 Timer_u8)
{
#ifdef TIME_MEASURING_INT_ENABLE
    if (INT_TIME_MEASURING_MAX_ZEITSCHEIBEN < Timer_u8)
    {
        return;
    }

    TIME_MEASURING_Start_st (&INT_TIME_MEASURING_st[Timer_u8]);
#endif
}

/*******************************************************************************

  TIME_MEASURING_INT_Stop

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 TIME_MEASURING_INT_Stop (u8 Timer_u8)
{
#ifdef TIME_MEASURING_INT_ENABLE
    if (INT_TIME_MEASURING_MAX_ZEITSCHEIBEN < Timer_u8)
    {
        return (0);
    }

    return (TIME_MEASURING_Stop_st (&INT_TIME_MEASURING_st[Timer_u8], TIME_MEASURING_INT_RuntimeSlice_in_ticks_u32));
#else
    return (0);
#endif

}

#ifdef TIME_MEASURING_INT_ENABLE

/*******************************************************************************

  TIME_MEASURING_INT_Callback

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_INT_Callback (void)
{
    u8 Timer_u8 = 0;

    TIME_MEASURING_INT_Start (Timer_u8);

    // *(_int_handler_table[0]._int_line_handler_table[0]) (void);

    TIME_MEASURING_INT_Stop (Timer_u8);
}

/*******************************************************************************

  TIME_MEASURING_PrintIntLine

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_PrintIntLine (u8 Entry_u8)
{
    CI_LocalPrintf ("%-17s %9ld   ", TIME_MEASURING_IntTimerNames_u8[Entry_u8], INT_TIME_MEASURING_st[Entry_u8].Events_u32);

    if (0xFFFFFFFF != INT_TIME_MEASURING_st[Entry_u8].LastRuntimeMin_u32)
    {
        CI_LocalPrintf ("%9ld ", INT_TIME_MEASURING_st[Entry_u8].LastRuntimeMin_u32 / TIME_MEASURING_TICKS_IN_USEC);
    }
    else
    {
        CI_LocalPrintf ("          ");
    }
    CI_LocalPrintf ("%9ld  ", INT_TIME_MEASURING_st[Entry_u8].LastRuntimeMax_u32 / TIME_MEASURING_TICKS_IN_USEC);

    if (0xFFFFFFFF != INT_TIME_MEASURING_st[Entry_u8].LastStoptimeMin_u32)
    {
        CI_LocalPrintf ("%9ld ", INT_TIME_MEASURING_st[Entry_u8].LastStoptimeMin_u32 / TIME_MEASURING_TICKS_IN_USEC);
    }
    else
    {
        CI_LocalPrintf ("          ");
    }
    CI_LocalPrintf ("%10ld ", INT_TIME_MEASURING_st[Entry_u8].LastStoptimeMax_u32 / TIME_MEASURING_TICKS_IN_USEC);

    if (1 >= NoTimereset_u8)
    {
        // CI_LocalPrintf ("%5.3f%%\n\r",(float)(INT_TIME_MEASURING_st[Entry_u8].SumRuntime_u64/TIME_MEASURING_TICKS_IN_USEC)/(float)OS_Time/10);
    }
    else
    {
        CI_LocalPrintf ("\n\r");
    }
}

/*******************************************************************************

  TIME_MEASURING_PrintInt

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_PrintInt (void)
{
    u8 i;

    CI_LocalPrintf ("Interrupt Runtime-Analyse\n\r");
    CI_LocalPrintf ("   Interrupt                      Runtime              Stoptime\n\r");
    CI_LocalPrintf (" Nr   Name           Anzahl  Min (usec) Max(usec) Min (usec)  Max(usec)\n\r");

    for (i = 0; i < INT_TIME_MEASURING_MAX_ZEITSCHEIBEN; i++)
    {
        if (0 != INT_TIME_MEASURING_st[i].Events_u32)
        {
            TIME_MEASURING_PrintIntLine (i);
        }
    }
}

/*******************************************************************************

  TIME_MEASURING_PrintIntRuntimeSlots

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_PrintIntRuntimeSlots (u8 Entry_u8)
{
u8 i;

    if (INT_TIME_MEASURING_MAX_ZEITSCHEIBEN <= Entry_u8)
    {
        CI_LocalPrintf ("ERROR: Entry %d not found. Max slot is %d\n\r", Entry_u8, INT_TIME_MEASURING_MAX_ZEITSCHEIBEN - 1);
        return;
    }

    CI_LocalPrintf ("Interrupt-slot %s\n\r\n\r", TIME_MEASURING_IntTimerNames_u8[Entry_u8]);


    CI_LocalPrintf ("Runtime (usec)          Count\n\r");
    for (i = 0; i < TIME_MEASURING_MAX_TIME_SLICES - 1; i++)
    {
        CI_LocalPrintf (" %5ld - %5ld : %9ld\n\r", TIME_MEASURING_INT_RuntimeSlice_in_ticks_u32 * i / TIME_MEASURING_TICKS_IN_USEC,
                        TIME_MEASURING_INT_RuntimeSlice_in_ticks_u32 * (i + 1) / TIME_MEASURING_TICKS_IN_USEC,
                        INT_TIME_MEASURING_st[Entry_u8].RuntimeSliceCount_u32[i]);
    }
    CI_LocalPrintf ("       > %5ld : %9ld\n\r",
                    TIME_MEASURING_INT_RuntimeSlice_in_ticks_u32 * (TIME_MEASURING_MAX_TIME_SLICES - 1) / TIME_MEASURING_TICKS_IN_USEC,
                    INT_TIME_MEASURING_st[Entry_u8].RuntimeSliceCount_u32[TIME_MEASURING_MAX_TIME_SLICES - 1]);
}


#else
void TIME_MEASURING_PrintIntRuntimeSlots (u8 Entry_u8)
{
    CI_LocalPrintf ("Interruptzeitmessung nicht aktiv\n\r");
}
#endif // TIME_MEASURING_INT_ENABLE

/*******************************************************************************

  TIME_MEASURING_PrintRuntimeSlots

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_PrintRuntimeSlots (u8 Entry_u8)
{
u8 i;

    if (TIME_MEASURING_MAX_ZEITSCHEIBEN <= Entry_u8)
    {
        CI_LocalPrintf ("ERROR: Entry %d not found. Max slot is %d\n\r", Entry_u8, TIME_MEASURING_MAX_TIME_SLICES - 1);
        return;
    }

    CI_LocalPrintf ("Runtimeslot %s\n\r\n\r", TIME_MEASURING_TimerNames_u8[Entry_u8]);

    CI_LocalPrintf ("Runtime (usec)          Count\n\r");
    for (i = 0; i < TIME_MEASURING_MAX_TIME_SLICES - 1; i++)
    {
        CI_LocalPrintf (" %5ld - %5ld : %9ld\n\r", TIME_MEASURING_RuntimeSlice_in_ticks_u32 * i / TIME_MEASURING_TICKS_IN_USEC,
                        TIME_MEASURING_RuntimeSlice_in_ticks_u32 * (i + 1) / TIME_MEASURING_TICKS_IN_USEC,
                        TIME_MEASURING_st[Entry_u8].RuntimeSliceCount_u32[i]);
    }
    CI_LocalPrintf ("       > %5ld : %9ld\n\r",
                    TIME_MEASURING_RuntimeSlice_in_ticks_u32 * (TIME_MEASURING_MAX_TIME_SLICES - 1) / TIME_MEASURING_TICKS_IN_USEC,
                    TIME_MEASURING_st[Entry_u8].RuntimeSliceCount_u32[TIME_MEASURING_MAX_TIME_SLICES - 1]);
}

/*******************************************************************************

  TIME_MEASURING_Print

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void TIME_MEASURING_Print (u8 Value_u8)
{
    switch (Value_u8)
    {
        case 1:
            TIME_MEASURING_Init (); // Clear counters
            return;
        case 2:
#ifdef TIME_MEASURING_INT_ENABLE
            TIME_MEASURING_PrintInt ();
#else
            CI_LocalPrintf ("Interruptzeitmessung nicht aktiv\n\r");
#endif
            return;

    }

    CI_LocalPrintf ("Runtime-Analyse\n\r");
    CI_LocalPrintf ("                             Runtime              Stoptime\n\r");
    CI_LocalPrintf ("Counter        Anzahl  Min (usec) Max(usec) Min (usec) Max(usec)\n\r");
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIMER_IDF_10MS);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIMER_IW_10MS);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_3);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_4);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_5);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIMER_KEY_10MS);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_SD_ACCESS);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_8);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_9);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_10);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_CCID_USB_GET);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_CCID_USB_SEND);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_MSD_AES_READ);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_MSD_AES_WRITE);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_15);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_16);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_17);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_18);
    TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_FOR_ALL);
    // TIME_MEASURING_PrintLine (TIME_MEASURING_TIME_20 ); // CMD Laufzeit
}

/*******************************************************************************

  time_ms

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 time_ms (u32 * timer)
{
    if (timer != NULL)
    {
        *timer = (u32) (TIME_MEASURING_GetTime () / (u64) TIME_MEASURING_TICKS_IN_USEC / 1000LL);
        *timer %= 1000;
        return (*timer);
    }
    return (1);
}


#endif // TIME_MEASURING_ENABLE
