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
 * Copyright (C) 2002-2024 by Thomas Dreibholz
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


/* ###### Dump handlespace ################################################# */
void registrarDumpHandlespace(struct Registrar* registrar)
{
   fputs("*************** Handlespace Dump ***************\n", stdlog);
   printTimeStamp(stdlog);
   fputs("\n", stdlog);
   ST_CLASS(poolHandlespaceManagementPrint)(&registrar->Handlespace, stdlog,
            PNNPO_POOLS_INDEX|PNNPO_POOLS_SELECTION|PNNPO_POOLS_TIMER|PENPO_USERTRANSPORT|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PENPO_HOME_PR);
   fputs("**********************************************\n", stdlog);
}


/* ###### Dump peers ##################################################### */
void registrarDumpPeers(struct Registrar* registrar)
{
   fputs("*************** Peers ************************\n", stdlog);
   ST_CLASS(peerListManagementPrint)(&registrar->Peers, stdlog, PLPO_FULL);
   fputs("**********************************************\n", stdlog);
}


/* ###### Initialization is complete ##################################### */
void registrarBeginNormalOperation(struct Registrar* registrar,
                                   const bool        initializedFromMentor)
{
   LOG_NOTE
   if(initializedFromMentor) {
      fputs("Initialization phase ended after obtaining handlespace from mentor server. The registrar is ready!\n", stdlog);
   }
   else {
      fputs("Initialization phase ended after ENRP mentor discovery timeout. The registrar is ready!\n", stdlog);
   }
   LOG_END

   registrar->InStartupPhase = false;
   if(registrar->ASAPSendAnnounces) {
      LOG_ACTION
      fputs("Starting to send ASAP announcements\n", stdlog);
      LOG_END
      timerStart(&registrar->ASAPAnnounceTimer, 0);
   }
}


/* ###### Handle incoming message ######################################## */
void registrarHandleMessage(struct Registrar*       registrar,
                            struct RSerPoolMessage* message,
                            int                     sd)
{
#ifdef ENABLE_REGISTRAR_STATISTICS
   unsigned long long now;
#endif

   if(sd == registrar->ENRPMulticastInputSocket) {
      if(message->Type == EHT_PRESENCE) {
         registrarHandleENRPPresence(registrar, sd, 0, message);
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "Got unsupported message type $%02x on ENRP multicast socket\n", message->Type);
         LOG_END
      }
   }
   else if(sd == registrar->ASAPSocket) {
      switch(message->Type) {
         case AHT_REGISTRATION:
            registrarHandleASAPRegistration(registrar, sd, message->AssocID, message);
          break;
         case AHT_HANDLE_RESOLUTION:
            registrarHandleASAPHandleResolution(registrar, sd, message->AssocID, message);
          break;
         case AHT_DEREGISTRATION:
            registrarHandleASAPDeregistration(registrar, sd, message->AssocID, message);
          break;
         case AHT_ENDPOINT_KEEP_ALIVE_ACK:
            registrarHandleASAPEndpointKeepAliveAck(registrar, sd, message->AssocID, message);
          break;
         case AHT_ENDPOINT_UNREACHABLE:
            registrarHandleASAPEndpointUnreachable(registrar, sd, message->AssocID, message);
          break;
         default:
            LOG_WARNING
            fprintf(stdlog, "Got unsupported message type $%02x on ASAP unicast socket\n",
                    message->Type);
            LOG_END
          break;
      }
   }
   else if(sd == registrar->ENRPUnicastSocket) {
      switch(message->Type) {
         case EHT_PRESENCE:
            registrarHandleENRPPresence(registrar, sd, message->AssocID, message);
          break;
         case EHT_HANDLE_UPDATE:
            registrarHandleENRPHandleUpdate(registrar, sd, message->AssocID, message);
          break;
         case EHT_LIST_REQUEST:
            registrarHandleENRPListRequest(registrar, sd, message->AssocID, message);
          break;
         case EHT_LIST_RESPONSE:
            registrarHandleENRPListResponse(registrar, sd, message->AssocID, message);
          break;
         case EHT_HANDLE_TABLE_REQUEST:
            registrarHandleENRPHandleTableRequest(registrar, sd, message->AssocID, message);
          break;
         case EHT_HANDLE_TABLE_RESPONSE:
            registrarHandleENRPHandleTableResponse(registrar, sd, message->AssocID, message);
          break;
         case EHT_INIT_TAKEOVER:
            registrarHandleENRPInitTakeover(registrar, sd, message->AssocID, message);
          break;
         case EHT_INIT_TAKEOVER_ACK:
            registrarHandleENRPInitTakeoverAck(registrar, sd, message->AssocID, message);
          break;
         case EHT_TAKEOVER_SERVER:
            registrarHandleENRPTakeoverServer(registrar, sd, message->AssocID, message);
          break;
         default:
            LOG_WARNING
            fprintf(stdlog, "Got unsupported message type $%02x on ENRP unicast socket\n",
                    message->Type);
            LOG_END
          break;
      }
   }

#ifdef ENABLE_REGISTRAR_STATISTICS
   if(registrar->Stats.NeedsWeightedStatValues) {
      now = getMicroTime();
      updateWeightedStatValue(&registrar->Stats.PoolsCount, now, ST_CLASS(poolHandlespaceManagementGetPools)(&registrar->Handlespace));
      updateWeightedStatValue(&registrar->Stats.PoolElementsCount, now, ST_CLASS(poolHandlespaceManagementGetPoolElements)(&registrar->Handlespace));
      updateWeightedStatValue(&registrar->Stats.OwnedPoolElementsCount, now, ST_CLASS(poolHandlespaceManagementGetOwnedPoolElements)(&registrar->Handlespace));
      updateWeightedStatValue(&registrar->Stats.PeersCount, now, ST_CLASS(peerListManagementGetPeers)(&registrar->Peers));
   }
#endif
}


/* ###### Handle events on sockets ####################################### */
void registrarHandleSocketEvent(struct Dispatcher* dispatcher,
                                int                fd,
                                unsigned int       eventMask,
                                void*              userData)
{
   struct Registrar*        registrar = (struct Registrar*)userData;
   struct RSerPoolMessage*  message;
   union sctp_notification* notification;
   union sockaddr_union     remoteAddress;
   socklen_t                remoteAddressLength;
   struct MessageBuffer*    messageBuffer;
   int                      flags;
   uint32_t                 ppid;
   sctp_assoc_t             assocID;
   unsigned short           streamID;
   ssize_t                  received;
   unsigned int             result;

   CHECK((fd == registrar->ASAPSocket) ||
         (fd == registrar->ENRPUnicastSocket) ||
         ((registrar->ENRPMulticastInputSocket >= 0) && (fd == registrar->ENRPMulticastInputSocket)));
   LOG_VERBOSE3
   fprintf(stdlog, "Event on socket %d...\n", fd);
   LOG_END

   if(fd == registrar->ASAPSocket) {
      messageBuffer = registrar->ASAPMessageBuffer;
   }
   else if(fd == registrar->ENRPUnicastSocket) {
      messageBuffer = registrar->ENRPUnicastMessageBuffer;
   }
   else {
      messageBuffer = registrar->UDPMessageBuffer;
   }

   flags               = 0;
   remoteAddressLength = sizeof(remoteAddress);
   received = messageBufferRead(messageBuffer, fd, &flags,
                                (struct sockaddr*)&remoteAddress,
                                &remoteAddressLength,
                                &ppid, &assocID, &streamID, 0);
   if(received > 0) {
      if(!(flags & MSG_NOTIFICATION)) {
         if(!( (((ppid == PPID_ASAP) && (fd != registrar->ASAPSocket)) ||
                ((ppid == PPID_ENRP) && (fd != registrar->ENRPUnicastSocket))) )) {

            if(fd == registrar->ENRPMulticastInputSocket) {
               /* ENRP via UDP -> Set PPID so that rserpoolPacket2Message can
                  correctly decode the packet */
               ppid = PPID_ENRP;
            }

            result = rserpoolPacket2Message(messageBuffer->Buffer,
                                            &remoteAddress, assocID, ppid,
                                            received, messageBuffer->BufferSize, &message);
            if(message != NULL) {
               if((result == RSPERR_OKAY) && (message->Error == RSPERR_OKAY)) {
                  message->BufferAutoDelete = false;
                  LOG_VERBOSE3
                  fprintf(stdlog, "Got %u bytes message from ", (unsigned int)message->BufferSize);
                  fputaddress((struct sockaddr*)&remoteAddress, true, stdlog);
                  fprintf(stdlog, ", assoc #%u, PPID $%x\n",
                           (unsigned int)message->AssocID, message->PPID);
                  LOG_END

                  registrarHandleMessage(registrar, message, fd);
               }
               else if( (message->Error != RSPERR_UNRECOGNIZED_PARAMETER_SILENT) &&
                        ( (fd == registrar->ASAPSocket) || (fd == registrar->ENRPUnicastSocket) ) &&
                        (message->Type != AHT_ERROR) &&
                        (message->Type != EHT_ERROR) ) {
                  LOG_WARNING
                  fprintf(stdlog, "Sending %s Error message in reply to message type $%02x: ",
                          (message->PPID == PPID_ASAP) ? "ASAP" : "ENRP",
                          message->Type & 0xff);
                  rserpoolErrorPrint(message->Error, stdlog);
                  fputs("\n", stdlog);
                  LOG_END
                  if((ppid == PPID_ASAP) || (ppid == PPID_ENRP)) {
                     if(message->OffendingParameterTLV) {
                        message->ErrorCauseParameterTLV           = (char*)memdup(message->OffendingParameterTLV, message->OffendingParameterTLVLength);
                        message->ErrorCauseParameterTLVLength     = message->OffendingParameterTLVLength;
                        message->ErrorCauseParameterTLVAutoDelete = true;
                     }

                     /* For ASAP or ENRP messages, we can reply
                        error message */
                     if(message->PPID == PPID_ASAP) {
                        message->Type = AHT_ERROR;
                     }
                     else if(message->PPID == PPID_ENRP) {
                        message->Type = EHT_ERROR;
                     }
                     rserpoolMessageSend(IPPROTO_SCTP,
                                         fd, assocID, 0, 0, 0, message);
                  }
               }
               rserpoolMessageDelete(message);
            }
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Received PPID $%08x on wrong socket -> Sending ABORT to assoc %u!\n",
                    ppid, (unsigned int)assocID);
            LOG_END
            sendabort(fd, assocID);
         }
      }
      else {
         notification = (union sctp_notification*)messageBuffer->Buffer;
         switch(notification->sn_header.sn_type) {
            case SCTP_ASSOC_CHANGE:
               if(notification->sn_assoc_change.sac_state == SCTP_COMM_LOST) {
                  LOG_ACTION
                  fprintf(stdlog, "Association communication lost for socket %d, assoc %u\n",
                          registrar->ASAPSocket,
                          (unsigned int)notification->sn_assoc_change.sac_assoc_id);

                  LOG_END
                  registrarRemovePoolElementsOfConnection(registrar, fd,
                                                          notification->sn_assoc_change.sac_assoc_id);
               }
               else if(notification->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP) {
                  LOG_ACTION
                  fprintf(stdlog, "Association shutdown completed for socket %d, assoc %u\n",
                          registrar->ASAPSocket,
                          (unsigned int)notification->sn_assoc_change.sac_assoc_id);

                  LOG_END
                  registrarRemovePoolElementsOfConnection(registrar, fd,
                                                          notification->sn_assoc_change.sac_assoc_id);
               }
               break;
            case SCTP_SHUTDOWN_EVENT:
               LOG_ACTION
               fprintf(stdlog, "Shutdown event for socket %d, assoc %u\n",
                       registrar->ASAPSocket,
                       (unsigned int)notification->sn_shutdown_event.sse_assoc_id);

               LOG_END
               registrarRemovePoolElementsOfConnection(registrar, fd,
                                                       notification->sn_shutdown_event.sse_assoc_id);
               break;
         }
      }
   }
   else if(received != MBRead_Partial) {
      LOG_WARNING
      logerror("Unable to read from registrar socket");
      LOG_END
   }
}
