/*
 *  $Id: pool.c,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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


#include "tdtypes.h"
#include "loglevel.h"
#include "pool.h"
#include "utilities.h"
#include "randomizer.h"



/* ###### Constructor #################################################### */
struct Pool* poolNew(const struct PoolHandle* poolHandle,
                     const struct PoolPolicy* poolPolicy)
{
   struct Pool* pool = (struct Pool*)malloc(sizeof(struct Pool));
   if(pool != NULL) {
      pool->PoolElements       = 0;
      pool->PoolElementList    = NULL;
      pool->OwnerNamespace     = NULL;

      pool->NextSelection      = NULL;
      pool->MultiSelectCounter = 0;

      pool->UserCounter        = 0;
      pool->TimeStamp          = getMicroTime();
      pool->UserData           = NULL;

      pool->Handle = poolHandleDuplicate(poolHandle);
      pool->Policy = poolPolicyDuplicate(poolPolicy);

      if((pool->Handle == NULL) || (pool->Policy == NULL)) {
         poolDelete(pool);
         pool = NULL;
      }
   }
   return(pool);
}


/* ###### Destructor ##################################################### */
void poolDelete(struct Pool* pool)
{
   GList*              list;
   struct PoolElement* poolElement;

   if(pool != NULL) {
      if(pool->UserCounter > 0) {
         LOG_WARNING
         fputs("UserCounter > 0!\n",stdlog);
         LOG_END
      }
      if(pool->OwnerNamespace != NULL) {
         LOG_WARNING
         fputs("Pool is still in namespace!\n",stdlog);
         LOG_END
      }

      list = g_list_first(pool->PoolElementList);
      while(list != NULL) {
         poolElement = (struct PoolElement*)list->data;

         poolRemovePoolElement(pool,poolElement);
         poolElementDelete(poolElement);

         list = g_list_first(pool->PoolElementList);
      }

      if(pool->Handle != NULL) {
         free(pool->Handle);
         pool->Handle = NULL;
      }
      if(pool->Policy != NULL) {
         free(pool->Policy);
         pool->Policy = NULL;
      }
      free(pool);
   }
}


/* ###### Duplicate Pool ################################################# */
struct Pool* poolDuplicate(const struct Pool* source)
{
   struct Pool*        copy = NULL;
   GList*              poolElementList;
   struct PoolElement* poolElement;

   if(source != NULL) {
      copy = poolNew(source->Handle,source->Policy);
      if(copy != NULL) {
         copy->TimeStamp = source->TimeStamp;

         poolElementList = g_list_first(source->PoolElementList);
         while(poolElementList != NULL) {
            poolElement = poolElementDuplicate((struct PoolElement*)poolElementList->data);
            if(poolElement == NULL) {
               poolDelete(copy);
               return(NULL);
            }
            poolAddPoolElement(copy,poolElement);
            poolElementList = g_list_next(poolElementList);
         }
      }
   }

   return(copy);
}


/* ###### Print Pool ##################################################### */
void poolPrint(const struct Pool* pool, FILE* fd)
{
   struct PoolElement* poolElement;
   GList*              list;

   if(pool != NULL) {
      fprintf(fd,"Pool ");
      poolHandlePrint(pool->Handle,fd);
      fprintf(fd,":\n");
      poolPolicyPrint(pool->Policy,fd);
      fprintf(fd,"   UserCounter = %d\n",    pool->UserCounter);

      list = g_list_first(pool->PoolElementList);
      while(list != NULL) {
         poolElement = (struct PoolElement*)list->data;
         fprintf(fd,"   ");
         poolElementPrint(poolElement,fd);
         list = g_list_next(list);
      }
   }
   else {
      fprintf(fd,"Pool (null)\n");
   }
}


/* ###### Check if pool is empty ######################################### */
bool poolIsEmpty(const struct Pool* pool)
{
   if(pool != NULL) {
      return(g_list_nth(pool->PoolElementList,0) == NULL);
   }
   return(true);
}


/* ###### Pool comparision function ###################################### */
gint poolCompareFunc(gconstpointer a,
                     gconstpointer b)
{
   const struct Pool* p1 = (struct Pool*)a;
   const struct Pool* p2 = (struct Pool*)b;

   return(poolHandleCompareFunc((gconstpointer)p1->Handle,(gconstpointer)p2->Handle));
}


/* ###### Add pool element to pool ####################################### */
void poolAddPoolElement(struct Pool* pool, struct PoolElement* poolElement)
{
   if((pool != NULL) && (poolElement != NULL)) {
      if(poolElement->OwnerPool == NULL) {
         poolElement->OwnerPool = pool;
         pool->PoolElementList  = g_list_append(pool->PoolElementList,
                                                poolElement);
         pool->PoolElements++;

         if(pool->NextSelection == NULL) {
            pool->NextSelection      = poolElement;
            pool->MultiSelectCounter = 0;
         }
      }
      else {
         LOG_ERROR
         fputs("Element is already in a pool!\n",stdlog);
         LOG_END
      }
   }
}


/* ###### Remove pool element from pool ################################## */
void poolRemovePoolElement(struct Pool* pool, struct PoolElement* poolElement)
{
   GList* list;
   GList* next;

   if((pool != NULL) && (poolElement != NULL)) {
      if(pool == poolElement->OwnerPool) {
         list = g_list_find(pool->PoolElementList,
                            (gpointer)poolElement);
         if(list != NULL) {

            if(pool->NextSelection == poolElement) {
               next = g_list_next(list);
               if(next == NULL) {
                  next = g_list_first(list);
               }
               if(next == list) {
                  pool->NextSelection = NULL;
               }
               else {
                  pool->NextSelection = (struct PoolElement*)next->data;
               }
               pool->MultiSelectCounter = 0;
            }

            pool->PoolElementList = g_list_remove_link(pool->PoolElementList,
                                                       list);
            g_list_free(list);
            pool->PoolElements--;
            poolElement->OwnerPool   = NULL;
         }
      }
      else {
         LOG_ERROR
         fputs("Invalid pool!\n",stdlog);
         LOG_END
      }
   }
}


/* ###### Find pool element ############################################## */
struct PoolElement* poolFindPoolElement(
                       struct Pool*                pool,
                       const PoolElementIdentifier poolElementIdentifier)
{
   struct PoolElement* poolElement;
   GList*              list;

   list = g_list_first(pool->PoolElementList);
   while(list != NULL) {
      poolElement = (struct PoolElement*)list->data;
      if(poolElement->Identifier == poolElementIdentifier) {
         return(poolElement);
      }
      list = g_list_next(list);
   }
   return(NULL);
}


/* ###### Get next possible pool element from list ####################### */
static struct PoolElement* getNextPossiblePoolElement(GList* baseList, GList** list)
{
   GList*              original = *list;
   struct PoolElement* poolElement;

   do {
      poolElement = (struct PoolElement*)(*list)->data;
      if(!(poolElement->Flags & PEF_FAILED)) {
         return(poolElement);
      }

      *list = g_list_next(*list);
      if(*list == NULL) {
         *list = g_list_first(baseList);
      }
   } while(*list != original);

   return(NULL);
}


/* ###### Select pool element ############################################ */
struct PoolElement* poolSelectRandom(struct Pool* pool)
{
   struct PoolElement* selected = NULL;
   GList*              list;
   guint               count;
   guint               number;

   if(pool->PoolElements > 0) {
      count = 0;
      list = g_list_first(pool->PoolElementList);
      while(list != NULL) {
         selected = (struct PoolElement*)list->data;
         if(!(selected->Flags & PEF_FAILED)) {
            count++;
         }
         list = g_list_next(list);
      }

      if(count != 0) {
         number = (guint)random32() % count;
      }
      else {
         number = 0;
      }

      LOG_VERBOSE3
      fprintf(stdlog,"Select by RD policy: %d of %d\n",1 + number,count);
      LOG_END

      list = g_list_first(pool->PoolElementList);
      while(list != NULL) {
         selected = (struct PoolElement*)list->data;
         if(!(selected->Flags & PEF_FAILED)) {
            if(number == 0) {
               return(selected);
            }
            number--;
         }
         list = g_list_next(list);
      }

      selected = NULL;
   }

   return(selected);
}


/* ###### Select pool element ############################################ */
struct PoolElement* poolSelectWeightedRandom(struct Pool* pool)
{
   struct PoolElement* selected = NULL;
   GList*              list;
   card64              number;
   card64              weightSum;

   if(pool->PoolElements > 0) {
      weightSum = 0;
      list = g_list_first(pool->PoolElementList);
      while(list != NULL) {
         selected = (struct PoolElement*)list->data;
         if(!(selected->Flags & PEF_FAILED)) {
            weightSum += (card64)selected->Policy->Weight;
         }
         list = g_list_next(list);
      }

      if(weightSum != 0) {
         number = random64() % weightSum;
      }
      else {
         number = 0;
      }

      LOG_VERBOSE3
      fprintf(stdlog,"Select by WRD policy: %Ld of %Ld\n",number,weightSum);
      LOG_END

      weightSum = 0;
      list = g_list_first(pool->PoolElementList);
      while(list != NULL) {
         selected = (struct PoolElement*)list->data;
         if(!(selected->Flags & PEF_FAILED)) {
            if(weightSum >= number) {
               return(selected);
            }
            weightSum += (card64)selected->Policy->Weight;
         }
         list = g_list_next(list);
      }

      selected = NULL;
   }

   return(selected);
}


/* ###### Select pool element ############################################ */
struct PoolElement* poolSelectLeastUsed(struct Pool* pool)
{
   struct PoolElement* poolElement;
   struct PoolElement* selected = NULL;
   GList*              list;
   uint32_t            load = (uint32_t)-1;

   list = g_list_first(pool->PoolElementList);
   while(list != NULL) {
      poolElement = (struct PoolElement*)list->data;
      if((selected == NULL) || (poolElement->Policy->Load < load)) {
         selected = poolElement;
         load     = poolElement->Policy->Load;
      }
      list = g_list_next(list);
   }
   return(selected);
}


/* ###### Select pool element ############################################ */
struct PoolElement* poolSelectLeastUsedDegradation(struct Pool* pool)
{
   struct PoolElement* poolElement = poolSelectLeastUsed(pool);
   if(poolElement) {
      if(poolElement->Policy->Load < 0x00ffffff) {
         poolElement->Policy->Load++;
      }
   }
   return(poolElement);
}


/* ###### Select pool element ############################################ */
static struct PoolElement* poolSelectRoundRobinImplementation(struct Pool* pool, const bool weighted)
{
   struct PoolElement* poolElement = NULL;
   GList*              list;

   list = g_list_find(pool->PoolElementList,pool->NextSelection);
   if(list) {
      poolElement = getNextPossiblePoolElement(pool->PoolElementList,&list);
      if(poolElement != pool->NextSelection) {
         pool->NextSelection      = poolElement;
         pool->MultiSelectCounter = 0;
      }

      if(poolElement) {
         pool->MultiSelectCounter++;
         if((!weighted) || (pool->MultiSelectCounter >= poolElement->Policy->Weight)) {
            list = g_list_next(list);
            if(list == NULL) {
               list = g_list_first(pool->PoolElementList);
            }
            if(list != NULL) {
               pool->NextSelection = (struct PoolElement*)list->data;
            }
            pool->MultiSelectCounter = 0;
         }
      }
   }

   return(poolElement);
}


/* ###### Select pool element ############################################ */
struct PoolElement* poolSelectRoundRobin(struct Pool* pool)
{
   return(poolSelectRoundRobinImplementation(pool,false));
}


/* ###### Select pool element ############################################ */
struct PoolElement* poolSelectWeightedRoundRobin(struct Pool* pool)
{
   return(poolSelectRoundRobinImplementation(pool,false));
}


/* ###### Select pool element ############################################ */
struct PoolElement* poolSelectByPolicy(struct Pool* pool)
{
   if(pool != NULL) {
      switch(pool->Policy->Type) {
         case PPT_LEASTUSED_DEGRADATION:
            return(poolSelectLeastUsedDegradation(pool));
          break;
         case PPT_LEASTUSED:
            return(poolSelectLeastUsed(pool));
          break;
         case PPT_ROUNDROBIN:
            return(poolSelectRoundRobin(pool));
          break;
         case PPT_WEIGHTED_ROUNDROBIN:
            return(poolSelectWeightedRoundRobin(pool));
          break;
         case PPT_WEIGHTED_RANDOM:
            return(poolSelectWeightedRandom(pool));
          break;
         case PPT_RANDOM:
         default:
            return(poolSelectRandom(pool));
          break;
      }
   }
   return(NULL);
}
