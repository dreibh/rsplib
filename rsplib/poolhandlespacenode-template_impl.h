/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
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
void ST_CLASS(poolHandlespaceNodeNew)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                      const RegistrarIdentifierType            homeRegistrarIdentifier)
{
   ST_METHOD(New)(&poolHandlespaceNode->PoolIndexStorage, ST_CLASS(poolIndexStorageNodePrint), ST_CLASS(poolIndexStorageNodeComparison));
   ST_METHOD(New)(&poolHandlespaceNode->PoolElementTimerStorage, ST_CLASS(poolElementTimerStorageNodePrint), ST_CLASS(poolElementTimerStorageNodeComparison));
   ST_METHOD(New)(&poolHandlespaceNode->PoolElementOwnershipStorage, ST_CLASS(poolElementOwnershipStorageNodePrint), ST_CLASS(poolElementOwnershipStorageNodeComparison));
   ST_METHOD(New)(&poolHandlespaceNode->PoolElementConnectionStorage, ST_CLASS(poolElementConnectionStorageNodePrint), ST_CLASS(poolElementConnectionStorageNodeComparison));
   poolHandlespaceNode->HomeRegistrarIdentifier = homeRegistrarIdentifier;
   poolHandlespaceNode->PoolElements     = 0;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolHandlespaceNodeDelete)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   CHECK(ST_METHOD(IsEmpty)(&poolHandlespaceNode->PoolIndexStorage));
   CHECK(ST_METHOD(IsEmpty)(&poolHandlespaceNode->PoolElementTimerStorage));
   CHECK(ST_METHOD(IsEmpty)(&poolHandlespaceNode->PoolElementOwnershipStorage));
   CHECK(ST_METHOD(IsEmpty)(&poolHandlespaceNode->PoolElementConnectionStorage));
   ST_METHOD(Delete)(&poolHandlespaceNode->PoolIndexStorage);
   ST_METHOD(Delete)(&poolHandlespaceNode->PoolElementTimerStorage);
   ST_METHOD(Delete)(&poolHandlespaceNode->PoolElementOwnershipStorage);
   ST_METHOD(Delete)(&poolHandlespaceNode->PoolElementConnectionStorage);
   poolHandlespaceNode->PoolElements = 0;
}


/* ###### Get number of timers ########################################### */
size_t ST_CLASS(poolHandlespaceNodeGetTimerNodes)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(ST_METHOD(GetElements)(&poolHandlespaceNode->PoolElementTimerStorage));
}


/* ###### Get first timer ################################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolHandlespaceNode->PoolElementTimerStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last timer ################################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetLastPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolHandlespaceNode->PoolElementTimerStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next timer ################################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetPrevPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolHandlespaceNode->PoolElementTimerStorage,
                                                   &poolElementNode->PoolElementTimerStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous timer ############################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolHandlespaceNode->PoolElementTimerStorage,
                                                   &poolElementNode->PoolElementTimerStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get number of ownership nodes ################################## */
size_t ST_CLASS(poolHandlespaceNodeGetOwnershipNodes)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(ST_METHOD(GetElements)(&poolHandlespaceNode->PoolElementOwnershipStorage));
}


/* ###### Get first ownership ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolHandlespaceNode->PoolElementOwnershipStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last ownership ############################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetLastPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolHandlespaceNode->PoolElementOwnershipStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next ownership ############################################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetPrevPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                                   &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous ownership ######################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                                   &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const RegistrarIdentifierType         homeRegistrarIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetLastPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const RegistrarIdentifierType         homeRegistrarIdentifier);


/* ###### Get prev ownership of same home NS identifier ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetPrevPoolElementOwnershipNodeForSameIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetPrev)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                               &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      prevPoolElementNode = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node);
      if(prevPoolElementNode->HomeRegistrarIdentifier == poolElementNode->HomeRegistrarIdentifier) {
         return(prevPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get next ownership of same home NS identifier ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetNext)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                               &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      nextPoolElementNode = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node);
      if(nextPoolElementNode->HomeRegistrarIdentifier == poolElementNode->HomeRegistrarIdentifier) {
         return(nextPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get number of connection nodes ################################## */
size_t ST_CLASS(poolHandlespaceNodeGetConnectionNodes)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(ST_METHOD(GetElements)(&poolHandlespaceNode->PoolElementConnectionStorage));
}


/* ###### Get first connection ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolHandlespaceNode->PoolElementConnectionStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last connection ############################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetLastPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolHandlespaceNode->PoolElementConnectionStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next connection ############################################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetPrevPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolHandlespaceNode->PoolElementConnectionStorage,
                                                   &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous connection ######################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolHandlespaceNode->PoolElementConnectionStorage,
                                                   &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get prev connection of same home NS identifier ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetPrevPoolElementConnectionNodeForSameConnection)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetPrev)(&poolHandlespaceNode->PoolElementConnectionStorage,
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
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetNext)(&poolHandlespaceNode->PoolElementConnectionStorage,
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
size_t ST_CLASS(poolHandlespaceNodeGetPoolElementNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(poolHandlespaceNode->PoolElements);
}


/* ###### Get number of pools ############################################ */
size_t ST_CLASS(poolHandlespaceNodeGetPoolNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(ST_METHOD(GetElements)(&poolHandlespaceNode->PoolIndexStorage));
}


/* ###### Get first PoolNode ############################################# */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetFirst)(&poolHandlespaceNode->PoolIndexStorage));
}


/* ###### Get last PoolNode ############################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeGetLastPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetLast)(&poolHandlespaceNode->PoolIndexStorage));
}


/* ###### Get next PoolNode ############################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              struct ST_CLASS(PoolNode)*            poolNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetNext)(&poolHandlespaceNode->PoolIndexStorage, &poolNode->PoolIndexStorageNode));
}


/* ###### Get previous PoolNode ########################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeGetPrevPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              struct ST_CLASS(PoolNode)*            poolNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetPrev)(&poolHandlespaceNode->PoolIndexStorage, &poolNode->PoolIndexStorageNode));
}


/* ###### Find PoolElementNode ########################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeFindPoolElementNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const struct PoolHandle*              poolHandle,
                                     const PoolElementIdentifierType       poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolHandlespaceNodeFindPoolNode)(
                                            poolHandlespaceNode,
                                            poolHandle);
   if(poolNode != NULL) {
      return(ST_CLASS(poolNodeFindPoolElementNode)(poolNode, poolElementIdentifier));
   }
   return(NULL);
}


/* ###### Insert PoolElementNode into timer storage ###################### */
void ST_CLASS(poolHandlespaceNodeActivateTimer)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*     poolElementNode,
        const unsigned int                    timerCode,
        const unsigned long long              timerTimeStamp)
{
   struct STN_CLASSNAME* result;

   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
   poolElementNode->TimerCode      = timerCode;
   poolElementNode->TimerTimeStamp = timerTimeStamp;
   result = ST_METHOD(Insert)(&poolHandlespaceNode->PoolElementTimerStorage,
                              &poolElementNode->PoolElementTimerStorageNode);
   CHECK(result == &poolElementNode->PoolElementTimerStorageNode);
}


/* ###### Remove PoolElementNode from timer storage ###################### */
void ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct STN_CLASSNAME* result;

   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
      result = ST_METHOD(Remove)(&poolHandlespaceNode->PoolElementTimerStorage,
                                 &poolElementNode->PoolElementTimerStorageNode);
      CHECK(result == &poolElementNode->PoolElementTimerStorageNode);
   }
}


/* ###### Select PoolElementNodes by Pool Policy ######################### */
size_t ST_CLASS(poolHandlespaceNodeSelectPoolElementNodesByPolicy)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
          const struct PoolHandle*              poolHandle,
          struct ST_CLASS(PoolElementNode)**    poolElementNodeArray,
          const size_t                          maxPoolElementNodes,
          const size_t                          maxIncrement,
          unsigned int*                         errorCode)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolHandlespaceNodeFindPoolNode)(poolHandlespaceNode, poolHandle);
   size_t                     count    = 0;
   if(poolNode != NULL) {
      *errorCode = RSPERR_OKAY;
      count = poolNode->Policy->SelectionFunction(poolNode, poolElementNodeArray, maxPoolElementNodes, maxIncrement);
#ifdef VERIFY
      ST_CLASS(poolHandlespaceNodeVerify)(poolHandlespaceNode);
#endif
   }
   else {
      *errorCode = RSPERR_NOT_FOUND;
   }
   return(count);
}


/* ###### Add PoolNode ################################################### */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeAddPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              struct ST_CLASS(PoolNode)*            poolNode)
{
   struct STN_CLASSNAME* result = ST_METHOD(Insert)(&poolHandlespaceNode->PoolIndexStorage,
                                                    &poolNode->PoolIndexStorageNode);
   if(result == &poolNode->PoolIndexStorageNode) {
      poolNode->OwnerPoolHandlespaceNode = poolHandlespaceNode;
   }
   return((struct ST_CLASS(PoolNode)*)result);
}


/* ###### Get number of pool elements of certain pool #################### */
size_t ST_CLASS(poolHandlespaceNodeGetPoolElementNodesOfPool)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
          const struct PoolHandle*              poolHandle)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolHandlespaceNodeFindPoolNode)(
                                            poolHandlespaceNode,
                                            poolHandle);
   if(poolNode) {
      return(ST_CLASS(poolNodeGetPoolElementNodes)(poolNode));
   }
   return(0);
}


/* ###### Find PoolNode ################################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeFindPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              const struct PoolHandle*              poolHandle)
{
   struct ST_CLASS(PoolNode)* poolNode;
   struct ST_CLASS(PoolNode)  cmpPoolNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   poolNode = (struct ST_CLASS(PoolNode)*)ST_METHOD(Find)(&poolHandlespaceNode->PoolIndexStorage,
                                                          &cmpPoolNode.PoolIndexStorageNode);
   return(poolNode);
}


/* ###### Find PoolNode ################################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeFindNearestPrevPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              const struct PoolHandle*              poolHandle)
{
   struct ST_CLASS(PoolNode)* poolNode;
   struct ST_CLASS(PoolNode)  cmpPoolNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   poolNode = (struct ST_CLASS(PoolNode)*)ST_METHOD(GetNearestPrev)(&poolHandlespaceNode->PoolIndexStorage,
                                                                    &cmpPoolNode.PoolIndexStorageNode);
   return(poolNode);
}


/* ###### Find PoolNode ################################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeFindNearestNextPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              const struct PoolHandle*              poolHandle)
{
   struct ST_CLASS(PoolNode)* poolNode;
   struct ST_CLASS(PoolNode)  cmpPoolNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   poolNode = (struct ST_CLASS(PoolNode)*)ST_METHOD(GetNearestNext)(&poolHandlespaceNode->PoolIndexStorage,
                                                                    &cmpPoolNode.PoolIndexStorageNode);
   return(poolNode);
}


/* ###### Find nearest prev ownership #################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeFindNearestPrevPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const RegistrarIdentifierType         homeRegistrarIdentifier,
                                     const struct PoolHandle*              poolHandle,
                                     const PoolElementIdentifierType       poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            ownershipNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode    = &cmpPoolNode;
   cmpPoolElementNode.Identifier       = poolElementIdentifier;
   cmpPoolElementNode.HomeRegistrarIdentifier = homeRegistrarIdentifier;

   ownershipNode = ST_METHOD(GetNearestPrev)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                             &cmpPoolElementNode.PoolElementOwnershipStorageNode);
   if(ownershipNode) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(ownershipNode));
   }
   return(NULL);
}


/* ###### Find nearest next ownership #################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeFindNearestNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const RegistrarIdentifierType         homeRegistrarIdentifier,
                                     const struct PoolHandle*              poolHandle,
                                     const PoolElementIdentifierType       poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            ownershipNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode    = &cmpPoolNode;
   cmpPoolElementNode.Identifier       = poolElementIdentifier;
   cmpPoolElementNode.HomeRegistrarIdentifier = homeRegistrarIdentifier;

   ownershipNode = ST_METHOD(GetNearestNext)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                             &cmpPoolElementNode.PoolElementOwnershipStorageNode);
   if(ownershipNode) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(ownershipNode));
   }
   return(NULL);
}


/* ###### Find nearest prev connection #################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeFindNearestPrevPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const int                             connectionSocketDescriptor,
                                     const sctp_assoc_t                    assocID,
                                     const struct PoolHandle*              poolHandle,
                                     const PoolElementIdentifierType       poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            connectionNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode               = &cmpPoolNode;
   cmpPoolElementNode.ConnectionSocketDescriptor = connectionSocketDescriptor;
   cmpPoolElementNode.ConnectionAssocID          = assocID;
   cmpPoolElementNode.Identifier                  = poolElementIdentifier;

   connectionNode = ST_METHOD(GetNearestPrev)(&poolHandlespaceNode->PoolElementConnectionStorage,
                                              &cmpPoolElementNode.PoolElementConnectionStorageNode);
   if(connectionNode) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(connectionNode));
   }
   return(NULL);
}


/* ###### Find nearest next connection ################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeFindNearestNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const int                             connectionSocketDescriptor,
                                     const sctp_assoc_t                    assocID,
                                     const struct PoolHandle*              poolHandle,
                                     const PoolElementIdentifierType       poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)        cmpPoolNode;
   struct ST_CLASS(PoolElementNode) cmpPoolElementNode;
   struct STN_CLASSNAME*            connectionNode;

   poolHandleNew(&cmpPoolNode.Handle, poolHandle->Handle, poolHandle->Size);
   cmpPoolElementNode.OwnerPoolNode               = &cmpPoolNode;
   cmpPoolElementNode.ConnectionSocketDescriptor = connectionSocketDescriptor;
   cmpPoolElementNode.ConnectionAssocID          = assocID;
   cmpPoolElementNode.Identifier                  = poolElementIdentifier;

   connectionNode = ST_METHOD(GetNearestNext)(&poolHandlespaceNode->PoolElementConnectionStorage,
                                              &cmpPoolElementNode.PoolElementConnectionStorageNode);
   if(connectionNode) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(connectionNode));
   }
   return(NULL);
}


/* ###### Remove PoolNode ################################################ */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeRemovePoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              struct ST_CLASS(PoolNode)*            poolNode)
{
   const struct STN_CLASSNAME* result = ST_METHOD(Remove)(&poolHandlespaceNode->PoolIndexStorage,
                                                          &poolNode->PoolIndexStorageNode);
   CHECK(result == &poolNode->PoolIndexStorageNode);
   poolNode->OwnerPoolHandlespaceNode = NULL;
   return(poolNode);
}


/* ###### Add PoolElementNode ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeAddPoolElementNode)(
                                    struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                    struct ST_CLASS(PoolNode)*            poolNode,
                                    struct ST_CLASS(PoolElementNode)*     poolElementNode,
                                    unsigned int*                         errorCode)
{
   struct ST_CLASS(PoolElementNode)* result = ST_CLASS(poolNodeAddPoolElementNode)(poolNode, poolElementNode, errorCode);
   struct STN_CLASSNAME*             result2;
   if(result == poolElementNode) {
      CHECK(*errorCode == RSPERR_OKAY);
      poolHandlespaceNode->PoolElements++;

      if(poolElementNode->HomeRegistrarIdentifier != 0) {
         result2 = ST_METHOD(Insert)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                     &poolElementNode->PoolElementOwnershipStorageNode);
         CHECK(result2 == &poolElementNode->PoolElementOwnershipStorageNode);
      }
      if(poolElementNode->ConnectionSocketDescriptor > 0) {
         result2 = ST_METHOD(Insert)(&poolHandlespaceNode->PoolElementConnectionStorage,
                                     &poolElementNode->PoolElementConnectionStorageNode);
         CHECK(result2 == &poolElementNode->PoolElementConnectionStorageNode);
      }
   }
   return(result);
}


/* ###### Update PoolElementNode's ownership ############################# */
void ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*     poolElementNode,
        const RegistrarIdentifierType         newHomeRegistrarIdentifier)
{
   struct STN_CLASSNAME* result;

   if(newHomeRegistrarIdentifier != poolElementNode->HomeRegistrarIdentifier) {
      if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementOwnershipStorageNode)) {
         result = ST_METHOD(Remove)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                    &poolElementNode->PoolElementOwnershipStorageNode);
         CHECK(result == &poolElementNode->PoolElementOwnershipStorageNode);
      }
      poolElementNode->HomeRegistrarIdentifier = newHomeRegistrarIdentifier;
      result = ST_METHOD(Insert)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                 &poolElementNode->PoolElementOwnershipStorageNode);
      CHECK(result == &poolElementNode->PoolElementOwnershipStorageNode);
   }
}


/* ###### Update PoolElementNode's connection ############################ */
void ST_CLASS(poolHandlespaceNodeUpdateConnectionOfPoolElementNode)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*     poolElementNode,
        const int                             connectionSocketDescriptor,
        const sctp_assoc_t                    connectionAssocID)
{
   struct STN_CLASSNAME* result;

   if((connectionSocketDescriptor != poolElementNode->ConnectionSocketDescriptor) ||
      (connectionAssocID          != poolElementNode->ConnectionAssocID)) {
      if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementConnectionStorageNode)) {
         result = ST_METHOD(Remove)(&poolHandlespaceNode->PoolElementConnectionStorage,
                                    &poolElementNode->PoolElementConnectionStorageNode);
         CHECK(result == &poolElementNode->PoolElementConnectionStorageNode);
      }
      poolElementNode->ConnectionSocketDescriptor = connectionSocketDescriptor;
      poolElementNode->ConnectionAssocID          = connectionAssocID;
      if(poolElementNode->ConnectionSocketDescriptor > 0) {
         result = ST_METHOD(Insert)(&poolHandlespaceNode->PoolElementConnectionStorage,
                                       &poolElementNode->PoolElementConnectionStorageNode);
         CHECK(result == &poolElementNode->PoolElementConnectionStorageNode);
      }
   }
}


/* ###### Update PoolElementNode ######################################### */
void ST_CLASS(poolHandlespaceNodeUpdatePoolElementNode)(
        struct ST_CLASS(PoolHandlespaceNode)*     poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*         poolElementNode,
        const struct ST_CLASS(PoolElementNode)*   source,
        unsigned int*                             errorCode)
{
   ST_CLASS(poolNodeUpdatePoolElementNode)(poolElementNode->OwnerPoolNode, poolElementNode, source, errorCode);
   if(*errorCode == RSPERR_OKAY) {
      /* ====== Change ownership ========================================= */
      ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
         poolHandlespaceNode, poolElementNode,
         source->HomeRegistrarIdentifier);

      /* ====== Change connection ======================================== */
      ST_CLASS(poolHandlespaceNodeUpdateConnectionOfPoolElementNode)(
         poolHandlespaceNode, poolElementNode,
         source->ConnectionSocketDescriptor,
         source->ConnectionAssocID);
   }
}


/* ###### Add or Update PoolElementNode ##################################### */
/*
   Allocation behavior:
   User program places PoolNode and PoolElementNode data in new memory areas.
   If the PoolNode or PoolElementNode structure is used for insertion into
   the handlespace, their pointer is set to NULL. So, the user program has to
   allocate new spaces for the next element. Otherwise, data has been copied
   into already existing nodes and the memory areas can be reused.
*/
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeAddOrUpdatePoolElementNode)(
                                    struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                    struct ST_CLASS(PoolNode)**           poolNode,
                                    struct ST_CLASS(PoolElementNode)**    poolElementNode,
                                    unsigned int*                         errorCode)
{
   struct ST_CLASS(PoolNode)*        newPoolNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;

   newPoolNode = ST_CLASS(poolHandlespaceNodeAddPoolNode)(poolHandlespaceNode, *poolNode);
   newPoolElementNode = ST_CLASS(poolHandlespaceNodeAddPoolElementNode)(poolHandlespaceNode, newPoolNode, *poolElementNode, errorCode);

   if(newPoolElementNode != NULL) {
      if(newPoolElementNode != *poolElementNode) {
         ST_CLASS(poolHandlespaceNodeUpdatePoolElementNode)(poolHandlespaceNode, newPoolElementNode, *poolElementNode, errorCode);
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
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeRemovePoolElementNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   struct STN_CLASSNAME*             result;
   struct ST_CLASS(PoolElementNode)* result2;

   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
      result = ST_METHOD(Remove)(&poolHandlespaceNode->PoolElementTimerStorage,
                                 &poolElementNode->PoolElementTimerStorageNode);
      CHECK(result == &poolElementNode->PoolElementTimerStorageNode);
   }
   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementOwnershipStorageNode)) {
      result = ST_METHOD(Remove)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                 &poolElementNode->PoolElementOwnershipStorageNode);
      CHECK(result == &poolElementNode->PoolElementOwnershipStorageNode);
   }
   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementConnectionStorageNode)) {
      result = ST_METHOD(Remove)(&poolHandlespaceNode->PoolElementConnectionStorage,
                                 &poolElementNode->PoolElementConnectionStorageNode);
      CHECK(result == &poolElementNode->PoolElementConnectionStorageNode);
   }
   result2 = ST_CLASS(poolNodeRemovePoolElementNode)(poolElementNode->OwnerPoolNode,
                                                     poolElementNode);
   CHECK(result2 == poolElementNode);
   CHECK(poolHandlespaceNode->PoolElements > 0);
   poolHandlespaceNode->PoolElements--;

   return(poolElementNode);
}


/* ###### Get textual description ######################################## */
void ST_CLASS(poolHandlespaceNodeGetDescription)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        char*                                 buffer,
        const size_t                          bufferSize)
{
   snprintf(buffer, bufferSize,
            "Handlespace[home=$%08x]: (%u Pools, %u PoolElements, %u owned)",
            poolHandlespaceNode->HomeRegistrarIdentifier,
            (unsigned int)ST_CLASS(poolHandlespaceNodeGetPoolNodes)(poolHandlespaceNode),
            (unsigned int)ST_CLASS(poolHandlespaceNodeGetPoolElementNodes)(poolHandlespaceNode),
            (unsigned int)ST_CLASS(poolHandlespaceNodeGetOwnershipNodes)(poolHandlespaceNode));
}


/* ###### Print ########################################################## */
void ST_CLASS(poolHandlespaceNodePrint)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        FILE*                                 fd,
        const unsigned int                    fields)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolNode)*        poolNode;
   char                              poolHandlespaceNodeDescription[256];

   ST_CLASS(poolHandlespaceNodeGetDescription)(poolHandlespaceNode,
                                               (char*)&poolHandlespaceNodeDescription,
                                               sizeof(poolHandlespaceNodeDescription));
   fputs(poolHandlespaceNodeDescription, fd);
   fputs("\n", fd);

   if(fields & PNNPO_POOLS_INDEX) {
      fputs("*-- Index:\n", fd);
      poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(poolHandlespaceNode);
      while(poolNode != NULL) {
         fprintf(fd, " +-- ");
         ST_CLASS(poolNodePrint)(poolNode, fd, (fields | PNPO_INDEX) & (~PNPO_SELECTION));
         poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(poolHandlespaceNode, poolNode);
      }
   }

   if(fields & PNNPO_POOLS_SELECTION) {
      fputs("*-- Selection:\n", fd);
      poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(poolHandlespaceNode);
      while(poolNode != NULL) {
         fprintf(fd, " +-- ");
         ST_CLASS(poolNodePrint)(poolNode, fd, (fields | PNPO_SELECTION) & (~PNPO_INDEX));
         poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(poolHandlespaceNode, poolNode);
      }
   }

   if(fields & PNNPO_POOLS_OWNERSHIP) {
      fprintf(fd,
              " *-- Ownership: (%u nodes)\n",
              (unsigned int)ST_CLASS(poolHandlespaceNodeGetOwnershipNodes)(poolHandlespaceNode));
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(poolHandlespaceNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - $%08x -> \"", poolElementNode->HomeRegistrarIdentifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_ONLY_ID);
         if(poolElementNode->HomeRegistrarIdentifier == poolHandlespaceNode->HomeRegistrarIdentifier) {
            fputs(" (property of local handlespace)", fd);
         }
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(poolHandlespaceNode, poolElementNode);
      }
   }

   if(fields & PNNPO_POOLS_CONNECTION) {
      fprintf(fd,
              " *-- Connection: (%u nodes)\n",
              (unsigned int)ST_CLASS(poolHandlespaceNodeGetConnectionNodes)(poolHandlespaceNode));
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNode)(poolHandlespaceNode);
      while(poolElementNode != NULL) {
         fputs("   - \"", fd);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_CONNECTION);
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNode)(poolHandlespaceNode, poolElementNode);
      }
   }

   if(fields & PNNPO_POOLS_TIMER) {
      fprintf(fd,
              "*-- Timer: (%u nodes)\n",
              (unsigned int)ST_CLASS(poolHandlespaceNodeGetTimerNodes)(poolHandlespaceNode));
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)(poolHandlespaceNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - \"");
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_ONLY_ID);
         fprintf(fd, " code=%u ts=%llu\n", poolElementNode->TimerCode, poolElementNode->TimerTimeStamp);
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementTimerNode)(poolHandlespaceNode, poolElementNode);
      }
   }
}


/* ###### Verify ######################################################### */
void ST_CLASS(poolHandlespaceNodeVerify)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct ST_CLASS(PoolNode)*        poolNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            i, j;

   const size_t pools        = ST_CLASS(poolHandlespaceNodeGetPoolNodes)(poolHandlespaceNode);
   const size_t poolElements = ST_CLASS(poolHandlespaceNodeGetPoolElementNodes)(poolHandlespaceNode);
   const size_t timers       = ST_CLASS(poolHandlespaceNodeGetTimerNodes)(poolHandlespaceNode);
   const size_t properties   = ST_CLASS(poolHandlespaceNodeGetOwnershipNodes)(poolHandlespaceNode);

/*
   puts("------------- VERIFY -------------------");
   ST_CLASS(poolHandlespaceNodePrint)(poolHandlespaceNode);
   puts("----------------------------------------");
*/

   ST_METHOD(Verify)(&poolHandlespaceNode->PoolIndexStorage);
   ST_METHOD(Verify)(&poolHandlespaceNode->PoolElementTimerStorage);
   ST_METHOD(Verify)(&poolHandlespaceNode->PoolElementOwnershipStorage);

   i = 0;
   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)(poolHandlespaceNode);
   while(poolElementNode != NULL) {
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementTimerNode)(poolHandlespaceNode, poolElementNode);
      i++;
   }
   CHECK(i == timers);

   i = 0;
   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(poolHandlespaceNode);
   while(poolElementNode != NULL) {
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(poolHandlespaceNode, poolElementNode);
      i++;
   }
   CHECK(i == properties);

   i = 0; j = 0;
   poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(poolHandlespaceNode);
   while(poolNode != NULL) {
      ST_METHOD(Verify)(&poolNode->PoolElementIndexStorage);
      ST_METHOD(Verify)(&poolNode->PoolElementSelectionStorage);
      CHECK(ST_METHOD(GetElements)(&poolNode->PoolElementSelectionStorage)
               == ST_METHOD(GetElements)(&poolNode->PoolElementIndexStorage));
      CHECK(ST_CLASS(poolNodeGetPoolElementNodes)(poolNode) > 0);
      j += ST_CLASS(poolNodeGetPoolElementNodes)(poolNode);
      poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(poolHandlespaceNode, poolNode);
      i++;
   }
   CHECK(i == pools);
   CHECK(j == poolElements);
   CHECK(properties <= poolElements);
}


/* ###### Clear ########################################################## */
void ST_CLASS(poolHandlespaceNodeClear)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                        void                                (*poolNodeDisposer)(void* poolNode, void* userData),
                                        void                                (*poolElementNodeDisposer)(void* poolElementNode, void* userData),
                                        void*                               userData)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(poolHandlespaceNode);
   while(poolNode != NULL) {
      ST_CLASS(poolNodeClear)(poolNode, poolElementNodeDisposer, userData);
      ST_CLASS(poolHandlespaceNodeRemovePoolNode)(poolHandlespaceNode, poolNode);
      ST_CLASS(poolNodeDelete)(poolNode);
      poolNodeDisposer(poolNode, userData);
      poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(poolHandlespaceNode);
   }
}


/* ###### Get first ownership node of given home NS identifier ########### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const RegistrarIdentifierType         homeRegistrarIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct PoolHandle                 lastPoolHandle;

   poolHandleNew(&lastPoolHandle, (unsigned char*)"\x00", 1);
   poolElementNode = ST_CLASS(poolHandlespaceNodeFindNearestNextPoolElementOwnershipNode)(
                        poolHandlespaceNode,
                        homeRegistrarIdentifier,
                        &lastPoolHandle,
                        0);
   if(poolElementNode) {
      prevPoolElementNode = ST_CLASS(poolHandlespaceNodeGetPrevPoolElementOwnershipNode)(
                               poolHandlespaceNode, poolElementNode);
      while(prevPoolElementNode) {
         if(prevPoolElementNode->HomeRegistrarIdentifier == homeRegistrarIdentifier) {
            poolElementNode = prevPoolElementNode;
         }
         else {
            break;
         }
         prevPoolElementNode = ST_CLASS(poolHandlespaceNodeGetPrevPoolElementOwnershipNode)(
                                 poolHandlespaceNode, prevPoolElementNode);
      }
   }
   if((poolElementNode != NULL) &&
      (poolElementNode->HomeRegistrarIdentifier == homeRegistrarIdentifier)) {
      return(poolElementNode);
   }
   return(NULL);
}


/* ###### Get last ownership node of given home NS identifier ########### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetLastPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const RegistrarIdentifierType         homeRegistrarIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct PoolHandle                 lastPoolHandle;

   poolHandleNew(&lastPoolHandle, (unsigned char*)"\x00", 1);
   if(homeRegistrarIdentifier == 0xffffffff) {
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetLastPoolElementOwnershipNode)(poolHandlespaceNode);
   }
   else {
      poolElementNode = ST_CLASS(poolHandlespaceNodeFindNearestPrevPoolElementOwnershipNode)(
                           poolHandlespaceNode,
                           homeRegistrarIdentifier + 1,
                           &lastPoolHandle,
                           0);
   }
   if(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(
                               poolHandlespaceNode, poolElementNode);
      while(nextPoolElementNode) {
         if(nextPoolElementNode->HomeRegistrarIdentifier == homeRegistrarIdentifier) {
            poolElementNode = nextPoolElementNode;
         }
         else {
            break;
         }
         nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(
                                 poolHandlespaceNode, nextPoolElementNode);
      }
   }
   if((poolElementNode != NULL) &&
      (poolElementNode->HomeRegistrarIdentifier == homeRegistrarIdentifier)) {
      return(poolElementNode);
   }
   return(NULL);
}


/* ###### Get number of ownership nodes ################################## */
size_t ST_CLASS(poolHandlespaceNodeGetOwnershipNodesForIdentifier)(
         struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
         const RegistrarIdentifierType         homeRegistrarIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            nodes = 0;

   poolElementNode = ST_CLASS(poolHandlespaceNodeGetLastPoolElementOwnershipNodeForIdentifier)(
                        poolHandlespaceNode, homeRegistrarIdentifier);
   while(poolElementNode != NULL) {
      nodes++;
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetPrevPoolElementOwnershipNodeForSameIdentifier)(
                           poolHandlespaceNode, poolElementNode);
   }
   return(nodes);
}


/* ###### Get first connection node of given connection ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNodeForConnection)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const int                             connectionSocketDescriptor,
                                     const sctp_assoc_t                    assocID)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct PoolHandle                 lastPoolHandle;

   poolHandleNew(&lastPoolHandle, (unsigned char*)"\x00", 1);
   poolElementNode = ST_CLASS(poolHandlespaceNodeFindNearestNextPoolElementConnectionNode)(
                        poolHandlespaceNode,
                        connectionSocketDescriptor, assocID,
                        &lastPoolHandle,
                        0);
   if(poolElementNode) {
      prevPoolElementNode = ST_CLASS(poolHandlespaceNodeGetPrevPoolElementConnectionNode)(
                               poolHandlespaceNode, poolElementNode);
      while(prevPoolElementNode) {
         if((prevPoolElementNode->ConnectionSocketDescriptor == connectionSocketDescriptor) &&
            (prevPoolElementNode->ConnectionAssocID          == assocID)) {
            poolElementNode = prevPoolElementNode;
         }
         else {
            break;
         }
         prevPoolElementNode = ST_CLASS(poolHandlespaceNodeGetPrevPoolElementConnectionNode)(
                                 poolHandlespaceNode, prevPoolElementNode);
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
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetLastPoolElementConnectionNodeForConnection)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const int                             connectionSocketDescriptor,
                                     const sctp_assoc_t                    assocID)
{
CHECK(0);
/*
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct PoolHandle                 lastPoolHandle;

   poolHandleNew(&lastPoolHandle, (unsigned char*)"\x00", 1);
   if(homeRegistrarIdentifier == 0xffffffff) {  ?????????????????????
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetLastPoolElementConnectionNode)(poolHandlespaceNode);
   }
   else {
      poolElementNode = ST_CLASS(poolHandlespaceNodeFindNearestPrevPoolElementConnectionNode)(
                           poolHandlespaceNode,
                           connectionSocketDescriptor, assocID,
                           &lastPoolHandle,
                           0);
   }
   if(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNode)(
                               poolHandlespaceNode, poolElementNode);
      while(nextPoolElementNode) {
         if((nextPoolElementNode->SocketDescriptor == connectionSocketDescriptor) &&
            (nextPoolElementNode->AssocID          == assocID)) {
            poolElementNode = nextPoolElementNode;
         }
         else {
            break;
         }
         nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNode)(
                                 poolHandlespaceNode, nextPoolElementNode);
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
size_t ST_CLASS(poolHandlespaceNodeGetConnectionNodesForConnection)(
                 struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                 const int                             connectionSocketDescriptor,
                 const sctp_assoc_t                    assocID)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            nodes = 0;

   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNodeForConnection)(
                        poolHandlespaceNode, connectionSocketDescriptor, assocID);
   while(poolElementNode != NULL) {
      nodes++;
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(
                           poolHandlespaceNode, poolElementNode);
   }
   return(nodes);
}
