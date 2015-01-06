/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 19.08.2011
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


#ifndef DEBUG_T1_H_
#define DEBUG_T1_H_


void LogStart_T1_Block (int nLenght,unsigned char *acStartBlockData);
void LogEnd_T1_Block (int nLenght,unsigned char *acEndBlockData);

#endif /* DEBUG_T1_H_ */
