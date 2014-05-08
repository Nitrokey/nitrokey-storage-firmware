/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 22.11.2012
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

/*
 * SD_Test.h
 *
 *  Created on: 22.11.2012
 *      Author: RB
 */

#ifndef SD_TEST_H_
#define SD_TEST_H_

u8 SD_SecureEraseHoleCard (void);
u8 SD_SecureEraseCryptedVolume (void);

void IBN_SD_Tests (u8 nParamsGet_u8,u8 CMD_u8,u32 Param_u32,u32 Param_1_u32,u32 Param_2_u32);

#endif /* SD_TEST_H_ */