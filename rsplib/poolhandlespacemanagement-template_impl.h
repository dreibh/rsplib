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


/*
#define PRINT_SELECTION_RESULT
*/
/*
#define PRINT_GETNAMETABLE_RESULT
*/


/* ###### Initialize ##################################################### */
void ST_CLASS(poolHandlespaceManagementNew)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        const RegistrarIdentifierType               homeRegistrarIdentifier,
        void (*poolNodeUserDataDisposer)(struct ST_CLASS(PoolNode)* poolElementNode,
                                         void*                      userData),
        void (*poolElementNodeUserDataDisposer)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                                void*                             userData),
        void* disposerUserData)
{
   ST_CLASS(poolHandlespaceNodeNew)(&poolHandlespaceManagement->Handlespace, homeRegistrarIdentifier);
   poolHandlespaceManagement->NewPoolNode                     = NULL;
   poolHandlespaceManagement->NewPoolElementNode              = NULL;
   poolHandlespaceManagement->PoolNodeUserDataDisposer        = poolNodeUserDataDisposer;
   poolHandlespaceManagement->PoolElementNodeUserDataDisposer = poolElementNodeUserDataDisposer;
   poolHandlespaceManagement->DisposerUserData                = disposerUserData;
}


/* ###### PoolElementNode deallocation helper ############################ */
static void ST_CLASS(poolHandlespaceManagementPoolElementNodeDisposer)(void* arg1,
                                                                       void* arg2)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode                 = (struct ST_CLASS(PoolElementNode)*)arg1;
   struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement = (struct ST_CLASS(PoolHandlespaceManagement)*)arg2;
   if((poolElementNode->UserData) && (poolHandlespaceManagement->PoolElementNodeUserDataDisposer))  {
      poolHandlespaceManagement->PoolElementNodeUserDataDisposer(poolElementNode,
                                                               poolHandlespaceManagement->DisposerUserData);
      poolElementNode->UserData = NULL;
   }
   transportAddressBlockDelete(poolElementNode->UserTransport);
   free(poolElementNode->UserTransport);
   poolElementNode->UserTransport = NULL;
   free(poolElementNode);
}


/* ###### PoolNode deallocation helper ################################### */
static void ST_CLASS(poolHandlespaceManagementPoolNodeDisposer)(void* arg1,
                                                                void* arg2)
{
   struct ST_CLASS(PoolNode)* poolNode                                   = (struct ST_CLASS(PoolNode)*)arg1;
   struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement = (struct ST_CLASS(PoolHandlespaceManagement)*)arg2;
   if((poolNode->UserData) && (poolHandlespaceManagement->PoolNodeUserDataDisposer))  {
      poolHandlespaceManagement->PoolNodeUserDataDisposer(poolNode,
                                                          poolHandlespaceManagement->DisposerUserData);
      poolNode->UserData = NULL;
   }
   free(poolNode);
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolHandlespaceManagementDelete)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   ST_CLASS(poolHandlespaceManagementClear)(poolHandlespaceManagement);
   ST_CLASS(poolHandlespaceNodeDelete)(&poolHandlespaceManagement->Handlespace);
   if(poolHandlespaceManagement->NewPoolNode) {
      free(poolHandlespaceManagement->NewPoolNode);
      poolHandlespaceManagement->NewPoolNode = NULL;
   }
   if(poolHandlespaceManagement->NewPoolElementNode) {
      free(poolHandlespaceManagement->NewPoolElementNode);
      poolHandlespaceManagement->NewPoolElementNode = NULL;
   }
}


/* ###### Clear handlespace ################################################ */
void ST_CLASS(poolHandlespaceManagementClear)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   ST_CLASS(poolHandlespaceNodeClear)(&poolHandlespaceManagement->Handlespace,
                                      ST_CLASS(poolHandlespaceManagementPoolNodeDisposer),
                                      ST_CLASS(poolHandlespaceManagementPoolElementNodeDisposer),
                                      (void*)poolHandlespaceManagement);
}


/* ###### Registration ################################################### */
unsigned int ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                const RegistrarIdentifierType               homeRegistrarIdentifier,
                const PoolElementIdentifierType             poolElementIdentifier,
                const unsigned int                          registrationLife,
                const struct PoolPolicySettings*            poolPolicySettings,
                const struct TransportAddressBlock*         userTransport,
                const struct TransportAddressBlock*         registratorTransport,
                const int                                   connectionSocketDescriptor,
                const sctp_assoc_t                          connectionAssocID,
                const unsigned long long                    currentTimeStamp,
                struct ST_CLASS(PoolElementNode)**          poolElementNode)
{
   const struct ST_CLASS(PoolPolicy)*  poolPolicy;
   struct TransportAddressBlock*       userTransportCopy;
   struct TransportAddressBlock*       registratorTransportCopy;
   unsigned int                        errorCode;

   *poolElementNode = 0;
   if((poolHandle->Size < 1) || (poolHandle->Size > MAX_POOLHANDLESIZE)) {
      return(RSPERR_INVALID_POOL_HANDLE);
   }
   poolPolicy = ST_CLASS(poolPolicyGetPoolPolicyByType)(poolPolicySettings->PolicyType);
   if(poolPolicy == NULL) {
      return(RSPERR_INVALID_POOL_POLICY);
   }
   if(poolHandlespaceManagement->NewPoolNode == NULL) {
      poolHandlespaceManagement->NewPoolNode = (struct ST_CLASS(PoolNode)*)malloc(sizeof(struct ST_CLASS(PoolNode)));
      if(poolHandlespaceManagement->NewPoolNode == NULL) {
         return(RSPERR_NO_RESOURCES);
      }
   }
   ST_CLASS(poolNodeNew)(poolHandlespaceManagement->NewPoolNode,
                         poolHandle, poolPolicy,
                         userTransport->Protocol,
                         (userTransport->Flags & TABF_CONTROLCHANNEL) ? PNF_CONTROLCHANNEL : 0);

   if(poolHandlespaceManagement->NewPoolElementNode == NULL) {
      poolHandlespaceManagement->NewPoolElementNode = (struct ST_CLASS(PoolElementNode)*)malloc(sizeof(struct ST_CLASS(PoolElementNode)));
      if(poolHandlespaceManagement->NewPoolElementNode == NULL) {
         return(RSPERR_NO_RESOURCES);
      }
   }

   /*
      Attention:
      userTransport is given as TransportAddressBlock here, since
      poolHandlespaceNodeAddOrUpdatePoolElementNode must be able to check it
      for compatibility to the pool.
      If the node is inserted as new pool element node, the AddressBlock field
      *must* be updated with a duplicate of userTransport's data!

      The same is necessary for registratorTransport.
    */
   ST_CLASS(poolElementNodeNew)(poolHandlespaceManagement->NewPoolElementNode,
                                poolElementIdentifier,
                                homeRegistrarIdentifier,
                                registrationLife,
                                poolPolicySettings,
                                (struct TransportAddressBlock*)userTransport,
                                (struct TransportAddressBlock*)registratorTransport,
                                connectionSocketDescriptor,
                                connectionAssocID);
   *poolElementNode = ST_CLASS(poolHandlespaceNodeAddOrUpdatePoolElementNode)(&poolHandlespaceManagement->Handlespace,
                                                                              &poolHandlespaceManagement->NewPoolNode,
                                                                              &poolHandlespaceManagement->NewPoolElementNode,
                                                                              &errorCode);
   if(errorCode == RSPERR_OKAY) {
      (*poolElementNode)->LastUpdateTimeStamp = currentTimeStamp;

      userTransportCopy        = transportAddressBlockDuplicate(userTransport);
      registratorTransportCopy = transportAddressBlockDuplicate(registratorTransport);

      if((userTransportCopy != NULL) &&
         ((registratorTransportCopy != NULL) || (registratorTransport == NULL))) {
         if((*poolElementNode)->UserTransport != userTransport) {   /* see comment above! */
            transportAddressBlockDelete((*poolElementNode)->UserTransport);
            free((*poolElementNode)->UserTransport);
         }
         (*poolElementNode)->UserTransport = userTransportCopy;

         if(((*poolElementNode)->RegistratorTransport != registratorTransport) &&
            ((*poolElementNode)->RegistratorTransport != NULL)) {   /* see comment above! */
            transportAddressBlockDelete((*poolElementNode)->RegistratorTransport);
            free((*poolElementNode)->RegistratorTransport);
         }
         (*poolElementNode)->RegistratorTransport = registratorTransportCopy;
      }
      else {
         if(userTransportCopy) {
            transportAddressBlockDelete(userTransportCopy);
            free(userTransportCopy);
         }
         if(registratorTransportCopy) {
            transportAddressBlockDelete(registratorTransportCopy);
            free(registratorTransportCopy);
         }
         ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
            poolHandlespaceManagement,
            poolHandle,
            poolElementIdentifier);
         *poolElementNode = NULL;
         errorCode = RSPERR_NO_RESOURCES;
      }
   }

#ifdef VERIFY
   ST_CLASS(poolHandlespaceNodeVerify)(&poolHandlespaceManagement->Handlespace);
#endif
   return(errorCode);
}


/* ###### Print handlespace content ######################################## */
void ST_CLASS(poolHandlespaceManagementPrint)(
              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
              FILE*                                       fd,
              const unsigned int                          fields)
{
   ST_CLASS(poolHandlespaceNodePrint)(&poolHandlespaceManagement->Handlespace, fd, fields);
}


/* ###### Verify structures ############################################## */
void ST_CLASS(poolHandlespaceManagementVerify)(
              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   ST_CLASS(poolHandlespaceNodeVerify)(&poolHandlespaceManagement->Handlespace);
}


/* ###### Get number of pools ############################################ */
size_t ST_CLASS(poolHandlespaceManagementGetPools)(
          const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(ST_CLASS(poolHandlespaceNodeGetPoolNodes)(&poolHandlespaceManagement->Handlespace));
}


/* ###### Get number of pool elements #################################### */
size_t ST_CLASS(poolHandlespaceManagementGetPoolElements)(
          const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(ST_CLASS(poolHandlespaceNodeGetPoolElementNodes)(&poolHandlespaceManagement->Handlespace));
}


/* ###### Get number of pool elements of given pool ###################### */
size_t ST_CLASS(poolHandlespaceManagementGetPoolElementsOfPool)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const struct PoolHandle*                    poolHandle)
{
   return(ST_CLASS(poolHandlespaceNodeGetPoolElementNodesOfPool)(
             &poolHandlespaceManagement->Handlespace,
             poolHandle));
}


/* ###### Get number of pool elements of given connection ################ */
size_t ST_CLASS(poolHandlespaceManagementGetPoolElementsOfConnection)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const int                                   socketDescriptor,
          const sctp_assoc_t                          assocID)
{
   return(ST_CLASS(poolHandlespaceNodeGetConnectionNodesForConnection)(
             &poolHandlespaceManagement->Handlespace,
             socketDescriptor, assocID));
}


/* ###### Registration ################################################### */
unsigned int ST_CLASS(poolHandlespaceManagementRegisterPoolElementByPtr)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                const struct ST_CLASS(PoolElementNode)*     originalPoolElementNode,
                const unsigned long long                    currentTimeStamp,
                struct ST_CLASS(PoolElementNode)**          poolElementNode)
{
   return(ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
             poolHandlespaceManagement,
             poolHandle,
             originalPoolElementNode->HomeRegistrarIdentifier,
             originalPoolElementNode->Identifier,
             originalPoolElementNode->RegistrationLife,
             &originalPoolElementNode->PolicySettings,
             originalPoolElementNode->UserTransport,
             originalPoolElementNode->RegistratorTransport,
             originalPoolElementNode->ConnectionSocketDescriptor,
             originalPoolElementNode->ConnectionAssocID,
             currentTimeStamp,
             poolElementNode));
}


/* ###### Get next timer time stamp ###################################### */
unsigned long long ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                      struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   const struct ST_CLASS(PoolElementNode)* nextTimer =
      ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)(
         &poolHandlespaceManagement->Handlespace);
   if(nextTimer != NULL) {
      return(nextTimer->TimerTimeStamp);
   }
   return(~0);
}


/* ###### Find pool element ############################################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     const struct PoolHandle*                    poolHandle,
                                     const PoolElementIdentifierType             poolElementIdentifier)
{
   return(ST_CLASS(poolHandlespaceNodeFindPoolElementNode)(
             &poolHandlespaceManagement->Handlespace,
             poolHandle,
             poolElementIdentifier));
}


/* ###### Deregistration ################################################# */
unsigned int ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                struct ST_CLASS(PoolElementNode)*           poolElementNode)
{
   struct ST_CLASS(PoolNode)* poolNode = poolElementNode->OwnerPoolNode;

   ST_CLASS(poolHandlespaceNodeRemovePoolElementNode)(&poolHandlespaceManagement->Handlespace, poolElementNode);
   ST_CLASS(poolElementNodeDelete)(poolElementNode);
   ST_CLASS(poolHandlespaceManagementPoolElementNodeDisposer)(poolElementNode, poolHandlespaceManagement);

   if(ST_CLASS(poolNodeGetPoolElementNodes)(poolNode) == 0) {
      ST_CLASS(poolHandlespaceNodeRemovePoolNode)(
         &poolHandlespaceManagement->Handlespace, poolNode);
      ST_CLASS(poolNodeDelete)(poolNode);
      ST_CLASS(poolHandlespaceManagementPoolNodeDisposer)(poolNode, poolHandlespaceManagement);
   }
#ifdef VERIFY
      ST_CLASS(poolHandlespaceNodeVerify)(&poolHandlespaceManagement->Handlespace);
#endif
   return(RSPERR_OKAY);
}


/* ###### Deregistration ################################################# */
unsigned int ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                const PoolElementIdentifierType             poolElementIdentifier)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode = ST_CLASS(poolHandlespaceNodeFindPoolElementNode)(
                                                          &poolHandlespaceManagement->Handlespace,
                                                          poolHandle,
                                                          poolElementIdentifier);
   if(poolElementNode) {
      return(ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                poolHandlespaceManagement,
                poolElementNode));
   }
   return(RSPERR_NOT_FOUND);
}


/* ###### Name Resolution ################################################ */
unsigned int ST_CLASS(poolHandlespaceManagementNameResolution)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                struct ST_CLASS(PoolElementNode)**          poolElementNodeArray,
                size_t*                                     poolElementNodes,
                const size_t                                maxNameResolutionItems,
                const size_t                                maxIncrement)
{
   unsigned int errorCode;
   *poolElementNodes = ST_CLASS(poolHandlespaceNodeSelectPoolElementNodesByPolicy)(
                          &poolHandlespaceManagement->Handlespace,
                          poolHandle,
                          poolElementNodeArray,
                          maxNameResolutionItems, maxIncrement,
                          &errorCode);
#ifdef VERIFY
   ST_CLASS(poolHandlespaceNodeVerify)(&poolHandlespaceManagement->Handlespace);
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


/* ###### Get name table from handlespace ################################## */
static int ST_CLASS(getOwnershipNameTable)(
              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
              struct ST_CLASS(NameTableExtract)*          nameTableExtract,
              const RegistrarIdentifierType               homeRegistrarIdentifier,
              const unsigned int                          flags)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   if(flags & NTEF_START) {
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                           &poolHandlespaceManagement->Handlespace,
                           homeRegistrarIdentifier);
   }
   else {
      poolElementNode = ST_CLASS(poolHandlespaceNodeFindNearestNextPoolElementOwnershipNode)(
                           &poolHandlespaceManagement->Handlespace,
                           homeRegistrarIdentifier,
                           &nameTableExtract->LastPoolHandle,
                           nameTableExtract->LastPoolElementIdentifier);
   }

   nameTableExtract->PoolElementNodes = 0;
   while(poolElementNode != NULL) {
      nameTableExtract->PoolElementNodeArray[nameTableExtract->PoolElementNodes++] = poolElementNode;
      if(nameTableExtract->PoolElementNodes >= NTE_MAX_POOL_ELEMENT_NODES) {
         break;
      }
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                           &poolHandlespaceManagement->Handlespace, poolElementNode);
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


/* ###### Get name table from handlespace ################################## */
static int ST_CLASS(getGlobalNameTable)(struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                        struct ST_CLASS(NameTableExtract)*          nameTableExtract,
                                        const unsigned int                          flags)
{
   struct ST_CLASS(PoolNode)*        poolNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   nameTableExtract->PoolElementNodes = 0;
   if(flags & NTEF_START) {
      nameTableExtract->LastPoolElementIdentifier = 0;
      nameTableExtract->LastPoolHandle.Size       = 0;
      poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(&poolHandlespaceManagement->Handlespace);
      if(poolNode == NULL) {
         return(0);
      }
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
   }
   else {
      poolNode = ST_CLASS(poolHandlespaceNodeFindPoolNode)(
                    &poolHandlespaceManagement->Handlespace,
                    &nameTableExtract->LastPoolHandle);
      if(poolNode == NULL) {
         poolNode = ST_CLASS(poolHandlespaceNodeFindNearestNextPoolNode)(
                       &poolHandlespaceManagement->Handlespace,
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
      poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(
                    &poolHandlespaceManagement->Handlespace, poolNode);
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


/* ###### Get name table from handlespace ################################## */
int ST_CLASS(poolHandlespaceManagementGetNameTable)(
       struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
       const RegistrarIdentifierType               homeRegistrarIdentifier,
       struct ST_CLASS(NameTableExtract)*          nameTableExtract,
       const unsigned int                          flags)
{
   if(flags & NTEF_OWNCHILDSONLY) {
      return(ST_CLASS(getOwnershipNameTable)(
                poolHandlespaceManagement, nameTableExtract,
                homeRegistrarIdentifier,
                flags));
   }
   return(ST_CLASS(getGlobalNameTable)(
             poolHandlespaceManagement, nameTableExtract, flags));
}


/* ###### Restart PE expiry timer to last update TS + expiry timeout ##### */
void ST_CLASS(poolHandlespaceManagementRestartPoolElementExpiryTimer)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        struct ST_CLASS(PoolElementNode)*           poolElementNode,
        const unsigned long long                    expiryTimeout)
{
   ST_CLASS(poolHandlespaceNodeDeactivateTimer)(&poolHandlespaceManagement->Handlespace,
                                                poolElementNode);
   ST_CLASS(poolHandlespaceNodeActivateTimer)(&poolHandlespaceManagement->Handlespace,
                                            poolElementNode,
                                            PENT_EXPIRY,
                                            poolElementNode->LastUpdateTimeStamp + expiryTimeout);
}


/* ###### Purge handlespace from expired PE entries ######################## */
size_t ST_CLASS(poolHandlespaceManagementPurgeExpiredPoolElements)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const unsigned long long                    currentTimeStamp)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   size_t                            purgedPoolElements = 0;

   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)(&poolHandlespaceManagement->Handlespace);
   while(poolElementNode != NULL) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementTimerNode)(&poolHandlespaceManagement->Handlespace, poolElementNode);

      CHECK(poolElementNode->TimerCode == PENT_EXPIRY);
      CHECK(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
      if(poolElementNode->TimerTimeStamp <= currentTimeStamp) {
         ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
            poolHandlespaceManagement,
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