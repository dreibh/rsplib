/*
 *  $Id: asapcache.c,v 1.1 2004/07/18 15:30:43 dreibh Exp $
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


#include "tdtypes.h"
#include "loglevel.h"
#include "asapcache.h"
#include "utilities.h"
#include "asaperror.h"


/* ###### Print cache's content ########################################## */
static void asapCacheDump(struct ASAPCache* asapCache, FILE* fd)
{
   fputs("*************** Name Cache Dump ***************\n",fd);
   fputs("\n",fd);
   ST_CLASS(poolNamespaceManagementPrint)(asapCache->Namespace,fd);
   fputs("***********************************************\n",fd);
}


/* ###### Check cache for timeouts and remove outdated pool elements ##### */
static void cacheMaintenanceTimer(struct Dispatcher* dispatcher,
                                  struct Timer*      timer,
                                  void*              userData)
{
   struct ASAPCache* asapCache = (struct ASAPCache*)userData;

   LOG_VERBOSE2
   fputs("Cache Maintenance...\n", stdlog);
   LOG_END

   dispatcherLock(asapCache->StateMachine);
   ST_CLASS(poolNamespaceManagementPurgeExpiredPoolElements)(
      &asapCache->Namespace,
      getMicroTime());
   dispatcherUnlock(asapCache->StateMachine);

   LOG_VERBOSE2
   fputs("Cache Maintenance completed. Cache content:\n",stdlog);
   asapCacheDump(asapCache, stdlog);
   LOG_END
}


/* ###### Constructor ##################################################### */
struct ASAPCache* asapCacheNew(struct Dispatcher* dispatcher,
                               const card64       cacheElementTimeout)
{
   struct ASAPCache* asapCache = (struct ASAPCache*)malloc(sizeof(struct ASAPCache));
   if(asapCache != NULL) {
      asapCache->StateMachine        = dispatcher;
      asapCache->CacheElementTimeout = cacheElementTimeout;
      ST_CLASS(poolNamespaceManagementNew)(&asapCache->Namespace,
                                           0x00000000,
                                           NULL, NULL);
      asapCache->CacheMaintenanceTimer = timerNew(asapCache->StateMachine,
                                                  cacheMaintenanceTimer,
                                                  (void*)asapCache);
      if(asapCache->CacheMaintenanceTimer == NULL) {
         asapCacheDelete(asapCache);
         asapCache = NULL;
      }
   }
   return(asapCache);
}


/* ###### Destructor ###################################################### */
void asapCacheDelete(struct ASAPCache* asapCache)
{
   if(asapCache) {
      if(asapCache->CacheMaintenanceTimer) {
         timerDelete(asapCache->CacheMaintenanceTimer);
         asapCache->CacheMaintenanceTimer = NULL;
      }
      ST_CLASS(poolNamespaceManagementClear)(&asapCache->Namespace);
      ST_CLASS(poolNamespaceManagementDelete)(&asapCache->Namespace);
      free(asapCache);
   }
}


/* ###### Update cache entry for pool element ############################ */
struct PoolElement* asapCacheUpdatePoolElement(struct ASAPCache*    asapCache,
                                               struct PoolHandle*   poolHandle,
                                               struct PoolElement*  poolElement,
                                               const bool           increment)
{
   struct PoolElement* copy;
   struct Pool*        pool;

   LOG_VERBOSE2
   fprintf(stdlog,"Cache Update for ");
   poolElementPrint(poolElement,stdlog);
   LOG_END

   copy = poolNamespaceFindPoolElement(asapCache->Namespace,poolHandle,poolElement->Identifier);
   if(copy != NULL) {
      LOG_VERBOSE2
      fputs("Already cached, only timestamp update necessary.\n",stdlog);
      LOG_END

      copy->TimeStamp = getMicroTime();
      copy->Flags &= ~PEF_FAILED;
   }
   else {
      pool = poolNamespaceFindPool(asapCache->Namespace,poolHandle);
      if(pool == NULL) {
         LOG_VERBOSE2
         fputs("New pool, to be created first.\n",stdlog);
         LOG_END

         pool = poolNew(poolHandle,poolElement->Policy);
         if(pool != NULL) {
            poolNamespaceAddPool(asapCache->Namespace,pool);
         }
      }

      if(pool != NULL) {
         LOG_VERBOSE2
         fputs("Adding pool element.\n",stdlog);
         LOG_END

         copy = poolElementDuplicate(poolElement);
         if(copy != NULL) {
            poolAddPoolElement(pool,copy);
         }
      }
   }

   if((increment) && (copy != NULL))  {
      copy->UserCounter++;
   }

   LOG_VERBOSE2
   fputs("\n",stdlog);
   cacheDump(asapCache,stdlog);
   LOG_END
   return(copy);
}


/* ###### Purge pool element ############################################# */
void asapCachePurgePoolElement(struct ASAPCache*   asapCache,
                               struct PoolHandle*  poolHandle,
                               struct PoolElement* poolElement,
                               const bool          decrement)
{
   struct Pool*        pool;
   struct PoolElement* found = poolNamespaceFindPoolElement(
                                  asapCache->Namespace,
                                  poolHandle,
                                  poolElement->Identifier);

   LOG_VERBOSE2
   fprintf(stdlog,"Cache Purge for ");
   poolElementPrint(poolElement,stdlog);
   LOG_END

   if(found != NULL) {
      pool = found->OwnerPool;

      if(decrement) {
         if(found->UserCounter > 0) {
            found->UserCounter--;
         }
         else {
            LOG_WARNING
            fputs("Tried decrement of zero UserCounter!\n",stdlog);
            LOG_END
         }
      }

      if(found->UserCounter == 0) {
         poolRemovePoolElement(pool,found);
         poolElementDelete(found);
         LOG_VERBOSE2
         fputs("UserCounter=0 -> removing pool element.\n",stdlog);
         LOG_END

         if(pool->PoolElements == 0) {
            LOG_VERBOSE2
            fputs("Pool is empty -> removing pool.\n",stdlog);
            LOG_END
            poolNamespaceRemovePool(asapCache->Namespace,pool);
            poolDelete(pool);
         }
      }
      else {
         LOG_VERBOSE2
         fprintf(stdlog,"UserCounter=%d -> pool element is still in use!\n",found->UserCounter);
         LOG_END
      }

      LOG_VERBOSE2
      fputs("\n",stdlog);
      cacheDump(asapCache,stdlog);
      LOG_END
   }
   else {
      LOG_VERBOSE2
      fputs("Pool element not found!\n",stdlog);
      LOG_END
   }
}


/* ###### Find pool element in cache ###################################### */
struct PoolElement* asapCacheFindPoolElement(struct ASAPCache*           asapCache,
                                             const struct PoolHandle*    poolHandle,
                                             const PoolElementIdentifier identifier)
{
  return(poolNamespaceFindPoolElement(asapCache->Namespace, poolHandle, identifier));
}
