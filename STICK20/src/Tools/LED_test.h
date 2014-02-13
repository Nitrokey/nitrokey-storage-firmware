/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 13.07.2012
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
 * LED_test.h
 *
 *  Created on: 13.07.2012
 *      Author: RB
 */

#ifndef LED_TEST_H
#define LED_TEST_H

void LED_GreenToggle (void);
void LED_GreenOff (void);
void LED_GreenOn (void);
void LED_RedToggle (void);
void LED_RedOff (void);
void LED_RedOn (void);

void LED_RedGreenToggle (void);
void LED_RedGreenOff (void);
void LED_RedGreenOn (void);

void IBN_LED_Tests (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8);


#endif /* LED_TEST_H */
