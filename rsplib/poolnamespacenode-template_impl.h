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
   ST_METHOD(New)(&poolNamespaceNode->PoolElementOwnershipStorage, ST_CLASS(poolElementOwnershipStorageNodePrint), ST_CLASS(poolElementOwnershipStorageNodeComparison));
   ST_METHOD(New)(&poolNamespaceNode->PoolElementConnectionStorage, ST_CLASS(poolElementConnectionStorageNodePrint), ST_CLASS(poolElementConnectionStorageNodeComparison));
   poolNamespaceNode->HomeNSIdentifier = homeNSIdentifier;
   poolNamespaceNode->PoolElements     = 0;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolNamespaceNodeDelete)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   CHECK(ST_METHOD(IsEmpty)(&poolNamespaceNode->PoolIndexStorage));
   CHECK(ST_METHOD(IsEmpty)(&poolNamespaceNode->PoolElementTimerStorage));
   CHECK(ST_METHOD(IsEmpty)(&poolNamespaceNode->PoolElementOwnershipStorage));
   CHECK(ST_METHOD(IsEmpty)(&poolNamespaceNode->PoolElementConnectionStorage));
   ST_METHOD(Delete)(&poolNamespaceNode->PoolIndexStorage);
   ST_METHOD(Delete)(&poolNamespaceNode->PoolElementTimerStorage);
   ST_METHOD(Delete)(&poolNamespaceNode->PoolElementOwnershipStorage);
   ST_METHOD(Delete)(&poolNamespaceNode->PoolElementConnectionStorage);
   poolNamespaceNode->PoolElements = 0;
}


/* ###### Get number of timers ########################################### */
size_t ST_CLASS(poolNamespaceNodeGetTimerNodes)(
          struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(ST_METHOD(GetElements)(&poolNamespaceNode->PoolElementTimerStorage));
}


/* ###### Get first timer ################################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementTimerNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNamespaceNode->PoolElementTimerStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last timer ################################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementTimerNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNamespaceNode->PoolElementTimerStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next timer ################################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementTimerNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementTimerStorage,
                                                   &poolElementNode->PoolElementTimerStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous timer ############################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementTimerNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementTimerStorage,
                                                   &poolElementNode->PoolElementTimerStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get number of ownership nodes ################################## */
size_t ST_CLASS(poolNamespaceNodeGetOwnershipNodes)(
          struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(ST_METHOD(GetElements)(&poolNamespaceNode->PoolElementOwnershipStorage));
}


/* ###### Get first ownership ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNamespaceNode->PoolElementOwnershipStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last ownership ############################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNamespaceNode->PoolElementOwnershipStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next ownership ############################################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                                   &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous ownership ######################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                                   &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier);


/* ###### Get prev ownership of same home NS identifier ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementOwnershipNodeForSameIdentifier)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                               &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      prevPoolElementNode = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node);
      if(prevPoolElementNode->HomeNSIdentifier == poolElementNode->HomeNSIdentifier) {
         return(prevPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get next ownership of same home NS identifier ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                               &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      nextPoolElementNode = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node);
      if(nextPoolElementNode->HomeNSIdentifier == poolElementNode->HomeNSIdentifier) {
         return(nextPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get number of connection nodes ################################## */
size_t ST_CLASS(poolNamespaceNodeGetConnectionNodes)(
          struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(ST_METHOD(GetElements)(&poolNamespaceNode->PoolElementConnectionStorage));
}


/* ###### Get first connection ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNamespaceNode->PoolElementConnectionStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last connection ############################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNamespaceNode->PoolElementConnectionStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next connection ############################################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementConnectionStorage,
                                                   &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous connection ######################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementConnectionStorage,
                                                   &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get prev connection of same home NS identifier ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementConnectionNodeForSameConnection)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementConnectionStorage,
                                               &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      prevPoolElementNode = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node);
      if((prevPoolElementNode->ConnectionSocketDescriptor == poolElementNode->ConnectionSocketDescriptor) &&
         (prevPoolElementNode->ConnectionAssocID          == poolElementNode->ConnectionAssocID)) {
         return(prevPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get next connection of same home NS identifier ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementConnectionStorage,
                                               &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      nextPoolElementNode = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node);
      if((nextPoolElementNode->ConnectionSocketDescriptor == poolElementNode->ConnectionSocketDescriptor) &&
         (nextPoolElementNode->ConnectionAssocID          == poolElementNode->ConnectionAssocID)) {
         return(nextPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get number of pools ############################################ */
size_t ST_CLASS(poolNamespaceNodeGetPoolElementNodes)(
          const struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(poolNamespaceNode->PoolElements);
}


/* ###### Get number of pools ############################################ */
size_t ST_CLASS(poolNamespaceNodeGetPoolNodes)(
          const struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(ST_METHOD(GetElements)(&poolNamespaceNode->PoolIndexStorage));
}


/* ###### Get first PoolNode ############################################# */
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetFirst)(&poolNamespaceNode->PoolIndexStorage));
}


/* ###### Get last PoolNode ############################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeGetLastPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetLast)(&poolNamespaceNode->PoolIndexStorage));
}


/* ###### Get next PoolNode ############################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeGetNextPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              struct ST_CLASS(PoolNode)*          poolNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetNext)(&poolNamespaceNode->PoolIndexStorage, &poolNode->PoolIndexStorageNode));
}


/* ###### Get previous PoolNode ########################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              struct ST_CLASS(PoolNode)*          poolNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetPrev)(&poolNamespaceNode->PoolIndexStorage, &poolNode->PoolIndexStorageNode));
}


/* ###### Find PoolElementNode ########################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindPoolElementNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolNamespaceNodeFindPoolNode)(
                                            poolNamespaceNode,
                                            poolHandle);
   if(poolNode != NULL) {
      return(ST_CLASS(poolNodeFindPoolElementNode)(poolNode, poolElementIdentifier));
   }
   return(NULL);
}


/* ###### Insert PoolElementNode into timer storage ###################### */
void ST_CLASS(poolNamespaceNodeActivateTimer)(
        struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
        struct ST_CLASS(PoolElementNode)*   poolElementNode,
        const unsigned int                  timerCode,
        const unsigned long long            timerTimeStamp)
{
   struct STN_CLASSNAME* result;

   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
   poolElementNode->TimerCode      = timerCode;
   poolElementNode->TimerTimeStamp = timerTimeStamp;
   result = ST_METHOD(Insert)(&poolNamespaceNode->PoolElementTimerStorage,
                              &poolElementNode->PoolElementTimerStorageNode);
   CHECK(result == &poolElementNode->PoolElementTimerStorageNode);
}


/* ###### Remove PoolElementNode from timer storage ###################### */
void ST_CLASS(poolNamespaceNodeDeactivateTimer)(
        struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
        struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* result;

   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
      result = ST_METHOD(Remove)(&poolNamespaceNode->PoolElementTimerStorage,
                                 &poolElementNode->PoolElementTimerStorageNode);
      CHECK(result == &poolElementNode->PoolElementTimerStorageNode);
   }
}


/* ###### Select PoolElementNodes by Pool Policy ######################### */
size_t ST_CLASS(poolNamespaceNodeSelectPoolElementNodesByPolicy)(
          struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
          const struct PoolHandle*            poolHandle,
          struct ST_CLASS(PoolElementNode)**  poolElementNodeArray,
          const size_t                        maxPoolElementNodes,
          const size_t                        maxIncrement,
          unsigned int*                       errorCode)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolNamespaceNodeFindPoolNode)(poolNamespaceNode, poolHandle);
   size_t                     count    = 0;
   if(poolNode != NULL) {
      *errorCode = RSPERR_OKAY;
      count = poolNode->Policy->SelectionFunction(poolNode, poolElementNodeArray, maxPoolElementNodes, maxIncrement);
#ifdef VERIFY
      ST_CLASS(poolNamespaceNodeVerify)(poolNamespaceNode);
#endif
   }
   else {
      *errorCode = RSPERR_NOT_FOUND;
   }
   return(count);
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
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeFindNearestPrevPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              const struct PoolHandle*            poolHandle)
{
   struct ST_CLASS(PoolNode)* poolNode;
   struct ST_CLASS(PoolNode)  cmpPoolNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   poolNode = (struct ST_CLASS(PoolNode)*)ST_METHOD(GetNearestPrev)(&poolNamespaceNode->PoolIndexStorage,
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


/* ###### Find nearest prev ownership #################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestPrevPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            ownershipNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode    = &cmpPoolNode;
   cmpPoolElementNode.Identifier       = poolElementIdentifier;
   cmpPoolElementNode.HomeNSIdentifier = homeNSIdentifier;

   ownershipNode = ST_METHOD(GetNearestPrev)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                             &cmpPoolElementNode.PoolElementOwnershipStorageNode);
   if(ownershipNode) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(ownershipNode));
   }
   return(NULL);
}


/* ###### Find nearest next ownership #################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            ownershipNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode    = &cmpPoolNode;
   cmpPoolElementNode.Identifier       = poolElementIdentifier;
   cmpPoolElementNode.HomeNSIdentifier = homeNSIdentifier;

   ownershipNode = ST_METHOD(GetNearestNext)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                             &cmpPoolElementNode.PoolElementOwnershipStorageNode);
   if(ownershipNode) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(ownershipNode));
   }
   return(NULL);
}


/* ###### Find nearest prev connection #################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestPrevPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const int                           connectionSocketDescriptor,
                                     const sctp_assoc_t                  assocID,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            connectionNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode               = &cmpPoolNode;
   cmpPoolElementNode.ConnectionSocketDescriptor = connectionSocketDescriptor;
   cmpPoolElementNode.ConnectionAssocID          = assocID;
   cmpPoolElementNode.Identifier                  = poolElementIdentifier;

   connectionNode = ST_METHOD(GetNearestPrev)(&poolNamespaceNode->PoolElementConnectionStorage,
                                              &cmpPoolElementNode.PoolElementConnectionStorageNode);
   if(connectionNode) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(connectionNode));
   }
   return(NULL);
}


/* ###### Find nearest next connection ################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const int                           connectionSocketDescriptor,
                                     const sctp_assoc_t                  assocID,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            connectionNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode               = &cmpPoolNode;
   cmpPoolElementNode.ConnectionSocketDescriptor = connectionSocketDescriptor;
   cmpPoolElementNode.ConnectionAssocID          = assocID;
   cmpPoolElementNode.Identifier                  = poolElementIdentifier;

   connectionNode = ST_METHOD(GetNearestNext)(&poolNamespaceNode->PoolElementConnectionStorage,
                                              &cmpPoolElementNode.PoolElementConnectionStorageNode);
   if(connectionNode) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(connectionNode));
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

      if(poolElementNode->HomeNSIdentifier != 0) {
         result2 = ST_METHOD(Insert)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                     &poolElementNode->PoolElementOwnershipStorageNode);
         CHECK(result2 == &poolElementNode->PoolElementOwnershipStorageNode);
      }
      if(poolElementNode->ConnectionSocketDescriptor > 0) {
         result2 = ST_METHOD(Insert)(&poolNamespaceNode->PoolElementConnectionStorage,
                                     &poolElementNode->PoolElementConnectionStorageNode);
         CHECK(result2 == &poolElementNode->PoolElementConnectionStorageNode);
      }
   }
   return(result);
}


/* ###### Update PoolElementNode's ownership ############################# */
void ST_CLASS(poolNamespaceNodeUpdateOwnershipOfPoolElementNode)(
        struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
        struct ST_CLASS(PoolElementNode)*   poolElementNode,
        const ENRPIdentifierType            newHomeNSIdentifier)
{
   struct STN_CLASSNAME* result;

   if(newHomeNSIdentifier != poolElementNode->HomeNSIdentifier) {
      if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementOwnershipStorageNode)) {
         result = ST_METHOD(Remove)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                    &poolElementNode->PoolElementOwnershipStorageNode);
         CHECK(result == &poolElementNode->PoolElementOwnershipStorageNode);
      }
      poolElementNode->HomeNSIdentifier = newHomeNSIdentifier;
      result = ST_METHOD(Insert)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                 &poolElementNode->PoolElementOwnershipStorageNode);
      CHECK(result == &poolElementNode->PoolElementOwnershipStorageNode);
   }
}


/* ###### Update PoolElementNode's connection ############################ */
void ST_CLASS(poolNamespaceNodeUpdateConnectionOfPoolElementNode)(
        struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
        struct ST_CLASS(PoolElementNode)*   poolElementNode,
        const int                           connectionSocketDescriptor,
        const sctp_assoc_t                  connectionAssocID)
{
   struct STN_CLASSNAME* result;

   if((connectionSocketDescriptor != poolElementNode->ConnectionSocketDescriptor) ||
      (connectionAssocID          != poolElementNode->ConnectionAssocID)) {
      if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementConnectionStorageNode)) {
         result = ST_METHOD(Remove)(&poolNamespaceNode->PoolElementConnectionStorage,
                                    &poolElementNode->PoolElementConnectionStorageNode);
         CHECK(result == &poolElementNode->PoolElementConnectionStorageNode);
      }
      poolElementNode->ConnectionSocketDescriptor = connectionSocketDescriptor;
      poolElementNode->ConnectionAssocID          = connectionAssocID;
      if(poolElementNode->ConnectionSocketDescriptor > 0) {
         result = ST_METHOD(Insert)(&poolNamespaceNode->PoolElementConnectionStorage,
                                       &poolElementNode->PoolElementConnectionStorageNode);
         CHECK(result == &poolElementNode->PoolElementConnectionStorageNode);
      }
   }
}


/* ###### Update PoolElementNode ######################################### */
void ST_CLASS(poolNamespaceNodeUpdatePoolElementNode)(
        struct ST_CLASS(PoolNamespaceNode)*     poolNamespaceNode,
        struct ST_CLASS(PoolElementNode)*       poolElementNode,
        const struct ST_CLASS(PoolElementNode)* source,
        unsigned int*                           errorCode)
{
   ST_CLASS(poolNodeUpdatePoolElementNode)(poolElementNode->OwnerPoolNode, poolElementNode, source, errorCode);
   if(*errorCode == RSPERR_OKAY) {
      /* ====== Change ownership ========================================= */
      ST_CLASS(poolNamespaceNodeUpdateOwnershipOfPoolElementNode)(
         poolNamespaceNode, poolElementNode,
         source->HomeNSIdentifier);

      /* ====== Change connection ======================================== */
      ST_CLASS(poolNamespaceNodeUpdateConnectionOfPoolElementNode)(
         poolNamespaceNode, poolElementNode,
         source->ConnectionSocketDescriptor,
         source->ConnectionAssocID);
   }
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
   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementOwnershipStorageNode)) {
      result = ST_METHOD(Remove)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                 &poolElementNode->PoolElementOwnershipStorageNode);
      CHECK(result == &poolElementNode->PoolElementOwnershipStorageNode);
   }
   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementConnectionStorageNode)) {
      result = ST_METHOD(Remove)(&poolNamespaceNode->PoolElementConnectionStorage,
                                 &poolElementNode->PoolElementConnectionStorageNode);
      CHECK(result == &poolElementNode->PoolElementConnectionStorageNode);
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
            ST_CLASS(poolNamespaceNodeGetOwnershipNodes)(poolNamespaceNode));
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

   if(fields & PNNPO_POOLS_OWNERSHIP) {
      fprintf(fd,
              " *-- Ownership: (%u nodes)\n",
              ST_CLASS(poolNamespaceNodeGetOwnershipNodes)(poolNamespaceNode));
      poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNode)(poolNamespaceNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - $%08x -> \"", poolElementNode->HomeNSIdentifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_ONLY_ID);
         if(poolElementNode->HomeNSIdentifier == poolNamespaceNode->HomeNSIdentifier) {
            fputs(" (property of local namespace)", fd);
         }
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNode)(poolNamespaceNode, poolElementNode);
      }
   }

   if(fields & PNNPO_POOLS_CONNECTION) {
      fprintf(fd,
              " *-- Connection: (%u nodes)\n",
              ST_CLASS(poolNamespaceNodeGetConnectionNodes)(poolNamespaceNode));
      poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementConnectionNode)(poolNamespaceNode);
      while(poolElementNode != NULL) {
         fputs("   - \"", fd);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_CONNECTION);
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNode)(poolNamespaceNode, poolElementNode);
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
   const size_t properties   = ST_CLASS(poolNamespaceNodeGetOwnershipNodes)(poolNamespaceNode);

/*
   puts("------------- VERIFY -------------------");
   ST_CLASS(poolNamespaceNodePrint)(poolNamespaceNode);
   puts("----------------------------------------");
*/

   ST_METHOD(Verify)(&poolNamespaceNode->PoolIndexStorage);
   ST_METHOD(Verify)(&poolNamespaceNode->PoolElementTimerStorage);
   ST_METHOD(Verify)(&poolNamespaceNode->PoolElementOwnershipStorage);

   i = 0;
   poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementTimerNode)(poolNamespaceNode);
   while(poolElementNode != NULL) {
      poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementTimerNode)(poolNamespaceNode, poolElementNode);
      i++;
   }
   CHECK(i == timers);

   i = 0;
   poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNode)(poolNamespaceNode);
   while(poolElementNode != NULL) {
      poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNode)(poolNamespaceNode, poolElementNode);
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


/* ###### Get first ownership node of given home NS identifier ########### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct PoolHandle                 lastPoolHandle;

   poolHandleNew(&lastPoolHandle, (unsigned char*)"\x00", 1);
   poolElementNode = ST_CLASS(poolNamespaceNodeFindNearestNextPoolElementOwnershipNode)(
                        poolNamespaceNode,
                        homeNSIdentifier,
                        &lastPoolHandle,
                        0);
   if(poolElementNode) {
      prevPoolElementNode = ST_CLASS(poolNamespaceNodeGetPrevPoolElementOwnershipNode)(
                               poolNamespaceNode, poolElementNode);
      while(prevPoolElementNode) {
         if(prevPoolElementNode->HomeNSIdentifier == homeNSIdentifier) {
            poolElementNode = prevPoolElementNode;
         }
         else {
            break;
         }
         prevPoolElementNode = ST_CLASS(poolNamespaceNodeGetPrevPoolElementOwnershipNode)(
                                 poolNamespaceNode, prevPoolElementNode);
      }
   }
   if((poolElementNode != NULL) &&
      (poolElementNode->HomeNSIdentifier == homeNSIdentifier)) {
      return(poolElementNode);
   }
   return(NULL);
}


/* ###### Get last ownership node of given home NS identifier ########### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct PoolHandle                 lastPoolHandle;

   poolHandleNew(&lastPoolHandle, (unsigned char*)"\x00", 1);
   if(homeNSIdentifier == 0xffffffff) {
      poolElementNode = ST_CLASS(poolNamespaceNodeGetLastPoolElementOwnershipNode)(poolNamespaceNode);
   }
   else {
      poolElementNode = ST_CLASS(poolNamespaceNodeFindNearestPrevPoolElementOwnershipNode)(
                           poolNamespaceNode,
                           homeNSIdentifier + 1,
                           &lastPoolHandle,
                           0);
   }
   if(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNode)(
                               poolNamespaceNode, poolElementNode);
      while(nextPoolElementNode) {
         if(nextPoolElementNode->HomeNSIdentifier == homeNSIdentifier) {
            poolElementNode = nextPoolElementNode;
         }
         else {
            break;
         }
         nextPoolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNode)(
                                 poolNamespaceNode, nextPoolElementNode);
      }
   }
   if((poolElementNode != NULL) &&
      (poolElementNode->HomeNSIdentifier == homeNSIdentifier)) {
      return(poolElementNode);
   }
   return(NULL);
}


/* ###### Get number of ownership nodes ################################## */
size_t ST_CLASS(poolNamespaceNodeGetOwnershipNodesForIdentifier)(
                 struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                 const ENRPIdentifierType            homeNSIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            nodes = 0;

   poolElementNode = ST_CLASS(poolNamespaceNodeGetLastPoolElementOwnershipNodeForIdentifier)(
                        poolNamespaceNode, homeNSIdentifier);
   while(poolElementNode != NULL) {
      nodes++;
      poolElementNode = ST_CLASS(poolNamespaceNodeGetPrevPoolElementOwnershipNodeForSameIdentifier)(
                           poolNamespaceNode, poolElementNode);
   }
   return(nodes);
}


/* ###### Get first connection node of given connection ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementConnectionNodeForConnection)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const int                           connectionSocketDescriptor,
                                     const sctp_assoc_t                  assocID)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct PoolHandle                 lastPoolHandle;

   poolHandleNew(&lastPoolHandle, (unsigned char*)"\x00", 1);
   poolElementNode = ST_CLASS(poolNamespaceNodeFindNearestNextPoolElementConnectionNode)(
                        poolNamespaceNode,
                        connectionSocketDescriptor, assocID,
                        &lastPoolHandle,
                        0);
   if(poolElementNode) {
      prevPoolElementNode = ST_CLASS(poolNamespaceNodeGetPrevPoolElementConnectionNode)(
                               poolNamespaceNode, poolElementNode);
      while(prevPoolElementNode) {
         if((prevPoolElementNode->ConnectionSocketDescriptor == connectionSocketDescriptor) &&
            (prevPoolElementNode->ConnectionAssocID          == assocID)) {
            poolElementNode = prevPoolElementNode;
         }
         else {
            break;
         }
         prevPoolElementNode = ST_CLASS(poolNamespaceNodeGetPrevPoolElementConnectionNode)(
                                 poolNamespaceNode, prevPoolElementNode);
      }
   }
   if((poolElementNode                              != NULL) &&
      (poolElementNode->ConnectionSocketDescriptor == connectionSocketDescriptor) &&
      (poolElementNode->ConnectionAssocID          == assocID)) {
      return(poolElementNode);
   }
   return(NULL);
}


/* ###### Get last connection node of given connection ################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementConnectionNodeForConnection)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const int                           connectionSocketDescriptor,
                                     const sctp_assoc_t                  assocID)
{
CHECK(0);
/*
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct PoolHandle                 lastPoolHandle;

   poolHandleNew(&lastPoolHandle, (unsigned char*)"\x00", 1);
   if(homeNSIdentifier == 0xffffffff) {  ?????????????????????
      poolElementNode = ST_CLASS(poolNamespaceNodeGetLastPoolElementConnectionNode)(poolNamespaceNode);
   }
   else {
      poolElementNode = ST_CLASS(poolNamespaceNodeFindNearestPrevPoolElementConnectionNode)(
                           poolNamespaceNode,
                           connectionSocketDescriptor, assocID,
                           &lastPoolHandle,
                           0);
   }
   if(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNode)(
                               poolNamespaceNode, poolElementNode);
      while(nextPoolElementNode) {
         if((nextPoolElementNode->SocketDescriptor == connectionSocketDescriptor) &&
            (nextPoolElementNode->AssocID          == assocID)) {
            poolElementNode = nextPoolElementNode;
         }
         else {
            break;
         }
         nextPoolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNode)(
                                 poolNamespaceNode, nextPoolElementNode);
      }
   }
   if((poolElementNode                   != NULL) &&
      (poolElementNode->SocketDescriptor == connectionSocketDescriptor) &&
      (poolElementNode->AssocID          == assocID)) {
      return(poolElementNode);
   }
   return(NULL);
*/
}


/* ###### Get number of connection nodes ################################# */
size_t ST_CLASS(poolNamespaceNodeGetConnectionNodesForConnection)(
                 struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                 const int                           connectionSocketDescriptor,
                 const sctp_assoc_t                  assocID)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            nodes = 0;

   poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementConnectionNodeForConnection)(
                        poolNamespaceNode, connectionSocketDescriptor, assocID);
   while(poolElementNode != NULL) {
      nodes++;
      poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(
                           poolNamespaceNode, poolElementNode);
   }
   return(nodes);
}
