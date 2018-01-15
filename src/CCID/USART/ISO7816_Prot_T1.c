/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 09.09.2010
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
 * ISO7816_Prot_T1.c
 *
 *  Created on: 09.09.2010
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

#include "FreeRTOS.h"
#include "global.h"
#include "task.h"
#include "tools.h"
#include "ISO7816_USART.h"
#include "ISO7816_ADPU.h"
#include "ISO7816_Prot_T1.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

// #define DEBUG_LOG_CCID_DETAIL

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

unsigned char ISO7816_T1_SendNr = 0;
unsigned char ISO7816_T1_R_BlockNr = 0;
unsigned char ISO7816_T1_LastSendNr = 0;

/*******************************************************************************

  ISO7816_T1_InitSendNr

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_T1_InitSendNr (void)
{
    ISO7816_T1_SendNr = 0;
    ISO7816_T1_R_BlockNr = 0;
    ISO7816_T1_LastSendNr = 0;
}

/*******************************************************************************

  ISO7816_T1_Add_I_BlockNr

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_T1_Add_I_BlockNr (void)
{
    if (FALSE == ISO7816_T1_SendNr)
    {
        ISO7816_T1_LastSendNr = 0;
        ISO7816_T1_R_BlockNr = 0;
        ISO7816_T1_SendNr = 1;
    }
    else
    {
        ISO7816_T1_LastSendNr = 1;
        ISO7816_T1_R_BlockNr = 0;
        ISO7816_T1_SendNr = 0;
    }
}

/*******************************************************************************

  ISO7816_T1_Get_I_BlockNr

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

unsigned char ISO7816_T1_Get_I_BlockNr (void)
{
    return (ISO7816_T1_SendNr);

}

/*******************************************************************************

  ISO7816_T1_GetLast_I_BlockNr

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

unsigned char ISO7816_T1_GetLast_I_BlockNr (void)
{
    return (ISO7816_T1_LastSendNr);
}

/*******************************************************************************

  ISO7816_T1_GetNext_I_BlockNr

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

unsigned char ISO7816_T1_GetNext_I_BlockNr (void)
{
    return (ISO7816_T1_LastSendNr);
}

/*******************************************************************************

  ISO7816_T1_Receive_R_Block

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#define ISO7816_MAX_SENDBUFFER			300 // NAD 1 + PCB 1 + LEN 2 + APDU 254 + EDC 2 = 260 (+2 for secure)
#define ISO7816_MAX_RECEIVEBUFFER		300 // NAD 1 + PCB 1 + LEN 2 + APDU 254 + EDC 2 = 260 (+2 for secure)

#define ISO7816_MAX_SEND_HEADER     6   // NAD 1 + PCB 1 + LEN 2 = 4 (+2 for secure)
#define ISO7816_MAX_EDC             4   // EDC 2 = 2 (+2 for secure)


typedef struct
{
    unsigned char cHeaderBuffer[ISO7816_MAX_SEND_HEADER];
    unsigned char cHeaderSize;
    unsigned char* cDataBuffer;
    unsigned int nDataSize;
    unsigned char cEDC_Buffer[ISO7816_MAX_EDC];
    unsigned char cEDC_Size;
} typedef_ISO7816_T1_Block;

typedef_ISO7816_T1_Block ISO7816_T1_DataBlock;

unsigned char ISO7816_T1_SendHeaderBuffer[ISO7816_MAX_SEND_HEADER];
unsigned char ISO7816_T1_SendHeaderSize = 0;


unsigned char ISO7816_T1_SendBuffer[ISO7816_MAX_SENDBUFFER];
unsigned char ISO7816_T1_ReceiveBuffer[ISO7816_MAX_RECEIVEBUFFER];
unsigned char ISO7816_T1_BlockSize = 32;    // 32; // Standard block size (= IFSD)


int ISO7816_T1_Receive_R_Block (void)
{
    int nRet;
    unsigned int nResponseLen;
    unsigned char cReceiveBuffer[4];



    nRet = ISO7816_T1_ReceiveBlock (&nResponseLen);
    if (FALSE == nRet)
    {
        return (FALSE);
    }

    if (4 < nResponseLen)
    {
        return (FALSE);
    }
    memcpy (cReceiveBuffer, ISO7816_T1_ReceiveBuffer, nResponseLen);

    if (0 != cReceiveBuffer[0]) // NAD
    {
        return (FALSE);
    }

    if (cReceiveBuffer[1] != 0x80 + ISO7816_T1_GetNext_I_BlockNr () * ISO7816_T1_PCB_SEND_NR_BIT_R_BLOCK)   // PCB
    {
        return (FALSE);
    }

    if (0 != cReceiveBuffer[2]) // LEN
    {
        return (FALSE);
    }

    if (cReceiveBuffer[3] != ISO7816_XOR_String (3, cReceiveBuffer))    // EDC
    {
        return (FALSE);
    }

    // R block is correct : add I block counter
    ISO7816_T1_Add_I_BlockNr ();

    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_Send_R_Block

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_Send_R_Block (void)
{
    int nRet;
    unsigned char cSendBuffer[4];

    // Build r block
    cSendBuffer[0] = 0; // NAD
    cSendBuffer[1] = 0x80 + ISO7816_T1_Get_I_BlockNr () * ISO7816_T1_PCB_SEND_NR_BIT_R_BLOCK;   // PCB
    cSendBuffer[2] = 0; // LEN
    cSendBuffer[3] = ISO7816_XOR_String (3, cSendBuffer);   // EDC


#ifdef DEBUG_LOG_CCID_DETAIL
    LogStart_T1_Block (4, cSendBuffer);
#endif

//    CI_TickLocalPrintf ("Send R Block - %02x %02x %02x %02x ",cSendBuffer[0],cSendBuffer[1],cSendBuffer[2],cSendBuffer[3]);

    // Send block
    nRet = ISO7816_SendString (4, cSendBuffer);

    if (USART_SUCCESS != nRet)
    {
//      CI_TickLocalPrintf ("FAIL\r\n");
        return (FALSE);
    }

//    CI_TickLocalPrintf ("OK\r\n");
    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_CheckReceiveBlock

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#define ISO7816_T1_BLOCK_KIND_I_BLOCK				0
#define ISO7816_T1_BLOCK_KIND_I_BLOCK_CHAINING		1
#define ISO7816_T1_BLOCK_KIND_R_BLOCK				2
#define ISO7816_T1_BLOCK_KIND_S_BLOCK				3


#define ISO7816_T1_RECEIVE_BLOCK_ERROR				0
#define ISO7816_T1_RECEIVE_I_BLOCK					1
#define ISO7816_T1_RECEIVE_I_BLOCK_CHAINING			2
#define ISO7816_T1_RECEIVE_R_BLOCK					3
#define ISO7816_T1_RECEIVE_S_BLOCK					4

int ISO7816_T1_CheckReceiveBlock (unsigned int nResponseLen)
{
    int nBlockKind;

    if (4 > nResponseLen)
    {
        return (ISO7816_T1_RECEIVE_BLOCK_ERROR);
    }

    // Check block integrity
    if (0 != ISO7816_T1_ReceiveBuffer[ISO7816_T1_NAD_ADDR]) // NAD
    {
        return (ISO7816_T1_RECEIVE_BLOCK_ERROR);
    }

    if (nResponseLen - 4 != ISO7816_T1_ReceiveBuffer[ISO7816_T1_LEN_ADDR])  // LEN
    {
        return (ISO7816_T1_RECEIVE_BLOCK_ERROR);
    }


    // Get block kind
    nBlockKind = ISO7816_T1_ReceiveBuffer[ISO7816_T1_PCB_ADDR] >> 6;
    if (1 == nBlockKind)    // I-Block with blocknr = 1
    {
        nBlockKind = 0;
    }


    switch (nBlockKind)
    {
        case ISO7816_T1_BLOCK_KIND_I_BLOCK:
            if ((ISO7816_T1_ReceiveBuffer[ISO7816_T1_PCB_ADDR] & ISO7816_T1_PCB_SEND_NR_BIT_I_BLOCK) !=
                (ISO7816_T1_Get_I_BlockNr () * ISO7816_T1_PCB_SEND_NR_BIT_I_BLOCK))
            {
                if ((ISO7816_T1_SendBuffer[ISO7816_T1_PCB_ADDR] & ISO7816_T1_PCB_CHAINING_BIT_I_BLOCK) != 0)    // Todo check: When no chaining bis
                                                                                                                // is set
                {
                    return (ISO7816_T1_RECEIVE_BLOCK_ERROR);
                }
            }

            // Check for chaining flag
            if (0 != (ISO7816_T1_ReceiveBuffer[ISO7816_T1_PCB_ADDR] & 0x20))
            {
                return (ISO7816_T1_RECEIVE_I_BLOCK_CHAINING);
            }

            if (ISO7816_T1_ReceiveBuffer[nResponseLen - 1] != ISO7816_XOR_String (nResponseLen - 1, ISO7816_T1_ReceiveBuffer))  // EDC
            {
                return (ISO7816_T1_RECEIVE_BLOCK_ERROR);
            }


            return (ISO7816_T1_RECEIVE_I_BLOCK);



        case ISO7816_T1_BLOCK_KIND_R_BLOCK:
            if (ISO7816_T1_ReceiveBuffer[nResponseLen - 1] != ISO7816_XOR_String (nResponseLen - 1, ISO7816_T1_ReceiveBuffer))  // EDC
            {
                return (ISO7816_T1_RECEIVE_BLOCK_ERROR);
            }

            if (ISO7816_T1_ReceiveBuffer[ISO7816_T1_PCB_ADDR] != 0x80 + ISO7816_T1_GetLast_I_BlockNr () * ISO7816_T1_PCB_SEND_NR_BIT_R_BLOCK)   // PCB
            {
                return (ISO7816_T1_RECEIVE_BLOCK_ERROR);
            }
            return (ISO7816_T1_RECEIVE_R_BLOCK);

        case ISO7816_T1_BLOCK_KIND_S_BLOCK:
            return (ISO7816_T1_RECEIVE_S_BLOCK);
    }
    // Check block follow bit


    return (ISO7816_T1_RECEIVE_BLOCK_ERROR);
}

/*******************************************************************************

  ISO7816_T1_InitDataBlock

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_T1_InitDataBlock (void)
{
    memset (ISO7816_T1_DataBlock.cHeaderBuffer, 0, ISO7816_MAX_SEND_HEADER);
    ISO7816_T1_DataBlock.cHeaderSize = 0;

    ISO7816_T1_DataBlock.cDataBuffer = NULL;
    ISO7816_T1_DataBlock.nDataSize = 0;

    memset (ISO7816_T1_DataBlock.cEDC_Buffer, 0, ISO7816_MAX_EDC);
    ISO7816_T1_DataBlock.cEDC_Size = 0;
}

/*******************************************************************************

  ISO7816_T1_BuildSendHeader

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#define ISO7816_T1_PROTOCOL_OVERHEAD  4 // NAD 1 + PCB 1 + LEN 1 + EDC 1 = 4

unsigned int ISO7816_T1_BuildSendHeader (unsigned int nRestBlocklen)
{
    int nSendBlockLen;

    nSendBlockLen = nRestBlocklen;

    // Build header
    ISO7816_T1_DataBlock.cHeaderSize = 0;

    // NAD
    ISO7816_T1_DataBlock.cHeaderBuffer[ISO7816_T1_DataBlock.cHeaderSize] = 0;
    ISO7816_T1_DataBlock.cHeaderSize++;

    // PCB
    ISO7816_T1_DataBlock.cHeaderBuffer[ISO7816_T1_DataBlock.cHeaderSize] = ISO7816_T1_Get_I_BlockNr () * ISO7816_T1_PCB_SEND_NR_BIT_I_BLOCK;
    if (nRestBlocklen + ISO7816_T1_PROTOCOL_OVERHEAD > ISO7816_T1_BlockSize)
    {
        ISO7816_T1_DataBlock.cHeaderBuffer[ISO7816_T1_DataBlock.cHeaderSize] |= ISO7816_T1_PCB_CHAINING_BIT_I_BLOCK;
        nSendBlockLen = ISO7816_T1_BlockSize - ISO7816_T1_PROTOCOL_OVERHEAD;
    }
    ISO7816_T1_DataBlock.cHeaderSize++;

    // LEN
    ISO7816_T1_DataBlock.cHeaderBuffer[ISO7816_T1_DataBlock.cHeaderSize] = nSendBlockLen + ISO7816_T1_PROTOCOL_OVERHEAD;
    ISO7816_T1_DataBlock.cHeaderSize++;

    return (nSendBlockLen);
}

/*******************************************************************************

  ISO7816_T1_BuildEDC

  Only CRC

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void ISO7816_T1_BuildEDC (void)
{
    int i;
    int nXOR;

    nXOR = 0;

    // XOR header
    for (i = 0; i < ISO7816_T1_DataBlock.cHeaderSize; i++)
    {
        nXOR = nXOR ^ ISO7816_T1_DataBlock.cHeaderBuffer[i];
    }

    // XOR data
    for (i = 0; i < ISO7816_T1_DataBlock.nDataSize; i++)
    {
        nXOR = nXOR ^ ISO7816_T1_DataBlock.cDataBuffer[i];
    }

    // Save XOR value in EDC buffer
    ISO7816_T1_DataBlock.cEDC_Buffer[0] = nXOR;
    ISO7816_T1_DataBlock.cEDC_Size = 1;
}


/*******************************************************************************

  ISO7816_T1_Send_Block

  Send a part of the APDU data

  NAD
  PCB
  LEN

  APDU Part

  EDC

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_Send_Block (void)
{
    int nRet;

    // Send T1 header
    nRet = ISO7816_SendString (ISO7816_T1_DataBlock.cHeaderSize, ISO7816_T1_DataBlock.cHeaderBuffer);
    if (USART_SUCCESS != nRet)
    {
        return (FALSE);
    }

    // Send data from apdu
    nRet = ISO7816_SendString (ISO7816_T1_DataBlock.nDataSize, ISO7816_T1_DataBlock.cDataBuffer);
    if (USART_SUCCESS != nRet)
    {
        return (FALSE);
    }

    // Send EDC
    nRet = ISO7816_SendString (ISO7816_T1_DataBlock.cEDC_Size, ISO7816_T1_DataBlock.cEDC_Buffer);
    if (USART_SUCCESS != nRet)
    {
        return (FALSE);
    }

    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_Send_APDU_String

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/


#define ISO7816_T1_MAX_SEND_ERROR_COUNT    5

#define ISO7816_T1_SEND_I_BLOCK            0
#define ISO7816_T1_SEND_R_BLOCK            1


void ISO7816_T1_Send_APDU_String (unsigned int nAPDULen, unsigned char* cAPDUData, unsigned int* nReceiveBytes, unsigned char* cReceiveData)
{
    unsigned int nBytesToSend;
    unsigned int nSendPointer;
    unsigned int nReceivePointer;
    unsigned int nResponseLen;
    unsigned int nBlockSize;
    unsigned int nErrorCount;
    unsigned int nSendFlag;
    int nRet;


    *nReceiveBytes = 0;

    nBytesToSend = nAPDULen;
    nSendPointer = 0;
    nReceivePointer = 0;
    nErrorCount = 0;
    nSendFlag = ISO7816_T1_SEND_I_BLOCK;

    // Is something to send ?
    while (nAPDULen > nSendPointer)
    {
        // Build T1 String
        nBlockSize = ISO7816_T1_BuildSendHeader (nBytesToSend);
        ISO7816_T1_DataBlock.cDataBuffer = &cAPDUData[nSendPointer];
        ISO7816_T1_DataBlock.nDataSize = nBlockSize;
        ISO7816_T1_BuildEDC ();

        // Send block to smartcard via T1
        nRet = ISO7816_T1_Send_Block ();
        if (FALSE == nRet)
        {
            return; // (FALSE);
        }

        // Receive block
        nRet = ISO7816_T1_ReceiveBlock (&nResponseLen); // to do: use "cReceiveData" buffer
        if (FALSE == nRet)
        {
            return; // (FALSE);
        }

#ifdef DEBUG_LOG_CCID_DETAIL
        LogEnd_T1_Block (nResponseLen, ISO7816_T1_ReceiveBuffer);
#endif

        if (4 == nResponseLen)  // Must be a r- or s- block
        {
            if (ISO7816_T1_RECEIVE_R_BLOCK != ISO7816_T1_CheckReceiveBlock (nResponseLen))
            {
                break;  // error occurred
            }
        }
        else
        {
            break;  // End of transmission or error occurred
        }

        // Set pointer to the next data
        nSendPointer += nBlockSize;
    }
}


/*******************************************************************************

  ISO7816_T1_SendBlock

	nCount 		Bytes to send
	cData		  Data

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_SendBlock (unsigned int nCount, unsigned char* cData, unsigned int nChainingFlag)
{
    int nRet;
    int nPointer;

    // Build header
    nPointer = 0;


    ISO7816_T1_SendBuffer[nPointer] = 0;    // NAD
    nPointer++;

    ISO7816_T1_SendBuffer[nPointer] = ISO7816_T1_Get_I_BlockNr () * ISO7816_T1_PCB_SEND_NR_BIT_I_BLOCK; // PCB
    if (TRUE == nChainingFlag)
    {
        ISO7816_T1_SendBuffer[nPointer] |= ISO7816_T1_PCB_CHAINING_BIT_I_BLOCK;
    }
    nPointer++;

    ISO7816_T1_SendBuffer[nPointer] = nCount;   // LEN
    nPointer++;

    memcpy (&ISO7816_T1_SendBuffer[nPointer], cData, nCount);
    nPointer += nCount;

    ISO7816_T1_SendBuffer[nPointer] = ISO7816_XOR_String (nPointer, ISO7816_T1_SendBuffer); // EDC
    nPointer += 1;

#ifdef DEBUG_LOG_CCID_DETAIL
    LogStart_T1_Block (nPointer, ISO7816_T1_SendBuffer);
#endif

    nRet = ISO7816_SendString (nPointer, ISO7816_T1_SendBuffer);
    if (USART_SUCCESS != nRet)
    {
        return (FALSE);
    }

    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_SendString

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_SendString (unsigned int nSendBytes, unsigned char* cSendData, unsigned int* nReceiveBytes, unsigned char* cReceiveData)
{
    unsigned int nSendPointer;
    unsigned int nReceivePointer;
    unsigned int nResponseLen;
    unsigned int nBlockSize;
    unsigned int nErrorCount;
    unsigned int nSendFlag;
    unsigned int nChainingFlag;
    unsigned int nFolgeDatenBloecke;
    int nRet;

    // Buffer overflow ?
    if (ISO7816_MAX_SENDBUFFER <= nSendBytes + 5)
    {
        return (FALSE);
    }

    *nReceiveBytes = 0;

    // Send data in blocks
    nSendPointer = 0;
    nReceivePointer = 0;
    nErrorCount = 0;
    nSendFlag = ISO7816_T1_SEND_I_BLOCK;
    nFolgeDatenBloecke = 0;

    while (nSendBytes > nSendPointer)
    {
        nChainingFlag = FALSE;

        if (ISO7816_T1_SEND_I_BLOCK == nSendFlag)
        {
            nBlockSize = ISO7816_T1_BlockSize;

            // Reduce last blocksize ?
            if (nSendBytes - nSendPointer < nBlockSize)
            {
                nBlockSize = nSendBytes - nSendPointer;
            }

            // Set chaining flag ?
            if (nSendPointer + nBlockSize != nSendBytes)
            {
                nChainingFlag = TRUE;
            }


            // Send block to smartcard via T1
            nRet = ISO7816_T1_SendBlock (nBlockSize, &cSendData[nSendPointer], nChainingFlag);
            if (FALSE == nRet)
            {
                return (FALSE);
            }

        }

        if (ISO7816_T1_SEND_R_BLOCK == nSendFlag)
        {
            ISO7816_T1_Send_R_Block ();
        }

        // Receive block
        nRet = ISO7816_T1_ReceiveBlock (&nResponseLen);
        if (FALSE == nRet)
        {
            return (FALSE);
        }

#ifdef DEBUG_LOG_CCID_DETAIL
        LogEnd_T1_Block (nResponseLen, ISO7816_T1_ReceiveBuffer);
#endif

        // Check receive block
        nRet = ISO7816_T1_CheckReceiveBlock (nResponseLen);

        switch (nRet)
        {
            case ISO7816_T1_RECEIVE_I_BLOCK:
                memcpy (&cReceiveData[nReceivePointer], &ISO7816_T1_ReceiveBuffer[3], nResponseLen - 4);
                nReceivePointer += nResponseLen - 4;
                nSendPointer += nBlockSize;
                nSendFlag = ISO7816_T1_SEND_I_BLOCK;
                ISO7816_T1_Add_I_BlockNr ();    // Switch block nr for next i-block
                break;

            case ISO7816_T1_RECEIVE_I_BLOCK_CHAINING:
                memcpy (&cReceiveData[nReceivePointer], &ISO7816_T1_ReceiveBuffer[3], nResponseLen - 4);
                nReceivePointer += nResponseLen - 4;
                nSendFlag = ISO7816_T1_SEND_R_BLOCK;
                nFolgeDatenBloecke++;
                ISO7816_T1_Add_I_BlockNr ();    // Switch block nr for next i-block
                break;

            case ISO7816_T1_RECEIVE_R_BLOCK:
                nSendPointer += nBlockSize;
                ISO7816_T1_Add_I_BlockNr ();    // Switch block nr for next i-block
                break;

            case ISO7816_T1_RECEIVE_S_BLOCK:
                nErrorCount++;
                break;

            case ISO7816_T1_RECEIVE_BLOCK_ERROR:
                nErrorCount++;
                break;

        }

        if (ISO7816_T1_MAX_SEND_ERROR_COUNT < nErrorCount)
        {
            return (FALSE);
        }
    }
/*
    CI_LocalPrintf ("**  - ");
    HexPrint (nReceivePointer, cReceiveData);
    CI_LocalPrintf ("\r\n");
*/
    if (0 != (nFolgeDatenBloecke % 2))
    {
      ISO7816_T1_Add_I_BlockNr ();    // Korrektur bei BLOCK_CHAINING - Switch block nr for next i-block
    }

    *nReceiveBytes = nReceivePointer;
    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_SendString_o

  Old version

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_SendString_o (unsigned int nCount, unsigned char* cAPDU)
{
    int nPointer = 0;
    int nRet;

    // Buffer overflow ?
    if (ISO7816_MAX_SENDBUFFER <= nCount + 5)
    {
        return (FALSE); // yes
    }

    ISO7816_T1_SendBuffer[nPointer] = 0;    // NAD
    nPointer++;

    ISO7816_T1_Add_I_BlockNr ();

    ISO7816_T1_SendBuffer[nPointer] = ISO7816_T1_Get_I_BlockNr () * ISO7816_T1_PCB_SEND_NR_BIT_I_BLOCK; // PCB
    nPointer++;

    ISO7816_T1_SendBuffer[nPointer] = nCount;   // LEN
    nPointer++;

    memcpy (&ISO7816_T1_SendBuffer[nPointer], cAPDU, nCount);
    nPointer += nCount;

    ISO7816_T1_SendBuffer[nPointer] = ISO7816_XOR_String (nPointer, ISO7816_T1_SendBuffer); // EDC
    nPointer += 1;

    nRet = ISO7816_SendString (nPointer, ISO7816_T1_SendBuffer);

    if (USART_SUCCESS != nRet)
    {
        return (FALSE);
    }

    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_ReceiveBlock

  *nCount     Pointer to var for saving received char count

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_ReceiveBlock (unsigned int* nCount)
{
    int nResponseLen;
    int nRet;

    ISO7816_ClearRx ();
    nResponseLen = ISO7816_MAX_RECEIVEBUFFER;
    nRet = ISO7816_ReadString (&nResponseLen, ISO7816_T1_ReceiveBuffer);

    if ((USART_SUCCESS != nRet) && (ISO7816_USART_TIMEOUT != nRet))
    {
        return (FALSE);
    }

    // check error detection
    if (ISO7816_T1_ReceiveBuffer[nResponseLen - 1] != ISO7816_XOR_String (nResponseLen - 1, ISO7816_T1_ReceiveBuffer))
    {
        return (FALSE);
    }

    // check send counter
    /* Check Full PCB if ((ISO7816_T1_SendBuffer[ISO7816_T1_PCB_ADDR] & ISO7816_T1_PCB_SEND_NR_BIT_I_BLOCK) != (ISO7816_T1_GetLastSendNr () *
       ISO7816_T1_PCB_SEND_NR_BIT_I_BLOCK)) { return (FALSE); } */

    // check length
    if (0 == nResponseLen)
    {
        return (FALSE);
    }

    // check length byte
    if (ISO7816_T1_ReceiveBuffer[ISO7816_T1_LEN_ADDR] != nResponseLen - 4)
    {
        return (FALSE);
    }

    *nCount = nResponseLen;

    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_Check_PCB

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

enum
{
    ISO7816_T1_I_BLOCK_OK = 0,
    ISO7816_T1_I_BLOCK_FOLLOW,
    ISO7816_T1_R_BLOCK_OK,
    ISO7816_T1_R_BLOCK_EDC_ERROR,
    ISO7816_T1_R_BLOCK_ERROR,
    ISO7816_T1_S_BLOCK_RESYNC_Q,
    ISO7816_T1_S_BLOCK_RESYNC_A,
    ISO7816_T1_S_BLOCK_CHANGE_Q,
    ISO7816_T1_S_BLOCK_CHANGE_A,
    ISO7816_T1_S_BLOCK_ABORT_Q,
    ISO7816_T1_S_BLOCK_ABORT_A,
    ISO7816_T1_S_BLOCK_DELAY_Q,
    ISO7816_T1_S_BLOCK_DELAY_A,
    ISO7816_T1_S_BLOCK_VPP_ERROR,
    ISO7816_T1_I_BLOCK_ERROR
} ISO7816_T1_PCB;

int ISO7816_T1_Check_PCB (unsigned char cPCB)
{

    // I-Block
    if (0x00 == (cPCB & 0x80))
    {
        if (0x20 == (cPCB & 0x20))
        {
            return (ISO7816_T1_I_BLOCK_FOLLOW);
        }
        return (ISO7816_T1_I_BLOCK_OK);
    }

    // R-Block
    if (0x80 == (cPCB & 0xC0))
    {
        if (0x00 == (cPCB & 0x2F))
        {
            return (ISO7816_T1_R_BLOCK_OK);
        }
        if (0x01 == (cPCB & 0x2F))
        {
            return (ISO7816_T1_R_BLOCK_EDC_ERROR);
        }
        if (0x02 == (cPCB & 0x2F))
        {
            return (ISO7816_T1_R_BLOCK_ERROR);
        }
        return (ISO7816_T1_R_BLOCK_ERROR);
    }

    // S-Block
    if (0xC0 == (cPCB & 0xC0))
    {
        if (0x00 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_RESYNC_A);
        }
        if (0x20 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_RESYNC_Q);
        }
        if (0x01 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_CHANGE_Q);
        }
        if (0x21 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_CHANGE_A);
        }
        if (0x02 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_ABORT_Q);
        }
        if (0x22 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_ABORT_A);
        }
        if (0x02 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_DELAY_Q);
        }
        if (0x23 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_DELAY_A);
        }
        if (0x24 == (cPCB & 0x3F))
        {
            return (ISO7816_T1_S_BLOCK_VPP_ERROR);
        }
        return (ISO7816_T1_I_BLOCK_ERROR);
    }

    // Function can't come to this point
    return (ISO7816_T1_I_BLOCK_ERROR);
}

/*******************************************************************************

  ISO7816_T1_ReceiveString

  *nCount     Pointer to var for saving received char count
  cReceiveBuffer  Buffer for received chars

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_ReceiveString (unsigned int* nCount, unsigned char* cReceiveBuffer)
{
    unsigned int nReceivePos;
    int nReceiveLen;
    int nEndFlag;
    int nRet;
    unsigned int nResponseLen;
    int nPCB_Status;

    nReceiveLen = *nCount;
    nReceivePos = 0;

    nEndFlag = FALSE;

    while (FALSE == nEndFlag)
    {

        // Get I-block from smartcard
        nRet = ISO7816_T1_ReceiveBlock (&nResponseLen);
        if (TRUE != nRet)
        {
            return (FALSE);
        }
        // copy the I-block to the receive buffer
        memcpy (&cReceiveBuffer[nReceivePos], &ISO7816_T1_ReceiveBuffer[3], nResponseLen - 4);
        nReceivePos += nResponseLen - 4;

        // Shorten the max receive length
        nReceiveLen -= nResponseLen - 4;

        // Check I-block status
        nPCB_Status = ISO7816_T1_Check_PCB (ISO7816_T1_ReceiveBuffer[1]);

        switch (nPCB_Status)
        {
            case ISO7816_T1_I_BLOCK_OK:
                nEndFlag = TRUE;
                break;

            case ISO7816_T1_I_BLOCK_FOLLOW:
                ISO7816_T1_Send_R_Block ();
                break;

            case ISO7816_T1_I_BLOCK_ERROR:
            case ISO7816_T1_R_BLOCK_OK:
            case ISO7816_T1_R_BLOCK_EDC_ERROR:
            case ISO7816_T1_R_BLOCK_ERROR:
            case ISO7816_T1_S_BLOCK_RESYNC_Q:
            case ISO7816_T1_S_BLOCK_RESYNC_A:
            case ISO7816_T1_S_BLOCK_CHANGE_Q:
            case ISO7816_T1_S_BLOCK_CHANGE_A:
            case ISO7816_T1_S_BLOCK_ABORT_Q:
            case ISO7816_T1_S_BLOCK_ABORT_A:
            case ISO7816_T1_S_BLOCK_DELAY_Q:
            case ISO7816_T1_S_BLOCK_DELAY_A:
            case ISO7816_T1_S_BLOCK_VPP_ERROR:
                nEndFlag = 3;
                break;  // Not implemented

        }
    }

    // Return the received char count
    *nCount = nReceivePos;

    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_SendAPDU

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_SendAPDU (typeAPDU * tSC)
{
int nRet;

    // Clear tx buffer
    ISO7816_ClearTx ();

    // Send APDU via T1
    nRet = ISO7816_T1_SendString (tSC->nSendDataLen, tSC->cSendData, &tSC->nReceiveDataLen, tSC->cReceiveData);
    if (TRUE != nRet)
    {
        return (FALSE);
    }

    // Got response
    if (2 > tSC->nReceiveDataLen)
    {
        return (FALSE);
    }

    // Set response
    tSC->tState.cSW1 = tSC->cReceiveData[tSC->nReceiveDataLen - 2];
    tSC->tState.cSW2 = tSC->cReceiveData[tSC->nReceiveDataLen - 1];

    // Remove response bytes from data
    tSC->nReceiveDataLen -= 2;
    return (TRUE);
}

/*******************************************************************************

  ISO7816_T1_DirectXfr

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

int ISO7816_T1_DirectXfr (signed long* nLength, unsigned char* szXfrBuffer, signed long nInputLength)
{
    int nRet;
    static int nCount = 0;

    nRet = ISO7816_SendString (*nLength, szXfrBuffer);
    if (USART_SUCCESS != nRet)
    {
        return (nRet);
    }

    ISO7816_ClearRx ();

    *nLength = nInputLength;

    nRet = ISO7816_ReadString ((int *) nLength, szXfrBuffer);
    /*
       if (nCount == 5) { if (ISO7816_USART_TIMEOUT == nRet) { return (nRet); } } */
    if ((USART_SUCCESS != nRet) && (ISO7816_USART_TIMEOUT != nRet))
    {
        return (nRet);
    }

    nCount++;
    /*
       if (ISO7816_USART_TIMEOUT == nRet) { CI_LocalPrintf ("%7d : ISO7816_T1_DirectXfr - Timeout\n",xTaskGetTickCount()); } */
    return (USART_SUCCESS);
}
