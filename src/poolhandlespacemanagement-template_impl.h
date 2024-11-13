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
 * Copyright (C) 2003-2025 by Thomas Dreibholz
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

/*
#define PRINT_SELECTION_RESULT
*/
/*
#define PRINT_GETNAMETABLE_RESULT
*/


/* ###### Helper function for update notification ######################## */
static void ST_CLASS(poolNodeUpdateNotification)(
   struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
   struct ST_CLASS(PoolElementNode)*     poolElementNode,
   enum PoolNodeUpdateAction             updateAction,
   HandlespaceChecksumAccumulatorType    preUpdateChecksum,
   RegistrarIdentifierType               preUpdateHomeRegistrar,
   void*                                 userData)
{
   struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement = (struct ST_CLASS(PoolHandlespaceManagement)*)userData;
   if(poolHandlespaceManagement->PoolNodeUpdateNotification) {
      poolHandlespaceManagement->PoolNodeUpdateNotification(poolHandlespaceManagement,
                                                            poolElementNode,
                                                            updateAction,
                                                            preUpdateChecksum,
                                                            preUpdateHomeRegistrar,
                                                            poolHandlespaceManagement->NotificationUserData);
   }
}


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
   ST_CLASS(poolHandlespaceNodeNew)(&poolHandlespaceManagement->Handlespace, homeRegistrarIdentifier,
                                    ST_CLASS(poolNodeUpdateNotification),
                                    (void*)poolHandlespaceManagement);
   poolHandlespaceManagement->NewPoolNode                     = NULL;
   poolHandlespaceManagement->NewPoolElementNode              = NULL;
   poolHandlespaceManagement->PoolNodeUserDataDisposer        = poolNodeUserDataDisposer;
   poolHandlespaceManagement->PoolElementNodeUserDataDisposer = poolElementNodeUserDataDisposer;
   poolHandlespaceManagement->DisposerUserData                = disposerUserData;
   poolHandlespaceManagement->PoolNodeUpdateNotification      = NULL;
   poolHandlespaceManagement->NotificationUserData            = NULL;
}


/* ###### PoolElementNode deallocation helper ############################ */
static void ST_CLASS(poolHandlespaceManagementPoolElementNodeDisposer)(void* arg1,
                                                                       void* arg2)
{
   struct ST_CLASS(PoolElementNode)*           poolElementNode           = (struct ST_CLASS(PoolElementNode)*)arg1;
   struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement = (struct ST_CLASS(PoolHandlespaceManagement)*)arg2;
   if((poolElementNode->UserData) && (poolHandlespaceManagement->PoolElementNodeUserDataDisposer))  {
      poolHandlespaceManagement->PoolElementNodeUserDataDisposer(poolElementNode,
                                                                 poolHandlespaceManagement->DisposerUserData);
      poolElementNode->UserData = NULL;
   }
   transportAddressBlockDelete(poolElementNode->UserTransport);
   free(poolElementNode->UserTransport);
   poolElementNode->UserTransport = NULL;
   if(poolElementNode->RegistratorTransport) {
      transportAddressBlockDelete(poolElementNode->RegistratorTransport);
      free(poolElementNode->RegistratorTransport);
      poolElementNode->RegistratorTransport = NULL;
   }
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


/* ###### Get handlespace checksum ####################################### */
HandlespaceChecksumType ST_CLASS(poolHandlespaceManagementGetHandlespaceChecksum)(
                           const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(handlespaceChecksumFinish(ST_CLASS(poolHandlespaceNodeGetHandlespaceChecksum)(&poolHandlespaceManagement->Handlespace)));
}


/* ###### Get ownership checksum ######################################### */
HandlespaceChecksumType ST_CLASS(poolHandlespaceManagementGetOwnershipChecksum)(
                           const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(handlespaceChecksumFinish(ST_CLASS(poolHandlespaceNodeGetOwnershipChecksum)(&poolHandlespaceManagement->Handlespace)));
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
         return(RSPERR_OUT_OF_MEMORY);
      }
   }
   ST_CLASS(poolNodeNew)(poolHandlespaceManagement->NewPoolNode,
                         poolHandle, poolPolicy,
                         userTransport->Protocol,
                         (userTransport->Flags & TABF_CONTROLCHANNEL) ? PNF_CONTROLCHANNEL : 0);

   if(poolHandlespaceManagement->NewPoolElementNode == NULL) {
      poolHandlespaceManagement->NewPoolElementNode = (struct ST_CLASS(PoolElementNode)*)malloc(sizeof(struct ST_CLASS(PoolElementNode)));
      if(poolHandlespaceManagement->NewPoolElementNode == NULL) {
         return(RSPERR_OUT_OF_MEMORY);
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
         errorCode = RSPERR_OUT_OF_MEMORY;
      }
   }

#ifdef VERIFY
#warning VERIFY is on! The Handlespace Management will be very slow!
   ST_CLASS(poolHandlespaceNodeVerify)(&poolHandlespaceManagement->Handlespace);
#endif
   return(errorCode);
}


/* ###### Get textual description ######################################## */
void ST_CLASS(poolHandlespaceManagementGetDescription)(
        const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        char*                                             buffer,
        const size_t                                      bufferSize)
{
   ST_CLASS(poolHandlespaceNodeGetDescription)(&poolHandlespaceManagement->Handlespace, buffer, bufferSize);
}


/* ###### Print handlespace content ###################################### */
void ST_CLASS(poolHandlespaceManagementPrint)(
              const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
              FILE*                                             fd,
              const unsigned int                                fields)
{
   ST_CLASS(poolHandlespaceNodePrint)(&poolHandlespaceManagement->Handlespace, fd, fields);
}


/* ###### Verify structures ############################################## */
void ST_CLASS(poolHandlespaceManagementVerify)(
              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   ST_CLASS(poolHandlespaceNodeVerify)(&poolHandlespaceManagement->Handlespace);
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
#warning VERIFY is on! The Handlespace Management will be very slow!
      ST_CLASS(poolHandlespaceNodeVerify)(&poolHandlespaceManagement->Handlespace);
#endif
   return(RSPERR_OKAY);
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


/* ###### Update PoolElementNode's ownership ############################# */
void ST_CLASS(poolHandlespaceManagementUpdateOwnershipOfPoolElementNode)(
              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
              struct ST_CLASS(PoolElementNode)*           poolElementNode,
              const RegistrarIdentifierType               newHomeRegistrarIdentifier)
{
   return(ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
             &poolHandlespaceManagement->Handlespace,
             poolElementNode,
             newHomeRegistrarIdentifier));
}


/* ###### Update PoolElementNode's connection ############################ */
void ST_CLASS(poolHandlespaceManagementUpdateConnectionOfPoolElementNode)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        struct ST_CLASS(PoolElementNode)*           poolElementNode,
        const int                                   connectionSocketDescriptor,
        const sctp_assoc_t                          connectionAssocID)
{
   return(ST_CLASS(poolHandlespaceNodeUpdateConnectionOfPoolElementNode)(
             &poolHandlespaceManagement->Handlespace,
             poolElementNode,
             connectionSocketDescriptor,
             connectionAssocID));
}


/* ###### Handle Resolution ############################################## */
unsigned int ST_CLASS(poolHandlespaceManagementHandleResolution)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                struct ST_CLASS(PoolElementNode)**          poolElementNodeArray,
                size_t*                                     poolElementNodes,
                const size_t                                maxHandleResolutionItems,
                const size_t                                maxIncrement)
{
   unsigned int errorCode;
   *poolElementNodes = ST_CLASS(poolHandlespaceNodeSelectPoolElementNodesByPolicy)(
                          &poolHandlespaceManagement->Handlespace,
                          poolHandle,
                          poolElementNodeArray,
                          maxHandleResolutionItems, maxIncrement,
                          &errorCode);
#ifdef VERIFY
#warning VERIFY is on! The Handlespace Management will be very slow!
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
static int ST_CLASS(getOwnershipHandleTable)(
              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
              struct ST_CLASS(HandleTableExtract)*        handleTableExtract,
              const RegistrarIdentifierType               homeRegistrarIdentifier,
              const unsigned int                          flags,
              size_t                                      maxElements)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
#ifdef PRINT_GETNAMETABLE_RESULT
   size_t                            i;
#endif

   if(maxElements > NTE_MAX_POOL_ELEMENT_NODES) {
      maxElements = NTE_MAX_POOL_ELEMENT_NODES;
   }
   else if(maxElements < 1) {
      return(0);
   }
   if(flags & HTEF_START) {
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                           &poolHandlespaceManagement->Handlespace,
                           homeRegistrarIdentifier);
   }
   else {
      poolElementNode = ST_CLASS(poolHandlespaceNodeFindNearestNextPoolElementOwnershipNode)(
                           &poolHandlespaceManagement->Handlespace,
                           homeRegistrarIdentifier,
                           &handleTableExtract->LastPoolHandle,
                           handleTableExtract->LastPoolElementIdentifier);
   }

   handleTableExtract->PoolElementNodes = 0;
   while(poolElementNode != NULL) {
      handleTableExtract->PoolElementNodeArray[handleTableExtract->PoolElementNodes++] = poolElementNode;
      if(handleTableExtract->PoolElementNodes >= maxElements) {
         break;
      }
      poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                           &poolHandlespaceManagement->Handlespace, poolElementNode);
   }

   if(handleTableExtract->PoolElementNodes > 0) {
      struct ST_CLASS(PoolElementNode)* lastPoolElementNode = handleTableExtract->PoolElementNodeArray[handleTableExtract->PoolElementNodes - 1];
      struct ST_CLASS(PoolNode)* lastPoolNode               = lastPoolElementNode->OwnerPoolNode;
      handleTableExtract->LastPoolHandle                    = lastPoolNode->Handle;
      handleTableExtract->LastPoolElementIdentifier         = lastPoolElementNode->Identifier;
   }

#ifdef PRINT_GETNAMETABLE_RESULT
   printf("GetOwnershipHandleTable result: %u items\n", (unsigned int)handleTableExtract->PoolElementNodes);
   for(i = 0;i < handleTableExtract->PoolElementNodes;i++) {
      poolElementNode = handleTableExtract->PoolElementNodeArray[i];
      printf("#%3d: \"", (int)i);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdout);
      printf("\"/%d, $%08x", (int)poolElementNode->OwnerPoolNode->Handle.Size, poolElementNode->Identifier);
      puts("");
   }
#endif
   return(handleTableExtract->PoolElementNodes > 0);
}


/* ###### Get name table from handlespace ################################## */
static int ST_CLASS(getGlobalHandleTable)(struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                          struct ST_CLASS(HandleTableExtract)*        handleTableExtract,
                                          const unsigned int                          flags,
                                          size_t                                      maxElements)
{
   struct ST_CLASS(PoolNode)*        poolNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
#ifdef PRINT_GETNAMETABLE_RESULT
   size_t                            i;
#endif

   if(maxElements > NTE_MAX_POOL_ELEMENT_NODES) {
      maxElements = NTE_MAX_POOL_ELEMENT_NODES;
   }
   else if(maxElements < 1) {
      return(0);
   }
   handleTableExtract->PoolElementNodes = 0;
   if(flags & HTEF_START) {
      handleTableExtract->LastPoolElementIdentifier = 0;
      handleTableExtract->LastPoolHandle.Size       = 0;
      poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(&poolHandlespaceManagement->Handlespace);
      if(poolNode == NULL) {
         return(0);
      }
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
   }
   else {
      poolNode = ST_CLASS(poolHandlespaceNodeFindPoolNode)(
                    &poolHandlespaceManagement->Handlespace,
                    &handleTableExtract->LastPoolHandle);
      if(poolNode == NULL) {
         poolNode = ST_CLASS(poolHandlespaceNodeFindNearestNextPoolNode)(
                       &poolHandlespaceManagement->Handlespace,
                       &handleTableExtract->LastPoolHandle);
      }
      if(poolNode == NULL) {
         return(0);
      }
      if(poolHandleComparison(&handleTableExtract->LastPoolHandle,
                              &poolNode->Handle) == 0) {
         poolElementNode = ST_CLASS(poolNodeFindNearestNextPoolElementNode)(poolNode, handleTableExtract->LastPoolElementIdentifier);
      }
      else {
         poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
      }
   }


   for(;;) {
      while(poolElementNode != NULL) {
         handleTableExtract->PoolElementNodeArray[handleTableExtract->PoolElementNodes++] = poolElementNode;
         if(handleTableExtract->PoolElementNodes >= maxElements) {
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
   if(handleTableExtract->PoolElementNodes > 0) {
      struct ST_CLASS(PoolElementNode)* lastPoolElementNode = handleTableExtract->PoolElementNodeArray[handleTableExtract->PoolElementNodes - 1];
      struct ST_CLASS(PoolNode)* lastPoolNode               = lastPoolElementNode->OwnerPoolNode;
      handleTableExtract->LastPoolHandle            = lastPoolNode->Handle;
      handleTableExtract->LastPoolElementIdentifier = lastPoolElementNode->Identifier;
   }


#ifdef PRINT_GETNAMETABLE_RESULT
   printf("GetGlobalHandleTable result: %u items\n", (unsigned int)handleTableExtract->PoolElementNodes);
   for(i = 0;i < handleTableExtract->PoolElementNodes;i++) {
      poolElementNode = handleTableExtract->PoolElementNodeArray[i];
      printf("#%3d: \"", (int)i);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdout);
      printf("\"/%d, $%08x", (int)poolElementNode->OwnerPoolNode->Handle.Size,
                             poolElementNode->Identifier);
      puts("");
   }
#endif
   return(handleTableExtract->PoolElementNodes > 0);
}


/* ###### Get name table from handlespace ################################## */
int ST_CLASS(poolHandlespaceManagementGetHandleTable)(
       struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
       const RegistrarIdentifierType               homeRegistrarIdentifier,
       struct ST_CLASS(HandleTableExtract)*        handleTableExtract,
       const unsigned int                          flags,
       size_t                                      maxElements)
{
   if(flags & HTEF_OWNCHILDSONLY) {
      return(ST_CLASS(getOwnershipHandleTable)(
                poolHandlespaceManagement, handleTableExtract,
                homeRegistrarIdentifier,
                flags, maxElements));
   }
   return(ST_CLASS(getGlobalHandleTable)(
             poolHandlespaceManagement, handleTableExtract, flags, maxElements));
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


/* ###### Mark pool element nodes owned by given PR ###################### */
void ST_CLASS(poolHandlespaceManagementMarkPoolElementNodes)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        const RegistrarIdentifierType ownerID)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;

   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &poolHandlespaceManagement->Handlespace, ownerID);
   while(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                               &poolHandlespaceManagement->Handlespace, poolElementNode);
      poolElementNode->Flags |= PENF_MARKED;
      poolElementNode = nextPoolElementNode;
   }
}


/* ###### Purge marked pool element nodes owned by given PR ############## */
size_t ST_CLASS(poolHandlespaceManagementPurgeMarkedPoolElementNodes)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const RegistrarIdentifierType ownerID)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   size_t                            count = 0;

   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &poolHandlespaceManagement->Handlespace, ownerID);
   while(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                               &poolHandlespaceManagement->Handlespace, poolElementNode);
      if(poolElementNode->Flags & PENF_MARKED) {
         CHECK(ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                  poolHandlespaceManagement, poolElementNode) == RSPERR_OKAY);
         count++;
      }
      poolElementNode = nextPoolElementNode;
   }
   return(count);
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


/* ###### Get number of owned pool elements ############################## */
size_t ST_CLASS(poolHandlespaceManagementGetOwnedPoolElements)(
          const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(ST_CLASS(poolHandlespaceNodeGetOwnedPoolElementNodes)(&poolHandlespaceManagement->Handlespace));
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


/* ###### Get first PoolNode ############################################# */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolNode)(
                              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(&poolHandlespaceManagement->Handlespace));
}


/* ###### Get next PoolNode ############################################## */
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolNode)(
                              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                              struct ST_CLASS(PoolNode)*                  poolNode)
{
   return(ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(&poolHandlespaceManagement->Handlespace,
                                                       poolNode));
}


/* ###### Get first timer ################################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)(&poolHandlespaceManagement->Handlespace));
}


/* ###### Get previous timer ############################################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode)
{
   return(ST_CLASS(poolHandlespaceNodeGetNextPoolElementTimerNode)(&poolHandlespaceManagement->Handlespace,
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


/* ###### Get first connection ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNode)(&poolHandlespaceManagement->Handlespace));
}


/* ###### Get previous connection ######################################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode)
{
   return(ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNode)(&poolHandlespaceManagement->Handlespace,
                                                                        poolElementNode));
}


/* ###### Get first ownership node of given home PR identifier ########### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     const RegistrarIdentifierType               homeRegistrarIdentifier)
{
   return(ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(&poolHandlespaceManagement->Handlespace,
                                                                                     homeRegistrarIdentifier));
}


/* ###### Get next connection of same home PR identifier ################# */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementConnectionNodeForSameConnection)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode)
{
   return(ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(&poolHandlespaceManagement->Handlespace,
                                                                                         poolElementNode));
}


/* ###### Get first ownership ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   return(ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(&poolHandlespaceManagement->Handlespace));
}


/* ###### Get previous ownership ######################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode)
{
   return(ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(&poolHandlespaceManagement->Handlespace,
                                                                       poolElementNode));
}


/* ###### Get next ownership of same home PR identifier ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementOwnershipNodeForSameIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode)
{
   return(ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(&poolHandlespaceManagement->Handlespace,
                                                                                        poolElementNode));
}
