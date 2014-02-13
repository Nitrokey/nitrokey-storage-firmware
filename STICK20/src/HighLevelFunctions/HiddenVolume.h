/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-07-14
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
 * HiddenVolume.h
 *
 *  Created on: 14.07.2013
 *      Author: RB
 */

#ifndef HIDDENVOLUME_H_
#define HIDDENVOLUME_H_

u8 *BuildAESKeyFromUserpassword_u8 (u8 *Userpassword_pu8,u8 *AESKey_au8);
u8 InitRamdomBaseForHiddenKey_u8 (void);

#endif /* HIDDENVOLUME_H_ */
