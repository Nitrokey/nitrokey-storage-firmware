/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 15.06.2012
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
 * HandleAesStorageKey.c
 *
 *  Created on: 15.06.2012
 *      Author: RB
 */

#include "string.h"
#include "aes.h"
#include "flashc.h"

#include "global.h"
#include "tools.h"
#include "Interpreter.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/Local_ACCESS/OpenPGP_V20.h"
#include "FlashStorage.h"
#include "HandleAesStorageKey.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

#define LOCAL_DEBUG

#define AES_KEYSIZE_256_BIT     32        // 32 * 8 = 256

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/


/*******************************************************************************

  BuildNewAesStorageKey_u32

  Build a new storage key for the SD card encryption

  Steps:

  - get random number for the storage key (SD card key)
  - encrypt the storage key
  - store the storage key in the user page

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 BuildNewAesStorageKey_u32 (u8 *MasterKey_pu8)
{
//  u32 Ret_u32;
  u8  StorageKey_au8[AES_KEYSIZE_256_BIT];
  u8  Buffer_au8[AES_KEYSIZE_256_BIT];

#ifdef LOCAL_DEBUG
  CI_TickLocalPrintf ("BuildNewAesStorageKey\r\n");
#endif
// Wait for next smartcard cmd
  DelayMs (10);

// Get a random number for the storage key
  if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT/2,StorageKey_au8))
  {
#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
    return (FALSE);
  }

// Get a random number for the storage key
  if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT/2,&StorageKey_au8[AES_KEYSIZE_256_BIT/2]))
  {
#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
    return (FALSE);
  }

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Uncrypted storage key       : ");
  HexPrint (AES_KEYSIZE_256_BIT,StorageKey_au8);
  CI_LocalPrintf ("\r\n");
#endif

  memcpy (Buffer_au8,StorageKey_au8,AES_KEYSIZE_256_BIT);

// Local encryption of the storage key
  AES_StorageKeyEncryption (AES_KEYSIZE_256_BIT, Buffer_au8, MasterKey_pu8, AES_PMODE_CIPHER);

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Local encrypted storage key : ");
  HexPrint (AES_KEYSIZE_256_BIT,Buffer_au8);
  CI_LocalPrintf ("\r\n");
#endif

// Store the encrypted storage key in USER PAGE
  WriteAESStorageKeyToUserPage (Buffer_au8);
//  flashc_memcpy(AVR32_FLASHC_USER_PAGE,Buffer_au8,AES_KEYSIZE_256_BIT,TRUE);


#ifdef LOCAL_DEBUG
// Test the storage key

// Wait for next smartcard cmd
  DelayMs (10);

/* Enable smartcard */
  if (FALSE == LA_OpenPGP_V20_Test_SendUserPW2 ((u8*)"123456"))
  {
    return (FALSE);
  }

// Wait for next smartcard cmd
  DelayMs (10);

/* SC AES key decryption */
  if (FALSE == LA_OpenPGP_V20_Test_ScAESKey (AES_KEYSIZE_256_BIT,Buffer_au8))
  {
    return (FALSE);
  }

  CI_LocalPrintf ("SC decrypted storage key    : ");
  HexPrint (AES_KEYSIZE_256_BIT,Buffer_au8);
  CI_LocalPrintf ("\r\n");

#endif

// Clear the critical memory
  memset (StorageKey_au8,0,AES_KEYSIZE_256_BIT);
  memset (Buffer_au8,0,AES_KEYSIZE_256_BIT);


  return (TRUE);

}


/*******************************************************************************

  BuildNewAesMasterKey_u32

  Build a new storage key for the SD card encryption

  Steps:

  - get random number for the master key
  - store master key on smartcard

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 BuildNewAesMasterKey_u32 (u8 *AdminPW_pu8,u8 *MasterKey_pu8)
{
#ifdef LOCAL_DEBUG
  CI_TickLocalPrintf ("BuildNewAesMasterKey_u32\r\n");
#endif

// Wait for next smartcard cmd
  DelayMs (10);

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber\n\r");
#endif

// Get a random number for the master key
  if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT/2,MasterKey_pu8))
  {
#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
    return (FALSE);
  }

// Get a random number for the master key
  if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT/2,&MasterKey_pu8[AES_KEYSIZE_256_BIT/2]))
  {
#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
    return (FALSE);
  }

// Wait for next smartcard cmd
  DelayMs (10);

#ifdef LOCAL_DEBUG
  CI_TickLocalPrintf ("Send AdminPW -%s-\r\n",AdminPW_pu8);
#endif
// Unlock smartcard for sending master key
  if (FALSE == LA_OpenPGP_V20_Test_SendAdminPW (AdminPW_pu8))
  {
#ifdef LOCAL_DEBUG
  CI_TickLocalPrintf ("AdminPW wrong\r\n");
#endif
    return (FALSE);
  }


#ifdef LOCAL_DEBUG
  CI_TickLocalPrintf ("AES Masterkey : ");
  HexPrint (AES_KEYSIZE_256_BIT,MasterKey_pu8);
  CI_TickLocalPrintf ("\r\n");
#endif

// Wait for next smartcard cmd
  DelayMs (10);

// Store master key in smartcard
  if (FALSE == LA_OpenPGP_V20_Test_SendAESMasterKey (AES_KEYSIZE_256_BIT,MasterKey_pu8))
  {
#ifdef LOCAL_DEBUG
  CI_TickLocalPrintf ("SendAESMasterKey fails\r\n");
#endif
    return (FALSE);
  }

  return (TRUE);
}

/*******************************************************************************

  BuildStorageKeys_u32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 BuildStorageKeys_u32 (u8 *AdminPW_pu8)
{
  u32 Ret_u32;
  u8  MasterKey_au8[AES_KEYSIZE_256_BIT];

  Ret_u32 = BuildNewAesMasterKey_u32 (AdminPW_pu8,MasterKey_au8);
  if (FALSE == Ret_u32)
  {
    return (FALSE);
  }

  Ret_u32 = BuildNewAesStorageKey_u32 (MasterKey_au8);
  if (FALSE == Ret_u32)
  {
    return (FALSE);
  }

// Clear the critical memory
  memset (MasterKey_au8,0,AES_KEYSIZE_256_BIT);

  return (TRUE);
}

/*******************************************************************************

  GetStorageKey_u32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 GetStorageKey_u32 (u8 *UserPW_pu8, u8 *StorageKey_pu8)
{

//  memcpy (StorageKey_pu8,(void*)(AVR32_FLASHC_USER_PAGE),AES_KEYSIZE_256_BIT);
  ReadAESStorageKeyToUserPage (StorageKey_pu8);

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("GetStorageKey_u32: Encrypted storage key : ");
  HexPrint (AES_KEYSIZE_256_BIT,StorageKey_pu8);
  CI_LocalPrintf ("\r\n");
#endif


/* Enable smartcard */
  if (FALSE == LA_OpenPGP_V20_Test_SendUserPW2 (UserPW_pu8))
  {
    return (FALSE);
  }

// Wait for next smartcard cmd
  DelayMs (10);

/* SC AES key decryption */
  if (FALSE == LA_OpenPGP_V20_Test_ScAESKey (AES_KEYSIZE_256_BIT,StorageKey_pu8))
  {
    return (FALSE);
  }

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("GetStorageKey_u32: Decrypted storage key : ");
  HexPrint (AES_KEYSIZE_256_BIT,StorageKey_pu8);
  CI_LocalPrintf ("\r\n");
#endif

  return (TRUE);
}


/*******************************************************************************

  HighLevelTests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void HighLevelTests (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8)
{

  if (0 == nParamsGet_u8)
  {
    CI_LocalPrintf ("Highlevel test functions\r\n");
    CI_LocalPrintf ("\r\n");
    CI_LocalPrintf ("0          Build storage keys\r\n");
    CI_LocalPrintf ("1          Print 32 byte of USER PAGE\r\n");
    CI_LocalPrintf ("2          Write 12345678 to USER PAGE\r\n");
    CI_LocalPrintf ("2          Write 987654321 to USER PAGE\r\n");
    CI_LocalPrintf ("\r\n");
    return;
  }

  switch (CMD_u8)
  {
    case 0 :
          BuildStorageKeys_u32 ((u8*)"12345678");
          break;

    case 1 :
          CI_LocalPrintf ("Print USER PAGE : ");
          HexPrint (32,(u8*)AVR32_FLASHC_USER_PAGE);
          CI_LocalPrintf ("\r\n");
          break;

    case 2 :
          CI_LocalPrintf ("Write 12345678 to USER PAGE\r\n");
          flashc_memcpy(AVR32_FLASHC_USER_PAGE,"12345678",8,TRUE);
          break;
    case 3 :
          CI_LocalPrintf ("Write 987654321 to USER PAGE\r\n");
          flashc_memcpy(AVR32_FLASHC_USER_PAGE,"987654321",9,TRUE);
          break;
    default :
          break;
  }
}


