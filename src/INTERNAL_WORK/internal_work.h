/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 10.04.2012
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
 * internal_work.h
 *
 *  Created on: 10.04.2012
 *      Author: RB
 */

#ifndef INTERNAL_WORK_H_
#define INTERNAL_WORK_H_

void IW_task_init(void);

u32 IW_SendToSC_PW1 (u8 *PW_pu8);
u32 IW_SendToSC_PW3 (u8 *PW_pu8);

#endif /* INTERNAL_WORK_H_ */
