/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 09.04.2011
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
 * Inbetriebnahme.h
 *
 *  Created on: 09.04.2011
 *      Author: RB
 */

#ifndef INBETRIEBNAHME_H_
#define INBETRIEBNAHME_H_


/*
 * File private variables. --------------------------------
 */
extern volatile unsigned portBASE_TYPE uxCurrentNumberOfTasks;
extern volatile portTickType xTickCount;
extern unsigned portBASE_TYPE uxTopUsedPriority;
extern volatile unsigned portBASE_TYPE uxTopReadyPriority;
extern volatile signed portBASE_TYPE xSchedulerRunning;
extern volatile unsigned portBASE_TYPE uxSchedulerSuspended;
extern volatile unsigned portBASE_TYPE uxMissedTicks;
extern volatile portBASE_TYPE xMissedYield;
extern volatile portBASE_TYPE xNumOfOverflows;
extern unsigned portBASE_TYPE uxTaskNumber;

extern tskTCB *volatile pxCurrentTCB;

/*
 * Lists for ready and blocked tasks. --------------------
 */

extern xList pxReadyTasksLists[configMAX_PRIORITIES];   /* < Prioritised
                                                         * ready tasks. */
extern xList xDelayedTaskList1; /* < Delayed tasks. */
extern xList xDelayedTaskList2; /* < Delayed tasks (two lists are used - one
                                 * for delays that have overflowed the
                                 * current tick count. */
extern xList *volatile pxDelayedTaskList;   /* < Points to the delayed task
                                             * list currently being used. */
extern xList *volatile pxOverflowDelayedTaskList;   /* < Points to the
                                                     * delayed task list
                                                     * currently being used
                                                     * to hold tasks that
                                                     * have overflowed the
                                                     * current tick count. */
extern xList xPendingReadyList; /* < Tasks that have been readied while the
                                 * scheduler was suspended.  They will be
                                 * moved to the ready queue when the
                                 * scheduler is resumed. */
#if ( INCLUDE_vTaskDelete == 1 )
volatile xList xTasksWaitingTermination;    /* < Tasks that have been deleted 
                                             * - but the their memory not yet 
                                             * freed. */
#endif
#if ( INCLUDE_vTaskSuspend == 1 )
xList xSuspendedTaskList;   /* < Tasks that are currently suspended. */
#endif

unsigned short usTaskCheckFreeStackSpace (const unsigned char *pucStackByte);

/**********************************************************************************************/

void IBN_GetTaskList (void);
void IBN_ShowTaskStatus (u8 CMD_u8);
void IBN_FileAccess (u8 nParamsGet_u8, u8 CMD_u8, u32 Param_u32);
char *IBN_FileSystemErrorText (u8 FS_Error_u8);

void IBN_LogADInput (u8 nParamsGet_u8, u8 CMD_u8, u32 Param_u32);
void IBN_TimeAccess (u8 nParamsGet_u8, u8 CMD_u8, u32 Param_u32,
                     u8 * String_pu8);
void IBN_USB_Stats (u8 nParamsGet_u8, u8 CMD_u8, u32 Param_u32,
                    u8 * String_pu8);
void IBN_HTML_Tests (unsigned char nParamsGet_u8, unsigned char CMD_u8,
                     unsigned int Param_u32, unsigned char *String_pu8);
char *IBN_SdRetValueString (Ctrl_status Ret_e);

#endif /* INBETRIEBNAHME_H_ */
