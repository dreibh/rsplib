/*
 *  $Id: poolelement.h,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Pool Element
 *
 */


#ifndef POOLELEMENT_H
#define POOLELEMENT_H


#include "tdtypes.h"
#include "transportaddress.h"
#include "poolpolicy.h"
#include "poolelementidentifier.h"

#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



#define PEF_FAILED (1 << 0)

struct PoolElement
{
   PoolElementIdentifier Identifier;
   struct PoolPolicy*    Policy;
   cardinal              TransportAddresses;
   GList*                TransportAddressList;
   uint32_t              RegistrationLife;
   uint32_t              HomeENRPServerID;
   bool                  HasControlChannel;

   cardinal              UserCounter;
   cardinal              Flags;
   card64                TimeStamp;
   struct Pool*          OwnerPool;
   void*                 UserData;
};



/**
  * Constructor.
  *
  * @param poolElementIdentifier Pool element identifier.
  * @param poolPolicy Pool policy.
  * @return PoolElement or NULL in case of error.
  */
struct PoolElement* poolElementNew(const PoolElementIdentifier poolElementIdentifier,
                                   const struct PoolPolicy*    poolPolicy,
                                   const bool                  hasControlChannel);

/**
  * Destructor.
  *
  * @param poolElement PoolElement.
  */
void poolElementDelete(struct PoolElement* poolElement);

/**
  * Duplicate pool element.
  *
  * @param source Pool element to be duplicated.
  * @return Copy of pool element or NULL in case of error.
  */
struct PoolElement* poolElementDuplicate(const struct PoolElement* source);

/**
  * Print pool element.
  *
  * @param poolElement PoolElement.
  * @param fd File to write pool handle to (e.g. stdout, stderr, ...).
  */
void poolElementPrint(const struct PoolElement* poolElement, FILE* fd);

/**
  * Add transport address to pool element.
  *
  * @param poolElement PoolElement.
  * @param transportAddress TransportAddress.
  */
void poolElementAddTransportAddress(struct PoolElement*      poolElement,
                                    struct TransportAddress* transportAddress);

/**
  * Remove transport address from pool element.
  *
  * @param poolElement PoolElement.
  * @param transportAddress TransportAddress.
  */
void poolElementRemoveTransportAddress(struct PoolElement*      poolElement,
                                       struct TransportAddress* transportAddress);

/**
  * Adapt pool element's policy and transport addresses to given source pool element.
  *
  * @param poolElement PoolElement to be adapted.
  * @param source Source PoolElement.
  * @return Error code or 0 in case of success.
  */
uint16_t poolElementAdapt(struct PoolElement*       poolElement,
                          const struct PoolElement* source);

/**
  * Pool element comparision function.
  *
  * @param a Pointer to pool element 1.
  * @param b Pointer to pool element 2.
  * @return Comparision result.
  */
gint poolElementCompareFunc(gconstpointer a,
                            gconstpointer b);



#ifdef __cplusplus
}
#endif


#endif
