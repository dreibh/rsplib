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
 * Copyright (C) 2002-2025 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#ifndef CALCAPPPACKETS_H
#define CALCAPPPACKETS_H

#include "netdouble.h"


#define PPID_CALCAPP         34   /* old value: 0x29097603 */

#define CALCAPP_REQUEST       1
#define CALCAPP_ACCEPT        2
#define CALCAPP_REJECT        3
#define CALCAPP_ABORT         4
#define CALCAPP_COMPLETE      5
#define CALCAPP_KEEPALIVE     6
#define CALCAPP_KEEPALIVE_ACK 7

struct CalcAppMessage
{
   uint8_t  Type;
   uint8_t  Flags;
   uint16_t Length;

   uint32_t JobID;
   uint64_t JobSize;
   uint64_t Completed;
} __attribute__((packed));


struct CalcAppCookie
{
   uint32_t JobID;
   uint64_t JobSize;
   uint64_t Completed;
} __attribute__((packed));

#endif
