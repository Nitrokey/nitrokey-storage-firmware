/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 24.06.2010
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
 * ISO7816_ADPU.h
 *
 *  Created on: 24.06.2010
 *      Author: RB
 */

#ifndef ISO7816_ADPU_H_
#define ISO7816_ADPU_H_

// Flags for status byte
#define ISO7861_RESPONSE_STATE_ERROR 		0
#define ISO7861_RESPONSE_STATE_NO_STATUS 	1
#define ISO7861_RESPONSE_STATE_ACK 			2
#define ISO7861_RESPONSE_STATE_STATUS 		3

// Definitions for APDU
#define ISO7816_APDU_MAX_RESPONSE_LEN   500
#define ISO7816_APDU_SEND_HEADER_LEN      5
#define ISO7816_MAX_APDU_DATA           500
#define ISO7816_APDU_OFERHEAD             4 // for Le+Lc and checksum


typedef struct
{
    unsigned char cCLA;
    unsigned char cINS;
    unsigned char cP1;
    unsigned char cP2;
    int nLc;
    unsigned char cData[ISO7816_MAX_APDU_DATA];
    int nLe;
} typeAPDU_Data;

typedef struct
{
    unsigned char cStatus;
    unsigned char cSW1;
    unsigned char cSW2;
} typeAPDU_Status;

typedef struct
{
    unsigned int nSendDataLen;
    unsigned char cSendData[ISO7816_MAX_APDU_DATA + ISO7816_APDU_SEND_HEADER_LEN + ISO7816_APDU_OFERHEAD];
    unsigned int nReceiveDataLen;
    unsigned char cReceiveData[ISO7816_APDU_MAX_RESPONSE_LEN + 1];
    typeAPDU_Data tAPDU;
    typeAPDU_Status tState;
} typeAPDU;

u8 ISO7816_IsSmartcardUsable (void);
void ISO7816_SetLockCounter (u32 Value_u32);
void ISO7816_DecLockCounter (void);
u32 ISO7816_GetLockCounter (void);


int ISO7816_SendAPDU_Le_Lc (typeAPDU * tSC);
int ISO7816_SendAPDU_Le_NoLc (typeAPDU * tSC);
int ISO7816_SendAPDU_NoLe_Lc (typeAPDU * tSC);
int ISO7816_SendAPDU_NoLe_NoLc (typeAPDU * tSC);
void ISO7816_SetFiDI (u8 FiDi);

int ISO7816_CopyATR (unsigned char* szATR, int nMaxLength);
int ISO7816_InitSC (void);
int ISO7816_SendAPDU (typeAPDU * tSC);
void ISO7816_APDU_Test (void);
int ISO7816_SC_StartupTest (int nCount);

#endif /* ISO7816_ADPU_H_ */
