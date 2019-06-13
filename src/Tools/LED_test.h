/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 13.07.2012
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
 * LED_test.h
 *
 *  Created on: 13.07.2012
 *      Author: RB
 */

#ifndef LED_TEST_H
#define LED_TEST_H

void LED_Manager10ms_v (void);
void LED_ScCardOnline_v (void);
void LED_SdCardOnline_v (void);
void LED_GreenToggle (void);
void LED_GreenOff (void);
void LED_GreenOn (void);
void LED_RedToggle (void);
void LED_RedOff (void);
void LED_RedOn (void);

void LED_RedGreenToggle (void);
void LED_RedGreenOff (void);
void LED_RedGreenOn (void);

void LED_RedFlashNTimes (unsigned char times);
void LED_GreenFlashNTimes (unsigned char times);
void LED_ClearFlashing (void);

void LED_WinkOn (void);
void LED_WinkOff (void);

void IBN_LED_Tests (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8);


#endif /* LED_TEST_H */
