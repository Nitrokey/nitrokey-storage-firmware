/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 24.11.2010
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
 * USB_CCID.c
 *
 *  Created on: 24.11.2010
 *      Author: RB
 */

#ifdef FREERTOS_USED
  #include "FreeRTOS.h"
  #include "task.h"
#endif

#include <avr32/io.h>
#include "stdio.h"
#include "string.h"
#include "compiler.h"
#include "board.h"
#include "power_clocks_lib.h"


#include "global.h"

#include "tools.h"
#include "usart.h"
#include "CCID\\USART\\ISO7816_USART.h"
#include "CCID\\USART\\ISO7816_ADPU.h"
#include "CCID\\USART\\ISO7816_Prot_T1.h"
#include "USB_CCID.h"
#include "USB_CCID_task.h"
#include "CCID/Local_ACCESS/Debug_T1.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

#ifdef INTERPRETER_ENABLE
  #define DEBUG_USB_CCID_IO
  //#define DEBUG_USB_CCID_IO_DETAIL
  //#define DEBUG_USB_CCID_LOCK

  #ifdef DEBUG_USB_CCID_IO_DETAIL
    #define DEBUG_USB_CCID_IO
  #endif
#endif

#ifdef DEBUG_USB_CCID_IO
  int CI_LocalPrintf (char *szFormat,...);
  int CI_TickLocalPrintf (char *szFormat,...);
  int CI_StringOut (char *szText);
#else
  #define CI_LocalPrintf(...)
  #define CI_TickLocalPrintf(...)
  #define CI_StringOut(...)
#endif

//#define DEBUG_LOG_CCID_DETAIL


/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/
extern  volatile portTickType xTickCount            ;

/*******************************************************************************

 Local declarations

*******************************************************************************/

u32 USB_CCID_LockCounter_u32 = 0;       // 1 Tick = 10 ms
u32 USB_CCID_PowerOffDelay_u32 = 0;     // 1 Tick = 10 ms


t_USB_CCID_data_st g_USB_CCID_data_st;


static u8 CCID_SlotStatus_u8  = CCID_SLOT_STATUS_PRESENT_INACTIVE;    // Todo present check at startup
static u8 CCID_ClockStatus_u8 = CCID_SLOT_STATUS_CLOCK_UNKNOWN;

#define USB_CCID_LOCK_COUNT_NORMAL    50    // =   500 ms
#define USB_CCID_LOCK_COUNT_POWERON 1000    // = 10000 ms
#define USB_CCID_LOCK_COUNT_LONG    3000    // = 30000 ms
#define USB_CCID_LOCK_COUNT_CLEAR     10    // =   100 ms

#define USB_CCID_POWER_OFF_NORMAL   3000    // = 30000 ms


/*******************************************************************************

  USB_CCID_SetPowerOffDelayCounter

  Changes
  Date      Author          Info
  17.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void USB_CCID_SetPowerOffDelayCounter (u32 Value_u32)
{
#ifdef DEBUG_USB_CCID_LOCK
  {
    u8 Text[20];
    itoa (xTickCount/2,Text);   // in msec
    CI_StringOut (Text);
    CI_StringOut (" USB_CCID - Set power off counter = ");
    itoa (Value_u32*10,Text);
    CI_StringOut (Text);
    CI_StringOut (" msec\r\n");
  }
#endif

  USB_CCID_PowerOffDelay_u32 = Value_u32;
}

/*******************************************************************************

  USB_CCID_DecPowerOffDelayCounter

  Changes
  Date      Author          Info
  17.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void USB_CCID_DecPowerOffDelayCounter (void)
{
  if (0 != USB_CCID_PowerOffDelay_u32)
  {
    USB_CCID_PowerOffDelay_u32--;
#ifdef DEBUG_USB_CCID_LOCK
    if (0 == USB_CCID_PowerOffDelay_u32)
    {
      u8 Text[20];
      itoa (xTickCount/2,Text);
      CI_StringOut (Text);
      CI_StringOut (" USB_CCID - Poweroff counter = 0\r\n");
    }
#endif
  }
}

/*******************************************************************************

  USB_CCID_GetPowerOffDelayCounter

  Changes
  Date      Author          Info
  17.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 USB_CCID_GetPowerOffDelayCounter (void)
{
  return (USB_CCID_PowerOffDelay_u32);
}



/*******************************************************************************

  USB_CCID_LockCounter_u32

  Changes
  Date      Author          Info
  16.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void USB_CCID_SetLockCounter (u32 Value_u32)
{
  int LockActive_u32;

#ifdef DEBUG_USB_CCID_LOCK
  {
    u8 Text[20];
    itoa (xTickCount/2,Text);   // in msec
    CI_StringOut (Text);
    CI_StringOut (" USB_CCID - Set lock counter = ");
    itoa (Value_u32*10,Text);
    CI_StringOut (Text);
    CI_StringOut (" msec\r\n");
  }
#endif

  LockActive_u32 = FALSE;

// Check lock
  portENTER_CRITICAL();
  while (0 != ISO7816_GetLockCounter ())
  {
    portEXIT_CRITICAL();
    CI_TickLocalPrintf ("USB_CCID - *** WAIT for unlock ISO7816 counter  - %3d msec***\r\n", ISO7816_GetLockCounter ()*10);
    LockActive_u32 = TRUE;
    DelayMs (50);       // Wait for unlock
    portENTER_CRITICAL();
  }

// Set lock counter
  USB_CCID_LockCounter_u32 = Value_u32;

// Restart power of counter
  USB_CCID_SetPowerOffDelayCounter (USB_CCID_POWER_OFF_NORMAL);

  portEXIT_CRITICAL();

  if (TRUE == LockActive_u32)     // Clear the IO line when switching the access route
  {
//    ISO7816_InitSC ();
    ISO7816_ClearRx ();
  }
}

/*******************************************************************************

  USB_CCID_DecLockCounter

  Changes
  Date      Author          Info
  16.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void USB_CCID_DecLockCounter (void)
{

  USB_CCID_DecPowerOffDelayCounter ();

  if (0 != USB_CCID_LockCounter_u32)
  {
    USB_CCID_LockCounter_u32--;
#ifdef DEBUG_USB_CCID_LOCK
    if (0 == USB_CCID_LockCounter_u32)
    {
      u8 Text[20];
      itoa (xTickCount/2,Text);
      CI_StringOut (Text);
      CI_StringOut (" USB_CCID - Lock counter = 0\r\n");
    }
#endif
  }
}

/*******************************************************************************

  USB_CCID_GetLockCounter

  Changes
  Date      Author          Info
  16.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 USB_CCID_GetLockCounter (void)
{
  return (USB_CCID_LockCounter_u32);
}

/*
    case CCID_CONTROL_ABORT:
      CI_TickLocalPrintf ("USB_CCID - CCID_CONTROL_ABORT\r\n");
      break;
    case CCID_CONTROL_GET_CLOCK_FREQUENCIES:
      CI_TickLocalPrintf ("USB_CCID - CCID_CONTROL_GET_CLOCK_FREQUENCIES\r\n");
      break;
    case CCID_CONTROL_GET_DATA_RATES:
      CI_TickLocalPrintf ("USB_CCID - CCID_CONTROL_GET_DATA_RATES\r\n");
      break;
*/

/*******************************************************************************

  USB_CCID_DebugCmdStart

  Changes
  Date      Author          Info
  16.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
#ifdef DEBUG_USB_CCID_IO_DETAIL
void USB_CCID_DebugCmdStart (t_USB_CCID_data_st *USB_CCID_data_pst)
{
  switch(USB_CCID_data_pst->USB_data[CCID_OFFSET_MESSAGE_TYPE])
  {
case PC_TO_RDR_ICCPOWERON:
      CI_TickLocalPrintf ("USB_CCID - ICCPOWERON\r\n");
      break;
    case PC_TO_RDR_ICCPOWEROFF:
      CI_TickLocalPrintf ("USB_CCID - ICCPOWEROFF\r\n");
      break;
    case PC_TO_RDR_GETSLOTSTATUS:
      CI_TickLocalPrintf ("USB_CCID - GETSLOTSTATUS\r\n");
      break;
    case PC_TO_RDR_XFRBLOCK:
      CI_TickLocalPrintf ("USB_CCID - XFRBLOCK\r\n");
      break;
    case PC_TO_RDR_GETPARAMETERS:
      CI_TickLocalPrintf ("USB_CCID - GETPARAMETERS\r\n");
      break;
    case PC_TO_RDR_RESETPARAMETERS:
      CI_TickLocalPrintf ("USB_CCID - RESETPARAMETERS\r\n");
      break;
    case PC_TO_RDR_SETPARAMETERS:
      CI_TickLocalPrintf ("USB_CCID - SETPARAMETERS\r\n");
      break;
    case PC_TO_RDR_ESCAPE:
      CI_TickLocalPrintf ("USB_CCID - ESCAPE\r\n");
      break;
    case PC_TO_RDR_ICCCLOCK:
      CI_TickLocalPrintf ("USB_CCID - ICCCLOCK\r\n");
      break;
    case PC_TO_RDR_ABORT:
      CI_TickLocalPrintf ("USB_CCID - ABORT\r\n");
      break;
    case PC_TO_RDR_T0APDU:
      CI_TickLocalPrintf ("USB_CCID - T0APDU\r\n");
      break;
    case PC_TO_RDR_SECURE:
      CI_TickLocalPrintf ("USB_CCID - SECURE\r\n");
      break;
    case PC_TO_RDR_MECHANICAL:
      CI_TickLocalPrintf ("USB_CCID - MECHANICAL\r\n");
      break;
    case PC_TO_RDR_SET_DATA_RATE_AND_CLOCK_FREQUENCY:
      CI_TickLocalPrintf ("USB_CCID - SET_DATA_RATE_AND_CLOCK_FREQUENCY\r\n");
      break;
    default:
      CI_TickLocalPrintf ("USB_CCID - *** UNKNOWN ***\r\n");
      break;
  }
}
#endif
/*******************************************************************************

  USB_CCID_DebugCmdEnd

  Changes
  Date      Author          Info
  16.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
#ifdef DEBUG_USB_CCID_IO_DETAIL
void USB_CCID_DebugCmdEnd (t_USB_CCID_data_st *USB_CCID_data_pst)
{
  CI_TickLocalPrintf ("USB_CCID - End of call\r\n");
}
#endif
/*******************************************************************************

  CCID_RestartSmartcard_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 CCID_RestartSmartcard_u8 (void)
{
	int nRet;

	nRet = ISO7816_InitSC ();
	if (TRUE == nRet)
	{
	  CI_TickLocalPrintf ("*** Smartcard ON ***\n");
		CCID_SlotStatus_u8 = CCID_SLOT_STATUS_PRESENT_ACTIVE;
	}
	else
	{
	  CI_TickLocalPrintf ("*** ERROR Smartcard is not ON ***\n");
		CCID_SlotStatus_u8 = CCID_SLOT_STATUS_PRESENT_INACTIVE;
	}

	return (nRet);
}

/*******************************************************************************

  CCID_SmartcardOff_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
u8 CCID_ExternalSetSmartcardOffFlag_u8 = FALSE;

u8 CCID_SmartcardOff_u8 (void)
{

  if (0 == ISO7816_GetLockCounter ())
  {
    CI_TickLocalPrintf ("*** Smartcard off (CCID)***\n");
    Smartcard_Reset_off ();		// Disable SC
    SmartcardPowerOff ();
    CCID_SlotStatus_u8 = CCID_SLOT_STATUS_PRESENT_INACTIVE;
    CCID_ExternalSetSmartcardOffFlag_u8 = FALSE;
  }
  else
  {
    CI_TickLocalPrintf ("*** Smartcard NOT switched off (because internal access) ***\n");
    CCID_ExternalSetSmartcardOffFlag_u8 = TRUE;
  }

	return (TRUE);
}

/*******************************************************************************

  CCID_InternalSmartcardOff_u8

  Changes
  Date      Author          Info
  18.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 CCID_InternalSetSmartcardOffFlag_u8 = FALSE;

u8 CCID_InternalSmartcardOff_u8 (void)
{

  if (0 == USB_CCID_GetPowerOffDelayCounter ())
  {
    CI_TickLocalPrintf ("*** Smartcard off (I)***\n");
    Smartcard_Reset_off ();   // Disable SC
    SmartcardPowerOff ();
    CCID_SlotStatus_u8 = CCID_SLOT_STATUS_PRESENT_INACTIVE;
    CCID_InternalSetSmartcardOffFlag_u8 = FALSE;
  }
  else
  {
    CI_TickLocalPrintf ("*** Smartcard NOT switched off (because external access) ***\n");
    CCID_InternalSetSmartcardOffFlag_u8 = TRUE;
  }

  return (TRUE);
}

/*******************************************************************************

  CCID_GetHwError_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 CCID_GetHwError_u8 (u8 *SC_ErrorCode_u8)
{
	return (CCID_NO_ERROR);
}

/*******************************************************************************

  CCID_SetCardState_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CCID_SetCardState_v (unsigned char nState)
{
//	cCRD_CardPresent = nState;	
}
/*******************************************************************************

  CCID_GetCardState_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 CCID_GetCardState_u8 (void)
{
	return (TRUE);	
}
/*******************************************************************************

  CCID_GetSlotStatus_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 CCID_GetSlotStatus_u8 (void)
{
	return (CCID_SlotStatus_u8); // RB TODO CCID_SlotStatus_u8);
}
/*******************************************************************************

  CCID_SetSlotStatus_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CCID_SetSlotStatus_u8 (u8 SlotStatus_u8)
{
	CCID_SlotStatus_u8 = SlotStatus_u8;	
}

/*******************************************************************************

  CCID_GetClockStatus_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 CCID_GetClockStatus_u8 (void)
{
	return (CCID_ClockStatus_u8);	
}

/*******************************************************************************

  CCID_SetClockStatus_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CCID_SetClockStatus_u8 (u8 ClockStatus_u8)
{
	CCID_ClockStatus_u8 = ClockStatus_u8;	
}
/*******************************************************************************

  CCID_SetATRData_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 CCID_SetATRData_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	USB_CCID_data_pst->CCID_datalen = 0; //CCID_OFFSET_XFR_BLOCK_DATA;

	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH] = ISO7816_CopyATR ((u8*)&USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA],USB_CCID_MAX_LENGTH);
	USB_CCID_data_pst->CCID_datalen += USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH];

	USB_CCID_data_pst->USB_data[12] = 0x18; // Hack for slow SC baudrate

	return (TRUE);	
}
/*******************************************************************************

  CCID_XfrBlock_u8

  Erhöht den Eventzähler eines Zählers

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 CCID_XfrBlock_u8 (t_USB_CCID_data_st *USB_CCID_data_pst,u16 *CCID_AnswerSize_pu16,u16 CCID_LevelParameter_u16)
{
	s32 Ret_s32;
	s32 XfrLenght_s32;
//	u32 TickStart_u32;


	XfrLenght_s32 = USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH];
/*
TickStart_u32 =	xTaskGetTickCount();
CI_LocalPrintf ("%7d : CCID_XfrBlock - Max len %3d - ",TickStart_u32,XfrLenght_s32);

Print_T1_Block (USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH],&USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA]);
*/
#ifdef DEBUG_LOG_CCID_DETAIL
	LogStart_T1_Block (USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH],(u8*)&USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA]);
#endif

	Ret_s32 = ISO7816_T1_DirectXfr (&XfrLenght_s32,(u8*)&USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA],CCID_MAX_XFER_LENGTH);

	if (USART_SUCCESS == Ret_s32)
	{
#ifdef DEBUG_LOG_CCID_DETAIL
	  LogEnd_T1_Block (XfrLenght_s32,(u8*)&USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA]);
#endif
//CI_LocalPrintf (" Answer len %3d - %5d ms\n",XfrLenght_s32,xTaskGetTickCount()-TickStart_u32);
//HexPrint (XfrLenght_s32,&USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA]);

		*CCID_AnswerSize_pu16 = (u16)XfrLenght_s32;
		Ret_s32 = CCID_NO_ERROR;
	}
	else
	{
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH] = 0;
		Ret_s32 = CCID_ERROR_HW_ERROR;
CI_TickLocalPrintf ("CCID ERROR\n");
	}

	return (Ret_s32);
}
/*******************************************************************************

  CCID_CheckAbortRequest_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 CCID_CheckAbortRequest_u8 (void)
{
	return (FALSE);	
}
/*******************************************************************************

  RDR_to_PC_DataBlock_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 RDR_to_PC_DataBlock_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{

	USB_CCID_data_pst->USB_data[CCID_OFFSET_MESSAGE_TYPE] = RDR_TO_PC_DATA_BLOCK;
	return (TRUE);
}
/*******************************************************************************

  RDR_to_PC_SlotStatus_CardStopped_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 RDR_to_PC_SlotStatus_CardStopped_u8 (u8 ErrorCode_u8)
{
	return (TRUE);
}

/*******************************************************************************

  PC_to_RDR_IccPowerOn_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PC_to_RDR_IccPowerOn_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	if(0 != USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT])
	{
		return (CCID_ERROR_BAD_SLOT);
	}

	if (0 != USB_CCID_data_pst->CCID_datalen)
	{
		return (CCID_ERROR_BAD_LENTGH);
	}

	if(TRUE == USB_CCID_data_pst->CCID_CMD_aborted)
	{
		return (CCID_ERROR_CMD_ABORTED);
	}

	if (CCID_SLOT_STATUS_PRESENT_ACTIVE == CCID_SlotStatus_u8) // If smartcard is on, don't start
  {
    return (CCID_NO_ERROR);
  }

// We used only one voltage
	if (FALSE == CCID_RestartSmartcard_u8 ())
	{
		return (CCID_ERROR_HW_ERROR);
	}

	return (CCID_NO_ERROR);
}

/*******************************************************************************

  PC_to_RDR_IccPowerOff_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PC_to_RDR_IccPowerOff_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	if(0 != USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT])
	{
		return (CCID_ERROR_BAD_SLOT);
	}

	if (0 != USB_CCID_data_pst->CCID_datalen)
	{
		return (CCID_ERROR_BAD_LENTGH);
	}

// Do nothing, restart card at SC on
  CCID_SmartcardOff_u8 ();

	return (CCID_NO_ERROR);
}
/*******************************************************************************

  PC_to_RDR_GetSlotStatus_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PC_to_RDR_GetSlotStatus_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	u8 ErrorCode_u8;
	u8 SC_ErrorCode_u8;

	if(0 != USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT])
	{
		return (CCID_ERROR_BAD_SLOT);
	}

	if (0 != USB_CCID_data_pst->CCID_datalen)
	{
		return (CCID_ERROR_BAD_LENTGH);
	}

	ErrorCode_u8 = CCID_GetHwError_u8 (&SC_ErrorCode_u8);

	return (ErrorCode_u8);
}
/*******************************************************************************

  PC_to_RDR_XfrBlock_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PC_to_RDR_XfrBlock_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	u16 CCID_AnswerBlockSize_u16;
	u16 CCID_LevelParameter_u16;
	u32 UsbMessageLength_u32;

	u8 ErrorCode_u8;
	u8 SC_ErrorCode_u8;

	if(0 != USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT])
	{
		return (CCID_ERROR_BAD_SLOT);
	}

 	if (0 == USB_CCID_data_pst->CCID_datalen)
	{
		return (CCID_ERROR_BAD_LENTGH);
	}

	if(TRUE == USB_CCID_data_pst->CCID_CMD_aborted)
	{
		return (CCID_ERROR_CMD_ABORTED);
	}

	ErrorCode_u8 = CCID_GetHwError_u8(&SC_ErrorCode_u8);

	if(CCID_NO_ERROR != ErrorCode_u8)
	{
		return (ErrorCode_u8);
	}

// Check for size command
	if ((0xC1 == USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA+1]) &&
		(0x01 == USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA+2]) &&
		(0xFE == USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA+3]) &&
		(0x3E == USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA+4]))
	{
		USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA+1] = 0xE1;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA+2] = 0x01;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA+3] = 0x20;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_DATA+4] = 0xC0;
		CCID_AnswerBlockSize_u16 = 5;
		return (CCID_NO_ERROR);
	}


// This parameter did not define the answer size RB ???
	CCID_LevelParameter_u16 = USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_LEVEL_PARAMETER+1] * 256 +
							  USB_CCID_data_pst->USB_data[CCID_OFFSET_XFR_BLOCK_LEVEL_PARAMETER];

	ErrorCode_u8 = CCID_XfrBlock_u8 ( USB_CCID_data_pst, 
		                              &CCID_AnswerBlockSize_u16,
									   CCID_LevelParameter_u16);

	if(TRUE ==  CCID_CheckAbortRequest_u8 ())
	{
		return (CCID_ERROR_CMD_ABORTED);
	}

	if(CCID_NO_ERROR == ErrorCode_u8)
	{
		UsbMessageLength_u32 = CCID_AnswerBlockSize_u16;
		USB_CCID_data_pst->CCID_datalen                   = CCID_AnswerBlockSize_u16;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH]   = (u8)  UsbMessageLength_u32;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+1] = (u8) (UsbMessageLength_u32 >> 8);
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+2] = 0x00;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+3] = 0x00;
	}

	return (ErrorCode_u8);
}
/*******************************************************************************

  PC_to_RDR_GetParameters_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PC_to_RDR_GetParameters_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	u8 ErrorCode_u8 = CCID_NO_ERROR;

	if(0 != USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT])
	{
		return (CCID_ERROR_BAD_SLOT);
	}

	if (0 != USB_CCID_data_pst->CCID_datalen)
	{
		return (CCID_ERROR_BAD_LENTGH);
	}

// Return default parameter 

	return (ErrorCode_u8);
}
/*******************************************************************************

  PC_to_RDR_ResetParameters_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define		CCID_PARAMETER_CLOCK_NOTSTOPPED					 	0x00
#define		CCID_PARAMETER_CLOCK_STOPLOW					    0x01
#define		CCID_PARAMETER_CLOCK_STOPHIGH					    0x02
#define		CCID_PARAMETER_CLOCK_STOPHIGHORLOW				0x03

#define		CCID_DEFAULT_PARAMETER_FIDI								0x11
#define		CCID_DEFAULT_PARAMETER_T01CONVCHECKSUM		0x00				// No value ??
#define		CCID_DEFAULT_PARAMETER_GUARDTIME					0x00
#define		CCID_DEFAULT_PARAMETER_WAITING_INTEGER		0x0A
#define		CCID_DEFAULT_PARAMETER_CLOCK_STOP					CCID_PARAMETER_CLOCK_STOPHIGHORLOW
#define		CCID_DEFAULT_PARAMETER_IFSC								0x20
#define		CCID_DEFAULT_PARAMETER_NAD								0x00

u8 PC_to_RDR_ResetParameters_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	u8 ErrorCode_u8 = CCID_NO_ERROR;
	u8 SC_ErrorCode_u8;

	if(0 != USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT])
	{
		return (CCID_ERROR_BAD_SLOT);
	}

	if (0 != USB_CCID_data_pst->CCID_datalen)
	{
		return (CCID_ERROR_BAD_LENTGH);
	}

// Todo: Send default parameter
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA]   = CCID_DEFAULT_PARAMETER_FIDI;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+1] = CCID_DEFAULT_PARAMETER_T01CONVCHECKSUM;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+2] = CCID_DEFAULT_PARAMETER_GUARDTIME;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+3] = CCID_DEFAULT_PARAMETER_WAITING_INTEGER;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+4] = CCID_DEFAULT_PARAMETER_CLOCK_STOP;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+5] = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+6] = 0x00;

//	ErrorCode_u8 = IFD_SetParameters(&USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA], 0x00);

	if(CCID_NO_ERROR != ErrorCode_u8)
	{
		return (ErrorCode_u8);
	}

	ErrorCode_u8 = CCID_GetHwError_u8(&SC_ErrorCode_u8);

	return (ErrorCode_u8);
}

/*******************************************************************************

  PC_to_RDR_SetParameters_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PC_to_RDR_SetParameters_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	u8 ErrorCode_u8;
	u8 SC_ErrorCode_u8;

	if(0 != USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT])
	{
		return (CCID_ERROR_BAD_SLOT);
	}

	if ((5 != USB_CCID_data_pst->CCID_datalen) && (7 != USB_CCID_data_pst->CCID_datalen))
	{
		return (CCID_ERROR_BAD_LENTGH);
	}

	ErrorCode_u8 = CCID_NO_ERROR;

	if(CCID_NO_ERROR != ErrorCode_u8)
	{
		return (ErrorCode_u8);
	}

// Answer of SetParameters	Test only for OpenPGG cards
	USB_CCID_data_pst->USB_data[CCID_OFFSET_MESSAGE_TYPE]    = RDR_TO_PC_PARAMETERS;

	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH]          = (unsigned char) 7;				// Protocol T=1
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+1]        = (unsigned char) (7>>8);
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+2]        = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+3]        = 0x00;

	USB_CCID_data_pst->USB_data[CCID_OFFSET_STATUS] 		     = CCID_GetSlotStatus_u8 ();
	USB_CCID_data_pst->USB_data[CCID_OFFSET_ERROR] 		       = 0x00;

// Todo: Send default parameter
// Take the send parameter
/*
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA]   = CCID_DEFAULT_PARAMETER_FIDI;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+1] = CCID_DEFAULT_PARAMETER_T01CONVCHECKSUM;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+2] = CCID_DEFAULT_PARAMETER_GUARDTIME;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+3] = CCID_DEFAULT_PARAMETER_WAITING_INTEGER;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+4] = CCID_DEFAULT_PARAMETER_CLOCK_STOP;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+5] = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+6] = 0x00;
*/

	ErrorCode_u8 = CCID_GetHwError_u8(&SC_ErrorCode_u8);

	return (ErrorCode_u8);

}
/*******************************************************************************

  PC_to_RDR_Escape_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PC_to_RDR_Escape_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{


	u8 ErrorCode_u8;
	u8 SC_ErrorCode_u8;

	if(0 != USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT])
	{
		return (CCID_ERROR_BAD_SLOT);
	}

	ErrorCode_u8 = CCID_GetHwError_u8(&SC_ErrorCode_u8);

	return (ErrorCode_u8);

}
/*******************************************************************************

  PC_to_RDR_IccClock_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PC_to_RDR_IccClock_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{

	u8 ErrorCode_u8 = CCID_NO_ERROR;

	return ErrorCode_u8;

}
/*******************************************************************************

  PC_to_RDR_Abort_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

/******************************************************************************

  PC_to_RDR_Abort_u8

******************************************************************************/

u8 PC_to_RDR_Abort_u8 (t_USB_CCID_data_st *USB_CCID_data_pst)
{

	u8 ErrorCode_u8 = CCID_NO_ERROR;

	return (ErrorCode_u8);

}
/*******************************************************************************

  RDR_to_PC_DataBlock_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void RDR_to_PC_DataBlock_v (t_USB_CCID_data_st *USB_CCID_data_pst,u8 ErrorCode_u8)
{
	USB_CCID_data_pst->USB_data[CCID_OFFSET_MESSAGE_TYPE] = RDR_TO_PC_DATA_BLOCK;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_STATUS] 	  = CCID_GetSlotStatus_u8 ();
	USB_CCID_data_pst->USB_data[CCID_OFFSET_ERROR]        = 0x00;

	if(CCID_NO_ERROR != ErrorCode_u8)
	{
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH]        = 0x00;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+1]      = 0x00;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+2]      = 0x00;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+3]      = 0x00;

		USB_CCID_data_pst->USB_data[CCID_OFFSET_STATUS]       += 0x40;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_ERROR]         = ErrorCode_u8;
	}

	USB_CCID_data_pst->USB_data[CCID_OFFSET_CHAIN_PARAMETER] = 0x00;
}
/*******************************************************************************

  RDR_to_PC_SlotStatus_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void RDR_to_PC_SlotStatus_v (t_USB_CCID_data_st *USB_CCID_data_pst,u8 ErrorCode_u8)
{

	USB_CCID_data_pst->USB_data[CCID_OFFSET_MESSAGE_TYPE] = RDR_TO_PC_SLOT_STATUS;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_STATUS] 		  = CCID_GetSlotStatus_u8 ();

	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH]       = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+1]     = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+2]     = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+3]     = 0x00;

	if(CCID_NO_ERROR == ErrorCode_u8)
	{
		USB_CCID_data_pst->USB_data[CCID_OFFSET_ERROR]		 = 0x00;
	}
	else
	{
		USB_CCID_data_pst->USB_data[CCID_OFFSET_STATUS]    += 0x40;
		USB_CCID_data_pst->USB_data[CCID_OFFSET_ERROR]      = ErrorCode_u8;
	}

	USB_CCID_data_pst->USB_data[CCID_OFFSET_CLOCK_STATUS] = CCID_GetClockStatus_u8 ();

}
/*******************************************************************************

  RDR_to_PC_SlotStatus_CardStopped_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void RDR_to_PC_SlotStatus_CardStopped_v (t_USB_CCID_data_st *USB_CCID_data_pst)
{

	RDR_to_PC_SlotStatus_v (USB_CCID_data_pst,CCID_NO_ERROR);

	USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT_STATUS_STATUS] 		  = CCID_SLOT_STATUS_PRESENT_INACTIVE;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SLOT_STATUS_CLOCK_STATUS] = CCID_SLOT_STATUS_CLOCK_STOPPED_LOW;
}
/*******************************************************************************

  RDR_to_PC_Parameters_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void RDR_to_PC_Parameters_v (t_USB_CCID_data_st *USB_CCID_data_pst)
{
  USB_CCID_data_pst->USB_data[CCID_OFFSET_MESSAGE_TYPE] = RDR_TO_PC_PARAMETERS;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH]       = 0x07;									// Only T1
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+1] 	  = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+2] 	  = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+3] 	  = 0x00;

	USB_CCID_data_pst->USB_data[CCID_OFFSET_STATUS] 	  = CCID_GetSlotStatus_u8 ();
	USB_CCID_data_pst->USB_data[CCID_OFFSET_PROTOCOL_NUM] = 0x01;


// Todo: Send default parameter
// Take the send parameter...
/*
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA]   = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+1] = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+2] = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+3] = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+4] = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+5] = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+6] = 0x00;
*/
/*
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA]   = CCID_DEFAULT_PARAMETER_FIDI;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+1] = CCID_DEFAULT_PARAMETER_T01CONVCHECKSUM;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+2] = CCID_DEFAULT_PARAMETER_GUARDTIME;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+3] = CCID_DEFAULT_PARAMETER_WAITING_INTEGER;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+4] = CCID_DEFAULT_PARAMETER_CLOCK_STOP;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+5] = 0x00;
	USB_CCID_data_pst->USB_data[CCID_OFFSET_SET_PARAMS_DATA+6] = 0x00;
*/
}
/*******************************************************************************

  RDR_to_PC_Escape_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void RDR_to_PC_Escape_v (t_USB_CCID_data_st *USB_CCID_data_pst)
{
/*
	UsbMessageBuffer[OFFSET_BMESSAGETYPE] = RDR_TO_PC_ESCAPE;
	UsbMessageBuffer[OFFSET_BSTATUS] 			= CRD_GetSlotStatus();
	UsbMessageBuffer[OFFSET_BERROR] 			= 0x00;

	if(ErrorCode_u8 != SLOT_NO_ERROR)
	{
		UsbMessageBuffer[OFFSET_BSTATUS]    += 0x40;
		UsbMessageBuffer[OFFSET_DWLENGTH]    = 0x00;
		UsbMessageBuffer[OFFSET_DWLENGTH+1]  = 0x00;
		UsbMessageBuffer[OFFSET_DWLENGTH+2]  = 0x00;
		UsbMessageBuffer[OFFSET_DWLENGTH+3]  = 0x00;
		UsbMessageBuffer[OFFSET_BERROR]      = ErrorCode_u8;
	}

	UsbMessageBuffer[OFFSET_BRFU] = 0x00;
*/
}

/*******************************************************************************

  RDR_to_PC_CmdNotSupported_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void RDR_to_PC_CmdNotSupported_v (t_USB_CCID_data_st *USB_CCID_data_pst)
{
/*
	//UsbMessageBuffer[OFFSET_BMESSAGETYPE] = 0x00;
	UsbMessageBuffer[OFFSET_DWLENGTH] 				= 0x00;
	UsbMessageBuffer[OFFSET_DWLENGTH+1] 			= 0x00;
	UsbMessageBuffer[OFFSET_DWLENGTH+2] 			= 0x00;
	UsbMessageBuffer[OFFSET_DWLENGTH+3]		 		= 0x00;
	UsbMessageBuffer[OFFSET_BSTATUS] 					= 0x40 + CRD_GetSlotStatus();
	UsbMessageBuffer[OFFSET_BERROR] 					= 0x00;
	UsbMessageBuffer[OFFSET_BCHAINPARAMETER] 	= 0x00;
*/
}
/*******************************************************************************

  CDR_to_USB_NotifySlotChange_v

  ISR

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CDR_to_USB_NotifySlotChange_v (t_USB_CCID_data_st *USB_CCID_data_pst)
{
/*
	UsbIntMessageBuffer[OFFSET_INT_BMESSAGETYPE] = RDR_TO_PC_NOTIFYSLOTCHANGE;

	if( CRD_GetSlotStatus() == CRD_NOTPRESENT )
	{
		UsbIntMessageBuffer[OFFSET_INT_BMSLOTICCSTATE] = 0x02;
	}
	else
	{
		UsbIntMessageBuffer[OFFSET_INT_BMSLOTICCSTATE] = 0x03;
	}
*/
}
/*******************************************************************************

  CRD_to_USB_HardwareError

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

/******************************************************************************

  CRD_to_USB_HardwareError

******************************************************************************/

u8 CRD_to_USB_HardwareError (t_USB_CCID_data_st *USB_CCID_data_pst)
{
/*
	unsigned char HwErrorCode = SLOT_NO_ERROR;
//	unsigned char ErrorCode_u8;

	UsbIntMessageBuffer[OFFSET_INT_BMESSAGETYPE] 				= RDR_TO_PC_HARDWAREERROR;
	UsbIntMessageBuffer[OFFSET_INT_BSLOT] 							= 0x00;
	UsbIntMessageBuffer[OFFSET_INT_BSEQ] 								= UsbMessageBuffer[OFFSET_BSEQ];
//	ErrorCode_u8 																					= CCID_GetHwError_u8(&HwErrorCode);
	UsbIntMessageBuffer[OFFSET_INT_BHARDWAREERRORCODE] 	= HwErrorCode;
*/
	return (0);
}

/*******************************************************************************

  USB_to_CRD_DispatchUSBMessage_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int USB_CCID_Simulate_Answer (t_USB_CCID_data_st *USB_CCID_data_pst);

extern volatile avr32_mci_t *mci;

void USB_to_CRD_DispatchUSBMessage_v (t_USB_CCID_data_st *USB_CCID_data_pst)
{
	u8 ErrorCode_u8 = CCID_NO_ERROR;
	u8 n;

	USB_CCID_data_pst->CCID_datalen = USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH+1] * 256 +
									   USB_CCID_data_pst->USB_data[CCID_OFFSET_LENGTH];

	n = USB_CCID_data_pst->USB_data[CCID_OFFSET_MESSAGE_TYPE];

  if (mci != &AVR32_MCI)      // To get sure that mci has the correct value
  {
    mci = &AVR32_MCI;
  }

#ifdef DEBUG_USB_CCID_IO_DETAIL
  USB_CCID_DebugCmdStart (USB_CCID_data_pst);
#endif

  USB_CCID_SetLockCounter (USB_CCID_LOCK_COUNT_LONG);

	switch(USB_CCID_data_pst->USB_data[CCID_OFFSET_MESSAGE_TYPE])
	{
		case PC_TO_RDR_ICCPOWERON:
		  USB_CCID_SetLockCounter (USB_CCID_LOCK_COUNT_POWERON);

			ErrorCode_u8	= PC_to_RDR_IccPowerOn_u8 (USB_CCID_data_pst);
			if (CCID_NO_ERROR == ErrorCode_u8)
			{
				DelayMs (100);	// We wait 100 ms for answering
				ErrorCode_u8 = CCID_SetATRData_u8 (USB_CCID_data_pst);														// Create ATR output Message
			}

#ifdef SIMULATE_USB_CCID_DISPATCH
			USB_CCID_Simulate_Answer (USB_CCID_data_pst);
#endif


			RDR_to_PC_DataBlock_u8 (USB_CCID_data_pst);
//			portDBG_TRACE("ICCPOWERON End   %d",xTaskGetTickCount());
			break;

		case PC_TO_RDR_ICCPOWEROFF:
			ErrorCode_u8 = PC_to_RDR_IccPowerOff_u8 (USB_CCID_data_pst);
//				RDR_to_PC_SlotStatus(ErrorCode_u8);
			RDR_to_PC_SlotStatus_CardStopped_v (USB_CCID_data_pst);		 	// simulate power off
      USB_CCID_SetLockCounter (USB_CCID_LOCK_COUNT_CLEAR);
			break;

		case PC_TO_RDR_GETSLOTSTATUS:
			ErrorCode_u8 = PC_to_RDR_GetSlotStatus_u8 (USB_CCID_data_pst);
			RDR_to_PC_SlotStatus_v (USB_CCID_data_pst,ErrorCode_u8);
			break;

		case PC_TO_RDR_XFRBLOCK:
if (mci != &AVR32_MCI)
{
  mci = &AVR32_MCI;
}
			ErrorCode_u8 = PC_to_RDR_XfrBlock_u8 (USB_CCID_data_pst);
if (mci != &AVR32_MCI)
{
  mci = &AVR32_MCI;
}
			RDR_to_PC_DataBlock_v (USB_CCID_data_pst,ErrorCode_u8);
#ifdef SIMULATE_USB_CCID_DISPATCH
			USB_CCID_Simulate_Answer (USB_CCID_data_pst);
#endif
			break;

		case PC_TO_RDR_GETPARAMETERS:
			ErrorCode_u8 = PC_to_RDR_GetParameters_u8 (USB_CCID_data_pst);
			RDR_to_PC_Parameters_v (USB_CCID_data_pst);
			break;

		case PC_TO_RDR_RESETPARAMETERS:
			ErrorCode_u8 = PC_to_RDR_ResetParameters_u8 (USB_CCID_data_pst);
			RDR_to_PC_Parameters_v (USB_CCID_data_pst);
			break;

		case PC_TO_RDR_SETPARAMETERS:
			ErrorCode_u8 = PC_to_RDR_SetParameters_u8 (USB_CCID_data_pst);
//USB_CCID_Simulate_Answer (USB_CCID_data_pst);
			RDR_to_PC_Parameters_v (USB_CCID_data_pst);
			break;

		case PC_TO_RDR_ESCAPE:
			ErrorCode_u8 = PC_to_RDR_Escape_u8 (USB_CCID_data_pst);
			RDR_to_PC_Escape_v (USB_CCID_data_pst);
			break;

		case PC_TO_RDR_ICCCLOCK:
			ErrorCode_u8 = PC_to_RDR_IccClock_u8 (USB_CCID_data_pst);
			RDR_to_PC_SlotStatus_v (USB_CCID_data_pst,ErrorCode_u8);
			break;

		case PC_TO_RDR_ABORT:
			ErrorCode_u8 = PC_to_RDR_Abort_u8 (USB_CCID_data_pst);
			RDR_to_PC_SlotStatus_v (USB_CCID_data_pst,ErrorCode_u8);
			break;

		case PC_TO_RDR_T0APDU:
		case PC_TO_RDR_SECURE:
		case PC_TO_RDR_MECHANICAL:
		case PC_TO_RDR_SET_DATA_RATE_AND_CLOCK_FREQUENCY:
		default:
			RDR_to_PC_CmdNotSupported_v (USB_CCID_data_pst);
			break;
	}

#ifdef DEBUG_USB_CCID_IO_DETAIL
  USB_CCID_DebugCmdEnd (USB_CCID_data_pst);
#endif

  USB_CCID_SetLockCounter (USB_CCID_LOCK_COUNT_NORMAL);     // Set delay to lock the smartcard for the next command

  if (mci != &AVR32_MCI)
  {
    mci = &AVR32_MCI;

  }
}

/*******************************************************************************

  CCID_IntMessage

  Transmit the CCID message by the Interrupt Endpoint 1.
  this message was in UsbIntMessageBuffer.

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CCID_IntMessage(void)
{
	USB_CCID_send_INT_Message ();
}

/*******************************************************************************

  CcidClassRequestAbort

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CcidClassRequestAbort(void)
{
}
/*******************************************************************************

  CcidClassRequestGetClockFrequencies

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CcidClassRequestGetClockFrequencies(void)
{
}

/*******************************************************************************

  CcidClassRequestGetDataRates

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CcidClassRequestGetDataRates(void)
{
}

/*******************************************************************************

  CRD_to_USB_SendCardDetect

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CRD_to_USB_SendCardDetect (void)
{
	CCID_IntMessage ();
}







