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
 * Copyright (C) 2003-2021 by Thomas Dreibholz
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
 * Contact: dreibh@iem.uni-due.de
 */

/* ###### Initialize ##################################################### */
void ST_CLASS(peerListNew)(struct ST_CLASS(PeerList)*    peerList,
                           const RegistrarIdentifierType ownIdentifier)
{
   ST_METHOD(New)(&peerList->PeerListIndexStorage,
                  ST_CLASS(peerListIndexStorageNodePrint),
                  ST_CLASS(peerListIndexStorageNodeComparison));
   ST_METHOD(New)(&peerList->PeerListTimerStorage,
                  ST_CLASS(peerListTimerStorageNodePrint),
                  ST_CLASS(peerListTimerStorageNodeComparison));
   peerList->OwnIdentifier = ownIdentifier;
   peerList->UserData      = NULL;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(peerListDelete)(struct ST_CLASS(PeerList)* peerList)
{
   ST_METHOD(Delete)(&peerList->PeerListIndexStorage);
   ST_METHOD(Delete)(&peerList->PeerListTimerStorage);

   peerList->OwnIdentifier = UNDEFINED_REGISTRAR_IDENTIFIER;
}


/* ###### Get number of pool elements #################################### */
size_t ST_CLASS(peerListGetPeerListNodes)(
          const struct ST_CLASS(PeerList)* peerList)
{
   return(ST_METHOD(GetElements)(&peerList->PeerListIndexStorage));
}


/* ###### Get first PeerListNode from Index ############################## */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerList)* peerList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&peerList->PeerListIndexStorage);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last PeerListNode from Index ############################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetLastPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerList)* peerList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&peerList->PeerListIndexStorage);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next PeerListNode from Index ############################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&peerList->PeerListIndexStorage,
                                                   &peerListNode->PeerListIndexStorageNode);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous PeerListNode from Index ########################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetPrevPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&peerList->PeerListIndexStorage,
                                                   &peerListNode->PeerListIndexStorageNode);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get first PeerListNode from Timer ############################## */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerList)* peerList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&peerList->PeerListTimerStorage);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last PeerListNode from Timer ############################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetLastPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerList)* peerList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&peerList->PeerListTimerStorage);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next PeerListNode from Timer ############################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetNextPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&peerList->PeerListTimerStorage,
                                                   &peerListNode->PeerListTimerStorageNode);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous PeerListNode from Timer ########################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetPrevPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&peerList->PeerListTimerStorage,
                                                   &peerListNode->PeerListTimerStorageNode);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Insert PeerListNode into timer storage ######################### */
void ST_CLASS(peerListActivateTimer)(
        struct ST_CLASS(PeerList)*     peerList,
        struct ST_CLASS(PeerListNode)* peerListNode,
        const unsigned int             timerCode,
        const unsigned long long       timerTimeStamp)
{
   struct STN_CLASSNAME* result;

   CHECK(!STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode));
   peerListNode->TimerCode      = timerCode;
   peerListNode->TimerTimeStamp = timerTimeStamp;
   result = ST_METHOD(Insert)(&peerList->PeerListTimerStorage,
                              &peerListNode->PeerListTimerStorageNode);
   CHECK(result == &peerListNode->PeerListTimerStorageNode);
}


/* ###### Remove PeerListNode from timer storage ######################### */
void ST_CLASS(peerListDeactivateTimer)(
        struct ST_CLASS(PeerList)*     peerList,
        struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* result;

   if(STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode)) {
      result = ST_METHOD(Remove)(&peerList->PeerListTimerStorage,
                                 &peerListNode->PeerListTimerStorageNode);
      CHECK(result == &peerListNode->PeerListTimerStorageNode);
   }
}


/* ###### Get textual description ######################################## */
void ST_CLASS(peerListGetDescription)(struct ST_CLASS(PeerList)* peerList,
                                      char*                      buffer,
                                      const size_t               bufferSize)
{
   snprintf(buffer, bufferSize, "PeerList of $%08x (%u peers)",
            peerList->OwnIdentifier,
            (unsigned int)ST_CLASS(peerListGetPeerListNodes)(peerList));
}


/* ###### Verify ######################################################### */
void ST_CLASS(peerListVerify)(struct ST_CLASS(PeerList)* peerList)
{
   ST_METHOD(Verify)(&peerList->PeerListIndexStorage);
   ST_METHOD(Verify)(&peerList->PeerListTimerStorage);
}


/* ###### Print ########################################################## */
void ST_CLASS(peerListPrint)(struct ST_CLASS(PeerList)* peerList,
                             FILE*                      fd,
                             const unsigned int         fields)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   char                           peerListDescription[128];
   unsigned int                   i;

   ST_CLASS(peerListGetDescription)(peerList,
                                    (char*)&peerListDescription,
                                    sizeof(peerListDescription));
   fputs(peerListDescription, fd);
   fputs("\n", fd);

   if(fields & PLPO_PEERS_INDEX) {
      fputs(" +-- Peers by Index:\n", fd);
      i = 1;
      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(peerList);
      while(peerListNode != NULL) {
         fprintf(fd, "   - idx:#%04u: ", i);
         ST_CLASS(peerListNodePrint)(peerListNode, fd, fields);
         fputs("\n", fd);
         i++;
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(peerList, peerListNode);
      }
   }
   if(fields & PLPO_PEERS_TIMER) {
      fputs(" +-- Peers by Timer:\n", fd);
      i = 1;
      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(peerList);
      while(peerListNode != NULL) {
         fprintf(fd, "   - idx:#%04u: ", i);
         ST_CLASS(peerListNodePrint)(peerListNode, fd, fields);
         fputs("\n", fd);
         i++;
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromTimerStorage)(peerList, peerListNode);
      }
   }
}


/* ###### Clear ########################################################## */
void ST_CLASS(peerListClear)(struct ST_CLASS(PeerList)* peerList,
                             void                       (*peerListNodeDisposer)(void* peerListNode, void* userData),
                             void*                      userData)
{
   struct ST_CLASS(PeerListNode)* peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(peerList);
   while(peerListNode != NULL) {
      ST_CLASS(peerListRemovePeerListNode)(peerList, peerListNode);
      ST_CLASS(peerListNodeDelete)(peerListNode);
      peerListNodeDisposer(peerListNode, userData);
      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(peerList);
   }
}


/* ###### Check, if node may be inserted into pool ####################### */
unsigned int ST_CLASS(peerListCheckPeerListNodeCompatibility)(
                const struct ST_CLASS(PeerList)*     peerList,
                const struct ST_CLASS(PeerListNode)* peerListNode)
{
   /* It does not make sense to insert its own ID into the peer list! */
   if((peerList->OwnIdentifier != UNDEFINED_REGISTRAR_IDENTIFIER) &&
      (peerListNode->Identifier == peerList->OwnIdentifier)) {
      return(RSPERR_OWN_ID);
   }
   return(RSPERR_OKAY);
}


/* ###### Add PeerListNode ############################################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListAddPeerListNode)(
                                     struct ST_CLASS(PeerList)*     peerList,
                                     struct ST_CLASS(PeerListNode)* peerListNode,
                                     unsigned int*                  errorCode)
{
   struct STN_CLASSNAME* result;

   *errorCode = ST_CLASS(peerListCheckPeerListNodeCompatibility)(peerList, peerListNode);
   if(*errorCode != RSPERR_OKAY) {
      return(NULL);
   }

   /* Allow random selection. It might be useful to add priorities ... */
   peerListNode->PeerListIndexStorageNode.Value = 1;

   result = ST_METHOD(Insert)(&peerList->PeerListIndexStorage,
                              &peerListNode->PeerListIndexStorageNode);
   if(result == &peerListNode->PeerListIndexStorageNode) {
      peerListNode->OwnerPeerList = peerList;
      *errorCode = RSPERR_OKAY;
      return(peerListNode);
   }
   *errorCode = RSPERR_DUPLICATE_ID;
   return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(result));
}


/* ###### Update PeerListNode ############################################ */
void ST_CLASS(peerListUpdatePeerListNode)(
        struct ST_CLASS(PeerList)*           peerList,
        struct ST_CLASS(PeerListNode)*       peerListNode,
        const struct ST_CLASS(PeerListNode)* source,
        unsigned int*                        errorCode)
{
   struct STN_CLASSNAME* result;

   *errorCode = ST_CLASS(peerListCheckPeerListNodeCompatibility)(peerList, source);
   if(*errorCode == RSPERR_OKAY) {
      /* Update ID */
      if(peerListNode->Identifier != source->Identifier) {
         result = ST_METHOD(Remove)(&peerList->PeerListIndexStorage,
                                    &peerListNode->PeerListIndexStorageNode);
         CHECK(result == &peerListNode->PeerListIndexStorageNode);

         peerListNode->Identifier = source->Identifier;

         result = ST_METHOD(Insert)(&peerList->PeerListIndexStorage,
                                    &peerListNode->PeerListIndexStorageNode);
         CHECK(result == &peerListNode->PeerListIndexStorageNode);
      }
      ST_CLASS(peerListNodeUpdate)(peerListNode, source);
      peerListNode->Flags &= ~PLNF_NEW;
   }
}


/* ###### Add or Update PeerListNode ##################################### */
/*
   Allocation behavior:
   User program places PeerListNode data in new memory area.
   If the structure is used for insertion into the list, their pointer is
   set to NULL. So, the user program has to allocate new spaces for the next
   element. Otherwise, data has been copied into already existing nodes and the
   memory area can be reused.
*/
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListAddOrUpdatePeerListNode)(
                                  struct ST_CLASS(PeerList)*      peerList,
                                  struct ST_CLASS(PeerListNode)** peerListNode,
                                  unsigned int*                   errorCode)
{
   struct ST_CLASS(PeerListNode)* newPeerListNode;

   newPeerListNode = ST_CLASS(peerListAddPeerListNode)(peerList, *peerListNode, errorCode);
   if(newPeerListNode != NULL) {
      if(newPeerListNode != *peerListNode) {
         ST_CLASS(peerListUpdatePeerListNode)(peerList, newPeerListNode, *peerListNode, errorCode);
      }
      else {
         newPeerListNode->Flags |= PLNF_NEW;
         *peerListNode = NULL;
      }
   }
   return(newPeerListNode);
}


/* ###### Find PeerListNode ############################################## */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const RegistrarIdentifierType       identifier,
                                  const struct TransportAddressBlock* transportAddressBlock)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct ST_CLASS(PeerListNode)  cmpElement;
   struct STN_CLASSNAME*          result;

   if(identifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
      cmpElement.Identifier   = identifier;
      cmpElement.AddressBlock = (struct TransportAddressBlock*)transportAddressBlock;
      result = ST_METHOD(Find)(&peerList->PeerListIndexStorage,
                               &cmpElement.PeerListIndexStorageNode);
      if(result) {
         return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(result));
      }
   }
   else {
      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(peerList);
      while(peerListNode != NULL) {
         if(transportAddressBlockOverlapComparison(peerListNode->AddressBlock,
                                                   transportAddressBlock) == 0) {
            return(peerListNode);
         }
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(peerList, peerListNode);
      }
   }
   return(NULL);
}


/* ###### Find nearest prev PeerListNode ################################# */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindNearestPrevPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const RegistrarIdentifierType       identifier,
                                  const struct TransportAddressBlock* transportAddressBlock)
{
   struct ST_CLASS(PeerListNode) cmpElement;
   struct STN_CLASSNAME*         result;

   cmpElement.Identifier   = identifier;
   cmpElement.AddressBlock = (struct TransportAddressBlock*)transportAddressBlock;
   result = ST_METHOD(GetNearestPrev)(&peerList->PeerListIndexStorage,
                                      &cmpElement.PeerListIndexStorageNode);
   if(result) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(result));
   }
   return(NULL);
}


/* ###### Find nearest next PeerListNode ################################# */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindNearestNextPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const RegistrarIdentifierType       identifier,
                                  const struct TransportAddressBlock* transportAddressBlock)
{
   struct ST_CLASS(PeerListNode) cmpElement;
   struct STN_CLASSNAME*         result;

   cmpElement.Identifier   = identifier;
   cmpElement.AddressBlock = (struct TransportAddressBlock*)transportAddressBlock;
   result = ST_METHOD(GetNearestNext)(&peerList->PeerListIndexStorage,
                                      &cmpElement.PeerListIndexStorageNode);
   if(result) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(result));
   }
   return(NULL);
}


/* ###### Remove PeerListNode ############################################ */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListRemovePeerListNode)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* result;

   result = ST_METHOD(Remove)(&peerList->PeerListIndexStorage,
                              &peerListNode->PeerListIndexStorageNode);
   CHECK(result == &peerListNode->PeerListIndexStorageNode);
   if(STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode)) {
      result = ST_METHOD(Remove)(&peerList->PeerListTimerStorage,
                                 &peerListNode->PeerListTimerStorageNode);
      CHECK(result == &peerListNode->PeerListTimerStorageNode);
   }

   peerListNode->OwnerPeerList = NULL;
   return(peerListNode);
}


/* ###### Get random PeerListNode from Index ############################# */
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetRandomPeerListNode)(
                                  struct ST_CLASS(PeerList)* peerList)
{
   unsigned long long       value;
   const unsigned long long maxValue = ST_METHOD(GetValueSum)(
                                          &peerList->PeerListIndexStorage);
   if(maxValue < 1) {
      return(NULL);
   }

   value = random64() % maxValue;
   return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(
             ST_METHOD(GetNodeByValue)(&peerList->PeerListIndexStorage, value)));
}
