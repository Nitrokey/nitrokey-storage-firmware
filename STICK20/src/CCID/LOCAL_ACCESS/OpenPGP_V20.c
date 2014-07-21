/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 12.09.2010
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

#include <avr32/io.h>
#include "stdio.h"
#include "string.h"
#include "compiler.h"
#include "board.h"
#include "power_clocks_lib.h"
#include "gpio.h"
#include "usart.h"
#include "aes.h"
#include "time.h"

#include "tools.h"
#include "TIME_MEASURING.h"
#include "Interpreter.h"
#include "CCID/USART/ISO7816_USART.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"
#include "USB_CCID/USB_CCID.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

//#define DEBUG_OPENPGP
//#define DEBUG_OPENPGP_SHOW_CALLS

#ifndef DEBUG_OPENPGP
  #define CI_LocalPrintfDebug(...)
#else
  #define CI_LocalPrintfDebug(...)    CI_LocalPrintf(...)
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

typeAPDU 	tSC_OpenPGP_V20;



/*******************************************************************************

  LA_OpenPGP_V20_ChangeReset1

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int LA_OpenPGP_V20_ChangeReset1 (typeAPDU *tSC)
{
  int     nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call Reset1\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }

// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x44;
  tSC->tAPDU.cP1      = 0x00;
  tSC->tAPDU.cP2      = 0x00;
  tSC->tAPDU.nLc      = 0;

  nRet = ISO7816_SendAPDU_NoLe_NoLc (tSC);

  // Check for error
  if ((0x90 != tSC->tState.cSW1) && (0x90 != tSC->tState.cSW2))
  {
    nRet = FALSE;
  }

  return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_ChangeReset2

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int LA_OpenPGP_V20_ChangeReset2 (typeAPDU *tSC)
{
  int     nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call Reset2\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0xe6;
  tSC->tAPDU.cP1      = 0x00;
  tSC->tAPDU.cP2      = 0x00;
  tSC->tAPDU.nLc      = 0;

  nRet = ISO7816_SendAPDU_NoLe_NoLc (tSC);

  // Check for error
  if ((0x90 != tSC->tState.cSW1) && (0x90 != tSC->tState.cSW2))
  {
    nRet = FALSE;
  }

  return (nRet);
}
/*******************************************************************************

  LA_OpenPGP_V20_SelectFile

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_SelectFile (typeAPDU *tSC)
{
	int 		nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
	CI_TickLocalPrintf ("ISO7816: Call SelectFile\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }

/*
	SELECT FILE - so übernehmen
	00 00 0B
	gpg: DBG: send apdu: c=00 i=A4 p1=04 p2=00 lc=6 le=-1 em=0
	gpg: DBG:   PCSC_data: 00 A4 04 00 06 D2 76 00 01 24 01
	gpg: DBG:  response: sw=9000  deadline=0
	gpg: DBG:     dump:
	2D
*/

// Command
	tSC->tAPDU.cCLA     = 0x00;
	tSC->tAPDU.cINS     = 0xA4;
	tSC->tAPDU.cP1      = 0x04;
	tSC->tAPDU.cP2      = 0x00;
	tSC->nSendDataLen   = 6;
	tSC->tAPDU.nLc      = tSC->nSendDataLen;

// Send data
	tSC->tAPDU.cData[0] = 0xD2;
	tSC->tAPDU.cData[1] = 0x76;
	tSC->tAPDU.cData[2] = 0x00;
	tSC->tAPDU.cData[3] = 0x01;
	tSC->tAPDU.cData[4] = 0x24;
	tSC->tAPDU.cData[5] = 0x01;

// Receive data
	tSC->tAPDU.nLe      = 0;

	nRet = ISO7816_SendAPDU_Le_Lc (tSC); // Todo _NoLe_NoLc

	return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_GetData

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_GetData (typeAPDU *tSC, unsigned char cP1, unsigned char cP2)
{
	int 		nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
	CI_TickLocalPrintf ("ISO7816: Call GetData\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
/*
	SELECT FILE - so übernehmen
	00 00 0B
	gpg: DBG: send apdu: c=00 i=A4 p1=04 p2=00 lc=6 le=-1 em=0
	gpg: DBG:   PCSC_data: 00 A4 04 00 06 D2 76 00 01 24 01
	gpg: DBG:  response: sw=9000  deadline=0
	gpg: DBG:     dump:
	2D
*/

// Command
	tSC->tAPDU.cCLA     = 0x00;
	tSC->tAPDU.cINS     = 0xCA;
	tSC->tAPDU.cP1      = cP1;
	tSC->tAPDU.cP2      = cP2;
	tSC->nSendDataLen   = 0;
	tSC->tAPDU.nLc      = tSC->nSendDataLen;

// Receive data - length unknown
	tSC->tAPDU.nLe      = 0;

	nRet = ISO7816_SendAPDU_Le_NoLc (tSC);

	return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_Verify

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Verify (typeAPDU *tSC, unsigned char cPW,unsigned char cPasswordLen,unsigned char *pcPassword)
{
	int 		nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
	CI_TickLocalPrintf ("ISO7816: Call Verify %d\r\n",cPW);
#endif

// 	Correct PW level ?
	if ((0 != cPW) && (3 < cPW))
	{
		return (FALSE);
	}

// 	Correct length ?
  if (0 == cPasswordLen)
  {
    return (FALSE);
  }
	if (ISO7816_MAX_APDU_DATA+ISO7816_APDU_SEND_HEADER_LEN+ISO7816_APDU_OFERHEAD <= cPasswordLen)
	{
		return (FALSE);
	}

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
	tSC->tAPDU.cCLA     = 0x00;
	tSC->tAPDU.cINS     = 0x20;
	tSC->tAPDU.cP1      = 0x00;
	tSC->tAPDU.cP2      = 0x80 + cPW;

// Send password
	tSC->tAPDU.nLc      = cPasswordLen;
	memcpy (tSC->tAPDU.cData,pcPassword,cPasswordLen);

// Nothing to receive
	tSC->tAPDU.nLe      = 0;

	nRet = ISO7816_SendAPDU_NoLe_Lc (tSC); // Todo _NoLe_NoLc

	// Check for error
  if ((0x90 != tSC->tState.cSW1) && (0x90 != tSC->tState.cSW2))
  {
    nRet = FALSE;
  }

	return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_GetChallenge

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_GetChallenge (typeAPDU *tSC, int nReceiveLength, unsigned char *cReceiveData)
{
  int     nRet;
  int     n;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call GetChallenge\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x84;
  tSC->tAPDU.cP1      = 0x00;
  tSC->tAPDU.cP2      = 0x00;

// Send data
  tSC->tAPDU.nLc      = 0;
  tSC->tAPDU.cData[0] = 0;

// Something to receive
  tSC->tAPDU.nLe      = nReceiveLength; // nReceiveLength;

  nRet = ISO7816_SendAPDU_Le_NoLc (tSC);  // Todo _NoLe_NoLc

  n = tSC->nReceiveDataLen;
  if (n < tSC->nReceiveDataLen)
  {
    n = nReceiveLength;
  }

  memcpy (cReceiveData,tSC->cReceiveData,n);

  return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_GetPublicKey

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_GetPublicKey (typeAPDU *tSC,int nKind, int nReceiveLength, unsigned char **cReceiveData)
{
  int     nRet;
  int     n;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call GetPublicKey\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x47;
  tSC->tAPDU.cP1      = 0x81;
  tSC->tAPDU.cP2      = 0x00;

// Send data
  tSC->tAPDU.nLc      = 2;

  switch (nKind)
  {
    case 0 :
      tSC->tAPDU.cData[0] = 0xb6;   // Digital signature
      tSC->tAPDU.cData[1] = 0x00;
      break;
    case 1 :
      tSC->tAPDU.cData[0] = 0xb8;   // Confidentiality
      tSC->tAPDU.cData[1] = 0x00;
      break;
    case 2 :
      tSC->tAPDU.cData[0] = 0xA4;   // Authentication
      tSC->tAPDU.cData[1] = 0x00;
      break;
    default :
      tSC->tAPDU.cData[0] = 0xb6;   // Digital signature
      tSC->tAPDU.cData[1] = 0x00;
      break;
  }

// Something to receive
  tSC->tAPDU.nLe      = 0;

  nRet = ISO7816_SendAPDU_Le_Lc (tSC); // Todo _NoLe_NoLc

  n = tSC->nReceiveDataLen;
  if (n < tSC->nReceiveDataLen)
  {
    n = nReceiveLength;
  }


  *cReceiveData = tSC->cReceiveData;


// Check for error
  if ((0x90 != tSC->tState.cSW1) && (0x90 != tSC->tState.cSW2))
  {
    nRet = FALSE;
  }

  return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_ComputeSignature

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_ComputeSignature (typeAPDU *tSC,int nSendLength,unsigned char *cSendData, int nReceiveLength, unsigned char **cReceiveData)
{
  int     nRet;
  int     n;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call ComputeSignature\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x2A;
  tSC->tAPDU.cP1      = 0x9E;
  tSC->tAPDU.cP2      = 0x9A;

// Send data
  tSC->tAPDU.nLc      = nReceiveLength;

  memcpy (&tSC->tAPDU.cData,cSendData,nSendLength);

// Something to receive
  tSC->tAPDU.nLe      = nReceiveLength;

  nRet = ISO7816_SendAPDU_Le_Lc (tSC); // Todo _NoLe_NoLc

  n = tSC->nReceiveDataLen;
  if (n < tSC->nReceiveDataLen)
  {
    n = nReceiveLength;
  }

  *cReceiveData = tSC->cReceiveData;

// Check for error
  if ((0x90 != tSC->tState.cSW1) && (0x90 != tSC->tState.cSW2))
  {
    nRet = FALSE;
  }

  return (nRet);
}


/*******************************************************************************

  LA_OpenPGP_V20_Decipher

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Decipher (typeAPDU *tSC, int nSendLength,unsigned char *cSendData,int nReceiveLength, unsigned char *cReceiveData)
{
  int     nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call Decipher\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x2A;
  tSC->tAPDU.cP1      = 0x80;
  tSC->tAPDU.cP2      = 0x86;

// Send cryptogram
  tSC->tAPDU.nLc      = nSendLength+1;
  tSC->tAPDU.cData[0] = 0;

  memcpy (&tSC->tAPDU.cData[1],cSendData,nSendLength);

// Something to receive
  tSC->tAPDU.nLe      = 0; // 2048; // nReceiveLength;

  nRet = ISO7816_SendAPDU_Le_Lc (tSC); // Todo _NoLe_NoLc
  return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_Put_AES_key

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Put_AES_key (typeAPDU *tSC,unsigned char cKeyLen,unsigned char *pcAES_Key)
{
  int     nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call Put_AES_key\r\n");
#endif

//  Correct key len ?
  if ((16 != cKeyLen) && (32 != cKeyLen))
  {
    return (FALSE);
  }

//  Correct buffer length ?
  if (ISO7816_MAX_APDU_DATA+ISO7816_APDU_SEND_HEADER_LEN+ISO7816_APDU_OFERHEAD <= cKeyLen)
  {
    return (FALSE);
  }

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0xDA;
  tSC->tAPDU.cP1      = 0x00;
  tSC->tAPDU.cP2      = 0xD5;

// Send password
  tSC->tAPDU.nLc      = cKeyLen;
  memcpy (tSC->tAPDU.cData,pcAES_Key,cKeyLen);

// Nothing to receive
  tSC->tAPDU.nLe      = 0;

  nRet = ISO7816_SendAPDU_NoLe_Lc (tSC); // Todo _NoLe_NoLc

  return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_AES_Enc

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_AES_Enc (typeAPDU *tSC, int nSendLength,unsigned char *cSendData,int nReceiveLength, unsigned char *cReceiveData)
{
  int     nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call AES_Enc\r\n");
#endif

//  Correct key len ?
  if ((16 != nSendLength) && (32 != nSendLength))
  {
    return (FALSE);
  }

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }

// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x2A;
  tSC->tAPDU.cP1      = 0x80;
  tSC->tAPDU.cP2      = 0x86;

// Send cryptogram
  tSC->tAPDU.nLc      = nSendLength+1;
  tSC->tAPDU.cData[0] = 0x02;           // AES key

  memcpy (&tSC->tAPDU.cData[1],cSendData,nSendLength);

// Something to receive
  tSC->tAPDU.nLe      = 0;  // nReceiveLength;

  nRet = ISO7816_SendAPDU_Le_Lc (tSC); // Todo _NoLe_NoLc
  return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_AES_Dec_SUB

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_AES_Dec_SUB (typeAPDU *tSC, int nSendLength,unsigned char *cSendData,int nReceiveLength, unsigned char *cReceiveData)
{
  int     nRet;
  int     n;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call AES_Dec_SUB\r\n");
#endif

//  Correct key len ?
  if ((16 != nSendLength) && (32 != nSendLength))
  {
    return (FALSE);
  }

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }

// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x2A;
  tSC->tAPDU.cP1      = 0x80;
  tSC->tAPDU.cP2      = 0x86;

// Send cryptogram
  tSC->tAPDU.nLc      = nSendLength+1;
  tSC->tAPDU.cData[0] = 0x02;           // AES key

  memcpy (&tSC->tAPDU.cData[1],cSendData,nSendLength);

// Something to receive
  tSC->tAPDU.nLe      = nSendLength; // nReceiveLength;

  nRet = ISO7816_SendAPDU_Le_Lc (tSC); // Todo _NoLe_NoLc

  n = tSC->nReceiveDataLen;
  if (n < tSC->nReceiveDataLen)
  {
    n = nReceiveLength;
  }

  memcpy (cReceiveData,tSC->cReceiveData,n);

  return (nRet);
}


/*******************************************************************************

  LA_OpenPGP_V20_ChangeUserPin

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_ChangeUserPin (typeAPDU *tSC, int nSendLength,unsigned char *cSendData)
{
  int     nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call ChangeUserPin\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x24;
  tSC->tAPDU.cP1      = 0x00;
  tSC->tAPDU.cP2      = 0x81;

// Send data
  tSC->tAPDU.nLc      = nSendLength;
  memcpy (&tSC->tAPDU.cData,cSendData,nSendLength);

  nRet = ISO7816_SendAPDU_NoLe_Lc (tSC);

  // Check for error
  if ((0x90 != tSC->tState.cSW1) && (0x90 != tSC->tState.cSW2))
  {
    nRet = FALSE;
  }

  return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_ChangeAdminPin

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_ChangeAdminPin (typeAPDU *tSC, int nSendLength,unsigned char *cSendData)
{
  int     nRet;

#ifdef DEBUG_OPENPGP_SHOW_CALLS
  CI_TickLocalPrintf ("ISO7816: Call ChangeAdminPin\r\n");
#endif

  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }
// Command
  tSC->tAPDU.cCLA     = 0x00;
  tSC->tAPDU.cINS     = 0x24;
  tSC->tAPDU.cP1      = 0x00;
  tSC->tAPDU.cP2      = 0x83;

// Send data
  tSC->tAPDU.nLc      = nSendLength;
  memcpy (&tSC->tAPDU.cData,cSendData,nSendLength);

  nRet = ISO7816_SendAPDU_NoLe_Lc (tSC);

  // Check for error
  if ((0x90 != tSC->tState.cSW1) && (0x90 != tSC->tState.cSW2))
  {
    nRet = FALSE;
  }

  return (nRet);
}

/*******************************************************************************

  LA_OpenPGP_V20_AES_Dec

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_AES_Dec (typeAPDU *tSC, int nSendLength,unsigned char *cSendData,int nReceiveLength, unsigned char *cReceiveData)
{
  int     nRet;

//  128 bit key ?
  if (16 == nSendLength)
  {
    nRet = LA_OpenPGP_V20_AES_Dec_SUB (tSC,nSendLength,cSendData,nReceiveLength,cReceiveData);

    if ((0x90 != tSC->tState.cSW1) || (0x00 != tSC->tState.cSW2))   // Sc send no ok
    {
      return (FALSE);
    }

    return (nRet);
  }


//  256 bit key ?
  if (32 == nSendLength)
  {
    // Decrypt first 16 Byte
    nRet = LA_OpenPGP_V20_AES_Dec_SUB (tSC,16,cSendData,16,cReceiveData);

    if ((0x90 != tSC->tState.cSW1) || (0x00 != tSC->tState.cSW2)) // Sc send no ok
    {
      return (FALSE);
    }

    if (FALSE == nRet)
    {
      return (FALSE);
    }

    // Decrypt second 16 Byte
    nRet = LA_OpenPGP_V20_AES_Dec_SUB (tSC,16,&cSendData[16],16,&cReceiveData[16]);

    if ((0x90 != tSC->tState.cSW1) || (0x00 != tSC->tState.cSW2)) // Sc send no ok
    {
      return (FALSE);
    }
    return (nRet);
  }

  return (FALSE);
}



/*******************************************************************************

  LA_OpenPGP_V20_Test_AES_Key

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#define AES_KEY_SIZE_IN_BYTE   32     // 16 = 128 Bit Achtung in AES_StorageKeyEncryption aes size anpassen

unsigned char acAES_MasterKey[32+1]  = "01234567890123456789012345678901";
unsigned char acAES_StorageKey[32+1] = "11111111111111111111111111111111";
unsigned char acBuffer[32];
unsigned char acBufferOut[32];

/*******************************************************************************

  LA_OpenPGP_V20_Test_ChangeUserPin

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/
#define MAX_PIN_STRING  60

int LA_OpenPGP_V20_Test_ChangeUserPin (unsigned char *pcOldPin,unsigned char *pcNewPin)
{
  int nRet;
  int n;
  char acPinString[MAX_PIN_STRING+1];

  CI_LocalPrintf ("Change user pin  : old -%s- > new -%s-\r\n",pcOldPin,pcNewPin);

  n  = strlen ((char *)pcOldPin);
  n += strlen ((char *)pcNewPin);

  if (MAX_PIN_STRING <= n)
  {
    CI_LocalPrintf ("Change user pin  :MAX_PIN_STRING to small %d\r\n",n);
    return (FALSE);
  }

  strcpy (acPinString,(char *)pcOldPin);
  strcat (acPinString,(char *)pcNewPin);

  nRet = LA_OpenPGP_V20_ChangeUserPin (&tSC_OpenPGP_V20,n,(unsigned char *)acPinString);
  if (FALSE == nRet)
  {
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }
  CI_LocalPrintf ("OK \n\r");

  return (TRUE);
}

/*******************************************************************************

  LA_OpenPGP_V20_Test_ChangeAdminPin

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test_ChangeAdminPin (unsigned char *pcOldPin,unsigned char *pcNewPin)
{
  int nRet;
  int n;
  char acPinString[MAX_PIN_STRING+1];

  CI_LocalPrintf ("Change user pin  : old -%s- > new -%d-\r\n",pcOldPin,pcNewPin);

  n  = strlen ((char *)pcOldPin);
  n += strlen ((char *)pcNewPin);

  if (MAX_PIN_STRING <= n)
  {
    CI_LocalPrintf ("Change user pin  :MAX_PIN_STRING to small %d\r\n",n);
    return (FALSE);
  }

  strcpy (acPinString,(char *)pcOldPin);
  strcat (acPinString,(char *)pcNewPin);

  nRet = LA_OpenPGP_V20_ChangeAdminPin (&tSC_OpenPGP_V20,n,(unsigned char *)acPinString);
  if (FALSE == nRet)
  {
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }
  CI_LocalPrintf ("OK \n\r");

  return (TRUE);
}
/*******************************************************************************

  LA_OpenPGP_V20_Test_GetAID

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test_GetAID (void)
{
  int nRet;

  CI_LocalPrintf ("Get AID  : ");
  nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x4F);
  if (FALSE == nRet)
  {
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }
  CI_LocalPrintf ("OK \n\r");

  return (TRUE);
}

/*******************************************************************************

  LA_OpenPGP_V20_GetPasswordstatus

  Changes
  Date      Author          Info
  31.03.14  RB              Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

int LA_OpenPGP_V20_GetPasswordstatus (char *PasswordStatus)
{
  int nRet;

//  memset (PasswordStatus,0,7);

  CI_LocalPrintf ("Get password status  : ");

  nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0xC4);
  if (FALSE == nRet)
  {
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }
  CI_LocalPrintf ("OK  P %d - A %d\n\r",tSC_OpenPGP_V20.cReceiveData[4],tSC_OpenPGP_V20.cReceiveData[6]);

  // Copy password status
  memcpy (PasswordStatus,tSC_OpenPGP_V20.cReceiveData,7);

  return (TRUE);
}


/*******************************************************************************

  LA_OpenPGP_V20_GetAID

  Changes
  Date      Author          Info
  02.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

int LA_OpenPGP_V20_GetAID (char *AID)
{
  int Ret_u32;

  CI_LocalPrintf ("Get AID  : ");
  Ret_u32 = LA_OpenPGP_V20_SelectFile (&tSC_OpenPGP_V20);
  if (FALSE == Ret_u32)
  {
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }
  CI_LocalPrintf ("OK\n\r");

  // Copy password status
  memcpy (AID,&tSC_OpenPGP_V20.cReceiveData[4],16);

  return (TRUE);
}


/*******************************************************************************

  LA_OpenPGP_V20_Test_SendAdminPW

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test_SendAdminPW (unsigned char *pcPW)
{
  int nRet;
  int n;

  n = strlen ((char *)pcPW);
  CI_LocalPrintf ("Send admin password : %s ",pcPW);

  nRet = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,3,n,pcPW);
  if (FALSE == nRet)
  {
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }
  CI_LocalPrintf ("OK\n\r");
  return (TRUE);
}
/*******************************************************************************

  LA_OpenPGP_V20_Test_SendUserPW2

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test_SendUserPW2 (unsigned char *pcPW)
{
  int nRet;
  int n;

  n = strlen ((char *)pcPW);

  CI_LocalPrintf ("Send user password  : ");
  nRet = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,2,n,pcPW);
  if (FALSE == nRet)
  {
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }
  CI_LocalPrintf ("OK \n\r");

  return (TRUE);
}
/*******************************************************************************

  LA_OpenPGP_V20_Test_SendAESMasterKey

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test_SendAESMasterKey (int nLen,unsigned char *pcMasterKey)
{
  int nRet;

  CI_LocalPrintf ("Put AES master key     : ");
  HexPrint (nLen,pcMasterKey);
  CI_LocalPrintf ("\r\n");

  nRet = LA_OpenPGP_V20_Put_AES_key (&tSC_OpenPGP_V20,nLen,pcMasterKey);

  if (FALSE == nRet)
  {
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }

  CI_LocalPrintf ("OK \n\r");
  return (TRUE);
}
/*******************************************************************************

  LA_OpenPGP_V20_Test_LocalAESKey

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test_LocalAESKey (void)
{
/* Local AES key test */
  CI_LocalPrintf ("\n\rTest local AES engine\r\n");

  memcpy (acBuffer,acAES_StorageKey,AES_KEY_SIZE_IN_BYTE);

  CI_LocalPrintf ("Local Unverschlüsselter Key : ");
  HexPrint (AES_KEY_SIZE_IN_BYTE,acBuffer);
  CI_LocalPrintf ("\r\n");

  AES_StorageKeyEncryption (AES_KEY_SIZE_IN_BYTE, acBuffer, acAES_MasterKey, AES_PMODE_CIPHER);

  CI_LocalPrintf ("Local Verschlüsselter Key   : ");
  HexPrint (AES_KEY_SIZE_IN_BYTE,acBuffer);
  CI_LocalPrintf ("\r\n");

  memcpy (acBufferOut,acBuffer,AES_KEY_SIZE_IN_BYTE);

  AES_StorageKeyEncryption (AES_KEY_SIZE_IN_BYTE, acBufferOut, acAES_MasterKey, AES_PMODE_DECIPHER);

  CI_LocalPrintf ("Local Entschlüsselter Key   : ");
  HexPrint (AES_KEY_SIZE_IN_BYTE,acBufferOut);
  CI_LocalPrintf ("\r\n");
  CI_LocalPrintf ("\r\n");

  return (TRUE);
}
/*******************************************************************************

  LA_OpenPGP_V20_Test_ScAESKey

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test_ScAESKey (int nLen,unsigned char *pcKey)
{
  int nRet;

  CI_LocalPrintf ("Encrypted AES key  : ");
  HexPrint (nLen,pcKey);
  CI_LocalPrintf ("\r\n");

  if (32 < nLen)
  {
    CI_LocalPrintf ("len fail\n\r");
    return (FALSE);
  }

  memset (acBufferOut,0,nLen);

  nRet = LA_OpenPGP_V20_AES_Dec (&tSC_OpenPGP_V20,nLen,pcKey,nLen,acBufferOut);
  if (TRUE == nRet)
  {
    CI_LocalPrintf ("Decrypted AES key  : ");
    HexPrint (nLen,acBufferOut);
    CI_LocalPrintf ("\r\n");
  }
  else
  {
    memset (pcKey,0,nLen);
    CI_LocalPrintf ("fail\n\r");
    return (FALSE);
  }

  memcpy (pcKey,acBufferOut,nLen);

  return (TRUE);
}
/*******************************************************************************

  LA_OpenPGP_V20_Test_AES_Key

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test_AES_Key (void)
{

  if (FALSE == LA_OpenPGP_V20_Test_SendAdminPW ((unsigned char *)"12345678"))
  {
    return (FALSE);
  }

  if (FALSE == LA_OpenPGP_V20_Test_SendAESMasterKey (AES_KEY_SIZE_IN_BYTE,acAES_MasterKey))
  {
    return (FALSE);
  }
//return (TRUE);

/* Local AES key test */
  LA_OpenPGP_V20_Test_LocalAESKey ();


/* SC AES key decryption */
  if (FALSE == LA_OpenPGP_V20_Test_SendUserPW2 ((unsigned char *)"123456"))
  {
    return (FALSE);
  }

//  CI_LocalPrintf ("Test smartcard AES encryption\r\n");

  if (FALSE == LA_OpenPGP_V20_Test_ScAESKey (AES_KEY_SIZE_IN_BYTE,acBuffer))
  {
    return (FALSE);
  }

  return (TRUE);

}

/*******************************************************************************

  LA_OpenPGP_V20_ResetCard

  Changes
  Date      Author          Info
  15.07.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int LA_OpenPGP_V20_ResetCard (void)
{
  int i;
  int nRet;

// Disable user password
  for (i=0;i<4;i++)
  {
    nRet = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,1,8,(unsigned char*)"@@@@@@@@");
    if (FALSE == nRet)
    {
    }
  }

// Disable admin password
  for (i=0;i<4;i++)
  {
    nRet = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,3,8,(unsigned char*)"@@@@@@@@");
    if (FALSE == nRet)
    {
    }
  }

  nRet = LA_OpenPGP_V20_ChangeReset1 (&tSC_OpenPGP_V20);
  if (FALSE == nRet)
  {
  }

  nRet = LA_OpenPGP_V20_ChangeReset2 (&tSC_OpenPGP_V20);
  if (FALSE == nRet)
  {
  }

}


/*******************************************************************************

  LA_OpenPGP_V20_Test_AES_Key

*******************************************************************************/

/*******************************************************************************

  GetRandomNumber_u32

  Changes
  Date      Author          Info
  03.06.14  RB              XOR random number with random number from second source

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

u32 GetRandomNumber_u32 (u32 Size_u32,u8 *Data_pu8)
{
  u32 Ret_u32;
  u32 i;
  time_t  now;

  // Size ok ?
  if (ISO7816_APDU_MAX_RESPONSE_LEN <= Size_u32)
  {
    return (FALSE);
  }

  // Get a random number from smartcard
  Ret_u32 = LA_OpenPGP_V20_GetChallenge (&tSC_OpenPGP_V20,Size_u32,Data_pu8);


#ifdef GENERATE_RANDOM_NUMBER_WITH_2ND_SOURCE
  // Paranoia: if the random number is not really random, xor it with another random number from a second source
  time (&now);                      // Get the local time
  srand (now + Data_pu8[0]);        // Init the local random generator
  for (i=0;i<Size_u32;i++)
  {
    Data_pu8[i] = Data_pu8[i] ^ (u8)(rand () % 256);
  }
#endif
  return (Ret_u32);
}




#define OPENPGP_V20_TEST_C_LENGTH_OLD 128

unsigned char cCryptogramTest_old[OPENPGP_V20_TEST_C_LENGTH_OLD] =
{
	0x7D,0xFC,0xDC,0xDC,0xC2,0x9E,0xFD,0x28,0xC1,0xA4,
	0x83,0xAB,0xD3,0xC7,0x54,0x61,0xB7,0x2E,0x12,0x82,
	0x02,0xBC,0x3F,0x8D,0xFB,0xC8,0x43,0xE8,0xBA,0xD4,
	0x10,0xF0,0xCD,0x49,0x89,0x43,0x92,0xCD,0x8C,0x39,
	0xF4,0xD2,0x00,0xB2,0x9F,0xD3,0x47,0x31,0x09,0x19,
	0x55,0x66,0x03,0x3F,0xCE,0xE7,0xC6,0xE4,0xE2,0x57,
	0x64,0x65,0x7E,0xDE,0xC1,0x66,0xAB,0x0B,0xB0,0x42,
	0x6C,0x72,0x01,0xD3,0xBB,0x79,0x47,0x8B,0x09,0x39,
	0xEA,0xEB,0xCB,0xAB,0x6A,0xBD,0x45,0x8C,0x99,0xE9,
	0xA4,0xE1,0x26,0x3C,0x9E,0x30,0x7A,0x0F,0x4E,0xFF,
	0x19,0x2D,0x64,0x70,0xB0,0x6A,0x1D,0xE4,0x76,0x6B,
	0xF5,0xF6,0xD2,0x2F,0x1E,0xDD,0xE5,0x01,0x39,0x2E,
	0xBF,0x31,0x0C,0xD3,0x9C,0xA0,0x60,0x14
};


#define OPENPGP_V20_TEST_C_LENGTH 256

unsigned char cCryptogramTest[OPENPGP_V20_TEST_C_LENGTH] =
{
	0x53 ,0x82 ,0xC6 ,0xE5 ,0x8D ,0xEF ,0x3D ,0x63 ,0x37 ,0x46,
	0x52 ,0x40 ,0x62 ,0x19 ,0xE0 ,0x54 ,0x5E ,0x8F ,0x05 ,0x86,
	0x5E ,0x0E ,0x9E ,0x3B ,0x07 ,0x1A ,0x14 ,0xC9 ,0x0E ,0x9E,
	0xC2 ,0x2B ,0xB5 ,0x89 ,0xD2 ,0xFA ,0x65 ,0x32 ,0x3D ,0x81,
	0xF8 ,0xC7 ,0x2D ,0x31 ,0x10 ,0x9F ,0x85 ,0x69 ,0x8D ,0xCA,

	0xF9 ,0xFA ,0x5A ,0xF5 ,0x71 ,0xEE ,0x49 ,0x36 ,0x94 ,0x9F,
	0x72 ,0x2C ,0x3C ,0x63 ,0xD6 ,0x11 ,0x69 ,0x5E ,0x96 ,0x18,
	0xAC ,0x5D ,0x5D ,0x58 ,0xFB ,0xF6 ,0xE9 ,0xCA ,0xA5 ,0xEA,
	0x55 ,0x35 ,0x36 ,0x8A ,0x66 ,0xDB ,0xB9 ,0x0E ,0x89 ,0xF9,
	0x11 ,0x98 ,0x94 ,0xAE ,0x2C ,0xA7 ,0x90 ,0x55 ,0xE2 ,0xA4,

	0xBA ,0x49 ,0x32 ,0x28 ,0xE8 ,0x05 ,0x85 ,0xE3 ,0x1D ,0xFD,
	0x5C ,0x21 ,0x17 ,0xF2 ,0x6D ,0xBE ,0xBC ,0x3E ,0x82 ,0x4B,
	0xE1 ,0xA1 ,0x64 ,0x3B ,0x8B ,0xDC ,0x40 ,0xC0 ,0xCD ,0xD1,
	0x5D ,0x16 ,0xB7 ,0x19 ,0x78 ,0xDC ,0xB2 ,0x70 ,0x0F ,0xA3,
	0x91 ,0x28 ,0x8C ,0xF1 ,0x48 ,0x4E ,0x14 ,0x9B ,0xB8 ,0xD2,

	0x7F ,0xDA ,0x43 ,0x8C ,0xA3 ,0xC7 ,0x09 ,0x0A ,0xDF ,0xC1,
	0xCC ,0xBC ,0xE2 ,0x46 ,0x74 ,0xDD ,0x88 ,0x1E ,0x08 ,0xC2,
	0x57 ,0x1B ,0x5C ,0x88 ,0x34 ,0x5D ,0x20 ,0x20 ,0xF0 ,0x59,
	0xED ,0x05 ,0x6E ,0x07 ,0x08 ,0xBD ,0xFB ,0x03 ,0x8E ,0xBD,
	0x8A ,0x57 ,0x5D ,0xC3 ,0x4D ,0xBD ,0x94 ,0xB2 ,0x17 ,0x5D,

	0x4F ,0x25 ,0xA5 ,0x11 ,0xE7 ,0xFD ,0x9C ,0x36 ,0x42 ,0x68,
	0x1B ,0x3B ,0x69 ,0xF5 ,0x6A ,0x96 ,0x5C ,0x9E ,0x12 ,0x3F,
	0x51 ,0x0E ,0xF9 ,0x2F ,0xB5 ,0x6C ,0x36 ,0x1C ,0x70 ,0xBB,
	0xCC ,0xC0 ,0x1D ,0xAD ,0xBE ,0xB3 ,0xC5 ,0x9A ,0xD5 ,0xA3,
	0xCB ,0x64 ,0xB4 ,0x21 ,0x0D ,0xF2 ,0x78 ,0x23 ,0x95 ,0xC7,

	0xB7 ,0xCB ,0x23 ,0x5A ,0xD1 ,0xA3
};

/*******************************************************************************

  LA_OpenPGP_V20_Test

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int LA_OpenPGP_V20_Test (void)
{
	int nRet;
	unsigned char cBuffer[OPENPGP_V20_TEST_C_LENGTH];

	ISO7816_InitSC ();

	nRet = LA_OpenPGP_V20_SelectFile (&tSC_OpenPGP_V20);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 00 4F - Application identifier (AID), 16 (dec.) bytes (ISO 7816-4)
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x4F);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 5F 52 - Historical bytes, up to 15 bytes (dec.), according to ISO 7816-4. Card capabilities shall be included
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x5F,0x52);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 00 C4 - PW Status Bytes
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0xC4);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 00 5E - Login data
/*
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x5E);
	if (FALSE == nRet)
	{
		return (FALSE);
	}
*/
// GETDATA 00 6E - Application Related Data
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x6E);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 00 6E - Application Related Data
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x6E);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 00 6E - Application Related Data
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x6E);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 00 6E - Application Related Data
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x6E);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 00 6E - Application Related Data
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x6E);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// GETDATA 00 6E - Application Related Data
	nRet = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x6E);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

/**/
// VERIFY PIN 1

	nRet = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,1,6,(unsigned char*)"123456");
	if (FALSE == nRet)
	{
		return (FALSE);
	}

// VERIFY PIN 2
	nRet = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,2,6,(unsigned char*)"123456");
	if (FALSE == nRet)
	{
		return (FALSE);
	}

	LA_OpenPGP_V20_Decipher (&tSC_OpenPGP_V20,OPENPGP_V20_TEST_C_LENGTH,cCryptogramTest,OPENPGP_V20_TEST_C_LENGTH,cBuffer);
	if (FALSE == nRet)
	{
		return (FALSE);
	}

	LA_OpenPGP_V20_Decipher (&tSC_OpenPGP_V20,OPENPGP_V20_TEST_C_LENGTH,cCryptogramTest,OPENPGP_V20_TEST_C_LENGTH,cBuffer);
	if (FALSE == nRet)
	{
		return (FALSE);
	}
/**/
	return (nRet);

}

/*******************************************************************************

  LA_SC_StartSmartcard

  Changes
  Date      Reviewer        Info
  14.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 LA_SC_StartSmartcard (void)
{
  u32 Ret_u32;

//  LA_RestartSmartcard_u8 ();

// Check for smartcard on
  if (CCID_SLOT_STATUS_PRESENT_ACTIVE == CCID_GetSlotStatus_u8 ())
  {
    return (TRUE);    // Smartcard is on
  }

  CI_TickLocalPrintf ("LA_SC_StartSmartcard: Start smartcard\r\n");
  LA_RestartSmartcard_u8 ();
  if (CCID_SLOT_STATUS_PRESENT_ACTIVE == CCID_GetSlotStatus_u8 ())
  {
    return (TRUE);
  }

  CI_TickLocalPrintf ("LA_SC_StartSmartcard: Restart smartcard\r\n");
  LA_RestartSmartcard_u8 ();
  if (CCID_SLOT_STATUS_PRESENT_ACTIVE == CCID_GetSlotStatus_u8 ())
  {
    return (TRUE);
  }

  CI_TickLocalPrintf ("LA_SC_StartSmartcard: Can't start smartcard\r\n");
  return (FALSE);
}

/*******************************************************************************

  LA_SC_SendVerify

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 LA_SC_SendVerify (u8 PasswordFlag_u8,u8 *String_pu8)
{
  u32 Ret_u32;

// Check for smartcard on
  if (FALSE == LA_SC_StartSmartcard ())
  {
    return (FALSE);
  }

  CI_TickLocalPrintf ("LA_SC_SendVerify: %d -%s-\r\n",PasswordFlag_u8,String_pu8);

  Ret_u32 = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,PasswordFlag_u8,strlen ((char *)String_pu8),String_pu8);
  if (FALSE == Ret_u32)
  {
    CI_TickLocalPrintf ("LA_SC_SendVerify: Error\r\n");
  }

  return (Ret_u32);
}

/*******************************************************************************

  LA_RestartSmartcard_u8

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

u8 LA_RestartSmartcard_u8 (void)
{
  u32 Ret_u32;

  CCID_RestartSmartcard_u8 ();

  Ret_u32 = LA_OpenPGP_V20_SelectFile (&tSC_OpenPGP_V20);
  if (FALSE == Ret_u32)
  {
    CI_LocalPrintf ("ERROR: Select file\r\n");
    return (FALSE);
  }

  DelayMs (10);

  // GETDATA 00 4F - Application identifier (AID), 16 (dec.) bytes (ISO 7816-4)
  Ret_u32 = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x4F);
  if (FALSE == Ret_u32)
  {
    CI_LocalPrintf ("ERROR: GETDATA 00 4F - Application identifier (AID) \r\n");
    return(FALSE);
  }

  DelayMs (10);
  // GETDATA 5F 52 - Historical bytes, up to 15 bytes (dec.), according to ISO 7816-4. Card capabilities shall be included
  Ret_u32 = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x5F,0x52);
  if (FALSE == Ret_u32)
  {
    CI_LocalPrintf ("ERROR: GETDATA 5F 52 - Historical bytes\r\n");
    return(FALSE);
  }

  return (TRUE);
}
/*******************************************************************************

  IBN_SC_Tests

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#define IBN_BUFFER_SIZE  500

int AES_local_test (void);

unsigned char cBuffer[IBN_BUFFER_SIZE];

void IBN_SC_Tests (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8)
{
//  u32   Runtime_u32;
  u32   Ret_u32;
  u32   i;
  u32   nSize;
  u8   *BufferPointer_pu8 = cBuffer;
//  static u8    FlagSCOpen_u8 = FALSE;
  u8    LocalString_au8[50];
  u64   Runtime_u64;


  if (0 == nParamsGet_u8)
  {
    CI_LocalPrintf ("Smartcard test functions\r\n");
    CI_LocalPrintf ("\r\n");
    CI_LocalPrintf ("0          Reset sc, send select file command\r\n");
    CI_LocalPrintf ("1  UserPW1 Enter user1 password\r\n");
    CI_LocalPrintf ("2  UserPW2 Enter user2 password\r\n");
    CI_LocalPrintf ("3  UserAD  Enter admin password\r\n");
    CI_LocalPrintf ("4          Ciphertest\r\n");
    CI_LocalPrintf ("5          Deciphertest\r\n");
    CI_LocalPrintf ("6  Size    Generate random number\r\n");
    CI_LocalPrintf ("7  Kind    Show key:0=Signature,1=Confidentiality,2=Authentication\r\n");
    CI_LocalPrintf ("8          Signature erstellen\r\n");
    CI_LocalPrintf ("9          AES Key test\r\n");
    CI_LocalPrintf ("12         Change user pin old pin+new pin  -> 123456abcdef\r\n");
    CI_LocalPrintf ("13         Change admin pin old pin+new pin -> 12345678abcdefgh\r\n");
    CI_LocalPrintf ("14         SC startup test\r\n");
    CI_LocalPrintf ("15         Get password status\r\n");
    CI_LocalPrintf ("16 count   Generate [count] random numbers a 10 byte\r\n");
    CI_LocalPrintf ("17 count   Generate [count] *10 random numbers [output as 0,1 stream]\r\n");
    CI_LocalPrintf ("18 FiDi    FiDi Test\r\n");
    CI_LocalPrintf ("99         Factory reset smartcard \r\n");
    CI_LocalPrintf ("\r\n");
    return;
  }

  memcpy (LocalString_au8,String_pu8,50);

  switch (CMD_u8)
  {
    case 0 :
          CCID_RestartSmartcard_u8 ();
//          ISO7816_InitSC ();
          ISO7816_T1_InitSendNr ();

          Ret_u32 = LA_OpenPGP_V20_SelectFile (&tSC_OpenPGP_V20);
          if (FALSE == Ret_u32)
          {
            CI_LocalPrintf ("ERROR: Select file\r\n");
            return;
          }
          DelayMs (10);
          // GETDATA 00 4F - Application identifier (AID), 16 (dec.) bytes (ISO 7816-4)
          Ret_u32 = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x4F);
          if (FALSE == Ret_u32)
          {
            CI_LocalPrintf ("ERROR: GETDATA 00 4F - Application identifier (AID) \r\n");
            return;
          }

          DelayMs (10);
          // GETDATA 5F 52 - Historical bytes, up to 15 bytes (dec.), according to ISO 7816-4. Card capabilities shall be included
          Ret_u32 = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x5F,0x52);
          if (FALSE == Ret_u32)
          {
            CI_LocalPrintf ("ERROR: GETDATA 5F 52 - Historical bytes\r\n");
            return;
          }
          break;
    case 1 :
          sprintf (LocalString_au8,"%d",Param_u32);
//          strcpy (LocalString_au8,String_pu8);
          CI_LocalPrintf ("Send password -%s-\r\n",LocalString_au8);

          Ret_u32 = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,1,strlen ((char *)LocalString_au8),LocalString_au8); //6, (unsigned char*)"123456");
//          Ret_u32 = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,1,6,(unsigned char*)"123456");
          if (FALSE == Ret_u32)
          {
            CI_LocalPrintf ("PW Error\r\n");
          }
          break;
    case 2 :
          sprintf (LocalString_au8,"%d",Param_u32);
//          strcpy (LocalString_au8,String_pu8);
          CI_LocalPrintf ("Send password -%s-\r\n",LocalString_au8);
          Ret_u32 = LA_OpenPGP_V20_Test_SendUserPW2 (LocalString_au8);
//          Ret_u32 = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,2,strlen ((char*)LocalString_au8),LocalString_au8);
//          Ret_u32 = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,2,6,(unsigned char*)"123456");
          if (FALSE == Ret_u32)
          {
            CI_LocalPrintf ("PW Error\r\n");
          }
          break;

    case 3 :
          sprintf (LocalString_au8,"%d",Param_u32);
//          sprintf (cBuffer,"%d",Param_u32);
          Ret_u32 = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,3,strlen ((char*)LocalString_au8),LocalString_au8);
//          Ret_u32 = LA_OpenPGP_V20_Verify (&tSC_OpenPGP_V20,3,8,(unsigned char*)"12345678");
          if (FALSE == Ret_u32)
          {
            CI_LocalPrintf ("PW Error\r\n");
          }
          break;
    case 4 :
//          LA_OpenPGP_V20_Cipher (&tSC_OpenPGP_V20,OPENPGP_V20_TEST_C_LENGTH,cCryptogramTest,OPENPGP_V20_TEST_C_LENGTH,cBuffer);
          break;
    case 5 :
          LA_OpenPGP_V20_Decipher (&tSC_OpenPGP_V20,OPENPGP_V20_TEST_C_LENGTH,cCryptogramTest,OPENPGP_V20_TEST_C_LENGTH,cBuffer);
          break;
    case 6 : // Zufallszahl generieren
          nSize = Param_u32;
          if (IBN_BUFFER_SIZE <= nSize)
          {
            nSize = IBN_BUFFER_SIZE;
          }

          Ret_u32 = GetRandomNumber_u32 (nSize,BufferPointer_pu8);
          if (TRUE == Ret_u32)
          {
            CI_LocalPrintf ("Zufallszahl : \r\n");
            HexPrint (nSize,BufferPointer_pu8);
            CI_LocalPrintf ("\r\n");
          }
          else
          {
            CI_LocalPrintf ("Error\r\n");
          }
          break;

    case 7 : // Public key auslesen

          if (3 <= Param_u32)
          {
            CI_LocalPrintf ("7  Kind    Show key:0=Signature,1=Confidentiality,2=Authentication\r\n");
            break;
          }
          Ret_u32 = LA_OpenPGP_V20_GetPublicKey (&tSC_OpenPGP_V20,Param_u32,IBN_BUFFER_SIZE,&BufferPointer_pu8);
          if (TRUE == Ret_u32)
          {
            CI_LocalPrintf ("Key : Length %d\n\r",tSC_OpenPGP_V20.nReceiveDataLen);
            HexPrint (tSC_OpenPGP_V20.nReceiveDataLen,BufferPointer_pu8);
            CI_LocalPrintf ("\r\n");
          }
          else
          {
            CI_LocalPrintf ("Error\r\n");
          }
          break;

    case 8 : // Signature erstellen
          Ret_u32 = LA_OpenPGP_V20_ComputeSignature (&tSC_OpenPGP_V20,10,(unsigned char *)"1234567890",IBN_BUFFER_SIZE,&BufferPointer_pu8);
          if (TRUE == Ret_u32)
          {
            CI_LocalPrintf ("Signature : Length %d\n\r",tSC_OpenPGP_V20.nReceiveDataLen);
            HexPrint (tSC_OpenPGP_V20.nReceiveDataLen,BufferPointer_pu8);
            CI_LocalPrintf ("\r\n");
          }
          else
          {
            CI_LocalPrintf ("Error\r\n");
          }
          break;

    case 9 :
          LA_OpenPGP_V20_Test_AES_Key ();
          break;

    case 10 :
          AES_local_test ();
          break;

    case 11 :
          LA_OpenPGP_V20_Test ();
          break;

    case 12 :
          LA_OpenPGP_V20_Test_ChangeUserPin (String_pu8,(unsigned char *)"");
          break;
    case 13 :
          LA_OpenPGP_V20_Test_ChangeAdminPin (String_pu8,(unsigned char *)"");
          break;
    case 14 :
          ISO7816_SC_StartupTest (Param_u32);
          break;
    case 15 :
          Ret_u32 = LA_OpenPGP_V20_GetPasswordstatus (LocalString_au8);
          if (TRUE == Ret_u32)
          {
            CI_LocalPrintf ("Password status -");
            HexPrint (7,LocalString_au8);
            CI_LocalPrintf ("\n\r");

            CI_LocalPrintf ("User   %d\n\r",LocalString_au8[4]);
            CI_LocalPrintf ("Admin  %d\n\r",LocalString_au8[6]);
          }
          break;
    case 16 : // Zufallszahl Dauertest
          CI_LocalPrintf ("Generate %d times a 10 char random\n",Param_u32);

          for (i=0;i<Param_u32;i++)
          {
            Ret_u32 = GetRandomNumber_u32 (10,BufferPointer_pu8);
            if (TRUE == Ret_u32)
            {
              CI_LocalPrintf ("Zufallszahl %6d :",i);
              HexPrint (10,BufferPointer_pu8);
              CI_LocalPrintf ("\r\n");
            }
            else
            {
              CI_LocalPrintf ("Error\r\n");
            }
            DelayMs (50);
          }
          break;
    case 17 : // Zufallszahl Dauertest
          CI_LocalPrintf ("Generate %d times 10 random chars  [Output as 0,1 stream]\n",Param_u32);

          for (i=0;i<Param_u32;i++)
          {
            Ret_u32 = GetRandomNumber_u32 (10,BufferPointer_pu8);
            if (TRUE == Ret_u32)
            {
              u32 i,i1;

//              HexPrint (10,BufferPointer_pu8);
              for (i=0;i<10;i++)
              {
                for (i1=0;i1<8;i1++)
                {
                  if (((BufferPointer_pu8[i]>>i1) & 1) == 1)
                  {
                    CI_LocalPrintf ("1");
                  }
                  else
                  {
                    CI_LocalPrintf ("0");
                  }
                }
              }
            }
            else
            {
              CI_LocalPrintf ("Error\r\n");
            }

            DelayMs (50);
          }
          CI_LocalPrintf ("\r\n");
          break;
    case 18 :
          CI_LocalPrintf ("Set FiDi to 0x%02x\r\n",Param_u32);

          ISO7816_SetFiDI (Param_u32);

          LA_RestartSmartcard_u8 ();

#ifdef TIME_MEASURING_ENABLE
          Runtime_u64 = TIME_MEASURING_GetTime ();
#endif
          // GETDATA 00 4F - Application identifier (AID), 16 (dec.) bytes (ISO 7816-4)
          Ret_u32 = LA_OpenPGP_V20_GetData (&tSC_OpenPGP_V20,0x00,0x4F);
          if (FALSE == Ret_u32)
          {
            CI_LocalPrintf ("ERROR: GETDATA 00 4F - Application identifier (AID) \r\n");
            return;
          }

#ifdef TIME_MEASURING_ENABLE
          Runtime_u64 = TIME_MEASURING_GetTime () - Runtime_u64;
          CI_LocalPrintf ("SC runtime %ld msec\n\r",(u32)(Runtime_u64/(u64)TIME_MEASURING_TICKS_IN_USEC/(u64)1000L));
#endif

          break;
    case 99 :
        LA_OpenPGP_V20_ResetCard();
        break;

    default :
          break;
  }
}

