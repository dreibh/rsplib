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


/*
#define PRINT_SELECTION_RESULT
*/
/*
#define PRINT_GETNAMETABLE_RESULT
*/


/* ###### Initialize ##################################################### */
void ST_CLASS(poolNamespaceManagementNew)(
        struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
        const ENRPIdentifierType                  homeNSIdentifier,
        void (*poolNodeUserDataDisposer)(struct ST_CLASS(PoolNode)* poolElementNode,
                                         void*                      userData),
        void (*poolElementNodeUserDataDisposer)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                                void*                             userData),
        void* disposerUserData)
{
   ST_CLASS(poolNamespaceNodeNew)(&poolNamespaceManagement->Namespace, homeNSIdentifier);
   poolNamespaceManagement->NewPoolNode                     = NULL;
   poolNamespaceManagement->NewPoolElementNode              = NULL;
   poolNamespaceManagement->PoolNodeUserDataDisposer        = poolNodeUserDataDisposer;
   poolNamespaceManagement->PoolElementNodeUserDataDisposer = poolElementNodeUserDataDisposer;
   poolNamespaceManagement->DisposerUserData                = disposerUserData;
}


/* ###### PoolElementNode deallocation helper ############################ */
static void ST_CLASS(poolNamespaceManagementPoolElementNodeDisposer)(void* arg1,
                                                                     void* arg2)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode                 = (struct ST_CLASS(PoolElementNode)*)arg1;
   struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement = (struct ST_CLASS(PoolNamespaceManagement)*)arg2;
   if((poolElementNode->UserData) && (poolNamespaceManagement->PoolElementNodeUserDataDisposer))  {
      poolNamespaceManagement->PoolElementNodeUserDataDisposer(poolElementNode,
                                                               poolNamespaceManagement->DisposerUserData);
      poolElementNode->UserData = NULL;
   }
   transportAddressBlockDelete(poolElementNode->AddressBlock);
   free(poolElementNode->AddressBlock);
   poolElementNode->AddressBlock = NULL;
   free(poolElementNode);
}


/* ###### PoolNode deallocation helper ################################### */
static void ST_CLASS(poolNamespaceManagementPoolNodeDisposer)(void* arg1,
                                                              void* arg2)
{
   struct ST_CLASS(PoolNode)* poolNode                               = (struct ST_CLASS(PoolNode)*)arg1;
   struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement = (struct ST_CLASS(PoolNamespaceManagement)*)arg2;
   if((poolNode->UserData) && (poolNamespaceManagement->PoolNodeUserDataDisposer))  {
      poolNamespaceManagement->PoolNodeUserDataDisposer(poolNode,
                                                        poolNamespaceManagement->DisposerUserData);
      poolNode->UserData = NULL;
   }
   free(poolNode);
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolNamespaceManagementDelete)(
        struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement)
{
   ST_CLASS(poolNamespaceManagementClear)(poolNamespaceManagement);
   ST_CLASS(poolNamespaceNodeDelete)(&poolNamespaceManagement->Namespace);
   if(poolNamespaceManagement->NewPoolNode) {
      free(poolNamespaceManagement->NewPoolNode);
      poolNamespaceManagement->NewPoolNode = NULL;
   }
   if(poolNamespaceManagement->NewPoolElementNode) {
      free(poolNamespaceManagement->NewPoolElementNode);
      poolNamespaceManagement->NewPoolElementNode = NULL;
   }
}


/* ###### Clear namespace ################################################ */
void ST_CLASS(poolNamespaceManagementClear)(
        struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement)
{
   ST_CLASS(poolNamespaceNodeClear)(&poolNamespaceManagement->Namespace,
                                    ST_CLASS(poolNamespaceManagementPoolNodeDisposer),
                                    ST_CLASS(poolNamespaceManagementPoolElementNodeDisposer),
                                    (void*)poolNamespaceManagement);
}


/* ###### Registration ################################################### */
unsigned int ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
                struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                const struct PoolHandle*                  poolHandle,
                const ENRPIdentifierType                  homeNSIdentifier,
                const PoolElementIdentifierType           poolElementIdentifier,
                const unsigned int                        registrationLife,
                const struct PoolPolicySettings*          poolPolicySettings,
                const struct TransportAddressBlock*       transportAddressBlock,
                const int                                 registratorSocketDescriptor,
                const sctp_assoc_t                        registratorAssocID,
                const unsigned long long                  currentTimeStamp,
                struct ST_CLASS(PoolElementNode)**        poolElementNode)
{
   const struct ST_CLASS(PoolPolicy)*       poolPolicy;
   struct TransportAddressBlock*            userTransport;
   unsigned int                             errorCode;

   *poolElementNode = 0;
   if((poolHandle->Size < 1) || (poolHandle->Size > MAX_POOLHANDLESIZE)) {
      return(RSPERR_INVALID_POOL_HANDLE);
   }
   poolPolicy = ST_CLASS(poolPolicyGetPoolPolicyByType)(poolPolicySettings->PolicyType);
   if(poolPolicy == NULL) {
      return(RSPERR_INVALID_POOL_POLICY);
   }
   if(poolNamespaceManagement->NewPoolNode == NULL) {
      poolNamespaceManagement->NewPoolNode = (struct ST_CLASS(PoolNode)*)malloc(sizeof(struct ST_CLASS(PoolNode)));
      if(poolNamespaceManagement->NewPoolNode == NULL) {
         return(RSPERR_NO_RESOURCES);
      }
   }
   ST_CLASS(poolNodeNew)(poolNamespaceManagement->NewPoolNode,
                         poolHandle, poolPolicy,
                         transportAddressBlock->Protocol,
                         (transportAddressBlock->Flags & TABF_CONTROLCHANNEL) ? PNF_CONTROLCHANNEL : 0);

   if(poolNamespaceManagement->NewPoolElementNode == NULL) {
      poolNamespaceManagement->NewPoolElementNode = (struct ST_CLASS(PoolElementNode)*)malloc(sizeof(struct ST_CLASS(PoolElementNode)));
      if(poolNamespaceManagement->NewPoolElementNode == NULL) {
         return(RSPERR_NO_RESOURCES);
      }
   }

   /*
      Attention:
      transportAddressBlock is given as TransportAddressBlock here, since
      poolNamespaceNodeAddOrUpdatePoolElementNode must be able to check it
      for compatibility to the pool.
      If the node is inserted as new pool element node, the AddressBlock field
      *must* be updated with a duplicate of transportAddressBlock's data!
    */
   ST_CLASS(poolElementNodeNew)(poolNamespaceManagement->NewPoolElementNode,
                                poolElementIdentifier,
                                homeNSIdentifier,
                                registrationLife,
                                poolPolicySettings,
                                (struct TransportAddressBlock*)transportAddressBlock,
                                registratorSocketDescriptor,
                                registratorAssocID);
   *poolElementNode = ST_CLASS(poolNamespaceNodeAddOrUpdatePoolElementNode)(&poolNamespaceManagement->Namespace,
                                                                            &poolNamespaceManagement->NewPoolNode,
                                                                            &poolNamespaceManagement->NewPoolElementNode,
                                                                            &errorCode);
   if(errorCode == RSPERR_OKAY) {
      (*poolElementNode)->LastUpdateTimeStamp = currentTimeStamp;

      userTransport = transportAddressBlockDuplicate(transportAddressBlock);
      if(userTransport != NULL) {
         if((*poolElementNode)->AddressBlock != transportAddressBlock) {  /* see comment above! */
            transportAddressBlockDelete((*poolElementNode)->AddressBlock);
            free((*poolElementNode)->AddressBlock);
         }
         (*poolElementNode)->AddressBlock = userTransport;
      }
      else {
         ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
            poolNamespaceManagement,
            poolHandle,
            poolElementIdentifier);
         *poolElementNode = NULL;
         errorCode = RSPERR_NO_RESOURCES;
      }
   }

#ifdef VERIFY
   ST_CLASS(poolNamespaceNodeVerify)(&poolNamespaceManagement->Namespace);
#endif
   return(errorCode);
}


/* ###### Deregistration ################################################# */
unsigned int ST_CLASS(poolNamespaceManagementDeregisterPoolElementByPtr)(
                struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                struct ST_CLASS(PoolElementNode)*         poolElementNode)
{
   struct ST_CLASS(PoolNode)* poolNode = poolElementNode->OwnerPoolNode;

   ST_CLASS(poolNamespaceNodeRemovePoolElementNode)(&poolNamespaceManagement->Namespace, poolElementNode);
   ST_CLASS(poolElementNodeDelete)(poolElementNode);
   ST_CLASS(poolNamespaceManagementPoolElementNodeDisposer)(poolElementNode, poolNamespaceManagement);

   if(ST_CLASS(poolNodeGetPoolElementNodes)(poolNode) == 0) {
      ST_CLASS(poolNamespaceNodeRemovePoolNode)(
         &poolNamespaceManagement->Namespace, poolNode);
      ST_CLASS(poolNodeDelete)(poolNode);
      ST_CLASS(poolNamespaceManagementPoolNodeDisposer)(poolNode, poolNamespaceManagement);
   }
#ifdef VERIFY
      ST_CLASS(poolNamespaceNodeVerify)(&poolNamespaceManagement->Namespace);
#endif
   return(RSPERR_OKAY);
}


/* ###### Deregistration ################################################# */
unsigned int ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
                struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                const struct PoolHandle*                  poolHandle,
                const PoolElementIdentifierType           poolElementIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode = ST_CLASS(poolNamespaceNodeFindPoolElementNode)(
                                                          &poolNamespaceManagement->Namespace,
                                                          poolHandle,
                                                          poolElementIdentifier);
   if(poolElementNode) {
      return(ST_CLASS(poolNamespaceManagementDeregisterPoolElementByPtr)(
                poolNamespaceManagement,
                poolElementNode));
   }
   return(RSPERR_NOT_FOUND);
}


/* ###### Name Resolution ################################################ */
unsigned int ST_CLASS(poolNamespaceManagementNameResolution)(
                struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                const struct PoolHandle*                  poolHandle,
                struct ST_CLASS(PoolElementNode)**        poolElementNodeArray,
                size_t*                                   poolElementNodes,
                const size_t                              maxNameResolutionItems,
                const size_t                              maxIncrement)
{
   unsigned int errorCode;
   *poolElementNodes = ST_CLASS(poolNamespaceNodeSelectPoolElementNodesByPolicy)(
                          &poolNamespaceManagement->Namespace,
                          poolHandle,
                          poolElementNodeArray,
                          maxNameResolutionItems, maxIncrement,
                          &errorCode);
#ifdef VERIFY
   ST_CLASS(poolNamespaceNodeVerify)(&poolNamespaceManagement->Namespace);
#endif

#ifdef PRINT_SELECTION_RESULT
   size_t i;
   puts("--- Selection -------------------------------------------------------");
   printf("Pool \"");
   poolHandlePrint(poolHandle, stdout);
   puts("\":");
   for(i = 0;i < *poolElementNodes;i++) {
      printf("#%02u -> ", i);
      ST_CLASS(poolElementNodePrint)(poolElementNodeArray[i], PENPO_ONLY_POLICY);
      puts("");
   }
   puts("---------------------------------------------------------------------");
#endif

   return(errorCode);
}


/* ###### Get name table from namespace ################################## */
static int ST_CLASS(getOwnershipNameTable)(
              struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
              struct ST_CLASS(NameTableExtract)*        nameTableExtract,
              const ENRPIdentifierType                  homeNSIdentifier,
              const unsigned int                        flags)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   if(flags & NTEF_START) {
      poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                           &poolNamespaceManagement->Namespace,
                           homeNSIdentifier);
   }
   else {
      poolElementNode = ST_CLASS(poolNamespaceNodeFindNearestNextPoolElementOwnershipNode)(
                           &poolNamespaceManagement->Namespace,
                           homeNSIdentifier,
                           &nameTableExtract->LastPoolHandle,
                           nameTableExtract->LastPoolElementIdentifier);
   }

   nameTableExtract->PoolElementNodes = 0;
   while(poolElementNode != NULL) {
      nameTableExtract->PoolElementNodeArray[nameTableExtract->PoolElementNodes++] = poolElementNode;
      if(nameTableExtract->PoolElementNodes >= NTE_MAX_POOL_ELEMENT_NODES) {
         break;
      }
      poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                           &poolNamespaceManagement->Namespace, poolElementNode);
   }

   if(nameTableExtract->PoolElementNodes > 0) {
      struct ST_CLASS(PoolElementNode)* lastPoolElementNode = nameTableExtract->PoolElementNodeArray[nameTableExtract->PoolElementNodes - 1];
      struct ST_CLASS(PoolNode)* lastPoolNode               = lastPoolElementNode->OwnerPoolNode;
      nameTableExtract->LastPoolHandle                      = lastPoolNode->Handle;
      nameTableExtract->LastPoolElementIdentifier           = lastPoolElementNode->Identifier;
   }

#ifdef PRINT_GETNAMETABLE_RESULT
   printf("GetOwnershipNameTable result: %u items\n", nameTableExtract->PoolElementNodes);
   for(size_t i = 0;i < nameTableExtract->PoolElementNodes;i++) {
      poolElementNode = nameTableExtract->PoolElementNodeArray[i];
      printf("#%3d: \"", i);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdout);
      printf("\"/%d, $%08x", poolElementNode->OwnerPoolNode->HandleSize, poolElementNode->Identifier);
      puts("");
   }
#endif
   return(nameTableExtract->PoolElementNodes > 0);
}


/* ###### Get name table from namespace ################################## */
static int ST_CLASS(getGlobalNameTable)(struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                                        struct ST_CLASS(NameTableExtract)*        nameTableExtract,
                                        const unsigned int                        flags)
{
   struct ST_CLASS(PoolNode)*        poolNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   nameTableExtract->PoolElementNodes = 0;
   if(flags & NTEF_START) {
      nameTableExtract->LastPoolElementIdentifier = 0;
      nameTableExtract->LastPoolHandle.Size       = 0;
      poolNode = ST_CLASS(poolNamespaceNodeGetFirstPoolNode)(&poolNamespaceManagement->Namespace);
      if(poolNode == NULL) {
         return(0);
      }
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
   }
   else {
      poolNode = ST_CLASS(poolNamespaceNodeFindPoolNode)(
                    &poolNamespaceManagement->Namespace,
                    &nameTableExtract->LastPoolHandle);
      if(poolNode == NULL) {
         poolNode = ST_CLASS(poolNamespaceNodeFindNearestNextPoolNode)(
                       &poolNamespaceManagement->Namespace,
                       &nameTableExtract->LastPoolHandle);
      }
      if(poolNode == NULL) {
         return(0);
      }
      if(poolHandleComparison(&nameTableExtract->LastPoolHandle,
                              &poolNode->Handle) == 0) {
         poolElementNode = ST_CLASS(poolNodeFindNearestNextPoolElementNode)(poolNode, nameTableExtract->LastPoolElementIdentifier);
      }
      else {
         poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
      }
   }


   for(;;) {
      while(poolElementNode != NULL) {
         nameTableExtract->PoolElementNodeArray[nameTableExtract->PoolElementNodes++] = poolElementNode;
         if(nameTableExtract->PoolElementNodes >= NTE_MAX_POOL_ELEMENT_NODES) {
            goto finish;
         }
         poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)(poolNode, poolElementNode);
      }
      poolNode = ST_CLASS(poolNamespaceNodeGetNextPoolNode)(
                    &poolNamespaceManagement->Namespace, poolNode);
      if(poolNode) {
         poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
      }
      else {
         break;
      }
   }


finish:
   if(nameTableExtract->PoolElementNodes > 0) {
      struct ST_CLASS(PoolElementNode)* lastPoolElementNode = nameTableExtract->PoolElementNodeArray[nameTableExtract->PoolElementNodes - 1];
      struct ST_CLASS(PoolNode)* lastPoolNode               = lastPoolElementNode->OwnerPoolNode;
      nameTableExtract->LastPoolHandle            = lastPoolNode->Handle;
      nameTableExtract->LastPoolElementIdentifier = lastPoolElementNode->Identifier;
   }


#ifdef PRINT_GETNAMETABLE_RESULT
   printf("GetGlobalNameTable result: %u items\n", nameTableExtract->PoolElementNodes);
   for(size_t i = 0;i < nameTableExtract->PoolElementNodes;i++) {
      poolElementNode = nameTableExtract->PoolElementNodeArray[i];
      printf("#%3d: \"", i);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdout);
      printf("\"/%d, $%08x", poolElementNode->OwnerPoolNode->Handle->Size,
                             poolElementNode->Identifier);
      puts("");
   }
#endif
   return(nameTableExtract->PoolElementNodes > 0);
}


/* ###### Get name table from namespace ################################## */
int ST_CLASS(poolNamespaceManagementGetNameTable)(
       struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
       const ENRPIdentifierType                  homeNSIdentifier,
       struct ST_CLASS(NameTableExtract)*        nameTableExtract,
       const unsigned int                        flags)
{
   if(flags & NTEF_OWNCHILDSONLY) {
      return(ST_CLASS(getOwnershipNameTable)(
                poolNamespaceManagement, nameTableExtract,
                homeNSIdentifier,
                flags));
   }
   return(ST_CLASS(getGlobalNameTable)(
             poolNamespaceManagement, nameTableExtract, flags));
}


/* ###### Restart PE expiry timer to last update TS + expiry timeout ##### */
void ST_CLASS(poolNamespaceManagementRestartPoolElementExpiryTimer)(
        struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
        struct ST_CLASS(PoolElementNode)*         poolElementNode,
        const unsigned long long                  expiryTimeout)
{
   ST_CLASS(poolNamespaceNodeDeactivateTimer)(&poolNamespaceManagement->Namespace,
                                              poolElementNode);
   ST_CLASS(poolNamespaceNodeActivateTimer)(&poolNamespaceManagement->Namespace,
                                            poolElementNode,
                                            PENT_EXPIRY,
                                            poolElementNode->LastUpdateTimeStamp + expiryTimeout);
}


/* ###### Purge namespace from expired PE entries ######################## */
size_t ST_CLASS(poolNamespaceManagementPurgeExpiredPoolElements)(
          struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
          const unsigned long long                  currentTimeStamp)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   size_t                            purgedPoolElements = 0;

   poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementTimerNode)(&poolNamespaceManagement->Namespace);
   while(poolElementNode != NULL) {
      nextPoolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementTimerNode)(&poolNamespaceManagement->Namespace, poolElementNode);

      CHECK(poolElementNode->TimerCode == PENT_EXPIRY);
      CHECK(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
      if(poolElementNode->TimerTimeStamp <= currentTimeStamp) {
         ST_CLASS(poolNamespaceManagementDeregisterPoolElementByPtr)(
            poolNamespaceManagement,
            poolElementNode);
         purgedPoolElements++;
      }
      else {
         /* No more relevant entries, since time list is sorted! */
         break;
      }

      poolElementNode = nextPoolElementNode;
   }

   return(purgedPoolElements);
}
