/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#ifndef TRANSPORTADDRESSBLOCK_H
#define TRANSPORTADDRESSBLOCK_H

#include <sys/types.h>
#include <inttypes.h>

#include <stdio.h>

#include "sockaddrunion.h"


#ifdef __cplusplus
extern "C" {
#endif


#define TABF_CONTROLCHANNEL (1 << 0)

struct TransportAddressBlock
{
   struct TransportAddressBlock* Next;
   int                           Protocol;
   uint16_t                      Port;
   uint16_t                      Flags;
   size_t                        Addresses;
   union sockaddr_union          AddressArray[0];
};

#define transportAddressBlockGetSize(addresses) (sizeof(struct TransportAddressBlock) + (addresses * sizeof(union sockaddr_union)))


/**
  * Initialize.
  *
  * @param transportAddressBlock TransportAddressBlock.
  * @param protocol Protocol.
  * @param port Port.
  * @param flags Flags.
  * @param addressArray Address array.
  * @param addresses Number of addresses.
  */
void transportAddressBlockNew(struct TransportAddressBlock* transportAddressBlock,
                              const int                     protocol,
                              const uint16_t                port,
                              const uint16_t                flags,
                              const union sockaddr_union*   addressArray,
                              const size_t                  addresses);

/**
  * Invalidate.
  *
  * @param transportAddressBlock TransportAddressBlock.
  */
void transportAddressBlockDelete(struct TransportAddressBlock* transportAddressBlock);

/**
  * Get textual description of transport address block.
  *
  * @param transportAddressBlock TransportAddressBlock.
  * @param buffer Buffer to write description to.
  * @param bufferSize Size of buffer.
  */
void transportAddressBlockGetDescription(
        const struct TransportAddressBlock* transportAddressBlock,
        char*                               buffer,
        const size_t                        bufferSize);

/**
  * Print transport address block.
  *
  * @param transportAddressBlock TransportAddressBlock.
  * @param fd File to write transport address to (e.g. stdout, stderr, ...).
  */
void transportAddressBlockPrint(const struct TransportAddressBlock* transportAddressBlock,
                                FILE*                               fd);


/**
  * Print transport address block.
  *
  * @param transportAddressBlock TransportAddressBlock.
  * @return Copy of TransportAddressBlock or NULL in case of out of memory.
  */
struct TransportAddressBlock* transportAddressBlockDuplicate(const struct TransportAddressBlock* transportAddressBlock);

/**
  * Compare TransportAddressBlocks.
  *
  * @param transportAddressBlockPtr1 TransportAddressBlock 1.
  * @param transportAddressBlockPtr2 TransportAddressBlock 2.
  * @return Comparison result.
  */
int transportAddressBlockComparison(const void* transportAddressBlockPtr1,
                                    const void* transportAddressBlockPtr2);


/**
  * Initialize TransportAddressBlock with local addresses
  * of given SCTP socket.
  *
  * @param transportAddressBlock TransportAddressBlock.
  * @param sockFD Socket FD.
  * @param maxAddresses Maximum number of addresses to be returned.
  * @return Number of obtained addresses.
  */
size_t transportAddressBlockGetLocalAddressesFromSCTPSocket(
          struct TransportAddressBlock* sctpAddress,
          int                           sockFD,
          const size_t                  maxAddresses);


#ifdef __cplusplus
}
#endif


#endif
