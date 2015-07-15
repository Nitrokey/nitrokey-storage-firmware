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
   and/or fitness for purpose. --------------------------------------------------------------------------- Issue Date: 20/12/2007 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#if defined( __BORLANDC__ )
#  define str_compare strnicmp
#else
#  define str_compare strnicmp
#endif

#if defined( DUAL_CORE ) && defined( _WIN32 )
#include <windows.h>
#endif

#include "xts.h"

int CI_LocalPrintf (char* szFormat, ...);

#define printf CI_LocalPrintf

#define BLOCK_SIZE AES_BLOCK_SIZE

enum des_type
{ EKY = 0, TKY, LBA, PTX, CTX,  /* sequence values */
    VEC = 16, END, REM, SLN, NUL, RPT, ERR
};  /* non-sequence values */

char* des_name[] = { "EKY ", "TKY ", "LBA ", "PTX ", "CTX ", "SLN ", "VEC ", "END", "REM ", "RPT " };

enum des_type des_m[] =
{ EKY, TKY, LBA, PTX, CTX, SLN, VEC, END, REM, RPT };

enum des_type find_des (char* s, char** cptr)
{
    int i = (int) strlen (s);

    while (i && (s[i - 1] < ' ' || s[i - 1] > '\x7e'))
        --i;
    s[i] = '\0';
    if (i == 0)
        return NUL;
    for (i = 0; i < sizeof (des_name) / sizeof (des_name[0]); ++i)
        if (!str_compare (des_name[i], s, strlen (des_name[i])))
        {
            *cptr = s + strlen (des_name[i]);
            return des_m[i];
        }
    return ERR;
}

unsigned int hex (char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return 0;
}

unsigned char hex_in (char* s)
{
    return 16 * hex (s[0]) + hex (s[1]);
}

unsigned int dec_in (char* s)
{
    unsigned int n = 0;

    while (*s >= '0' && *s <= '9')
        n = 10 * n + *s++ - '0';
    return n;
}

uint_64t bytes_to_number (unsigned char x[], int len)
{
uint_64t t = 0;

    while (len--)
        t = 256 * t + x[len];
    return t;
}

void do_test (const char* in_dir, const char* out_dir, const char* name, int gen_flag)
{
    char tvi_name[64], tvo_name[64], line[80],* lp;
    FILE* inf = 0, *outf = 0;
    unsigned char tky[32], eky[64], sln[16], lba[16], ptx[4096], ctx[4096], res[4096],* p;
    int tky_len, eky_len, sln_len, lba_len, ptx_len, ctx_len, ptx_rpt, rpt_cnt, vec_no,* cnt,* rpt = &rpt_cnt, err, i;
    enum des_type line_is;
    xts_encrypt_ctx ectx[1];
    xts_decrypt_ctx dctx[1];

    strcpy (tvi_name, in_dir);
    lp = tvi_name + strlen (tvi_name);
    if (strrchr (tvi_name, '\\') != lp - 1 && strrchr (tvi_name, '/') != lp - 1)
    {
        *lp++ = '\\';
        *lp = '\0';
    }
    strcat (tvi_name, name);
    strcat (tvi_name, ".0");
    if (gen_flag)
    {
        strcpy (tvo_name, out_dir);
        lp = tvo_name + strlen (tvo_name);
        if (strrchr (tvo_name, '\\') != lp - 1 && strrchr (tvo_name, '/') != lp - 1)
        {
            *lp++ = '\\';
            *lp = '\0';
        }
        strcat (tvo_name, name);
        strcat (tvo_name, ".0");
    }
    for (;;)
    {
        tvi_name[strlen (tvi_name) - 1]++;

        if (gen_flag)
        {
            tvo_name[strlen (tvo_name) - 1]++;
            lp = tvo_name - 1;
            while (*++lp)
                *lp = tolower (*lp);
        }

        if (!(inf = fopen (tvi_name, "r")))
            break;

        printf ("\nUsing %s test vectors in \"%s\": ", name, tvi_name);
        do
        {
            fgets (line, 80, inf);
            if (str_compare (line, "MDE ", 4) == 0)
                break;
            if (str_compare (line, "END ", 4) == 0)
                break;
        }
        while (!feof (inf));

        if (feof (inf) || str_compare (line, "END ", 4) == 0)
            break;

        if (str_compare (line + 4, name, 3) != 0)
        {
            printf ("this file does not match %s", name);
            break;
        }

        if (gen_flag)
        {
            if (!(outf = fopen (tvo_name, "w")))
                break;
            fprintf (outf, "\nMDE %s", name);
        }

        for (;;)
        {
            err = -1;
            fgets (line, 80, inf);
            if (feof (inf))
                break;

            if (strlen (line) < 4)
            {
                tky_len = eky_len = sln_len = lba_len = ptx_len = ctx_len = rpt_cnt = 0;
                ptx_rpt = 1;
                if (gen_flag)
                    fprintf (outf, "\n");
                continue;
            }

            switch (line_is = find_des (line, &lp))
            {
                case VEC:
                    vec_no = dec_in (lp);
                    if (gen_flag)
                        fprintf (outf, "\nVEC %04i", vec_no);
                    continue;
                case RPT:
                    *rpt = dec_in (lp);
                    if (gen_flag)
                        fprintf (outf, "\nRPT %04i", *rpt);
                    continue;
                case EKY:
                    p = eky, cnt = &eky_len, rpt = &rpt_cnt;
                    break;
                case TKY:
                    p = tky, cnt = &tky_len, rpt = &rpt_cnt;
                    break;
                case SLN:
                    p = sln, cnt = &sln_len, rpt = &rpt_cnt;
                    break;
                case LBA:
                    p = lba, cnt = &lba_len, rpt = &rpt_cnt;
                    break;
                case PTX:
                    p = ptx, cnt = &ptx_len, rpt = &ptx_rpt;
                    break;
                case CTX:
                    p = ctx, cnt = &ctx_len, rpt = &rpt_cnt;
                    break;
                case END:
                case REM:
                case NUL:
                    continue;
                case ERR:
                    printf ("\nThe file \'%s\' contains an unrecognised line\n", tvi_name);
                    continue;
            }

            if (line_is == END)
                break;

            if (gen_flag)
            {
                if (line[strlen (line) - 1] == '\n' || line[strlen (line) - 1] == '\r')
                    line[strlen (line) - 1] = '\0';
                if (line_is != CTX)
                    fprintf (outf, "\n%s", line);
            }

            if (line_is == REM)
                continue;

            lp = line + 4;
            while (*lp != '\n' && *lp != '\0' && *(lp + 1) != '\n' && *(lp + 1) != '\0')
            {
                p[(*cnt)++] = hex_in (lp);
                lp += 2;
            }

            if (line_is != CTX || ctx_len < ptx_len)
                continue;

            if (sln_len && ptx_len != bytes_to_number (sln, sln_len))
            {
                printf ("plaintext length is not equal to sector length");
                break;
            }

            err = 0;
            memcpy (eky + eky_len, tky, tky_len);
            xts_encrypt_key (eky, eky_len + tky_len, ectx);
            xts_decrypt_key (eky, eky_len + tky_len, dctx);

            i = ptx_rpt;
            while (i--)
            {
                memcpy (res, ptx, ptx_len);
                xts_encrypt_sector (res, bytes_to_number (lba, lba_len), ptx_len, ectx);
                if (memcmp (ctx, res, ptx_len))
                {
                    printf ("\n\tencrypt error on test number %i", vec_no), err++;
                    break;
                }
                xts_decrypt_sector (res, bytes_to_number (lba, lba_len), ptx_len, dctx);
                if (memcmp (ptx, res, ptx_len))
                {
                    printf ("\n\tdecrypt error on test number %i", vec_no), err++;
                    break;
                }
            }
            if (err)
                break;
        }

        if (err <= 0)
            printf ("matched");
        if (inf)
            fclose (inf);
        if (gen_flag && outf)
            fclose (outf);
    }
    return;
}

const int loops = 100;          // number of timing loops

unsigned int rand32 (void)
{
    static unsigned int r4, r_cnt = -1, w = 521288629, z = 362436069;

    z = 36969 * (z & 65535) + (z >> 16);
    w = 18000 * (w & 65535) + (w >> 16);

    r_cnt = 0;
    r4 = (z << 16) + w;
    return r4;
}

unsigned char rand8 (void)
{
    static unsigned int r4, r_cnt = 4;

    if (r_cnt == 4)
    {
        r4 = rand32 ();
        r_cnt = 0;
    }

    return (char) (r4 >> (8 * r_cnt++));
}

// fill a block with random charactrers

void block_rndfill (unsigned char l[], unsigned int len)
{
    unsigned int i;

    for (i = 0; i < len; ++i)

        l[i] = rand8 ();
}

// int XTS_TestMain(int argc, char *argv[])
int XTS_TestMain (char* path_pu8, char* node_name_pu8)
{
    do_test (path_pu8, path_pu8, node_name_pu8, 0);
    return 0;
}
