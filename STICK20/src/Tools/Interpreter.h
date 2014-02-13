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


#ifndef INTERPRETER_HEADER
#define INTERPRETER_HEADER
/********************************************************************

   Datei    Interpreter.H

   Author   Rudolf Böddeker

********************************************************************/

#define CI_MAX_COMMANDS 40

#define IDF_TOOL_VERSION "1.0"

#define MAX_TEXT_LENGTH 150

#define L_OK         1
#define L_FALSE      0
#define L_ERROR     -1

#define CI_OK         1
#define CI_TRUE       CI_OK
#define CI_FALSE      0
#define CI_ERROR     -1

#define CI_CMD_NOTHING                  0
#define CI_CMD_HELP                     1
#define CI_CMD_TASKSTATUS               2
#define CI_CMD_RUNTIME_OVERVIEW         3
#define CI_CMD_RUNTIME_SLOT             4
#define CI_CMD_RUNTIME_INT_SLOT         5 
#define CI_CMD_FILE_ACCESS              6
#define CI_CMD_MOUNT                    7
#define CI_CMD_SHOW_LUNS                8
#define CI_CMD_DISK_FREE                9
#define CI_CMD_DIR                     10
#define CI_CMD_SELECT_LUN              11
#define CI_CMD_CD                      12
#define CI_CMD_DEL                     13
#define CI_CMD_TOUCH                   14
#define CI_CMD_UPDATE_LUN              15
#define CI_CMD_MAKE_DIR                16
#define CI_CMD_CAT                     17
#define CI_CMD_XTS_TEST                18
#define CI_CMD_AD_TEST                 19
#define CI_CMD_TIME_TEST               20
#define CI_CMD_USB                     21
#define CI_CMD_SET_SLOTSIZE            22
#define CI_CMD_SD                      23
#define CI_CMD_SC                      24
#define CI_CMD_HTML                    25
#define CI_CMD_HIGHLEVEL_TESTS         26
#define CI_CMD_LED                     27
#define CI_CMD_DFU                     28
#define CI_CMD_INTTIME                 29
#define CI_CMD_FILE                    30
#define CI_CMD_OTP                     31
#define CI_CMD_PWM                     32





#define CI_PROMPT                "->"


#define CI_CMD_STATE_START          0
#define CI_CMD_STATE_PRINT_PROMT    1
#define CI_CMD_STATE_WAIT_FOR_CMD   2

int CI_LocalPrintf (char *szFormat,...);
int CI_TickLocalPrintf (char *szFormat,...);
void CI_Print8BitValue (unsigned char cValue);
int CI_StringOut (char *szText);
s32 IDF_Debugtool (void);
void IDF_task_init(void);
void IDF_PrintStartupInfo (void);

#endif /* INTERPRETER_HEADER */
