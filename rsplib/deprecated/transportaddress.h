/*
 *  $Id: transportaddress.h,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * Purpose: Transport Address
 *
 */


#ifndef TRANSPORTADDRESS_H
#define TRANSPORTADDRESS_H


#include "tdtypes.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



struct TransportAddress
{
   size_t                Addresses;
   union sockaddr_union* AddressArray;
   int                   Protocol;
   uint16_t              Port;
};



/**
  * Constructor.
  *
  * @param protocol Protocol.
  * @param port Port.
  * @param addressArray Address array.
  * @param addresses Number of addresses.
  * @return TransportAddress or NULL in case of error.
  */
struct TransportAddress* transportAddressNew(const int                      protocol,
                                             const uint16_t                 port,
                                             const struct sockaddr_storage* addressArray,
                                             const size_t                   addresses);

/**
  * Destructor.
  *
  * @param transportAddress TransportAddress.
  */
void transportAddressDelete(struct TransportAddress* transportAddress);

/**
  * Duplicate transport address.
  *
  * @param source TransportAddress to be duplicated.
  * @return Copy of transport address or NULL in case of error.
  */
struct TransportAddress* transportAddressDuplicate(const struct TransportAddress* source);

/**
  * Print transport address.
  *
  * @param transportAddress TransportAddress.
  * @param fd File to write transport address to (e.g. stdout, stderr, ...).
  */
void transportAddressPrint(const struct TransportAddress* transportAddress, FILE* fd);

/**
  * Drop all addresses having a scope less than the given scope.
  *
  * @param transportAddress TransportAddress.
  * @param minScope Minimum scope to keep address included.
  */
void transportAddressScopeFilter(struct TransportAddress* transportAddress,
                                 const unsigned int       minScope);

/**
  * Transport address comparision function.
  *
  * @param a Pointer to transport address 1.
  * @param b Pointer to transport address 2.
  * @return Comparision result.
  */
gint transportAddressCompareFunc(gconstpointer a, gconstpointer b);



/**
  * Duplicate list of transport addresss.
  *
  * @param source TransportAddress list to be duplicated.
  * @return Copy of transport address list or NULL in case of error.
  */
GList* transportAddressListDuplicate(GList* source);

/**
  * Delete list of transport addresss.
  *
  * @param transportAddressList List of transport addresses.
  */
void transportAddressListDelete(GList* transportAddressList);

/**
  * Transport address list comparision function.
  *
  * @param a Pointer to transport address list 1.
  * @param b Pointer to transport address list 2.
  * @return Comparision result.
  */
gint transportAddressListCompareFunc(gconstpointer a, gconstpointer b);



#ifdef __cplusplus
}
#endif


#endif
