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

#ifndef _BRG_ENDIAN_H
#define _BRG_ENDIAN_H

#define IS_BIG_ENDIAN      4321 /* byte 0 is most significant (mc68k) */
#define IS_LITTLE_ENDIAN   1234 /* byte 0 is least significant (i386) */


//#define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN			// 4321
#define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN		// 1234


#endif
