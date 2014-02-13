/* hmac-otp_sha1.c */
/*
    This file is part of the ARM-Crypto-Lib.
    Copyright (C) 2006-2010  Daniel Otte (daniel.otte@rub.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * 
 * implementation of HMAC as described in RFC2104
 * Author:      Daniel Otte
 * email:       daniel.otte@rub.de
 * License:     GPLv3 or later
 **/

/* 
 * hmac = hash ( k^opad , hash( k^ipad  , msg))
 */

#include "global.h"
#include "tools.h"

#include <stdint.h>
#include <string.h>
//#include "config.h"
#include "sha1.h"
#include "hmac-sha1.h"

#define IPAD 0x36
#define OPAD 0x5C


#ifndef HMAC_SHORTONLY

void hmac_sha1_init(hmac_sha1_ctx_t *s, const void* key, u16 keylength_b)
{
	u8 buffer[SHA1_BLOCK_BYTES];
	u8 i;
	
	memset(buffer, 0, SHA1_BLOCK_BYTES);
	if (keylength_b > SHA1_BLOCK_BITS){
		otp_sha1((void*)buffer, key, keylength_b);
	} else {
		memcpy(buffer, key, (keylength_b+7)/8);
	}
	
	for (i=0; i<SHA1_BLOCK_BYTES; ++i){
		buffer[i] ^= IPAD;
	}
	sha1_init(&(s->a));
	sha1_nextBlock(&(s->a), buffer);
	
	for (i=0; i<SHA1_BLOCK_BYTES; ++i){
		buffer[i] ^= IPAD^OPAD;
	}
	sha1_init(&(s->b));
	sha1_nextBlock(&(s->b), buffer);
	
	
#if defined SECURE_WIPE_BUFFER
	memset(buffer, 0, SHA1_BLOCK_BYTES);
#endif
}

void hmac_sha1_nextBlock(hmac_sha1_ctx_t *s, const void* block){
	sha1_nextBlock(&(s->a), block);
}
void hmac_sha1_lastBlock(hmac_sha1_ctx_t *s, const void* block, u16 length_b){
	while(length_b>=SHA1_BLOCK_BITS){
		sha1_nextBlock(&(s->a), block);
		block = (u8*)block + SHA1_BLOCK_BYTES;
		length_b -= SHA1_BLOCK_BITS;
	}
	sha1_lastBlock(&(s->a), block, length_b);
}

void hmac_sha1_final(void* dest, hmac_sha1_ctx_t *s){
	sha1_ctx2hash((sha1_hash_t*)dest, &(s->a));
	sha1_lastBlock(&(s->b), dest, SHA1_HASH_BITS);
	sha1_ctx2hash((sha1_hash_t*)dest, &(s->b));
}

#endif

/*
 * keylength in bits!
 * message length in bits!
 */
void hmac_sha1(void* dest, const void* key, u16 keylength_b, const void* msg, u32 msglength_b){ /* a one-shot*/
	sha1_ctx_t s;
	u8 i;
	u8 buffer[SHA1_BLOCK_BYTES];
	
	memset(buffer, 0, SHA1_BLOCK_BYTES);
	
	/* if key is larger than a block we have to hash it*/
	if (keylength_b > SHA1_BLOCK_BITS){
		otp_sha1((void*)buffer, key, keylength_b);
	} else {
		memcpy(buffer, key, (keylength_b+7)/8);
	}
	
	for (i=0; i<SHA1_BLOCK_BYTES; ++i){
		buffer[i] ^= IPAD;
	}
	sha1_init(&s);
	sha1_nextBlock(&s, buffer);
	while (msglength_b >= SHA1_BLOCK_BITS){
		sha1_nextBlock(&s, msg);
		msg = (u8*)msg + SHA1_BLOCK_BYTES;
		msglength_b -=  SHA1_BLOCK_BITS;
	}
	sha1_lastBlock(&s, msg, msglength_b);
	/* since buffer still contains key xor ipad we can do ... */
	for (i=0; i<SHA1_BLOCK_BYTES; ++i){
		buffer[i] ^= IPAD ^ OPAD;
	}
	sha1_ctx2hash(dest, &s); /* save inner hash temporary to dest */
	sha1_init(&s);
	sha1_nextBlock(&s, buffer);
	sha1_lastBlock(&s, dest, SHA1_HASH_BITS);
	sha1_ctx2hash(dest, &s);
}


