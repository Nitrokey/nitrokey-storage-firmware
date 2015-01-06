/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 19.05.2011
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
 * AD_Test.h
 *
 *  Created on: 19.05.2011
 *      Author: RB
 */

#ifndef AD_TEST_H_
#define AD_TEST_H_

#include "tools.h"

void AD_init (void);
void AD_Readdata (void);
void AD_PrintSummary (void);
void AD_disable (void);
u8 AD_FileIO_Init_u8 (void);
u8 AD_FileIO_AppendData_u8 (u8 *Filename_pu8,u32 Data_u32);
u8 AD_WriteTestData_u8 (u32 LoopCount_u32);
u32 AD_FileIO_ReadLocalTime_u32 (void);
void AD_ApplicationTickHook( void );

void AD_LOG_Start (void);
void AD_LOG_Stop (void);
void AD_LOG_init(void);

#endif /* AD_TEST_H_ */
