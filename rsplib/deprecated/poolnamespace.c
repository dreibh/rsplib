/*
 *  $Id: poolnamespace.c,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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


#include "tdtypes.h"
#include "loglevel.h"
#include "poolnamespace.h"
#include "utilities.h"



/* ###### Constructor #################################################### */
struct PoolNamespace* poolNamespaceNew()
{
   struct PoolNamespace* poolNamespace = (struct PoolNamespace*)malloc(sizeof(struct PoolNamespace));
   if(poolNamespace != NULL) {
      poolNamespace->Deleting                 = false;
      poolNamespace->Pools                    = g_hash_table_new(poolHandleHashFunc,poolHandleEqualFunc);
      if(poolNamespace->Pools == NULL) {
         poolNamespaceDelete(poolNamespace);
         poolNamespace = NULL;
      }
   }
   return(poolNamespace);
}


/* ###### Delete all pools in the namespace ############################## */
static gboolean poolNamespaceDeleteAllPools(gpointer key, gpointer value, gpointer userData)
{
   struct PoolNamespace* poolNamespace = (struct PoolNamespace*)userData;
   struct Pool*          pool          = (struct Pool*)value;

   poolNamespaceRemovePool(poolNamespace,pool);
   poolDelete(pool);

   return(true);
}


/* ###### Destructor ##################################################### */
void poolNamespaceDelete(struct PoolNamespace* poolNamespace)
{
   if(poolNamespace != NULL) {
      poolNamespace->Deleting = true;
      if(poolNamespace->Pools) {
         g_hash_table_foreach_remove(poolNamespace->Pools,
                                     poolNamespaceDeleteAllPools,
                                     (gpointer)poolNamespace);
         g_hash_table_destroy(poolNamespace->Pools);
         poolNamespace->Pools = NULL;
      }
      free(poolNamespace);
   }
}


/* ###### Print all pool elements in the namespace ####################### */
static void poolNamespacePrintAllPools(gpointer key, gpointer value, gpointer userData)
{
   poolPrint((struct Pool*)value,(FILE*)userData);
}


/* ###### Print pool namespace ########################################### */
void poolNamespacePrint(const struct PoolNamespace* poolNamespace, FILE* fd)
{
   if(poolNamespace != NULL) {
      g_hash_table_foreach(poolNamespace->Pools,poolNamespacePrintAllPools,(gpointer)fd);
   }
   else {
      fputs("(null)",fd);
   }
}


/* ###### Add pool to pool namespace ##################################### */
void poolNamespaceAddPool(struct PoolNamespace* poolNamespace, struct Pool* pool)
{
   if((poolNamespace != NULL) && (pool != NULL)) {
      if(pool->OwnerNamespace == NULL) {
         LOG_VERBOSE2
         fprintf(stdlog,"Adding new pool ");
         poolHandlePrint(pool->Handle,stdlog);
         fputs(".\n",stdlog);
         LOG_END

         pool->OwnerNamespace = poolNamespace;
         g_hash_table_insert(poolNamespace->Pools,
                             (gpointer)pool->Handle,
                             (gpointer)pool);
      }
      else {
         LOG_ERROR
         fputs("Element is already in a pool!\n",stdlog);
         LOG_END
      }
   }
}


/* ###### Remove pool from pool namespace ################################ */
void poolNamespaceRemovePool(struct PoolNamespace* poolNamespace, struct Pool* pool)
{
   if((poolNamespace != NULL) && (pool != NULL)) {
      if(poolNamespace == pool->OwnerNamespace) {
         LOG_VERBOSE2
         fprintf(stdlog,"Removing pool ");
         poolHandlePrint(pool->Handle,stdlog);
         fputs(".\n",stdlog);
         LOG_END

         if(!poolNamespace->Deleting) {
            g_hash_table_remove(pool->OwnerNamespace->Pools,
                                (gpointer)pool->Handle);
         }
         pool->OwnerNamespace = NULL;
      }
      else {
         LOG_ERROR
         fputs("Invalid namespace!\n",stdlog);
         LOG_END
      }
   }
}


/* ###### Find pool in pool namespace #################################### */
struct Pool* poolNamespaceFindPool(struct PoolNamespace*    poolNamespace,
                                   const struct PoolHandle* poolHandle)
{
   if(poolNamespace != NULL) {
      return((struct Pool*)g_hash_table_lookup(poolNamespace->Pools,
                                               (gconstpointer)poolHandle));
   }
   return(NULL);
}


/* ###### Find pool element in pool namespace ############################ */
struct PoolElement* poolNamespaceFindPoolElement(
                       struct PoolNamespace*       poolNamespace,
                       const struct PoolHandle*    poolHandle,
                       const PoolElementIdentifier poolElementIdentifier)
{
   struct Pool*        pool;
   struct PoolElement* poolElement;

   pool = poolNamespaceFindPool(poolNamespace,poolHandle);
   if(pool != NULL) {
      poolElement = poolFindPoolElement(pool,poolElementIdentifier);
      return(poolElement);
   }
   return(NULL);
}


/* ###### Select pool element by policy ################################## */
struct PoolElement* poolNamespaceSelectPoolElement(struct PoolNamespace*    poolNamespace,
                                                   const struct PoolHandle* poolHandle)
{
   struct Pool* pool = poolNamespaceFindPool(poolNamespace,poolHandle);
   if(pool != NULL) {
      return(poolSelectByPolicy(pool));
   }
   return(NULL);
}
