/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 19.05.2011
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
 * AD_Test.c
 *
 *  Created on: 19.05.2011
 *      Author: RB
 */


#include "board.h"

#ifdef FREERTOS_USED
#include "FreeRTOS.h"
#include "task.h"
#endif
#include "string.h"
#include "print_funcs.h"
#include "gpio.h"
#include "pm.h"
#include "adc.h"
#include "time.h"
#include "global.h"
#include "AD_Test.h"
#include "tools.h"
#include "Interpreter.h"
#include "BUFFERED_SIO.h"
#include "..\FILE_IO\FileAccessInterface.h"
#include "ctrl_access.h"
#include "Inbetriebnahme.h"
#include "TIME_MEASURING.h"

/*
 * FAT includes 
 */
#include "file.h"
#include "navigation.h"
#include "ctrl_access.h"
#include "fsaccess.h"



#ifdef AD_LOGGING_ACTIV

/*******************************************************************************

 Local defines

*******************************************************************************/

#define EXAMPLE_ADC_TEST_CHANNEL           2
#define EXAMPLE_ADC_TEST_PIN               AVR32_ADC_AD_2_PIN
#define EXAMPLE_ADC_TEST_FUNCTION          AVR32_ADC_AD_2_FUNCTION

#define AD_SAMPELS_PER_SEC          50
#define AD_OUTPUTLINES_PER_SEC       5
#define AD_FILTERED_OUTPUTSAMPLES   25
#define AD_FILTER_SIZE              AD_FILTERED_OUTPUTSAMPLES

#define AD_FILEIO_LUN                  1    // SD Card
#define AD_FILEIO_WORKING_DIR     "AD_DATA"

#define AD_MINUTES_COUNT        60
#define AD_HOURS_COUNT          24
#define AD_EVENTLOG_COUNT      100

#define AD_EVENTBUFFER_SIZE 1000    // = 1000 ms

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

typedef struct
{
    u32 LastStartSec_u32;
    u32 LastStart_Msec_u32;
    u32 LastValue_u32;
    u32 Filter_Sum_u32;
    u32 Filter_SampleCount_u32;
    u16 FilterMax_u16;
    u16 FilterMin_u16;
    u16 ValueMinMinutes_u16[AD_MINUTES_COUNT];
    u16 ValueMaxMinutes_u16[AD_MINUTES_COUNT];
    u32 ValueMinutesTime_u32[AD_MINUTES_COUNT];
    u8 PointerMinutesStart_u8;
    u8 PointerMinutesEnd_u8;
    u16 ValueMinHours_u16[AD_HOURS_COUNT];
    u16 ValueMaxHours_u16[AD_HOURS_COUNT];
    u32 ValueHoursTime_u16[AD_HOURS_COUNT];
    u8 PointerHoursStart_u8;
    u8 PointerHoursEnd_u8;
    u8 EventLogStart_u8;
    u8 EventLogEnd_u8;
    u32 EventLogEventTime[AD_EVENTLOG_COUNT];
} AD_Datastore_tst;

AD_Datastore_tst AD_Data_st;

u32 AD_Filter_Sum_u32 = 0;
u32 AD_Filter_SampleCount_u32 = 0;

u16 AD_Filter_Data_u16[AD_FILTER_SIZE];
u16 AD_Filter_Pointer_u16 = 0;
u16 AD_Filter_Min = 0xFFFF;
u16 AD_Filter_Max = 0;

u8 AD_FileLogFlag_u8 = FALSE;

u16 AD_EventBufferActiv_u16 = TRUE;
u16 AD_EventBufferStopInSamples_u16 = 0;
u16 AD_EventBufferPointer_u16 = 0;
u16 AD_EventBuffer_u16[AD_EVENTBUFFER_SIZE];


/*******************************************************************************

  AD_init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_init (void)
{
    u32 i;

    static const gpio_map_t ADC_GPIO_MAP = {
        {EXAMPLE_ADC_TEST_PIN, EXAMPLE_ADC_TEST_FUNCTION},
    };

    volatile avr32_adc_t *adc = &AVR32_ADC; // ADC IP registers address

    // Assign and enable GPIO pins to the ADC function.
    gpio_enable_module (ADC_GPIO_MAP,
                        sizeof (ADC_GPIO_MAP) / sizeof (ADC_GPIO_MAP[0]));

    // configure ADC
    // Lower the ADC clock to match the ADC characteristics (because we
    // configured
    // the CPU clock to 12MHz, and the ADC clock characteristics are usually
    // lower;
    // cf. the ADC Characteristic section in the datasheet).
    AVR32_ADC.mr |= 0x8 << AVR32_ADC_MR_PRESCAL_OFFSET;
    adc_configure (adc);

    // Enable the ADC channels.
    adc_enable (adc, EXAMPLE_ADC_TEST_CHANNEL);

    AD_Filter_Min = 0xFFFF;
    AD_Filter_Max = 0;

    AD_Filter_Sum_u32 = 0;
    AD_Filter_SampleCount_u32 = 0;

    // Init AD log struct
    AD_Data_st.LastStartSec_u32 = 0;
    AD_Data_st.LastStart_Msec_u32 = 0;
    AD_Data_st.LastValue_u32 = 0;


    AD_Data_st.Filter_Sum_u32 = 0;
    AD_Data_st.Filter_SampleCount_u32 = 0;
    AD_Data_st.FilterMax_u16 = 0;
    AD_Data_st.FilterMin_u16 = 0xffff;

    AD_Data_st.PointerMinutesStart_u8 = 0;
    AD_Data_st.PointerMinutesEnd_u8 = 0;
    for (i = 0; i < AD_MINUTES_COUNT; i++)
    {
        AD_Data_st.ValueMinMinutes_u16[i] = 0;
        AD_Data_st.ValueMaxMinutes_u16[i] = 0;
        AD_Data_st.ValueMinutesTime_u32[i] = 0;
    }

    AD_Data_st.PointerHoursStart_u8 = 0;
    AD_Data_st.PointerHoursEnd_u8 = 0;
    for (i = 0; i < AD_HOURS_COUNT; i++)
    {
        AD_Data_st.ValueMinHours_u16[i] = 0;
        AD_Data_st.ValueMaxHours_u16[i] = 0;
        AD_Data_st.ValueHoursTime_u16[i] = 0;
    }

    AD_Data_st.EventLogStart_u8 = 0;
    AD_Data_st.EventLogEnd_u8 = 0;
    for (i = 0; i < AD_EVENTLOG_COUNT; i++)
    {
        AD_Data_st.EventLogEventTime[i] = 0;
    }

}

/*******************************************************************************

  AD_CheckTimeSet

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 AD_CheckTimeSet (void)
{
    time_t Now_u32;

    time (&Now_u32);

    if (1000000 > Now_u32)
    {
        return (FALSE);
    }
    return (TRUE);
}

/*******************************************************************************

  iir

  WinFilter version 0.8
  http://www.winfilter.20m.com
  akundert@hotmail.com

  Filter type: Low Pass
  Filter model: Bessel
  Filter order: 2
  Sampling Frequency: 1000 Hz
  Cut Frequency: 20.000000 Hz
  Coefficents Quantization: 16-bit

  Z domain Zeros
  z = -1.000000 + j 0.000000
  z = -1.000000 + j 0.000000

  Z domain Poles
  z = 0.894958 + j -0.056458
  z = 0.894958 + j 0.056458

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


#define NCoef 2
#define DCgain 256

s16 iir (s16 NewSample)
{
s16 ACoef[NCoef + 1] = {
    14888,
    29777,
    14888
};

s16 BCoef[NCoef + 1] = {
    16384,
    -29326,
    13175
};

static s32 y[NCoef + 1];    // output samples

    // Warning!!!!!! This variable should be signed (input sample width +
    // Coefs width + 2 )-bit width to avoid saturation.

static s16 x[NCoef + 1];    // input samples
int n;

    // shift the old samples
    for (n = NCoef; n > 0; n--)
    {
        x[n] = x[n - 1];
        y[n] = y[n - 1];
    }

    // Calculate the new output
    x[0] = NewSample;
    y[0] = ACoef[0] * x[0];
    for (n = 1; n <= NCoef; n++)
        y[0] += ACoef[n] * x[n] - BCoef[n] * y[n];

    y[0] /= BCoef[0];

    return y[0] / DCgain;
}

/*******************************************************************************

  AD_LogHourData_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_LogHourData_v (void)
{
    u32 i;
    u16 min_u16 = 0xffff;
    u16 max_u16 = 0;

    for (i = 0; i < AD_MINUTES_COUNT; i++)
    {

        if (AD_Data_st.ValueMinMinutes_u16[i] < min_u16)
        {
            min_u16 = AD_Data_st.ValueMinMinutes_u16[i];
        }

        if (AD_Data_st.ValueMaxMinutes_u16[i] > max_u16)
        {
            max_u16 = AD_Data_st.ValueMaxMinutes_u16[i];
        }
    }

    AD_Data_st.ValueHoursTime_u16[AD_Data_st.PointerHoursEnd_u8] =
        AD_Data_st.ValueMinutesTime_u32[0];
    AD_Data_st.ValueMinHours_u16[AD_Data_st.PointerHoursEnd_u8] = min_u16;
    AD_Data_st.ValueMaxHours_u16[AD_Data_st.PointerHoursEnd_u8] = max_u16;

    // Manage barrel buffer
    AD_Data_st.PointerHoursEnd_u8++;
    if (AD_HOURS_COUNT <= AD_Data_st.PointerHoursEnd_u8)
    {
        AD_Data_st.PointerHoursEnd_u8 = 0;
    }

    if (AD_Data_st.PointerHoursStart_u8 == AD_Data_st.PointerHoursEnd_u8)
    {
        AD_Data_st.PointerHoursStart_u8++;
        if (AD_HOURS_COUNT <= AD_Data_st.PointerHoursEnd_u8)
        {
            AD_Data_st.PointerHoursEnd_u8 = 0;
        }
    }

}

/*******************************************************************************

  AD_LogMinuteData_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_LogMinuteData_v (void)
{
    AD_Data_st.ValueMinutesTime_u32[AD_Data_st.PointerMinutesEnd_u8] =
        AD_Data_st.LastStartSec_u32;
    AD_Data_st.ValueMinMinutes_u16[AD_Data_st.PointerMinutesEnd_u8] =
        AD_Data_st.FilterMin_u16;
    AD_Data_st.ValueMaxMinutes_u16[AD_Data_st.PointerMinutesEnd_u8] =
        AD_Data_st.FilterMax_u16;

    // Manage barrel buffer
    AD_Data_st.PointerMinutesEnd_u8++;
    if (AD_MINUTES_COUNT <= AD_Data_st.PointerMinutesEnd_u8)
    {
        AD_Data_st.PointerMinutesEnd_u8 = 0;
    }

    if (AD_Data_st.PointerMinutesStart_u8 == AD_Data_st.PointerMinutesEnd_u8)
    {
        AD_Data_st.PointerMinutesStart_u8++;
        if (AD_MINUTES_COUNT <= AD_Data_st.PointerMinutesStart_u8)
        {
            AD_Data_st.PointerMinutesStart_u8 = 0;
        }
    }
}

/*******************************************************************************

  AD_GetData_u16

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u16 AD_GetData_u16 (void)
{
    u32 Sum_32 = 0;
    static u32 NextMinuteLog_u32 = 0;
    static u32 NextHourLog_u32 = 0;

    // Get sample value
    if (0 != AD_Data_st.Filter_SampleCount_u32)
    {
        Sum_32 =
            (AD_Data_st.Filter_Sum_u32) / AD_Data_st.Filter_SampleCount_u32;
    }

    // Check for event
    if (TRUE == AD_EventBufferActiv_u16)
    {
        if ((0 == AD_EventBufferStopInSamples_u16)
            && (0 != AD_Data_st.LastValue_u32))
        {
            if (abs (AD_Data_st.LastValue_u32 - Sum_32) > 50)
            {
                AD_EventBufferStopInSamples_u16 = AD_EVENTBUFFER_SIZE / 2;
            }
        }
    }

    AD_Data_st.LastValue_u32 = Sum_32;

    // Get min/max
    if (Sum_32 > AD_Data_st.FilterMax_u16)
    {
        AD_Data_st.FilterMax_u16 = Sum_32;
    }

    if (Sum_32 < AD_Data_st.FilterMin_u16)
    {
        AD_Data_st.FilterMin_u16 = Sum_32;
    }


    // Save start time
    time ((time_t *) & AD_Data_st.LastStartSec_u32);
    time_ms (&AD_Data_st.LastStart_Msec_u32);


    // Time changed ? todo: make better
    if (100000 < NextMinuteLog_u32 - AD_Data_st.LastStartSec_u32)
    {
        NextMinuteLog_u32 =
            AD_Data_st.LastStartSec_u32 - AD_Data_st.LastStartSec_u32 % 60 +
            60;
        NextHourLog_u32 =
            AD_Data_st.LastStartSec_u32 - AD_Data_st.LastStartSec_u32 % 3600 +
            3600;
    }

    // Log minute data
    if (NextMinuteLog_u32 <= AD_Data_st.LastStartSec_u32)
    {
        NextMinuteLog_u32 = AD_Data_st.LastStartSec_u32 + 60;

        AD_LogMinuteData_v ();

        // Reset min max data
        AD_Data_st.FilterMax_u16 = Sum_32;
        AD_Data_st.FilterMin_u16 = Sum_32;
    }

    if (0 == NextHourLog_u32)
    {
        NextHourLog_u32 =
            AD_Data_st.LastStartSec_u32 - AD_Data_st.LastStartSec_u32 % 3600 +
            3600;
    }

    if (NextHourLog_u32 <= AD_Data_st.LastStartSec_u32)
    {
        AD_LogHourData_v ();

        NextHourLog_u32 = AD_Data_st.LastStartSec_u32 + 3600;
    }

    // Reset data

    AD_Data_st.Filter_Sum_u32 = 0;
    AD_Data_st.Filter_SampleCount_u32 = 0;


    return (Sum_32);
}

/*******************************************************************************

  AD_StoreData_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


void AD_StoreData_v (u16 Data_u16)
{
#ifdef TIME_MEASURING_ENABLE
    TIME_MEASURING_Start (TIME_MEASURING_TIME_3);
#endif
    AD_Data_st.Filter_Sum_u32 += Data_u16 * 8;  // iir(Data_u16*8);
    AD_Data_st.Filter_SampleCount_u32++;

    // Save Eventbuffer
    if (TRUE == AD_EventBufferActiv_u16)
    {
        AD_EventBuffer_u16[AD_EventBufferPointer_u16] = Data_u16;
        AD_EventBufferPointer_u16++;

        if (AD_EVENTBUFFER_SIZE <= AD_EventBufferPointer_u16)
        {
            AD_EventBufferPointer_u16 = 0;
        }

        if (0 != AD_EventBufferStopInSamples_u16)
        {
            AD_EventBufferStopInSamples_u16--;
            if (0 == AD_EventBufferStopInSamples_u16)
            {
                AD_EventBufferActiv_u16 = FALSE;
            }
        }
    }


    if (100 == AD_Data_st.Filter_SampleCount_u32)
    {
#ifdef TIME_MEASURING_ENABLE
        TIME_MEASURING_Start (TIME_MEASURING_TIME_4);
#endif
        AD_GetData_u16 ();
#ifdef TIME_MEASURING_ENABLE
        TIME_MEASURING_Stop (TIME_MEASURING_TIME_4);
#endif
    }

#ifdef TIME_MEASURING_ENABLE
    TIME_MEASURING_Stop (TIME_MEASURING_TIME_3);
#endif
}

/*******************************************************************************

  AD_GetADData_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_GetADData_v (void)
{
    volatile avr32_adc_t *adc;
    u16 adc_value;

    adc = &AVR32_ADC;   // ADC IP registers address

    adc_value = adc_get_value (adc, EXAMPLE_ADC_TEST_CHANNEL);

    adc_start (adc);    // Restart

    // AD_StoreFilterData_v (adc_value);
    AD_StoreData_v (adc_value);
}

/*******************************************************************************

  AD_LogData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


/*******************************************************************************

  AD_

*******************************************************************************/

void AD_LogData (void)
{
    u16 i;
    u16 Value_u16;
    u32 n;

    // Value_u16 = AD_GetFilterData_u16 ();
    Value_u16 = AD_GetData_u16 ();

    n = Value_u16 * 80 / 1024;

    CI_LocalPrintf ("Min %4d ", AD_Filter_Min);
    CI_LocalPrintf ("Max %4d ", AD_Filter_Max);
    CI_LocalPrintf ("%4d ", Value_u16);

    for (i = 0; i < n; i++)
    {
        CI_LocalPrintf (" ");
    }
    CI_LocalPrintf ("*\r\n");
}

/*******************************************************************************

  AD_GetPrintLine100ms

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_GetPrintLine100ms (u8 * Text_pu8)
{
struct tm tm_st;
time_t now;
u32 now_ms;

    time_ms (&now_ms);
    time (&now);

    localtime_r (&now, &tm_st);


    sprintf ((char *) Text_pu8,
             "%2d.%02d.%02d %2d:%02d:%02d %3d ms - Min %4d Max %4d %4d  - %4d\r\n",
             tm_st.tm_mday, tm_st.tm_mon + 1, tm_st.tm_year - 100,
             tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec, now_ms,
             AD_Data_st.FilterMin_u16, AD_Data_st.FilterMax_u16,
             AD_Data_st.LastValue_u32,
             AD_Data_st.FilterMax_u16 - AD_Data_st.FilterMin_u16);
}

/*******************************************************************************

  AD_GetPrintLineMin

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_GetPrintLineMin (u8 * Text_pu8)
{
struct tm tm_st;
u32 i;

    // Get last minute entry
    i = AD_Data_st.PointerMinutesEnd_u8;
    if (0 == AD_Data_st.PointerMinutesEnd_u8)
    {
        i = AD_MINUTES_COUNT;
    }
    i--;

    localtime_r ((time_t *) & AD_Data_st.ValueMinutesTime_u32[i], &tm_st);

    sprintf ((char *) Text_pu8,
             "%2d.%02d.%02d %2d:%02d:%02d - Min %4d Max %4d - %4d\r\n",
             tm_st.tm_mday, tm_st.tm_mon + 1, tm_st.tm_year - 100,
             tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec,
             AD_Data_st.ValueMinMinutes_u16[i],
             AD_Data_st.ValueMaxMinutes_u16[i],
             AD_Data_st.ValueMaxMinutes_u16[i] -
             AD_Data_st.ValueMinMinutes_u16[i]);
}

/*******************************************************************************

  AD_GetPrintLineHour

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_GetPrintLineHour (u8 * Text_pu8)
{
struct tm tm_st;
u32 i;

    // Get last minute entry
    i = AD_Data_st.PointerHoursEnd_u8;
    if (AD_Data_st.PointerHoursStart_u8 != AD_Data_st.PointerHoursEnd_u8)
    {
        if (0 == AD_Data_st.PointerHoursEnd_u8)
        {
            i = AD_MINUTES_COUNT;
        }
        i--;
    }

    localtime_r ((time_t *) & AD_Data_st.ValueHoursTime_u16[i], &tm_st);

    sprintf ((char *) Text_pu8,
             "%2d.%02d.%02d %2d:%02d:%02d - Min %4d Max %4d - %4d\r\n",
             tm_st.tm_mday, tm_st.tm_mon + 1, tm_st.tm_year - 100,
             tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec,
             AD_Data_st.ValueMinHours_u16[i], AD_Data_st.ValueMaxHours_u16[i],
             AD_Data_st.ValueMaxHours_u16[i] -
             AD_Data_st.ValueMinHours_u16[i]);
}

/*******************************************************************************

  AD_GetPrintLineData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_GetPrintLineData (u8 * Text_pu8)
{
u16 Value_u16;
struct tm tm_st;
time_t now;
u32 now_ms;

    time_ms (&now_ms);
    time (&now);

    localtime_r (&now, &tm_st);

    Value_u16 = AD_Data_st.LastValue_u32;

    sprintf ((char *) Text_pu8,
             "%2d:%02d:%02d %3d ms - Min %4d Max %4d - %4d - %4d\r\n",
             tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec, now_ms,
             AD_Data_st.FilterMin_u16, AD_Data_st.FilterMax_u16, Value_u16,
             AD_Data_st.FilterMax_u16 - AD_Data_st.FilterMin_u16);
}

/*******************************************************************************

  AD_PrintADData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_PrintADData (void)
{
    u8 Time_u8[100];

    AD_GetPrintLine100ms (Time_u8);
    CI_LocalPrintf ("%s", Time_u8);

}

/*******************************************************************************

  AD_Readdata

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_Readdata (void)
{
    u8 Byte_u8;
    u32 n;
    u32 LoopCount_u32;
    u32 NextTickCount_u32 = 0;

    // AD_init ();

    LoopCount_u32 = 0;
    NextTickCount_u32 = xTickCount + 100;

    while (1)
    {
        DelayMs (10);

        LoopCount_u32++;
        if (NextTickCount_u32 <= xTickCount)
        {
            NextTickCount_u32 = xTickCount + 100;   // Overflow after 49 days
            AD_PrintADData ();
        }

        n = BUFFERED_SIO_ByteReceived ();   // Char receive
        if (0 != n)
        {
            BUFFERED_SIO_GetByte (&Byte_u8);
            break;
        }
    }
}

/*******************************************************************************

  AD_PrintSummary

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_PrintSummary (void)
{
    u32 i;
    u32 n;
    struct tm tm_st;

    CI_LocalPrintf ("Minutes log\r\n");

    n = 0;
    for (i = AD_Data_st.PointerMinutesStart_u8;
         i != AD_Data_st.PointerMinutesEnd_u8; i++)
    {
        if (AD_MINUTES_COUNT <= i)
        {
            i = 0;
        }

        localtime_r ((time_t *) & AD_Data_st.ValueMinutesTime_u32[i], &tm_st);
        CI_LocalPrintf ("%2d %2d: %02d.%02d.%02d %2d:%02d:%02d ", n, i,
                        tm_st.tm_mday, tm_st.tm_mon + 1, tm_st.tm_year - 100,
                        tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec);

        CI_LocalPrintf ("min %4d  max %4d - %4d\r\n",
                        AD_Data_st.ValueMinMinutes_u16[i],
                        AD_Data_st.ValueMaxMinutes_u16[i],
                        AD_Data_st.ValueMaxMinutes_u16[i] -
                        AD_Data_st.ValueMinMinutes_u16[i]);

        n++;
    }

    CI_LocalPrintf ("\r\nHours log\r\n");

    n = 0;
    for (i = AD_Data_st.PointerHoursStart_u8;
         i != AD_Data_st.PointerHoursEnd_u8; i++)
    {
        if (AD_HOURS_COUNT <= i)
        {
            i = 0;
        }

        localtime_r ((time_t *) & AD_Data_st.ValueHoursTime_u16[i], &tm_st);
        CI_LocalPrintf ("%2d: %02d.%02d.%02d %2d:%02d:%02d ", n,
                        tm_st.tm_mday, tm_st.tm_mon + 1, tm_st.tm_year - 100,
                        tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec);
        CI_LocalPrintf ("min %4d  max %4d - %4d\r\n",
                        AD_Data_st.ValueMinHours_u16[i],
                        AD_Data_st.ValueMaxHours_u16[i],
                        AD_Data_st.ValueMaxHours_u16[i] -
                        AD_Data_st.ValueMinHours_u16[i]);
        // Todo: find bug
        n++;
        if (AD_HOURS_COUNT <= n)
        {
            break;
        }

    }


}

/*******************************************************************************

  AD_disable

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_disable (void)
{
    volatile avr32_adc_t *adc = &AVR32_ADC; // ADC IP registers address

    adc_disable (adc, EXAMPLE_ADC_TEST_CHANNEL);
}

/*******************************************************************************

  AD_LOG_GetPathname

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define AD_FILEPATH_TXT_MINUTE     "AD_MINUT"
#define AD_FILEPATH_TXT_HOUR       "AD_HOUR"
#define AD_FILEPATH_TXT_DAY        "AD_DAY"
#define AD_FILEPATH_TXT_EVENT      "AD_EVENT"

#define AD_FILEPATH_MINUTE     0
#define AD_FILEPATH_HOUR       1
#define AD_FILEPATH_DAY        2
#define AD_FILEPATH_EVENT      3
#define AD_FILEPATH_DATA       5


char *AD_LOG_GetPathname (u8 Filetype_u8)
{
    /*
     * switch (Filetype_u8) { case AD_FILEPATH_MINUTE : return
     * (AD_FILEPATH_TXT_MINUTE);
     * 
     * case AD_FILEPATH_HOUR : return (AD_FILEPATH_TXT_HOUR);
     * 
     * case AD_FILEPATH_DAY : return (AD_FILEPATH_TXT_DAY);
     * 
     * case AD_FILEPATH_EVENT : return (AD_FILEPATH_TXT_EVENT);
     * 
     * } 
     */
    return (AD_FILEIO_WORKING_DIR);
}

/*******************************************************************************

  AD_LOG_GetFilename

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 *AD_LOG_GetFilename (u8 Filetype_u8)
{
static u8 Time_u8[6 + 8 + 1 + 4 + 4 + 1];
struct tm tm_st;
time_t now;

    time (&now);
    localtime_r (&now, &tm_st);

    switch (Filetype_u8)
    {
        case AD_FILEPATH_MINUTE:
            sprintf ((char *) Time_u8, "MIN_%04d%02d%02d_%02d.txt",
                     1900 + tm_st.tm_year, tm_st.tm_mon + 1, tm_st.tm_mday,
                     tm_st.tm_hour);
            break;

        case AD_FILEPATH_HOUR:
            sprintf ((char *) Time_u8, "HOUR_%04d%02d%02d.txt",
                     1900 + tm_st.tm_year, tm_st.tm_mon + 1, tm_st.tm_mday);
            break;

        case AD_FILEPATH_DAY:
            sprintf ((char *) Time_u8, "DAY_%04d%02d%02d.txt",
                     1900 + tm_st.tm_year, tm_st.tm_mon + 1, tm_st.tm_mday);
            break;

        case AD_FILEPATH_EVENT:
            sprintf ((char *) Time_u8, "EVENT_%04d%02d%02d.txt",
                     1900 + tm_st.tm_year, tm_st.tm_mon + 1, tm_st.tm_mday);
            break;

        case AD_FILEPATH_DATA:
            sprintf ((char *) Time_u8, "DATA_%04d%02d%02d_%02d.txt",
                     1900 + tm_st.tm_year, tm_st.tm_mon + 1, tm_st.tm_mday,
                     tm_st.tm_hour);
            break;

    }

    return (Time_u8);
}

/*******************************************************************************

  AD_LOG_Time

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

struct tm *AD_LOG_Time (void)
{
    static struct tm tm_st;
    time_t now;

    time (&now);
    localtime_r (&now, &tm_st);

    return (&tm_st);
}

/*******************************************************************************

  AD_FileIO_CdDir_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 AD_FileIO_CdDir_u8 (u8 * Dir_pu8)
{
u8 Path_u8[20];

    // cd dir
    CI_LocalPrintf ("\r\ncd to %s\r\n", Dir_pu8);
    if (FALSE == FAI_cd ((u8 *) Dir_pu8))
    {
        // Make new dir if not present
        CI_LocalPrintf ("\r\nmake dir %s\r\n", Dir_pu8);
        FAI_mkdir ((u8 *) Dir_pu8);

        CI_LocalPrintf ("\r\ncd to %s\r\n", Dir_pu8);
        if (FALSE == FAI_cd ((u8 *) Dir_pu8))
        {
            return (FALSE);
        }
    }
    return (TRUE);
}

/*******************************************************************************

  AD_FileIO_CD_WorkingDir

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 AD_FileIO_CD_WorkingDir (void)
{
static u32 LastDate_u32 = 0;
u8 Path_u8[10];
time_t Now_u32;
struct tm tm_st;
struct tm tm_st_old;

    time (&Now_u32);

    localtime_r ((time_t *) & Now_u32, &tm_st);
    localtime_r ((time_t *) & LastDate_u32, &tm_st_old);

    if ((tm_st_old.tm_year == tm_st.tm_year) &&
        (tm_st_old.tm_mon == tm_st.tm_mon) &&
        (tm_st_old.tm_mday == tm_st.tm_mday))
    {
        return (TRUE);
    }

    LastDate_u32 = Now_u32;

    sprintf ((char *) Path_u8, "%2d%02d%02d", tm_st.tm_year - 100,
             tm_st.tm_mon + 1, tm_st.tm_mday);

    // cd dir
    CI_LocalPrintf ("New working dir %s\r\n", Path_u8);

    if (FALSE == FAI_cd ((u8 *) ".."))
    {
        CI_LocalPrintf ("Can't change to ..\r\n");
        return (FALSE);
    }

    if (FALSE == FAI_cd ((u8 *) Path_u8))
    {
        // Make new dir if not present
        CI_LocalPrintf ("\r\nmake dir %s\r\n", Path_u8);
        FAI_mkdir ((u8 *) Path_u8);

        CI_LocalPrintf ("\r\ncd to %s\r\n", Path_u8);
        if (FALSE == FAI_cd ((u8 *) Path_u8))
        {
            return (FALSE);
        }
    }
    return (TRUE);
}

/*******************************************************************************

  AD_FileIO_Init_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 AD_FileIO_Init_u8 (void)
{
u8 Path_u8[20];

    b_fsaccess_init ();

    // Move path in RAM , necessary for cd
    strcpy ((char *) Path_u8, AD_FILEIO_WORKING_DIR);

    // Mount SD drive
    CI_LocalPrintf ("\r\nMount SD Card\r\n");

    if (FALSE == FAI_mount (AD_FILEIO_LUN))
    {
        return (FALSE);
    }

    // Show free space
    CI_LocalPrintf ("\r\n\r\n");
    FAI_free_space (AD_FILEIO_LUN);

    // Show dirs
    CI_LocalPrintf ("\r\nOld dir\r\n");
    FAI_Dir (0);

    // Change to master dir
    AD_FileIO_CdDir_u8 (Path_u8);


    // Change to day dir
    AD_FileIO_CD_WorkingDir ();

    // Show new dir
    /*
     * CI_LocalPrintf ("\r\nNew dir\r\n"); FAI_Dir (0); 
     */
    return (TRUE);
}

/*******************************************************************************

  AD_FileIO_LogEvent_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 AD_FileIO_LogEvent_u8 (u32 time_u32)
{
s32 FileID_s32;
s32 Ret_s32;
u16 Pointer_u16;
u32 i;
u8 Text_u8[50];
struct tm tm_st;

    localtime_r ((time_t *) & time_u32, &tm_st);
    sprintf ((char *) Text_u8, "E_%2d%02d%02d.txt", tm_st.tm_hour,
             tm_st.tm_min, tm_st.tm_sec);

    FileID_s32 = open ((char *) Text_u8, O_CREAT | O_APPEND);
    if (0 > FileID_s32)
    {
        CI_LocalPrintf ("Error open %s - %d - %s\r\n", Text_u8, fs_g_status,
                        IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    localtime_r ((time_t *) & time_u32, &tm_st);

    sprintf ((char *) Text_u8, "Event at %02d.%02d.%02d %2d:%02d:%02d\n",
             tm_st.tm_mday, tm_st.tm_mon + 1, tm_st.tm_year - 100,
             tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec);
    Ret_s32 = write (FileID_s32, Text_u8, strlen ((char *) Text_u8));

    Pointer_u16 = AD_EventBufferPointer_u16;
    for (i = 0; i < AD_EVENTBUFFER_SIZE; i++)
    {
        sprintf ((char *) Text_u8, "%4d;%5d\n", i,
                 AD_EventBuffer_u16[Pointer_u16]);
        Ret_s32 = write (FileID_s32, Text_u8, strlen ((char *) Text_u8));

        Pointer_u16++;

        if (AD_EVENTBUFFER_SIZE <= Pointer_u16)
        {
            Pointer_u16 = 0;
        }
    }

    Ret_s32 = close (FileID_s32);
    if (-1 == Ret_s32)
    {
        CI_LocalPrintf ("Close  = %d \r\n", Ret_s32);
    }

    return (TRUE);
}

/*******************************************************************************

  AD_FileIO_AppendText_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 AD_FileIO_AppendText_u8 (u8 * Filename_pu8, u8 * Text_pu8)
{
s32 FileID_s32;
s32 Ret_s32;

    FileID_s32 = open ((char *) Filename_pu8, O_CREAT | O_APPEND);
    if (0 > FileID_s32)
    {
        CI_LocalPrintf ("Error open %s - %d - %s\r\n", Filename_pu8,
                        fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    Ret_s32 = write (FileID_s32, Text_pu8, strlen ((char *) Text_pu8));


    Ret_s32 = close (FileID_s32);
    if (-1 == Ret_s32)
    {
        CI_LocalPrintf ("Close  = %d \r\n", Ret_s32);
    }

    return (TRUE);
}

/*******************************************************************************

  AD_FileIO_SaveLocalTime_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define AD_FILE_LOCALTIME     "time.txt"

u8 AD_FileIO_SaveLocalTime_u8 (u32 LocalTime_u32)
{
s32 FileID_s32;
s32 Ret_s32;
u8 Text_u8[50];
struct tm tm_st;

    FileID_s32 = open ((char *) AD_FILE_LOCALTIME, O_CREAT | O_WRONLY);
    if (0 > FileID_s32)
    {
        CI_LocalPrintf
            ("AD_FileIO_SaveLocalTime_u8: Error open %s - %d - %s\r\n",
             AD_FILE_LOCALTIME, fs_g_status,
             IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    // sprintf ((char*)Text_u8,"%12ld - ",(unsigned long)LocalTime_u32);


    localtime_r ((time_t *) & LocalTime_u32, &tm_st);
    sprintf ((char *) Text_u8, "%12ld - %02d.%02d.%02d %2d:%02d:%02d ",
             (unsigned long) LocalTime_u32, tm_st.tm_mday, tm_st.tm_mon + 1,
             tm_st.tm_year - 100, tm_st.tm_hour, tm_st.tm_min, tm_st.tm_sec);


    Ret_s32 = write (FileID_s32, Text_u8, strlen ((char *) Text_u8));
    if (-1 == Ret_s32)
    {
        CI_LocalPrintf
            ("AD_FileIO_SaveLocalTime_u8: write FAIL - %d - %s\r\n",
             fs_g_status, IBN_FileSystemErrorText (fs_g_status));
    }
    /*
     * if (TRUE != fat_cache_flush ()) { CI_LocalPrintf
     * ("AD_FileIO_SaveLocalTime_u8: fat_cache_flush FAIL - %d -
     * %s\r\n",fs_g_status, IBN_FileSystemErrorText(fs_g_status)); } 
     */
    Ret_s32 = close (FileID_s32);
    if (-1 == Ret_s32)
    {
        CI_LocalPrintf ("AD_FileIO_SaveLocalTime_u8: Close  = %d \r\n",
                        Ret_s32);
    }

    return (TRUE);
}

/*******************************************************************************

  AD_FileIO_ReadLocalTime_u32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 AD_FileIO_ReadLocalTime_u32 (void)
{
s32 FileID_s32;
s32 Ret_s32;
u32 LocalTime_u32 = 0;
u8 Text_u8[15];

    FileID_s32 = open ((char *) AD_FILE_LOCALTIME, O_RDONLY);
    if (0 > FileID_s32)
    {
        CI_LocalPrintf ("Error open %s - %d - %s\r\n", AD_FILE_LOCALTIME,
                        fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    Ret_s32 = read (FileID_s32, Text_u8, 14);
    if (-1 != Ret_s32)
    {
        LocalTime_u32 = atoi ((char *) Text_u8);
    }
    else
    {
        CI_LocalPrintf ("read fail  = %d \r\n", Ret_s32);
    }


    Ret_s32 = close (FileID_s32);
    if (-1 == Ret_s32)
    {
        CI_LocalPrintf ("Close  = %d \r\n", Ret_s32);
    }

    return (LocalTime_u32);
}

/*******************************************************************************

  AD_FileIO_AppendData_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

/*
 * u8 AD_FileIO_AppendData_u8 (u8 *Filename_pu8,u32 Data_u32) { s32
 * FileID_s32; s32 Ret_s32; u8 Text_u8[20];
 * 
 * // FileID_s32 = open ("test.txt",O_CREAT|O_APPEND); FileID_s32 = open
 * ((char*)Filename_pu8,O_CREAT|O_APPEND); if (0 > FileID_s32) {
 * CI_LocalPrintf ("Error open %s - %d - %s\r\n",Filename_pu8,fs_g_status,
 * IBN_FileSystemErrorText(fs_g_status)); return (FALSE); }
 * 
 * // CI_LocalPrintf ("%2d.%02d.%02d %2d:%02d:%02d %3d ms -
 * ",tm_st.tm_mday,tm_st.tm_mon+1,tm_st.tm_year-100,tm_st.tm_hour,tm_st.tm_min,tm_st.tm_sec,now_ms);
 * 
 * sprintf ((char*)Text_u8,"Tick %d %d\r\n",(int)xTickCount,Data_u32);
 * Ret_s32 = write (FileID_s32,Text_u8,strlen ((char*)Text_u8));
 * 
 * // CI_LocalPrintf ("Write = %d \r\n",Ret_s32);
 * 
 * if (TRUE != fat_cache_flush ()) { CI_LocalPrintf ("fat_cache_flush FAIL -
 * %d - %s\r\n",fs_g_status, IBN_FileSystemErrorText(fs_g_status)); }
 * 
 * Ret_s32 = close (FileID_s32); if (-1 == Ret_s32) { CI_LocalPrintf ("Close
 * = %d \r\n",Ret_s32); }
 * 
 * return (TRUE); }
 * 
 */

/*******************************************************************************

  AD_ApplicationTickHook

  ISR ?

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_ApplicationTickHook (void)
{
    static u8 Init_u8 = FALSE;

    if (FALSE == Init_u8)
    {
    volatile avr32_adc_t *adc;

        adc = &AVR32_ADC;   // ADC IP registers address
        AD_init ();
        adc_start (adc);    // Restart
        Init_u8 = TRUE;
    }
    else
    {
        AD_GetADData_v ();
    }
}

/*******************************************************************************

  AD_LOG_Start

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_LOG_Start (void)
{
    AD_FileLogFlag_u8 = TRUE;
}

/*******************************************************************************

  AD_LOG_Stop

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_LOG_Stop (void)
{
    AD_FileLogFlag_u8 = FALSE;
}

/*******************************************************************************

  AD_LOG_WriteDataToFile

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_LOG_WriteDataToFile (void)
{
    static u32 LastMinuteLogTime_u32 = 0;
    static u32 LastHourLogTime_u32 = 0;
    u8 Text_u8[80];
    time_t Now_u32;

#ifdef TIME_MEASURING_ENABLE
    TIME_MEASURING_Start (TIME_MEASURING_TIME_5);
#endif

    AD_FileIO_CD_WorkingDir ();

    AD_GetPrintLine100ms (Text_u8); // 100 ms Data
    AD_FileIO_AppendText_u8 (AD_LOG_GetFilename (AD_FILEPATH_DATA), Text_u8);

    // Add local tick
    LastMinuteLogTime_u32++;

    // Store time
    if (0 == LastMinuteLogTime_u32 % 10)    // Every second
    {
        AD_FileIO_SaveLocalTime_u8 (AD_Data_st.LastStartSec_u32);
    }

    // Toggle LED
    if (0 == LastMinuteLogTime_u32 % 2)
    {
        ToolPinSet ();
    }
    else
    {
        ToolPinClr ();
    }

    if (FALSE == AD_EventBufferActiv_u16)
    {
        time (&Now_u32);
        AD_FileIO_LogEvent_u8 (Now_u32);
        AD_EventBufferActiv_u16 = TRUE;
    }



    // Log minute data
    if (600 == LastMinuteLogTime_u32)
    {
        AD_GetPrintLineMin (Text_u8);
        AD_FileIO_AppendText_u8 (AD_LOG_GetFilename (AD_FILEPATH_MINUTE),
                                 Text_u8);
        LastMinuteLogTime_u32 = 0;
    }

    // Log hour data
    LastHourLogTime_u32++;
    if (36000 == LastHourLogTime_u32)
    {
        AD_GetPrintLineHour (Text_u8);
        AD_FileIO_AppendText_u8 (AD_LOG_GetFilename (AD_FILEPATH_HOUR),
                                 Text_u8);
        LastHourLogTime_u32 = 0;
    }

#ifdef TIME_MEASURING_ENABLE
    TIME_MEASURING_Stop (TIME_MEASURING_TIME_5);
#endif
}

/*******************************************************************************

  AD_LOG_Data

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

typedef struct
{
    u32 Time_u32;
    u16 TimeMs_u16;
    u16 Value_u16;
} Log100msData_tst;

typedef struct
{
    u32 Time_u32;
    u16 Value_u16;
    u16 ValueMin_u16;
    u16 ValueMax_u16;
} LogSecData_tst;

typedef struct
{
    u32 Time_u32;
    u16 Value_u16;
    u16 ValueMin_u16;
    u16 ValueMax_u16;
} LogMinData_tst;

typedef struct
{
    u32 Time_u32;
    u16 Value_u16;
    u16 ValueMin_u16;
    u16 ValueMax_u16;
} LogHourData_tst;

// Log every 5 sec

#define AD_LOG_100MS_SIZE     100
#define AD_LOG_SEC_SIZE        10
#define AD_LOG_MIN_SIZE         2
#define AD_LOG_HOUR_SIZE        2

typedef struct
{
    u16 Log100msStart_u16;
    u16 Log100msEnd_u16;
    Log100msData_tst Log100msData_st[AD_LOG_100MS_SIZE];
    u16 LogSecStart_u16;
    u16 LogSecEnd_u16;
    LogSecData_tst LogSecData_st[AD_LOG_SEC_SIZE];
    u16 LogMinStart_u16;
    u16 LogMinEnd_u16;
    LogSecData_tst LogMinData_st[AD_LOG_MIN_SIZE];
    u16 LogHourStart_u16;
    u16 LogHourEnd_u16;
    LogSecData_tst LogHourData_st[AD_LOG_HOUR_SIZE];
} LogData_tst;


void AD_LOG_Data (void)
{
    static u32 LastMinuteLogTime_u32 = 0;
    static u32 LastHourLogTime_u32 = 0;
    u8 Text_u8[80];

#ifdef TIME_MEASURING_ENABLE
    TIME_MEASURING_Start (TIME_MEASURING_TIME_6);
#endif

    AD_GetPrintLine100ms (Text_u8); // 100 ms Data
    AD_FileIO_AppendText_u8 (AD_LOG_GetFilename (AD_FILEPATH_DATA), Text_u8);

    // Add local tick
    LastMinuteLogTime_u32++;

    // Store time
    if (0 == LastMinuteLogTime_u32 % 10)    // Every second
    {
        AD_FileIO_SaveLocalTime_u8 (AD_Data_st.LastStartSec_u32);
    }

    // Log minute data
    if (600 == LastMinuteLogTime_u32)
    {
        AD_GetPrintLineMin (Text_u8);
        AD_FileIO_AppendText_u8 (AD_LOG_GetFilename (AD_FILEPATH_MINUTE),
                                 Text_u8);
        LastMinuteLogTime_u32 = 0;
    }

    // Log hour data
    LastHourLogTime_u32++;
    if (36000 == LastHourLogTime_u32)
    {
        AD_GetPrintLineHour (Text_u8);
        AD_FileIO_AppendText_u8 (AD_LOG_GetFilename (AD_FILEPATH_HOUR),
                                 Text_u8);
        LastHourLogTime_u32 = 0;
    }

#ifdef TIME_MEASURING_ENABLE
    TIME_MEASURING_Stop (TIME_MEASURING_TIME_6);
#endif
}

/*******************************************************************************

  AD_LOG_task

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_LOG_task (void *pvParameters)
{
    // static u32 LoopCount_u32 = 0;
    portTickType xLastWakeTime;
    u32 NextTickCount_u32 = 0;

    /*
     * AD_init (); AD_FileIO_Init_u8 (); 
     */
    NextTickCount_u32 = xTickCount + 100;

    xLastWakeTime = xTaskGetTickCount ();

    while (1)
    {
        vTaskDelayUntil (&xLastWakeTime, 100);  // configTSK_AD_LOG_PERIOD);
        if (TRUE == AD_FileLogFlag_u8)
        {
            AD_LOG_WriteDataToFile ();

            // Log minute data

            // Log hour data
        }
    }


}

/*******************************************************************************

  AD_LOG_init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void AD_LOG_init (void)
{
    static u8 TastStarte_u8 = 0;

    // Check for calling twice
    if (0 != TastStarte_u8)
    {
        return;
    }

    if (FALSE == AD_CheckTimeSet ())
    {
        CI_LocalPrintf ("AD_LOG_init: Can't start - time not ok\r\n");
        return;
    }

    AD_init ();
    AD_FileIO_Init_u8 ();

    TastStarte_u8 = 1;

    xTaskCreate (AD_LOG_task,
                 configTSK_AD_LOG_NAME,
                 configTSK_AD_LOG_STACK_SIZE,
                 NULL, configTSK_AD_LOG_PRIORITY, NULL);

}

#endif
