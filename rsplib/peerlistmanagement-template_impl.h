/*
 * An Efficient RSerPool Pool List Management Implementation
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
void ST_CLASS(peerListManagementNew)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        const RegistrarIdentifierType             ownRegistrarIdentifier,
        void (*peerListNodeUserDataDisposer)(struct ST_CLASS(PeerListNode)* peerListNode,
                                             void*                             userData),
        void* disposerUserData)
{
   ST_CLASS(peerListNew)(&peerListManagement->List, ownRegistrarIdentifier);
   peerListManagement->NewPeerListNode              = NULL;
   peerListManagement->PeerListNodeUserDataDisposer = peerListNodeUserDataDisposer;
   peerListManagement->DisposerUserData             = disposerUserData;
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
                                  const RegistrarIdentifierType             registrarIdentifier,
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
                                  const RegistrarIdentifierType             registrarIdentifier,
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
                                  const RegistrarIdentifierType             registrarIdentifier,
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
                const RegistrarIdentifierType             registrarIdentifier,
                const unsigned int                   flags,
                const struct TransportAddressBlock*  transportAddressBlock,
                const unsigned long long             currentTimeStamp,
                struct ST_CLASS(PeerListNode)**      peerListNode)
{
   struct TransportAddressBlock* userTransport;
   unsigned int                  errorCode;

   if(peerListManagement->NewPeerListNode == NULL) {
      peerListManagement->NewPeerListNode = (struct ST_CLASS(PeerListNode)*)malloc(sizeof(struct ST_CLASS(PeerListNode)));
      if(peerListManagement->NewPeerListNode == NULL) {
         return(RSPERR_NO_RESOURCES);
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
      }
      else {
         ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
            peerListManagement,
            *peerListNode);
         *peerListNode = NULL;
         errorCode = RSPERR_NO_RESOURCES;
      }
   }

#ifdef VERIFY
   ST_CLASS(peerListNodeVerify)(&peerListManagement->List);
#endif
   return(errorCode);
}


/* ###### Deregistration ################################################# */
unsigned int ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                struct ST_CLASS(PeerListNode)*       peerListNode)
{
   ST_CLASS(peerListRemovePeerListNode)(&peerListManagement->List, peerListNode);
   ST_CLASS(peerListNodeDelete)(peerListNode);
   ST_CLASS(peerListManagementPeerListNodeDisposer)(peerListNode, peerListManagement);

#ifdef VERIFY
   ST_CLASS(peerListNodeVerify)(&peerListManagement->List);
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
