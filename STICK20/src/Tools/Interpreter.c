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


/********************************************************************

   Datei    Interpreter.cpp

   Author   Rudolf Böddeker

   Achtung!!! 
   Mit Eingabe eines Befehls erfolgt ein "printf"-Aufruf
   welcher der Stackbedarf zusätzlich erhöht. 
   (Z.B. beim R32C, ca. 220 Byte)

********************************************************************/

#include "global.h"
#include "tools.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#ifdef INTERPRETER_ENABLE

#ifdef FREERTOS_USED
  #include "FreeRTOS.h"
  #include "task.h"
#endif

#include "conf_explorer.h"
/* FAT includes */
#include "file.h"
#include "navigation.h"
#include "ctrl_access.h"
#include "fsaccess.h"

#include "CCID/USART/ISO7816_USART.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/Local_ACCESS/OpenPGP_V20.h"

#include "BUFFERED_SIO.H"
#include "Interpreter.h"
#include "TIME_MEASURING.h"
#include "Inbetriebnahme.h"
#include "..\FILE_IO\FileAccessInterface.h"
#include "conf_usb.h"
#include "LED_test.h"
#include "DFU_test.h"
#include "..\USER_INTERFACE\file_io.h"
#include "OTP_test.h"
#include "..\HighLevelFunctions\MatrixPassword.h"
#include "..\HighLevelFunctions\HandleAesStorageKey.h"
#include "SD_Test.h"
#include "..\HighLevelFunctions\HiddenVolume.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

// #define PRINT_DELAY_FOR_DEBUGGING

#undef CI_DEBUG

#define CI_COMMAND_STORE_COUNT    30
#define CI_COMMAND_LINE_SIZE      20


/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

int XTS_TestMain(char *path_pu8,char *node_name_pu8);

/*******************************************************************************

 Local declarations

*******************************************************************************/

typedef struct {
   char *szCommand;
   char *szParameter;
   int   nCommandID;
   char *szHelpText;
} typeCommand;

typeCommand tCommandData[CI_MAX_COMMANDS] = 
{
    {   "help",     "",                                          CI_CMD_HELP,            "Display help", },
    {   "ts",       "[1 = heap info]",                           CI_CMD_TASKSTATUS,      "taskstatus",},
    {   "ro",       "(1-reset, 2-Interupt)",                     CI_CMD_RUNTIME_OVERVIEW,"Runtime", },
    {   "rs",       "slot",                                      CI_CMD_RUNTIME_SLOT,    "Slot-Runtime",},
    {   "ri",       "slot",                                      CI_CMD_RUNTIME_INT_SLOT,"Slot-Runt. Int.",},
    {   "ra",       "slotsize in usec",                          CI_CMD_SET_SLOTSIZE,    "Slotsize",},
    {   "fa",       "(no parameter for help)",                   CI_CMD_FILE_ACCESS,     "File access",},
    {   "mount",    "LUN",                                       CI_CMD_MOUNT,           "Mount Lun",},
    {   "dm",       "",                                          CI_CMD_SHOW_LUNS,       "Show mounted luns",},
    {   "df",       "",                                          CI_CMD_DISK_FREE,       "Show drive space",},
    {   "dir",      "[selection]",                               CI_CMD_DIR,             "dir",},
    {   "cl",       "LUN",                                       CI_CMD_SELECT_LUN,      "change lun",},
    {   "cd",       "PATH",                                      CI_CMD_CD,              "change directoy",},
    {   "del",      "FILENAME",                                  CI_CMD_DEL,             "delete file",},
    {   "touch",    "FILENAME",                                  CI_CMD_TOUCH,           "create file",},
    {   "ul",       "LUN",                                       CI_CMD_UPDATE_LUN,      "usb update lun",},
    {   "mkdir",    "DIR",                                       CI_CMD_MAKE_DIR,        "make directory",},
    {   "cat",      "Filename",                                  CI_CMD_CAT,             "cat",},
    {   "xtstest",  "",                                          CI_CMD_XTS_TEST,        "xtstest",},
    {   "ad",       "",                                          CI_CMD_AD_TEST,         "ad test",},
    {   "time",     "",                                          CI_CMD_TIME_TEST,       "time functions",},
    {   "usb",      "",                                          CI_CMD_USB,             "USB functions",},
    {   "sd",       "",                                          CI_CMD_SD,              "SD test functions",},
    {   "sc",       "",                                          CI_CMD_SC,              "SC test functions",},
    {   "html",     "",                                          CI_CMD_HTML,            "HTML test functions",},
    {   "hl",       "",                                          CI_CMD_HIGHLEVEL_TESTS, "Highlevel tests",},
    {   "led",      "",                                          CI_CMD_LED,             "LED tests",},
    {   "dfu",      "",                                          CI_CMD_DFU,             "DFU tests",},
    {   "int",      "",                                          CI_CMD_INTTIME,         "Interrupt infos",},
    {   "file",     "",                                          CI_CMD_FILE,            "File io test",},
    {   "otp",      "",                                          CI_CMD_OTP,             "OPT tests",},
    {   "pwm",      "",                                          CI_CMD_PWM,             "Password matrix",},
    {   "hv",       "",                                          CI_CMD_HIDDEN_VOULME,   "Hidden volumes",},
    { NULL,NULL,0,NULL}
};



char            szCommandBuffer[CI_COMMAND_STORE_COUNT][CI_COMMAND_LINE_SIZE];
unsigned char   cCommandBufferStart = 0;
unsigned char   cCommandBufferEnd   = 0;
unsigned char   cLastCommandPointer = 0;

/*******************************************************************************

  CI_StringOut

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int CI_StringOut (char *szText)
{
   BUFFERED_SIO_WriteString (strlen ((const char *)szText),(u8*)szText);

   BUFFERED_SIO_SendHandler();         // Start transmission of tx chars

   return (L_OK);
}

/*******************************************************************************

  CI_LocalPrintf

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int CI_LocalPrintf (char *szFormat,...)
{
   int nSize = 0;
   char szBuffer[MAX_TEXT_LENGTH];      // on task stack
   va_list args;

   va_start(args,szFormat);

   nSize = vsnprintf((char*)szBuffer,MAX_TEXT_LENGTH-1,szFormat,args);

   CI_StringOut (szBuffer);

   if (nSize >= MAX_TEXT_LENGTH-1) 
   {
      CI_StringOut ("OVERFLOW");
      return (L_ERROR);
   }

// Hack for debugging in step into mode
#ifdef PRINT_DELAY_FOR_DEBUGGING
   nSize = nSize / 4 + 1;
   DelayMs (nSize);
#endif
   return (L_OK);
}

/*******************************************************************************

  CI_TickLocalPrintf

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int CI_TickLocalPrintf (char *szFormat,...)
{
   int nSize = 0;
   char szBuffer[MAX_TEXT_LENGTH+10];       // on task stack
   va_list args;

   sprintf (szBuffer,"%7d ",(int)xTickCount);

   va_start(args,szFormat);

   nSize = vsnprintf((char*)&szBuffer[8],MAX_TEXT_LENGTH-1,szFormat,args);

   CI_StringOut (szBuffer);

   if (nSize >= MAX_TEXT_LENGTH-1)
   {
      CI_StringOut ("OVERFLOW");
      return (L_ERROR);
   }

// Hack for debugging in step into mode
#ifdef PRINT_DELAY_FOR_DEBUGGING
   nSize = (nSize+8) / 4 + 1;
   DelayMs (nSize);
#endif
   return (L_OK);
}

/*******************************************************************************

  CI_LocalPrintfNoDelay

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int CI_LocalPrintfNoDelay (char *szFormat,...)
{
   int nSize = 0;
   char szBuffer[MAX_TEXT_LENGTH];
   va_list args;

   va_start(args,szFormat);

   nSize = vsnprintf((char*)szBuffer,MAX_TEXT_LENGTH-1,szFormat,args);

   CI_StringOut (szBuffer);

   if (nSize >= MAX_TEXT_LENGTH-1)
   {
      CI_StringOut ("OVERFLOW");
      return (L_ERROR);
   }

   return (L_OK);
}

/*******************************************************************************

  CI_HardwareInit

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

static int CI_HardwareInit (void)
{
   return (L_OK);
}

/*******************************************************************************

  CI_CharIn

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

static int CI_CharIn (void)
{
   unsigned char Data_u8;

   BUFFERED_SIO_GetByte (&Data_u8);
   return ((int)Data_u8);
}

/*******************************************************************************

  CI_TestForCharIn

  L_ERROR    -1    no char in queue


  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

static int CI_TestForCharIn (void)
{
   int n;

   n = BUFFERED_SIO_ByteReceived ();   // Char receive
   if (0 != n)
   {
      return (L_OK);
   }

   return (L_ERROR);
}


/*******************************************************************************

  CI_Print8BitValue

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CI_Print8BitValue (unsigned char cValue)
{
    char cString[3];

    if (10 <= (cValue >> 4))
    {
        cString[0] = 'A' + (cValue >> 4) - 10;
    }
    else
    {
        cString[0] = '0' + (cValue >> 4);
    }

    if (10 <= (cValue & 0xF))
    {
        cString[1] = 'A' + (cValue & 0xF) - 10;
    }
    else
    {
        cString[1] = '0' + (cValue & 0xF);
    }
    cString[2] = 0;

    CI_StringOut (cString);
}

/*******************************************************************************

  CI_PrintIntro

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define HALCPU_REG8(reg)    *((volatile u8  *) (reg))

void CI_PrintIntro (void)
{

    CI_StringOut ("\n\rIDF Debugger V1.1 ");
    CI_StringOut (__DATE__" "__TIME__);
    CI_StringOut (", by R. Boeddeker\n\r(Enter HELP for help)\n\r");
    CI_StringOut ("\n\r");
}

/*******************************************************************************

  CI_CheckNewCommandLine

  RETURN       ERROR    kein Komando erhalten
               n        Länge der Komandozeile

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int CI_CheckNewCommandLine (char *szCommandLine)
{
    int i;

// No command entered
    if (0 == szCommandLine[0])
    {
        return (0);
    }

    for (i=0;i<CI_COMMAND_STORE_COUNT;i++)
    {
        if (0 == strcmp (szCommandLine,szCommandBuffer[i]))
        {
            return (0);
        }
    }
    return (1);
}

/*******************************************************************************

  CI_StoreCommandLine

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CI_StoreCommandLine (char *szCommandLine)
{
    if (0 == CI_CheckNewCommandLine (szCommandLine))
    {
        return;
    }
// Store command
    strcpy (szCommandBuffer[cCommandBufferEnd],szCommandLine);

// Get next buffer position
    cCommandBufferEnd++;
    if (CI_COMMAND_STORE_COUNT <= cCommandBufferEnd)
    {
        cCommandBufferEnd = 0;
    }

// Delete last entry ? 
    if (cCommandBufferEnd == cCommandBufferStart)
    {
        cCommandBufferStart++;
        if (CI_COMMAND_STORE_COUNT <= cCommandBufferStart)
        {
            cCommandBufferStart = 0;
        }
    }

    cLastCommandPointer = cCommandBufferStart;
}

/*******************************************************************************

  CI_GetStoredCommandLine

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

char *CI_GetStoredCommandLine (u8 Entry_u8)
{
    s8 EntryCount_s8;
    s8 ReturnEntry_s8;

// Get stored command line entry count
    EntryCount_s8 = (s8)cCommandBufferEnd - (s8)cCommandBufferStart;
    if (0 > EntryCount_s8)
    {
        EntryCount_s8 += CI_COMMAND_STORE_COUNT;
    }

// Get real entry no
    Entry_u8 %= EntryCount_s8;

// Get entry
    ReturnEntry_s8 = cCommandBufferEnd - Entry_u8 - 1;
    if (ReturnEntry_s8 < 0)
    {
        ReturnEntry_s8 += EntryCount_s8;
    }

    return (szCommandBuffer[ReturnEntry_s8]);
}

/*******************************************************************************

  CI_Clearline

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CI_Clearline (void)
{
    CI_StringOut ("\r                                            \r");
    CI_StringOut (CI_PROMPT);
}

/*******************************************************************************

  CI_GetCommandLine

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int CI_GetCommandLine (char *szCommandData, int nMaxTextLength)
{
   static int  nCharPointer = 0;
   static int  nOldCommandLinePointer = 0;
   int         n;

   while (nCharPointer < nMaxTextLength-1)
   {   
      if (L_ERROR == CI_TestForCharIn ())
      {

         return (L_ERROR);
      }

      n = CI_CharIn ();
      switch (n)
      {
         case 10 :               /* LF */
         case 13 :               /* CR */
            szCommandData[nCharPointer] = 0;
            n                           = nCharPointer;
            nCharPointer                = 0;
            nOldCommandLinePointer      = cLastCommandPointer; // 0
            // Store a new command
            CI_StoreCommandLine (szCommandData);
            return (n);

         case 8 :
            nCharPointer = strlen (szCommandData);
            if (0 < nCharPointer)
            {
                nCharPointer--;   
                szCommandData[nCharPointer] = 0;
                CI_Clearline ();
                CI_StringOut (szCommandData);
            }
            else
            {
                CI_StringOut (">"); // Hack to avoid delete prompt
            }
            break;

         case 27 :               /* ESC */
            nCharPointer = strlen (szCommandData);
            // Clear line
            CI_Clearline ();
            CI_StringOut (szCommandData);
            break;

         case 1:    // strg-a
            strcpy (szCommandData,CI_GetStoredCommandLine (nOldCommandLinePointer));
            nCharPointer = strlen (szCommandData);
            nOldCommandLinePointer++;
            CI_Clearline ();
            CI_StringOut (szCommandData);
            break;

         case 0x19: // strg-y
            strcpy (szCommandData,CI_GetStoredCommandLine (nOldCommandLinePointer));
            nCharPointer = strlen (szCommandData);
            nOldCommandLinePointer--;
            CI_Clearline ();
            CI_StringOut (szCommandData);
            break;

         case 23: // strg-w
                CI_StringOut ("\n\r");
            for (n=0;n<CI_COMMAND_STORE_COUNT;n++)
            {
                CI_Print8BitValue (n);
                CI_StringOut (" ");
                CI_StringOut (szCommandBuffer[n]);
                CI_StringOut ("\n\r");
            }
            CI_StringOut ("Pointer = ");
            CI_Print8BitValue (nOldCommandLinePointer);
            CI_StringOut ("\n\r");

            CI_StringOut (szCommandData);
            break;

         default :
		 	if ((' ' == n) || (0 != isprint (n)))
			{
            	szCommandData[nCharPointer] = n;    
            	nCharPointer++;
			}
            break;
      }
   }

   return (n);
}

/*******************************************************************************

  CI_Help

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CI_Help (void)
{
   int i;

   CI_StringOut ("Description Command Parameter\n\r");
   for (i=0;i<CI_MAX_COMMANDS;i++)
   {
      if (0 == tCommandData[i].nCommandID)
      {
         break;
      }
      CI_LocalPrintf ("%-20s %-5s %s\n\r",tCommandData[i].szHelpText,
                                          tCommandData[i].szCommand,                                          
                                          tCommandData[i].szParameter
                                          );
//      DelayMs (100);
   }
}

/*******************************************************************************

  CI_GetCommand

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define CI_MAX_COMMAND_LENGTH 10

int CI_GetCommand (u8 *szCommandLine,u8 **szCommandData)
{
   int i;
   int n;
   u8 szCommand[CI_MAX_COMMAND_LENGTH+1];

   i=0;
   while (' ' == szCommandLine[i])
   {
      i++;
   }

   for (;i<CI_MAX_COMMAND_LENGTH;i++)
   {
      if ((0 == szCommandLine[i]) || (' ' == szCommandLine[i]))
      {
         break;
      }
      szCommand[i] = szCommandLine[i];
   }
   szCommand[i] = 0;

/*   n = sscanf (szCommandLine,"%10s",szCommand);*/
   if (0 == i)
   {
      *szCommandData = szCommandLine;
      return (CI_CMD_NOTHING);
   }

   n = (int)strlen ((char*)szCommand);
   *szCommandData = &szCommandLine[n];

   for (i=0;i<CI_MAX_COMMANDS;i++)
   {
      if (0 == tCommandData[i].szCommand[0])
      {
         break;
      }

      if (0 == strncmp (tCommandData[i].szCommand,(char*)szCommand,n))
      {
         return (tCommandData[i].nCommandID);
      }
   }

   return (CI_CMD_NOTHING);
}

/*******************************************************************************

  CI_IsNumber

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

s8 CI_IsNumber (u8 *String_pu8)
{
	// jump over leading spaces
	while (' ' == *String_pu8)
	{
		String_pu8++;
	}

	// End of line ?
	if (0 == *String_pu8)
	{
		return (FALSE);
	}

	// Scan for number
	while ((0 != *String_pu8) && (' ' != *String_pu8))
	{
		if ((*String_pu8 < '0') || (*String_pu8 > '9' ))
		{
			return (FALSE);
		}
		String_pu8++;
	}
	return (TRUE);
}

/*******************************************************************************

  CI_GetString

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 *CI_GetString (u8 *CMD_pu8,u8 **StartParameter_pu8)
{
	// jump over leading spaces
	while (' ' == *CMD_pu8)
	{
		CMD_pu8++;
	}

	// End of line ?
	if (0 == *CMD_pu8)
	{
		return (CMD_pu8);
	}

	*StartParameter_pu8 = CMD_pu8;

	// Scan for string
	while ((0 != *CMD_pu8) && (' ' != *CMD_pu8))
	{
		CMD_pu8++;
	}

	// End of line ?
	if (0 == *CMD_pu8)
	{
		return (CMD_pu8);
	}

	// Set end of string of parameter
	*CMD_pu8 = 0;
	CMD_pu8++;

	return (CMD_pu8);
}

/*******************************************************************************

  CI_ExecCmd

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define MAX_CMD_VALUES  5
#define MAX_CMD_STRINGS 3

#ifdef TIME_MEASURING_ENABLE
    extern void TIME_MEASURING_Print (u8 Value_u8);
    extern void TIME_MEASURING_PrintRuntimeSlots(u8 Entry_u8);
#endif

int CI_ExecCmd (char *szCommandLine)
{
    unsigned long   nValue[MAX_CMD_VALUES];
    u8             *String_pu8[MAX_CMD_STRINGS];
    unsigned long   nCmd;
    u8              nParamsGet_u8;
#ifdef TIME_MEASURING_ENABLE
    u32             Runtime_u32;
#endif
    long            i;
    u8             *p;
    u8             *p1;
    u8             *szCommandData;
    u8 				StringPointer_u8;
    u8 				End_u8;

    nCmd = CI_GetCommand ((u8*)szCommandLine,(u8**)&szCommandData);

    if (CI_CMD_NOTHING == nCmd)
    {
        if (0 != strlen (szCommandLine))
        {
            CI_LocalPrintf ("Can't understand -%s-\n\r",szCommandLine);
        }
        return (L_OK);
    }
   
// Clear old command values
    nParamsGet_u8 = 0;
    for (i=0;MAX_CMD_VALUES > i;i++)
    {
        nValue[i] =  0;
    }

    StringPointer_u8 = 0;
    for (i=0;MAX_CMD_STRINGS > i;i++)
    {
    	String_pu8[i] =  NULL;
    }

// read parameter
    p             = szCommandData;
    End_u8        = FALSE;
    while (FALSE == End_u8)
    {
    	if (TRUE == CI_IsNumber (p))
    	{
        // Get number
        nValue[nParamsGet_u8] = (unsigned long)strtol ((char*)p,(char**)&p1,10);
        if (p1 == p)		// End of line ?
        {
          End_u8 = TRUE;
          break;
        }

        p = p1;

        nParamsGet_u8++;
        if (MAX_CMD_VALUES <= nParamsGet_u8)	// End of used params ?
        {
          End_u8 = TRUE;
          break;
        }
    	}
    	else
    	{
    		// Get string
    		p = CI_GetString (p,&String_pu8[StringPointer_u8]);
    		StringPointer_u8++;
        if (MAX_CMD_STRINGS <= nParamsGet_u8)	// End of used params ?
        {
          End_u8 = TRUE;
          break;
        }
    	}
    	// End of data ?
    	if (0 == *p)
    	{
    		break;
    	}
    }

#ifdef CI_DEBUG
   printf ("\nCI_ExecCmd : ID %2d - Parameter: \n",nCmd);
   for (i=0;nParamsGet>i;i++)
   {
      printf ("0x%X ",nValue[i]);
   }
   printf ("\n");
#endif

#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Start (TIME_MEASURING_TIME_CMD);
#endif

   switch (nCmd)
   {
      case CI_CMD_HELP :
         CI_Help ();
         break;

      case CI_CMD_TASKSTATUS :
         IBN_ShowTaskStatus ((u8)nValue[0]);
         break;

      case CI_CMD_RUNTIME_OVERVIEW :
#ifdef TIME_MEASURING_ENABLE
         TIME_MEASURING_Print ((u8)nValue[0]);
#else
         CI_LocalPrintf ("TIME_MEASURING not active\n\r");
#endif
         break;

      case CI_CMD_SET_SLOTSIZE :
#ifdef TIME_MEASURING_ENABLE
         TIME_MEASURING_SetRuntimeSliceSize (nValue[0]);
#else
         CI_LocalPrintf ("TIME_MEASURING not active\n\r");
#endif
          break;

      case CI_CMD_RUNTIME_SLOT :
#ifdef TIME_MEASURING_ENABLE
         TIME_MEASURING_PrintRuntimeSlots ((u8)nValue[0]);
#else
         CI_LocalPrintf ("TIME_MEASURING not active\n\r");
#endif
         break;

      case CI_CMD_RUNTIME_INT_SLOT :
#ifdef TIME_MEASURING_ENABLE
         TIME_MEASURING_PrintIntRuntimeSlots ((u8)nValue[0]);
#else
         CI_LocalPrintf ("TIME_MEASURING not active\n\r");
#endif
         break;

      case CI_CMD_FILE_ACCESS :
#ifdef ENABLE_IBN_FILE_ACCESS_TESTS
          IBN_FileAccess (nParamsGet_u8,(u8)nValue[0],nValue[1]);
#else
          CI_LocalPrintf ("Test not enabled - activate ENABLE_IBN_FILE_ACCESS_TESTS\n\r");
#endif

         break;

      case CI_CMD_MOUNT:
    	  if (1 != nParamsGet_u8)
    	  {
    		  CI_LocalPrintf ("USAGE: mount [lun]\n\r");
    		  break;
    	  }
 		  FAI_mount((u8)nValue[0]);
    	  break;

      case CI_CMD_SHOW_LUNS :
		  FAI_nb_drive();
		  break;

      case CI_CMD_DISK_FREE :
  		  FAI_free_space((u8)nValue[0]);
    	  break;

      case CI_CMD_DIR :
 		   FAI_Dir (0);
    	  break;

	  case CI_CMD_SELECT_LUN :
    	  if (1 != nParamsGet_u8)
    	  {
    		  CI_LocalPrintf ("USAGE: cl [lun]\n\r");
    		  break;
    	  }
		  if (TRUE == nav_drive_set ((u8)nValue[0]))
		  {
			   CI_LocalPrintf ("cl OK\n\r");
		  }
		  else
		  {
			  CI_LocalPrintf ("cl FAIL - %d - %s\n\r",fs_g_status, IBN_FileSystemErrorText(fs_g_status));
		  }
		  break;

    case CI_CMD_CD :
        if (1 != StringPointer_u8)
        {
          CI_LocalPrintf ("USAGE: cd path (Yet, no numbers)\n\r");
          break;
        }
      FAI_cd(String_pu8[0]);
      break;

    case CI_CMD_DEL :
        if (1 != StringPointer_u8)
        {
          CI_LocalPrintf ("USAGE: del filename (Yet, no numbers)\n\r");
          break;
        }
        FAI_DeleteFile(String_pu8[0]);
      break;



	  case CI_CMD_TOUCH :
    	  if (1 != StringPointer_u8)
    	  {
    		  CI_LocalPrintf ("USAGE: touch filename (Yet, no numbers)\n\r");
    		  break;
    	  }
    	  FAI_touch (String_pu8[0]);
		  break;

	  case CI_CMD_UPDATE_LUN :
    	  if (1 != nParamsGet_u8)
    	  {
    		  CI_LocalPrintf ("USAGE: ul lun\n\r");
    		  break;
    	  }
		  FAI_MarkLunAsChanged ((u8)nValue[0]);
		  break;

	  case CI_CMD_MAKE_DIR :
    	  if (1 != StringPointer_u8)
    	  {
    		  CI_LocalPrintf ("USAGE: mkdir filename (Yet, no numbers)\n\r");
    		  break;
    	  }
    	  FAI_mkdir (String_pu8[0]);
		  break;

	  case CI_CMD_CAT :
    	  if (1 != StringPointer_u8)
    	  {
    		  CI_LocalPrintf ("USAGE: cat filename (Yet, no numbers)\n\r");
    		  break;
    	  }
		  FAI_cat(String_pu8[0],(u8)nValue[0]);
		  break;

	  case CI_CMD_XTS_TEST :
    	  if (2 != StringPointer_u8)
    	  {
    		  CI_LocalPrintf ("USAGE: input_directory mode_name\n\r");
    		  break;
    	  }
//		  XTS_TestMain(String_pu8[0],String_pu8[1]);
		  break;

	  case CI_CMD_AD_TEST :
		  IBN_LogADInput (nParamsGet_u8,(u8)nValue[0],nValue[1]);
		  break;

	  case CI_CMD_TIME_TEST :
#ifdef ENABLE_IBN_TIME_ACCESS_TESTS
      IBN_TimeAccess (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
#else
      CI_LocalPrintf ("Test not enabled - activate ENABLE_IBN_TIME_ACCESS_TESTS\n\r");
#endif
		  break;

    case CI_CMD_USB :
      IBN_USB_Stats (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
      break;
    case CI_CMD_SD :
      IBN_SD_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],nValue[2],nValue[3]);
      break;
    case CI_CMD_SC :
      IBN_SC_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
      break;
    case CI_CMD_HTML :
      IBN_HTML_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
      break;
    case CI_CMD_HIGHLEVEL_TESTS :
      HighLevelTests (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
      break;
    case CI_CMD_LED :
      IBN_LED_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
      break;
    case CI_CMD_DFU :
      IBN_DFU_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
      break;
    case CI_CMD_INTTIME :
      TIME_MEASURING_INT_Infos (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
      break;
    case CI_CMD_FILE :
      IBN_FileIo_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],nValue[2],nValue[3]);
      break;
    case CI_CMD_OTP :
      IBN_OTP_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
      break;
    case CI_CMD_PWM :
#ifdef ENABLE_IBN_PWM_TESTS
      IBN_PWM_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
#else
      CI_LocalPrintf ("Test not enabled - activate ENABLE_IBN_PWM_TESTS\n\r");
#endif
      break;
    case CI_CMD_HIDDEN_VOULME :
#ifdef ENABLE_IBN_HV_TESTS
      IBN_HV_Tests (nParamsGet_u8,(u8)nValue[0],nValue[1],String_pu8[0]);
#else
      CI_LocalPrintf ("Test not enabled - activate ENABLE_IBN_HV_TESTS\n\r");
#endif
      break;
   }

#ifdef TIME_MEASURING_ENABLE
   Runtime_u32 = TIME_MEASURING_Stop (TIME_MEASURING_TIME_CMD);
   CI_LocalPrintf ("Command runtime %ld usec\n\r",Runtime_u32/TIME_MEASURING_TICKS_IN_USEC);
#endif

   return (L_OK);
}

/*******************************************************************************

  IDF_PrintStartupInfo

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IDF_PrintStartupInfo (void)
{
  CI_PrintIntro ();

#ifdef STICK_20_A_MUSTER_PROD
  CI_StringOut ("*** PROD version active ***\r\n");
#else
  CI_StringOut ("DEV version active\r\n");
#endif

#ifdef  SIMULATE_USB_CCID_DISPATCH
  CI_StringOut ("SIMULATE_USB_CCID_DISPATCH active\r\n");
#endif

#if USB_HIGH_SPEED_SUPPORT == ENABLED
  CI_StringOut ("USB high speed activ\r\n");
#else
  CI_StringOut ("USB low speed activ\r\n");
#endif

#ifdef  USB_MSD
  CI_StringOut ("MSD active\r\n");
#endif

#ifdef  USB_CCID
  CI_StringOut ("CCID active\r\n");
#endif

#ifdef  USB_KB
  CI_StringOut ("HID Keyboard active\r\n");
#endif
}

/*******************************************************************************

  IDF_Debugtool

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

s32 IDF_Debugtool (void)
{
   static char    szCommandData[MAX_TEXT_LENGTH];
   static char    cCI_State = CI_CMD_STATE_START;

   if (CI_CMD_STATE_START == cCI_State)
   {
      CI_HardwareInit ();
      cCI_State = CI_CMD_STATE_PRINT_PROMT;

      IDF_PrintStartupInfo ();


   }

   if (CI_CMD_STATE_PRINT_PROMT == cCI_State)
   {
      CI_StringOut (CI_PROMPT);
      cCI_State = CI_CMD_STATE_WAIT_FOR_CMD;
      // Clear command buffer
      szCommandData[0] = 0;  
   }

   if (L_ERROR == CI_GetCommandLine (szCommandData,MAX_TEXT_LENGTH))
   {
      return (L_OK);                                 /* kein Komando vorhanden */
   }

   CI_StringOut ("\n\r");                          /* Neue Zeile für Komandobearbeitung */

   CI_ExecCmd (szCommandData);

   cCI_State = CI_CMD_STATE_PRINT_PROMT;

   return (L_OK);
}

/*******************************************************************************

  IDF_task

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

extern U32 sd_MaxAccessedBlock_u32;

void IDF_task(void *pvParameters)
{
	u32 LoopCount_u32 = 0;
	portTickType xLastWakeTime;
	u32 sd_LastAccessedBlock_u32 = 0;

	xLastWakeTime = xTaskGetTickCount();
//	while (1) ;

	while (TRUE)
	{
		vTaskDelayUntil(&xLastWakeTime, configTSK_IDF_TEST_PERIOD);

    if(0 == LoopCount_u32 % 100)      // Every second
    {
      if (sd_LastAccessedBlock_u32 < sd_MaxAccessedBlock_u32)
      {
        CI_LocalPrintf ("Highest sd block accessed = %ld = %ld MB\n\r",sd_MaxAccessedBlock_u32,sd_MaxAccessedBlock_u32/2000);

        sd_LastAccessedBlock_u32 = sd_MaxAccessedBlock_u32;
      }
    }

#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Start (TIME_MEASURING_TIMER_IDF_10MS);
#endif
	   LoopCount_u32++;
	   if(0 == LoopCount_u32 % 100)
	   {
		   IBN_GetTaskList ();
	   }
	   IDF_Debugtool ();

#ifdef TIME_MEASURING_ENABLE
   TIME_MEASURING_Stop (TIME_MEASURING_TIMER_IDF_10MS);
#endif
	}
}

/*******************************************************************************

  IDF_task_init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IDF_task_init(void)
{
//	BUFFERED_SIO_Init ();

	xTaskCreate(IDF_task,
			    configTSK_IDF_TEST_NAME,
			    configTSK_IDF_TEST_STACK_SIZE,
			    NULL,
			    configTSK_IDF_TEST_PRIORITY,
			    NULL);
}



#else

/*******************************************************************************

  Dummy entrys

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int CI_TickLocalPrintf (char *szFormat,...)
{
    return (0);
}

int CI_LocalPrintf (char *szFormat,...)
{
    return (0);
}

int CI_StringOut (unsigned char *szText)
{
    return (0);
}

int IDF_Debugtool (void)
{
    return (0);
}

void CI_Print8BitValue (unsigned char n)
{
}

#endif // INTERPRETER_ENABLE
