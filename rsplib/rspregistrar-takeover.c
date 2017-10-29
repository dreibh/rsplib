/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2018 by Thomas Dreibholz
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

#include "rspregistrar.h"


static void registrarFinishTakeover(struct Registrar*             registrar,
                                    const RegistrarIdentifierType targetID,
                                    struct TakeoverProcess*       takeoverProcess);


/* ###### Handle ENRP Init Takeover ###################################### */
void registrarHandleENRPInitTakeover(struct Registrar*       registrar,
                                     int                     fd,
                                     sctp_assoc_t            assocID,
                                     struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own InitTakeover message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got InitTakeover from peer $%08x for target $%08x\n",
           message->SenderID,
           message->RegistrarIdentifier);
   LOG_END
#ifdef ENABLE_REGISTRAR_STATISTICS
   registrarWriteActionLog(registrar, "Recv", "ENRP", "InitTakeover", "", 0, 0, 0,
                           NULL, 0, message->SenderID, message->ReceiverID, message->RegistrarIdentifier, 0);
#endif

   /* ====== We are the takeover target -> try to stop it by Peer Presence */
   if(message->RegistrarIdentifier == registrar->ServerID) {
      LOG_WARNING
      fprintf(stdlog, "Peer $%08x has initiated takeover -> trying to stop this by Presences\n",
              message->SenderID);
      LOG_END
      /* ====== Send presence as multicast announce ====================== */
      if(registrar->ENRPAnnounceViaMulticast) {
         registrarSendENRPPresence(registrar, registrar->ENRPMulticastOutputSocket, 0, 0,
                                   (union sockaddr_union*)&registrar->ENRPMulticastAddress, 1,
                                   0, false);
      }
      /* ====== Send presence to all peers =============================== */
      registrarSendENRPPresenceToAllPeers(registrar);
   }

   /* ====== Another peer is the takeover target ========================= */
   else {
      peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                              &registrar->Peers, message->RegistrarIdentifier, NULL);
      if(peerListNode) {
         /* ====== We have also started a takeover -> negotiation required */
         if(peerListNode->TakeoverProcess != NULL) {
            if(message->SenderID < registrar->ServerID) {
               LOG_ACTION
               fprintf(stdlog, "Peer $%08x, also trying takeover of $%08x, has lower ID -> we ($%08x) abort our takeover\n",
                       message->SenderID,
                       message->RegistrarIdentifier,
                       registrar->ServerID);
               LOG_END
               /* Acknowledge peer's takeover */
               peerListNode->TakeoverRegistrarID = message->SenderID;
               registrarSendENRPInitTakeoverAck(registrar,
                                                fd, assocID,
                                                message->SenderID, message->RegistrarIdentifier);
               /* Cancel our takeover process */
               takeoverProcessDelete(peerListNode->TakeoverProcess);
               peerListNode->TakeoverProcess = NULL;
               /* Schedule a max-time-no-response timer */
               ST_CLASS(peerListManagementDeactivateTimer)(
                  &registrar->Peers, peerListNode);
               ST_CLASS(peerListManagementActivateTimer)(
                  &registrar->Peers, peerListNode, PLNT_MAX_TIME_NO_RESPONSE,
                  getMicroTime() + registrar->PeerMaxTimeNoResponse);
            }
            else {
               LOG_ACTION
               fprintf(stdlog, "Peer $%08x, also trying takeover of $%08x, has higher ID -> we ($%08x) continue our takeover\n",
                       message->SenderID,
                       message->RegistrarIdentifier,
                       registrar->ServerID);
               LOG_END
            }
         }

         /* ====== Acknowledge takeover ================================== */
         else {
            LOG_ACTION
            fprintf(stdlog, "Acknowledging peer $%08x's takeover of $%08x\n",
                  message->SenderID,
                  message->RegistrarIdentifier);
            LOG_END
            peerListNode->TakeoverRegistrarID = message->SenderID;
            registrarSendENRPInitTakeoverAck(registrar,
                                             fd, assocID,
                                             message->SenderID, message->RegistrarIdentifier);
         }
      }

      /* ====== The target is unknown. Agree to takeover. ================ */
      else {
         LOG_WARNING
         fprintf(stdlog, "Acknowledging peer $%08x's takeover of $%08x (which is unknown for us!)\n",
               message->SenderID,
               message->RegistrarIdentifier);
         LOG_END
         registrarSendENRPInitTakeoverAck(registrar,
                                          fd, assocID,
                                          message->SenderID, message->RegistrarIdentifier);
      }
   }
}


/* ###### Send ENRP Init Takeover ######################################## */
void registrarSendENRPInitTakeoverToAllPeers(struct Registrar*             registrar,
                                             const RegistrarIdentifierType targetID)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct RSerPoolMessage*        message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type         = EHT_INIT_TAKEOVER;
      message->Flags        = 0x00;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = 0;
      message->RegistrarIdentifier = targetID;

#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Send", "ENRP", "InitTakeover", "", 0, 0, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, message->RegistrarIdentifier, 0);
#endif

      peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
      while(peerListNode != NULL) {
         message->ReceiverID   = peerListNode->Identifier;
         message->AddressArray = peerListNode->AddressBlock->AddressArray;
         message->Addresses    = peerListNode->AddressBlock->Addresses;
         LOG_VERBOSE
         fprintf(stdlog, "Sending InitTakeover to unicast peer $%08x...\n",
                 peerListNode->Identifier);
         LOG_END
         rserpoolMessageSend(IPPROTO_SCTP,
                             registrar->ENRPUnicastSocket,
                             0, 0, 0, 0,
                             message);
         peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers, peerListNode);
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Handle ENRP Init Takeover Ack ################################## */
void registrarHandleENRPInitTakeoverAck(struct Registrar*       registrar,
                                        int                     fd,
                                        sctp_assoc_t            assocID,
                                        struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   size_t                         acksToGo;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own InitTakeoverAck message\n", stdlog);
      LOG_END
      return;
   }

   peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                     &registrar->Peers, message->RegistrarIdentifier, NULL);
   if((peerListNode) && (peerListNode->TakeoverProcess)) {
      LOG_VERBOSE
      fprintf(stdlog, "Got InitTakeoverAck from peer $%08x for target $%08x\n",
            message->SenderID,
            message->RegistrarIdentifier);
      LOG_END

      acksToGo = takeoverProcessAcknowledge(peerListNode->TakeoverProcess,
                                            message->RegistrarIdentifier,
                                            message->SenderID);

#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Recv", "ENRP", "InitTakeoverAck", "", 0, acksToGo, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, message->RegistrarIdentifier, 0);
#endif

      if(acksToGo > 0) {
         LOG_ACTION
         fprintf(stdlog, "Peer $%08x acknowledges takeover of target $%08x. %u acknowledges to go.\n",
               message->SenderID,
               message->RegistrarIdentifier,
               (unsigned int)takeoverProcessGetOutstandingAcks(peerListNode->TakeoverProcess));
         LOG_END
      }
      else {
         LOG_ACTION
         fprintf(stdlog, "Takeover of target $%08x confirmed. Taking it over now.\n",
                 message->RegistrarIdentifier);
         LOG_END
         registrarFinishTakeover(registrar, message->RegistrarIdentifier,
                                 peerListNode->TakeoverProcess);
         takeoverProcessDelete(peerListNode->TakeoverProcess);
         peerListNode->TakeoverProcess = NULL;
         ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                  &registrar->Peers,
                  peerListNode);
      }
   }
   else {
      LOG_VERBOSE
      fprintf(stdlog, "Ignoring InitTakeoverAck from peer $%08x for target $%08x\n",
              message->SenderID,
              message->RegistrarIdentifier);
      LOG_END
   }
}


/* ###### Send ENRP Init Takeover Ack #################################### */
void registrarSendENRPInitTakeoverAck(struct Registrar*             registrar,
                                      const int                     sd,
                                      const sctp_assoc_t            assocID,
                                      const RegistrarIdentifierType receiverID,
                                      const RegistrarIdentifierType targetID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type         = EHT_INIT_TAKEOVER_ACK;
      message->AssocID      = assocID;
      message->Flags        = 0x00;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = receiverID;
      message->RegistrarIdentifier = targetID;

      LOG_ACTION
      fprintf(stdlog, "Sending InitTakeoverAck for target $%08x to peer $%08x...\n",
              message->RegistrarIdentifier,
              message->ReceiverID);
      LOG_END
#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Send", "ENRP", "InitTakeoverAck", "", 0, 0, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, message->RegistrarIdentifier, 0);
#endif

      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, 0, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending InitTakeoverAck failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Take over all PEs from dead peer ############################### */
static void registrarFinishTakeover(struct Registrar*             registrar,
                                    const RegistrarIdentifierType targetID,
                                    struct TakeoverProcess*       takeoverProcess)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;

   LOG_WARNING
   fprintf(stdlog, "Taking over peer $%08x...\n", targetID);
   LOG_END

   /* ====== Send Takeover Servers ======================================= */
   registrarSendENRPTakeoverServerToAllPeers(registrar, targetID);

   /* ====== Update PEs' home PR identifier ============================== */
   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &registrar->Handlespace.Handlespace, targetID);
   while(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                               &registrar->Handlespace.Handlespace, poolElementNode);

      LOG_ACTION
      fprintf(stdlog, "Taking ownership of pool element $%08x in pool ",
              poolElementNode->Identifier);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END

      /* Deactivate expiry timer */
      ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
         &registrar->Handlespace.Handlespace,
         poolElementNode);

      /* Update registrarHandlespace */
      ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
         &registrar->Handlespace.Handlespace,
         poolElementNode,
         registrar->ServerID);

      /* Tell node about new home PR */
      registrarSendASAPEndpointKeepAlive(registrar, poolElementNode, true);

      /* Schedule endpoint keep-alive timeout */
      ST_CLASS(poolHandlespaceNodeActivateTimer)(
         &registrar->Handlespace.Handlespace,
         poolElementNode,
         PENT_KEEPALIVE_TIMEOUT,
         getMicroTime() + registrar->EndpointKeepAliveTimeoutInterval);

      poolElementNode = nextPoolElementNode;
   }

   /* ====== Restart the registrarHandlespace action timer ======================== */
   timerRestart(&registrar->HandlespaceActionTimer,
                ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                   &registrar->Handlespace));

   /* ====== Remove takeover process ===================================== */
   LOG_WARNING
   fprintf(stdlog, "Takeover of peer $%08x completed\n", targetID);
   LOG_END
}


/* ###### Handle ENRP Init Takeover Server ############################### */
void registrarHandleENRPTakeoverServer(struct Registrar*       registrar,
                                       int                     fd,
                                       sctp_assoc_t            assocID,
                                       struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own TakeoverServer message\n", stdlog);
      LOG_END
      return;
   }

   LOG_ACTION
   fprintf(stdlog, "Got TakeoverServer from peer $%08x for target $%08x\n",
           message->SenderID,
           message->RegistrarIdentifier);
   LOG_END

#ifdef ENABLE_REGISTRAR_STATISTICS
   registrarWriteActionLog(registrar, "Recv", "ENRP", "TakeoverServer", "", 0, 0, 0,
                           NULL, 0, message->SenderID, message->ReceiverID, message->RegistrarIdentifier, 0);
#endif


   /* ====== Update PEs' home PR identifier ============================== */
   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &registrar->Handlespace.Handlespace, message->RegistrarIdentifier);
   while(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                               &registrar->Handlespace.Handlespace, poolElementNode);

      LOG_ACTION
      fprintf(stdlog, "Changing ownership of pool element $%08x in pool ",
              poolElementNode->Identifier);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END

      /* Update registrarHandlespace */
      ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
         &registrar->Handlespace.Handlespace,
         poolElementNode,
         message->SenderID);

      poolElementNode = nextPoolElementNode;
   }
}


/* ###### Send ENRP Init Takeover Server ################################# */
static void registrarSendENRPTakeoverServer(struct Registrar*             registrar,
                                            int                           sd,
                                            const sctp_assoc_t            assocID,
                                            int                           msgSendFlags,
                                            const union sockaddr_union*   destinationAddressList,
                                            const size_t                  destinationAddresses,
                                            const RegistrarIdentifierType receiverID,
                                            const RegistrarIdentifierType targetID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type                = EHT_TAKEOVER_SERVER;
      message->PPID                = PPID_ENRP;
      message->AssocID             = assocID;
      message->AddressArray        = (union sockaddr_union*)destinationAddressList;
      message->Addresses           = destinationAddresses;
      message->Flags               = 0x00;
      message->SenderID            = registrar->ServerID;
      message->ReceiverID          = receiverID;
      message->RegistrarIdentifier = targetID;

#ifdef ENABLE_REGISTRAR_STATISTICS
      registrarWriteActionLog(registrar, "Send", "ENRP", "TakeoverServer", "", 0, 0, 0,
                              NULL, 0, message->SenderID, message->ReceiverID, message->RegistrarIdentifier, 0);
#endif

      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending TakeoverServer failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Send ENRP takeover server message to all peers ################# */
void registrarSendENRPTakeoverServerToAllPeers(struct Registrar*             registrar,
                                               const RegistrarIdentifierType targetID)
{
#ifndef MSG_SEND_TO_ALL
   struct ST_CLASS(PeerListNode)* peerListNode;

   peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
   while(peerListNode != NULL) {
      if(peerListNode->Identifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
         registrarSendENRPTakeoverServer(registrar,
                                         registrar->ENRPUnicastSocket, 0, 0,
                                         NULL, 0,
                                         peerListNode->Identifier,
                                         targetID);
      }
      peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                        &registrar->Peers, peerListNode);
   }
#else
#warning Using MSG_SEND_TO_ALL!
   registrarSendENRPTakeoverServer(registrar,
                                   registrar->ENRPUnicastSocket, 0,
                                   MSG_SEND_TO_ALL,
                                   NULL, 0,
                                   UNDEFINED_REGISTRAR_IDENTIFIER,
                                   targetID);
#endif
}
