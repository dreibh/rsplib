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
 * Copyright (C) 2003-2026 by Thomas Dreibholz
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
      takeoverProcess->OutstandingAcks = 0;
      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&peerList->List);
      while(peerListNode != NULL) {
         if((peerListNode->Identifier != targetID) &&
            (peerListNode->Identifier != peerList->List.OwnIdentifier) &&
            (peerListNode->Identifier != 0)) {
            takeoverProcess->PeerIDArray[takeoverProcess->OutstandingAcks++] =
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


/* ###### Get number of outstanding acknowledgements ##################### */
size_t takeoverProcessGetOutstandingAcks(const struct TakeoverProcess* takeoverProcess)
{
   return(takeoverProcess->OutstandingAcks);
}


/* ###### Acknowledge takeover process ################################### */
size_t takeoverProcessAcknowledge(struct TakeoverProcess*       takeoverProcess,
                                  const RegistrarIdentifierType targetID,
                                  const RegistrarIdentifierType acknowledgerID)
{
   size_t i;

   for(i = 0;i < takeoverProcess->OutstandingAcks;i++) {
      if(takeoverProcess->PeerIDArray[i] == acknowledgerID) {
         for(   ;i < takeoverProcess->OutstandingAcks - 1;i++) {
            takeoverProcess->PeerIDArray[i] = takeoverProcess->PeerIDArray[i + 1];
         }
         takeoverProcess->OutstandingAcks--;
      }
   }
   return(takeoverProcess->OutstandingAcks);
}
