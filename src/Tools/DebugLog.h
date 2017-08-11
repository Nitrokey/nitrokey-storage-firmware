/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 09.08.2017
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
 * DebugLog.h
 *
 *  Created on: 09.08.2017
 *      Author: RB
 */

#ifndef DEBUGLOG_H_
#define DEBUGLOG_H_



#define DL_LOG__ERROR__NO_ENTRY          0
#define DL_LOG__ERROR__INVALID_ENTRY     1
#define DL_LOG__REQ_GET_DESCRIPTOR       2
#define DL_LOG__REQ_GET_CONFIGURATION    3
#define DL_LOG__REQ_SET_ADDRESS          4
#define DL_LOG__REQ_SET_CONFIGURATION    5
#define DL_LOG__REQ_CLEAR_FEATURE        6
#define DL_LOG__REQ_SET_FEATURE          7
#define DL_LOG__REQ_GET_STATUS           8
#define DL_LOG__REQ_GET_INTERFACE        9
#define DL_LOG__REQ_SET_INTERFACE       10
#define DL_LOG__REQ_SET_DESCRIPTOR      11
#define DL_LOG__REQ_SYNCH_FRAME         12
#define DL_LOG__REQ_UNSUPPORTED         13
#define DL_LOG__REQ_UNKNOWN             14
#define DL_LOG__MAX_ENTRY               15


#ifndef DEBUG_LOG_ENABLE
  #undef DL_Init
  #undef DL_LogEvent
  #undef DL_LogEventError
  #undef DL_SaveLog
  #undef DL_AddSequenceLog
  #undef DL_ShowSequenceLog

  #define DL_Init(...)
  #define DL_LogEvent(...)
  #define DL_LogEventError(...)
  #define DL_SaveLog(...)
  #define DL_AddSequenceLog(...)
  #define DL_ShowSequenceLog(...)
#else
  void DL_Init (void);

  void DL_LogEvent (unsigned char Event);
  void DL_LogEventError (unsigned char Event);
  void DL_SaveLog (void);
  void DL_AddSequenceLog (unsigned int Tick,unsigned char Event);
  void DL_ShowSequenceLog (void);

  void IBN_DL_Test (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8);
#endif





#endif /* DEBUGLOG_H_ */
