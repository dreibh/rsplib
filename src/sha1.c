/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2023 by Thomas Dreibholz
 * Based on Linux kernel SHA-1 code:
 * Copyright (C) Jean-Luc Cooke <jlcooke@certainkey.com>
 * Copyright (C) Andrew McDonald <andrew@mcdonald.org.uk>
 * Copyright (C) Kyle McMartin <kyle@debian.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#include "sha1.h"

#include <string.h>
#include <netinet/in.h>


static inline uint32_t rol(uint32_t value, uint32_t bits)
{
   return (((value) << (bits)) | ((value) >> (32 - (bits))));
}

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#define blk0(i) block32[i]

#define blk(i) (block32[i&15] = rol(block32[(i+13)&15]^block32[(i+8)&15] \
    ^block32[(i+2)&15]^block32[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5); \
                        w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5); \
                        w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5); \
                        w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


/* Hash a single 512-bit block. This is the core of the algorithm. */
static void sha1_transform(uint32_t *state, const uint8_t *in)
{
   uint32_t a, b, c, d, e;
   uint32_t block32[16];

   /* convert/copy data to workspace */
   for (a = 0; a < sizeof(block32)/sizeof(uint32_t); a++)
     block32[a] = ntohl(((const uint32_t *)in)[a]);

   /* Copy context->state[] to working vars */
   a = state[0];
   b = state[1];
   c = state[2];
   d = state[3];
   e = state[4];

   /* 4 rounds of 20 operations each. Loop unrolled. */
   R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
   R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
   R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
   R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
   R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
   R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
   R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
   R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
   R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
   R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
   R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
   R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
   R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
   R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
   R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
   R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
   R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
   R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
   R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
   R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
   /* Add the working vars back into context.state[] */
   state[0] += a;
   state[1] += b;
   state[2] += c;
   state[3] += d;
   state[4] += e;
   memset (block32, 0x00, sizeof block32);
}

void sha1_init(struct sha1_ctx* ctx)
{
   static const struct sha1_ctx initstate = {
     0,
     { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 },
     { 0, }
   };

   *ctx = initstate;
}

void sha1_update(struct sha1_ctx* ctx, const uint8_t* data, unsigned int len)
{
   unsigned int i, j;

   j = (ctx->count >> 3) & 0x3f;
   ctx->count += len << 3;

   if ((j + len) > 63) {
      memcpy(&ctx->buffer[j], data, (i = 64-j));
      sha1_transform(ctx->state, ctx->buffer);
      for ( ; i + 63 < len; i += 64) {
         sha1_transform(ctx->state, &data[i]);
      }
      j = 0;
   }
   else i = 0;
   memcpy(&ctx->buffer[j], &data[i], len - i);
}


/* Add padding and return the message digest. */
void sha1_final(struct sha1_ctx* ctx, uint8_t *out)
{
   uint32_t i, j, index, padlen;
   uint64_t t;
   uint8_t bits[8] = { 0, };
   static const uint8_t padding[64] = { 0x80, };

   t = ctx->count;
   bits[7] = 0xff & t; t>>=8;
   bits[6] = 0xff & t; t>>=8;
   bits[5] = 0xff & t; t>>=8;
   bits[4] = 0xff & t; t>>=8;
   bits[3] = 0xff & t; t>>=8;
   bits[2] = 0xff & t; t>>=8;
   bits[1] = 0xff & t; t>>=8;
   bits[0] = 0xff & t;

   /* Pad out to 56 mod 64 */
   index = (ctx->count >> 3) & 0x3f;
   padlen = (index < 56) ? (56 - index) : ((64+56) - index);
   sha1_update(ctx, padding, padlen);

   /* Append length */
   sha1_update(ctx, bits, sizeof bits);

   /* Store state in digest */
   for (i = j = 0; i < 5; i++, j += 4) {
      uint32_t t2 = ctx->state[i];
      out[j+3] = t2 & 0xff; t2>>=8;
      out[j+2] = t2 & 0xff; t2>>=8;
      out[j+1] = t2 & 0xff; t2>>=8;
      out[j  ] = t2 & 0xff;
   }

   /* Wipe context */
   memset(ctx, 0, sizeof *ctx);
}


unsigned long long  sha1_computeHashOfFile(const char* filename, uint8_t* hash)
{
   struct sha1_ctx    ctx;
   uint8_t            buffer[65536];
   FILE*              fh;
   ssize_t            count;
   unsigned long long total = 0;

   sha1_init(&ctx);
   fh = fopen(filename, "r");
   if(fh != NULL) {
      while(!feof(fh)) {
         count = fread(&buffer, 1, sizeof(buffer), fh);
         if(count > 0) {
            sha1_update(&ctx, buffer, count);
         }
         total += (unsigned long long)count;
      }
      fclose(fh);
      sha1_final(&ctx, hash);
   }
   return(total);
}
