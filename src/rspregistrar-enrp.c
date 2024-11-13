/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2025 by Thomas Dreibholz
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

#include "rspregistrar.h"


/* ###### ENRP Presence timer callback ################################### */
void registrarHandleENRPAnnounceTimer(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData)
{
   struct Registrar* registrar = (struct Registrar*)userData;

   /* ====== Leave startup phase ========================================= */
   if(registrar->InStartupPhase) {
      registrarBeginNormalOperation(registrar, false);
   }

   /* ====== Send Presence as multicast announce ========================= */
   if(registrar->ENRPAnnounceViaMulticast) {
      if(multicastGroupControl(registrar->ENRPMulticastInputSocket,
                               &registrar->ENRPMulticastAddress,
                               true) == false) {
         LOG_WARNING
         fputs("Unable to join multicast group ", stdlog);
         fputaddress(&registrar->ENRPMulticastAddress.sa, true, stdlog);
         fputs(". Registrar will probably be unable to detect peers!\n", stdlog);
         LOG_END;
      }
      registrarSendENRPPresence(registrar, registrar->ENRPMulticastOutputSocket, 0, 0,
                                (union sockaddr_union*)&registrar->ENRPMulticastAddress, 1,
                                0, false);
   }

   /* ====== Send Presence to peers ====================================== */
   registrarSendENRPPresenceToAllPeers(registrar);

   /* ====== Restart heartbeat cycle timer =============================== */
   unsigned long long n = getMicroTime() + registrarRandomizeCycle(registrar->PeerHeartbeatCycle);
   timerStart(timer, n);
}




/* ###### Handle peer management timers ################################## */
void registrarHandlePeerEvent(struct Dispatcher* dispatcher,
                              struct Timer*      timer,
                              void*              userData)
{
   struct Registrar*              registrar = (struct Registrar*)userData;
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct ST_CLASS(PeerListNode)* nextPeerListNode;
   size_t                         outstandingAcks;
   size_t                         i;

   peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromTimerStorage)(
                     &registrar->Peers);
   while((peerListNode != NULL) &&
         (peerListNode->TimerTimeStamp <= getMicroTime())) {
      nextPeerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromTimerStorage)(
                            &registrar->Peers,
                            peerListNode);

      /* ====== Max-time-last-heard timer ================================ */
      if(peerListNode->TimerCode == PLNT_MAX_TIME_LAST_HEARD) {
         LOG_ACTION
         fputs("Peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fprintf(stdlog, " not head since MaxTimeLastHeard=%lluus -> requesting immediate Presence\n",
               registrar->PeerMaxTimeLastHeard);
         LOG_END
         registrarSendENRPPresence(registrar,
                                   registrar->ENRPUnicastSocket,
                                   0, 0,
                                   peerListNode->AddressBlock->AddressArray,
                                   peerListNode->AddressBlock->Addresses,
                                   peerListNode->Identifier,
                                   true);
         ST_CLASS(peerListManagementDeactivateTimer)(
            &registrar->Peers, peerListNode);
         ST_CLASS(peerListManagementActivateTimer)(
            &registrar->Peers, peerListNode, PLNT_MAX_TIME_NO_RESPONSE,
            getMicroTime() + registrar->PeerMaxTimeNoResponse);
      }

      /* ====== Max-time-no-response timer =============================== */
      else if(peerListNode->TimerCode == PLNT_MAX_TIME_NO_RESPONSE) {
         LOG_ACTION
         fputs("Peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fprintf(stdlog, " did not answer in MaxTimeNoResponse=%lluus -> removing it\n",
               registrar->PeerMaxTimeLastHeard);
         LOG_END

         CHECK(peerListNode->TakeoverProcess == NULL);
         if(peerListNode->TakeoverRegistrarID == UNDEFINED_REGISTRAR_IDENTIFIER) {
            /* Nobody else seems to have started a takeover yet ... */
            if(ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &registrar->Handlespace.Handlespace, peerListNode->Identifier) != NULL) {
               peerListNode->TakeoverProcess = takeoverProcessNew(
                                                  peerListNode->Identifier,
                                                  &registrar->Peers);
               if(peerListNode->TakeoverProcess) {
                  LOG_ACTION
                  fprintf(stdlog, "Initiating takeover of dead peer $%08x\n",
                        peerListNode->Identifier);
                  LOG_END
                  registrarSendENRPInitTakeoverToAllPeers(registrar, peerListNode->Identifier);
                  ST_CLASS(peerListManagementDeactivateTimer)(
                     &registrar->Peers, peerListNode);
                  ST_CLASS(peerListManagementActivateTimer)(
                     &registrar->Peers, peerListNode, PLNT_TAKEOVER_EXPIRY,
                     getMicroTime() + registrar->TakeoverExpiryInterval);
               }
            }
            else {
               LOG_ACTION
               fprintf(stdlog, "Dead peer $%08x has no PEs -> no takeover procedure necessary\n",
                     peerListNode->Identifier);
               LOG_END
               ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                        &registrar->Peers,
                        peerListNode);
            }
         }
         else {
            LOG_ACTION
            fprintf(stdlog, "Dead peer $%08x. Removing it from the peer list\n",
                  peerListNode->Identifier);
            LOG_END
            ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                     &registrar->Peers,
                     peerListNode);
         }
      }

      /* ====== Takeover expiry timer ==================================== */
      else if(peerListNode->TimerCode == PLNT_TAKEOVER_EXPIRY) {
         CHECK(peerListNode->TakeoverProcess != NULL);

         outstandingAcks = takeoverProcessGetOutstandingAcks(peerListNode->TakeoverProcess);
         LOG_WARNING
         fprintf(stdlog, "Takeover of peer $%08x has expired, %u outstanding acknowledgements:\n",
                 peerListNode->Identifier,
                 (unsigned int)outstandingAcks);
         for(i = 0;i < outstandingAcks;i++) {
            fprintf(stdlog, "- $%08x (%s)\n",
                    peerListNode->TakeoverProcess->PeerIDArray[i],
                    (ST_CLASS(peerListManagementFindPeerListNode)(
                              &registrar->Peers,
                              peerListNode->TakeoverProcess->PeerIDArray[i],
                              NULL) == NULL) ? "not found!" : "still available");
         }
         LOG_END

         takeoverProcessDelete(peerListNode->TakeoverProcess);
         peerListNode->TakeoverProcess = NULL;
         ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                  &registrar->Peers,
                  peerListNode);
      }

      /* ====== Bad timer ================================================ */
      else {
         LOG_ERROR
         fputs("Unexpected timer\n", stdlog);
         LOG_END_FATAL
      }

      peerListNode = nextPeerListNode;
   }

   timerRestart(&registrar->PeerActionTimer,
                ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                   &registrar->Peers));
}


/* ###### Handle ENRP Handle Update ###################################### */
void registrarHandleENRPHandleUpdate(struct Registrar*       registrar,
                                     const int               fd,
                                     const sctp_assoc_t      assocID,
                                     struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* delPoolElementNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct PoolPolicySettings         updatedPolicySettings;
   unsigned int                      distance;
   int                               result;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own HandleUpdate message\n", stdlog);
      LOG_END
      return;
   }

#ifdef ENABLE_REGISTRAR_STATISTICS
   registrar->Stats.HandleUpdateCount++;
#endif

   LOG_VERBOSE
   fputs("Got HandleUpdate for ", stdlog);
   poolHandlePrint(&message->Handle, stdlog);
   fprintf(stdlog, "/$%08x, action $%04x\n",
           message->PoolElementPtr->Identifier, message->Action);
   LOG_END
   LOG_VERBOSE2
   fputs("Updated pool element: ", stdlog);
   ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
   fputs("\n", stdlog);
   LOG_END

   if(message->Action == PNUP_ADD_PE) {
      if(message->PoolElementPtr->HomeRegistrarIdentifier != registrar->ServerID) {
         /* ====== Set distance for distance-sensitive policies ======= */
         distance = 0xffffffff;
         registrarUpdateDistance(registrar,
                                 fd, assocID, message->PoolElementPtr,
                                 &updatedPolicySettings, true, &distance);

         result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                     &registrar->Handlespace,
                     &message->Handle,
                     message->PoolElementPtr->HomeRegistrarIdentifier,
                     message->PoolElementPtr->Identifier,
                     message->PoolElementPtr->RegistrationLife,
                     &updatedPolicySettings,
                     message->PoolElementPtr->UserTransport,
                     message->PoolElementPtr->RegistratorTransport,
                     -1, 0,
                     getMicroTime(),
                     &newPoolElementNode);
         if(result == RSPERR_OKAY) {
            registrarRegistrationHook(registrar, newPoolElementNode);

            LOG_VERBOSE
            fputs("Successfully registered ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fprintf(stdlog, "/$%08x\n", newPoolElementNode->Identifier);
            LOG_END
            LOG_VERBOSE2
            fputs("Registered pool element: ", stdlog);
            ST_CLASS(poolElementNodePrint)(newPoolElementNode, stdlog, PENPO_FULL);
            fputs("\n", stdlog);
            LOG_END

            if(STN_METHOD(IsLinked)(&newPoolElementNode->PoolElementTimerStorageNode)) {
               ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
                  &registrar->Handlespace.Handlespace,
                  newPoolElementNode);
            }
            ST_CLASS(poolHandlespaceNodeActivateTimer)(
               &registrar->Handlespace.Handlespace,
               newPoolElementNode,
               PENT_EXPIRY,
               getMicroTime() + (1000ULL * newPoolElementNode->RegistrationLife));
            timerRestart(&registrar->HandlespaceActionTimer,
                         ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                            &registrar->Handlespace));

            LOG_VERBOSE3
            fputs("Handlespace content:\n", stdlog);
            registrarDumpHandlespace(registrar);
            LOG_END


            /* ====== Takeover suggestion ================================ */
            if( (message->Flags & EHF_TAKEOVER_SUGGESTED) &&
                (registrar->ENRPSupportTakeoverSuggestion) &&
                (newPoolElementNode->HomeRegistrarIdentifier != registrar->ServerID) ) {
               LOG_ACTION
               fprintf(stdlog, "Trying to take over PE $%08x from peer $%08x upon its suggestion\n",
                       newPoolElementNode->Identifier, newPoolElementNode->HomeRegistrarIdentifier);
               LOG_END
               registrarSendASAPEndpointKeepAlive(registrar, newPoolElementNode, true);
            }
            /* =========================================================== */
         }
         else {
            LOG_WARNING
            fputs("Failed to register to pool ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fputs(" pool element ", stdlog);
            ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
            fputs(": ", stdlog);
            rserpoolErrorPrint(result, stdlog);
            fputs("\n", stdlog);
            LOG_END
         }

#ifdef ENABLE_REGISTRAR_STATISTICS
         registrarWriteActionLog(registrar, "Recv", "ENRP", "Update", "AddPE", 0, 0, 0,
                                 &message->Handle, message->PoolElementPtr->Identifier, message->SenderID, message->ReceiverID, 0, result);
#endif
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "PR $%08x sent me a HandleUpdate for a PE owned by myself!\n",
                 message->SenderID);
         LOG_END
      }
   }

   else if(message->Action == PNUP_DEL_PE) {
      delPoolElementNode = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                              &registrar->Handlespace,
                              &message->Handle,
                              message->PoolElementPtr->Identifier);
      if(delPoolElementNode != NULL) {
         registrarDeregistrationHook(registrar, delPoolElementNode);

         result = ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                     &registrar->Handlespace,
                     delPoolElementNode);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Successfully deregistered ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fprintf(stdlog, "/$%08x\n", message->PoolElementPtr->Identifier);
            LOG_END
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Failed to deregister pool element $%08x from pool ",
                    message->PoolElementPtr->Identifier);
            poolHandlePrint(&message->Handle, stdlog);
            fputs(": ", stdlog);
            rserpoolErrorPrint(result, stdlog);
            fputs("\n", stdlog);
            LOG_END
         }
#ifdef ENABLE_REGISTRAR_STATISTICS
         registrarWriteActionLog(registrar, "Recv", "ENRP", "Update", "DelPE", 0, 0, 0,
                                 &message->Handle, message->PoolElementPtr->Identifier, message->SenderID, message->ReceiverID, 0, result);
#endif
      }
   }

   else {
      LOG_WARNING
      fprintf(stdlog, "Got HandleUpdate with invalid action $%04x\n",
              message->Action);
      LOG_END
   }
}


/* ###### Send peer name update ########################################## */
void registrarSendENRPHandleUpdate(struct Registrar*                 registrar,
                                   struct ST_CLASS(PoolElementNode)* poolElementNode,
                                   const uint16_t                    action)
{
#ifndef MSG_SEND_TO_ALL
   struct ST_CLASS(PeerListNode)* peerListNode;
#endif
   struct ST_CLASS(PeerListNode)* betterPeerListNode = NULL;
   struct RSerPoolMessage*        message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type                     = EHT_HANDLE_UPDATE;
      message->Flags                    = 0x00;
      message->Action                   = action;
      message->SenderID                 = registrar->ServerID;
      message->Handle                   = poolElementNode->OwnerPoolNode->Handle;
      message->PoolElementPtr           = poolElementNode;
      message->PoolElementPtrAutoDelete = false;

      LOG_VERBOSE
      fputs("Sending HandleUpdate for ", stdlog);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fprintf(stdlog, "/$%08x, action $%04x\n", poolElementNode->Identifier, action);
      LOG_END
      LOG_VERBOSE2
      fputs("Updated pool element: ", stdlog);
      ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
      fputs("\n", stdlog);
      LOG_END

      /* ====== Takeover suggestion ====================================== */
      if( (action == PNUP_ADD_PE) &&
          (registrar->ENRPSupportTakeoverSuggestion) &&
          (poolElementNode->HomeRegistrarIdentifier == registrar->ServerID) ) {
         betterPeerListNode = ST_CLASS(peerListManagementGetUsefulPeerForPE)(&registrar->Peers, poolElementNode->Identifier);
         if(betterPeerListNode) {
            LOG_ACTION
            fprintf(stdlog, "Found better peer $%08x for PE $%08x\n",
                    betterPeerListNode->Identifier, poolElementNode->Identifier);
            LOG_END
         }
      }
      /* ================================================================= */

#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Send", "ENRP", "Update", ((message->Action == PNUP_ADD_PE) ? "AddPE" : "DelPE"), 0, 0, 0,
                              &message->Handle, message->PoolElementPtr->Identifier, message->SenderID, message->ReceiverID, 0, 0);
#endif

#ifndef MSG_SEND_TO_ALL
      peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
      while(peerListNode != NULL) {
         message->ReceiverID   = peerListNode->Identifier;
         message->AddressArray = peerListNode->AddressBlock->AddressArray;
         message->Addresses    = peerListNode->AddressBlock->Addresses;
         LOG_VERBOSE
         fprintf(stdlog, "Sending HandleUpdate to unicast peer $%08x...\n",
                 peerListNode->Identifier);
         LOG_END
         rserpoolMessageSend(IPPROTO_SCTP,
                             registrar->ENRPUnicastSocket,
                             0, 0, 0, 0,
                             message);
         peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers, peerListNode);
      }
#else
#warning Using MSG_SEND_TO_ALL!
      rserpoolMessageSend(IPPROTO_SCTP,
                          registrar->ENRPUnicastSocket,
                          0,
                          MSG_SEND_TO_ALL, 0, 0, message);
#endif

      if(betterPeerListNode) {
         message->Flags |= EHF_TAKEOVER_SUGGESTED;
         message->ReceiverID   = betterPeerListNode->Identifier;
         message->AddressArray = betterPeerListNode->AddressBlock->AddressArray;
         message->Addresses    = betterPeerListNode->AddressBlock->Addresses;
         LOG_VERBOSE1
         fprintf(stdlog, "Sending HandleUpdate to unicast peer $%08x with TakeoverSuggested flag...\n",
                 betterPeerListNode->Identifier);
         LOG_END
         rserpoolMessageSend(IPPROTO_SCTP,
                             registrar->ENRPUnicastSocket,
                             0, 0, 0, 0,
                             message);
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Handle ENRP List Request ####################################### */
void registrarHandleENRPListRequest(struct Registrar*       registrar,
                                    const int               fd,
                                    const sctp_assoc_t      assocID,
                                    struct RSerPoolMessage* message)
{
   struct RSerPoolMessage* response;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own ListRequest message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got ListRequest from peer $%08x\n",
           message->SenderID);
   LOG_END
#ifdef ENABLE_REGISTRAR_STATISTICS
   registrarWriteActionLog(registrar, "Recv", "ENRP", "ListRequest", "", 0, 0, 0,
                           NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif

   response = rserpoolMessageNew(NULL, 65536);
   if(response != NULL) {
      response->Type       = EHT_LIST_RESPONSE;
      response->AssocID    = assocID;
      response->Flags      = 0x00;
      response->SenderID   = registrar->ServerID;
      response->ReceiverID = message->SenderID;

      response->PeerListPtr           = &registrar->Peers;
      response->PeerListPtrAutoDelete = false;

      LOG_VERBOSE
      fprintf(stdlog, "Sending ListResponse to peer $%08x...\n",
              message->SenderID);
      LOG_END
#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Send", "ENRP", "ListResponse", "", message->Flags,
                              ST_CLASS(peerListGetPeerListNodes)(&response->PeerListPtr->List), 0,
                              NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             fd, assocID, 0, 0, 0, response) == false) {
         LOG_WARNING
         fputs("Sending ListResponse failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(response);
   }
}


/* ###### Send ENRP List Request ######################################### */
void registrarSendENRPListRequest(struct Registrar*           registrar,
                                  int                         sd,
                                  const sctp_assoc_t          assocID,
                                  int                         msgSendFlags,
                                  const union sockaddr_union* destinationAddressList,
                                  const size_t                destinationAddresses,
                                  RegistrarIdentifierType     receiverID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type         = EHT_LIST_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = assocID;
      message->AddressArray = (union sockaddr_union*)destinationAddressList;
      message->Addresses    = destinationAddresses;
      message->Flags        = 0x00;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = receiverID;
#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Send", "ENRP", "ListRequest", "", 0, 0, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending ListRequest failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Handle ENRP List Response ###################################### */
void registrarHandleENRPListResponse(struct Registrar*       registrar,
                                     int                     fd,
                                     sctp_assoc_t            assocID,
                                     struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct ST_CLASS(PeerListNode)* newPeerListNode;
   int                            result;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own ListResponse message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got ListResponse from peer $%08x\n",
           message->SenderID);
   LOG_END
#ifdef ENABLE_REGISTRAR_STATISTICS
   registrarWriteActionLog(registrar, "Recv", "ENRP", "ListResponse", "", message->Flags,
                           (message->PeerListPtr != NULL) ? ST_CLASS(peerListGetPeerListNodes)(&message->PeerListPtr->List) : 0, 0,
                           NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif

   /* ====== Handle response ============================================= */
   if(!(message->Flags & EHF_LIST_RESPONSE_REJECT)) {
      if(message->PeerListPtr) {
         peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                           &message->PeerListPtr->List);
         while(peerListNode) {
            LOG_VERBOSE2
            fputs("Trying to add peer ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, ~0);
            fputs(" to peer list\n", stdlog);
            LOG_END
            if(peerListNode->Identifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
               result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                           &registrar->Peers,
                           peerListNode->Identifier,
                           peerListNode->Flags,
                           peerListNode->AddressBlock,
                           getMicroTime(),
                           &newPeerListNode);
               if((result == RSPERR_OKAY) &&
                  (!STN_METHOD(IsLinked)(&newPeerListNode->PeerListTimerStorageNode))) {
                  /* ====== Activate keep alive timer ==================== */
                  ST_CLASS(peerListManagementActivateTimer)(
                     &registrar->Peers, newPeerListNode, PLNT_MAX_TIME_LAST_HEARD,
                     getMicroTime() + registrar->PeerMaxTimeLastHeard);

                  /* ====== New peer -> Send Peer Presence =============== */
                  registrarSendENRPPresence(registrar,
                                            registrar->ENRPUnicastSocket, 0, 0,
                                            newPeerListNode->AddressBlock->AddressArray,
                                            newPeerListNode->AddressBlock->Addresses,
                                            newPeerListNode->Identifier,
                                            false);
               }
            }
            else {
               LOG_ACTION
               fputs("Skipping unknown peer (due to ID=0) from ListResponse: ", stdlog);
               ST_CLASS(peerListNodePrint)(peerListNode, stdlog, ~0);
               fputs("\n", stdlog);
               LOG_END
            }

            peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                              &message->PeerListPtr->List, peerListNode);
         }
      }

      timerRestart(&registrar->PeerActionTimer,
                   ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                      &registrar->Peers));

      /* ====== Print debug information ============================ */
      LOG_VERBOSE3
      fputs("Peers:\n", stdlog);
      registrarDumpPeers(registrar);
      LOG_END
   }
   else {
      LOG_ACTION
      fprintf(stdlog, "Rejected ListResponse from peer $%08x\n",
              message->SenderID);
      LOG_END
   }
}


/* ###### Handle ENRP Handle Table Request ############################### */
void registrarHandleENRPHandleTableRequest(struct Registrar*       registrar,
                                           int                     fd,
                                           sctp_assoc_t            assocID,
                                           struct RSerPoolMessage* message)
{
   struct RSerPoolMessage*        response;
   struct ST_CLASS(PeerListNode)* peerListNode;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own HandleTableRequest message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE2
   fprintf(stdlog, "Got HandleTableRequest from peer $%08x\n",
           message->SenderID);
   LOG_END

#ifdef ENABLE_REGISTRAR_STATISTICS
   registrarWriteActionLog(registrar, "Recv", "ENRP", "HandleTableRequest", "", message->Flags, 0, 0,
                           NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
   registrar->Stats.SynchronizationCount++;
#endif

   peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                     &registrar->Peers,
                     message->SenderID,
                     NULL);

   /* We allow only 1400 bytes per HandleTableResponse... */
   response = rserpoolMessageNew(NULL, 1400);
   if(response != NULL) {
      response->Type                      = EHT_HANDLE_TABLE_RESPONSE;
      response->AssocID                   = assocID;
      response->Flags                     = 0x00;
      response->SenderID                  = registrar->ServerID;
      response->ReceiverID                = message->SenderID;
      response->Action                    = message->Flags;

      response->PeerListNodePtr           = peerListNode;
      response->PeerListNodePtrAutoDelete = false;
      response->HandlespacePtr            = &registrar->Handlespace;
      response->HandlespacePtrAutoDelete  = false;
      response->MaxElementsPerHTRequest   = registrar->MaxElementsPerHTRequest;

      if(peerListNode == NULL) {
         response->Flags |= EHF_HANDLE_TABLE_RESPONSE_REJECT;
         LOG_WARNING
         fprintf(stdlog, "HandleTableRequest from peer $%08x -> This peer is unknown, rejecting request!\n",
                 message->SenderID);
         LOG_END
      }

      LOG_VERBOSE
      fprintf(stdlog, "Sending HandleTableResponse to peer $%08x...\n",
              message->SenderID);
      LOG_END
#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Send", "ENRP", "HandleTableResponse", "", message->Flags,
                              (response->HandlespacePtr != NULL) ? ST_CLASS(poolHandlespaceManagementGetPoolElements)(response->HandlespacePtr) : 0, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             fd, assocID, 0, 0, 0, response) == false) {
         LOG_WARNING
         fputs("Sending HandleTableResponse failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(response);
   }
}


/* ###### Send ENRP Handle Table Request ################################# */
void registrarSendENRPHandleTableRequest(struct Registrar*           registrar,
                                         int                         sd,
                                         const sctp_assoc_t          assocID,
                                         int                         msgSendFlags,
                                         const union sockaddr_union* destinationAddressList,
                                         const size_t                destinationAddresses,
                                         RegistrarIdentifierType     receiverID,
                                         unsigned int                flags)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type         = EHT_HANDLE_TABLE_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = assocID;
      message->AddressArray = (union sockaddr_union*)destinationAddressList;
      message->Addresses    = destinationAddresses;
      message->Flags        = flags;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = receiverID;
#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Send", "ENRP", "HandleTableRequest", "", message->Flags, 0, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending HandleTableRequest failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Handle ENRP Handle Table Response ############################## */
void registrarHandleENRPHandleTableResponse(struct Registrar*       registrar,
                                            int                     fd,
                                            sctp_assoc_t            assocID,
                                            struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct ST_CLASS(PeerListNode)*    peerListNode;
   struct PoolPolicySettings         updatedPolicySettings;
   unsigned int                      distance;
   unsigned int                      result;
   size_t                            purged;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own HandleTableResponse message\n", stdlog);
      LOG_END
      return;
   }

   LOG_ACTION
   fprintf(stdlog, "Got HandleTableResponse from peer $%08x\n",
           message->SenderID);
   LOG_END
#ifdef ENABLE_REGISTRAR_STATISTICS
   registrarWriteActionLog(registrar, "Recv", "ENRP", "HandleTableResponse", "", message->Flags,
                           (message->HandlespacePtr != NULL) ? ST_CLASS(poolHandlespaceManagementGetPoolElements)(message->HandlespacePtr) : 0, 0,
                           NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif


   /* ====== Find peer list node ========================================= */
   peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                           &registrar->Peers, message->SenderID, NULL);
   if(peerListNode == NULL) {
      LOG_WARNING
      fprintf(stdlog, "Got HandleTableResponse from peer $%08x which is not in peer list\n",
            message->SenderID);
      LOG_END
      return;
   }

   /* ====== Propagate response data into the registrarHandlespace ================ */
   if(!(message->Flags & EHF_HANDLE_TABLE_RESPONSE_REJECT)) {
      if(message->HandlespacePtr) {
         distance = 0xffffffff;
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(&message->HandlespacePtr->Handlespace);
         while(poolElementNode != NULL) {
            /* ====== Set distance for distance-sensitive policies ======= */
            registrarUpdateDistance(registrar,
                                    fd, assocID, poolElementNode,
                                    &updatedPolicySettings, true, &distance);

            if(poolElementNode->HomeRegistrarIdentifier != registrar->ServerID) {
               result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                           &registrar->Handlespace,
                           &poolElementNode->OwnerPoolNode->Handle,
                           poolElementNode->HomeRegistrarIdentifier,
                           poolElementNode->Identifier,
                           poolElementNode->RegistrationLife,
                           &updatedPolicySettings,
                           poolElementNode->UserTransport,
                           poolElementNode->RegistratorTransport,
                           -1, 0,
                           getMicroTime(),
                           &newPoolElementNode);
               if(result == RSPERR_OKAY) {
                  registrarRegistrationHook(registrar, newPoolElementNode);

                  LOG_VERBOSE
                  fputs("Successfully registered ", stdlog);
                  poolHandlePrint(&message->Handle, stdlog);
                  fprintf(stdlog, "/$%08x\n", poolElementNode->Identifier);
                  LOG_END
                  LOG_VERBOSE2
                  fputs("Registered pool element: ", stdlog);
                  ST_CLASS(poolElementNodePrint)(newPoolElementNode, stdlog, PENPO_FULL);
                  fputs("\n", stdlog);
                  LOG_END

                  if(!STN_METHOD(IsLinked)(&newPoolElementNode->PoolElementTimerStorageNode)) {
                     ST_CLASS(poolHandlespaceNodeActivateTimer)(
                        &registrar->Handlespace.Handlespace,
                        newPoolElementNode,
                        PENT_EXPIRY,
                        getMicroTime() + (1000ULL * newPoolElementNode->RegistrationLife));
                  }
               }
               else {
                  LOG_WARNING
                  fputs("Failed to register to pool ", stdlog);
                  poolHandlePrint(&message->Handle, stdlog);
                  fputs(" pool element ", stdlog);
                  ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
                  fputs(": ", stdlog);
                  rserpoolErrorPrint(result, stdlog);
                  fputs("\n", stdlog);
                  LOG_END
               }
            }
            else {
               LOG_WARNING
               fprintf(stdlog, "PR $%08x sent me a HandleTableResponse containing a PE owned by myself!\n",
                       message->SenderID);
               ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
               fputs("\n", stdlog);
               LOG_END
            }
            poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(&message->HandlespacePtr->Handlespace, poolElementNode);
         }

         timerRestart(&registrar->HandlespaceActionTimer,
                      ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                         &registrar->Handlespace));


         LOG_VERBOSE3
         fputs("Handlespace content:\n", stdlog);
         registrarDumpHandlespace(registrar);
         fputs("Peers:\n", stdlog);
         registrarDumpPeers(registrar);
         LOG_END


         if(message->Flags & EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND) {
            LOG_ACTION
            fprintf(stdlog, "HandleTableResponse has MoreToSend flag set -> sending HandleTableRequest to peer $%08x to get more data\n",
                    message->SenderID);
            LOG_END
            registrarSendENRPHandleTableRequest(registrar, fd, message->AssocID, 0, NULL, 0, message->SenderID,
                                                (peerListNode->Status & PLNS_MENTOR) ? 0x00 : EHF_HANDLE_TABLE_REQUEST_OWN_CHILDREN_ONLY);
         }
         else {
            peerListNode->Status &= ~(PLNS_MENTOR|PLNS_HTSYNC);   /* Synchronization completed */
            purged = ST_CLASS(poolHandlespaceManagementPurgeMarkedPoolElementNodes)(
                        &registrar->Handlespace, message->SenderID);
            if(purged) {
               LOG_ACTION
               fprintf(stdlog, "Purged %u PEs after last Handle Table Response from $%08x\n",
                       (unsigned int)purged, message->SenderID);
               LOG_END
            }
            if( (registrar->InStartupPhase) &&
                (registrar->MentorServerID == message->SenderID) ) {
               registrarBeginNormalOperation(registrar, true);
            }
         }
      }
      else {
         peerListNode->Status &= ~(PLNS_MENTOR|PLNS_HTSYNC);   /* Synchronization completed */
      }
   }
   else {
      LOG_ACTION
      fprintf(stdlog, "Peer $%08x has rejected the HandleTableRequest\n",
              message->SenderID);
      LOG_END
      peerListNode->Status &= ~(PLNS_MENTOR|PLNS_HTSYNC);   /* Synchronization completed */
   }
}


/* ###### Handle ENRP Presence ########################################### */
void registrarHandleENRPPresence(struct Registrar*       registrar,
                                 int                     fd,
                                 sctp_assoc_t            assocID,
                                 struct RSerPoolMessage* message)
{
   HandlespaceChecksumType        checksum;
   struct ST_CLASS(PeerListNode)* peerListNode;
   char                           remoteAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*  remoteAddressBlock = (struct TransportAddressBlock*)&remoteAddressBlockBuffer;
   char                           enrpTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*  enrpTransportAddressBlock = (struct TransportAddressBlock*)&enrpTransportAddressBlockBuffer;
   int                            result;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own Presence message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE2
   fprintf(stdlog, "Got Presence from peer $%08x",
           message->PeerListNodePtr->Identifier);
   if(message->PeerListNodePtr) {
      fputs(" at address ", stdlog);
      transportAddressBlockPrint(message->PeerListNodePtr->AddressBlock, stdlog);
   }
   fputs("\n", stdlog);
   LOG_END


   /* ====== Filter addresses ============================================ */
   if(fd == registrar->ENRPMulticastInputSocket) {
      transportAddressBlockNew(enrpTransportAddressBlock,
                               IPPROTO_SCTP,
                               (message->PeerListNodePtr->AddressBlock) ? message->PeerListNodePtr->AddressBlock->Port : getPort(&message->SourceAddress.sa),
                               0,
                               &message->SourceAddress,
                               1, MAX_PE_TRANSPORTADDRESSES);
#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Recv", "MulticastENRP", "Presence", "",  message->Flags, 0, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif
   }
   else {
#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Recv", "ENRP", "Presence", "", message->Flags, 0, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif
      if(transportAddressBlockGetAddressesFromSCTPSocket(remoteAddressBlock,
                                                         fd, assocID,
                                                         MAX_PE_TRANSPORTADDRESSES,
                                                         false) > 0) {
         LOG_VERBOSE3
         fputs("SCTP association's valid peer addresses: ", stdlog);
         transportAddressBlockPrint(remoteAddressBlock, stdlog);
         fputs("\n", stdlog);
         LOG_END

         /* ====== Filter addresses ====================================== */
         if(transportAddressBlockFilter(message->PeerListNodePtr->AddressBlock,
                                        remoteAddressBlock,
                                        enrpTransportAddressBlock,
                                        MAX_PE_TRANSPORTADDRESSES, true, registrar->MinEndpointAddressScope) == 0) {
            LOG_WARNING
            fprintf(stdlog, "Presence from peer $%08x contains no usable ENRP endpoint addresses\n",
                    message->PeerListNodePtr->Identifier);
            fputs("Addresses from message: ", stdlog);
            transportAddressBlockPrint(message->PeerListNodePtr->AddressBlock, stdlog);
            fputs("\n", stdlog);
            fputs("Addresses from association: ", stdlog);
            transportAddressBlockPrint(remoteAddressBlock, stdlog);
            fputs("\n", stdlog);
            LOG_END
            return;
         }
      }
      else {
         LOG_ERROR
         fprintf(stdlog, "Unable to obtain peer addresses of FD %d, assoc %u\n",
                 fd, (unsigned int)assocID);
         LOG_END
         return;
      }
   }
   LOG_VERBOSE2
   fprintf(stdlog, "Using the following ENRP endpoint addresses for peer $%08x: ",
           message->PeerListNodePtr->Identifier);
   transportAddressBlockPrint(enrpTransportAddressBlock, stdlog);
   fputs("\n", stdlog);
   LOG_END


   /* ====== Add/update peer ============================================= */
   if(message->PeerListNodePtr) {
      result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                  &registrar->Peers,
                  message->PeerListNodePtr->Identifier,
                  PLNF_DYNAMIC|PLNF_NEW,   /* The entry is assumed to be new */
                  enrpTransportAddressBlock,
                  getMicroTime(),
                  &peerListNode);

      if(result == RSPERR_OKAY) {
         /* ====== Send Presence to new peer ============================= */
         /* PLNF_NEW will be removed when the entry was not new. If it is
            still set, there is a new PR -> send him a Presence */
         if(peerListNode->Flags & PLNF_NEW) {
            LOG_ACTION
            fputs("Sending Presence to new peer ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
            fputs("\n", stdlog);
            LOG_END
            registrarSendENRPPresence(registrar,
                                      registrar->ENRPUnicastSocket,
                                      0, 0,
                                      peerListNode->AddressBlock->AddressArray,
                                      peerListNode->AddressBlock->Addresses,
                                      peerListNode->Identifier,
                                      true);
         }

         /* ====== Mentor query ========================================== */
         if( (registrar->InStartupPhase) &&
             (registrar->MentorServerID == UNDEFINED_REGISTRAR_IDENTIFIER) ) {
            LOG_ACTION
            fputs("Trying ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
            fputs(" as mentor server...\n", stdlog);
            LOG_END
            peerListNode->Status |= PLNS_LISTSYNC|PLNS_HTSYNC|PLNS_MENTOR;
            registrar->MentorServerID = peerListNode->Identifier;
            registrarSendENRPListRequest(registrar,
                                         registrar->ENRPUnicastSocket,
                                         0, 0,
                                         peerListNode->AddressBlock->AddressArray,
                                         peerListNode->AddressBlock->Addresses,
                                         peerListNode->Identifier);
            registrarSendENRPHandleTableRequest(registrar,
                                                registrar->ENRPUnicastSocket,
                                                0, 0,
                                                peerListNode->AddressBlock->AddressArray,
                                                peerListNode->AddressBlock->Addresses,
                                                peerListNode->Identifier,
                                                0x00);
         }

         /* ====== Check if synchronization is necessary ================= */
         if(assocID != 0) {
            if(!(peerListNode->Status & PLNS_HTSYNC)) {
               /* Attention: We only synchronize on SCTP-received Presence messages! */
               checksum = ST_CLASS(peerListNodeGetOwnershipChecksum)(
                             peerListNode);
               if(checksum != message->Checksum) {
                  LOG_ACTION
                  fprintf(stdlog, "Handle Table synchronization with peer $%08x is necessary -> requesting it...\n",
                          peerListNode->Identifier);
                  LOG_END
                  LOG_VERBOSE
                  fprintf(stdlog, "Checksum is $%x, should be $%x\n",
                          checksum, message->Checksum);
                  LOG_END

                  peerListNode->Status |= PLNS_HTSYNC;
                  registrarSendENRPHandleTableRequest(registrar, fd, assocID, 0,
                                                      NULL, 0,
                                                      peerListNode->Identifier,
                                                      EHF_HANDLE_TABLE_REQUEST_OWN_CHILDREN_ONLY);
                  ST_CLASS(poolHandlespaceManagementMarkPoolElementNodes)(&registrar->Handlespace,
                                                                          message->SenderID);
               }
            }
            else {
               LOG_ACTION
               fprintf(stdlog, "Synchronization with peer $%08x is still in progress -> no new synchronization\n",
                       peerListNode->Identifier);
               LOG_END
            }
            if(!(peerListNode->Status & PLNS_LISTSYNC)) {
               LOG_ACTION
               fprintf(stdlog, "Peer List synchronization with peer $%08x is necessary -> requesting it...\n",
                        peerListNode->Identifier);
               LOG_END
               peerListNode->Status |= PLNS_LISTSYNC;
               registrarSendENRPListRequest(registrar, fd, assocID, 0,
                                            NULL, 0,
                                            peerListNode->Identifier);
            }
         }

         /* ====== Activate keep alive timer ============================= */
         if(STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode)) {
            ST_CLASS(peerListManagementDeactivateTimer)(
               &registrar->Peers, peerListNode);
         }
         ST_CLASS(peerListManagementActivateTimer)(
            &registrar->Peers, peerListNode, PLNT_MAX_TIME_LAST_HEARD,
            getMicroTime() + registrar->PeerMaxTimeLastHeard);
         timerRestart(&registrar->PeerActionTimer,
                      ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                         &registrar->Peers));

         /* ====== Print debug information =============================== */
         LOG_VERBOSE3
         fputs("Peers:\n", stdlog);
         registrarDumpPeers(registrar);
         LOG_END

         if((message->Flags & EHF_PRESENCE_REPLY_REQUIRED) &&
            (fd != registrar->ENRPMulticastOutputSocket) &&
            (message->SenderID != UNDEFINED_REGISTRAR_IDENTIFIER)) {
            LOG_VERBOSE
            fputs("Presence with ReplyRequired flag set -> Sending reply...\n", stdlog);
            LOG_END
            registrarSendENRPPresence(registrar,
                                      fd, message->AssocID, 0,
                                      NULL, 0,
                                      message->SenderID,
                                      false);
         }
      }
      else {
         LOG_WARNING
         fputs("Failed to add peer ", stdlog);
         ST_CLASS(peerListNodePrint)(message->PeerListNodePtr, stdlog, PLPO_FULL);
         fputs(": ", stdlog);
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }

      /* ====== Takeover handling ======================================== */
      peerListNode->TakeoverRegistrarID = UNDEFINED_REGISTRAR_IDENTIFIER;
      if(peerListNode->TakeoverProcess) {
         takeoverProcessDelete(peerListNode->TakeoverProcess);
         peerListNode->TakeoverProcess = NULL;
      }
   }
}


/* ###### Send ENRP Presence ############################################# */
void registrarSendENRPPresence(struct Registrar*             registrar,
                               int                           sd,
                               const sctp_assoc_t            assocID,
                               int                           msgSendFlags,
                               const union sockaddr_union*   destinationAddressList,
                               const size_t                  destinationAddresses,
                               const RegistrarIdentifierType receiverID,
                               const bool                    replyRequired)
{
   struct RSerPoolMessage*       message;
   size_t                        messageLength;
   struct ST_CLASS(PeerListNode) peerListNode;
   char                          localAddressArrayBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* localAddressArray = (struct TransportAddressBlock*)&localAddressArrayBuffer;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type                      = EHT_PRESENCE;
      message->PPID                      = PPID_ENRP;
      message->AssocID                   = assocID;
      message->AddressArray              = (union sockaddr_union*)destinationAddressList;
      message->Addresses                 = destinationAddresses;
      message->Flags                     = replyRequired ? EHF_PRESENCE_REPLY_REQUIRED : 0x00;
      message->PeerListNodePtr           = &peerListNode;
      message->PeerListNodePtrAutoDelete = false;
      message->SenderID                  = registrar->ServerID;
      message->ReceiverID                = receiverID;
      message->Checksum                  = ST_CLASS(poolHandlespaceManagementGetOwnershipChecksum)(&registrar->Handlespace);

      if(transportAddressBlockGetAddressesFromSCTPSocket(localAddressArray,
                                                         registrar->ENRPUnicastSocket,
                                                         assocID,
                                                         MAX_PE_TRANSPORTADDRESSES,
                                                         true) > 0) {
         ST_CLASS(peerListNodeNew)(&peerListNode,
                                   registrar->ServerID, 0,
                                   localAddressArray);

         LOG_VERBOSE3
         fputs("Sending Presence using peer list entry: \n", stdlog);
         ST_CLASS(peerListNodePrint)(&peerListNode, stdlog, ~0);
         fputs("\n", stdlog);
         LOG_END

         messageLength = rserpoolMessage2Packet(message);
         if(messageLength > 0) {
            if(sd == registrar->ENRPMulticastOutputSocket) {
#ifdef ENABLE_REGISTRAR_STATISTICS
               registrarWriteActionLog(registrar, "Send", "MulticastENRP", "Presence", "", message->Flags, 0, 0,
                                       NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif
               if(sendmulticast(
                     registrar->ENRPMulticastOutputSocket,
                     registrar->ENRPMulticastOutputSocketFamily,
                     message->Buffer,
                     messageLength,
                     0,
                     &destinationAddressList->sa,
                     getSocklen(&destinationAddressList->sa),
                     registrar->AnnounceTTL) <= 0) {
                  LOG_WARNING
                  fputs("Sending Presence via multicast failed\n", stdlog);
                  LOG_END
               }
            }
            else {
#ifdef ENABLE_REGISTRAR_STATISTICS
               registrarWriteActionLog(registrar, "Send", "ENRP", "Presence", "", message->Flags, 0, 0,
                                       NULL, 0, message->SenderID, message->ReceiverID, 0, 0);
#endif
               if(rserpoolMessageSend(IPPROTO_SCTP,
                                      sd, assocID, msgSendFlags, 0, 0, message) == false) {
                  LOG_WARNING
                  fputs("Sending Presence via unicast failed\n", stdlog);
                  LOG_END
               }
            }
            rserpoolMessageDelete(message);
         }
      }
      else {
         LOG_ERROR
         fputs("Unable to obtain local ASAP socket addresses\n", stdlog);
         LOG_END
      }
   }
}


/* ###### Send ENRP Presence to all peers ################################ */
void registrarSendENRPPresenceToAllPeers(struct Registrar* registrar)
{
   struct ST_CLASS(PeerListNode)* peerListNode;

   peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
   while(peerListNode != NULL) {
      /* If MSG_SEND_TO_ALL is available, unicast messages must only be sent
         to static peers not yet connected. That is, their ID is 0. */
#ifdef MSG_SEND_TO_ALL
      if(peerListNode->Identifier == UNDEFINED_REGISTRAR_IDENTIFIER) {
#else
      {
#endif
         LOG_VERBOSE2
         fprintf(stdlog, "Sending Presence to unicast peer $%08x...\n",
                 peerListNode->Identifier);
         LOG_END
         registrarSendENRPPresence(registrar,
                                   registrar->ENRPUnicastSocket, 0, 0,
                                   peerListNode->AddressBlock->AddressArray,
                                   peerListNode->AddressBlock->Addresses,
                                   peerListNode->Identifier,
                                   false);
      }
      peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                        &registrar->Peers, peerListNode);
   }
#ifdef MSG_SEND_TO_ALL
#warning Using MSG_SEND_TO_ALL!
   registrarSendENRPPresence(registrar,
                             registrar->ENRPUnicastSocket, 0,
                             MSG_SEND_TO_ALL,
                             NULL, 0, 0, false);
#endif
}
