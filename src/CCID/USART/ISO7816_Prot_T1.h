/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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
 * ISO7816_Prot_T1.h
 *
 *  Created on: 09.09.2010
 *      Author: RB
 */

#ifndef ISO7816_PROT_T1_H_
#define ISO7816_PROT_T1_H_

#define ISO7816_T1_NAD_ADDR			0
#define ISO7816_T1_PCB_ADDR			1
#define ISO7816_T1_LEN_ADDR			2

#define ISO7816_T1_PCB_SEND_NR_BIT_I_BLOCK			0x40
#define ISO7816_T1_PCB_CHAINING_BIT_I_BLOCK			0x20
#define ISO7816_T1_PCB_SEND_NR_BIT_R_BLOCK			0x10



void ISO7816_T1_InitSendNr (void);
int ISO7816_T1_ReceiveBlock (unsigned int *nCount);
int ISO7816_T1_SendString (unsigned int nSendBytes, unsigned char *cSendData,
                           unsigned int *nReceiveBytes,
                           unsigned char *cReceiveData);
int ISO7816_T1_ReceiveString (unsigned int *nCount,
                              unsigned char *cReceiveBuffer);
int ISO7816_T1_SendAPDU (typeAPDU * tSC);
int ISO7816_T1_DirectXfr (signed long *nLength, unsigned char *szXfrBuffer,
                          signed long nInputLength);

#endif /* ISO7816_PROT_T1_H_ */
