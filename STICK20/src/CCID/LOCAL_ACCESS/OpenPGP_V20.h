/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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
 * OpenPGP_V20.h
 *
 *  Created on: 12.09.2010
 *      Author: Rudolf Böddeker
 */

#ifndef CCID_OPENPGP_V20_H_
#define CCID_OPENPGP_V20_H_

extern typeAPDU   tSC_OpenPGP_V20;
extern unsigned int SmartCardSerialNumber;

void IBN_SC_Tests (u8 nParamsGet_u8,u8 CMD_u8,u32 Param_u32,u8 *String_pu8);


int LA_OpenPGP_V20_Put_AES_key (typeAPDU *tSC,unsigned char cKeyLen,unsigned char *pcAES_Key);
int LA_OpenPGP_V20_AES_Enc (typeAPDU *tSC, int nSendLength,unsigned char *cSendData,int nReceiveLength, unsigned char *cReceiveData);
int LA_OpenPGP_V20_AES_Dec_SUB (typeAPDU *tSC, int nSendLength,unsigned char *cSendData,int nReceiveLength, unsigned char *cReceiveData);

int LA_OpenPGP_V20_Verify (typeAPDU *tSC, unsigned char cPW,unsigned char cPasswordLen,unsigned char *pcPassword);
int LA_OpenPGP_V20_Put_AES_key (typeAPDU *tSC,unsigned char cKeyLen,unsigned char *pcAES_Key);
int LA_OpenPGP_V20_Test (void);

u32 LA_SC_SendVerify (u8 PasswordFlag_u8,u8 *String_pu8);
u32 LA_SC_StartSmartcard (void);

int LA_OpenPGP_V20_GetPasswordstatus (char *PasswordStatus);
int LA_OpenPGP_V20_GetAID (char *AID);

int LA_OpenPGP_V20_Test_GetAID (void);
int LA_OpenPGP_V20_Test_SendAdminPW (unsigned char *pcPW);
int LA_OpenPGP_V20_Test_SendUserPW2 (unsigned char *pcPW);
int LA_OpenPGP_V20_Test_SendAESMasterKey (int nLen,unsigned char *pcMasterKey);
int LA_OpenPGP_V20_Test_ScAESKey (int nLen,unsigned char *pcKey);

int LA_OpenPGP_V20_Test_ChangeUserPin (unsigned char *pcOldPin,unsigned char *pcNewPin);
int LA_OpenPGP_V20_Test_ChangeAdminPin (unsigned char *pcOldPin,unsigned char *pcNewPin);

u32 GetRandomNumber_u32 (u32 Size_u32,u8 *Data_pu8);

u8 LA_RestartSmartcard_u8 (void);

#endif /* CCID_OPENPGP_V20_H_ */
