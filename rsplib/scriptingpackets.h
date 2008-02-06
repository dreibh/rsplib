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
 * Copyright (C) 2002-2008 by Thomas Dreibholz
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

#include <stdint.h>


// Ensure consistent alignment!
#pragma pack(push)
#pragma pack(4)


#define PPID_SP          0x29097604

#define SPT_READY              0x01
#define SPT_UPLOAD             0x02
#define SPT_DOWNLOAD           0x03
#define SPT_KEEPALIVE          0x04
#define SPT_KEEPALIVE_ACK      0x05
#define SPT_STATUS             0x06

#define SD_MAX_DATASIZE       16384

struct ScriptingCommonHeader
{
   uint8_t  Type;
   uint8_t  Flags;
   uint16_t Length;
};


struct Ready
{
   struct ScriptingCommonHeader Header;
};

struct Upload
{
   struct ScriptingCommonHeader Header;
   char                         Data[SD_MAX_DATASIZE];
};

struct UploadComplete
{
   struct ScriptingCommonHeader Header;
};

struct Download
{
   struct ScriptingCommonHeader Header;
   char                         Data[SD_MAX_DATASIZE];
};

struct DownloadComplete
{
   struct ScriptingCommonHeader Header;
};

struct KeepAlive
{
   struct ScriptingCommonHeader Header;
};

struct KeepAliveAck
{
   struct ScriptingCommonHeader Header;
   uint32_t                     Status;
};

struct Status
{
   struct ScriptingCommonHeader Header;
   uint32_t                     Status;
};


#pragma pack(pop)

#endif
