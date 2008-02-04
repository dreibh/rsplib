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
 * Copyright (C) 2002-2008 by Thomas Dreibholz
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

#include "registrar.h"


/* ###### ASAP Announce timer ############################################ */
void registrarHandleASAPAnnounceTimer(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData)
{
   struct Registrar*       registrar = (struct Registrar*)userData;
   struct RSerPoolMessage* message;
   size_t                  messageLength;

   CHECK(registrar->ASAPSendAnnounces == true);
   CHECK(registrar->ASAPAnnounceSocket >= 0);

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type                = AHT_SERVER_ANNOUNCE;
      message->Flags               = 0x00;
      message->RegistrarIdentifier = registrar->ServerID;
      messageLength = rserpoolMessage2Packet(message);
      if(messageLength > 0) {
         if(sendMulticastOverAllInterfaces(
               registrar->ASAPAnnounceSocket,
               registrar->ASAPAnnounceSocketFamily,
               message->Buffer,
               messageLength,
               0,
               (struct sockaddr*)&registrar->ASAPAnnounceAddress,
               getSocklen((struct sockaddr*)&registrar->ASAPAnnounceAddress)) <= 0) {
            LOG_WARNING
            logerror("Unable to send announce");
            LOG_END
         }
      }
      rserpoolMessageDelete(message);
   }
   timerStart(timer, getMicroTime() + registrarRandomizeCycle(registrar->ServerAnnounceCycle));
}


/* ###### Handle handlespace management timers ########################### */
void registrarHandlePoolElementEvent(struct Dispatcher* dispatcher,
                                     struct Timer*      timer,
                                     void*              userData)
{
   struct Registrar*                 registrar = (struct Registrar*)userData;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   unsigned int                      result;

   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)(
                        &registrar->Handlespace.Handlespace);
   while((poolElementNode != NULL) &&
         (poolElementNode->TimerTimeStamp <= getMicroTime())) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementTimerNode)(
                               &registrar->Handlespace.Handlespace,
                               poolElementNode);

      if(poolElementNode->TimerCode == PENT_KEEPALIVE_TRANSMISSION) {
         ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
            &registrar->Handlespace.Handlespace,
            poolElementNode);

         registrarSendASAPEndpointKeepAlive(registrar, poolElementNode, false);

         ST_CLASS(poolHandlespaceNodeActivateTimer)(
            &registrar->Handlespace.Handlespace,
            poolElementNode,
            PENT_KEEPALIVE_TIMEOUT,
            getMicroTime() + registrar->EndpointKeepAliveTimeoutInterval);
      }

      else if( (poolElementNode->TimerCode == PENT_KEEPALIVE_TIMEOUT) ||
               (poolElementNode->TimerCode == PENT_EXPIRY) ) {
         if(poolElementNode->TimerCode == PENT_KEEPALIVE_TIMEOUT) {
            LOG_ACTION
            fprintf(stdlog, "Keep alive timeout expired for pool element $%08x of pool ",
                  poolElementNode->Identifier);
            poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
            fputs(" -> removing it\n", stdlog);
            LOG_END
         }
         else {
            LOG_ACTION
            fprintf(stdlog, "Expiry timeout expired for pool element $%08x of pool ",
                    poolElementNode->Identifier);
            poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
            fputs(" -> removing it\n", stdlog);
            LOG_END
         }

         if(poolElementNode->HomeRegistrarIdentifier == registrar->ServerID) {
            /* We own this PE -> send HandleUpdate for its removal. */
            registrarSendENRPHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
         }

         result = ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                     &registrar->Handlespace,
                     poolElementNode);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Deregistration successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Handlespace content:\n", stdlog);
            registrarDumpHandlespace(registrar);
            LOG_END
         }
         else {
            LOG_ERROR
            fprintf(stdlog, "Failed to deregister for pool element $%08x of pool ",
                    poolElementNode->Identifier);
            poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
            fputs(": ", stdlog);
            rserpoolErrorPrint(result, stdlog);
            fputs("\n", stdlog);
            LOG_END_FATAL
         }
      }

      else {
         LOG_ERROR
         fputs("Unexpected timer\n", stdlog);
         LOG_END_FATAL
      }

      poolElementNode = nextPoolElementNode;
   }

   timerRestart(&registrar->HandlespaceActionTimer,
                ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                   &registrar->Handlespace));
}


/* ###### Remove all PEs registered via a given connection ############### */
void registrarRemovePoolElementsOfConnection(struct Registrar*  registrar,
                                             const int          sd,
                                             const sctp_assoc_t assocID)
{
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode =
      ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNodeForConnection)(
         &registrar->Handlespace.Handlespace,
         sd, assocID);
   unsigned int                      result;

   if(poolElementNode) {
      LOG_ACTION
      fprintf(stdlog,
              "Removing all pool elements registered by user socket %u, assoc %u...\n",
              sd, (unsigned int)assocID);
      LOG_END

      do {
         nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(
                                  &registrar->Handlespace.Handlespace,
                                  poolElementNode);
         LOG_VERBOSE
         fprintf(stdlog, "Removing pool element $%08x of pool ",
                 poolElementNode->Identifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
         fputs("\n", stdlog);
         LOG_END

         if(poolElementNode->HomeRegistrarIdentifier == registrar->ServerID) {
            /* We own this PE -> send HandleUpdate for its removal. */
            registrarSendENRPHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
         }

         result = ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                     &registrar->Handlespace,
                     poolElementNode);
         if(result == RSPERR_OKAY) {
            LOG_VERBOSE2
            fputs("Pool element successfully removed\n", stdlog);
            LOG_END
         }
         else {
            LOG_ERROR
            fprintf(stdlog, "Failed to deregister for pool element $%08x of pool ",
                    poolElementNode->Identifier);
            poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
            fputs(": ", stdlog);
            rserpoolErrorPrint(result, stdlog);
            fputs("\n", stdlog);
            LOG_END_FATAL
         }

         poolElementNode = nextPoolElementNode;
      } while(poolElementNode != NULL);

      LOG_VERBOSE3
      fputs("Handlespace content:\n", stdlog);
      registrarDumpHandlespace(registrar);
      LOG_END
   }
}


/* ###### Handle ASAP Registration ####################################### */
void registrarHandleASAPRegistration(struct Registrar*       registrar,
                                     int                     fd,
                                     sctp_assoc_t            assocID,
                                     struct RSerPoolMessage* message)
{

   char                              remoteAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     remoteAddressBlock = (struct TransportAddressBlock*)&remoteAddressBlockBuffer;
   char                              asapTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     asapTransportAddressBlock = (struct TransportAddressBlock*)&asapTransportAddressBlockBuffer;
   char                              userTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     userTransportAddressBlock = (struct TransportAddressBlock*)&userTransportAddressBlockBuffer;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct PoolPolicySettings         updatedPolicySettings;
   unsigned int                      distance;

   LOG_ACTION
   fputs("Registration request for ", stdlog);
   poolHandlePrint(&message->Handle, stdlog);
   fprintf(stdlog, "/$%08x\n", message->PoolElementPtr->Identifier);
   LOG_END

   message->Type       = AHT_REGISTRATION_RESPONSE;
   message->Flags      = 0x00;
   message->Error      = RSPERR_OKAY;
   message->Identifier = message->PoolElementPtr->Identifier;

   /* ====== Get peer addresses ========================================== */
   if(transportAddressBlockGetAddressesFromSCTPSocket(remoteAddressBlock,
                                                      fd, assocID,
                                                      MAX_PE_TRANSPORTADDRESSES,
                                                      false) > 0) {
      LOG_VERBOSE2
      fputs("SCTP association's valid peer addresses: ", stdlog);
      transportAddressBlockPrint(remoteAddressBlock, stdlog);
      fputs("\n", stdlog);
      LOG_END

      /* ====== Filter addresses ========================================= */
      if(transportAddressBlockFilter(message->PoolElementPtr->UserTransport,
                                     remoteAddressBlock,
                                     userTransportAddressBlock,
                                     MAX_PE_TRANSPORTADDRESSES, false, registrar->MinEndpointAddressScope) > 0) {
         LOG_VERBOSE2
         fputs("Filtered user transport addresses: ", stdlog);
         transportAddressBlockPrint(userTransportAddressBlock, stdlog);
         fputs("\n", stdlog);
         LOG_END
         if(transportAddressBlockFilter(remoteAddressBlock,
                                        NULL,
                                        asapTransportAddressBlock,
                                        MAX_PE_TRANSPORTADDRESSES, true, registrar->MinEndpointAddressScope) > 0) {
            LOG_VERBOSE2
            fputs("Filtered ASAP transport addresses: ", stdlog);
            transportAddressBlockPrint(asapTransportAddressBlock, stdlog);
            fputs("\n", stdlog);
            LOG_END

            /* ====== Set distance for distance-sensitive policies ======= */
            distance = 0xffffffff;
            registrarUpdateDistance(registrar,
                                    fd, assocID, message->PoolElementPtr,
                                    &updatedPolicySettings, false, &distance);

            message->Error = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                              &registrar->Handlespace,
                              &message->Handle,
                              registrar->ServerID,
                              message->PoolElementPtr->Identifier,
                              message->PoolElementPtr->RegistrationLife,
                              &updatedPolicySettings,
                              userTransportAddressBlock,
                              asapTransportAddressBlock,
                              fd, assocID,
                              getMicroTime(),
                              &poolElementNode);
            if(message->Error == RSPERR_OKAY) {
               /* ====== Successful registration ============================ */
               LOG_ACTION
               fputs("Successfully registered ", stdlog);
               poolHandlePrint(&message->Handle, stdlog);
               fprintf(stdlog, "/$%08x\n", poolElementNode->Identifier);
               LOG_END
               LOG_VERBOSE2
               fputs("Registered pool element: ", stdlog);
               ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
               fputs("\n", stdlog);
               LOG_END

               /* ====== Tune SCTP association ============================== */
/*
               tags[0].Tag = TAG_TuneSCTP_MinRTO;
               tags[0].Data = 500;
               tags[1].Tag = TAG_TuneSCTP_MaxRTO;
               tags[1].Data = 1000;
               tags[2].Tag = TAG_TuneSCTP_InitialRTO;
               tags[2].Data = 750;
               tags[3].Tag = TAG_TuneSCTP_Heartbeat;
               tags[3].Data = (registrar->EndpointMonitoringHeartbeatInterval / 1000);
               tags[4].Tag = TAG_TuneSCTP_PathMaxRXT;
               tags[4].Data = 3;
               tags[5].Tag = TAG_TuneSCTP_AssocMaxRXT;
               tags[5].Data = 9;
               tags[6].Tag = TAG_DONE;
               if(!tuneSCTP(fd, assocID, (struct TagItem*)&tags)) {
                  LOG_WARNING
                  fprintf(stdlog, "Unable to tune SCTP association %u's parameters\n",
                        (unsigned int)assocID);
                  LOG_END
               }
*/

               /* ====== Activate keep alive timer ========================== */
               if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
                  ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
                     &registrar->Handlespace.Handlespace,
                     poolElementNode);

                  registrar->ReregistrationCount++;   /* We have a re-registration here */
               }
               else {
                  registrar->RegistrationCount++;   /* We have a new registration here */
               }
               ST_CLASS(poolHandlespaceNodeActivateTimer)(
                  &registrar->Handlespace.Handlespace,
                  poolElementNode,
                  PENT_KEEPALIVE_TRANSMISSION,
                  getMicroTime() + registrar->EndpointKeepAliveTransmissionInterval);
               timerRestart(&registrar->HandlespaceActionTimer,
                            ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                               &registrar->Handlespace));

               /* ====== Print debug information ============================ */
               LOG_VERBOSE3
               fputs("Handlespace content:\n", stdlog);
               registrarDumpHandlespace(registrar);
               LOG_END

               /* ====== Send update to peers =============================== */
               registrarSendENRPHandleUpdate(registrar, poolElementNode, PNUP_ADD_PE);
            }
            else {
               LOG_WARNING
               fputs("Failed to register to pool ", stdlog);
               poolHandlePrint(&message->Handle, stdlog);
               fputs(" pool element ", stdlog);
               ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
               fputs(": ", stdlog);
               rserpoolErrorPrint(message->Error, stdlog);
               fputs("\n", stdlog);
               LOG_END
            }
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Registration request for ");
            poolHandlePrint(&message->Handle, stdlog);
            fputs(" of pool element ", stdlog);
            ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
            fputs(" was not possible, since no usable ASAP transport addresses are available\n", stdlog);
            fputs("Addresses from association: ", stdlog);
            transportAddressBlockPrint(remoteAddressBlock, stdlog);
            fputs("\n", stdlog);
            LOG_END
            message->Error = RSPERR_NO_USABLE_ASAP_ADDRESSES;
         }
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "Registration request to pool ");
         poolHandlePrint(&message->Handle, stdlog);
         fputs(" of pool element ", stdlog);
         ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
         fputs(" was not possible, since no usable user transport addresses are available\n", stdlog);
         fputs("Addresses from message: ", stdlog);
         transportAddressBlockPrint(message->PoolElementPtr->UserTransport, stdlog);
         fputs("\n", stdlog);
         fputs("Addresses from association: ", stdlog);
         transportAddressBlockPrint(remoteAddressBlock, stdlog);
         fputs("\n", stdlog);
         LOG_END
         message->Error = RSPERR_NO_USABLE_USER_ADDRESSES;
      }

      if(message->Error) {
         message->Flags |= AHF_REGISTRATION_REJECT;
      }
      if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, 0, message) == false) {
         LOG_WARNING
         logerror("Sending RegistrationResponse failed");
         LOG_END
      }
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "Unable to obtain peer addresses of FD %d, assoc %u\n",
              fd, (unsigned int)assocID);
      LOG_END
   }
}


/* ###### Handle ASAP Deregistration ##################################### */
void registrarHandleASAPDeregistration(struct Registrar*       registrar,
                                       int                     fd,
                                       sctp_assoc_t            assocID,
                                       struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)  delPoolElementNode;
   struct ST_CLASS(PoolNode)         delPoolNode;

   LOG_ACTION
   fputs("Deregistration request for ", stdlog);
   poolHandlePrint(&message->Handle, stdlog);
   fprintf(stdlog, "/$%08x\n", message->Identifier);
   LOG_END

   message->Type  = AHT_DEREGISTRATION_RESPONSE;
   message->Flags = 0x00;

   poolElementNode = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                        &registrar->Handlespace,
                        &message->Handle,
                        message->Identifier);
   if(poolElementNode != NULL) {
      /*
         The ASAP draft says that HandleUpdate has to include a full
         Pool Element Parameter, even if this is unnecessary for
         a removal (ID would be sufficient). Since we cannot guarantee
         that deregistration in the handlespace is successful, we have
         to copy all data before!
         Obviously, this is a waste of CPU cycles, memory and bandwidth...
      */
      memset(&delPoolNode, 0, sizeof(delPoolNode));
      memset(&delPoolElementNode, 0, sizeof(delPoolElementNode));
      delPoolNode                      = *(poolElementNode->OwnerPoolNode);
      delPoolElementNode               = *poolElementNode;
      delPoolElementNode.OwnerPoolNode = &delPoolNode;
      delPoolElementNode.RegistratorTransport = transportAddressBlockDuplicate(poolElementNode->RegistratorTransport);
      delPoolElementNode.UserTransport        = transportAddressBlockDuplicate(poolElementNode->UserTransport);
      if((delPoolElementNode.UserTransport != NULL) &&
         (delPoolElementNode.RegistratorTransport != NULL)) {

         message->Error = ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                             &registrar->Handlespace,
                             poolElementNode);
         if(message->Error == RSPERR_OKAY) {
            message->Flags = 0x00;

            delPoolElementNode.HomeRegistrarIdentifier = registrar->ServerID;
            registrarSendENRPHandleUpdate(registrar, &delPoolElementNode, PNUP_DEL_PE);

            LOG_ACTION
            fputs("Deregistration successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Handlespace content:\n", stdlog);
            registrarDumpHandlespace(registrar);
            LOG_END

            registrar->DeregistrationCount++;
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Failed to deregister pool element $%08x of pool ",
                    message->Identifier);
            poolHandlePrint(&message->Handle, stdlog);
            fputs(": ", stdlog);
            rserpoolErrorPrint(message->Error, stdlog);
            fputs("\n", stdlog);
            LOG_END
         }

         transportAddressBlockDelete(delPoolElementNode.UserTransport);
         free(delPoolElementNode.UserTransport);
         delPoolElementNode.UserTransport = NULL;
         transportAddressBlockDelete(delPoolElementNode.RegistratorTransport);
         free(delPoolElementNode.RegistratorTransport);
      }
      else {
         if(delPoolElementNode.UserTransport) {
            transportAddressBlockDelete(delPoolElementNode.UserTransport);
            free(delPoolElementNode.UserTransport);
         }
         if(delPoolElementNode.RegistratorTransport) {
            transportAddressBlockDelete(delPoolElementNode.RegistratorTransport);
            free(delPoolElementNode.RegistratorTransport);
         }
         message->Error = RSPERR_OUT_OF_MEMORY;
      }
   }
   else {
      message->Error = RSPERR_OKAY;
      LOG_WARNING
      fprintf(stdlog, "Deregistration request for non-existing pool element $%08x of pool ",
              message->Identifier);
      poolHandlePrint(&message->Handle, stdlog);
      fputs(". Reporting success, since it seems to be already gone.\n", stdlog);
      LOG_END
   }

   if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, 0, message) == false) {
      LOG_WARNING
      logerror("Sending DeregistrationResponse failed");
      LOG_END
   }
}


/* ###### Handle ASAP Handle Resolution ################################## */
void registrarHandleASAPHandleResolution(struct Registrar*       registrar,
                                         int                     fd,
                                         sctp_assoc_t            assocID,
                                         struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNodeArray[MAX_MAX_HANDLE_RESOLUTION_ITEMS];
   size_t                            poolElementNodes = MAX_MAX_HANDLE_RESOLUTION_ITEMS;
   size_t                            items;
   size_t                            i;

   items = message->Addresses;
   if(items == 0) {
      items = registrar->MaxHandleResolutionItems;
   }
   items = min(items, MAX_MAX_HANDLE_RESOLUTION_ITEMS);

   LOG_ACTION
   fputs("Handle Resolution request for pool ", stdlog);
   poolHandlePrint(&message->Handle, stdlog);
   fprintf(stdlog, " for %u items (%u requested)\n",
           (unsigned int)items, (unsigned int)message->Addresses);
   LOG_END


   registrar->HandleResolutionCount++;

   message->Type  = AHT_HANDLE_RESOLUTION_RESPONSE;
   message->Flags = 0x00;
   message->Error = ST_CLASS(poolHandlespaceManagementHandleResolution)(
                       &registrar->Handlespace,
                       &message->Handle,
                       (struct ST_CLASS(PoolElementNode)**)&poolElementNodeArray,
                       &poolElementNodes,
                       items,
                       registrar->MaxIncrement);
   if(message->Error == RSPERR_OKAY) {
      LOG_VERBOSE1
      fprintf(stdlog, "Selected %u element%s\n", (unsigned int)poolElementNodes,
              (poolElementNodes == 1) ? "" : "s");
      LOG_END
      LOG_VERBOSE2
      fputs("Selected pool elements:\n", stdlog);
      for(i = 0;i < poolElementNodes;i++) {
         fprintf(stdlog, "#%02u: PE ", (unsigned int)i + 1);
         ST_CLASS(poolElementNodePrint)(poolElementNodeArray[i],
                  stdlog,
                  PENPO_USERTRANSPORT|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PENPO_HOME_PR);
         fputs("\n", stdlog);
      }
      LOG_END

      if(poolElementNodes > 0) {
         message->PolicySettings = poolElementNodeArray[0]->PolicySettings;
      }

      message->PoolElementPtrArrayAutoDelete = false;
      message->PoolElementPtrArraySize       = poolElementNodes;
      for(i = 0;i < poolElementNodes;i++) {
         message->PoolElementPtrArray[i] = poolElementNodeArray[i];
      }
   }
   else {
      LOG_WARNING
      fprintf(stdlog, "Handle Resolution request for pool ");
      poolHandlePrint(&message->Handle, stdlog);
      fputs(" failed: ", stdlog);
      rserpoolErrorPrint(message->Error, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }

   if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, 0, message) == false) {
      LOG_WARNING
      logerror("Sending handle resolution response failed");
      LOG_END
   }
}


/* ###### Handle ASAP Endpoint Keep-Alive Ack ############################ */
void registrarHandleASAPEndpointKeepAliveAck(struct Registrar*       registrar,
                                             int                     fd,
                                             sctp_assoc_t            assocID,
                                             struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   LOG_VERBOSE2
   fprintf(stdlog, "Got EndpointKeepAliveAck for pool element $%08x of pool ",
           message->Identifier);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   poolElementNode = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                        &registrar->Handlespace,
                        &message->Handle,
                        message->Identifier);
   if(poolElementNode != NULL) {
      LOG_VERBOSE2
      fputs("EndpointKeepAlive successful, resetting timer\n", stdlog);
      LOG_END

      ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
         &registrar->Handlespace.Handlespace,
         poolElementNode);
      ST_CLASS(poolHandlespaceNodeActivateTimer)(
         &registrar->Handlespace.Handlespace,
         poolElementNode,
         PENT_KEEPALIVE_TRANSMISSION,
         getMicroTime() + registrar->EndpointKeepAliveTransmissionInterval);
      timerRestart(&registrar->HandlespaceActionTimer,
                   ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                      &registrar->Handlespace));
   }
   else {
      LOG_WARNING
      fprintf(stdlog,
              "EndpointKeepAliveAck for not-existing pool element $%08x of pool ",
              message->Identifier);
      poolHandlePrint(&message->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }
}


/* ###### Send ASAP Endpoint Keep-Alive ################################## */
void registrarSendASAPEndpointKeepAlive(struct Registrar*                 registrar,
                                        struct ST_CLASS(PoolElementNode)* poolElementNode,
                                        const bool                        newHomeRegistrar)
{
   struct RSerPoolMessage* message;
   bool                    result;

   message = rserpoolMessageNew(NULL, 1024);
   if(message != NULL) {
      LOG_VERBOSE2
      fprintf(stdlog, "Sending EndpointKeepAlive for pool element $%08x in pool ",
              poolElementNode->Identifier);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END

      message->Handle              = poolElementNode->OwnerPoolNode->Handle;
      message->RegistrarIdentifier = registrar->ServerID;
      message->Identifier          = poolElementNode->Identifier;
      message->Type                = AHT_ENDPOINT_KEEP_ALIVE;
      message->Flags               = newHomeRegistrar ? AHF_ENDPOINT_KEEP_ALIVE_HOME : 0x00;
      if(poolElementNode->ConnectionSocketDescriptor >= 0) {
         LOG_VERBOSE3
         fprintf(stdlog, "Sending via association %u...\n",
                 (unsigned int)poolElementNode->ConnectionAssocID);
         LOG_END
         result = rserpoolMessageSend(IPPROTO_SCTP,
                                      poolElementNode->ConnectionSocketDescriptor,
                                      poolElementNode->ConnectionAssocID,
                                      0, 0, 0,
                                      message);
      }
      else {
         message->AddressArray = poolElementNode->RegistratorTransport->AddressArray;
         message->Addresses    = poolElementNode->RegistratorTransport->Addresses;
         LOG_VERBOSE3
         fputs("Sending to ", stdlog);
         transportAddressBlockPrint(poolElementNode->RegistratorTransport, stdlog);
         fputs("...\n", stdlog);
         LOG_END
         result = rserpoolMessageSend(IPPROTO_SCTP,
                                      registrar->ASAPSocket, 0,
                                      0, 0, 0,
                                      message);
      }
      if(result == false) {
         LOG_WARNING
         fprintf(stdlog, "Sending EndpointKeepAlive for pool element $%08x in pool ",
                 poolElementNode->Identifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
         fputs(" failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Handle ASAP Endpoint Unreachable ############################### */
void registrarHandleASAPEndpointUnreachable(struct Registrar*       registrar,
                                            int                     fd,
                                            sctp_assoc_t            assocID,
                                            struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   unsigned int                      result;

   LOG_ACTION
   fprintf(stdlog, "Got EndpointUnreachable for pool element $%08x of pool ",
           message->Identifier);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   registrar->FailureReportCount++;

   poolElementNode = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                        &registrar->Handlespace,
                        &message->Handle,
                        message->Identifier);
   if(poolElementNode != NULL) {
      poolElementNode->UnreachabilityReports++;
      if(poolElementNode->UnreachabilityReports >= registrar->MaxBadPEReports) {
         LOG_WARNING
         fprintf(stdlog, "%u unreachability reports for pool element ",
                 poolElementNode->UnreachabilityReports);
         ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
         fputs(" of pool ", stdlog);
         poolHandlePrint(&message->Handle, stdlog);
         fputs(" -> limit reached, removing it\n", stdlog);
         LOG_END

         if(poolElementNode->HomeRegistrarIdentifier == registrar->ServerID) {
            /* We own this PE -> send HandleUpdate for its removal. */
            registrarSendENRPHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
         }

         result = ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                     &registrar->Handlespace,
                     poolElementNode);
         if(message->Error == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Successfully deregistered ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fprintf(stdlog, "/$%08x\n", message->Identifier);
            LOG_END
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Failed to deregister pool element $%08x from pool ",
                  message->Identifier);
            poolHandlePrint(&message->Handle, stdlog);
            fputs(": ", stdlog);
            rserpoolErrorPrint(result, stdlog);
            fputs("\n", stdlog);
            LOG_END
         }
      }
   }
   else {
      LOG_WARNING
      fprintf(stdlog,
              "EndpointUnreachable for non-existing pool element $%08x of pool ",
              message->Identifier);
      poolHandlePrint(&message->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }
}
