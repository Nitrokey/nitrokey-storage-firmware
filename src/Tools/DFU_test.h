/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 13.08.2012
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
 * DFU_test.h
 *
 *  Created on: 13.08.2012
 *      Author: RB
 */

#ifndef DFU_TEST_H_
#define DFU_TEST_H_

void DFU_DisableFirmwareUpdate (void);
void DFU_EnableFirmwareUpdate (void);
void IBN_DFU_Tests (unsigned char nParamsGet_u8, unsigned char CMD_u8,
                    unsigned int Param_u32, unsigned char *String_pu8);
void DFU_ResetCPU (void);
void DFU_FirmwareResetUserpage (void);

#endif /* DFU_TEST_H_ */
