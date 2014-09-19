/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 24.06.2010
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
 * ISSO7816_ADPU.c
 *
 *  Created on: 24.06.2010
 *      Author: RB
 */

#include <avr32/io.h>
#include "stdio.h"
#include "string.h"
#include "compiler.h"
#include "board.h"
#include "power_clocks_lib.h"
#include "gpio.h"
#include "usart.h"

#ifdef FREERTOS_USED
	#include "FreeRTOS.h"
	#include "task.h"
#endif

#include "global.h"
#include "tools.h"
#include "ISO7816_USART.h"
#include "ISO7816_ADPU.h"
#include "ISO7816_Prot_T1.h"
#include "LED_test.h"



/*******************************************************************************

 Local defines

*******************************************************************************/

#define ISO7816_MAX_ATR_CHARS 		40

#define DEBUG_ISO7816_IO
//#define DEBUG_ISO7816_IO_DETAIL

#ifdef DEBUG_ISO7816_IO
  int CI_LocalPrintf (char *szFormat,...);
  int CI_TickLocalPrintf (char *szFormat,...);
  int CI_StringOut (char *szText);
#else
  #define CI_LocalPrintf(...)
  #define CI_TickLocalPrintf(...)
  #define CI_StringOut(...)
#endif
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

extern const usart_iso7816_options_t ISO7816_USART_ISO7816;

typedef struct  {
  int       nLen;
  unsigned char   szATR[ISO7816_MAX_ATR_CHARS];
}typedef_ISO7816_ATR;

int nISO7816_Error;

typedef_ISO7816_ATR tISO7816_ATR;

static u16 FI_List_au8[16] = {
    372,   // 0
    372,   // 1
    558,   // 2
    744,   // 3
   1116,   // 4
   1488,   // 5
   1860,   // 6
      0,   // 7, reserved
      0,   // 8, reserved
    512,   // 9
    768,   // 10
   1024,   // 11
   1536,   // 12
   2048,   // 13
      0,   // 14, reserved
      0,   // 15, reserved
};


static u8 DI_List_au8[16] = {
      0,   // 0, reserved
      1,   // 1
      2,   // 2
      4,   // 3
      8,   // 4
     16,   // 5
     32,   // 6
      0,   // 7, reserved
     12,   // 8
     20,   // 9
      0,   // 10, reserved
      0,   // 11, reserved
      0,   // 12, reserved
      0,   // 13, reserved
      0,   // 14, reserved
      0,   // 15, reserved
};

static u8 ISO7816_FiDi_u8 = 0x13;       // 0x11 = ca 10kbaud, 0x13 = ca. 40kbaud

static u32 ISO7816_LockCounter_u32 = 0;

#define ISO7816_LOCK_COUNT_NORMAL    50    // =   500 ms
#define ISO7816_LOCK_COUNT_POWERON 1000    // = 10000 ms
#define ISO7816_LOCK_COUNT_LONG    3000    // = 30000 ms
#define ISO7816_LOCK_COUNT_CLEAR     10    // =   100 ms

/*******************************************************************************

  ISO7816_SetLockCounter

  Changes
  Date      Author          Info
  16.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void ISO7816_SetLockCounter (u32 Value_u32)
{
  int LockActive_u32;

#ifdef DEBUG_ISO7816_IO_DETAIL
  {
    u8 Text[20];
    itoa (xTickCount/2,Text);   // in msec
    CI_StringOut (Text);
    CI_StringOut (" ISO7816 - Set lock counter = ");
    itoa (Value_u32*10,Text);
    CI_StringOut (Text);
    CI_StringOut (" msec\r\n");
  }
#endif

  LockActive_u32 = FALSE;

// Check lock
  portENTER_CRITICAL();
  while (0 != USB_CCID_GetLockCounter ())
  {
    portEXIT_CRITICAL();
    LockActive_u32 = TRUE;
    CI_TickLocalPrintf ("ISO7816 - *** WAIT for unlock CCID counter - %3d msec***\r\n", USB_CCID_GetLockCounter ()*10);
    DelayMs (50);       // Wait for unlock
    portENTER_CRITICAL();
  }

// Set lock counter
  ISO7816_LockCounter_u32 = Value_u32;
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

void ISO7816_DecLockCounter (void)
{
  if (0 != ISO7816_LockCounter_u32)
  {
    ISO7816_LockCounter_u32--;
#ifdef DEBUG_ISO7816_IO_DETAIL
    if (0 == ISO7816_LockCounter_u32)
    {
      u8 Text[20];
      itoa (xTickCount/2,Text);   // in msec
      CI_StringOut (Text);
      CI_StringOut (" ISO7816 - Lock counter = 0\r\n");
    }
#endif
  }
}

/*******************************************************************************

  ISO7816_GetLockCounter

  Changes
  Date      Author          Info
  16.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 ISO7816_GetLockCounter (void)
{
  return (ISO7816_LockCounter_u32);
}

/*******************************************************************************

  ISO7816_SetFiDI

  Changes
  Date      Author          Info
  15.07.14  RB              Creation of function

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void ISO7816_SetFiDI (u8 FiDi_u8)
{
  ISO7816_FiDi_u8 = FiDi_u8;
}

/*******************************************************************************

  ISO7816_ResetSC_test

  Reset smartcard and get ATR

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_GetATP (typedef_ISO7816_ATR *tATR)
{
	int nRet;

// Disable SC
	Smartcard_Reset_off ();

//
  SmartcardPowerOff ();

// Delay 40000 SC clocks ca. 10 ms
	DelayMs (40);

// Init UART
	nRet = Init_ISO7816_Usart ();

//	usart_enable_tx (ISO7816_USART);
//	usart_enable_rx (ISO7816_USART);
//	ISO7816_Usart_ISR_InitRxBuffer ();

  SmartcardPowerOn ();

// Delay 40000 SC clocks ca. 10 ms
	DelayMs (10);

//  ISO7816_Usart_ISR_InitRxBuffer ();
//  ISO7816_ClearRx ();


  usart_reset_tx (ISO7816_USART); // reset the tx port to free the port for receiving - Chip bug

  DelayMs(100);

// Clear SIO
//  usart_enable_tx (ISO7816_USART);
//  usart_enable_rx (ISO7816_USART);

// Clear SIO ISR Buffer
//  ISO7816_Usart_ISR_InitRxBuffer ();

// Enable SC
	Smartcard_Reset_on ();

// Get ATR
	tATR->nLen = ISO7816_MAX_ATR_CHARS;
	nRet = ISO7816_ReadString (&tATR->nLen,tATR->szATR);

// Get an ATR ?
	if (4 <= tATR->nLen)
	{
	  return (TRUE);     // get ATR
	}

// Try again
  tATR->nLen = ISO7816_MAX_ATR_CHARS;
  nRet = ISO7816_ReadString (&tATR->nLen,tATR->szATR);

  if (4 > tATR->nLen)
  {
	  nISO7816_Error = nRet; // No
		return (FALSE);
	}

  return (TRUE);     // get ATR
}

/*******************************************************************************

  ISO7816_SendPTS

  Send PTS to change baud rate

  Changes
  Date      Author          Info
  14.07.14  RB              Change FIDI for faster communication

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#define ISO7816_PTS_MAX_CHARS       5

#define ISO7816_ATR_TS 				      0
#define ISO7816_ATR_T0 			      	1
#define ISO7816_ATR_A1_OPENPGP 		  2

int ISO7816_SendPTS (typedef_ISO7816_ATR *tATR)
{
	int 			      i;
	int  			      nRet;
	int 			      nPTS_Status[ISO7816_PTS_MAX_CHARS];
	int 			      nDelaytime;
	int   			    szPTS_Answer[ISO7816_PTS_MAX_CHARS];
	unsigned char 	szPTS[ISO7816_PTS_MAX_CHARS];
	int             baud;
	unsigned int    cd;
  unsigned int    FiDi;
  unsigned int    FiDiRatio;

// Is TA1 transmitted ?
	if (0 == (tATR->szATR[ISO7816_ATR_T0] & 0x10))
	{
		return (TRUE);	// No, send no PTS
	}

	FiDi = ISO7816_FiDi_u8;

/*	*/
  cd = (CCID_TARGET_PBACLK_FREQ_HZ + ISO7816_USART_ISO7816.iso7816_hz / 2) / ISO7816_USART_ISO7816.iso7816_hz; // = frequency divisor
  baud = CCID_TARGET_PBACLK_FREQ_HZ / cd / 372 * DI_List_au8[FiDi & 0x0f];
  CI_TickLocalPrintf ("ISO7816_SendPTS : Set FIDI to 0x%04x = %d baud\n",FiDi,baud);

// Generate PTS String
	szPTS[0] = 0xFF;									        // PTSS
	szPTS[1] = 0x11;									        // PTS0 > Send PTS1 + protocol type T = 1
	szPTS[2] = FiDi;	 	                      // PTS1 = FIDI	// 0x13
	szPTS[3] = ISO7816_XOR_String (3,szPTS);

// Send PTS
	ISO7816_ClearTx ();
	nRet = ISO7816_SendString (4,szPTS);
	if (USART_SUCCESS != nRet)
	{
    CI_TickLocalPrintf ("ISO7816_SendPTS : FAIL sending PTS return=%d\n",nRet);
	}

// Receive SC answer
	ISO7816_ClearRx ();
	nDelaytime = ISO7816_CHAR_DELAY_TIME_FIRST_CHAR_PTS;
	for (i=0;i<4;i++)
	{
		szPTS_Answer[i] = 0;
		nPTS_Status[i]  = ISO7816_ReadChar (&szPTS_Answer[i],nDelaytime);
		nDelaytime      = ISO7816_CHAR_DELAY_TIME_MS;
	}

// Check for correct answer
	for (i=0;i<4;i++)
	{
		if ((szPTS[i]      != szPTS_Answer[i]) ||
			  (USART_SUCCESS != nPTS_Status[i]))
		{
      CI_TickLocalPrintf ("ISO7816_SendPTS : FAIL char %d Status=%d - %02x != %02x\n",i,nPTS_Status[i],szPTS[i],szPTS_Answer[i]);
      CI_LocalPrintf ("Status- %02x %02x %02x %02x\n",nPTS_Status[0],nPTS_Status[1],nPTS_Status[2],nPTS_Status[3]);
      CI_LocalPrintf ("Send  - %02x %02x %02x %02x\n",szPTS[0],szPTS[1],szPTS[2],szPTS[3]);
      CI_LocalPrintf ("Answer- %02x %02x %02x %02x\n",szPTS_Answer[0],szPTS_Answer[1],szPTS_Answer[2],szPTS_Answer[3]);
			return (FALSE);
		}
	}

// Set new FiDi ratio
	FiDiRatio = FI_List_au8[FiDi >> 4] / DI_List_au8[FiDi & 0x0f];
	ISO7816_SetFiDiRatio (FiDiRatio);

	return (TRUE);
}

/*******************************************************************************

  ISO7816_InitSC

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#define USB_CCID_POWER_OFF_NORMAL   3000    // = 30000 ms
void USB_CCID_SetPowerOffDelayCounter (u32 Value_u32);

int ISO7816_InitSC (void)
{
	nISO7816_Error = USART_SUCCESS;
	int      i;
	int      nRet;
	int      nRetryCount;

	nRet = FALSE;

	nRetryCount = 10;

  CI_TickLocalPrintf ("ISO7816_InitSC : SC power off\n");

	for (i=0;i<nRetryCount;i++)
	{
// Restart power off counter
	  USB_CCID_SetPowerOffDelayCounter (USB_CCID_POWER_OFF_NORMAL);

    SmartcardPowerOff ();
    Smartcard_Reset_off ();
    ISO7816_T1_InitSendNr ();
    DelayMs (100);

    if (TRUE == ISO7816_GetATP (&tISO7816_ATR))
    {
      if (TRUE == ISO7816_SendPTS (&tISO7816_ATR))
      {
        CI_TickLocalPrintf ("ISO7816_InitSC : OK\n");
        DelayMs (10);
        return (TRUE);      // Exit
      }
      else
      {
        CI_TickLocalPrintf ("ISO7816_InitSC : NO PTS\n");
        DelayMs (10);
      }
    }
    else
    {
      CI_TickLocalPrintf ("ISO7816_InitSC : NO ATR\n");
    }
	}

	if (i != nRetryCount)
	{
	  return (TRUE);
	}

  CI_TickLocalPrintf ("ISO7816_InitSC : Init FAIL\n");

//  CI_TickLocalPrintf ("ISO7816_InitSC : Reset_CPU\n");

//  DelayMs (50);
//  Reset_CPU ();

	return (FALSE);
}



/*******************************************************************************

  ISO7816_InitSC

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

int ISO7816_SC_StartupTest (int nCount)
{
  nISO7816_Error = USART_SUCCESS;
  int      i;
  int      nRet;
  int      nNoATRError = 0;
  int      nNoPTSError = 0;

  nRet = FALSE;

  for (i=0;i<nCount;i++)
  {
    CI_TickLocalPrintf (" %6d : ",i);

    SmartcardPowerOff ();
    Smartcard_Reset_off ();
    DelayMs (100);

    if (TRUE == ISO7816_GetATP (&tISO7816_ATR))
    {
      if (TRUE == ISO7816_SendPTS (&tISO7816_ATR))
      {
        CI_LocalPrintf ("ISO7816_InitSC : OK\n");
      }
      else
      {
        CI_LocalPrintf ("ISO7816_InitSC : NO PTS\n");
        nNoPTSError++;
//        break; // Stop for oscar
      }
    }
    else
    {
      CI_LocalPrintf ("ISO7816_InitSC : NO ATR\n");
      nNoATRError++;
    }
    DelayMs (100);
  }

  CI_LocalPrintf ("\nISO7816_SC_StartupTest : %d startups\n",nCount);
  CI_LocalPrintf ("No ATR erros : %d\n",nNoATRError);
  CI_LocalPrintf ("NO PTS erros : %d\n",nNoPTSError);

  return (FALSE);
}

/*******************************************************************************

  ISO7816_CopyATR

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_CopyATR (unsigned char *szATR,int nMaxLength)
{
	int n;

	n = tISO7816_ATR.nLen;

	if (n > nMaxLength)
	{
		n = nMaxLength;
	}

	memcpy (szATR,tISO7816_ATR.szATR,n);

	return (n);
}


/*******************************************************************************

  ISO7816_SendAPDU_NoLe_NoLc

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_SendAPDU_NoLe_NoLc (typeAPDU *tSC)
{
  int           nRet;
//  int       nPointer;

  LED_RedOn ();

  ISO7816_SetLockCounter (ISO7816_LOCK_COUNT_NORMAL);

// Build the send string
  tSC->cSendData[0] = tSC->tAPDU.cCLA;
  tSC->cSendData[1] = tSC->tAPDU.cINS;
  tSC->cSendData[2] = tSC->tAPDU.cP1;
  tSC->cSendData[3] = tSC->tAPDU.cP2;

  tSC->nSendDataLen    = 4;
  tSC->nReceiveDataLen = ISO7816_APDU_MAX_RESPONSE_LEN;

// go
  nRet = ISO7816_T1_SendAPDU (tSC);

  ISO7816_SetLockCounter (ISO7816_LOCK_COUNT_CLEAR);

  LED_RedOff ();

  return (nRet);
}


/*******************************************************************************

  ISO7816_SendAPDU_Le_NoLc

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_SendAPDU_Le_NoLc (typeAPDU *tSC)
{
	int           nRet;
//	int 		  nPointer;

	LED_RedOn ();

	ISO7816_SetLockCounter (ISO7816_LOCK_COUNT_NORMAL);

// Build the send string
	tSC->cSendData[0] = tSC->tAPDU.cCLA;
	tSC->cSendData[1] = tSC->tAPDU.cINS;
	tSC->cSendData[2] = tSC->tAPDU.cP1;
	tSC->cSendData[3] = tSC->tAPDU.cP2;

// Extended transfer ?
	if (256 > tSC->tAPDU.nLe)
	{
		tSC->cSendData[4] = tSC->tAPDU.nLe;
		tSC->nSendDataLen = 5;
	}
	else
	{
		tSC->cSendData[4] = 0;										// extended transfer
		tSC->cSendData[5] = (unsigned char)  tSC->tAPDU.nLe >> 8;
		tSC->cSendData[6] = (unsigned char) (tSC->tAPDU.nLe & 0xFF);
		tSC->nSendDataLen = 7;
	}

// Set max receive data length
	tSC->nReceiveDataLen = ISO7816_APDU_MAX_RESPONSE_LEN;

// go
	nRet = ISO7816_T1_SendAPDU (tSC);

  ISO7816_SetLockCounter (ISO7816_LOCK_COUNT_CLEAR);

  LED_RedOff ();

	return (nRet);
}

/*******************************************************************************

  ISO7816_SendAPDU_NoLe_Lc

  Reviews
  Date      Reviewer        Info
  16.07.14  RB              First review

*******************************************************************************/

int ISO7816_SendAPDU_NoLe_Lc (typeAPDU *tSC)
{
	int           nRet;

	LED_RedOn ();

  ISO7816_SetLockCounter (ISO7816_LOCK_COUNT_NORMAL);

// Build the send string
	tSC->cSendData[0] = tSC->tAPDU.cCLA;
	tSC->cSendData[1] = tSC->tAPDU.cINS;
	tSC->cSendData[2] = tSC->tAPDU.cP1;
	tSC->cSendData[3] = tSC->tAPDU.cP2;

// Extended transfer ?
	if ((256 > tSC->tAPDU.nLc) && (256 > tSC->tAPDU.nLe))
	{
		tSC->cSendData[4] = tSC->tAPDU.nLc;
		tSC->nSendDataLen = 5;
	}
	else
	{
		tSC->cSendData[4] = 0;										// extended transfer
		tSC->cSendData[5] = (unsigned char)  tSC->tAPDU.nLc >> 8;
		tSC->cSendData[6] = (unsigned char) (tSC->tAPDU.nLc & 0xFF);
		tSC->nSendDataLen = 7;
	}

	memcpy (&tSC->cSendData[tSC->nSendDataLen],tSC->tAPDU.cData,(tSC->tAPDU.nLc & 0xFF));

// Fix length
	tSC->nSendDataLen = tSC->nSendDataLen + tSC->tAPDU.nLc;

// Set max receive data length
	tSC->nReceiveDataLen = 0;

// go
	nRet = ISO7816_T1_SendAPDU (tSC);

  ISO7816_SetLockCounter (ISO7816_LOCK_COUNT_CLEAR);

  LED_RedOff ();

	return (nRet);
}

/*******************************************************************************

  ISO7816_SendAPDU_Le_Lc

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_SendAPDU_Le_Lc (typeAPDU *tSC)
{
	int           nRet;

	LED_RedOn ();

	ISO7816_SetLockCounter (ISO7816_LOCK_COUNT_NORMAL);

// Build the header
	tSC->cSendData[0] = tSC->tAPDU.cCLA;
	tSC->cSendData[1] = tSC->tAPDU.cINS;
	tSC->cSendData[2] = tSC->tAPDU.cP1;
	tSC->cSendData[3] = tSC->tAPDU.cP2;

// Extended transfer ?
	if (256 > tSC->tAPDU.nLc)
	{
		tSC->cSendData[4] = tSC->tAPDU.nLc;
		tSC->nSendDataLen = 5;
	}
	else
	{
		tSC->cSendData[4] = 0;										// extended transfer
		tSC->cSendData[5] = (unsigned char) ((tSC->tAPDU.nLc >> 8) & 0xFF);
		tSC->cSendData[6] = (unsigned char) (tSC->tAPDU.nLc & 0xFF);
		tSC->nSendDataLen = 7;
	}


// Add data
	memcpy (&tSC->cSendData[tSC->nSendDataLen],tSC->tAPDU.cData,tSC->tAPDU.nLc);
	tSC->nSendDataLen += tSC->tAPDU.nLc;

// Add LE
// Limit receive size
	if (ISO7816_APDU_MAX_RESPONSE_LEN <= tSC->tAPDU.nLe)
	{
// for testing		tSC->tAPDU.nLe = ISO7816_APDU_MAX_RESPONSE_LEN;
	}

// Extended transfer ?
	if (256 > tSC->tAPDU.nLe)
	{
		tSC->cSendData[tSC->nSendDataLen] = (unsigned char) (tSC->tAPDU.nLe & 0xFF);
		tSC->nSendDataLen += 1;
	}
	else
	{
		tSC->cSendData[tSC->nSendDataLen] = (unsigned char) (tSC->tAPDU.nLe >> 8);
		tSC->nSendDataLen += 1;
		tSC->cSendData[tSC->nSendDataLen] = (unsigned char) (tSC->tAPDU.nLe & 0xFF);
		tSC->nSendDataLen += 1;
	}

// Set max receive data length
	tSC->nReceiveDataLen = ISO7816_APDU_MAX_RESPONSE_LEN;

// go
	nRet = ISO7816_T1_SendAPDU (tSC);

  ISO7816_SetLockCounter (ISO7816_LOCK_COUNT_CLEAR);

  LED_RedOff ();

	return (nRet);
}

/*******************************************************************************

  ISO7816_SendAPDU

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_SendAPDU (typeAPDU *tSC)
{
	int           nRet;

	if ((0 == tSC->tAPDU.nLe) && (0 == tSC->tAPDU.nLc))
	{
//		nRet = ISO7816_SendAPDU_NoLe_NoLc (tSC);
	}
	else if ((0 != tSC->tAPDU.nLe) && (0 == tSC->tAPDU.nLc))
	{
		nRet = ISO7816_SendAPDU_Le_NoLc (tSC);
	}
	else if ((0 == tSC->tAPDU.nLe) && (0 != tSC->tAPDU.nLc))
	{
		nRet = ISO7816_SendAPDU_NoLe_Lc (tSC);
	}
	else if ((0 != tSC->tAPDU.nLe) && (0 != tSC->tAPDU.nLc))
	{
		nRet = ISO7816_SendAPDU_Le_Lc (tSC);
	}

	return (nRet);
}

/*******************************************************************************

  ISO7816_APDU_Test

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_APDU_Test (void)
{
	typeAPDU tSC;

	ISO7816_InitSC ();


/*
	SELECT FILE - so übernehmen
	00 00 0B
	gpg: DBG: send apdu: c=00 i=A4 p1=04 p2=00 lc=6 le=-1 em=0
	gpg: DBG:   PCSC_data: 00 A4 04 00 06 D2 76 00 01 24 01
	gpg: DBG:  response: sw=9000  datalen=0
	gpg: DBG:     dump:
	2D
*/

// Command
	tSC.tAPDU.cCLA     = 0x00;
	tSC.tAPDU.cINS     = 0xA4;
	tSC.tAPDU.cP1      = 0x04;
	tSC.tAPDU.cP2      = 0x00;
	tSC.nSendDataLen   = 6;
	tSC.tAPDU.nLc      = tSC.nSendDataLen;

// Send data
	tSC.tAPDU.cData[0] = 0xD2;
	tSC.tAPDU.cData[1] = 0x76;
	tSC.tAPDU.cData[2] = 0x00;
	tSC.tAPDU.cData[3] = 0x01;
	tSC.tAPDU.cData[4] = 0x24;
	tSC.tAPDU.cData[5] = 0x01;

// Receive data - up to 256 byte
	tSC.tAPDU.nLe      = 0x100;

	ISO7816_SendAPDU (&tSC);

}

