/*
 *  $Id: pool.h,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * Purpose: Pool
 *
 */


#ifndef POOL_H
#define POOL_H


#include "tdtypes.h"
#include "poolelement.h"
#include "poolhandle.h"
#include "poolpolicy.h"

#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



struct Pool
{
   struct PoolNamespace* OwnerNamespace;

   struct PoolHandle*    Handle;
   struct PoolPolicy*    Policy;
   cardinal              PoolElements;
   GList*                PoolElementList;

   /* Round Robin Management */
   struct PoolElement*   NextSelection;
   uint32_t              MultiSelectCounter;

   card64                TimeStamp;
   cardinal              UserCounter;
   void*                 UserData;
};



/**
  * Constructor.
  *
  * @param poolHandle PoolHandle.
  * @param poolPolicy PoolPolicy.
  * @return Pool or NULL in case of error.
  */
struct Pool* poolNew(const struct PoolHandle* poolHandle,
                     const struct PoolPolicy* poolPolicy);

/**
  * Destructor.
  *
  * @param pool Pool.
  */
void poolDelete(struct Pool* pool);

/**
  * Duplicate Pool.
  *
  * @param source Pool to be duplicated.
  * @return Copy of pool or NULL in case of error.
  */
struct Pool* poolDuplicate(const struct Pool* source);

/**
  * Print pool.
  *
  * @param pool Pool.
  * @param fd File to write pool to (e.g. stdout, stderr, ...).
  */
void poolPrint(const struct Pool* pool, FILE* fd);

/**
  * Check if pool is empty.
  *
  * @param pool Pool.
  * @return true if pool is empty; false otherwise.
  */
bool poolIsEmpty(const struct Pool* pool);

/**
  * Add pool element to pool.
  *
  * @param pool Pool.
  * @param poolElement PoolElement.
  */
void poolAddPoolElement(struct Pool* pool, struct PoolElement* poolElement);

/**
  * Remove pool element from pool.
  *
  * @param pool Pool.
  * @param poolElement PoolElement.
  */
void poolRemovePoolElement(struct Pool* pool, struct PoolElement* poolElement);

/**
  * Find pool element in pool.
  *
  * @param pool Pool.
  * @param poolElementIdentifier PoolElementIdentifier.
  * @return PoolElement or NULL if it does not exist in the pool.
  */
struct PoolElement* poolFindPoolElement(
                       struct Pool*                pool,
                       const PoolElementIdentifier poolElementIdentifier);

/**
  * Select one pool element of a pool by policy random.
  *
  * @param pool Pool.
  * @return Selected pool element or NULL if there is no pool element available.
  */
struct PoolElement* poolSelectRandom(struct Pool* pool);

/**
  * Select one pool element of a pool by policy weighted random.
  *
  * @param pool Pool.
  * @return Selected pool element or NULL if there is no pool element available.
  */
struct PoolElement* poolSelectWeightedRandom(struct Pool* pool);

/**
  * Select one pool element of a pool by policy least used.
  *
  * @param pool Pool.
  * @return Selected pool element or NULL if there is no pool element available.
  */
struct PoolElement* poolSelectLeastUsed(struct Pool* pool);

/**
  * Select one pool element of a pool by policy least used with degradation.
  *
  * @param pool Pool.
  * @return Selected pool element or NULL if there is no pool element available.
  */
struct PoolElement* poolSelectLeastUsedDegradation(struct Pool* pool);

/**
  * Select one pool element of a pool by policy round-robin.
  *
  * @param pool Pool.
  * @return Selected pool element or NULL if there is no pool element available.
  */
struct PoolElement* poolSelectRoundRobin(struct Pool* pool);

/**
  * Select one pool element of a pool by policy weighted round-robin.
  *
  * @param pool Pool.
  * @return Selected pool element or NULL if there is no pool element available.
  */
struct PoolElement* poolSelectWeightedRoundRobin(struct Pool* pool);

/**
  * Select one pool element of a pool by pool's policy.
  *
  * @param pool Pool.
  * @return Selected pool element or NULL if there is no pool element available.
  */
struct PoolElement* poolSelectByPolicy(struct Pool* pool);



/**
  * Pool comparision function.
  *
  * @param a Pointer to pool 1.
  * @param b Pointer to pool 2.
  * @return Comparision result.
  */
gint poolCompareFunc(gconstpointer a,
                     gconstpointer b);



#ifdef __cplusplus
}
#endif


#endif
