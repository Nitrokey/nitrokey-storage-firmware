/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 26.11.2012
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
 * file_io.h
 *
 *  Created on: 26.11.2012
 *      Author: RB
 */

#ifndef FILE_IO_H_
#define FILE_IO_H_

#include <stddef.h>
#include "tools.h"

u8 FileIO_SaveAppImage_u8 (void);
u8 WriteStrToDebugFile (u8 *String_pu8);
int printf_file (char* szFormat, ...);
void dump_arr(const char* name, const u8* p, const size_t size);

void IBN_FileIo_Tests (u8 nParamsGet_u8, u8 CMD_u8, u32 Param_u32, u32 Param_1_u32, u32 Param_2_u32);


#endif /* FILE_IO_H_ */
