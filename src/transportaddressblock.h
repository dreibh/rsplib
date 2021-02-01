/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2021 by Thomas Dreibholz
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

#ifndef TRANSPORTADDRESSBLOCK_H
#define TRANSPORTADDRESSBLOCK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <stdio.h>
#include <ext_socket.h>

#include "tdtypes.h"
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
                              const size_t                  addresses,
                              const size_t                  maxAddresses);

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
  * Compare TransportAddressBlocks for overlaps. The address blocks are
  * considered as equal when ports and flags are equal AND there is at least
  * one common address. For different address blocks, this function is
  * equal to transportAddressBlockComparison().
  *
  * @param transportAddressBlockPtr1 TransportAddressBlock 1.
  * @param transportAddressBlockPtr2 TransportAddressBlock 2.
  * @return Comparison result.
  *
  * @see transportAddressBlockComparison
  */
int transportAddressBlockOverlapComparison(const void* transportAddressBlockPtr1,
                                           const void* transportAddressBlockPtr2);


/**
  * Initialize TransportAddressBlock with local addresses
  * of given SCTP socket.
  *
  * @param transportAddressBlock TransportAddressBlock.
  * @param sockFD Socket FD.
  * @param assocID Association ID.
  * @param maxAddresses Maximum number of addresses to be returned.
  * @return Number of obtained addresses.
  */
size_t transportAddressBlockGetAddressesFromSCTPSocket(
          struct TransportAddressBlock* sctpAddress,
          int                           sockFD,
          sctp_assoc_t                  assocID,
          const size_t                  maxAddresses,
          const bool                    local);


/*
   This function takes the given addresses from sourceAddressBlock and
   writes valid addresses to validAddressBlock, the amount of
   valid addresses is returned.

   A valid address must fulfil the following conditions:
   - the address must have a sufficient scope,
   - the address must be in assocAddressArray or
     assocAddressArray == NULL
*/
size_t transportAddressBlockFilter(
          const struct TransportAddressBlock* originalAddressBlock,
          const struct TransportAddressBlock* associationAddressBlock,
          struct TransportAddressBlock*       filteredAddressBlock,
          const size_t                        maxAddresses,
          const bool                          filterPort,
          const unsigned int                  minScope);


#ifdef __cplusplus
}
#endif


#endif
