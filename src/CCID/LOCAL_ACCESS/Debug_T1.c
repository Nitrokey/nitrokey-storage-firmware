/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 19.08.2011
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

#ifdef FREERTOS_USED
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "string.h"

#include "global.h"
#include "tools.h"
#include "usart.h"
#include "CCID\\USART\\ISO7816_USART.h"
#include "CCID\\USART\\ISO7816_ADPU.h"
#include "CCID\\USART\\ISO7816_Prot_T1.h"
#include "USB_CCID\\USB_CCID.h"

#include "Debug_T1.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

// Set debuglevel
// 0 = print nothing
// 3 = print all
#ifdef DEBUG_LOG_CCID_DETAIL
int CCID_T1_DebugLevel = 3;
#else
int CCID_T1_DebugLevel = 0;
#endif

int nStartT1BlockLenght;
char acStartT1Block[CCID_MAX_XFER_LENGTH];
int nUSB_To_CCID_StartTime;
int nUSB_To_CCID_EndTime;



/*******************************************************************************

  Print_T1_Header_full

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void Print_T1_Header_full (unsigned char* acBlockData)
{
    CI_LocalPrintf ("\r\n------------------------------------\r\nNAD-Block: ");
    CI_LocalPrintf ("DAD (Ziel)   %d  Vqq %d -", acBlockData[0] >> 7, (acBlockData[0] >> 4) & 0x7);

    CI_LocalPrintf ("SAD (Quelle) %d  Vpp %d\r\n", (acBlockData[0] >> 3) & 0x1, (acBlockData[0] >> 0) & 0x7);

    CI_LocalPrintf ("PCB-Block: ");
    switch (acBlockData[1] >> 6)
    {
        case 0:
        case 1:
            CI_LocalPrintf ("I-Block ");
            CI_LocalPrintf ("Sende NR   %d  ", (acBlockData[1] >> 6) & 0x1);
            CI_LocalPrintf ("Folgedaten %d\r\n", (acBlockData[1] >> 5) & 0x1);
            break;
        case 2:
            CI_LocalPrintf ("R-Block ");
            CI_LocalPrintf ("Sende NR   %d  ", (acBlockData[1] >> 4) & 0x1);
            switch (acBlockData[1] & 0xF)
            {
                case 0:
                    CI_LocalPrintf ("Kein Fehler\r\n");
                    break;
                case 1:
                    CI_LocalPrintf ("EDC-, Paritaetsfehler\r\n");
                    break;
                case 2:
                    CI_LocalPrintf ("Sonstiger Fehler\r\n");
                    break;
                default:
                    CI_LocalPrintf ("R-Block unbekannt %d\r\n", acBlockData[1] & 0xF);
                    break;
            }
            break;
        case 3:
            CI_LocalPrintf ("S-Block ");
            switch (acBlockData[1] & 0x3F)
            {
                case 0:
                    CI_LocalPrintf ("Anfrage Resynch\r\n");
                    break;
                case 1:
                    CI_LocalPrintf ("Anfrage Groessenaenderung\r\n");
                    break;
                case 2:
                    CI_LocalPrintf ("Abort Anfrage\r\n");
                    break;
                case 3:
                    CI_LocalPrintf ("Anfrage Wartezeitverlaengerung\r\n");
                    break;
                case 32:
                    CI_LocalPrintf ("Antwort Resynch\r\n");
                    break;
                case 33:
                    CI_LocalPrintf ("Antwort Groessenaenderung\r\n");
                    break;
                case 34:
                    CI_LocalPrintf ("Abort Antwort\r\n");
                    break;
                case 35:
                    CI_LocalPrintf ("Antwort Wartezeitverlaengerung\r\n");
                    break;
                case 36:
                    CI_LocalPrintf ("Antwort Vpp Fehler\r\n");
                    break;
                default:
                    CI_LocalPrintf ("S-Block unbekannt %d\r\n", acBlockData[1] & 0x3F);
                    break;
            }
            break;
        default:
            CI_LocalPrintf ("PCB - Unbekannt %d\r\n", acBlockData[1] >> 6);
            break;
    }

    CI_LocalPrintf ("Datenlaenge %d\r\n", acBlockData[2]);
    // DelayMs (100);
}

/*******************************************************************************

  Print_USB_to_CCID_T1_Header

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

#define T1_I_BLOCK   0
#define T1_R_BLOCK   1
#define T1_S_BLOCK   2

void Print_USB_to_CCID_T1_Header (unsigned char* acBlockData)
{
    CI_LocalPrintf ("          nNAD ");
    CI_LocalPrintf ("Ziel %d - ", acBlockData[0] >> 7);

    CI_LocalPrintf ("Quelle %d ", (acBlockData[0] >> 3) & 0x1);

    CI_LocalPrintf (" PCB ");
    switch (acBlockData[1] >> 6)
    {
        case 0:
        case 1:
            CI_LocalPrintf ("- I-Block ");
            CI_LocalPrintf ("Sende NR   %d ", (acBlockData[1] >> 6) & 0x1);
            CI_LocalPrintf ("Folgedaten %d ", (acBlockData[1] >> 5) & 0x1);
            break;
        case 2:
            CI_LocalPrintf ("- R-Block ");
            CI_LocalPrintf ("Sende NR   %d ", (acBlockData[1] >> 4) & 0x1);
            switch (acBlockData[1] & 0xF)
            {
                case 0:
                    CI_LocalPrintf ("Kein Fehler ");
                    break;
                case 1:
                    CI_LocalPrintf ("EDC-, Paritaetsfehler ");
                    break;
                case 2:
                    CI_LocalPrintf ("Sonstiger Fehler ");
                    break;
                default:
                    CI_LocalPrintf ("R-Block unbekannt %d ", acBlockData[1] & 0xF);
                    break;
            }
            break;
        case 3:
            CI_LocalPrintf ("- S-Block ");
            switch (acBlockData[1] & 0x3F)
            {
                case 0:
                    CI_LocalPrintf ("Anfrage Resync");
                    break;
                case 1:
                    CI_LocalPrintf ("Anfrage Groessenaenderung ");
                    break;
                case 2:
                    CI_LocalPrintf ("Abort Anfrage ");
                    break;
                case 3:
                    CI_LocalPrintf ("Anfrage Wartezeitverlaengerung ");
                    break;
                case 32:
                    CI_LocalPrintf ("Antwort Resync ");
                    break;
                case 33:
                    CI_LocalPrintf ("Antwort Groessenaenderung ");
                    break;
                case 34:
                    CI_LocalPrintf ("Abort Antwort ");
                    break;
                case 35:
                    CI_LocalPrintf ("Antwort Wartezeitverlaengerung ");
                    break;
                case 36:
                    CI_LocalPrintf ("Antwort Vpp Fehler ");
                    break;
                default:
                    CI_LocalPrintf ("S-Block unbekannt %d ", acBlockData[1] & 0x3F);
                    break;
            }
            break;
        default:
            CI_LocalPrintf ("PCB - Unbekannt %d ", acBlockData[1] >> 6);
            break;
    }

    CI_LocalPrintf (" Bytes %d\r\n", acBlockData[2]);
    // DelayMs (100);
}


/*******************************************************************************

  Print_USB_to_CCID_T1_Data_I_Block

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void Print_USB_to_CCID_T1_Data_I_Block (int nLenght, unsigned char* acStartBlockData)
{
    /*
       CI_LocalPrintf (" CLA INS P1 P2 Lc \r\n"); CI_LocalPrintf (" %02x %02x %02x
       %02x\r\n",acStartBlockData[0],acStartBlockData[1],acStartBlockData[2],acStartBlockData[3]); */

    // Select file
    if ((0xA4 == acStartBlockData[1]) && (0x04 == acStartBlockData[2]) && (0x00 == acStartBlockData[3]))
    {
        CI_LocalPrintf ("          SELECT FILE ");
        AsciiHexPrint (acStartBlockData[4], &acStartBlockData[5]);
        CI_LocalPrintf ("\r\n");
    }

    // Verify
    if ((0x20 == acStartBlockData[1]) && (0x00 == acStartBlockData[2]))
    {
        CI_LocalPrintf ("          VERIFY PW %02x - ", acStartBlockData[3]);
        AsciiHexPrint (acStartBlockData[4], &acStartBlockData[5]);
        CI_LocalPrintf ("\r\n");
    }

    // Get Data
    if ((0xCA == acStartBlockData[1]))
    {
        CI_LocalPrintf ("          GET DATA P1=%02x  P2=%02x\r\n", acStartBlockData[2], acStartBlockData[3]);
    }

    // PUT DATA
    if ((0xDA == acStartBlockData[1]) || (0xDB == acStartBlockData[1]))
    {
        CI_LocalPrintf ("          PUT DATA\r\n");
    }


    // GENERATE ASYMMETRIC KEY PAIR
    if ((0x47 == acStartBlockData[1]) && (0x80 == acStartBlockData[2]))
    {
        CI_LocalPrintf ("          GENERATE KEY\r\n");
    }

    // GET PUBLIC KEY
    if ((0x47 == acStartBlockData[1]) && (0x81 == acStartBlockData[2]))
    {
        CI_LocalPrintf ("          GET PUBLIC KEY\r\n");
    }

    // PERFORM SECURITY OPERATION (PSO)
    if (0x2A == acStartBlockData[1])
    {
        CI_LocalPrintf ("          PERFORM SECURITY OPERATION\r\n");
    }

    // GET RESPONSE
    if ((0xC0 == acStartBlockData[1]) && (0x00 == acStartBlockData[2]) && (0x00 == acStartBlockData[3]))
    {
        CI_LocalPrintf ("          GET RESPONSE\r\n");
    }

    // GET CHALLENGE
    if ((0x84 == acStartBlockData[1]) && (0x00 == acStartBlockData[2]) && (0x00 == acStartBlockData[3]))
    {
        CI_LocalPrintf ("          GET CHALLENGE\r\n");
    }
}

/*******************************************************************************

  Print_T1_I_Block_Data

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void Print_T1_I_Block_Data (int nLenght, unsigned char* acBlockData)
{
    int nStatusByte;
    int n;

    // CI_LocalPrintf (" Length %d\r\n",acBlockData[2]);

    if (1 == ((acBlockData[1] >> 5) & 0x1))
    {
        CI_LocalPrintf ("          Folgedatenblock - %2d Bytes = ", acBlockData[2]);
        AsciiHexPrint (n, &acBlockData[3]);
    }
    else
    {
        if (0 != acBlockData[2] - 2)
        {
            CI_LocalPrintf ("          Data - ");
            AsciiHexPrint (acBlockData[2] - 2, &acBlockData[3]);
            CI_LocalPrintf ("\r\n");
        }
        CI_LocalPrintf ("          ");
        HexPrint (2, &acBlockData[nLenght - 3]);
        CI_LocalPrintf ("= ");
        nStatusByte = acBlockData[nLenght - 3] * 0x100 + acBlockData[nLenght - 2];
        switch (nStatusByte)
        {
                /*
                   61 xx Command correct, xx bytes available in response (normally used under T=0 or for commands under any protocol with long
                   response data that cannot be transmitted in one response) */
            case 0x6285:
                CI_LocalPrintf ("Selected file in termination state");
                break;
            case 0x6581:
                CI_LocalPrintf ("Memory failure");
                break;
            case 0x6700:
                CI_LocalPrintf ("Wrong length (Lc and/or Le)");
                break;
            case 0x6882:
                CI_LocalPrintf ("Secure messaging not supported");
                break;
            case 0x6883:
                CI_LocalPrintf ("Last command of the chain expected");
                break;
            case 0x6884:
                CI_LocalPrintf ("Command chaining not supported");
                break;
            case 0x6982:
                CI_LocalPrintf ("Security status not satisfied");
                /*
                   PW wrong PW not checked (command not allowed) Secure messaging incorrect (checksum and/or cryptogram) */
                break;
            case 0x6983:
                CI_LocalPrintf ("Authentication method blocked");
                // PW blocked (error counter zero)
                break;
            case 0x6985:
                CI_LocalPrintf ("Condition of use not satisfied");
                break;
            case 0x6A80:
                CI_LocalPrintf ("Incorrect parameters in the data field");
                break;
            case 0x6A88:
                CI_LocalPrintf ("Referenced data not found");
                break;
            case 0x6B00:
                CI_LocalPrintf ("Wrong parameters P1-P2");
                break;
            case 0x6D00:
                CI_LocalPrintf ("Instruction (INS) not supported");
                break;
            case 0x6E00:
                CI_LocalPrintf ("Class (CLA) not supported");
                break;
            case 0x9000:
                CI_LocalPrintf ("Command correct");
                break;
            default:
                CI_LocalPrintf ("Status byte 0x%04x not found", nStatusByte);
                break;

        }
    }

    CI_LocalPrintf ("\r\n");
}

/*******************************************************************************

  Print_T1_R_Block_Data

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void Print_T1_R_Block_Data (int nLenght, unsigned char* acBlockData)
{
    int nPCBByte;


    CI_LocalPrintf ("          ");
    HexPrint (nLenght, acBlockData);
    CI_LocalPrintf ("= ");

    nPCBByte = acBlockData[nLenght - 1] & 0x3;

    switch (nPCBByte)
    {
        case 0:
            CI_LocalPrintf ("No error");
            break;
        case 1:
            CI_LocalPrintf ("EDC- / parity error");
            break;
        case 2:
            CI_LocalPrintf ("error");
            break;
        default:
            CI_LocalPrintf ("PCB byte 0x%02x not found", nPCBByte);
            break;
    }

    CI_LocalPrintf ("\r\n");
}


/*******************************************************************************

  Print_T1_S_Block_Data



  Reviews
  Date      Reviewer        Info


*******************************************************************************/

void Print_T1_S_Block_Data (int nLenght, unsigned char* acBlockData)
{
    int nPCBByte;

    CI_LocalPrintf ("          ");
    HexPrint (nLenght, acBlockData);
    CI_LocalPrintf ("= ");

    nPCBByte = acBlockData[1] & 0x3F;

    switch (nPCBByte)
    {
        case 0x00:
            CI_LocalPrintf ("Resynch request");
            break;
        case 0x20:
            CI_LocalPrintf ("Resynch response");
            break;
        case 0x01:
            CI_LocalPrintf ("IFS request");
            break;
        case 0x21:
            CI_LocalPrintf ("IFS response");
            break;
        case 0x02:
            CI_LocalPrintf ("Abort request");
            break;
        case 0x22:
            CI_LocalPrintf ("Abort response");
            break;
        case 0x03:
            CI_LocalPrintf ("WTX request");
            break;
        case 0x23:
            CI_LocalPrintf ("WTX response");
            break;
        default:
            CI_LocalPrintf ("PCB byte 0x%02x not found", nPCBByte);
            break;
    }

    CI_LocalPrintf ("\r\n");
}


/*******************************************************************************

  LogStart_T1_Block

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void LogStart_T1_Block (int nLenght, unsigned char* acStartBlockData)
{
    if (0 == CCID_T1_DebugLevel)
    {
        return;
    }

    if (CCID_MAX_XFER_LENGTH <= nLenght)
    {
        nLenght = CCID_MAX_XFER_LENGTH - 1;
    }
    memcpy (acStartT1Block, acStartBlockData, nLenght);

    nStartT1BlockLenght = nLenght;
    nUSB_To_CCID_StartTime = xTaskGetTickCount ();
}

/*******************************************************************************

  LogEnd_T1_Block

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void LogEnd_T1_Block (int nLenght, unsigned char acEndBlockData[])
{
    if (0 == CCID_T1_DebugLevel)
    {
        return;
    }

    nUSB_To_CCID_EndTime = xTaskGetTickCount ();

    CI_LocalPrintf ("%7d : Send T1 block - Comand len %3d - ", nUSB_To_CCID_StartTime/2, nStartT1BlockLenght);
    CI_LocalPrintf (" Answer len %3d - %5d ms\n", nLenght, (nUSB_To_CCID_EndTime - nUSB_To_CCID_StartTime)/2);

    CI_LocalPrintf ("Command - ");

    HexPrint (nStartT1BlockLenght, (unsigned char *) acStartT1Block);
    CI_LocalPrintf ("\r\n");

    // Print_USB_to_CCID_T1_Header (acStartT1Block);

    if (3 == CCID_T1_DebugLevel)
    {
        if (1 == ((acStartT1Block[1] >> 5) & 0x1))
        {
            CI_LocalPrintf ("          chained block from %2d bytes\r\n", acStartT1Block[2]);
        }

        // Check block type
        switch (acStartT1Block[1] >> 6)
        {
            case 0:    // I-Block
            case 1:
                CI_LocalPrintf ("          I-Block\r\n");
                Print_USB_to_CCID_T1_Data_I_Block (nStartT1BlockLenght - 3, (unsigned char *) &acStartT1Block[3]);
                break;
            case 2:    // R-Block
                CI_LocalPrintf ("          R-Block\r\n");
                break;
            case 3:    // S-Block
                CI_LocalPrintf ("          S-Block\r\n");
                Print_T1_S_Block_Data (nStartT1BlockLenght, (unsigned char *) acStartT1Block);
                break;
        }
    }
    // CI_LocalPrintf ("\r\n");

    // ******************************************************************
    CI_LocalPrintf ("Answer  - ");

    HexPrint (nLenght, acEndBlockData);
    CI_LocalPrintf ("\r\n");
    /*
       Print_USB_to_CCID_T1_Header (acEndBlockData); */

    if (3 == CCID_T1_DebugLevel)
    {
        // Check block type
        switch (acEndBlockData[1] >> 6)
        {
            case 0:    // I-Block
            case 1:
                CI_LocalPrintf ("          I-Block\r\n");
                Print_T1_I_Block_Data (nLenght, acEndBlockData);
                break;

            case 2:    // R-Block
                CI_LocalPrintf ("          R-Block\r\n");
                Print_T1_R_Block_Data (nLenght, acEndBlockData);
                break;

            case 3:    // S-Block
                CI_LocalPrintf ("          S-Block\r\n");
                Print_T1_S_Block_Data (nLenght, acEndBlockData);
                break;
        }
    }
    CI_LocalPrintf ("-----------------------------------------\r\n");
}


/*******************************************************************************

  Print_T1_Block

  Reviews
  Date      Reviewer        Info
  14.08.13  RB              First review

*******************************************************************************/

void Print_T1_Block (int nLenght, unsigned char* acBlockData)
{


}
