/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2006 by Thomas Dreibholz
 *
 * $Id: peerlistnode-template.h 953 2006-02-22 09:05:42Z dreibh $
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

#include "takeoverprocesslist.h"


/* ###### Initialize ##################################################### */
void takeoverProcessListNew(struct TakeoverProcessList* takeoverProcessList)
{
   ST_METHOD(New)(&takeoverProcessList->TakeoverProcessIndexStorage,
                  takeoverProcessIndexPrint,
                  takeoverProcessIndexComparison);
   ST_METHOD(New)(&takeoverProcessList->TakeoverProcessTimerStorage,
                  NULL,
                  takeoverProcessTimerComparison);
}


/* ###### Invalidate takeover process list ############################### */
void takeoverProcessListDelete(struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessIndexStorage);
   while(node != NULL) {
      ST_METHOD(Remove)(&takeoverProcessList->TakeoverProcessIndexStorage, node);
      free(node);
      node = ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessIndexStorage);
   }
   ST_METHOD(Delete)(&takeoverProcessList->TakeoverProcessIndexStorage);
   ST_METHOD(Delete)(&takeoverProcessList->TakeoverProcessTimerStorage);
}


/* ###### Print takeover process list #################################### */
void takeoverProcessListPrint(struct TakeoverProcessList* takeoverProcessList,
                              FILE*                       fd)
{
   fprintf(fd, "Takeover Process List: (%u entries)\n",
           (unsigned int)ST_METHOD(GetElements)(&takeoverProcessList->TakeoverProcessIndexStorage));
   ST_METHOD(Print)(&takeoverProcessList->TakeoverProcessIndexStorage, fd);
}


/* ###### Get next takeover process's expiry time stamp ################## */
unsigned long long takeoverProcessListGetNextTimerTimeStamp(
                             struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessTimerStorage);
   if(node) {
      return(getTakeoverProcessFromTimerStorageNode(node)->ExpiryTimeStamp);
   }
   return(~0);
}


/* ###### Get earliest takeover process from list ######################## */
struct TakeoverProcess* takeoverProcessListGetEarliestTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessTimerStorage);
   if(node) {
      return(getTakeoverProcessFromTimerStorageNode(node));
   }
   return(NULL);
}


/* ###### Get first takeover process from list ########################### */
struct TakeoverProcess* takeoverProcessListGetFirstTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessIndexStorage);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Get last takeover process from list ############################ */
struct TakeoverProcess* takeoverProcessListGetLastTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetLast)(&takeoverProcessList->TakeoverProcessIndexStorage);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Get next takeover process from list ############################ */
struct TakeoverProcess* takeoverProcessListGetNextTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList,
                                  struct TakeoverProcess*     takeoverProcess)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetNext)(&takeoverProcessList->TakeoverProcessIndexStorage,
                         &takeoverProcess->IndexStorageNode);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Get previous takeover process from list ######################## */
struct TakeoverProcess* takeoverProcessListGetPrevTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList,
                                  struct TakeoverProcess*     takeoverProcess)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetPrev)(&takeoverProcessList->TakeoverProcessIndexStorage,
                         &takeoverProcess->IndexStorageNode);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Find takeover process ########################################## */
struct TakeoverProcess* takeoverProcessListFindTakeoverProcess(
                           struct TakeoverProcessList* takeoverProcessList,
                           const RegistrarIdentifierType    targetID)
{
   struct STN_CLASSNAME*  node;
   struct TakeoverProcess cmpTakeoverProcess;

   cmpTakeoverProcess.TargetID = targetID;
   node = ST_METHOD(Find)(&takeoverProcessList->TakeoverProcessIndexStorage,
                          &cmpTakeoverProcess.IndexStorageNode);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Create takeover process ######################################## */
unsigned int takeoverProcessListCreateTakeoverProcess(
                struct TakeoverProcessList*          takeoverProcessList,
                const RegistrarIdentifierType        targetID,
                struct ST_CLASS(PeerListManagement)* peerList,
                const unsigned long long             expiryTimeStamp)
{
   struct TakeoverProcess*        takeoverProcess;
   struct ST_CLASS(PeerListNode)* peerListNode;
   const size_t                   peers = ST_CLASS(peerListManagementGetPeers)(peerList);

   CHECK(targetID != 0);
   CHECK(targetID != peerList->List.OwnIdentifier);
   if(takeoverProcessListFindTakeoverProcess(takeoverProcessList, targetID)) {
      return(RSPERR_DUPLICATE_ID);
   }

   takeoverProcess = (struct TakeoverProcess*)malloc(sizeof(struct TakeoverProcess) +
                                                     sizeof(RegistrarIdentifierType) * peers);
   if(takeoverProcess == NULL) {
      return(RSPERR_OUT_OF_MEMORY);
   }

   STN_METHOD(New)(&takeoverProcess->IndexStorageNode);
   STN_METHOD(New)(&takeoverProcess->TimerStorageNode);
   takeoverProcess->TargetID                    = targetID;
   takeoverProcess->ExpiryTimeStamp             = expiryTimeStamp;
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

   ST_METHOD(Insert)(&takeoverProcessList->TakeoverProcessIndexStorage,
                     &takeoverProcess->IndexStorageNode);
   ST_METHOD(Insert)(&takeoverProcessList->TakeoverProcessTimerStorage,
                     &takeoverProcess->TimerStorageNode);
   return(RSPERR_OKAY);
}


/* ###### Acknowledge takeover process ################################### */
struct TakeoverProcess* takeoverProcessListAcknowledgeTakeoverProcess(
                           struct TakeoverProcessList* takeoverProcessList,
                           const RegistrarIdentifierType    targetID,
                           const RegistrarIdentifierType    acknowledgerID)
{
   size_t                  i;
   struct TakeoverProcess* takeoverProcess =
      takeoverProcessListFindTakeoverProcess(takeoverProcessList,
                                             targetID);
   if(takeoverProcess != NULL) {
      for(i = 0;i < takeoverProcess->OutstandingAcknowledgements;i++) {
         if(takeoverProcess->PeerIDArray[i] == acknowledgerID) {
            for(   ;i < takeoverProcess->OutstandingAcknowledgements - 1;i++) {
               takeoverProcess->PeerIDArray[i] = takeoverProcess->PeerIDArray[i + 1];
            }
            takeoverProcess->OutstandingAcknowledgements--;
            return(takeoverProcess);
         }
      }
   }
   return(NULL);
}


/* ###### Delete takeover process ######################################## */
void takeoverProcessListDeleteTakeoverProcess(
        struct TakeoverProcessList* takeoverProcessList,
        struct TakeoverProcess*     takeoverProcess)
{
   ST_METHOD(Remove)(&takeoverProcessList->TakeoverProcessIndexStorage,
                     &takeoverProcess->IndexStorageNode);
   ST_METHOD(Remove)(&takeoverProcessList->TakeoverProcessTimerStorage,
                     &takeoverProcess->TimerStorageNode);
   STN_METHOD(Delete)(&takeoverProcess->IndexStorageNode);
   STN_METHOD(Delete)(&takeoverProcess->TimerStorageNode);
   takeoverProcess->TargetID                    = 0;
   takeoverProcess->OutstandingAcknowledgements = 0;
   free(takeoverProcess);
}
