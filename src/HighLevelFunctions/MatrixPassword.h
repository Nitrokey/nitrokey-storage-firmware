/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-07-05
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
 * MatrixPassword.h
 *
 *  Created on: 05.07.2013
 *      Author: RB
 */

#ifndef MATRIXPASSWORD_H_
#define MATRIXPASSWORD_H_


void IBN_PWM_Tests (unsigned char nParamsGet_u8, unsigned char CMD_u8,
                    unsigned int Param_u32, unsigned char *String_pu8);

u8 *GetCorrectMatrix (void);
u8 ConvertMatrixDataToPassword (u8 * MatrixData_au8);

#endif /* MATRIXPASSWORD_H_ */
