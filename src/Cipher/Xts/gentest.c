/*
 ---------------------------------------------------------------------------
 Copyright (c) 1998-2008, Brian Gladman, Worcester, UK. All rights reserved.

 LICENSE TERMS

 The redistribution and use of this software (with or without changes)
 is allowed without the payment of fees or royalties provided that:

  1. source code distributions include the above copyright notice, this
     list of conditions and the following disclaimer;

  2. binary distributions include the above copyright notice, this list
     of conditions and the following disclaimer in their documentation;

  3. the name of the copyright holder is not used to endorse products
     built using this software without specific written permission.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 20/12/2007
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#define cstr_cmp _strnicmp
#include "xts.h"

#if defined( _MSC_VER ) && ( _MSC_VER < 1300 ) 
  typedef __int64 int_64t;
#else
  typedef long long int_64t;
#endif


char *rem = 
    "\nREM All inputs and outputs are arrays of 8-bit bytes (octets) with bytes being"
    "\nREM represented by consecutive pairs of hexadecimal digits (the pair 'ab', for"
    "\nREM example, yields the byte value 0xab in C). The index positions of bytes in"
    "\nREM arrays increase by 1 from left to right for each pair of digits and arrays"
    "\nREM on consecutive lines with the same initial designators are concatenated so"
    "\nREM that bytes on later lines have higher array indexes.  Numeric significance"
    "\nREM is undefined except between the two digits that form each individual byte."
    "\n"
    "\nREM EKY = The AES encrypt/decrypt key"
    "\nREM TKY = The AES tweak key"
    "\nREM LBA = The logical block address"
    "\nREM PTX = The unencrypted sector contents" 
    "\nREM CTX = The encrypted sector contents\n"; 

#define MAX_BLOCK_SIZE 4096

/*  The following definitions describe the different lines that can occur
    in test vector files.  Each line starts with a four character line 
    designator in which the last character is a space (which must be 
    present).  Some lines are commands (e.g GEN means generate a test 
    vector) while others specify the values of inputs (e.g. KEY gives the 
    value of the key to be used) while others (e.g. CTX and TAG) give the 
    expected value of outputs. In the list of sequence designators, inputs
    follow outputs and inputs are listed with the values that vary slowly 
    between suquential test vectors before those that vary more rapidly.
*/
enum des_type { EKY  = 0, TKY, LBA, PTX, CTX,               /* sequence values      */
                VEC = 16, GEN, END, REM, SLN, NUL, ERR };   /* non-sequence values  */

/*  Input sequence designators are listed first with the most slowly
    varying values earlier in the list. Then output values are listed,
    followed by other values such as commands and aliases (e.g. IV is
    an alias for NCE).
*/
char *des_name[] =      { "EKY", "TKY", "LBA", "PTX", "CTX", "VEC", "GEN", "END" };
enum des_type des_m[] = {  EKY ,  TKY ,  LBA ,  PTX ,  CTX ,  VEC ,  GEN ,  END  };

/*  Find if a line starts with a recognised line designator and
    return its descriptor if it does or ERR if not.
*/
enum des_type find_des(char *s, char **cptr)
{   size_t i = strlen(s);

    while( i && (s[i - 1] < ' ' || s[i - 1] > '\x7e') )
        --i;
    s[i] = '\0';
    if( i == 0 )
        return NUL;
    for(i = 0; i < sizeof(des_name) / sizeof(des_name[0]); ++i)
        if(!strncmp(des_name[i], s, strlen(des_name[i])))
        {   
            s += strlen(des_name[i]);
            while(*s == ' ' || *s == '\t')
                ++s;
            *cptr = s;
            return des_m[i];
        }
    return ERR;
}

/* input a decimal number from a C string */

char *decimal_in(char *s, int_64t *n)
{
    int g = -1;
    *n = 0;
    if(*s == '-')
        ++s;
    else
        g = 1;

    while( isdigit( *s ) )
        *n = 10 * *n + g * (*s++ - '0');

    return s;
}

int to_hex(char ch)
{
    return (isdigit(ch) ? 0 : 9) + (ch & 0x0f);
}

/* input a hexadecimal number from a C string */

char *hexadecimal_in(char *s, int_64t *n)
{
    *n = 0;
    while( isxdigit( *s ) )
        *n = (*n << 4) + to_hex(*s++);
    return s;
}

/* input an 8-bit byte from two charcters in a C string */

char *hexbyte_in(char *s, unsigned char *n)
{
    if( isxdigit( *s) && isxdigit( *(s + 1) ) )
    {
        *n = (to_hex(*s) << 4) | to_hex(*(s + 1));
        return s + 2;
    }
    else
        return 0;
}

/* output a byte array preceeded by a descriptor */

void hex_out( FILE *f, char *desc, unsigned char buf[], unsigned int len )
{   unsigned int i;

    if( len == 0 )
        fprintf(f, "\n%s ", desc);
    else
        for(i = 0; i < len; ++i)
        {
            if(i % 32 == 0)
                fprintf(f, "\n%s ", desc);
            fprintf(f, "%02x", buf[i]);
        }
}

uint_64t bytes_to_number(unsigned char x[], int len)
{   uint_64t t = 0;

    while(len--)
        t = 256 * t + x[len];
    return t;
}

/* AES based random number generator */

static unsigned char key[16] = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 2, 3, 8, 4 };
static unsigned char rnd[16] = { 2, 7, 1, 8, 2, 8, 1, 8, 2, 8, 4, 5, 9, 0, 4, 5 };
static aes_encrypt_ctx ctx[1];
static int     r_cnt = -1;  /* initialised negative to trigger AES keying */

void rand_update(void)
{
    if( r_cnt < 0 )
        aes_encrypt_key( key, 16, ctx );
    /* OFB stream cipher for randomness */
    aes_encrypt( rnd, rnd, ctx );
    /* number of valid random bytes in buffer */
    r_cnt = 16;
    return;
}

/* return one pseudo random byte */

unsigned char rand8(void)
{
    if(r_cnt <= 0)
        rand_update();
    return rnd[--r_cnt];
}

/* fill a byte array with pseudo random values */

void block_rndfill(unsigned char l[], unsigned long len)
{   unsigned long  i;

    for(i = 0; i < len; ++i)
        l[i] = rand8();
}

#define MAX_BLOCK_SIZE 16 * 256     /* 4096 byte limit on blocks */

/*  This structure accumulates the character strings for each specific
    line designator in a test vector template. The values are accumulated
    in the dynamically allocated C string pointed to by rp.  It also 
    records the position in this string while p;arsing it (pos) and
    numerical values obtained as a result (cnt[6]). The number of values
    obtained, up to five, is set in cnt[0]
*/
typedef struct
{
    char *rp;
    unsigned int  len;
    unsigned int  pos;
    int_64t cnt[6];
} v_rec;

void add_to_record( v_rec *v, char *c)
{   char *cp = c + strlen(c) - 1;

    /* remove extraneous characters at end of input */
    while( *cp <= 0x20 || *cp >= 0x7f )
        --cp;
    if(*(cp + 1))
        *(cp + 1) = '\0';
    if(*c == '\0' )
        return;

    /* if a string has not been allocated or it has but is not long enough */
    if( !v->rp || v->len < strlen( v->rp ) + strlen( c ) + 1 )
    {   char *p;

        /* set a new maximum length and allocate a new string */
        v->len = ( v->rp ? (unsigned int)strlen( v->rp ) : 0 ) + (unsigned int)strlen( c ) + 80;
        p = malloc(  v->len );
        *p = '\0';
        /* if necessary copy old content into it */
        if( v-> rp)
        {
            strcpy( p, v->rp );
            free( v-> rp );
        }
        v->rp = p;
    }
    /* if there is content in the string and the new value is not a */
    /* continuation of a hexadecimal sequence, insert a comma       */ 
    if( *(v->rp) && *c != ',' && v->rp[strlen( v->rp ) - 1 ] != ',' &&        
        (!isxdigit( v->rp[strlen( v->rp ) - 1] ) || !isxdigit( *c )) ) 
        strcat( v->rp, "," );
    /* add the new content */
    strcat( v->rp, c );
}

/*  input strings are comma separated values which are either hexadecimal 
    sequences or 'bracket terms' consisting of up to five comma separated 
    numerical values within brackets.  This routine parses the latter
    bracket terms and puts the number of values obtained in cnt[0] and 
    the values in cnt[1] .. cnt[5]. Numbers are hexadecimal unless they
    are preceeded by a '#', in which case they are decimal.
*/

int get_loop_vars( char **cp, int_64t cnt[], int hex_flag )
{   char *p = *cp;
    int i, retv = 0;

    /* remove any leading space */
    while(*p && ( *p == ' ' || *p == '\t' ) )
        ++p;
    /* parse up to closing bracket or a maximum of 5 inputs */
    for( i = 1 ; *p != ')' && i < 6 ; ++i )
    {
        if( *p == '#' )
            p = decimal_in( p + 1, cnt + i );
        else
            p = hexadecimal_in(p, cnt + i);
        while(*p && ( *p == ' ' || *p == '\t' || *p == ',' ) )
            ++p;
    }

    if( *p == ')' && i < 7 )    /* good input */
        cnt[0] = retv = i - 1;
    else                        /* bad input - remove to next closing bracket */
    {
        while( *p && *p != ')' )
            ++p;
        if(*p)
            ++p;
    }
    *cp = p;
    return retv;
}

int init_sequence( char **cp, unsigned char val[], int_64t cnt[5], int *ll )
{   int retv, i;

    (*cp)++;    /* step past opening bracket */

    /* input the values in the bracket term  */
    retv = get_loop_vars( cp, cnt, 1 );
    *ll = (int)(cnt[1]);
    switch( retv )
    {
    case 1: 
        /* (nn) - nn random bytes */
        block_rndfill( val, (unsigned long)(cnt[1]) );
        /* step past closing bracket */
        (*cp)++;
        break;
    case 2:
        /* (nn,hh) - nn bytes each of value hh  */
        cnt[3] = 0;
    case 3:
        /* (nn,hh,hi) - nn bytes of value hh, hh + hi, hh + 2 * hi, ...  */
        for( i = 0; i < cnt[1]; ++i )
        {    
            val[i] = (unsigned char)(cnt[2]);
            cnt[2] += cnt[3];
        }
        /* step past closing bracket */
        (*cp)++;
        break;
    case 4:
        /* (nn,v1,v2,v3=1..4) - signal with negative cnt[5] value */
        cnt[5] = -cnt[4];
    case 5:
        /* (nn,v1,v2,v3,v4) */
        /* signal a sequence of values */
        return 2;
    default:
        /* signal no value input */
        return 0;
    }
    return 1;
}

void reverse_seq(unsigned char *lo, unsigned char *hi)
{   unsigned char t;
    --lo;
    while(++lo < --hi)
        t = *lo, *lo = *hi, *hi = t;
}

int get_value( v_rec *v, unsigned char val[] , int *len, unsigned char w[])
{
    char *p = v->rp + v->pos;
    int ll = 0, retv = 0;

    if(v->rp == 0)
        return 0;

    /* remove white space and a ',' delimiter if present */
    while(*p && ( *p == ' ' || *p == '\t') )
        ++p;
    if( *p == ',' )
        ++p;

    if( *p )
    {
        if( *p == '=' )
        {   /* use previous ciphertext */
            if( w )
            {
                ++p; 
                retv = 1; 
                memcpy(val, w, *len); 
                ll = *len;
            }
        }
        else if( isxdigit( *p ) && isxdigit( *( p + 1 ) ) )
        {   /* input a hexadecimal byte sequence */
            while( hexbyte_in(p, &val[ll]) )
            {
                p += 2; ++ll;
            }
            retv = 1;
        }
        else if( *p == '#' )
        {   /* input a decimal value and assembler into */
            /* little endian byte sequence              */
            int_64t num;
            int f = 0;
            if(*++p == '>' || *p == '<')
                if(*p++ == '>')
                    f = 1;
            p = decimal_in(p, &num);
            ll = 0;
            while(num)
            {
                val[ll++] = (unsigned char)(num & 0xff);
                num >>= 8;
            }
            if(f)
                reverse_seq(val, val + ll);
            retv = 1;
        }
        else if( *p == '(' )    /* initialise for a bracket term */
            retv = init_sequence( &p, val, v->cnt, &ll );

        /* after the initialisation of a bracket term, the */
        /* parse point is positioned on the final bracket  */
        if( retv == 2 || *p == ')')
        {
            if(v->cnt[1] == 0)  /* if the sequence has finished */
            {                   /* go to next input value       */
                /* leave value in len untouched unless there is */
                /* actual input because we may need the prior   */
                /* length in case we have to copy the previous  */
                /* ciphertext into the next plaintext           */
                v->pos = (unsigned int)(++p - v->rp); ll = *len;
                retv = get_value(v, val, &ll, w);
            }
            else
            {
                (v->cnt[1])--;  /* reduce remaining item count  */
                ll = (int)(v->cnt[2]); /* normal length count   */
                if(v->cnt[5] == -1 || v->cnt[5] == -2)
                {   
                    /* assemble sequential values v1 + n * v2   */   
                    /* (nn,v1,v2,1) - little endian byte array  */
                    /* (nn,v1,v2,2) - big endian byte array     */
                    int_64t x = v->cnt[2];
                    ll = 0;
                    v->cnt[2] += v->cnt[3];
                    while(x)
                    {
                        val[ll++] = (unsigned char)x;
                        x >>= 8;
                    }
                    if(v->cnt[5] == -2)
                        reverse_seq(val, val + ll);
                }
                else if(v->cnt[5] == -3)
                {
                    /* (nn,v1,v2,3) - assemble random array of  */
                    /* bytes of increasing length v1 + n * v2   */
                    block_rndfill(val, ll);
                    v->cnt[2] += v->cnt[3];
                    
                }
                else if(v->cnt[5] == -4)
                {   
                    /* (nn,v1,v2,4) - assemble random array of  */
                    /* bytes of random length v1 <= len <= v2   */
                    uint_64t t = 0;
                    block_rndfill((unsigned char*)&t, 4);
                    ll += (int)(((v->cnt[3] + 1 - v->cnt[2]) * t) >> 32);
                    block_rndfill(val, ll);
                }
                else
                {   
                    /* (nn,v1,v2,h1,h2) - assemble random array */
                    /* bytes of increasing length v1 + n * v2   */
                    /* in which bytes also increment in value   */
                    /* h1 + n * h2 withing each array generated */
                    int i = 0, j = (int)(v->cnt[4]);
                    while(i < ll)
                    {
                        val[i++] = (unsigned char)j; 
                        j += (int)(v->cnt[5]);
                    }
                    v->cnt[2] += v->cnt[3];
                }
                retv = 1;
            }
        }
    }
    /* if a value is returned signal its length */
    if(retv)
        *len = ll;
    v->pos = (unsigned int)(p - v->rp);
    return retv;
}

/* restart a parse of a value record */

void restart( v_rec *v )
{
    v->pos = 0;
}

/* initialise a value record */

void init( v_rec *v )
{
    v->rp = NULL;
    v->len = 0;
    v->pos = 0;
}

/* empty a value record */

void reset( v_rec *v )
{
    if(v->rp)
        *(v->rp) = '\0';
    v->len = 0;
    v->pos = 0;
}

/* delete a value record */

void clear( v_rec *v )
{
    if( v-> rp )
        free( v->rp );
    v->rp = NULL;
    v->len = 0;
    v->pos = 0;
}

int gen_vectors(char *in_file_name, char *out_file_name)
{
    uint_8t     tky[2 * AES_BLOCK_SIZE];    /* the AES key 1            */
    uint_8t     eky[4 * AES_BLOCK_SIZE];    /* the AES key 2            */
    uint_8t     lba[AES_BLOCK_SIZE];        /* the nonce                */
    uint_8t     ptx[MAX_BLOCK_SIZE];        /* the plaintext            */
    uint_8t     ctx[2][MAX_BLOCK_SIZE];     /* the ciphertext           */
    char        line[128];
    v_rec       rec[4];
    int tky_len, eky_len, lba_len, ptx_len, err = EXIT_FAILURE;
    int_64t vec_no;
    FILE *in_file, *out_file;
    xts_encrypt_ctx enc_ctx[1];
    xts_decrypt_ctx dec_ctx[1];

    init( rec ); init( rec + 1 ); init( rec + 2 ); init( rec + 3 ); 

    if( (in_file = fopen( in_file_name, "r" ) ) == NULL )
        goto exit3;

    if( ( out_file = fopen( out_file_name, "w" ) ) == NULL )
        goto exit2;

    for( ; !feof( in_file ) ; )
    {
        do
        {
            fgets( line, 80, in_file );
        }
        while
            (strncmp(line, "MODE", 4) != 0);

        if( strncmp( line + 5, "XTS", 3 ) )
            continue;

        /* output mode header 'MDE XXX' */
        fprintf( out_file, "\nMDE %s", line + 5 );

        {   /* output date and time */
            time_t secs;
            struct tm *now;
            time( &secs );
            now = localtime( &secs );
            fprintf( out_file,  "REM Produced by GENTEST on %s", asctime( now ) );
        }

        /* output line designator summary */
        fprintf( out_file,  rem );

        for( ; !feof( in_file ) ; )
        {
            for( ; !feof( in_file ) ; )
            {   
                /* process lines in template file */
                enum des_type ty;
                char *cptr;
                fgets( line, 80, in_file );
                switch( ty = find_des(line, &cptr) )
                {
                case ERR:
                    goto error;
                case NUL:
                    goto more;
                case VEC:
                    decimal_in(cptr, &vec_no);
                    goto more;
                case GEN:
                case END:
                    goto generate;
                default:
                    if(ty < 0 || ty >= 16)
                        goto error;
                    add_to_record( rec + ty, cptr); 
                    goto more;
                }
            error:      printf("\nunrecognised input: %s", line);
            more:       continue;
            generate:   break;
            }

            while( get_value( rec, eky, &eky_len, 0 ) )
            {
                while( get_value( rec + 1, tky, &tky_len, 0 ) )
                {
                    while( get_value( rec + 2, lba, &lba_len, 0 ) )
                    {
                        while( get_value( rec + 3, ptx, &ptx_len, ctx[0] ) )
                        {
                            memcpy(eky + eky_len, tky, tky_len);
                            if(xts_encrypt_key(eky, eky_len + tky_len, enc_ctx) != EXIT_SUCCESS)
                                goto exit1;
                            if(xts_decrypt_key(eky, eky_len + tky_len, dec_ctx) != EXIT_SUCCESS)
                                goto exit1;
                            memcpy(ctx[0], ptx, ptx_len);
                            if(xts_encrypt_sector(ctx[0], bytes_to_number(lba, lba_len), ptx_len, enc_ctx)
                                                != EXIT_SUCCESS)
                                goto exit1;
                            memcpy(ctx[1], ctx[0], ptx_len);
                            if(xts_decrypt_sector(ctx[1], bytes_to_number(lba, lba_len), ptx_len, dec_ctx)
                                                != EXIT_SUCCESS)
                                goto exit1;

                            if( memcmp( ptx, ctx[1], ptx_len ) )
                            {
                                printf("\nPlaintext Mismatch");
                                goto exit1;
                            }
                            fprintf( out_file, "\nVEC %d", (int)vec_no++);
                            hex_out( out_file, des_name[EKY & 15], eky, eky_len );
                            hex_out( out_file, des_name[TKY & 15], tky, tky_len );
                            hex_out( out_file, des_name[LBA & 15], lba, lba_len );
                            hex_out( out_file, des_name[PTX & 15], ptx, ptx_len );
                            hex_out( out_file, des_name[CTX & 15], ctx[0], ptx_len );
                            fprintf(out_file, "\n");
                        }
                        restart( rec + 3 );
                    }
                    restart( rec + 2 );
                }
                restart( rec + 1 );
            }
            reset( rec + 3 );
            reset( rec + 2 );
            reset( rec + 1 );
            reset( rec );
        }
    }
    fprintf(out_file, "\nEND \n");
exit1:
    clear( rec + 3 );
    clear( rec + 2 );
    clear( rec + 1 );
    clear( rec );
    fclose(out_file);
exit2:
    fclose(in_file);
exit3:
    return err;
}

int main(int argc, char *argv[])
{
    if(argc == 3)
        gen_vectors( argv[1], argv[2] );
    else
        printf("\nusage: gentest vector_rule_file vector_output_file");
    printf("\n\n");
    return 0;
}

