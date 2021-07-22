/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 15.06.2012
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
 * HandleAesStorageKey.c
 *
 *  Created on: 15.06.2012
 *      Author: RB
 */

#include "USER_INTERFACE/file_io.h"
#include "string.h"
#include "aes.h"
#include "flashc.h"
#include "time.h"

#include "global.h"
#include "tools.h"
#include "Interpreter.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"
#include "FlashStorage.h"
#include "HandleAesStorageKey.h"
#include "password_safe.h"
#include "HiddenVolume.h"
#include "../OTP/hmac-sha1.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

#define LOCAL_DEBUG
// #define LOCAL_DEBUG_CHECK_KEY_GENERATION

#define AES_KEYSIZE_256_BIT     32  // 32 * 8 = 256

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/
void CalculateAndSaveStorageKeyHash_u32(const u8 * StorageKey_pu8);


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

u32 BuildNewAesStorageKey_u32 (u8 * MasterKey_pu8)
{
    // u32 Ret_u32;
u8 StorageKey_au8[AES_KEYSIZE_256_BIT];
u8 Buffer_au8[AES_KEYSIZE_256_BIT];

#ifdef LOCAL_DEBUG
    CI_TickLocalPrintf ("BuildNewAesStorageKey\r\n");
#endif
    // Wait for next smartcard cmd
    DelayMs (10);

    // Get a random number for the storage key
    if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT / 2, StorageKey_au8))
    {
#ifdef LOCAL_DEBUG
        CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
        return (FALSE);
    }

    // Get a random number for the storage key
    if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT / 2, &StorageKey_au8[AES_KEYSIZE_256_BIT / 2]))
    {
#ifdef LOCAL_DEBUG
        CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
        return (FALSE);
    }

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("Uncrypted storage key       : ");
    HexPrint (AES_KEYSIZE_256_BIT, StorageKey_au8);
    CI_LocalPrintf ("\r\n");
#endif

    CalculateAndSaveStorageKeyHash_u32(StorageKey_au8);

    memcpy (Buffer_au8, StorageKey_au8, AES_KEYSIZE_256_BIT);

    // Local encryption of the storage key
    AES_StorageKeyEncryption (AES_KEYSIZE_256_BIT, Buffer_au8, MasterKey_pu8, AES_PMODE_CIPHER);

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("Local encrypted storage key : ");
    HexPrint (AES_KEYSIZE_256_BIT, Buffer_au8);
    CI_LocalPrintf ("\r\n");
#endif

    // Store the encrypted storage key in USER PAGE
    WriteAESStorageKeyToUserPage (Buffer_au8);

#ifdef LOCAL_DEBUG_CHECK_KEY_GENERATION
    // Test the storage key

    // Wait for next smartcard cmd
    DelayMs (10);

    /* Enable smartcard */
    if (FALSE == LA_OpenPGP_V20_Test_SendUserPW2 ((u8 *) "123456"))
    {
        return (FALSE);
    }

    // Wait for next smartcard cmd
    DelayMs (10);

    /* SC AES key decryption */
    if (FALSE == LA_OpenPGP_V20_Test_ScAESKey (AES_KEYSIZE_256_BIT, Buffer_au8))
    {
        return (FALSE);
    }

    CI_LocalPrintf ("SC decrypted storage key    : ");
    HexPrint (AES_KEYSIZE_256_BIT, Buffer_au8);
    CI_LocalPrintf ("\r\n");

#endif

    // Clear the critical memory
    memset_safe (StorageKey_au8, 0, AES_KEYSIZE_256_BIT);
    memset_safe (Buffer_au8, 0, AES_KEYSIZE_256_BIT);


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

u32 BuildNewAesMasterKey_u32 (u8 * AdminPW_pu8, u8 * MasterKey_pu8)
{
#ifdef LOCAL_DEBUG
    CI_TickLocalPrintf ("BuildNewAesMasterKey_u32\r\n");
#endif
    LA_RestartSmartcard_u8 ();

    // Wait for next smartcard cmd
    // DelayMs (10);

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber\n\r");
#endif

    // Get a random number for the master key
    if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT / 2, MasterKey_pu8))
    {
#ifdef LOCAL_DEBUG
        CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
        return (FALSE);
    }

    // Get a random number for the master key
    if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT / 2, &MasterKey_pu8[AES_KEYSIZE_256_BIT / 2]))
    {
#ifdef LOCAL_DEBUG
        CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
        return (FALSE);
    }

    // Wait for next smartcard cmd
    DelayMs (10);

#ifdef LOCAL_DEBUG
    CI_TickLocalPrintf ("Send AdminPW -%s-\r\n", AdminPW_pu8);
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
    HexPrint (AES_KEYSIZE_256_BIT, MasterKey_pu8);
    CI_TickLocalPrintf ("\r\n");
#endif

    // Wait for next smartcard cmd
    DelayMs (10);

    // Store master key in smartcard
    if (FALSE == LA_OpenPGP_V20_Test_SendAESMasterKey (AES_KEYSIZE_256_BIT, MasterKey_pu8))
    {
#ifdef LOCAL_DEBUG
        CI_TickLocalPrintf ("SendAESMasterKey fails\r\n");
#endif
        return (FALSE);
    }

    ClearStickKeysNotInitatedToFlash ();

    return (TRUE);
}


/*******************************************************************************

  BuildNewXorPattern_u32

  Build a xor pattern, with this pattern every key received from the smartcard is xored

  Changes
  Date      Author          Info
  20.05.14  RB              Implementation of build xor pattern

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 BuildNewXorPattern_u32 (void)
{
u8 XorPattern_au8[AES_KEYSIZE_256_BIT];
    // time_t now;
    // u32 i;

#ifdef LOCAL_DEBUG
    CI_TickLocalPrintf ("BuildNewXorPattern_u32\r\n");
#endif
    LA_RestartSmartcard_u8 ();


#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("GetRandomNumber\n\r");
#endif

    // Get a random number for the master key
    if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT / 2, XorPattern_au8))
    {
#ifdef LOCAL_DEBUG
        CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
        return (FALSE);
    }

    // Get a random number for the master key
    if (FALSE == GetRandomNumber_u32 (AES_KEYSIZE_256_BIT / 2, &XorPattern_au8[AES_KEYSIZE_256_BIT / 2]))
    {
#ifdef LOCAL_DEBUG
        CI_LocalPrintf ("GetRandomNumber fails\n\r");
#endif
        return (FALSE);
    }

    WriteXorPatternToFlash (XorPattern_au8);

    return (TRUE);
}

/*******************************************************************************

  XorAesKey_v

  Changes
  Date      Author          Info
  20.05.14  RB              Implementation of xor a aes key

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void XorAesKey_v (u8 * AesKey_au8)
{
u8 XorKey_au8[AES_KEYSIZE_256_BIT];
u32 i;

    ReadXorPatternFromFlash (XorKey_au8);

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("XorAesKey with     : ");
    HexPrint (AES_KEYSIZE_256_BIT, XorKey_au8);
    CI_LocalPrintf ("\r\n");
#endif

    for (i = 0; i < AES_KEYSIZE_256_BIT; i++)
    {
        AesKey_au8[i] = AesKey_au8[i] ^ XorKey_au8[i];
    }

#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("New AesKey         : ");
    HexPrint (AES_KEYSIZE_256_BIT, AesKey_au8);
    CI_LocalPrintf ("\r\n");
#endif

}

/*******************************************************************************

  BuildStorageKeys_u32

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 BuildStorageKeys_u32 (u8 * AdminPW_pu8)
{
u32 Ret_u32;
u8 MasterKey_au8[AES_KEYSIZE_256_BIT];

    // Check admin password
    // Unlock smartcard for sending master key
    if (FALSE == LA_OpenPGP_V20_Test_SendAdminPW (AdminPW_pu8))
    {
#ifdef LOCAL_DEBUG
        CI_TickLocalPrintf ("AdminPW wrong\r\n");
#endif
        return (FALSE);
    }

    Ret_u32 = EraseLocalFlashKeyValues_u32 ();
    if (FALSE == Ret_u32)
    {
        return (FALSE);
    }


    Ret_u32 = BuildNewXorPattern_u32 ();
    if (FALSE == Ret_u32)
    {
        return (FALSE);
    }

    Ret_u32 = BuildNewAesMasterKey_u32 (AdminPW_pu8, MasterKey_au8);
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
    memset_safe (MasterKey_au8, 0, AES_KEYSIZE_256_BIT);

    return (TRUE);
}


/*******************************************************************************

  DecryptKeyViaSmartcard_u32

  Changes
  Date      Reviewer        Info
  13.04.14  RB              Modification for multiple hidden volumes

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

u32 DecryptKeyViaSmartcard_u32 (u8 * StorageKey_pu8)
{
#ifdef LOCAL_DEBUG
    CI_LocalPrintf ("DecryptKeyViaSmartcard\r\n");
    // HexPrint (AES_KEYSIZE_256_BIT,StorageKey_pu8);
    // CI_LocalPrintf ("\r\n");
#endif

    /* SC AES key decryption */
    if (FALSE == LA_OpenPGP_V20_Test_ScAESKey (AES_KEYSIZE_256_BIT, StorageKey_pu8))
    {
#ifdef LOCAL_DEBUG
        CI_LocalPrintf ("Smartcard access failed\r\n");
#endif
        return (FALSE);
    }

    XorAesKey_v (StorageKey_pu8);

#ifdef LOCAL_DEBUG
    // CI_LocalPrintf ("DecryptKeyViaSmartcard_u32: Decrypted key : ");
    // HexPrint (AES_KEYSIZE_256_BIT,StorageKey_pu8);
    // CI_LocalPrintf ("\r\n");
#endif

    return (TRUE);
}

int memcmp_safe(const u8 *a, size_t a_len, const u8 *b, size_t b_len){
    // constant time, buffer length checked compare
    // returns  0 on the same content of both buffers
    //          -1 on different buffer sizes
    //          +a on xor calculated differences, 1..255
    // NOT a drop-in replacement for memcmp
    if (a_len != b_len) {
        return -1;
    }
    int diff = 0, i;
    for (i = 0; i < a_len; ++i) {
        diff |= a[i] ^ b[i];
    }
    return diff;
}

u32 CheckStorageKeyHash_u32(const u8 * StorageKey_pu8){
    /**
     * Return TRUE on success, and FALSE if the hash is invalid or could not be fetched
     */
    // 1. get the hash from the flashstorage unit
    printf_file(__FUNCTION__);
    u8 StorageKeyHashSaved[20];
    u8 StorageKeyHashCalculated[20];
    int res = ReadStorageKeyHashFromUserPage(StorageKeyHashSaved, sizeof (StorageKeyHashSaved));
    if (res != TRUE) {
        return (FALSE);
    }

    // 2. run hashing
    const int KeyLengthBytes = AES_KEYSIZE_256_BIT;
    hmac_sha1(StorageKeyHashCalculated, StorageKey_pu8, KeyLengthBytes * 8, StorageKey_pu8, KeyLengthBytes * 8);

    const u8* p = StorageKeyHashSaved;
    printf_file("HashSaved: %x %x %x %x\n", p[0], p[1], p[2], p[3]);
    p = StorageKeyHashCalculated;
    printf_file("HashCalc: %x %x %x %x\n", p[0], p[1], p[2], p[3]);
    p = StorageKey_pu8;
    printf_file("StorageKey_pu8: %x %x %x %x\n", p[0], p[1], p[2], p[3]);

    // 3. compare constant time
//    if (memcmp_safe(StorageKeyHashSaved, sizeof StorageKeyHashSaved, StorageKeyHashCalculated, sizeof StorageKeyHashCalculated) == 0) {
    if (memcmp(StorageKeyHashSaved, StorageKeyHashCalculated, sizeof StorageKeyHashCalculated) == 0) {
        return (TRUE);
    }

    return (FALSE);
}

void CalculateAndSaveStorageKeyHash_u32(const u8 * StorageKey_pu8){
    printf_file(__FUNCTION__);
    u8 StorageKeyHashCalculated[20];
    const int KeyLengthBytes = AES_KEYSIZE_256_BIT;
    hmac_sha1(StorageKeyHashCalculated, StorageKey_pu8, KeyLengthBytes * 8, StorageKey_pu8, KeyLengthBytes * 8);
    WriteStorageKeyHashToUserPage(StorageKeyHashCalculated);
    const u8* p = StorageKeyHashCalculated;
    printf_file("CalcSave: %x %x %x %x\n", p[0], p[1], p[2], p[3]);
    p = StorageKey_pu8;
    printf_file("StorageKey_pu8: %x %x %x %x\n", p[0], p[1], p[2], p[3]);
    memset_safe(StorageKeyHashCalculated, 0, sizeof StorageKeyHashCalculated);
}

u32 IsStorageKeyEmpty_u32(const u8 * StorageKey_pu8, size_t StorageKey_len){
    // check if retrieved key is all-zeroes
    int diff = 0, i;
    for (i = 0; i < StorageKey_len; ++i) {
        diff |= StorageKey_pu8[i];
    }
    if (diff == 0) {
        return (TRUE);
    }

    return (FALSE);
}

/*******************************************************************************

  GetStorageKey_u32

  Key for the normal crypted volume (not the hidden volumes)

  Changes
  Date      Reviewer        Info
  13.04.14  RB              Modification for multiple hidden volumes

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define STORAGE_KEY_SIZE (32)
u32 GetStorageKey_u32 (u8 * UserPW_pu8, u8 * StorageKey_pu8)
{
    printf_file(__FUNCTION__);

    if (FALSE == CheckStorageKey_u8())
    {
        return (FALSE);
    }

    /* Enable smartcard */
    if (FALSE == LA_OpenPGP_V20_Test_SendUserPW2 (UserPW_pu8))
    {
        return (FALSE);
    }

    // Wait for next smartcard cmd
    DelayMs (10);

    // Get encrypted key for the crypted volume
    ReadAESStorageKeyToUserPage (StorageKey_pu8);

    if (FALSE == DecryptKeyViaSmartcard_u32 (StorageKey_pu8))
    {
        return (FALSE);
    }

    if (TRUE == IsStorageKeyEmpty_u32(StorageKey_pu8, STORAGE_KEY_SIZE))
    {
        return (FALSE);
    }

    if (FALSE == CheckStorageKeyHash_u32(StorageKey_pu8))
    {
        return (FALSE);
    }

    return (TRUE);
}

/*******************************************************************************

  CheckStorageKey_u8

  Check Key for the normal encrypted volume (not the hidden volumes)

  Changes
  Date      Reviewer        Info
  05.05.14  RB              Creation

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

u8 CheckStorageKey_u8 (void)
{
u8 StorageKey_au8[AES_KEYSIZE_256_BIT];
u32* p_pu32;

    // Get encrypted key for the crypted volume
    ReadAESStorageKeyToUserPage (StorageKey_au8);

    p_pu32 = (u32 *) & StorageKey_au8[0];
    if ((u32) 0xFFFFFFFF == *p_pu32)
    {
        return (FALSE); // No key generated - this is a security leak
    }

    return (TRUE);
}

/*******************************************************************************

  StartupCheck_u8

  Changes
  Date      Reviewer        Info
  05.05.14  RB              Creation

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

u8 StartupCheck_u8 (void)
{
u8 CheckStatus_u8 = TRUE;

    if (FALSE == CheckStorageKey_u8 ())
    {
        CheckStatus_u8 = FALSE;
    }

    if (FALSE == PWS_CheckPasswordSafeKey_u8 ())
    {
        CheckStatus_u8 = FALSE;
    }

    if (FALSE == HV_CheckHiddenVolumeSlotKey_u8 ())
    {
        CheckStatus_u8 = FALSE;
    }

    if (TRUE == CheckStatus_u8)
    {
        return (TRUE);
    }

    CI_LocalPrintf ("*** AES keys unsecure ***\r\n");

    ReadStickConfigurationFromUserPage ();

    if (TRUE == StickConfiguration_st.StickKeysNotInitiated_u8)
    {
        CI_LocalPrintf ("*** Set flash bit NotInitated ***\r\n");
        SetStickKeysNotInitatedToFlash ();
    }

    return (FALSE);
}

/*******************************************************************************

  HighLevelTests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#ifndef STICK_20_A_MUSTER_PROD
void HighLevelTests (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8)
{
    u8 Buffer_au8[32];

    if (0 == nParamsGet_u8)
    {
        CI_LocalPrintf ("Highlevel test functions\r\n");
        CI_LocalPrintf ("\r\n");
        CI_LocalPrintf ("0          Build storage keys\r\n");
        CI_LocalPrintf ("1          Print 32 byte of USER PAGE\r\n");
        CI_LocalPrintf ("2          Write 12345678 to USER PAGE\r\n");
        CI_LocalPrintf ("3          Write 987654321 to USER PAGE\r\n");
        CI_LocalPrintf ("4          Clear AES storage key\r\n");
        CI_LocalPrintf ("5          Print AES storage key\r\n");
        CI_LocalPrintf ("6          Set new SD card found\r\n");
        CI_LocalPrintf ("\r\n");
        return;
    }

    switch (CMD_u8)
    {
        case 0:
            BuildStorageKeys_u32 ((u8 *) "12345678");
            break;

        case 1:
            CI_LocalPrintf ("Print USER PAGE : ");
            HexPrint (32, (u8 *) AVR32_FLASHC_USER_PAGE);
            CI_LocalPrintf ("\r\n");
            break;

        case 2:
            CI_LocalPrintf ("Write 12345678 to USER PAGE\r\n");
            flashc_memcpy (AVR32_FLASHC_USER_PAGE, "12345678", 8, TRUE);
            break;
        case 3:
            CI_LocalPrintf ("Write 987654321 to USER PAGE\r\n");
            flashc_memcpy (AVR32_FLASHC_USER_PAGE, "987654321", 9, TRUE);
            break;
        case 4:
            CI_LocalPrintf ("Clear AES storage key in flash\r\n");
            memset_safe (Buffer_au8, 255, 32);
            // Store the encrypted storage key in USER PAGE
            WriteAESStorageKeyToUserPage (Buffer_au8);
            break;
        case 5:
            ReadAESStorageKeyToUserPage (Buffer_au8);
            CI_LocalPrintf ("Print AES storage key : ");
            HexPrint (32, (u8 *) Buffer_au8);
            CI_LocalPrintf ("\r\n");
            break;
        case 6:
            CI_LocalPrintf ("Set new SD card found\r\n");
            SetSdCardNotFilledWithRandomCharsToFlash ();
            break;

        default:
            break;
    }
}

#endif