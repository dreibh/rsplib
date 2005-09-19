/*
 *  $Id$
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: RSerPool Message Parser
 *
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
