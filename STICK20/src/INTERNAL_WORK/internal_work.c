/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 10.04.2012
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
 * internal_work.c
 *
 *  Created on: 10.04.2012
 *      Author: RB
 */



#include "board.h"

#ifdef FREERTOS_USED
  #include "FreeRTOS.h"
  #include "task.h"
#endif
#include "string.h"

/* FAT includes */
#include "file.h"
#include "navigation.h"
#include "ctrl_access.h"
#include "fsaccess.h"


#include "time.h"
#include "global.h"
#include "tools.h"
#include "Interpreter.h"
#include "Inbetriebnahme.h"
#include "TIME_MEASURING.h"

#include "USER_INTERFACE/html_io.h"

#include "CCID/USART/ISO7816_USART.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"

#include "OTP\report_protocol.h"
#include "internal_work.h"



#ifndef STICK_20_A_MUSTER_PROD
  #define INTERPRETER_ENABLE     // Disable for PROD Version
#endif

#ifdef STICK_20_SEND_DEBUGINFOS_VIA_HID
  #define INTERPRETER_ENABLE     // Enable also for PROD Version
#endif


/*******************************************************************************

 Local defines

*******************************************************************************/

#define DEBUG_IW_IO

#ifdef DEBUG_IW_IO
  int CI_LocalPrintf (char *szFormat,...);
  int CI_TickLocalPrintf (char *szFormat,...);
#else
  #define CI_LocalPrintf(...)
  #define CI_TickLocalPrintf(...)
#endif

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

  IW_SendToSC_PW1

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 IW_SendToSC_PW1 (u8 *PW_pu8)
{
  if (0 == strlen ((const char*)PW_pu8))
  {
    return (FALSE);
  }
  return (LA_SC_SendVerify (1,PW_pu8));
}

/*******************************************************************************

  IW_SendToSC_PW3

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 IW_SendToSC_PW3 (u8 *PW_pu8)
{
  if (0 == strlen ((const char*)PW_pu8))
  {
    return (FALSE);
  }
  return (LA_SC_SendVerify (3,PW_pu8));
}



/*******************************************************************************

  IW_task

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IW_task(void *pvParameters)
{
  u32 LoopCount_u32 = 0;
  portTickType xLastWakeTime;

// Init Ramdisk io
#ifdef HTML_ENABLE_HTML_INTERFACE
  HTML_FileIO_Init_u8 ();
#endif

  xLastWakeTime = xTaskGetTickCount();

#ifdef INTERPRETER_ENABLE
//  IDF_PrintStartupInfo ();
#endif

  while (TRUE)
  {
#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Start (TIME_MEASURING_TIMER_KEY_10MS);
#endif

// Check the OTP HID io
    OTP_main ();

#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Stop (TIME_MEASURING_TIMER_KEY_10MS);
#endif

    vTaskDelayUntil(&xLastWakeTime, configTSK_IW_TEST_PERIOD);

#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Start (TIME_MEASURING_TIMER_IW_10MS);
#endif

     LoopCount_u32++;
     if(0 == LoopCount_u32 % 100)
     {
#ifdef HTML_ENABLE_HTML_INTERFACE
       HTML_CheckRamDisk ();
#endif
       HID_ExcuteCmd ();
     }




#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Stop (TIME_MEASURING_TIMER_IW_10MS);
#endif
  }
}


/*******************************************************************************

  IW_task_init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IW_task_init(void)
{
  xTaskCreate(IW_task,
          configTSK_IW_TEST_NAME,
          configTSK_IW_TEST_STACK_SIZE,
          NULL,
          configTSK_IW_TEST_PRIORITY,
          NULL);
}


