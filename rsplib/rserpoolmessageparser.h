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
 * Copyright (C) 2002-2010 by Thomas Dreibholz
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

#ifndef RSERPOOLMESSAGEPARSER_H
#define RSERPOOLMESSAGEPARSER_H

#include "tdtypes.h"
#include "rserpoolmessage.h"
#include "sockaddrunion.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
  * Create RSerPoolMessage structure from ENRP packet.
  *
  * @param packet Packet.
  * @param sourceAddress Source address.
  * @param ppid PPID of the packet (ASAP or ENRP).
  * @param packetSize Size of packet.
  * @param minBufferSize Minimum size of RSerPoolMessage's buffer (e.g. for reply message).
  * @param message Reference to store RSerPoolMessage to.
  * @return Error code.
  */
unsigned int rserpoolPacket2Message(char*                       packet,
                                    const union sockaddr_union* sourceAddress,
                                    const sctp_assoc_t          assocID,
                                    const uint32_t              ppid,
                                    const size_t                packetSize,
                                    const size_t                minBufferSize,
                                    struct RSerPoolMessage**    message);


#ifdef __cplusplus
}
#endif

#endif
