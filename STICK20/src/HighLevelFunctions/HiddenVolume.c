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

#include "polarssl/sha4.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

#define LOCAL_DEBUG

//#define TEST_PBKDF2         // Check test vectors for PBKDF2, last test fails: password is over 500 byte long


#ifdef LOCAL_DEBUG
#else
  #define CI_LocalPrintf(...)
  #define CI_TickLocalPrintf(...)
  #define CI_StringOut(...)
  #define CI_Print8BitValue(...)
  #define HexPrint(...)
#endif

#define HIDDEN_VOLUME_DEFAUT_START_BLOCK     2000000   // 4.000.000 * 512 Byte = ca. 2 GB
#define HIDDEN_VOLUME_DEFAUT_SIZE            1000000   // 2.000.000 * 512 Byte = ca. 1 GB

#define AES_KEYSIZE_256_BIT                       32   // 32 Byte * 8 = 256 Bit

#define SHA512_HASH_LENGTH                        64   // 64 Byte


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

  HV_PrintSlotData_u8

  Changes
  Date      Reviewer        Info
  09.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 HV_PrintSlotData_u8 (u8 SlotNr_u8,HiddenVolumeKeySlot_tst *SlotData_st)
{
#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Hidden volume slot %d\r\n",SlotNr_u8);
  CI_LocalPrintf ("MagicNumber 0x%08x\r\n",SlotData_st->MagicNumber_u32);
  CI_LocalPrintf ("StartBlock  %8d\r\n",SlotData_st->StartBlock_u32);
  CI_LocalPrintf ("EndBlock    %8d\r\n",SlotData_st->EndBlock_u32);
  CI_LocalPrintf ("AesKey      ");
  HexPrint (32,SlotData_st->AesKey_au8);
  CI_LocalPrintf ("\r\n",SlotData_st->AesKey_au8[32]);
  CI_LocalPrintf ("Crc_u32     0x%08x\r\n",SlotData_st->Crc_u32);
#endif

  return (TRUE);
}

/*******************************************************************************

  HV_ReadSlot_u8

  Changes
  Date      Reviewer        Info
  07.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 HV_ReadSlot_u8 (u8 SlotNr_u8,HiddenVolumeKeySlot_tst *SlotData_st)
{
  u32 Crc32_u32;

// Read crypted slot data
  memcpy (SlotData_st,(u8*) (HV_SLOT_START_ADDRESS + SlotNr_u8 * HV_SLOT_SIZE),sizeof (HiddenVolumeKeySlot_tst));

// Decrypt slot data

// Check magic number
  if (HV_MAGIC_NUMBER_SLOT_ENTRY != SlotData_st->MagicNumber_u32)
  {
    return (FALSE);
  }

// Check CRC
  Crc32_u32 = generateCRC_len ((u8*)SlotData_st,(sizeof (HiddenVolumeKeySlot_tst) / 4) - 1);
  if (Crc32_u32 != SlotData_st->Crc_u32)
  {
    return (FALSE);
  }

  return (TRUE);
}

/*******************************************************************************

  HV_WriteSlot_u8

  Changes
  Date      Reviewer        Info
  07.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 HV_WriteSlot_u8 (u8 SlotNr_u8,HiddenVolumeKeySlot_tst *SlotData_st)
{
// Set magic number
  SlotData_st->MagicNumber_u32 = HV_MAGIC_NUMBER_SLOT_ENTRY;

// Set CRC32
  SlotData_st->Crc_u32 = generateCRC_len ((u8*)SlotData_st,(sizeof (HiddenVolumeKeySlot_tst) / 4) - 1);

// Encrypt slot data

// Write data to flash
  flashc_memcpy ((u8*)(HV_SLOT_START_ADDRESS + SlotNr_u8 * HV_SLOT_SIZE),SlotData_st,sizeof (HiddenVolumeKeySlot_tst),TRUE);

  return (TRUE);
}

/*******************************************************************************

  HV_InitSlot_u8

  Changes
  Date      Reviewer        Info
  08.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u8 HV_InitSlot_u8 (u8 SlotNr_u8)
{
  HiddenVolumeKeySlot_tst SlotData_st;

  SlotData_st.StartBlock_u32 = 0;
  SlotData_st.EndBlock_u32   = 0;
  memset (SlotData_st.AesKey_au8,0,32);

  HV_WriteSlot_u8 (SlotNr_u8,&SlotData_st);

  return (TRUE);
}

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
   ReadBaseForHiddenVolumeKey (AESKey_pu8);     // = Salt

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
























/*******************************************************************************

  Changes
  Date      Reviewer        Info
  12.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
void pbkdf2(u8 *out, const u8 *password, const u32 password_length, const u8 *salt, const u32 salt_length);

u8 GetHiddenVolumeAesKey_u8 (u8 *Userpassword_pu8,u32 PasswordLen_u32,u8 *AesBaseKey_pu8,u32 Keylen_u32)
{
  u8 output_au8[64];

#ifndef SAVE_FLASH_MEMORY_NO_PBKDF2
  pbkdf2 (output_au8,Userpassword_pu8,PasswordLen_u32,AesBaseKey_pu8,Keylen_u32);
#endif

  memcpy (AesBaseKey_pu8,output_au8,AES_KEYSIZE_256_BIT);   // copy 256 bit from the 512 bit output

#ifdef LOCAL_DEBUG
  CI_LocalPrintf ("Key : ");
  HexPrint (32,AesBaseKey_pu8);
  CI_LocalPrintf ("\r\n");
#endif


  return (TRUE);
}

/*******************************************************************************

  Changes
  Date      Reviewer        Info
  12.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
void pbkdf2(u8 *out, const u8 *password, const u32 password_length, const u8 *salt, const u32 salt_length);


void TestHiddenVolumeAesKey_u8 (void)
{
  u8 TestKey_pu8[50];
  u8 TestInput_pu8[50];

  strcpy (TestKey_pu8,"123456");
  strcpy (TestInput_pu8,"12345678901234567890123456789012)"); // 256 bit

  GetHiddenVolumeAesKey_u8 (TestKey_pu8,strlen (TestKey_pu8),TestInput_pu8,strlen (TestInput_pu8));
}

/*******************************************************************************

  IBN_HV_Tests

  Changes
  Date      Reviewer        Info
  07.04.14  RB              Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/
void pbkdf2_test (void);

#ifdef ENABLE_IBN_HV_TESTS

void IBN_HV_Tests (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8)
{
  HiddenVolumeKeySlot_tst SlotData_st;

  if (0 == nParamsGet_u8)
  {
    CI_LocalPrintf ("Hidden volume test functions\r\n");
    CI_LocalPrintf ("\r\n");
    CI_LocalPrintf ("0 nr  Init slot [nr]\r\n");
    CI_LocalPrintf ("1 nr  Read slot [nr]\r\n");
    CI_LocalPrintf ("2     Test key generation\r\n");
#ifdef TEST_PBKDF2
    CI_LocalPrintf ("3     PBKDF2 test\r\n");
#endif
    CI_LocalPrintf ("\r\n");
    return;
  }

  switch (CMD_u8)
  {
    case 0:
      CI_LocalPrintf ("Init slot : %d\r\n",Param_u32);
      HV_InitSlot_u8 (Param_u32);
      break;

    case 1:
      HV_ReadSlot_u8 (Param_u32,&SlotData_st);
      HV_PrintSlotData_u8 (Param_u32,&SlotData_st);
      break;

    case 2 :
      TestHiddenVolumeAesKey_u8 ();
      break;
#ifdef TEST_PBKDF2
    case 3 :
      pbkdf2_test ();
      break;
#endif
  }
}

#endif

/*****************************************************************************************/

// Code imported from https://github.com/someone42/hardware-bitcoin-wallet/blob/master/

/*
  Copyright (c) 2011-2012 someone42
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

      Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

      Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

 */


/** PBKDF2 is used to derive encryption keys. In order to make brute-force
* attacks more expensive, this should return a number which is as large
* as possible, without being so large that key derivation requires an
* excessive amount of time (> 1 s). This is a platform-dependent function
* because key derivation speed is platform-dependent.
*
* In order to permit key recovery when the number of iterations is unknown,
* this should be a power of 2. That way, an implementation can use
* successively greater powers of 2 until the correct number of iterations is
* found.
* \return Number of iterations to use in PBKDF2 algorithm.
*/

u32 getPBKDF2Iterations(void)
{
#ifdef TEST_PBKDF2
  return 1024;
#else
  return (128);       // for local CPU speed (ca. 0,3 sec)
#endif
}

/** Write 32 bit unsigned integer into a byte array in big-endian format.
* \param out The destination byte array. This must have space for at
* least 4 bytes.
* \param in The source integer.
*/
void writeU32BigEndian(u8 *out, u32 in)
{
  out[0] = (u8)(in >> 24);
  out[1] = (u8)(in >> 16);
  out[2] = (u8)(in >> 8);
  out[3] = (u8)in;
}



void pbkdf2(u8 *out, const u8 *password, const u32 password_length, const u8 *salt, const u32 salt_length)
{
  u8  u[SHA512_HASH_LENGTH];
  u8  hmac_result[SHA512_HASH_LENGTH];
  u32 u_length;
  u32 num_iterations;
  u32 i;
  u32 j;

  memset(out, 0, SHA512_HASH_LENGTH);
  memset(u, 0, sizeof(u));

  if (salt_length > (SHA512_HASH_LENGTH - 4))
  {
    // Salt too long.
    CI_LocalPrintf ("Salt too long\r\n");
    return;
  }
  else
  {
    u_length = salt_length;
  }

  memcpy(u, salt, u_length);

  writeU32BigEndian(&(u[u_length]), 1);

  u_length += 4;

  num_iterations = getPBKDF2Iterations ();

  for (i = 0; i < num_iterations; i++)
  {

    sha4_hmac(password,password_length,u,u_length,hmac_result, FALSE);

    memcpy(u, hmac_result, sizeof(u));

    u_length = SHA512_HASH_LENGTH;

    for (j = 0; j < SHA512_HASH_LENGTH; j++)
    {
      out[j] ^= u[j];
    }
  }
}



#ifdef TEST_PBKDF2

/** Stores one test vector for pbkdf2(). */
struct PBKDF2TestVector
{
  const char *password;
  unsigned long password_length;
  const char *salt;
  unsigned long salt_length;
  const u8 expected_result[SHA512_HASH_LENGTH];
};

/** Test vectors for pbkdf2().
* It's hard to find publicly available test vectors for PBKDF2-HMAC-SHA512
* (though there are lots for PBKDF2-HMAC-SHA1), so these were generated
* using Python 2.5.2 with pycrypto v2.6. Here's the source:
\code

from Crypto.Hash import SHA512, HMAC
from Crypto.Protocol.KDF import PBKDF2
import sys

p = "1" # Set to password
s = "2" # Set to salt

f = lambda key, msg: HMAC.new(key, msg, SHA512).digest()
out = PBKDF2(password=p, salt=s, dkLen=64, count=1024, prf=f)
str = out.encode("hex")

first = True
while len(str) > 0:
if not first:
sys.stdout.write(", ")
first = False
if len(str) % 16 == 0:
sys.stdout.write("\n")
sys.stdout.write("0x" + str[0:2])
str = str[2:]

\endcode
* \showinitializer
*/
struct PBKDF2TestVector pbkdf2_test_vectors[] =
{

  {"1", 1,
  "2", 1,
  {0xa1, 0x80, 0x45, 0x1f, 0x46, 0x18, 0xdf, 0x95,
  0x15, 0xab, 0x0b, 0xe2, 0xc5, 0x6a, 0xc3, 0x42,
  0x02, 0x87, 0xcb, 0x8f, 0xc0, 0x15, 0xf7, 0x84,
  0x94, 0xc9, 0x39, 0x4a, 0x62, 0xef, 0x6e, 0x66,
  0x57, 0xfd, 0xd1, 0x81, 0x1f, 0xfb, 0x8c, 0x54,
  0xeb, 0x89, 0x2e, 0xa5, 0x94, 0xa7, 0xe1, 0xf6,
  0xed, 0x81, 0xf2, 0x72, 0x64, 0xa8, 0xe9, 0xb1,
  0xc6, 0xed, 0x63, 0x9f, 0x35, 0xbb, 0x12, 0xe8}},

  {"", 0,
  "", 0,
  {0xfb, 0xaa, 0x2a, 0xcb, 0x43, 0xd1, 0xe1, 0xaf,
  0xff, 0x85, 0x83, 0x58, 0xaf, 0x28, 0x43, 0xcc,
  0x57, 0x8d, 0xb4, 0xe2, 0x2b, 0x94, 0x21, 0x18,
  0x09, 0xc0, 0xbf, 0x47, 0x29, 0xd2, 0x6c, 0x2f,
  0x65, 0x4a, 0x2d, 0x6c, 0xe6, 0xd5, 0x2b, 0xea,
  0x06, 0x20, 0xa7, 0xdb, 0x62, 0x32, 0x1c, 0x96,
  0x38, 0xae, 0xb2, 0xd1, 0x16, 0x61, 0x46, 0x7f,
  0xee, 0x96, 0x81, 0x04, 0xcc, 0x1b, 0x51, 0xa4}},

  {"Bitcoin uses peer to peer technology to operate with no central authority", 73,
  "Common salt has the chemical formula NaCl", 41,
  {0x34, 0x5c, 0x92, 0xb1, 0xaa, 0xf0, 0xc1, 0x9e,
  0xfd, 0x22, 0xfd, 0x80, 0x6d, 0x27, 0x35, 0x87,
  0xdc, 0xe8, 0x9c, 0xfa, 0xe6, 0xdc, 0x02, 0x50,
  0x9e, 0x5a, 0x40, 0xbe, 0xd8, 0x4d, 0x6f, 0x66,
  0xed, 0x24, 0x26, 0x9f, 0xd5, 0xcc, 0xe0, 0x6e,
  0x1f, 0xf4, 0xf5, 0x89, 0x83, 0xc5, 0x6e, 0x60,
  0xda, 0xb2, 0x9c, 0xd8, 0xb5, 0x7e, 0xe3, 0x24,
  0xf5, 0x37, 0xe7, 0xe6, 0x39, 0x83, 0x80, 0x37}},

  {"Bitcoin uses peer to peer technology to operate with no central authority", 73,
  "", 0,
  {0x5d, 0x8f, 0x77, 0x0f, 0xc9, 0xa7, 0x8c, 0x9f,
  0x7d, 0x3d, 0xde, 0x84, 0x49, 0xe0, 0x66, 0xca,
  0x3d, 0x62, 0x27, 0x97, 0x9f, 0x21, 0x8e, 0x18,
  0x93, 0x16, 0x87, 0xda, 0x0c, 0xfc, 0x8f, 0xd5,
  0x13, 0xdd, 0x1d, 0xb4, 0xcb, 0x29, 0x8b, 0x2c,
  0xa2, 0xe7, 0x34, 0x3d, 0xfd, 0xf3, 0x69, 0x4f,
  0x02, 0x69, 0x24, 0xea, 0xdd, 0x02, 0x5b, 0x0b,
  0x47, 0xcd, 0x39, 0xd3, 0x1b, 0x67, 0x42, 0x1d}},

  {"", 0,
  "Common salt has the chemical formula NaCl", 41,
  {0x12, 0x77, 0x94, 0xe0, 0x0d, 0xd3, 0x10, 0xcc,
  0xdb, 0x58, 0xc3, 0xc0, 0x35, 0x92, 0xe4, 0x0d,
  0x54, 0xc4, 0x10, 0xaf, 0x49, 0x98, 0x39, 0xee,
  0x91, 0xc4, 0x0f, 0xe6, 0x2f, 0xea, 0xe3, 0xc4,
  0x9f, 0xa0, 0x6a, 0x79, 0x9a, 0x2d, 0xe8, 0x29,
  0x48, 0xeb, 0x5f, 0x43, 0x40, 0xe9, 0x93, 0x64,
  0x99, 0xfb, 0x63, 0x38, 0x19, 0x07, 0x1b, 0xa2,
  0x7a, 0x72, 0x74, 0x7c, 0xb0, 0x43, 0xd5, 0x7a}},

  {"Pass\0word", 9,
  "Sa\0lt", 5,
  {0xaa, 0x55, 0xb0, 0x5f, 0x2c, 0x77, 0x11, 0xbb,
  0x83, 0xea, 0x01, 0xe5, 0x62, 0xc2, 0x5c, 0x8d,
  0x9f, 0xc1, 0xdc, 0xb2, 0xc5, 0xac, 0x7a, 0x36,
  0x20, 0x70, 0x86, 0x81, 0xb5, 0x15, 0x11, 0x29,
  0xf1, 0x31, 0x1b, 0xa0, 0xed, 0x88, 0xf7, 0x1a,
  0xa9, 0x01, 0x88, 0xf3, 0x22, 0x13, 0x0f, 0xf1,
  0x6a, 0x68, 0x3d, 0x47, 0x42, 0x7c, 0x2f, 0x31,
  0x75, 0xfc, 0xe4, 0x7a, 0x8b, 0xc2, 0xa0, 0xf5}},

  {"\x00\x01\xff\xfe", 4,
  "\x00\x01\xff\xfe", 4,
  {0x58, 0xee, 0x8a, 0xe3, 0x70, 0xc4, 0xed, 0x62,
  0x88, 0xf0, 0x48, 0x50, 0xf7, 0x22, 0x6e, 0xe1,
  0x82, 0xf3, 0x69, 0xaa, 0xdc, 0xf1, 0x1e, 0x6b,
  0xfc, 0xf5, 0x27, 0x0c, 0xec, 0x90, 0x9c, 0x90,
  0xd5, 0xee, 0x78, 0x26, 0x15, 0xd1, 0xc3, 0x45,
  0x61, 0x89, 0xba, 0x59, 0xd1, 0xde, 0xe4, 0xf0,
  0xaa, 0x28, 0x45, 0x38, 0xf1, 0x15, 0x15, 0x9c,
  0xad, 0x6a, 0x15, 0xe6, 0xff, 0x56, 0xe7, 0xdf}},

  {"Test vector", 11,
  "This is a maximum length salt ..ABCDEFGH[][][][][][][]\\\\!!!!", 60,
  {0xe0, 0xc1, 0x06, 0x48, 0xfc, 0x64, 0xa9, 0x3a,
  0xd1, 0xc8, 0x35, 0x1a, 0x2c, 0x03, 0x6b, 0xc6,
  0x16, 0xf5, 0x75, 0x96, 0xc1, 0x34, 0x5d, 0x73,
  0x63, 0xe7, 0x6a, 0x3f, 0x47, 0xd0, 0x97, 0x72,
  0x81, 0xf7, 0x78, 0x65, 0x01, 0x4e, 0x43, 0x11,
  0xa5, 0x9d, 0x3a, 0xb6, 0xe8, 0xbc, 0xe0, 0x2c,
  0x97, 0x37, 0x5c, 0xe8, 0x99, 0xf7, 0x4f, 0x81,
  0xd6, 0xbb, 0xdb, 0x1c, 0x03, 0x52, 0x0c, 0x43}},

  {"LongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLong\
  LongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLong\
  LongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLong\
  LongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLong\
  LongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLong\
  LongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLong\
  LongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLong\
  LongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLongLongCatIsLong\
  LongCatIsLongLongCatIsLong", 546,
  "salt", 4,
  {0x46, 0xdd, 0x09, 0xe7, 0x17, 0x2a, 0xf3, 0x93,
  0x6f, 0x36, 0xb5, 0x5a, 0xcc, 0x51, 0xc8, 0x09,
  0xf6, 0xcc, 0xd3, 0x1e, 0x8e, 0x53, 0x17, 0xea,
  0xa7, 0x9c, 0xab, 0xab, 0x28, 0xc7, 0xe6, 0x16,
  0x9f, 0xa9, 0x56, 0x0a, 0xc9, 0x4b, 0x53, 0x20,
  0x1f, 0xbc, 0x06, 0x04, 0xd6, 0xa5, 0xf0, 0xc2,
  0x39, 0x4f, 0xee, 0xa9, 0x91, 0x73, 0x61, 0xd7,
  0xf5, 0xb6, 0x3a, 0xdb, 0x30, 0x0f, 0xdc, 0x85}}
};

void pbkdf2_test (void)
{
  u32 num_test_vectors;
  u32 i;
  u8 out[SHA512_HASH_LENGTH];

//  initTests(__FILE__);

  num_test_vectors = sizeof(pbkdf2_test_vectors) / sizeof(struct PBKDF2TestVector);

//  num_test_vectors = 1;

  for (i = 0; i < num_test_vectors; i++)
  {
    CI_LocalPrintf ("Test %d - PW len %3d - Salt len %3d - ",i,pbkdf2_test_vectors[i].password_length,pbkdf2_test_vectors[i].salt_length);

    pbkdf2(
        out,
        (const u8 *)pbkdf2_test_vectors[i].password,
                    pbkdf2_test_vectors[i].password_length,
        (const u8 *)pbkdf2_test_vectors[i].salt,
                    pbkdf2_test_vectors[i].salt_length);


    if (memcmp(out, pbkdf2_test_vectors[i].expected_result, SHA512_HASH_LENGTH))
    {
      CI_LocalPrintf("fail, got:\n");
      HexPrint (SHA512_HASH_LENGTH,out);
      CI_LocalPrintf ("\r\n");

      CI_LocalPrintf("\nExpected:\n");
      HexPrint (SHA512_HASH_LENGTH,pbkdf2_test_vectors[i].expected_result);
      CI_LocalPrintf ("\r\n");
    }
    else
    {
      CI_LocalPrintf ("Test %d ok\r\n",i);
     }
  }

//  finishTests();
}

#endif // #ifdef TEST_PBKDF2











