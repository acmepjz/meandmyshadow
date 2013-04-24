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

#include <string.h>
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

#else
/*
 * NOTE: This code is used for both Android and Unix based systems.
 * The code is based on the md5 code of md5deep (http://sourceforge.net/projects/md5deep/), which is released under the public domain.
 */

#include <stdint.h>

typedef struct {
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
} context_md5_t;

// This is needed to make RSAREF happy on some MS-DOS compilers
typedef context_md5_t MD5_CTX;


static void byteReverse(unsigned char *buf, unsigned longs)
{
    uint32_t t;
    do {
	t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
	    ((unsigned) buf[1] << 8 | buf[0]);
	*(uint32_t *) buf = t;
	buf += 4;
    } while (--longs);
}

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )


/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void MD5_Transform(uint32_t buf[4], uint32_t const in[16])
{
    register uint32_t a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
static void MD5_Init(context_md5_t *ctx)
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
static void MD5_Update(context_md5_t *ctx, const unsigned char *buf, size_t len)
{
    uint32_t t;

    /* Update bitcount */

    t = ctx->bits[0];
    if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
	ctx->bits[1]++;		/* Carry from low to high */
    ctx->bits[1] += len >> 29;

    t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

    /* Handle any leading odd-sized chunks */

    if (t) {
	unsigned char *p = (unsigned char *) ctx->in + t;

	t = 64 - t;
	if (len < t) {
	    memcpy(p, buf, len);
	    return;
	}
	memcpy(p, buf, t);
	byteReverse(ctx->in, 16);
	MD5_Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += t;
	len -= t;
    }
    /* Process data in 64-byte chunks */

    while (len >= 64) {
	memcpy(ctx->in, buf, 64);
	byteReverse(ctx->in, 16);
	MD5_Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += 64;
	len -= 64;
    }

    /* Handle any remaining bytes of data. */

    memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
static void MD5_Final(unsigned char digest[16], context_md5_t *ctx)
{
    unsigned count;
    unsigned char *p;

    /* Compute number of bytes mod 64 */
    count = (ctx->bits[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8) {
	/* Two lots of padding:  Pad the first block to 64 bytes */
	memset(p, 0, count);
	byteReverse(ctx->in, 16);
	MD5_Transform(ctx->buf, (uint32_t *) ctx->in);

	/* Now fill the next block with 56 bytes */
	memset(ctx->in, 0, 56);
    } else {
	/* Pad block to 56 bytes */
	memset(p, 0, count - 8);
    }
    byteReverse(ctx->in, 14);

    /* Append length in bits and transform */

    // the two lines below generated this error:
    // "md5.c:147:5: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]"

    //((uint32_t *) ctx->in)[14] = ctx->bits[0];
    //((uint32_t *) ctx->in)[15] = ctx->bits[1];

    // We will manually expand the cast into two statements to make
    // the compiler happy...

    uint32_t *ctxin = (uint32_t *)ctx->in;
    ctxin[14] = ctx->bits[0];
    ctxin[15] = ctx->bits[1];

    MD5_Transform(ctx->buf, (uint32_t *) ctx->in);
    byteReverse((unsigned char *) ctx->buf, 4);
    memcpy(digest, ctx->buf, 16);

    memset(ctx, 0, sizeof(* ctx));	/* In case it's sensitive */
    /* The original version of this code omitted the asterisk. In
       effect, only the first part of ctx was wiped with zeros, not
       the whole thing. Bug found by Derek Jones. Original line: */
    // memset(ctx, 0, sizeof(ctx));	/* In case it's sensitive */
}

static unsigned char* MD5(const unsigned char* d,unsigned long n,unsigned char* md){
	static unsigned char result[16];
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context,d,n);
	MD5_Final(result,&context);
	if(md){
		memcpy(md,result,16);
		return md;
	}else{
		return result;
	}
}
#endif

#include <assert.h>

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
