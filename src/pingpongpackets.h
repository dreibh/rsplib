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

#ifndef PINGPONGPACKETS_H
#define PINGPONGPACKETS_H

#include "tdtypes.h"


#define PPID_PPP    33   /* old value: 0x29097602 */

#define PPPT_PING 0x01
#define PPPT_PONG 0x02

struct PingPongCommonHeader
{
   uint8_t  Type;
   uint8_t  Flags;
   uint16_t Length;
} __attribute__((packed));

struct Ping
{
   struct PingPongCommonHeader Header;
   uint64_t                    MessageNo;
   char                        Data[];
} __attribute__((packed));

struct Pong
{
   struct PingPongCommonHeader Header;
   uint64_t                    MessageNo;
   uint64_t                    ReplyNo;
   char                        Data[];
} __attribute__((packed));


#define PPP_COOKIE_ID "<PP-TD1>"

struct PPPCookie
{
   char     ID[8];
   uint64_t ReplyNo;
} __attribute__((packed));

#endif
