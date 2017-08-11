/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 09.08.2017
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


#include "compiler.h"
#include "preprocessor.h"
#include "board.h"
#include "gpio.h"
#include "ctrl_access.h"
#include "flashc.h"
#include "string.h"

#ifdef FREERTOS_USED
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "conf_usb.h"
#include "usb_drv.h"

#include "global.h"
#include "tools.h"

#include "DebugLog.h"


#ifdef DEBUG_LOG_ENABLE



#define ENABLE_IBN_DEBUG_LOG_TEST


extern volatile portTickType xTickCount;



#define DL_LOG__ENTRYS      DL_LOG__MAX_ENTRY

#define DL_LOG__SEQUENCE_LOG_SIZE     100


unsigned char *DL_LogNames[] =
{
    (unsigned char *)"NO_ENTRY",
    (unsigned char *)"INVALID_ENTRY",
    (unsigned char *)"REQ_GET_DESCRIPTOR   ",
    (unsigned char *)"REQ_GET_CONFIGURATION",
    (unsigned char *)"REQ_SET_ADDRESS      ",
    (unsigned char *)"REQ_SET_CONFIGURATION",
    (unsigned char *)"REQ_CLEAR_FEATURE    ",
    (unsigned char *)"REQ_SET_FEATURE      ",
    (unsigned char *)"REQ_GET_STATUS       ",
    (unsigned char *)"REQ_GET_INTERFACE    ",
    (unsigned char *)"REQ_SET_INTERFACE    ",
    (unsigned char *)"REQ_SET_DESCRIPTOR   ",
    (unsigned char *)"REQ_SYNCH_FRAME      ",
    (unsigned char *)"REQ_UNSUPPORTED      ",
    (unsigned char *)"REQ_UNKNOWN          ",
    (unsigned char *)"Text 3",
};

typedef struct {
  unsigned int  Event;
  unsigned int  Tick;
}type_DL_SequenceLog;


typedef struct {
  unsigned int  Count;
  unsigned int  ErrorCount;
  unsigned int  LastCallTick;
  unsigned int  LastErrorTick;
}type_DL_LogDataEntry;


type_DL_LogDataEntry tDL_LogData[DL_LOG__ENTRYS];
type_DL_LogDataEntry tDL_LogSavedData[DL_LOG__ENTRYS];


type_DL_SequenceLog  tDL_SequenceLog[DL_LOG__SEQUENCE_LOG_SIZE];
unsigned char tDL_SequenceLogPos = 0;
unsigned char tDL_SequenceLogStart = 0;


void DL_Init (void)
{
  unsigned char i;

// Init Event Log
  for (i=0;i<DL_LOG__ENTRYS;i++)
  {
    tDL_LogData[i].Count         = 0;
    tDL_LogData[i].ErrorCount    = 0;
    tDL_LogData[i].LastCallTick  = 0;
    tDL_LogData[i].LastErrorTick = 0;
  }
  DL_SaveLog ();

// Init Sequence Log
  tDL_SequenceLogPos = 0;
  tDL_SequenceLogStart = 0;
  for (i=0;i<DL_LOG__SEQUENCE_LOG_SIZE;i++)
  {
    tDL_SequenceLog[i].Event = DL_LOG__ERROR__NO_ENTRY;
    tDL_SequenceLog[i].Tick  = 0;
  }
}

void DL_LogEvent (unsigned char Event)
{
  if (DL_LOG__ENTRYS <= Event)
  {
    return;
  }
  tDL_LogData[Event].Count++;
  tDL_LogData[Event].LastCallTick = xTickCount;
  DL_AddSequenceLog (tDL_LogData[Event].LastCallTick,Event);
}

void DL_LogEventError (unsigned char Event)
{
  if (DL_LOG__ENTRYS <= Event)
  {
    return;
  }
  tDL_LogData[Event].ErrorCount++;
  tDL_LogData[Event].LastErrorTick = xTickCount;
  DL_AddSequenceLog (tDL_LogData[Event].LastErrorTick,Event);
}


void DL_SaveLog (void)
{
  memcpy (tDL_LogSavedData,tDL_LogData,sizeof (tDL_LogData[DL_LOG__ENTRYS]));
}


void DL_AddSequenceLog (unsigned int Tick,unsigned char Event)
{
  tDL_SequenceLog[tDL_SequenceLogPos].Event = Event;
  tDL_SequenceLog[tDL_SequenceLogPos].Tick  = Tick;

// Set next  entry
  tDL_SequenceLogPos++;
  if (DL_LOG__SEQUENCE_LOG_SIZE <= tDL_SequenceLogPos)
  {
    tDL_SequenceLogPos = 0;
  }

// Move start of Log
  if (tDL_SequenceLogPos == tDL_SequenceLogStart)
  {
    tDL_SequenceLogStart++;
  }
  if (DL_LOG__SEQUENCE_LOG_SIZE <= tDL_SequenceLogStart)
  {
    tDL_SequenceLogStart = 0;
  }
}

/*******************************************************************************

  DL_ShowSequenceLog

  Non ISR

  Reviews
  Date      Reviewer        Info


*******************************************************************************/


void DL_ShowSequenceLog (void)
{
  int nLogStart;

  while (1)
  {
    nLogStart = tDL_SequenceLogStart + 1;
    // Check for overflow
    if (DL_LOG__SEQUENCE_LOG_SIZE <= nLogStart)
    {
      nLogStart = 0;
    }

    // End of while
    if (nLogStart == tDL_SequenceLogPos)
    {
      break;
    }

    if (DL_LOG__MAX_ENTRY < tDL_SequenceLog[tDL_SequenceLogPos].Event)
    {
      tDL_SequenceLog[tDL_SequenceLogPos].Event = DL_LOG__ERROR__INVALID_ENTRY;
    }

    // Print entry
    CI_LocalPrintf ("-%07d %02x - %s\n",tDL_SequenceLog[tDL_SequenceLogStart].Tick,
                                        tDL_SequenceLog[tDL_SequenceLogStart].Event,
                                        DL_LogNames[tDL_SequenceLog[tDL_SequenceLogStart].Event]);
    tDL_SequenceLogStart++;
    if (DL_LOG__SEQUENCE_LOG_SIZE <= tDL_SequenceLogStart)
    {
      tDL_SequenceLogStart = 0;
    }
  }
}


#endif // DEBUG_LOG_ENABLE

#ifdef ENABLE_IBN_DEBUG_LOG_TEST

void IBN_DL_Test (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8)
{

    if (0 == nParamsGet_u8)
    {
        CI_LocalPrintf ("Debug log test functions\r\n");
        CI_LocalPrintf ("\r\n");
        CI_LocalPrintf ("0 [slot] Init test slot [slot]\r\n");
        CI_LocalPrintf ("\r\n");
        return;
    }

    switch (CMD_u8)
    {
        case 0:
            DL_AddSequenceLog (xTickCount / 2,Param_u32);
            break;
    }
}
#else
void IBN_DL_Test (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8)
{
}
#endif



