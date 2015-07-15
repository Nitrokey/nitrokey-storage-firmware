/*
   --------------------------------------------------------------------------- Copyright (c) 1998-2008, Brian Gladman, Worcester, UK. All rights
   reserved.

   LICENSE TERMS

   The redistribution and use of this software (with or without changes) is allowed without the payment of fees or royalties provided that:

   1. source code distributions include the above copyright notice, this list of conditions and the following disclaimer;

   2. binary distributions include the above copyright notice, this list of conditions and the following disclaimer in their documentation;

   3. the name of the copyright holder is not used to endorse products built using this software without specific written permission.

   DISCLAIMER

   This software is provided 'as is' with no explicit or implied warranties in respect of its properties, including, but not limited to, correctness
   and/or fitness for purpose. --------------------------------------------------------------------------- Issue Date: 20/12/2007

   My thanks to both Doug Whiting and Olaf Pors for their much appreciated assistance in debugging and testing this code. */

#include "mode_hdr.h"
#include "xts.h"

UNIT_TYPEDEF (buf_unit, UNIT_BITS);
BUFR_TYPEDEF (buf_type, UNIT_BITS, AES_BLOCK_SIZE);

void gf_mulx (void* x)
{
#if UNIT_BITS == 8

    uint_8t i = 16, t = ((uint_8t *) x)[15];
    while (--i)
        ((uint_8t *) x)[i] = (((uint_8t *) x)[i] << 1) | (((uint_8t *) x)[i - 1] & 0x80 ? 1 : 0);
    ((uint_8t *) x)[0] = (((uint_8t *) x)[0] << 1) ^ (t & 0x80 ? 0x87 : 0x00);

#elif PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN

#  if UNIT_BITS == 64

#   define GF_MASK  li_64(8000000000000000)
#   define GF_XOR   li_64(0000000000000087)
    uint_64t _tt = ((UPTR_CAST (x, 64)[1] & GF_MASK) ? GF_XOR : 0);
    UPTR_CAST (x, 64)[1] = (UPTR_CAST (x, 64)[1] << 1) | (UPTR_CAST (x, 64)[0] & GF_MASK ? 1 : 0);
    UPTR_CAST (x, 64)[0] = (UPTR_CAST (x, 64)[0] << 1) ^ _tt;

#  else /* UNIT_BITS == 32 */

#   define GF_MASK  li_32(80000000)
#   define GF_XOR   li_32(00000087)
    uint_32t _tt = ((UPTR_CAST (x, 32)[3] & GF_MASK) ? GF_XOR : 0);;
    UPTR_CAST (x, 32)[3] = (UPTR_CAST (x, 32)[3] << 1) | (UPTR_CAST (x, 32)[2] & GF_MASK ? 1 : 0);
    UPTR_CAST (x, 32)[2] = (UPTR_CAST (x, 32)[2] << 1) | (UPTR_CAST (x, 32)[1] & GF_MASK ? 1 : 0);
    UPTR_CAST (x, 32)[1] = (UPTR_CAST (x, 32)[1] << 1) | (UPTR_CAST (x, 32)[0] & GF_MASK ? 1 : 0);
    UPTR_CAST (x, 32)[0] = (UPTR_CAST (x, 32)[0] << 1) ^ _tt;

#  endif

#else /* PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN */

#  if UNIT_BITS == 64

#   define MASK_01  li_64(0101010101010101)
#   define GF_MASK  li_64(0000000000000080)
#   define GF_XOR   li_64(8700000000000000)
    uint_64t _tt = ((UPTR_CAST (x, 64)[1] & GF_MASK) ? GF_XOR : 0);
    UPTR_CAST (x, 64)[1] = ((UPTR_CAST (x, 64)[1] << 1) & ~MASK_01) | (((UPTR_CAST (x, 64)[1] >> 15) | (UPTR_CAST (x, 64)[0] << 49)) & MASK_01);
    UPTR_CAST (x, 64)[0] = (((UPTR_CAST (x, 64)[0] << 1) & ~MASK_01) | ((UPTR_CAST (x, 64)[0] >> 15) & MASK_01)) ^ _tt;

#  else /* UNIT_BITS == 32 */

#   define MASK_01  li_32(01010101)
#   define GF_MASK  li_32(00000080)
#   define GF_XOR   li_32(87000000)
    uint_32t _tt = ((UPTR_CAST (x, 32)[3] & GF_MASK) ? GF_XOR : 0);
    UPTR_CAST (x, 32)[3] = ((UPTR_CAST (x, 32)[3] << 1) & ~MASK_01) | (((UPTR_CAST (x, 32)[3] >> 15) | (UPTR_CAST (x, 32)[2] << 17)) & MASK_01);
    UPTR_CAST (x, 32)[2] = ((UPTR_CAST (x, 32)[2] << 1) & ~MASK_01) | (((UPTR_CAST (x, 32)[2] >> 15) | (UPTR_CAST (x, 32)[1] << 17)) & MASK_01);
    UPTR_CAST (x, 32)[1] = ((UPTR_CAST (x, 32)[1] << 1) & ~MASK_01) | (((UPTR_CAST (x, 32)[1] >> 15) | (UPTR_CAST (x, 32)[0] << 17)) & MASK_01);
    UPTR_CAST (x, 32)[0] = (((UPTR_CAST (x, 32)[0] << 1) & ~MASK_01) | ((UPTR_CAST (x, 32)[0] >> 15) & MASK_01)) ^ _tt;

#  endif

#endif
}

INT_RETURN xts_encrypt_key (const unsigned char key[], int key_len, xts_encrypt_ctx ctx[1])
{
    int aes_klen_by;

    switch (key_len)
    {
        default:
            return EXIT_FAILURE;
        case 32:
        case 256:
            aes_klen_by = 16;
            break;
        case 64:
        case 512:
            aes_klen_by = 32;
            break;
    }

    return aes_encrypt_key (key, aes_klen_by, ctx->enc_ctx) == EXIT_SUCCESS &&
        aes_encrypt_key (key + aes_klen_by, aes_klen_by, ctx->twk_ctx) == EXIT_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

INT_RETURN xts_decrypt_key (const unsigned char key[], int key_len, xts_decrypt_ctx ctx[1])
{
int aes_klen_by;

    switch (key_len)
    {
        default:
            return EXIT_FAILURE;
        case 32:
        case 256:
            aes_klen_by = 16;
            break;
        case 64:
        case 512:
            aes_klen_by = 32;
            break;
    }

    return aes_decrypt_key (key, aes_klen_by, ctx->dec_ctx) == EXIT_SUCCESS &&
        aes_encrypt_key (key + aes_klen_by, aes_klen_by, ctx->twk_ctx) == EXIT_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

INT_RETURN xts_encrypt_sector (unsigned char sector[], lba_type sector_address, unsigned int sector_len, const xts_encrypt_ctx ctx[1])
{
buf_type hh;
uint_8t* pos = sector, *hi = sector + sector_len;

xor_function f_ptr = (!ALIGN_OFFSET (sector, UNIT_BITS >> 3) ? xor_block_aligned : xor_block);

    if (sector_len < AES_BLOCK_SIZE)
        return EXIT_FAILURE;

#if defined( LONG_LBA )
    *UPTR_CAST (hh, 64) = sector_address;
    memset (UPTR_CAST (hh, 8) + 8, 0, 8);
    uint_64t_to_le (*UPTR_CAST (hh, 64));
#else
    *UPTR_CAST (hh, 32) = sector_address;
    memset (UPTR_CAST (hh, 8) + 4, 0, 12);
    uint_32t_to_le (*UPTR_CAST (hh, 32));
#endif

    aes_encrypt (UPTR_CAST (hh, 8), UPTR_CAST (hh, 8), ctx->twk_ctx);

    while (pos + AES_BLOCK_SIZE <= hi)
    {
        f_ptr (pos, pos, hh);
        aes_encrypt (pos, pos, ctx->enc_ctx);
        f_ptr (pos, pos, hh);
        pos += AES_BLOCK_SIZE;
        gf_mulx (hh);
    }

    if (pos < hi)
    {
uint_8t* tp = pos - AES_BLOCK_SIZE;
        while (pos < hi)
        {
uint_8t tt = *(pos - AES_BLOCK_SIZE);
            *(pos - AES_BLOCK_SIZE) = *pos;
            *pos++ = tt;
        }
        f_ptr (tp, tp, hh);
        aes_encrypt (tp, tp, ctx->enc_ctx);
        f_ptr (tp, tp, hh);
    }
    return EXIT_SUCCESS;
}

INT_RETURN xts_decrypt_sector (unsigned char sector[], lba_type sector_address, unsigned int sector_len, const xts_decrypt_ctx ctx[1])
{
buf_type hh, hh2;
uint_8t* pos = sector, *hi = sector + sector_len;

xor_function f_ptr = (!ALIGN_OFFSET (sector, UNIT_BITS >> 3) ? xor_block_aligned : xor_block);

    if (sector_len < AES_BLOCK_SIZE)
        return EXIT_FAILURE;

#if defined( LONG_LBA )
    *UPTR_CAST (hh, 64) = sector_address;
    memset (UPTR_CAST (hh, 8) + 8, 0, 8);
    uint_64t_to_le (*UPTR_CAST (hh, 64));
#else
    *UPTR_CAST (hh, 32) = sector_address;
    memset (UPTR_CAST (hh, 8) + 4, 0, 12);
    uint_32t_to_le (*UPTR_CAST (hh, 32));
#endif

    aes_encrypt (UPTR_CAST (hh, 8), UPTR_CAST (hh, 8), ctx->twk_ctx);

    while (pos + AES_BLOCK_SIZE <= hi)
    {
        if (hi - pos > AES_BLOCK_SIZE && hi - pos < 2 * AES_BLOCK_SIZE)
        {
            memcpy (hh2, hh, AES_BLOCK_SIZE);
            gf_mulx (hh);
        }
        f_ptr (pos, pos, hh);
        aes_decrypt (pos, pos, ctx->dec_ctx);
        f_ptr (pos, pos, hh);
        pos += AES_BLOCK_SIZE;
        gf_mulx (hh);
    }

    if (pos < hi)
    {
uint_8t* tp = pos - AES_BLOCK_SIZE;
        while (pos < hi)
        {
uint_8t tt = *(pos - AES_BLOCK_SIZE);
            *(pos - AES_BLOCK_SIZE) = *pos;
            *pos++ = tt;
        }
        f_ptr (tp, tp, hh2);
        aes_decrypt (tp, tp, ctx->dec_ctx);
        f_ptr (tp, tp, hh2);
    }

    return EXIT_SUCCESS;
}
