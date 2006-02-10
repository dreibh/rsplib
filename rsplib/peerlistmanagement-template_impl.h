/*
 * An Efficient RSerPool Pool List Management Implementation
 * Copyright (C) 2004-2006 by Thomas Dreibholz
 *
 * $Id$
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


static void ST_CLASS(peerListManagementPoolNodeUpdateNotification)(
               struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
               struct ST_CLASS(PoolElementNode)*           poolElementNode,
               enum PoolNodeUpdateAction                   updateAction,
               HandlespaceChecksumAccumulatorType          preUpdateChecksum,
               RegistrarIdentifierType                     preUpdateHomeRegistrar,
               void*                                       userData);


/* ###### Initialize ##################################################### */
void ST_CLASS(peerListManagementNew)(
        struct ST_CLASS(PeerListManagement)*        peerListManagement,
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        const RegistrarIdentifierType               ownRegistrarIdentifier,
        void (*peerListNodeUserDataDisposer)(struct ST_CLASS(PeerListNode)* peerListNode,
                                             void*                          userData),
        void* disposerUserData)
{
   ST_CLASS(peerListNew)(&peerListManagement->List, ownRegistrarIdentifier);
   peerListManagement->NewPeerListNode              = NULL;
   peerListManagement->Handlespace                  = poolHandlespaceManagement;
   peerListManagement->PeerListNodeUserDataDisposer = peerListNodeUserDataDisposer;
   peerListManagement->DisposerUserData             = disposerUserData;

   if(peerListManagement->Handlespace) {
      peerListManagement->Handlespace->PoolNodeUpdateNotification = ST_CLASS(peerListManagementPoolNodeUpdateNotification);
      peerListManagement->Handlespace->NotificationUserData       = (void*)peerListManagement;
   }
}


/* ###### PeerListNode deallocation helper ############################ */
static void ST_CLASS(peerListManagementPeerListNodeDisposer)(void* arg1,
                                                             void* arg2)
{
   struct ST_CLASS(PeerListNode)*       peerListNode       = (struct ST_CLASS(PeerListNode)*)arg1;
   struct ST_CLASS(PeerListManagement)* peerListManagement = (struct ST_CLASS(PeerListManagement)*)arg2;
   if((peerListNode->UserData) && (peerListManagement->PeerListNodeUserDataDisposer))  {
      peerListManagement->PeerListNodeUserDataDisposer(peerListNode,
                                                       peerListManagement->DisposerUserData);
      peerListNode->UserData = NULL;
   }
   transportAddressBlockDelete(peerListNode->AddressBlock);
   free(peerListNode->AddressBlock);
   peerListNode->AddressBlock = NULL;
   free(peerListNode);
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(peerListManagementDelete)(
        struct ST_CLASS(PeerListManagement)* peerListManagement)
{
   if(peerListManagement->Handlespace) {
      peerListManagement->Handlespace->PoolNodeUpdateNotification = NULL;
      peerListManagement->Handlespace->NotificationUserData       = NULL;
   }
   peerListManagement->Handlespace = NULL;

   ST_CLASS(peerListManagementClear)(peerListManagement);
   if(peerListManagement->NewPeerListNode) {
      ST_CLASS(peerListNodeDelete)(peerListManagement->NewPeerListNode);
      free(peerListManagement->NewPeerListNode);
      peerListManagement->NewPeerListNode = NULL;
   }
   ST_CLASS(peerListDelete)(&peerListManagement->List);
}


/* ###### Clear handlespace ################################################ */
void ST_CLASS(peerListManagementClear)(
        struct ST_CLASS(PeerListManagement)* peerListManagement)
{
   ST_CLASS(peerListClear)(&peerListManagement->List,
                           ST_CLASS(peerListManagementPeerListNodeDisposer),
                           (void*)peerListManagement);
}


/* ###### Print peer list ################################################ */
void ST_CLASS(peerListManagementPrint)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        FILE*                                fd,
        const unsigned int                   fields)
{
   ST_CLASS(peerListPrint)(&peerListManagement->List, fd, fields);
}


/* ###### Verify structures ############################################## */
void ST_CLASS(peerListManagementVerify)(
        struct ST_CLASS(PeerListManagement)* peerListManagement)
{
   ST_CLASS(peerListVerify)(&peerListManagement->List);
}


/* ###### Get number of pools ############################################ */
size_t ST_CLASS(peerListManagementGetPeers)(
                 const struct ST_CLASS(PeerListManagement)* peerListManagement)
{
   return(ST_CLASS(peerListGetPeerListNodes)(&peerListManagement->List));
}


/* ###### Find PeerListNode ############################################## */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindPeerListNode)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  const RegistrarIdentifierType        registrarIdentifier,
                                  const struct TransportAddressBlock*  transportAddressBlock)
{
   return(ST_CLASS(peerListFindPeerListNode)(
             &peerListManagement->List,
             registrarIdentifier,
             transportAddressBlock));
}


/* ###### Find nearest prev PeerListNode ################################# */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindNearestPrevPeerListNode)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  const RegistrarIdentifierType        registrarIdentifier,
                                  const struct TransportAddressBlock*  transportAddressBlock)
{
   return(ST_CLASS(peerListFindNearestPrevPeerListNode)(
             &peerListManagement->List,
             registrarIdentifier,
             transportAddressBlock));
}


/* ###### Find nearest next PeerListNode ################################# */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindNearestNextPeerListNode)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  const RegistrarIdentifierType        registrarIdentifier,
                                  const struct TransportAddressBlock*  transportAddressBlock)
{
   return(ST_CLASS(peerListFindNearestNextPeerListNode)(
             &peerListManagement->List,
             registrarIdentifier,
             transportAddressBlock));
}


/* ###### Get next timer time stamp ###################################### */
unsigned long long ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                      struct ST_CLASS(PeerListManagement)* peerListManagement)
{
   const struct ST_CLASS(PeerListNode)* nextTimer =
      ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(
         &peerListManagement->List);
   if(nextTimer != NULL) {
      return(nextTimer->TimerTimeStamp);
   }
   return(~0);
}


/* ###### Registration ################################################### */
unsigned int ST_CLASS(peerListManagementRegisterPeerListNode)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                const RegistrarIdentifierType        registrarIdentifier,
                const unsigned int                   flags,
                const struct TransportAddressBlock*  transportAddressBlock,
                const unsigned long long             currentTimeStamp,
                struct ST_CLASS(PeerListNode)**      peerListNode)
{
   struct ST_CLASS(PeerListNode) updatedPeerListNode;
   struct TransportAddressBlock* userTransport;
   unsigned int                  errorCode;

   if( ((flags & PLNF_DYNAMIC) && (registrarIdentifier == UNDEFINED_REGISTRAR_IDENTIFIER)) ||
       ((!(flags & PLNF_DYNAMIC)) && (registrarIdentifier != UNDEFINED_REGISTRAR_IDENTIFIER)) ) {
      return(RSPERR_INVALID_ID);
   }

   /* Check, if a static entry is updated with an ID */
   if(registrarIdentifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
      *peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(peerListManagement,
                                                                   UNDEFINED_REGISTRAR_IDENTIFIER,
                                                                   transportAddressBlock);
      if(*peerListNode) {
         /* When the found ID is a static entry, update this entry with
            the ID */
         if(!((*peerListNode)->Flags & PLNF_DYNAMIC)) {
            ST_CLASS(peerListNodeNew)(&updatedPeerListNode,
                                    registrarIdentifier,
                                    (*peerListNode)->Flags, /* PLNF_DYNAMIC is never set here! */
                                    (*peerListNode)->AddressBlock);
            ST_CLASS(peerListUpdatePeerListNode)(&peerListManagement->List, *peerListNode,
                                                &updatedPeerListNode, &errorCode);
            CHECK(errorCode == RSPERR_OKAY);
            return(RSPERR_OKAY);
         }
      }
   }

   if(peerListManagement->NewPeerListNode == NULL) {
      peerListManagement->NewPeerListNode = (struct ST_CLASS(PeerListNode)*)malloc(sizeof(struct ST_CLASS(PeerListNode)));
      if(peerListManagement->NewPeerListNode == NULL) {
         return(RSPERR_OUT_OF_MEMORY);
      }
   }

   /* Attention: transportAddressBlock MUST be copied when this
      PeerListNode is added! */
   ST_CLASS(peerListNodeNew)(peerListManagement->NewPeerListNode,
                             registrarIdentifier,
                             flags,
                             (struct TransportAddressBlock*)transportAddressBlock);
   *peerListNode = ST_CLASS(peerListAddOrUpdatePeerListNode)(&peerListManagement->List,
                                                             &peerListManagement->NewPeerListNode,
                                                             &errorCode);
   if(errorCode == RSPERR_OKAY) {
      (*peerListNode)->LastUpdateTimeStamp = currentTimeStamp;

      userTransport = transportAddressBlockDuplicate(transportAddressBlock);
      if(userTransport != NULL) {
         if((*peerListNode)->AddressBlock != transportAddressBlock) {  /* see comment above! */
            transportAddressBlockDelete((*peerListNode)->AddressBlock);
            free((*peerListNode)->AddressBlock);
         }
         (*peerListNode)->AddressBlock = userTransport;

         if(peerListManagement->Handlespace) {
            (*peerListNode)->OwnershipChecksum =
               ST_CLASS(poolHandlespaceNodeComputeOwnershipChecksum)(
                  &peerListManagement->Handlespace->Handlespace,
                  (*peerListNode)->Identifier);
         }
      }
      else {
         ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
            peerListManagement,
            *peerListNode);
         *peerListNode = NULL;
         errorCode = RSPERR_OUT_OF_MEMORY;
      }
   }

#ifdef VERIFY
   ST_CLASS(peerListVerify)(&peerListManagement->List);
#endif
   return(errorCode);
}


/* ###### Deregistration ################################################# */
unsigned int ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                struct ST_CLASS(PeerListNode)*       peerListNode)
{
   struct ST_CLASS(PeerListNode) updatedPeerListNode;
   unsigned int                  errorCode;

   /* Check, if a static entry with ID should be turned into ID-less entry only */
   if((!(peerListNode->Flags & PLNF_DYNAMIC)) &&
      (peerListNode->Identifier != UNDEFINED_REGISTRAR_IDENTIFIER)) {
      ST_CLASS(peerListNodeNew)(&updatedPeerListNode,
                                0,
                                peerListNode->Flags,
                                peerListNode->AddressBlock);
      ST_CLASS(peerListUpdatePeerListNode)(&peerListManagement->List, peerListNode,
                                           &updatedPeerListNode, &errorCode);
      CHECK(errorCode == RSPERR_OKAY);
   }
   else {
      ST_CLASS(peerListRemovePeerListNode)(&peerListManagement->List, peerListNode);
      ST_CLASS(peerListNodeDelete)(peerListNode);
      ST_CLASS(peerListManagementPeerListNodeDisposer)(peerListNode, peerListManagement);
   }

#ifdef VERIFY
   ST_CLASS(peerListVerify)(&peerListManagement->List);
#endif
   return(RSPERR_OKAY);
}


/* ###### Deregistration ################################################# */
unsigned int ST_CLASS(peerListManagementDeregisterPeerListNode)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                const RegistrarIdentifierType             registrarIdentifier,
                const struct TransportAddressBlock*  transportAddressBlock)
{
   struct ST_CLASS(PeerListNode)* peerListNode = ST_CLASS(peerListFindPeerListNode)(
                                                          &peerListManagement->List,
                                                          registrarIdentifier,
                                                          transportAddressBlock);
   if(peerListNode) {
      return(ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                peerListManagement,
                peerListNode));
   }
   return(RSPERR_NOT_FOUND);
}


/* ###### Restart PE expiry timer to last update TS + expiry timeout ##### */
void ST_CLASS(peerListManagementRestartPeerListNodeExpiryTimer)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        struct ST_CLASS(PeerListNode)*       peerListNode,
        const unsigned long long             expiryTimeout)
{
   ST_CLASS(peerListDeactivateTimer)(&peerListManagement->List,
                                     peerListNode);
   ST_CLASS(peerListActivateTimer)(&peerListManagement->List,
                                   peerListNode,
                                   PLNT_MAX_TIME_NO_RESPONSE,
                                   peerListNode->LastUpdateTimeStamp + expiryTimeout);
}


/* ###### Purge handlespace from expired PE entries ######################## */
size_t ST_CLASS(peerListManagementPurgeExpiredPeerListNodes)(
          struct ST_CLASS(PeerListManagement)* peerListManagement,
          const unsigned long long             currentTimeStamp)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct ST_CLASS(PeerListNode)* nextPeerListNode;
   size_t                         purgedPeerLists = 0;

   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(&peerListManagement->List);
   while(peerListNode != NULL) {
      nextPeerListNode = ST_CLASS(peerListGetNextPeerListNodeFromTimerStorage)(&peerListManagement->List, peerListNode);

      CHECK(peerListNode->TimerCode == PLNT_MAX_TIME_NO_RESPONSE);
      CHECK(STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode));
      if(peerListNode->TimerTimeStamp <= currentTimeStamp) {
         ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
            peerListManagement,
            peerListNode);
         purgedPeerLists++;
      }
      else {
         /* No more relevant entries, since time list is sorted! */
         break;
      }

      peerListNode = nextPeerListNode;
   }

   return(purgedPeerLists);
}


/* ###### PE update callback for checksum consistency with handlespace ### */
static void ST_CLASS(peerListManagementPoolNodeUpdateNotification)(
               struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
               struct ST_CLASS(PoolElementNode)*           poolElementNode,
               enum PoolNodeUpdateAction                   updateAction,
               HandlespaceChecksumAccumulatorType          preUpdateChecksum,
               RegistrarIdentifierType                     preUpdateHomeRegistrar,
               void*                                       userData)
{
   struct ST_CLASS(PeerListManagement)* peerListManagement      = (struct ST_CLASS(PeerListManagement)*)userData;
   RegistrarIdentifierType              homeRegistrarIdentifier = poolElementNode->HomeRegistrarIdentifier;
   struct ST_CLASS(PeerListNode)*       peerListNode;

   if(updateAction == PNUA_Create) {
      peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(peerListManagement,
                                                                  homeRegistrarIdentifier,
                                                                  NULL);
      if(peerListNode) {
         peerListNode->OwnershipChecksum =
            handlespaceChecksumAdd(peerListNode->OwnershipChecksum,
                                   poolElementNode->Checksum);
      }
   }
   else if(updateAction == PNUA_Delete) {
      peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(peerListManagement,
                                                                  homeRegistrarIdentifier,
                                                                  NULL);
      if(peerListNode) {
         peerListNode->OwnershipChecksum =
            handlespaceChecksumSub(peerListNode->OwnershipChecksum,
                                   poolElementNode->Checksum);
      }
   }
   else if(updateAction == PNUA_Update) {
      peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(peerListManagement,
                                                                  preUpdateHomeRegistrar,
                                                                  NULL);
      if(peerListNode) {
         peerListNode->OwnershipChecksum =
            handlespaceChecksumSub(peerListNode->OwnershipChecksum,
                                   preUpdateChecksum);
      }

      peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(peerListManagement,
                                                                  homeRegistrarIdentifier,
                                                                  NULL);
      if(peerListNode) {
         peerListNode->OwnershipChecksum =
            handlespaceChecksumAdd(peerListNode->OwnershipChecksum,
                                   poolElementNode->Checksum);
      }
   }
}


/* ###### Verify computed ownership checksums in handlespace ############# */
void ST_CLASS(peerListManagementVerifyChecksumsInHandlespace)(
        struct ST_CLASS(PeerListManagement)*        peerListManagement,
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement)
{
   struct ST_CLASS(PeerListNode)* peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&peerListManagement->List);
   while(peerListNode != NULL) {
      if(peerListNode->Identifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
         CHECK(peerListNode->OwnershipChecksum ==
                  ST_CLASS(poolHandlespaceNodeComputeOwnershipChecksum)(
                     &poolHandlespaceManagement->Handlespace,
                     peerListNode->Identifier));
      }
      peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(&peerListManagement->List, peerListNode);
   }
}
