/*
 * Copyright (C) 2012 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MD5.h"

#ifdef WIN32

#include <windows.h>

typedef struct MD5_CTX {
  ULONG         i[2];
  ULONG         buf[4];
  unsigned char in[64];
  unsigned char digest[16];
} MD5_CTX;

typedef void (WINAPI* F_MD5Init)(MD5_CTX *context);
typedef void (WINAPI* F_MD5Update)(MD5_CTX *context, const unsigned char *input, unsigned int inlen);
typedef void (WINAPI* F_MD5Final)(MD5_CTX *context);

//first time calling API will be redirect to these functions
static void WINAPI myMD5Init(MD5_CTX *context);
static void WINAPI myMD5Update(MD5_CTX *context, const unsigned char *input, unsigned int inlen);
static void WINAPI myMD5Final(MD5_CTX *context);

static F_MD5Init MD5Init=myMD5Init;
static F_MD5Update MD5Update=myMD5Update;
static F_MD5Final MD5Final=myMD5Final;

#elif defined(ANDROID)

/* When OpenSSL is not available we use this code segment */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

#include <string.h>

/* UINT4 defines a four byte word */
typedef unsigned int UINT4;

/* MD5 context. */
struct md5_ctx {
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
};

typedef struct md5_ctx MD5_CTX;

static void MD5_Init(struct md5_ctx *);
static void MD5_Update(struct md5_ctx *, const unsigned char *, unsigned int);
static void MD5_Final(unsigned char [16], struct md5_ctx *);

/* Constants for MD5Transform routine.
 */

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static void MD5Transform(UINT4 [4], const unsigned char [64]);
static void Encode(unsigned char *, UINT4 *, unsigned int);
static void Decode(UINT4 *, const unsigned char *, unsigned int);

static const unsigned char PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

/* MD5 initialization. Begins an MD5 operation, writing a new context.
 */
static void MD5_Init(struct md5_ctx *context)
{
  context->count[0] = context->count[1] = 0;
  /* Load magic initialization constants. */
  context->state[0] = 0x67452301;
  context->state[1] = 0xefcdab89;
  context->state[2] = 0x98badcfe;
  context->state[3] = 0x10325476;
}

/* MD5 block update operation. Continues an MD5 message-digest
  operation, processing another message block, and updating the
  context.
 */
static void MD5_Update (struct md5_ctx *context,    /* context */
                        const unsigned char *input, /* input block */
                        unsigned int inputLen)      /* length of input block */
{
  unsigned int i, bufindex, partLen;

  /* Compute number of bytes mod 64 */
  bufindex = (unsigned int)((context->count[0] >> 3) & 0x3F);

  /* Update number of bits */
  if((context->count[0] += ((UINT4)inputLen << 3))
      < ((UINT4)inputLen << 3))
    context->count[1]++;
  context->count[1] += ((UINT4)inputLen >> 29);

  partLen = 64 - bufindex;

  /* Transform as many times as possible. */
  if(inputLen >= partLen) {
    memcpy(&context->buffer[bufindex], input, partLen);
    MD5Transform(context->state, context->buffer);

    for (i = partLen; i + 63 < inputLen; i += 64)
      MD5Transform(context->state, &input[i]);

    bufindex = 0;
  }
  else
    i = 0;

  /* Buffer remaining input */
  memcpy(&context->buffer[bufindex], &input[i], inputLen-i);
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the
   the message digest and zeroizing the context.
*/
static void MD5_Final(unsigned char digest[16], /* message digest */
                      struct md5_ctx *context) /* context */
{
  unsigned char bits[8];
  unsigned int count, padLen;

  /* Save number of bits */
  Encode (bits, context->count, 8);

  /* Pad out to 56 mod 64. */
  count = (unsigned int)((context->count[0] >> 3) & 0x3f);
  padLen = (count < 56) ? (56 - count) : (120 - count);
  MD5_Update (context, PADDING, padLen);

  /* Append length (before padding) */
  MD5_Update (context, bits, 8);

  /* Store state in digest */
  Encode (digest, context->state, 16);

  /* Zeroize sensitive information. */
  memset ((void *)context, 0, sizeof (*context));
}

/* MD5 basic transformation. Transforms state based on block. */
static void MD5Transform(UINT4 state[4],
                         const unsigned char block[64])
{
  UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

  Decode (x, block, 64);

  /* Round 1 */
  FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
  FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
  FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
  FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
  FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
  FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
  FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
  FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
  FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
  FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
  FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
  FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
  FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
  FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
  FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
  FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

 /* Round 2 */
  GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
  GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
  GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
  GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
  GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
  GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
  GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
  GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
  GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
  GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
  GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
  GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
  GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
  GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
  GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
  GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

  /* Round 3 */
  HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
  HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
  HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
  HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
  HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
  HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
  HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
  HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
  HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
  HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
  HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
  HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
  HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
  HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
  HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
  HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

  /* Round 4 */
  II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
  II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
  II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
  II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
  II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
  II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
  II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
  II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
  II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
  II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
  II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
  II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
  II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
  II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
  II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
  II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  /* Zeroize sensitive information. */
  memset((void *)x, 0, sizeof (x));
}

/* Encodes input (UINT4) into output (unsigned char). Assumes len is
  a multiple of 4.
 */
static void Encode (unsigned char *output,
                    UINT4 *input,
                    unsigned int len)
{
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4) {
    output[j] = (unsigned char)(input[i] & 0xff);
    output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
    output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
    output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
  }
}

/* Decodes input (unsigned char) into output (UINT4). Assumes len is
   a multiple of 4.
*/
static void Decode (UINT4 *output,
                    const unsigned char *input,
                    unsigned int len)
{
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
    output[i] = ((UINT4)input[j]) | (((UINT4)input[j+1]) << 8) |
      (((UINT4)input[j+2]) << 16) | (((UINT4)input[j+3]) << 24);
}

static unsigned char *MD5(const unsigned char *d, unsigned long n, unsigned char *md){
	MD5_CTX ctx;
	static unsigned char digest[16];

	MD5_Init(&ctx);
	MD5_Update(&ctx,d,n);
	MD5_Final(digest,&ctx);

	if(md){
		memcpy(md,digest,16);
		return md;
	}else{
		return digest;
	}
}

#else

#include <openssl/md5.h>

#endif

#include <assert.h>
#include <string.h>

#ifdef WIN32

static void loadMD5Functions(){
	HMODULE h;

	h=LoadLibraryA("cryptdll.dll");
	if(h!=NULL){
		MD5Init=(F_MD5Init)GetProcAddress(h,"MD5Init");
		MD5Update=(F_MD5Update)GetProcAddress(h,"MD5Update");
		MD5Final=(F_MD5Final)GetProcAddress(h,"MD5Final");
		if(MD5Init==NULL || MD5Update==NULL || MD5Final==NULL) FreeLibrary(h);
		else return;
	}

	h=LoadLibraryA("advapi32.dll");
	if(h!=NULL){
		MD5Init=(F_MD5Init)GetProcAddress(h,"MD5Init");
		MD5Update=(F_MD5Update)GetProcAddress(h,"MD5Update");
		MD5Final=(F_MD5Final)GetProcAddress(h,"MD5Final");
		if(MD5Init==NULL || MD5Update==NULL || MD5Final==NULL) FreeLibrary(h);
		else return;
	}

	FatalAppExitA(-1,"Can't find MD5 functions!!!");
}

static void WINAPI myMD5Init(MD5_CTX *context){
	loadMD5Functions();
	MD5Init(context);
}

static void WINAPI myMD5Update(MD5_CTX *context, const unsigned char *input, unsigned int inlen){
	loadMD5Functions();
	MD5Update(context,input,inlen);
}

static void WINAPI myMD5Final(MD5_CTX *context){
	loadMD5Functions();
	MD5Final(context);
}

#endif

//compute the MD5 message digest of the n bytes at d and place it in md
//(which must have space for 16 bytes of output). If md is NULL,
//the digest is placed in a static array. 
unsigned char *Md5::calc(const void *d, unsigned long n, unsigned char *md){
#ifdef WIN32
	static MD5_CTX ctx;

	MD5Init(&ctx);
	MD5Update(&ctx,(const unsigned char*)d,n);
	MD5Final(&ctx);

	if(md){
		memcpy(md,ctx.digest,16);
		return md;
	}else{
		return ctx.digest;
	}
#else
	return MD5((const unsigned char*)d,n,md);
#endif
}

char *Md5::toString(unsigned char *md){
	static char s[40];
	const char* hex="0123456789abcdef";
	
	for(int i=0;i<16;i++){
		s[i*2]=hex[(md[i]&0xF0)>>4];
		s[i*2+1]=hex[md[i]&0xF];
	}
	s[32]='\0';

	return s;
}

//initializes the class for calculating MD5.
void Md5::init(){
	//First check the size
	assert(sizeof(MD5_CTX)<=MD5_CTX_SIZE);

#ifdef WIN32
	MD5Init((MD5_CTX*)md5_ctx);
#else
	MD5_Init((MD5_CTX*)md5_ctx);
#endif
}

//add chunks of the message to be hashed (len bytes at data). 
void Md5::update(const void *data, unsigned long len){
	//First check the size
	assert(sizeof(MD5_CTX)<=MD5_CTX_SIZE);

#ifdef WIN32
	MD5Update((MD5_CTX*)md5_ctx,(const unsigned char*)data,len);
#else
	MD5_Update((MD5_CTX*)md5_ctx,(const unsigned char*)data,len);
#endif
}

//finished the caluclation, places the message digest in md,
//which must have space for 16 bytes of output (or NULL).
unsigned char *Md5::final(unsigned char *md){
	static unsigned char digest[16];
	//First check the size
	assert(sizeof(MD5_CTX)<=MD5_CTX_SIZE);

#ifdef WIN32
	MD5_CTX *ctx=(MD5_CTX*)md5_ctx;
	MD5Final(ctx);
	if(md==NULL) md=digest;
	memcpy(md,ctx->digest,16);
	return md;
#else
	if(md==NULL) md=digest;
	MD5_Final(md,(MD5_CTX*)md5_ctx);
	return md;
#endif
}
