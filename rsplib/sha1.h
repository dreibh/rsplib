/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2012 by Thomas Dreibholz
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

#ifndef SHA1_H
#define SHA1_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA1_DIGEST_SIZE      20
#define SHA1_HMAC_BLOCK_SIZE  64

struct sha1_ctx {
   uint64_t count;
   uint32_t state[5];
   uint8_t buffer[64];
};

void sha1_init(struct sha1_ctx* ctx);
void sha1_update(struct sha1_ctx* ctx, const uint8_t* data, unsigned int len);
void sha1_final(struct sha1_ctx* ctx, uint8_t* out);

unsigned long long sha1_computeHashOfFile(const char* filename, uint8_t* hash);

#ifdef __cplusplus
}
#endif

#endif
