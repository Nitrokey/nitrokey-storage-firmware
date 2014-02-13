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
 * HiddenVolume.c
 *
 *  Created on: 14.07.2013
 *      Author: RB
 */

#include <avr32/io.h>
#include <stddef.h>
#include "compiler.h"
#include "flashc.h"
#include "string.h"
#include "aes.h"

#include "global.h"
#include "tools.h"
#include "HiddenVolume.h"

#include "CCID/USART/ISO7816_USART.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"

#include "OTP/report_protocol.h"
#include "FlashStorage.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

#define LOCAL_DEBUG

#define HIDDEN_VOLUME_DEFAUT_START_BLOCK     2000000   // 4.000.000 * 512 Byte = ca. 2 GB
#define HIDDEN_VOLUME_DEFAUT_SIZE            1000000   // 2.000.000 * 512 Byte = ca. 1 GB

#define AES_KEYSIZE_256_BIT                       32   // 32 Byte * 8 = 256 Bit

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

  InitRamdomBaseForHiddenKey_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 InitRamdomBaseForHiddenKey_u8 (void)
{
  u8 BaseKey_au8[AES_KEYSIZE_256_BIT];

  LA_RestartSmartcard_u8 ();

#ifdef LOCAL_DEBUG
  CI_TickLocalPrintf ("InitRamdomBaseForHiddenKey_u8\r\n");
#endif

// Get a random number for the base key
  if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT/2,BaseKey_au8))
  {
#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
    return (FALSE);
  }

// Get a random number for the storage key
  if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT/2,&BaseKey_au8[AES_KEYSIZE_256_BIT/2]))
  {
#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
    return (FALSE);
  }

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Base key       : ");
  HexPrint (AES_KEYSIZE_256_BIT,BaseKey_au8);
  CI_LocalPrintf ("\r\n");
#endif

// Todo: Crypted base key with key from smartcard


// Save base key
  WriteBaseForHiddenVolumeKey (BaseKey_au8);

  return (TRUE);
}

/*******************************************************************************

  GetAESKeyFromUserpassword_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 GetAESKeyFromUserpassword_u8 (u8 *Userpassword_pu8, u8 *AESKey_pu8)
{
  u32 i,n,i1;
  u8  pointer_u8;

// Get base key
   ReadBaseForHiddenVolumeKey (AESKey_pu8);

// Todo: Encrypt base key with key from smartcard

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Base for hidden volume AES key  : ");
  HexPrint (AES_KEYSIZE_256_BIT,AESKey_pu8);
  CI_LocalPrintf ("\r\n");
#endif

   n = strlen ((char*)Userpassword_pu8);

// shake it a little bit
   pointer_u8 = 0;
   for (i=0;i<1000;i++)
   {
     srand (i + AESKey_pu8[pointer_u8]*119);
     for (i1=0;i1<n;i1++)
     {
       AESKey_pu8[pointer_u8] = (u8)((u16)AESKey_pu8[pointer_u8] + (u16)Userpassword_pu8[i1] + (u16)(rand () % 256));
       pointer_u8 += (u8)((u16)AESKey_pu8[pointer_u8] + (u16)Userpassword_pu8[i1] + (u16)(rand () % 256));
       pointer_u8 = pointer_u8 % AES_KEYSIZE_256_BIT;
     }
   }


// Finaly crypt it with ...


   return (TRUE);
}

/*******************************************************************************

  BuildAESKeyFromUserpassword_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 *BuildAESKeyFromUserpassword_u8 (u8 *Userpassword_pu8,u8 *AESKey_au8)
{

// Expand Userpassword to 256 bit size

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("User password               : ");
  HexPrint (strlen ((char*)Userpassword_pu8),Userpassword_pu8);
  CI_LocalPrintf ("\r\n");
#endif

// AESKey_au8 =  f(Userpassword_pu8)

  // Not a real secure function
  memset (AESKey_au8,0,AES_KEYSIZE_256_BIT);
  GetAESKeyFromUserpassword_u8 (Userpassword_pu8,AESKey_au8);

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Uncrypted hidden volume key  : ");
  HexPrint (AES_KEYSIZE_256_BIT,AESKey_au8);
  CI_LocalPrintf ("\r\n");
#endif

// Clear the critical memory
  memset (Userpassword_pu8,0,strlen ((char*)Userpassword_pu8));

  return (AESKey_au8);
}

/*******************************************************************************

  CryptHiddenVolumeKey_u32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
/* not used
u32 CryptHiddenVolumeKey_u32 (u8 *Userpassword_pu8)
{
//  u32 Ret_u32;
  u8  AESKey_au8[AES_KEYSIZE_256_BIT];
  u8  Buffer_au8[AES_KEYSIZE_256_BIT];

// Expand Userpassword to 256 bit size

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("User password               : ");
  HexPrint (strlen ((char*)Userpassword_pu8),Userpassword_pu8);
  CI_LocalPrintf ("\r\n");
#endif

// AESKey_au8 =  f(Userpassword_pu8)

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Uncrypted hidden volume key  : ");
  HexPrint (AES_KEYSIZE_256_BIT,AESKey_au8);
  CI_LocalPrintf ("\r\n");
#endif

// Local encryption of the storage key
  AES_StorageKeyEncryption (AES_KEYSIZE_256_BIT, Buffer_au8, AESKey_au8, AES_PMODE_CIPHER);

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Crypted hidden volume key   : ");
  HexPrint (AES_KEYSIZE_256_BIT,Buffer_au8);
  CI_LocalPrintf ("\r\n");
#endif

// Store the encrypted storage key in USER PAGE
  flashc_memcpy(AVR32_FLASHC_USER_PAGE,Buffer_au8,AES_KEYSIZE_256_BIT,TRUE);



// Clear the critical memory
  memset (AESKey_au8,0,AES_KEYSIZE_256_BIT);
  memset (Buffer_au8,0,AES_KEYSIZE_256_BIT);


  return (TRUE);

}

*/


