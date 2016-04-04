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
 * Copyright (C) 2002-2016 by Thomas Dreibholz
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

#ifndef SCRIPTINGPACKETS_H
#define SCRIPTINGPACKETS_H

#include "tdtypes.h"


#define PPID_SP                  35 /* old value: 0x29097604 */

#define SPT_NOTREADY           0x00
#define SPT_READY              0x01
#define SPT_UPLOAD             0x02
#define SPT_DOWNLOAD           0x03
#define SPT_KEEPALIVE          0x04
#define SPT_KEEPALIVE_ACK      0x05
#define SPT_STATUS             0x06
#define SPT_ENVIRONMENT        0x07

#define SD_MAX_DATASIZE       16384
#define SR_MAX_INFOSIZE          64
#define SE_HASH_SIZE             20   /* 160-bit SHA-1 hash */

struct ScriptingCommonHeader
{
   uint8_t  Type;
   uint8_t  Flags;
   uint16_t Length;
} __attribute__((packed));


struct NotReady
{
   struct ScriptingCommonHeader Header;
   uint32_t                     Reason;
   char                         Info[SR_MAX_INFOSIZE];
} __attribute__((packed));

#define SSNR_FULLY_LOADED     0x00000001
#define SSNR_OUT_OF_RESOURCES 0x00000002

struct Ready
{
   struct ScriptingCommonHeader Header;
   char                         Info[SR_MAX_INFOSIZE];
} __attribute__((packed));

struct Upload
{
   struct ScriptingCommonHeader Header;
   char                         Data[SD_MAX_DATASIZE];
} __attribute__((packed));

struct Download
{
   struct ScriptingCommonHeader Header;
   char                         Data[SD_MAX_DATASIZE];
} __attribute__((packed));

struct KeepAlive
{
   struct ScriptingCommonHeader Header;
} __attribute__((packed));

struct KeepAliveAck
{
   struct ScriptingCommonHeader Header;
   uint32_t                     Status;
} __attribute__((packed));

struct Status
{
   struct ScriptingCommonHeader Header;
   uint32_t                     Status;
} __attribute__((packed));

struct Environment
{
   struct ScriptingCommonHeader Header;
   uint8_t                      Hash[SE_HASH_SIZE];
} __attribute__((packed));

#define SEF_UPLOAD_NEEDED   (1 << 0)

#endif
