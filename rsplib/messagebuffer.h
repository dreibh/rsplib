/*
 *  $Id: messagebuffer.h,v 1.2 2004/11/13 03:24:13 dreibh Exp $
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
 * Purpose:  Message Buffer
 *
 */

#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H

#include "tdtypes.h"
#include "sockaddrunion.h"


#ifdef __cplusplus
extern "C" {
#endif


struct TLVHeader
{
   uint16_t Type;
   uint16_t Length;
};

struct MessageBuffer
{
   size_t Size;
   size_t ToGo;
   size_t Position;
   card64 StartTimeStamp;
   char   Buffer[0];
};


/**
  * Constructor.
  *
  * @param size Buffer size.
  * @return MessageBuffer.
  */
struct MessageBuffer* messageBufferNew(const size_t size);

/**
  * Destructor.
  *
  * @param mb MessageBuffer.
  */
void messageBufferDelete(struct MessageBuffer* mb);

/**
  * Read from buffer.
  *
  * @param mb MessageBuffer.
  * @param fd File descriptor.
  * @param sourceAddress Pointer to sockaddr_union to store source address to.
  * @param sourceAddressLength Pointer to variable containing maximum source address length. Actual length will be written to it.
  * @param requiredPPID Required PPID (SCTP only).
  * @param peekTimeout Maximum timeout to peek message size.
  * @param readTimeout Maximum timeout to read complete message.
  * @return Size of message read or error code (< 0) in case of error.
  */
ssize_t messageBufferRead(struct MessageBuffer* mb,
                          int                   fd,
                          union sockaddr_union* sourceAddress,
                          socklen_t*            sourceAddressLength,
                          const uint32_t        requiredPPID,
                          const card64          peekTimeout,
                          const card64          readTimeout);


#ifdef __cplusplus
}
#endif

#endif
