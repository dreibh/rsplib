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

/* ###### Print ########################################################## */
void ST_CLASS(peerListIndexStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PeerListNode)* peerListNode = (struct ST_CLASS(PeerListNode)*)nodePtr;
   ST_CLASS(peerListNodePrint)(peerListNode, fd, 0);
}


/* ###### Comparison ##################################################### */
int ST_CLASS(peerListIndexStorageNodeComparison)(const void* nodePtr1,
                                                 const void* nodePtr2)
{
   const struct ST_CLASS(PeerListNode)* node1 = ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PeerListNode)* node2 = ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)((void*)nodePtr2);

   /* Compare IDs */
   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }

   /* The IDs are equal. If both IDs are undefined, we compare
      the addresses completely to get a sorting order. */
   if((node1->Identifier == UNDEFINED_REGISTRAR_IDENTIFIER) &&
      (node2->Identifier == UNDEFINED_REGISTRAR_IDENTIFIER)) {
      return(transportAddressBlockComparison(node1->AddressBlock, node2->AddressBlock));
   }
   return(0);
}


/* ###### Print ########################################################## */
void ST_CLASS(peerListTimerStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PeerListNode)* peerListNode =
      ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)((void*)nodePtr);
   ST_CLASS(peerListNodePrint)(peerListNode, fd, 0);
}


/* ###### Comparison ##################################################### */
int ST_CLASS(peerListTimerStorageNodeComparison)(const void* nodePtr1,
                                                 const void* nodePtr2)
{
   const struct ST_CLASS(PeerListNode)* node1 =
      ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PeerListNode)* node2 =
      ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)((void*)nodePtr2);

   /* Compare time stamps */
   if(node1->TimerTimeStamp < node2->TimerTimeStamp) {
      return(-1);
   }
   else if(node1->TimerTimeStamp > node2->TimerTimeStamp) {
      return(1);
   }

   /* Compare IDs */
   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }

   /* The IDs are equal. If both IDs are undefined, we compare
      the addresses completely to get a sorting order. */
   if((node1->Identifier == UNDEFINED_REGISTRAR_IDENTIFIER) &&
      (node2->Identifier == UNDEFINED_REGISTRAR_IDENTIFIER)) {
      return(transportAddressBlockComparison(node1->AddressBlock, node2->AddressBlock));
   }
   return(0);
}


/* ###### Initialize ##################################################### */
void ST_CLASS(peerListNodeNew)(struct ST_CLASS(PeerListNode)* peerListNode,
                               const RegistrarIdentifierType  identifier,
                               const unsigned int             flags,
                               struct TransportAddressBlock*  transportAddressBlock)
{
   STN_METHOD(New)(&peerListNode->PeerListIndexStorageNode);
   STN_METHOD(New)(&peerListNode->PeerListTimerStorageNode);

   peerListNode->OwnerPeerList       = NULL;

   peerListNode->Identifier          = identifier;
   peerListNode->Flags               = flags;
   peerListNode->OwnershipChecksum   = INITIAL_HANDLESPACE_CHECKSUM;

   peerListNode->Status              = 0;
   peerListNode->TakeoverRegistrarID = UNDEFINED_REGISTRAR_IDENTIFIER;
   peerListNode->TakeoverProcess     = NULL;

   peerListNode->LastUpdateTimeStamp = 0;
   peerListNode->TimerCode           = 0;
   peerListNode->TimerTimeStamp      = 0;

   peerListNode->AddressBlock        = transportAddressBlock;
   peerListNode->UserData            = NULL;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(peerListNodeDelete)(struct ST_CLASS(PeerListNode)* peerListNode)
{
   CHECK(!STN_METHOD(IsLinked)(&peerListNode->PeerListIndexStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode));

   peerListNode->Flags               = 0;
   peerListNode->LastUpdateTimeStamp = 0;
   peerListNode->TimerCode           = 0;
   peerListNode->TimerTimeStamp      = 0;
   /* Do not clear AddressBlock, UserData and Identifier yet, they may be used for
      user-specific dispose function! */
}


/* ###### Get PeerListNode from given Index Node ######################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(void* node)
{
   const struct ST_CLASS(PeerListNode)* dummy = (struct ST_CLASS(PeerListNode)*)node;
   long n = (long)node - ((long)&dummy->PeerListIndexStorageNode - (long)dummy);
   return((struct ST_CLASS(PeerListNode)*)n);
}


/* ###### Get PeerListNode from given Timer Node ######################### */
struct ST_CLASS(PeerListNode)* ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(void* node)
{
   const struct ST_CLASS(PeerListNode)* dummy = (struct ST_CLASS(PeerListNode)*)node;
   long n = (long)node - ((long)&dummy->PeerListTimerStorageNode - (long)dummy);
   return((struct ST_CLASS(PeerListNode)*)n);
}


/* ###### Update ######################################################### */
int ST_CLASS(peerListNodeUpdate)(struct ST_CLASS(PeerListNode)*       peerListNode,
                                 const struct ST_CLASS(PeerListNode)* source)
{
   peerListNode->Flags = source->Flags;
   return(1);
}


/* ###### Get ownership checksum ######################################### */
HandlespaceChecksumType ST_CLASS(peerListNodeGetOwnershipChecksum)(
                           const struct ST_CLASS(PeerListNode)* peerListNode)
{
   return(handlespaceChecksumFinish(peerListNode->OwnershipChecksum));
}


/* ###### Get textual description ######################################## */
void ST_CLASS(peerListNodeGetDescription)(
        const struct ST_CLASS(PeerListNode)* peerListNode,
        char*                                buffer,
        const size_t                         bufferSize,
        const unsigned int                   fields)
{
   char transportAddressDescription[1024];

   snprintf(buffer, bufferSize,
            "$%08x upd=%llu chsum=$%08x flags=",
            peerListNode->Identifier,
            peerListNode->LastUpdateTimeStamp,
            (unsigned int)handlespaceChecksumFinish(peerListNode->OwnershipChecksum));
   if(peerListNode->Flags & PLNF_NEW) {
      safestrcat(buffer, "[new]", bufferSize);
   }
   if(!(peerListNode->Flags & PLNF_DYNAMIC)) {
      safestrcat(buffer, "(static)", bufferSize);
   }
   else {
      safestrcat(buffer, "[dynamic]", bufferSize);
   }
   if(peerListNode->Flags & PLNF_FROM_PEER) {
      safestrcat(buffer, "[fromPeer]", bufferSize);
   }

   if(peerListNode->Status & PLNS_LISTSYNC) {
      safestrcat(buffer, " LISTSYNC", bufferSize);
   }
   if(peerListNode->Status & PLNS_HTSYNC) {
      safestrcat(buffer, " HTSYNC", bufferSize);
   }
   if(peerListNode->Status & PLNS_MENTOR) {
      safestrcat(buffer, " MENTOR", bufferSize);
   }
   if(peerListNode->TakeoverProcess) {
      safestrcat(buffer, " TAKEOVER(own)", bufferSize);
   }
   if(peerListNode->TakeoverRegistrarID) {
      safestrcat(buffer, " TAKEOVER(other)", bufferSize);
   }

   if((fields & PLNPO_TRANSPORT) &&
      (peerListNode->AddressBlock->Addresses > 0)) {
      transportAddressBlockGetDescription(peerListNode->AddressBlock,
                                          (char*)&transportAddressDescription,
                                          sizeof(transportAddressDescription));
      safestrcat(buffer, "\n        addrs: ", bufferSize);
      safestrcat(buffer, transportAddressDescription, bufferSize);
   }
}


/* ###### Print ########################################################## */
void ST_CLASS(peerListNodePrint)(const struct ST_CLASS(PeerListNode)* peerListNode,
                                 FILE*                                fd,
                                 const unsigned int                   fields)
{
   char peerListNodeDescription[1532];
   ST_CLASS(peerListNodeGetDescription)(peerListNode,
                                        (char*)&peerListNodeDescription,
                                        sizeof(peerListNodeDescription),
                                        fields);
   fputs(peerListNodeDescription, fd);
}
