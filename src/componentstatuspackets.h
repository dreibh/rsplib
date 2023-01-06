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

#ifndef COMPONENTSTATUSPACKETS_H
#define COMPONENTSTATUSPACKETS_H

#include "rserpool-csp.h"
#include "tdtypes.h"


#define CSP_VERSION 0x0200
#define CSPT_REPORT   0x01


struct ComponentStatusCommonHeader
{
   uint8_t  Type;
   uint8_t  Flags;
   uint16_t Length;
   uint32_t Version;
   uint64_t SenderID;
   uint64_t SenderTimeStamp;
} __attribute__((packed));

#define CSPF_FINAL         (1 << 0)


#define CSPR_LOCATION_SIZE 128
#define CSPR_STATUS_SIZE   128

struct ComponentAssociation
{
   uint64_t ReceiverID;
   uint64_t Duration;
   uint16_t Flags;
   uint16_t ProtocolID;
   uint32_t PPID;
} __attribute__((packed));

struct ComponentStatusReport
{
   struct ComponentStatusCommonHeader Header;

   uint32_t                           ReportInterval;
   char                               Location[CSPR_LOCATION_SIZE];
   char                               Status[CSPR_STATUS_SIZE];

   uint16_t                           Workload;
   uint16_t                           Associations;
   struct ComponentAssociation        AssociationArray[0];
} __attribute__((packed));

#define CSR_SET_WORKLOAD(w) ((w < 0.0) ? 0xffff : (uint16_t)rint(w * 0xfffe))
#define CSR_GET_WORKLOAD(w) ((w == 0xffff) ? -1.0 : (double)(w / (double)0xfffe))

#endif
