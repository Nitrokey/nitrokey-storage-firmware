/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 14.04.2011
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
 * FileAccessInterface.h
 *
 *  Created on: 14.04.2011
 *      Author: RB
 */

#ifndef FILE_ACCESS_INTERFACE_H_
#define FILE_ACCESS_INTERFACE_H_

//! Maximal number of characters in file path.
#define FAI_MAX_FILE_PATH_LENGTH  30


u8 FAI_mount(u8 Lun_u8);
u8 FAI_Write (u8 Lun_u8);
u8 FAI_ShowDirContent (void);
void FAI_free_space(u8 Param_u8);
void FAI_nb_drive(void);
void FAI_Dir (u8 more_u8);
u8  FAI_cd(u8 *Path_pu8);
void FAI_touch (u8 *Path_pu8);
u8 FAI_mkdir(u8 *Name_pu8);
void FAI_cat(u8 *Name_pu8,u8 More_u8);
u8 FAI_MarkLunAsChanged (u8 Lun_u8);
void FAI_UpdateContend (u8 Lun_u8);
void FAI_DeleteFile(u8 *Filename_pu8);

u32 FAI_CopyFile (u8 *SourceFilename_pu8, u8 *DestFilename_pu8);

#endif /* FILE_ACCESS_INTERFACE_H_ */
