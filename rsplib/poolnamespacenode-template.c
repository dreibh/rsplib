/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

/* ###### Initialize ##################################################### */
void ST_CLASS(poolNamespaceNodeNew)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                    const ENRPIdentifierType            homeNSIdentifier)
{
   ST_METHOD(New)(&poolNamespaceNode->PoolIndexStorage, ST_CLASS(poolIndexStorageNodePrint), ST_CLASS(poolIndexStorageNodeComparison));
   ST_METHOD(New)(&poolNamespaceNode->PoolElementTimerStorage, ST_CLASS(poolElementTimerStorageNodePrint), ST_CLASS(poolElementTimerStorageNodeComparison));
   ST_METHOD(New)(&poolNamespaceNode->PoolElementPropertyStorage, ST_CLASS(poolElementPropertyStorageNodePrint), ST_CLASS(poolElementPropertyStorageNodeComparison));
   poolNamespaceNode->HomeNSIdentifier = homeNSIdentifier;
   poolNamespaceNode->PoolElements     = 0;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolNamespaceNodeDelete)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   CHECK(ST_METHOD(IsEmpty)(&poolNamespaceNode->PoolIndexStorage));
   CHECK(ST_METHOD(IsEmpty)(&poolNamespaceNode->PoolElementTimerStorage));
   CHECK(ST_METHOD(IsEmpty)(&poolNamespaceNode->PoolElementPropertyStorage));
   ST_METHOD(Delete)(&poolNamespaceNode->PoolIndexStorage);
   ST_METHOD(Delete)(&poolNamespaceNode->PoolElementTimerStorage);
   ST_METHOD(Delete)(&poolNamespaceNode->PoolElementPropertyStorage);
   poolNamespaceNode->PoolElements = 0;
}


/* ###### Add PoolNode ################################################### */
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeAddPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              struct ST_CLASS(PoolNode)*          poolNode)
{
   struct STN_CLASSNAME* result = ST_METHOD(Insert)(&poolNamespaceNode->PoolIndexStorage,
                                                    &poolNode->PoolIndexStorageNode);
   if(result == &poolNode->PoolIndexStorageNode) {
      poolNode->OwnerPoolNamespaceNode = poolNamespaceNode;
   }
   return((struct ST_CLASS(PoolNode)*)result);
}


/* ###### Get number of pool elements of certain pool #################### */
size_t ST_CLASS(poolNamespaceNodeGetPoolElementNodesOfPool)(
          struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
          const struct PoolHandle*            poolHandle)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolNamespaceNodeFindPoolNode)(
                                            poolNamespaceNode,
                                            poolHandle);
   if(poolNode) {
      return(ST_CLASS(poolNodeGetPoolElementNodes)(poolNode));
   }
   return(0);
}


/* ###### Find PoolNode ################################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeFindPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              const struct PoolHandle*            poolHandle)
{
   struct ST_CLASS(PoolNode)* poolNode;
   struct ST_CLASS(PoolNode)  cmpPoolNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   poolNode = (struct ST_CLASS(PoolNode)*)ST_METHOD(Find)(&poolNamespaceNode->PoolIndexStorage,
                                                          &cmpPoolNode.PoolIndexStorageNode);
   return(poolNode);
}


/* ###### Find PoolNode ################################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeFindNearestNextPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              const struct PoolHandle*            poolHandle)
{
   struct ST_CLASS(PoolNode)* poolNode;
   struct ST_CLASS(PoolNode)  cmpPoolNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   poolNode = (struct ST_CLASS(PoolNode)*)ST_METHOD(GetNearestNext)(&poolNamespaceNode->PoolIndexStorage,
                                                                    &cmpPoolNode.PoolIndexStorageNode);
   return(poolNode);
}


/* ###### Find nearest next property ##################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestNextPoolElementPropertyNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            propertyNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode = &cmpPoolNode;
   cmpPoolElementNode.Identifier    = poolElementIdentifier;

   propertyNode = ST_METHOD(GetNearestNext)(&poolNamespaceNode->PoolElementPropertyStorage,
                                            &cmpPoolElementNode.PoolElementPropertyStorageNode);
   if(propertyNode) {
      return(ST_CLASS(getPoolElementNodeFromPropertyStorageNode)(propertyNode));
   }
   return(NULL);
}


/* ###### Remove PoolNode ################################################ */
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeRemovePoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              struct ST_CLASS(PoolNode)*          poolNode)
{
   const struct STN_CLASSNAME* result = ST_METHOD(Remove)(&poolNamespaceNode->PoolIndexStorage,
                                                          &poolNode->PoolIndexStorageNode);
   CHECK(result == &poolNode->PoolIndexStorageNode);
   poolNode->OwnerPoolNamespaceNode = NULL;
   return(poolNode);
}


/* ###### Add PoolElementNode ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeAddPoolElementNode)(
                                    struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                    struct ST_CLASS(PoolNode)*          poolNode,
                                    struct ST_CLASS(PoolElementNode)*   poolElementNode,
                                    unsigned int*                       errorCode)
{
   struct ST_CLASS(PoolElementNode)* result = ST_CLASS(poolNodeAddPoolElementNode)(poolNode, poolElementNode, errorCode);
   struct STN_CLASSNAME*             result2;
   if(result == poolElementNode) {
      CHECK(*errorCode == RSPERR_OKAY);
      poolNamespaceNode->PoolElements++;
      if(poolNamespaceNode->HomeNSIdentifier == poolElementNode->HomeNSIdentifier) {
         result2 = ST_METHOD(Insert)(&poolNamespaceNode->PoolElementPropertyStorage,
                                     &poolElementNode->PoolElementPropertyStorageNode);
         CHECK(result2 == &poolElementNode->PoolElementPropertyStorageNode);
      }
   }
   return(result);
}


/* ###### Update PoolElementNode ######################################### */
void ST_CLASS(poolNamespaceNodeUpdatePoolElementNode)(
        struct ST_CLASS(PoolNamespaceNode)*     poolNamespaceNode,
        struct ST_CLASS(PoolElementNode)*       poolElementNode,
        const struct ST_CLASS(PoolElementNode)* source,
        unsigned int*                           errorCode)
{
   ST_CLASS(poolNodeUpdatePoolElementNode)(poolElementNode->OwnerPoolNode, poolElementNode, source, errorCode);
}


/* ###### Add or Update PoolElementNode ##################################### */
/*
   Allocation behavior:
   User program places PoolNode and PoolElementNode data in new memory areas.
   If the PoolNode or PoolElementNode structure is used for insertion into
   the namespace, their pointer is set to NULL. So, the user program has to
   allocate new spaces for the next element. Otherwise, data has been copied
   into already existing nodes and the memory areas can be reused.
*/
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeAddOrUpdatePoolElementNode)(
                                    struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                    struct ST_CLASS(PoolNode)**         poolNode,
                                    struct ST_CLASS(PoolElementNode)**  poolElementNode,
                                    unsigned int*                       errorCode)
{
   struct ST_CLASS(PoolNode)*        newPoolNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;

   newPoolNode = ST_CLASS(poolNamespaceNodeAddPoolNode)(poolNamespaceNode, *poolNode);
   newPoolElementNode = ST_CLASS(poolNamespaceNodeAddPoolElementNode)(poolNamespaceNode, newPoolNode, *poolElementNode, errorCode);

   if(newPoolElementNode != NULL) {
      if(newPoolElementNode != *poolElementNode) {
         ST_CLASS(poolNamespaceNodeUpdatePoolElementNode)(poolNamespaceNode, newPoolElementNode, *poolElementNode, errorCode);
      }
      else {
         *poolElementNode = NULL;
      }
   }
   if(newPoolNode == *poolNode) {
      *poolNode = NULL;
   }

   return(newPoolElementNode);
}


/* ###### Remove PoolElementNode ######################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeRemovePoolElementNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME*             result;
   struct ST_CLASS(PoolElementNode)* result2;

   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
      result = ST_METHOD(Remove)(&poolNamespaceNode->PoolElementTimerStorage,
                                 &poolElementNode->PoolElementTimerStorageNode);
      CHECK(result == &poolElementNode->PoolElementTimerStorageNode);
   }
   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementPropertyStorageNode)) {
      result = ST_METHOD(Remove)(&poolNamespaceNode->PoolElementPropertyStorage,
                                 &poolElementNode->PoolElementPropertyStorageNode);
      CHECK(result == &poolElementNode->PoolElementPropertyStorageNode);
   }
   result2 = ST_CLASS(poolNodeRemovePoolElementNode)(poolElementNode->OwnerPoolNode,
                                                     poolElementNode);
   CHECK(result2 == poolElementNode);
   CHECK(poolNamespaceNode->PoolElements > 0);
   poolNamespaceNode->PoolElements--;

   return(poolElementNode);
}


/* ###### Get textual description ######################################## */
void ST_CLASS(poolNamespaceNodeGetDescription)(
        struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
        char*                               buffer,
        const size_t                        bufferSize)
{
   snprintf(buffer, bufferSize,
            "Namespace[home=$%08x]: (%u Pools, %u PoolElements, %u owned)",
            poolNamespaceNode->HomeNSIdentifier,
            ST_CLASS(poolNamespaceNodeGetPoolNodes)(poolNamespaceNode),
            ST_CLASS(poolNamespaceNodeGetPoolElementNodes)(poolNamespaceNode),
            ST_CLASS(poolNamespaceNodeGetPropertyNodes)(poolNamespaceNode));
}


/* ###### Print ########################################################## */
void ST_CLASS(poolNamespaceNodePrint)(
        struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
        FILE*                               fd,
        const unsigned int                  fields)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolNode)*        poolNode;
   char                              poolNamespaceNodeDescription[256];

   ST_CLASS(poolNamespaceNodeGetDescription)(poolNamespaceNode,
                                             (char*)&poolNamespaceNodeDescription,
                                             sizeof(poolNamespaceNodeDescription));
   fputs(poolNamespaceNodeDescription, fd);
   fputs("\n", fd);

   if(fields & PNNPO_POOLS_INDEX) {
      fputs("*-- Index:\n", fd);
      poolNode = ST_CLASS(poolNamespaceNodeGetFirstPoolNode)(poolNamespaceNode);
      while(poolNode != NULL) {
         fprintf(fd, " +-- ");
         ST_CLASS(poolNodePrint)(poolNode, fd, (fields | PNPO_INDEX) & (~PNPO_SELECTION));
         poolNode = ST_CLASS(poolNamespaceNodeGetNextPoolNode)(poolNamespaceNode, poolNode);
      }
   }

   if(fields & PNNPO_POOLS_SELECTION) {
      fputs("*-- Selection:\n", fd);
      poolNode = ST_CLASS(poolNamespaceNodeGetFirstPoolNode)(poolNamespaceNode);
      while(poolNode != NULL) {
         fprintf(fd, " +-- ");
         ST_CLASS(poolNodePrint)(poolNode, fd, (fields | PNPO_SELECTION) & (~PNPO_INDEX));
         poolNode = ST_CLASS(poolNamespaceNodeGetNextPoolNode)(poolNamespaceNode, poolNode);
      }
   }

   if(fields & PNNPO_POOLS_PROPERTY) {
      fprintf(fd,
            " *-- Property: (%u nodes)\n",
            ST_CLASS(poolNamespaceNodeGetPropertyNodes)(poolNamespaceNode));
      poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementPropertyNode)(poolNamespaceNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - \"");
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_ONLY_ID);
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementPropertyNode)(poolNamespaceNode, poolElementNode);
      }
   }

   if(fields & PNNPO_POOLS_TIMER) {
      fprintf(fd,
            "*-- Timer: (%u nodes)\n",
            ST_CLASS(poolNamespaceNodeGetTimerNodes)(poolNamespaceNode));
      poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementTimerNode)(poolNamespaceNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - \"");
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_ONLY_ID);
         fprintf(fd, " code=%u ts=%Lu\n", poolElementNode->TimerCode, poolElementNode->TimerTimeStamp);
         poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementTimerNode)(poolNamespaceNode, poolElementNode);
      }
   }
}


/* ###### Verify ######################################################### */
void ST_CLASS(poolNamespaceNodeVerify)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct ST_CLASS(PoolNode)*        poolNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            i, j;

   const size_t pools        = ST_CLASS(poolNamespaceNodeGetPoolNodes)(poolNamespaceNode);
   const size_t poolElements = ST_CLASS(poolNamespaceNodeGetPoolElementNodes)(poolNamespaceNode);
   const size_t timers       = ST_CLASS(poolNamespaceNodeGetTimerNodes)(poolNamespaceNode);
   const size_t properties   = ST_CLASS(poolNamespaceNodeGetPropertyNodes)(poolNamespaceNode);

/*
   puts("------------- VERIFY -------------------");
   ST_CLASS(poolNamespaceNodePrint)(poolNamespaceNode);
   puts("----------------------------------------");
*/

   ST_METHOD(Verify)(&poolNamespaceNode->PoolIndexStorage);
   ST_METHOD(Verify)(&poolNamespaceNode->PoolElementTimerStorage);
   ST_METHOD(Verify)(&poolNamespaceNode->PoolElementPropertyStorage);

   i = 0;
   poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementTimerNode)(poolNamespaceNode);
   while(poolElementNode != NULL) {
      poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementTimerNode)(poolNamespaceNode, poolElementNode);
      i++;
   }
   CHECK(i == timers);

   i = 0;
   poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementPropertyNode)(poolNamespaceNode);
   while(poolElementNode != NULL) {
      poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementPropertyNode)(poolNamespaceNode, poolElementNode);
      i++;
   }
   CHECK(i == properties);

   i = 0; j = 0;
   poolNode = ST_CLASS(poolNamespaceNodeGetFirstPoolNode)(poolNamespaceNode);
   while(poolNode != NULL) {
      ST_METHOD(Verify)(&poolNode->PoolElementIndexStorage);
      ST_METHOD(Verify)(&poolNode->PoolElementSelectionStorage);
      CHECK(ST_METHOD(GetElements)(&poolNode->PoolElementSelectionStorage)
               == ST_METHOD(GetElements)(&poolNode->PoolElementIndexStorage));
      CHECK(ST_CLASS(poolNodeGetPoolElementNodes)(poolNode) > 0);
      j += ST_CLASS(poolNodeGetPoolElementNodes)(poolNode);
      poolNode = ST_CLASS(poolNamespaceNodeGetNextPoolNode)(poolNamespaceNode, poolNode);
      i++;
   }
   CHECK(i == pools);
   CHECK(j == poolElements);
   CHECK(properties <= poolElements);
}


/* ###### Clear ########################################################## */
void ST_CLASS(poolNamespaceNodeClear)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                      void                                (*poolNodeDisposer)(void* poolNode, void* userData),
                                      void                                (*poolElementNodeDisposer)(void* poolElementNode, void* userData),
                                      void*                               userData)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolNamespaceNodeGetFirstPoolNode)(poolNamespaceNode);
   while(poolNode != NULL) {
      ST_CLASS(poolNodeClear)(poolNode, poolElementNodeDisposer, userData);
      ST_CLASS(poolNamespaceNodeRemovePoolNode)(poolNamespaceNode, poolNode);
      ST_CLASS(poolNodeDelete)(poolNode);
      poolNodeDisposer(poolNode, userData);
      poolNode = ST_CLASS(poolNamespaceNodeGetFirstPoolNode)(poolNamespaceNode);
   }
}
