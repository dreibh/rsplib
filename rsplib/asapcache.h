/*
 *  $Id: asapcache.h,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: ASAP Cache
 *
 */


#ifndef ASAPCACHE_H
#define ASAPCACHE_H


#include "tdtypes.h"
#include "dispatcher.h"
#include "timer.h"
#include "poolhandle.h"
#include "poolnamespace.h"



#ifdef __cplusplus
extern "C" {
#endif



struct ASAPCache
{
   struct Dispatcher*    StateMachine;
   struct PoolNamespace* Namespace;
   struct Timer*         CacheMaintenanceTimer;

   card64                CacheMaintenanceInterval;
   card64                CacheElementTimeout;

   GList*                PurgeList;
};



/**
  * Constructor.
  *
  * @param dispatcher Dispatcher.
  * @param cacheMaintenaceInterval Cache maintenance interval.
  * @param cacheElementTimeout Timeout for element in cache.
  * @return ASAPCache or NULL in case of error.
  */
struct ASAPCache* asapCacheNew(struct Dispatcher* dispatcher,
                               const card64       cacheMaintenaceInterval,
                               const card64       cacheElementTimeout);

/**
  * Destructor.
  *
  * @param asapCache ASAPCache.
  */
void asapCacheDelete(struct ASAPCache* asapCache);

/**
  * Update pool in cache.
  *
  * @param asapCache ASAPCache.
  * @param pool Pool.
  */
void asapCacheUpdatePool(struct ASAPCache* asapCache,
                         struct Pool*      pool);

/**
  * Purge pool in cache.
  *
  * @param asapCache ASAPCache.
  * @param pool Pool.
  */
void asapCachePurgePool(struct ASAPCache* asapCache,
                        struct Pool*      pool);

/**
  * Find pool in cache by pool handle.
  *
  * @param asapCache ASAPCache.
  * @param poolHandle Pool handle.
  * @return Pool element or NULL, if not found.
  */
struct Pool* asapCacheFindPool(struct ASAPCache*        asapCache,
                               const struct PoolHandle* poolHandle);

/**
  * Update pool element in cache.
  *
  * @param asapCache ASAPCache.
  * @param poolHandle Pool handle.
  * @param poolElement Pool element.
  * @param increment true to lock the element in cache; false otherwise.
  */
struct PoolElement* asapCacheUpdatePoolElement(struct ASAPCache*   asap,
                                               struct PoolHandle*  poolHandle,
                                               struct PoolElement* poolElement,
                                               const bool          increment);

/**
  * Purge pool element in cache.
  *
  * @param asapCache ASAPCache.
  * @param poolHandle Pool handle.
  * @param poolElement Pool element.
  * @param decrement true to unlock the element in cache; false otherwise.
  */
void asapCachePurgePoolElement(struct ASAPCache*   asapCache,
                               struct PoolHandle*  poolHandle,
                               struct PoolElement* poolElement,
                               const bool          decrement);

/**
  * Find pool element in cache by pool element identifier.
  *
  * @param asapCache ASAPCache.
  * @param poolHandle Pool handle.
  * @param identifier Pool element identifier.
  * @return Pool element or NULL, if not found.
  */
struct PoolElement* asapCacheFindPoolElement(struct ASAPCache*           asapCache,
                                             const struct PoolHandle*    poolHandle,
                                             const PoolElementIdentifier identifier);


#ifdef __cplusplus
}
#endif


#endif
