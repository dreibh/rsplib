/*
 *  $Id: poolnamespace.h,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * Purpose: Pool Namespace
 *
 */


#ifndef POOLNAMESPACE_H
#define POOLNAMESPACE_H


#include "tdtypes.h"
#include "pool.h"
#include "poolhandle.h"

#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



struct PoolNamespace
{
   GHashTable* Pools;
   bool        Deleting;
};



/**
  * Constructor.
  *
  * @return PoolNamespace or NULL in case of error.
  */
struct PoolNamespace* poolNamespaceNew();

/**
  * Destructor.
  *
  * @param poolNamespace PoolNamespace.
  */
void poolNamespaceDelete(struct PoolNamespace* poolNamespace);

/**
  * Print PoolNamespace.
  *
  * @param poolNamespace PoolNamespace.
  * @param fd File to write pool namespace to (e.g. stdout, stderr, ...).
  */
void poolNamespacePrint(const struct PoolNamespace* poolNamespace, FILE* fd);

/**
  * Add pool to pool namespace.
  *
  * @param poolNamespace PoolNamespace.
  * @param pool Pool.
  */
void poolNamespaceAddPool(struct PoolNamespace* poolNamespace,
                          struct Pool*          pool);

/**
  * Remove pool from pool namespace.
  *
  * @param poolNamespace PoolNamespace.
  * @param pool Pool.
  */
void poolNamespaceRemovePool(struct PoolNamespace* poolNamespace,
                             struct Pool*          pool);

/**
  * Find pool in pool namespace by pool handle.
  *
  * @param poolNamespace PoolNamespace.
  * @param poolHandle PoolHandle.
  * @return Pool or NULL if it does not exist in the pool namespace.
  */
struct Pool* poolNamespaceFindPool(struct PoolNamespace*    poolNamespace,
                                   const struct PoolHandle* poolHandle);

/**
  * Find pool element in pool namespace.
  *
  * @param poolNamespace PoolNamespace.
  * @param poolHandle PoolHandle.
  * @param poolElementIdentifier PoolElementIdentifier.
  * @return PoolElement or NULL if it does not exist in the pool namespace.
  */
struct PoolElement* poolNamespaceFindPoolElement(
                       struct PoolNamespace*       poolNamespace,
                       const struct PoolHandle*    poolHandle,
                       const PoolElementIdentifier poolElementIdentifier);

/**
  * Select pool element by policy from pool given by pool handle.
  *
  * @param poolNamespace PoolNamespace.
  * @param poolHandle PoolHandle.
  * @return PoolElement or NULL if the pool does not exist or no pool element is available.
  */
struct PoolElement* poolNamespaceSelectPoolElement(struct PoolNamespace*    poolNamespace,
                                                   const struct PoolHandle* poolHandle);



#ifdef __cplusplus
}
#endif


#endif
