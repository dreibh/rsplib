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

/* ###### Print ########################################################## */
void ST_CLASS(peerListIndexStorageNodePrint)(const void *nodePtr, FILE* fd)
{
   const struct ST_CLASS(PeerListNode)* peerListNode = (struct ST_CLASS(PeerListNode)*)nodePtr;
   ST_CLASS(peerListNodePrint)(peerListNode, fd, 0);
}


/* ###### Comparison ##################################################### */
int ST_CLASS(peerListIndexStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PeerListNode)* node1 = (struct ST_CLASS(PeerListNode)*)nodePtr1;
   const struct ST_CLASS(PeerListNode)* node2 = (struct ST_CLASS(PeerListNode)*)nodePtr2;
   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
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
int ST_CLASS(peerListTimerStorageNodeComparison)(const void *nodePtr1, const void *nodePtr2)
{
   const struct ST_CLASS(PeerListNode)* node1 =
      ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PeerListNode)* node2 =
      ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)((void*)nodePtr2);

   if(node1->TimerTimeStamp < node2->TimerTimeStamp) {
      return(-1);
   }
   else if(node1->TimerTimeStamp > node2->TimerTimeStamp) {
      return(1);
   }
   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }
   return(0);
}


/* ###### Initialize ##################################################### */
void ST_CLASS(peerListNodeNew)(struct ST_CLASS(PeerListNode)* peerListNode,
                               const ENRPIdentifierType       identifier,
                               const unsigned int             flags,
                               struct TransportAddressBlock*  transportAddressBlock)
{
   STN_METHOD(New)(&peerListNode->PeerListIndexStorageNode);
   STN_METHOD(New)(&peerListNode->PeerListTimerStorageNode);

   peerListNode->OwnerPeerList       = NULL;

   peerListNode->Identifier          = identifier;
   peerListNode->Flags               = flags;

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

   peerListNode->Identifier          = 0;
   peerListNode->Flags               = 0;
   peerListNode->LastUpdateTimeStamp = 0;
   peerListNode->TimerCode           = 0;
   peerListNode->TimerTimeStamp      = 0;
   /* Do not clear AddressBlock and UserData yet, it must be used for
      user-specific dispose function! */
}


/* ###### Update ######################################################### */
int ST_CLASS(peerListNodeUpdate)(struct ST_CLASS(PeerListNode)*       peerListNode,
                                 const struct ST_CLASS(PeerListNode)* source)
{
   peerListNode->Flags = source->Flags;
   return(1);
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
            "$%08x upd=%Lu flgs=",
            peerListNode->Identifier,
            peerListNode->LastUpdateTimeStamp);
   if(!(peerListNode->Flags & PLNF_DYNAMIC)) {
      safestrcat(buffer, "static", bufferSize);
   }
   else {
      safestrcat(buffer, "dynamic", bufferSize);
   }
   if(peerListNode->Flags & PLNF_MULTICAST) {
      safestrcat(buffer, "+multicast", bufferSize);
   }

   if((fields & PLNPO_USERTRANSPORT) &&
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
