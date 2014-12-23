/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 09.04.2011
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

/* Depends on the AVR framework */

/*
 * Inbetriebnahme.c
 *
 *  Created on: 09.04.2011
 *      Author: RB
 */

#include "global.h"
#include "tools.h"
#include "FreeRTOS.h"
#include "task.h"
#include "malloc.h"
#include "string.h"

#include "conf_explorer.h"
/* FAT includes */
#include "file.h"
#include "navigation.h"
#include "ctrl_access.h"
#include "fsaccess.h"

#include "sd_mmc_mci.h"

#include "time.h"

#include "TIME_MEASURING.h"
#include "Inbetriebnahme.h"
#include "..\FILE_IO\FileAccessInterface.h"
#include "AD_test.h"
#include "polarssl/timing.h"
#include "ctrl_access.h"
#include "USER_INTERFACE/html_io.h"
#include "SD_Test.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

#define IBN_MAX_TASK_LIST       10

#define IBN_TASK_STATE_RUNNING          0
#define IBN_TASK_STATE_DELAYED          1
#define IBN_TASK_STATE_DELAYED_OVER     2
#define IBN_TASK_STATE_SUSPEND          3

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

tskTCB *IBN_Tasklist_st[IBN_MAX_TASK_LIST];
u8      IBN_TasklistState_u8[IBN_MAX_TASK_LIST];
u8      IBN_TasklistCounter_u8 = 0;

const char *IBN_TaskStates[] = {
    "RUNNING",
    "DELAYED",
    "OV_DELA",
    "SUSPEND"
};

/*******************************************************************************

  IBN_GetTaskListEntrys

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_GetTaskListEntrys (xList *pxList,u8 State_u8)
{
	volatile tskTCB *pxNextTCB, *pxFirstTCB;

	listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList);
	do
	{
		listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
		IBN_Tasklist_st[IBN_TasklistCounter_u8]      = (tskTCB *)pxNextTCB;
		IBN_TasklistState_u8[IBN_TasklistCounter_u8] = State_u8;
		if (IBN_MAX_TASK_LIST <= IBN_TasklistCounter_u8)
		{
			return;
		}
		IBN_TasklistCounter_u8++;
	} while( pxNextTCB != pxFirstTCB );
}

/*******************************************************************************

  IBN_GetTaskList

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_GetTaskList (void)
{
	u32 Queue_u32;

	IBN_TasklistCounter_u8 = 0;

// Scan all running queues
	portENTER_CRITICAL();

// Scan ready lists
	Queue_u32 = uxTopUsedPriority + 1;
	do
	{
		Queue_u32--;
		if( !listLIST_IS_EMPTY( &( pxReadyTasksLists[ Queue_u32 ] ) ) )
		{
			IBN_GetTaskListEntrys ( &(pxReadyTasksLists[ Queue_u32 ]),IBN_TASK_STATE_RUNNING);
		}
	} while( Queue_u32 > ( unsigned short ) tskIDLE_PRIORITY );


	if( !listLIST_IS_EMPTY( pxDelayedTaskList ) )
	{
		IBN_GetTaskListEntrys (pxDelayedTaskList,IBN_TASK_STATE_DELAYED);
	}

	if( !listLIST_IS_EMPTY( pxOverflowDelayedTaskList ) )
	{
		IBN_GetTaskListEntrys (pxOverflowDelayedTaskList,IBN_TASK_STATE_DELAYED_OVER);
	}

	if( !listLIST_IS_EMPTY( &xSuspendedTaskList ) )
	{
		IBN_GetTaskListEntrys (&xSuspendedTaskList,IBN_TASK_STATE_SUSPEND);
	}

	portEXIT_CRITICAL();

}

/*******************************************************************************

  IBN_StoreTaskRuntimeData

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_StoreTaskRuntimeData (void)
{
  u8 i;

  for (i=0;i<IBN_TasklistCounter_u8;i++)
  {
    IBN_Tasklist_st[i]->ullRunTimeCounterLastMinute = IBN_Tasklist_st[i]->ullRunTimeCounterMinute;
    IBN_Tasklist_st[i]->ullRunTimeCounterMinute     = 0ULL;
  }
}

/*******************************************************************************

  IBN_Taskdata

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
extern  unsigned long long  TIME_MEASURING_Void ();

void IBN_Taskdata (u8 i)
{
	u64 ullTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
  u64 ullStatsAsPercentage;
	u16 usStackRemaining;

	usStackRemaining = usTaskCheckFreeStackSpace( ( unsigned char * ) IBN_Tasklist_st[i]->pxStack ) * sizeof (portSTACK_TYPE);
	ullStatsAsPercentage       = (u64)0;

	if ((u64)0 != ullTotalRunTime)
	{
		ullStatsAsPercentage  =  (u64)10000 * (u64)(IBN_Tasklist_st[i]->ullRunTimeCounter);
		ullStatsAsPercentage /=  ullTotalRunTime;

	}

	CI_LocalPrintf( ( char * ) "%-21.21s ", IBN_Tasklist_st[i]->pcTaskName);
	CI_LocalPrintf( ( char * ) "%s   ", IBN_TaskStates[IBN_TasklistState_u8[i]]);
	CI_LocalPrintf( ( char * ) "%u  ", ( unsigned int ) IBN_Tasklist_st[i]->uxPriority);
	CI_LocalPrintf( ( char * ) "%5u ", usStackRemaining);
//	CI_LocalPrintf( ( char * ) "%10u ", (u32)(IBN_Tasklist_st[i]->ullRunTimeCounter / 60ULL));
	CI_LocalPrintf("%s ", TIME_MEASURING_ShortText (IBN_Tasklist_st[i]->ullRunTimeCounter / 60ULL));

  CI_LocalPrintf( ( char * ) "%6.2f %% ",(float)ullStatsAsPercentage/100.0);
  CI_LocalPrintf( ( char * ) "%6.2f %% ",(float)IBN_Tasklist_st[i]->ullRunTimeCounterLastMinute/(10000.0*60.0*60.0)*(float)(configTICK_RATE_HZ)/1000.0);
	CI_LocalPrintf( ( char * ) "%8u \r\n", IBN_Tasklist_st[i]->ulRunCounter);
}

/*******************************************************************************

  IBN_ListTaskWithinSingleList

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_ListTaskWithinSingleList (xList *pxList, char *pcStatus )
{
	volatile tskTCB *pxNextTCB, *pxFirstTCB;
	u16 usStackRemaining;

	u64 ullTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
	u64 ullStatsAsPercentage;

	/* Write the details of all the TCB's in pxList into the buffer. */
	listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
	do
	{
		listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );

		usStackRemaining = usTaskCheckFreeStackSpace( ( unsigned char * ) pxNextTCB->pxStack );
		ullStatsAsPercentage = (u64)0;
		if ((u64)0 != ullTotalRunTime)
		{
			ullStatsAsPercentage  =  (u64)10000 * (u64)(pxNextTCB->ullRunTimeCounter);
			ullStatsAsPercentage /=  ullTotalRunTime;
		}

		CI_LocalPrintf("%-21.21s ", pxNextTCB->pcTaskName);
		CI_LocalPrintf("%s   ", pcStatus);
		CI_LocalPrintf("%u  ", ( unsigned int ) pxNextTCB->uxPriority);
		CI_LocalPrintf("%5u ", usStackRemaining);
		CI_LocalPrintf("%10u ", (u32)(pxNextTCB->ullRunTimeCounter / 60ULL));
		CI_LocalPrintf("%s ", TIME_MEASURING_ShortText (pxNextTCB->ullRunTimeCounter / 60ULL));

		CI_LocalPrintf("%6.2f %% ",(float)ullStatsAsPercentage/100.0);
		CI_LocalPrintf("%7u \r\n", pxNextTCB->ulRunCounter);

//		CI_LocalPrintf( ( char * ) "%-21.21s %s   %u  %5u %9u %6.2f %%\r\n", pxNextTCB->pcTaskName, pcStatus, ( unsigned int ) pxNextTCB->uxPriority, usStackRemaining,pxNextTCB->ullRunTimeCounter/60,(float)ullStatsAsPercentage/100.0);
	} while( pxNextTCB != pxFirstTCB );
}

/*******************************************************************************

  IBN_ShowTaskStatusAll

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_ShowTaskStatusAll (void)
{
	u32 i;


	CI_LocalPrintf ("Taskstatus\r\n");
	CI_LocalPrintf ("\r\n");
	CI_LocalPrintf ("Taskcount : %d\r\n",uxTaskGetNumberOfTasks ());
	CI_LocalPrintf ("Tick      : %d - %d sec\r\n",xTickCount,xTickCount/configTICK_RATE_HZ);
	CI_LocalPrintf ("                                   Stack      Runtime      Last Minute\r\n");
	CI_LocalPrintf ("Taskname              State   Prio  free    usec       %%        %%      Starts\r\n");

	for (i=0;i<IBN_TasklistCounter_u8;i++)
	{
		IBN_Taskdata (i);
	}

}

/*******************************************************************************

  IBN_ShowHeadinfo

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_ShowHeadinfo (void)
{
#ifdef INTERPRETER_ENABLE
	extern void __heap_start__;
	extern void __heap_end__;
	struct mallinfo info = mallinfo();

	CI_LocalPrintf ("HEAP start     0x%08x\r\n",&__heap_start__);
	CI_LocalPrintf ("HEAP end       0x%08x\r\n",&__heap_end__);
	CI_LocalPrintf ("HEAP max size  %d\r\n",&__heap_end__-&__heap_start__);
	CI_LocalPrintf ("HEAP max used  %d\r\n",info.arena);
	CI_LocalPrintf ("HEAP used      %d\r\n",info.usmblks + info.uordblks);
	CI_LocalPrintf ("HEAP free      %d\r\n",(&__heap_end__ - &__heap_start__) - (info.usmblks + info.uordblks));
#endif
}

/*******************************************************************************

  IBN_ShowTaskStatus

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_ShowTaskStatus (u8 CMD_u8)
{
//	u8 i;

	switch (CMD_u8)
	{
		case 0 :
			IBN_ShowTaskStatusAll ();
			break;

		case 1 :
			IBN_ShowHeadinfo ();
			break;

	}
}

/*******************************************************************************

  IBN_FileSystemErrorText

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

char *IBN_FileSystemErrorText (u8 FS_Error_u8)
{
	switch (FS_Error_u8)
	{
		case FS_ERR_HW               : return ("FS_ERR_HW               Hardware driver error" );
		case FS_ERR_NO_FORMAT        : return ("FS_ERR_NO_FORMAT        The selected drive isn't formated" );
		case FS_ERR_NO_PART          : return ("FS_ERR_NO_PART          The selected partition doesn't existed" );
		case FS_ERR_NO_SUPPORT_PART  : return ("FS_ERR_NO_SUPPORT_PART  The selected partition isn't supported" );
		case FS_ERR_TOO_FILE_OPEN    : return ("FS_ERR_TOO_FILE_OPEN    The navigation have already opened a file" );
		case FS_ERR_END_OF_DRIVE     : return ("FS_ERR_END_OF_DRIVE     There are not other driver" );
		case FS_ERR_BAD_POS          : return ("FS_ERR_BAD_POS          The position is over the file" );
		case FS_ERR_FS               : return ("FS_ERR_FS               File system error" );
		case FS_ERR_NO_FIND          : return ("FS_ERR_NO_FIND          File no found" );
		case FS_ERR_ENTRY_EMPTY      : return ("FS_ERR_ENTRY_EMPTY      File entry empty" );
		case FS_ERR_ENTRY_BAD        : return ("FS_ERR_ENTRY_BAD        File entry bad" );
		case FS_ERR_ENTRY_BADTYPE    : return ("FS_ERR_ENTRY_BADTYPE    File entry type don't corresponding" );
		case FS_ERR_NO_DIR           : return ("FS_ERR_NO_DIR           The selected file isn't a directory" );
		case FS_ERR_NO_MOUNT         : return ("FS_ERR_NO_MOUNT         The partition isn't mounted" );
		case FS_ERR_NO_FILE_SEL      : return ("FS_ERR_NO_FILE_SEL      There are no selected file" );
		case FS_NO_LAST_LFN_ENTRY    : return ("FS_NO_LAST_LFN_ENTRY    The file entry isn't the last long file entry" );
		case FS_ERR_ID_FILE          : return ("FS_ERR_ID_FILE          The file identifier is bad" );
		case FS_ERR_NO_FILE          : return ("FS_ERR_NO_FILE          The selected file entry isn't a file" );
		case FS_LUN_WP               : return ("FS_LUN_WP               Drive is in read only mode" );
		case FS_ERR_READ_ONLY        : return ("FS_ERR_READ_ONLY        File is on read access only" );
		case FS_ERR_NAME_INCORRECT   : return ("FS_ERR_NAME_INCORRECT   The name don't corresponding at the filter name" );
		case FS_ERR_FILE_NO_OPEN     : return ("FS_ERR_FILE_NO_OPEN     No file is opened" );
		case FS_ERR_HW_NO_PRESENT    : return ("FS_ERR_HW_NO_PRESENT    Device is not present" );
		case FS_ERR_IS_ROOT          : return ("FS_ERR_IS_ROOT          There aren't parent because the current directory is a root directory" );
		case FS_ERR_OUT_LIST         : return ("FS_ERR_OUT_LIST         The position is outside the cluster list" );
		case FS_ERR_NO_FREE_SPACE    : return ("FS_ERR_NO_FREE_SPACE    No free cluster found in FAT" );
		case FS_ERR_INCORRECT_NAME   : return ("FS_ERR_INCORRECT_NAME   Incorrect name, this one cannot contain any of the following characters" );
		case FS_ERR_DIR_NOT_EMPTY    : return ("FS_ERR_DIR_NOT_EMPTY    This function erases only file and empty directory" );
		case FS_ERR_WRITE_ONLY       : return ("FS_ERR_WRITE_ONLY       File is on write access only" );
		case FS_ERR_MODE_NOAVIALABLE : return ("FS_ERR_MODE_NOAVIALABLE This open mode isn't available" );
		case FS_ERR_EOF              : return ("FS_ERR_EOF              End of file" );
		case FS_ERR_BAD_SIZE_FAT     : return ("FS_ERR_BAD_SIZE_FAT     The disk size is not supported by selected FAT format" );
		case FS_ERR_COMMAND          : return ("FS_ERR_COMMAND          This command is not supported" );
		case FS_ERR_BUFFER_FULL      : return ("FS_ERR_BUFFER_FULL      Buffer is too small" );
		case FS_ERR_COPY_DIR         : return ("FS_ERR_COPY_DIR         Directory copy is not supported" );
		case FS_ERR_COPY_RUNNING     : return ("FS_ERR_COPY_RUNNING     A copy action is always running" );
		case FS_ERR_COPY_IMPOSSIBLE  : return ("FS_ERR_COPY_IMPOSSIBLE  The copy is impossible" );
		case FS_ERR_BAD_NAV          : return ("FS_ERR_BAD_NAV          The navigator identifier doesn't existed" );
		case FS_ERR_FILE_OPEN        : return ("FS_ERR_FILE_OPEN        The file is already opened" );
		case FS_ERR_FILE_OPEN_WR     : return ("FS_ERR_FILE_OPEN_WR     The file is already opened in write mode" );
		case FS_ERR_FILE_EXIST       : return ("FS_ERR_FILE_EXIST       The file is already existed" );
		case FS_ERR_NAME_TOO_LARGE   : return ("FS_ERR_NAME_TOO_LARGE   The file name is too large (>260 characters)" );
		case FS_ERR_DEVICE_TOO_SMALL : return ("FS_ERR_DEVICE_TOO_SMALL The disk size is too small for format routine" );
		case FS_ERR_PL_NOT_OPEN      : return ("FS_ERR_PL_NOT_OPEN      The play list isn't opened" );
		case FS_ERR_PL_ALREADY_OPEN  : return ("FS_ERR_PL_ALREADY_OPEN  The play list is already opened" );
		case FS_ERR_PL_LST_END       : return ("FS_ERR_PL_LST_END       You are at the end of play list" );
		case FS_ERR_PL_LST_BEG       : return ("FS_ERR_PL_LST_BEG       You are at the beginning of play list" );
		case FS_ERR_PL_OUT_LST       : return ("FS_ERR_PL_OUT_LST       You are outside of the play list" );
		case FS_ERR_PL_READ_ONLY     : return ("FS_ERR_PL_READ_ONLY     Impossible to modify the play list" );
		default :
			return ("***Unknown FS  error ***");
			break;
	}
}

/*******************************************************************************

  IBN_FileAccess

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

extern Ctrl_status sd_mmc_mci_unit_state_e;


#ifdef ENABLE_IBN_FILE_ACCESS_TESTS
void IBN_FileAccess (u8 nParamsGet_u8,u8 CMD_u8,u32 Param_u32)
{
	s32 FileID_s32;
	s32 Ret_s32;

//	testtt[1000] = 4;

	if (0 == nParamsGet_u8)
	{
		CI_LocalPrintf ("File access\r\n");
		CI_LocalPrintf (" 0 = Set lun 0\r\n");
		CI_LocalPrintf (" 1 = Mount\r\n");
    CI_LocalPrintf (" 2 = Show free space\r\n");
    CI_LocalPrintf ("11 = Set BUSY Lun 0\r\n");
    CI_LocalPrintf ("12 = Update contend lun X\r\n");
    CI_LocalPrintf ("16 = Copy test.txt to test1.txt\n\r");
    CI_LocalPrintf ("17 = Format LUN X\n\r");
		return;
	}

	switch (CMD_u8)
	{
   	   case 0 :
			fs_g_nav.u8_lun = 0;   // On chip RAM
			break;
#if LUN_2 == ENABLE
   	   case 1 :
			if (2 != nParamsGet_u8)
			{
				CI_LocalPrintf ("USAGE: fa 1 [lun]\r\n");
				break;
			}
			b_fsaccess_init ();

			fs_g_nav.u8_lun = Param_u32;   // On chip RAM

			virtual_test_unit_ready() ;

			if (TRUE == fat_mount ())
			{
				CI_LocalPrintf ("Mount LUN %d OK\r\n",Param_u32);
			}
			else
			{
				CI_LocalPrintf ("Mount LUN %d FAIL - %d - %s\r\n",Param_u32,fs_g_status, IBN_FileSystemErrorText(fs_g_status));
			}
//			nav_partition_mount ();
			break;
#endif
   	   case 2 :
			CI_LocalPrintf ("Free mem = %d sectors\r\n",fat_getfreespace());
   		    break;
   	   case 3 :

   		    FileID_s32 = open ("test.txt",O_CREAT|O_RDWR);
			CI_LocalPrintf ("Open  = %d - %d - %s\r\n",FileID_s32,fs_g_status, IBN_FileSystemErrorText(fs_g_status));

			Ret_s32 = write (FileID_s32,"Test\n",6);
			CI_LocalPrintf ("Write  = %d \r\n",Ret_s32);
/*
			if (TRUE == fat_cache_flush ())
			{
				CI_LocalPrintf ("fat_cache_flush OK\r\n");
			}
			else
			{
				CI_LocalPrintf ("fat_cache_flush FAIL - %d - %s\r\n",fs_g_status, IBN_FileSystemErrorText(fs_g_status));
			}
*/
			Ret_s32 = close (FileID_s32);
			CI_LocalPrintf ("Close  = %d \r\n",Ret_s32);

#if LUN_2 == ENABLE
			virtual_unit_state_e = CTRL_BUSY;    // only for ram disk
#endif
			sd_mmc_mci_unit_state_e = CTRL_BUSY;
			vTaskDelay (500);
#if LUN_2 == ENABLE
			virtual_unit_state_e = CTRL_GOOD;
#endif
      sd_mmc_mci_unit_state_e = CTRL_GOOD;

   		    break;
   	   case 4 :
			if (2 != nParamsGet_u8)
			{
				CI_LocalPrintf ("USAGE: fa 6 [lun]\r\n");
				break;
			}

   		    FAI_Write (Param_u32);
			break;

   	   case 5 :
			FAI_ShowDirContent ();
   		    break;
   	   case 6:
			if (2 != nParamsGet_u8)
			{
				CI_LocalPrintf ("USAGE: fa 6 [lun]\r\n");
				break;
			}
   		   FAI_mount(Param_u32);
		   break;
   	   case 7 :
   		   FAI_free_space(Param_u32);
   		   break;
   	   case 8 :
   		   FAI_nb_drive();
   		   break;
   	   case 9 :
   		   FAI_Dir (0);
   		   break;
   	   case 10 :
		   if (TRUE == nav_drive_set (Param_u32))
		   {
			   CI_LocalPrintf ("nav_drive_set OK\r\n");
		   }
		   else
		   {
			   CI_LocalPrintf ("nav_drive_set FAIL - %d - %s\r\n",fs_g_status, IBN_FileSystemErrorText(fs_g_status));
		   }
   		   break;
   	   case 11 :
#if LUN_2 == ENABLE
         CI_LocalPrintf ("Set LUN %d CTRL_BUSY\n\r",Param_u32);
         //virtual_unit_state_e = CTRL_BUSY;
         set_virtual_unit_busy ();
#endif
         break;
   	   case 12 :
         CI_LocalPrintf ("Update LUN %d\n\r",Param_u32);
   	    FAI_UpdateContend (Param_u32);;
   	    break;
   	   case 13 :
         CI_LocalPrintf ("fat_cache_flush\n\r");
   	     fat_cache_flush ();
         break;
   	   case 14 :
         CI_LocalPrintf ("fat_check_device\n\r");
   	     fat_check_device ();
   	     break;
       case 15 :
         CI_LocalPrintf ("fat_check_device\n\r");
        fat_read_dir ();
        break;
       case 16 :
         CI_LocalPrintf ("Copy test.txt to test1.txt\n\r");
         {
           u8 F1[10];
           u8 F2[10];

           strcpy ((char*)F1,"test.txt");
           strcpy ((char*)F2,"test1.txt");
           FAI_CopyFile (F1,F2);
         }
        break;
       case 17 :
         CI_LocalPrintf ("Format LUN %d\n\r",Param_u32);
         nav_drive_set(Param_u32);
         if( !nav_drive_format(FS_FORMAT_FAT) )
         {
           CI_LocalPrintf ("Format LUN %d - ERROR\n\r",Param_u32);
         }
         break;

	}
}
#endif

/*******************************************************************************

  IBN_LogADInput

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_LogADInput (u8 nParamsGet_u8,u8 CMD_u8,u32 Param_u32)
{
#ifdef AD_LOGGING_ACTIV
	if (0 == nParamsGet_u8)
	{
		CI_LocalPrintf ("AD Test\r\n");
		CI_LocalPrintf ("\r\n");
		CI_LocalPrintf ("0   Print AD data\r\n");
    CI_LocalPrintf ("1   Start SD log\r\n");
    CI_LocalPrintf ("2   Stop SD log\r\n");
    CI_LocalPrintf ("3   Init SD log\r\n");
    CI_LocalPrintf ("4   Print summary\r\n");
		CI_LocalPrintf ("\r\n");
		return;
	}
	switch (CMD_u8)
	{
		case 0 :
			AD_Readdata ();
			break;
		case 1 :
		  AD_LOG_Start ();
			break;
		case 2 :
		  AD_LOG_Stop ();
			break;
		case 3 :
			AD_LOG_init();
			break;
		case 4:
		  AD_PrintSummary ();
		  break;
    case 5 :
      AD_FileIO_ReadLocalTime_u32 ();
      break;
	}
#else
  CI_LocalPrintf ("AD Log not active\r\n");
#endif // #ifdef AD_LOGGING_ACTIV
}

/*******************************************************************************

  IBN_TimeAccess

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
#ifdef ENABLE_IBN_TIME_ACCESS_TESTS

void IBN_TimeAccess (u8 nParamsGet_u8,u8 CMD_u8,u32 Param_u32,u8 *String_pu8)
{
	struct tm      tm_st;
//	struct tm      tm_st1;

	u8             Time_u8[30];
	time_t now;

	if (0 == nParamsGet_u8)
	{
		CI_LocalPrintf ("Time functions\r\n");
		CI_LocalPrintf ("\r\n");
		CI_LocalPrintf ("0   Show time\r\n");
		CI_LocalPrintf ("1   Set time - HH:MM:SS\r\n");
    CI_LocalPrintf ("2   Set date - DD:MM:YY\r\n");
    CI_LocalPrintf ("3   Set time in sec\r\n");
    CI_LocalPrintf ("4   Write time to flash\r\n");
    CI_LocalPrintf ("5   Read time from flash\r\n");
		CI_LocalPrintf ("\r\n");
		return;
	}
	switch (CMD_u8)
	{
		case 0 :
			time (&now);
			CI_LocalPrintf ("Time %ld\r\n",now);
			ctime_r (&now,(char*)Time_u8);
			CI_LocalPrintf ("Time %s\r\n",Time_u8);
			break;
		case 1 :
			if (NULL == String_pu8)
			{
				break;
			}
			time (&now);
			localtime_r (&now,&tm_st);
			tm_st.tm_hour = atoi ((char*)&String_pu8[0]);
			tm_st.tm_min  = atoi ((char*)&String_pu8[3]);
			tm_st.tm_sec  = atoi ((char*)&String_pu8[6]);
			CI_LocalPrintf ("Set time to %2d:%02d:%02d\r\n",tm_st.tm_hour,tm_st.tm_min,tm_st.tm_sec);
			now = mktime (&tm_st);
			set_time (now);
			break;

		case 2 :
			if (NULL == String_pu8)
			{
				break;
			}
			time (&now);
			localtime_r (&now,&tm_st);
			tm_st.tm_mday = atoi ((char*)&String_pu8[0]);
			tm_st.tm_mon  = atoi ((char*)&String_pu8[3]) - 1;
			tm_st.tm_year = atoi ((char*)&String_pu8[6]);
			if (50 > tm_st.tm_year)
			{
				tm_st.tm_year += 100;
			}
			CI_LocalPrintf ("Set date to %2d.%02d.%04d\r\n",tm_st.tm_mday,tm_st.tm_mon+1,1900+tm_st.tm_year);
			now = mktime (&tm_st);
			set_time (now);
			break;
    case 3 :
      CI_LocalPrintf ("Set Time %ld\r\n",Param_u32);
      set_time (Param_u32);
      time (&now);
      CI_LocalPrintf ("Time %ld\r\n",now);
      ctime_r (&now,(char*)Time_u8);
      CI_LocalPrintf ("Time %s\r\n",Time_u8);
      break;
    case 4 :
      time (&now);
      CI_LocalPrintf ("Write time %ld -",now);
      ctime_r (&now,(char*)Time_u8);
      CI_LocalPrintf ("%s\r\n",Time_u8);
      WriteDatetime (now);
      break;
    case 5 :
      ReadDatetime (&now);
      CI_LocalPrintf ("Stored time %ld - ",now);
      ctime_r (&now,(char*)Time_u8);
      CI_LocalPrintf ("Time %s\r\n",Time_u8);
      break;

	}
}
#endif
/*******************************************************************************

  IBN_USB_Stats

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

USB_Log_tst USB_Log_st;

void USB_CCID_send_INT_Message (void);

void IBN_USB_Stats (u8 nParamsGet_u8,u8 CMD_u8,u32 Param_u32,u8 *String_pu8)
{
  u8      Time_u8[30];
  time_t  now;

  if (0 == nParamsGet_u8)
  {
    CI_LocalPrintf ("USB functions\r\n");
    CI_LocalPrintf ("\r\n");
    CI_LocalPrintf ("0   Show USB stat\r\n");
    CI_LocalPrintf ("1   Set CCID card inserted\r\n");
    CI_LocalPrintf ("\r\n");
    return;
  }
  switch (CMD_u8)
  {
    case 0 :
       CI_LocalPrintf ("USB status\r\n");

       CI_LocalPrintf ("\r\nMSD\r\n");
       CI_LocalPrintf ("Read calls  : %9d\r\n",USB_Log_st.MSD_ReadCalls_u32);
       CI_LocalPrintf ("Read        : %9d blocks\r\n",USB_Log_st.MSD_BytesRead_u32);
       CI_LocalPrintf ("Write calls : %9d\r\n",USB_Log_st.MSD_WriteCalls_u32);
       CI_LocalPrintf ("Write       : %9d blocks\r\n",USB_Log_st.MSD_BytesWrite_u32);

       CI_LocalPrintf ("\r\nLast MSD access time\r\n");
       time (&now);
       ctime_r (&now,(char*)Time_u8);
       CI_LocalPrintf ("Actual time       : %s",Time_u8);
       ctime_r (&USB_Log_st.MSD_LastReadAccess_u32,(char*)Time_u8);
       CI_LocalPrintf ("Last read access  : %s",Time_u8);
       ctime_r (&USB_Log_st.MSD_LastWriteAccess_u32,(char*)Time_u8);
       CI_LocalPrintf ("Last write access : %s",Time_u8);

       CI_LocalPrintf ("\r\nCCID\r\n");
       CI_LocalPrintf ("USB > CCID calls : %9d\r\n",USB_Log_st.CCID_WriteCalls_u32);
       CI_LocalPrintf ("USB > CCID       : %9d bytes\r\n",USB_Log_st.CCID_BytesWrite_u32);
       CI_LocalPrintf ("CCID > USB calls : %9d\r\n",USB_Log_st.CCID_ReadCalls_u32);
       CI_LocalPrintf ("CCID > USB       : %9d bytes\r\n",USB_Log_st.CCID_BytesRead_u32);
       break;

    case 1:
      USB_CCID_send_INT_Message ();
      break;
  }
}


/*******************************************************************************

  IBN_SdRetValueString

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void sd_mmc_mci_resources_init(void);
Ctrl_status sd_mmc_mci_test_unit_ready(U8 slot);

char *IBN_SdRetValueString (Ctrl_status Ret_e)
{
  switch (Ret_e)
  {
    case CTRL_GOOD       : return ("GOOD      ");
    case CTRL_FAIL       : return ("FAIL      ");
    case CTRL_NO_PRESENT : return ("NO_PRESENT");
    case CTRL_BUSY       : return ("BUSY      ");
    default              : return ("** UNKNOWN **  ");
  }
}

/*******************************************************************************

  IBN_HTML_Tests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_HTML_Tests (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8)
{
#ifdef HTML_ENABLE_HTML_INTERFACE
  if (0 == nParamsGet_u8)
  {
    CI_LocalPrintf ("HTML test functions\r\n");
    CI_LocalPrintf ("\r\n");
    CI_LocalPrintf ("0   Write start file\r\n");
    CI_LocalPrintf ("\r\n");
    return;
  }
  switch (CMD_u8)
  {
    case 0 :
      HTML_GenerateStartPage ();
      break;
  }
#else
  CI_LocalPrintf ("HTML interface not active\r\n");
#endif
}

/*******************************************************************************

  vApplicationTickHook

  ISR ?

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void vApplicationTickHook( void )
{

#ifdef AD_LOGGING_ACTIV
  AD_ApplicationTickHook();
#endif
}


