/*
 *  $Id: asapcache.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
static void cacheDump(struct ASAPCache* asapCache, FILE* fd)
{
   fputs("*************** Name Cache Dump ***************\n",fd);
   fputs("\n",fd);
   poolNamespacePrint(asapCache->Namespace,fd);
   fputs("***********************************************\n",fd);
}


/* ###### Check for timeout of pool element ############################## */
static void cacheMaintenance(gpointer key, gpointer value, gpointer userData)
{
   card64               now         = getMicroTime();
   struct ASAPCache*    asapCache   = (struct ASAPCache*)userData;
   struct Pool*         pool        = (struct Pool*)value;
   GList*               list        = g_list_first(pool->PoolElementList);
   struct PoolElement*  poolElement;

   while(list != NULL) {
      poolElement = (struct PoolElement*)list->data;
      if(poolElement->TimeStamp + asapCache->CacheElementTimeout <= now) {
         if(poolElement->UserCounter > 0) {
            LOG_VERBOSE2
            fprintf(stdlog,"Cache timeout but UserCounter > 0 for ");
            poolElementPrint(poolElement,stdlog);
            LOG_END
         }
         else {
            asapCache->PurgeList = g_list_append(asapCache->PurgeList,poolElement);
         }
      }
      list = g_list_next(list);
   }
   timerRestart(asapCache->CacheMaintenanceTimer,asapCache->CacheMaintenanceInterval);
}


/* ###### Check cache for timeouts and remove outdated pool elements ##### */
static void cacheMaintenanceTimer(struct Dispatcher* dispatcher,
                                  struct Timer*      timer,
                                  void*              userData)
{
   struct ASAPCache*   asapCache = (struct ASAPCache*)userData;
   struct PoolElement* poolElement;
   GList*              list;

   LOG_VERBOSE2
   fputs("Cache Maintenance...\n",stdlog);
   LOG_END

   dispatcherLock(asapCache->StateMachine);

   g_hash_table_foreach(asapCache->Namespace->Pools,
                        cacheMaintenance,
                        (void*)asapCache);

   if(asapCache->PurgeList) {
      list = g_list_first(asapCache->PurgeList);
      while(list != NULL) {
         poolElement = (struct PoolElement*)list->data;
         asapCachePurgePoolElement(asapCache,poolElement->OwnerPool->Handle,poolElement,false);
         list = g_list_next(list);
      }
      g_list_free(asapCache->PurgeList);
      asapCache->PurgeList = NULL;
   }

   dispatcherUnlock(asapCache->StateMachine);

   LOG_VERBOSE2
   fputs("Cache Maintenance completed!\n",stdlog);
   LOG_END
}


/* ###### Constructor ##################################################### */
struct ASAPCache* asapCacheNew(struct Dispatcher* dispatcher,
                               const card64       cacheMaintenaceInterval,
                               const card64       cacheElementTimeout)
{
   struct ASAPCache* asapCache = (struct ASAPCache*)malloc(sizeof(struct ASAPCache));
   if(asapCache != NULL) {
      asapCache->StateMachine             = dispatcher;
      asapCache->CacheMaintenanceInterval = cacheMaintenaceInterval;
      asapCache->CacheElementTimeout      = cacheElementTimeout;
      asapCache->Namespace                = poolNamespaceNew();
      asapCache->PurgeList                = NULL;
      asapCache->CacheMaintenanceTimer    = timerNew(asapCache->StateMachine,
                                                     cacheMaintenanceTimer,
                                                     (void*)asapCache);
      if((asapCache->Namespace == NULL) ||
         (asapCache->CacheMaintenanceTimer == NULL)) {
         timerStart(asapCache->CacheMaintenanceTimer,
                    asapCache->CacheMaintenanceInterval);
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
      if(asapCache->Namespace) {
         poolNamespaceDelete(asapCache->Namespace);
         asapCache->Namespace = NULL;
      }
      free(asapCache);
   }
}


/* ###### Update cache entries of pool ################################### */
void asapCacheUpdatePool(struct ASAPCache* asapCache,
                         struct Pool*      pool)
{
   GList* poolElementList = g_list_first(pool->PoolElementList);
   while(poolElementList != NULL) {
      asapCacheUpdatePoolElement(asapCache,pool->Handle,(struct PoolElement*)poolElementList->data,false);
      poolElementList = g_list_next(poolElementList);
   }
}


/* ###### Find pool in cache ############################################## */
struct Pool* asapCacheFindPool(struct ASAPCache*        asapCache,
                               const struct PoolHandle* poolHandle)
{
   return(poolNamespaceFindPool(asapCache->Namespace,poolHandle));
}


/* ###### Purge pool ##################################################### */
void asapCachePurgePool(struct ASAPCache* asapCache,
                        struct Pool*      pool)
{
   struct PoolElement* poolElement;
   GList*              poolElementList = g_list_first(pool->PoolElementList);

   while(poolElementList != NULL) {
      poolElement = (struct PoolElement*)poolElementList->data;
      asapCachePurgePoolElement(asapCache,poolElement->OwnerPool->Handle,poolElement,false);
      poolElementList = g_list_next(poolElementList);
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
