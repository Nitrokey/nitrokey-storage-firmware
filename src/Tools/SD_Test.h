/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 22.11.2012
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
 * SD_Test.h
 *
 *  Created on: 22.11.2012
 *      Author: RB
 */

#ifndef SD_TEST_H_
#define SD_TEST_H_

u8 SD_SecureEraseHoleCard (void);
u8 SD_SecureEraseCryptedVolume (void);
u8 SD_GetRandomBlock (u8 * RandomData_u8);
u8 SD_WriteMultipleBlocks (u8 * Addr_u8, u32 Block_u32, u32 Count_u32);
u16 SD_SpeedTest (void);

void IBN_SD_Tests (u8 nParamsGet_u8, u8 CMD_u8, u32 Param_u32, u32 Param_1_u32, u32 Param_2_u32);

#endif /* SD_TEST_H_ */
