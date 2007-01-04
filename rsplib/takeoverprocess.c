/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
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

#include "takeoverprocess.h"


/* ###### Constructor #################################################### */
struct TakeoverProcess* takeoverProcessNew(
                           const RegistrarIdentifierType        targetID,
                           struct ST_CLASS(PeerListManagement)* peerList)
{
   struct TakeoverProcess*        takeoverProcess;
   struct ST_CLASS(PeerListNode)* peerListNode;
   const size_t                   peers = ST_CLASS(peerListManagementGetPeers)(peerList);

   CHECK(targetID != 0);
   CHECK(targetID != peerList->List.OwnIdentifier);

   takeoverProcess = (struct TakeoverProcess*)malloc(sizeof(struct TakeoverProcess) +
                                                     sizeof(RegistrarIdentifierType) * peers);
   if(takeoverProcess != NULL) {
      takeoverProcess->OutstandingAcknowledgements = 0;
      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&peerList->List);
      while(peerListNode != NULL) {
         if((peerListNode->Identifier != targetID) &&
            (peerListNode->Identifier != peerList->List.OwnIdentifier) &&
            (peerListNode->Identifier != 0)) {
            takeoverProcess->PeerIDArray[takeoverProcess->OutstandingAcknowledgements++] =
               peerListNode->Identifier;
         }
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(&peerList->List, peerListNode);
      }
   }

   return(takeoverProcess);
}


/* ###### Destructor ##################################################### */
void takeoverProcessDelete(struct TakeoverProcess* takeoverProcess)
{
   free(takeoverProcess);
}


/* ###### Acknowledge takeover process ################################### */
size_t takeoverProcessAcknowledge(struct TakeoverProcess*       takeoverProcess,
                                  const RegistrarIdentifierType targetID,
                                  const RegistrarIdentifierType acknowledgerID)
{
   size_t i;

   for(i = 0;i < takeoverProcess->OutstandingAcknowledgements;i++) {
      if(takeoverProcess->PeerIDArray[i] == acknowledgerID) {
         for(   ;i < takeoverProcess->OutstandingAcknowledgements - 1;i++) {
            takeoverProcess->PeerIDArray[i] = takeoverProcess->PeerIDArray[i + 1];
         }
         takeoverProcess->OutstandingAcknowledgements--;
      }
   }
   return(takeoverProcess->OutstandingAcknowledgements);
}
