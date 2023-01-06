/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2023 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: thomas.dreibholz@gmail.com
 */

/* ###### Initialize ##################################################### */
void ST_CLASS(poolHandlespaceNodeNew)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                      const RegistrarIdentifierType         homeRegistrarIdentifier,
                                      void (*poolNodeUpdateNotification)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                                                         struct ST_CLASS(PoolElementNode)*     poolElementNode,
                                                                         enum PoolNodeUpdateAction             updateAction,
                                                                         HandlespaceChecksumAccumulatorType    preUpdateChecksum,
                                                                         RegistrarIdentifierType               preUpdateHomeRegistrar,
                                                                         void*                                 userData),
                                      void* notificationUserData)
{
   ST_METHOD(New)(&poolHandlespaceNode->PoolIndexStorage, ST_CLASS(poolIndexStorageNodePrint), ST_CLASS(poolIndexStorageNodeComparison));
   ST_METHOD(New)(&poolHandlespaceNode->PoolElementTimerStorage, ST_CLASS(poolElementTimerStorageNodePrint), ST_CLASS(poolElementTimerStorageNodeComparison));
   ST_METHOD(New)(&poolHandlespaceNode->PoolElementOwnershipStorage, ST_CLASS(poolElementOwnershipStorageNodePrint), ST_CLASS(poolElementOwnershipStorageNodeComparison));
   ST_METHOD(New)(&poolHandlespaceNode->PoolElementConnectionStorage, ST_CLASS(poolElementConnectionStorageNodePrint), ST_CLASS(poolElementConnectionStorageNodeComparison));

   poolHandlespaceNode->HomeRegistrarIdentifier    = homeRegistrarIdentifier;
   poolHandlespaceNode->HandlespaceChecksum        = INITIAL_HANDLESPACE_CHECKSUM;
   poolHandlespaceNode->OwnershipChecksum          = INITIAL_HANDLESPACE_CHECKSUM;
   poolHandlespaceNode->PoolElements               = 0;
   poolHandlespaceNode->OwnedPoolElements          = 0;

   poolHandlespaceNode->PoolNodeUpdateNotification = poolNodeUpdateNotification;
   poolHandlespaceNode->NotificationUserData       = notificationUserData;
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
   poolHandlespaceNode->HandlespaceChecksum = 0;
   poolHandlespaceNode->OwnershipChecksum   = 0;
   poolHandlespaceNode->PoolElements        = 0;
   poolHandlespaceNode->OwnedPoolElements   = 0;
}


/* ###### Get handlespace checksum ####################################### */
HandlespaceChecksumAccumulatorType ST_CLASS(poolHandlespaceNodeGetHandlespaceChecksum)(
                                      const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(poolHandlespaceNode->HandlespaceChecksum);
}


/* ###### Get ownership checksum ######################################### */
HandlespaceChecksumAccumulatorType ST_CLASS(poolHandlespaceNodeGetOwnershipChecksum)(
                                      const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(poolHandlespaceNode->OwnershipChecksum);
}


/* ###### Compute handlespace checksum ################################### */
HandlespaceChecksumAccumulatorType ST_CLASS(poolHandlespaceNodeComputeHandlespaceChecksum)(
                                      const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct ST_CLASS(PoolElementNode)*  poolElementNode;
   struct ST_CLASS(PoolNode)*         poolNode;
   HandlespaceChecksumAccumulatorType sum = 0;

   poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(
                 (struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode);
   while(poolNode != NULL) {
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
      while(poolElementNode != NULL) {
         sum = handlespaceChecksumAdd(sum,
                                      ST_CLASS(poolElementNodeComputeChecksum)(poolElementNode));
         poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)(poolNode, poolElementNode);
      }
      poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(
                    (struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode, poolNode);
   }

   return(sum);
}


/* ###### Compute own nodes checksum ##################################### */
HandlespaceChecksumAccumulatorType ST_CLASS(poolHandlespaceNodeComputeOwnershipChecksum)(
                                      const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                      const RegistrarIdentifierType               registrarIdentifier)
{
   struct ST_CLASS(PoolElementNode)*  poolElementNode;
   HandlespaceChecksumAccumulatorType sum = 0;

   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        (struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode,
                        registrarIdentifier);
   while(poolElementNode != NULL) {
      sum = handlespaceChecksumAdd(sum,
                                   ST_CLASS(poolElementNodeComputeChecksum)(poolElementNode));
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                           (struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode,
                           poolElementNode);
   }

   return(sum);
}


/* ###### Get number of timers ########################################### */
size_t ST_CLASS(poolHandlespaceNodeGetTimerNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
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


/* ###### Get next ownership ############################################## */
static struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetPrevPoolElementOwnershipNode)(
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


/* ###### Get next ownership of same home PR identifier ################## */
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
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
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


/* ###### Get next connection ############################################## */
static struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetPrevPoolElementConnectionNode)(
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


/* ###### Get next connection of same home PR identifier ################## */
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


/* ###### Get number of pool elements #################################### */
size_t ST_CLASS(poolHandlespaceNodeGetPoolElementNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(poolHandlespaceNode->PoolElements);
}


/* ###### Get number of owned pool elements ############################## */
size_t ST_CLASS(poolHandlespaceNodeGetOwnedPoolElementNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   return(poolHandlespaceNode->OwnedPoolElements);
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


/* ###### Get next PoolNode ############################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              struct ST_CLASS(PoolNode)*            poolNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetNext)(&poolHandlespaceNode->PoolIndexStorage, &poolNode->PoolIndexStorageNode));
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


/* ###### Check, if PoolElementNode has an active timer ################## */
int ST_CLASS(poolHandlespaceNodeHasActiveTimer)(
       const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
       const struct ST_CLASS(PoolElementNode)*     poolElementNode)
{
   return(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
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
   cmpPoolElementNode.OwnerPoolNode           = &cmpPoolNode;
   cmpPoolElementNode.Identifier              = poolElementIdentifier;
   cmpPoolElementNode.HomeRegistrarIdentifier = homeRegistrarIdentifier;

   ownershipNode = ST_METHOD(GetNearestNext)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                             &cmpPoolElementNode.PoolElementOwnershipStorageNode);
   if(ownershipNode) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(ownershipNode));
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
   struct ST_CLASS(PoolElementNode)* result;
   struct STN_CLASSNAME*             result2;

   result = ST_CLASS(poolNodeAddPoolElementNode)(poolNode, poolElementNode, errorCode);
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
   struct STN_CLASSNAME*              result;
   HandlespaceChecksumAccumulatorType preUpdateChecksum;
   RegistrarIdentifierType            preUpdateHomeRegistrar;

   /* ====== Store old checksum and registrar for notification =========== */
   preUpdateChecksum      = poolElementNode->Checksum;
   preUpdateHomeRegistrar = poolElementNode->HomeRegistrarIdentifier;

   /* ====== Change ownership ============================================ */
   if(newHomeRegistrarIdentifier != poolElementNode->HomeRegistrarIdentifier) {
      if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementOwnershipStorageNode)) {
         result = ST_METHOD(Remove)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                    &poolElementNode->PoolElementOwnershipStorageNode);
         CHECK(result == &poolElementNode->PoolElementOwnershipStorageNode);
      }
      poolElementNode->Flags |= PENF_UPDATED;
      poolElementNode->HomeRegistrarIdentifier = newHomeRegistrarIdentifier;
      result = ST_METHOD(Insert)(&poolHandlespaceNode->PoolElementOwnershipStorage,
                                 &poolElementNode->PoolElementOwnershipStorageNode);
      CHECK(result == &poolElementNode->PoolElementOwnershipStorageNode);
   }
   else {
      poolElementNode->Flags &= ~PENF_UPDATED;
   }

   /* ====== Update handlespace checksum ================================= */
   poolHandlespaceNode->HandlespaceChecksum = handlespaceChecksumSub(
                                                   poolHandlespaceNode->HandlespaceChecksum,
                                                   poolElementNode->Checksum);
   if(preUpdateHomeRegistrar == poolHandlespaceNode->HomeRegistrarIdentifier) {
      CHECK(poolHandlespaceNode->OwnedPoolElements > 0);
      poolHandlespaceNode->OwnedPoolElements--;
      poolHandlespaceNode->OwnershipChecksum = handlespaceChecksumSub(
                                                   poolHandlespaceNode->OwnershipChecksum,
                                                   poolElementNode->Checksum);
   }

   poolElementNode->Checksum = ST_CLASS(poolElementNodeComputeChecksum)(poolElementNode);

   poolHandlespaceNode->HandlespaceChecksum = handlespaceChecksumAdd(
                                                   poolHandlespaceNode->HandlespaceChecksum,
                                                   poolElementNode->Checksum);
   if(poolElementNode->HomeRegistrarIdentifier == poolHandlespaceNode->HomeRegistrarIdentifier) {
      poolHandlespaceNode->OwnedPoolElements++;
      poolHandlespaceNode->OwnershipChecksum = handlespaceChecksumAdd(
                                                   poolHandlespaceNode->OwnershipChecksum,
                                                   poolElementNode->Checksum);
   }
   if(poolHandlespaceNode->PoolNodeUpdateNotification) {
      poolHandlespaceNode->PoolNodeUpdateNotification(poolHandlespaceNode,
                                                      poolElementNode,
                                                      PNUA_Update,
                                                      preUpdateChecksum,
                                                      preUpdateHomeRegistrar,
                                                      poolHandlespaceNode->NotificationUserData);
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
   ST_CLASS(poolNodeUpdatePoolElementNode)(poolElementNode->OwnerPoolNode,
                                           poolElementNode, source, errorCode);
   if(*errorCode == RSPERR_OKAY) {
      /* ====== Change connection ======================================== */
      ST_CLASS(poolHandlespaceNodeUpdateConnectionOfPoolElementNode)(
         poolHandlespaceNode, poolElementNode,
         source->ConnectionSocketDescriptor,
         source->ConnectionAssocID);

      /* ====== Change ownership ========================================= */
      /* Important: This function also takes care for updating
                    Handlespace Checksum and Ownership Checksum! */
      ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
         poolHandlespaceNode, poolElementNode,
         source->HomeRegistrarIdentifier);

      poolElementNode->Flags &= ~PENF_NEW;
   }

#ifdef VERIFY
   ST_CLASS(poolHandlespaceNodeVerify)(poolHandlespaceNode);
#endif
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
         /* ====== PE entry update ======================================= */
         ST_CLASS(poolHandlespaceNodeUpdatePoolElementNode)(poolHandlespaceNode, newPoolElementNode, *poolElementNode, errorCode);
      }
      else {
         /* ====== New PE entry ========================================== */
         *poolElementNode = NULL;

         /* ====== Update handlespace checksum =========================== */
         newPoolElementNode->Checksum = ST_CLASS(poolElementNodeComputeChecksum)(newPoolElementNode);
         poolHandlespaceNode->HandlespaceChecksum = handlespaceChecksumAdd(
                                                       poolHandlespaceNode->HandlespaceChecksum,
                                                       newPoolElementNode->Checksum);
         if(newPoolElementNode->HomeRegistrarIdentifier == poolHandlespaceNode->HomeRegistrarIdentifier) {
            poolHandlespaceNode->OwnedPoolElements++;
            poolHandlespaceNode->OwnershipChecksum = handlespaceChecksumAdd(
                                                        poolHandlespaceNode->OwnershipChecksum,
                                                        newPoolElementNode->Checksum);
         }
         if(poolHandlespaceNode->PoolNodeUpdateNotification) {
            poolHandlespaceNode->PoolNodeUpdateNotification(poolHandlespaceNode,
                                                            newPoolElementNode,
                                                            PNUA_Create,
                                                            INITIAL_HANDLESPACE_CHECKSUM,
                                                            UNDEFINED_REGISTRAR_IDENTIFIER,
                                                            poolHandlespaceNode->NotificationUserData);
         }
         newPoolElementNode->Flags |= PENF_NEW;
      }
   }
   if(newPoolNode == *poolNode) {
      /* A new pool has been created. */
      if(newPoolElementNode != NULL) {
         /* Keep the new pool, since the new PE is its first element. */
         *poolNode = NULL;
      }
      else {
         /* A new pool has been created but a PE not been registered.
            Remove the empty pool now! */
         CHECK(ST_CLASS(poolHandlespaceNodeRemovePoolNode)(poolHandlespaceNode, *poolNode) ==
                  *poolNode);
      }
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

   /* ====== Unlink PE entry ============================================= */
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

   /* ====== Update handlespace checksum ================================= */
   poolHandlespaceNode->HandlespaceChecksum = handlespaceChecksumSub(
                                                 poolHandlespaceNode->HandlespaceChecksum,
                                                 poolElementNode->Checksum);
   if(poolElementNode->HomeRegistrarIdentifier == poolHandlespaceNode->HomeRegistrarIdentifier) {
      CHECK(poolHandlespaceNode->OwnedPoolElements > 0);
      poolHandlespaceNode->OwnedPoolElements--;
      poolHandlespaceNode->OwnershipChecksum = handlespaceChecksumSub(
                                                  poolHandlespaceNode->OwnershipChecksum,
                                                  poolElementNode->Checksum);
   }
   if(poolHandlespaceNode->PoolNodeUpdateNotification) {
      poolHandlespaceNode->PoolNodeUpdateNotification(poolHandlespaceNode,
                                                      poolElementNode,
                                                      PNUA_Delete,
                                                      poolElementNode->Checksum,
                                                      poolElementNode->HomeRegistrarIdentifier,
                                                      poolHandlespaceNode->NotificationUserData);
   }

   return(poolElementNode);
}


/* ###### Get textual description ######################################## */
void ST_CLASS(poolHandlespaceNodeGetDescription)(
        const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        char*                                       buffer,
        const size_t                                bufferSize)
{
   snprintf(buffer, bufferSize,
            "Handlespace[home=$%08x]: (%u Pools, %u PoolElements, %u Owned)",
            poolHandlespaceNode->HomeRegistrarIdentifier,
            (unsigned int)ST_CLASS(poolHandlespaceNodeGetPoolNodes)(poolHandlespaceNode),
            (unsigned int)ST_CLASS(poolHandlespaceNodeGetPoolElementNodes)(poolHandlespaceNode),
            (unsigned int)ST_CLASS(poolHandlespaceNodeGetOwnedPoolElementNodes)(poolHandlespaceNode));
}


/* ###### Print ########################################################## */
void ST_CLASS(poolHandlespaceNodePrint)(
        const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        FILE*                                       fd,
        const unsigned int                          fields)
{
   const struct ST_CLASS(PoolElementNode)* poolElementNode;
   const struct ST_CLASS(PoolNode)*        poolNode;
   char                                    poolHandlespaceNodeDescription[256];

   ST_CLASS(poolHandlespaceNodeGetDescription)(poolHandlespaceNode,
                                               (char*)&poolHandlespaceNodeDescription,
                                               sizeof(poolHandlespaceNodeDescription));
   fputs(poolHandlespaceNodeDescription, fd);
   fputs("\n", fd);

   fprintf(fd, "-- Checksums: Handlespace=$%08x, Ownership=$%08x\n",
           (unsigned int)handlespaceChecksumFinish(poolHandlespaceNode->HandlespaceChecksum),
           (unsigned int)handlespaceChecksumFinish(poolHandlespaceNode->OwnershipChecksum));

   if(fields & PNNPO_POOLS_INDEX) {
      fputs("*-- Index:\n", fd);
      poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode);
      while(poolNode != NULL) {
         fprintf(fd, " +-- ");
         ST_CLASS(poolNodePrint)(poolNode, fd, (fields | PNPO_INDEX) & (~PNPO_SELECTION));
         poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode, (struct ST_CLASS(PoolNode)*)poolNode);
      }
   }

   if(fields & PNNPO_POOLS_SELECTION) {
      fputs("*-- Selection:\n", fd);
      poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode);
      while(poolNode != NULL) {
         fprintf(fd, " +-- ");
         ST_CLASS(poolNodePrint)(poolNode, fd, (fields | PNPO_SELECTION) & (~PNPO_INDEX));
         poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode, (struct ST_CLASS(PoolNode)*)poolNode);
      }
   }

   if(fields & PNNPO_POOLS_OWNERSHIP) {
      fputs("*-- Ownership:\n", fd);
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - $%08x -> \"", poolElementNode->HomeRegistrarIdentifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_ONLY_ID);
         if(poolElementNode->HomeRegistrarIdentifier == poolHandlespaceNode->HomeRegistrarIdentifier) {
            fputs(" (property of local handlespace)", fd);
         }
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode, (struct ST_CLASS(PoolElementNode)*)poolElementNode);
      }
   }

   if(fields & PNNPO_POOLS_CONNECTION) {
      fprintf(fd,
              "*-- Connection: (%u nodes)\n",
              (unsigned int)ST_CLASS(poolHandlespaceNodeGetConnectionNodes)(poolHandlespaceNode));
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode);
      while(poolElementNode != NULL) {
         fputs("   - \"", fd);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_CONNECTION);
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode, (struct ST_CLASS(PoolElementNode)*)poolElementNode);
      }
   }

   if(fields & PNNPO_POOLS_TIMER) {
      fprintf(fd,
              "*-- Timer: (%u nodes)\n",
              (unsigned int)ST_CLASS(poolHandlespaceNodeGetTimerNodes)(poolHandlespaceNode));
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - \"");
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, fd);
         fprintf(fd, "\" / ");
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, PENPO_ONLY_ID);
         fprintf(fd, " code=%u ts=%llu\n", poolElementNode->TimerCode, poolElementNode->TimerTimeStamp);
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementTimerNode)((struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode, (struct ST_CLASS(PoolElementNode)*)poolElementNode);
      }
   }
}


/* ###### Verify ######################################################### */
void ST_CLASS(poolHandlespaceNodeVerify)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode)
{
   struct ST_CLASS(PoolNode)*        poolNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            i, j;
   size_t                            ownedPEs;

   const size_t pools        = ST_CLASS(poolHandlespaceNodeGetPoolNodes)(poolHandlespaceNode);
   const size_t poolElements = ST_CLASS(poolHandlespaceNodeGetPoolElementNodes)(poolHandlespaceNode);
   const size_t timers       = ST_CLASS(poolHandlespaceNodeGetTimerNodes)(poolHandlespaceNode);
   const size_t ownerships   = ST_METHOD(GetElements)(&poolHandlespaceNode->PoolElementOwnershipStorage);

/*
   puts("------------- VERIFY -------------------");
   ST_CLASS(poolHandlespaceNodePrint)(poolHandlespaceNode, stdout, ~0);
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

   ownedPEs = 0;
   i = 0;
   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(poolHandlespaceNode);
   while(poolElementNode != NULL) {
      if(poolElementNode->HomeRegistrarIdentifier == poolHandlespaceNode->HomeRegistrarIdentifier) {
         ownedPEs++;
      }
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(poolHandlespaceNode, poolElementNode);
      i++;
   }
   CHECK(i == ownerships);
   CHECK((poolHandlespaceNode->HomeRegistrarIdentifier == UNDEFINED_REGISTRAR_IDENTIFIER) ||
         (poolHandlespaceNode->OwnedPoolElements == ownedPEs));

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
   CHECK(ownerships <= poolElements);

   CHECK(ST_CLASS(poolHandlespaceNodeGetHandlespaceChecksum)(
            (struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode) ==
         ST_CLASS(poolHandlespaceNodeComputeHandlespaceChecksum)(
            (struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode));

   if(poolHandlespaceNode->HomeRegistrarIdentifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
      CHECK(ST_CLASS(poolHandlespaceNodeGetOwnershipChecksum)(
               (struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode) ==
            ST_CLASS(poolHandlespaceNodeComputeOwnershipChecksum)(
               (struct ST_CLASS(PoolHandlespaceNode)*)poolHandlespaceNode,
               poolHandlespaceNode->HomeRegistrarIdentifier));
   }
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


/* ###### Get first ownership node of given home PR identifier ########### */
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


/* ###### Get number of ownership nodes ################################## */
size_t ST_CLASS(poolHandlespaceNodeGetOwnershipNodesForIdentifier)(
         struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
         const RegistrarIdentifierType         homeRegistrarIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            nodes = 0;

   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        poolHandlespaceNode, homeRegistrarIdentifier);
   while(poolElementNode != NULL) {
      nodes++;
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
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
