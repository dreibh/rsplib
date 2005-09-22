/*
 *  $Id$
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: ASAP Instance
 *
 */

#include "tdtypes.h"
#include "loglevel.h"
#include "asapinstance.h"
#include "rserpoolmessage.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "rsplib-tags.h"

#include <ext_socket.h>


#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif


static void handleRegistrarConnectionEvent(struct Dispatcher* dispatcher,
                                           int                fd,
                                           unsigned int       eventMask,
                                           void*              userData);
static void asapInstanceHandleEndpointKeepAlive(
               struct ASAPInstance*    asapInstance,
               struct RSerPoolMessage* message,
               int                     fd);
static void asapInstanceDisconnectFromRegistrar(
               struct ASAPInstance* asapInstance,
               bool                 sendAbort);


/* ###### Get configuration from file #################################### */
static void asapInstanceConfigure(struct ASAPInstance* asapInstance, struct TagItem* tags)
{
   /* ====== ASAP Instance settings ======================================= */
   asapInstance->RegistrarRequestMaxTrials = tagListGetData(tags, TAG_RspLib_RegistrarRequestMaxTrials,
                                                            ASAP_DEFAULT_REGISTRAR_REQUEST_MAXTRIALS);
   asapInstance->RegistrarRequestTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_RegistrarRequestTimeout,
                                                                              ASAP_DEFAULT_REGISTRAR_REQUEST_TIMEOUT);
   asapInstance->RegistrarResponseTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_RegistrarResponseTimeout,
                                                                               ASAP_DEFAULT_REGISTRAR_RESPONSE_TIMEOUT);
   asapInstance->CacheElementTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_CacheElementTimeout,
                                                                          ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT);


   /* ====== Show results =================================================== */
   LOG_VERBOSE3
   fputs("New ASAP instance's configuration:\n", stdlog);
   fprintf(stdlog, "registrar.request.maxtrials   = %u\n",     (unsigned int)asapInstance->RegistrarRequestMaxTrials);
   fprintf(stdlog, "registrar.request.timeout     = %lluus\n", asapInstance->RegistrarRequestTimeout);
   fprintf(stdlog, "registrar.response.timeout    = %lluus\n", asapInstance->RegistrarResponseTimeout);
   fprintf(stdlog, "cache.elementtimeout          = %lluus\n", asapInstance->CacheElementTimeout);
   LOG_END
}


/* ###### Constructor #################################################### */
struct ASAPInstance* asapInstanceNew(struct Dispatcher* dispatcher,
                                     struct TagItem*    tags)
{
   struct ASAPInstance*        asapInstance = NULL;
   struct sctp_event_subscribe sctpEvents;
   int                         autoCloseTimeout;

   if(dispatcher != NULL) {
      asapInstance = (struct ASAPInstance*)malloc(sizeof(struct ASAPInstance));
      if(asapInstance != NULL) {
         asapInstance->StateMachine = dispatcher;

         asapInstanceConfigure(asapInstance, tags);

         asapInstance->RegistrarConnectionTimeStamp = 0;
         asapInstance->RegistrarHuntSocket          = -1;
         asapInstance->RegistrarSocket              = -1;
         asapInstance->RegistrarIdentifier          = 0;
         ST_CLASS(poolHandlespaceManagementNew)(&asapInstance->Cache,
                                                0x00000000,
                                                NULL, NULL, NULL);
         ST_CLASS(poolHandlespaceManagementNew)(&asapInstance->OwnPoolElements,
                                                0x00000000,
                                                NULL, NULL, NULL);
         asapInstance->RegistrarSet = registrarTableNew(asapInstance->StateMachine, tags);
         if(asapInstance->RegistrarSet == NULL) {
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         /* ====== Registrar socket ========================================== */
         asapInstance->RegistrarHuntSocket = ext_socket(checkIPv6() ? AF_INET6 : AF_INET,
                                                        SOCK_SEQPACKET,
                                                        IPPROTO_SCTP);
         if(asapInstance->RegistrarHuntSocket < 0) {
            LOG_ERROR
            logerror("Creating registrar hunt socket failed");
            LOG_END
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         fdCallbackNew(&asapInstance->RegistrarHuntFDCallback,
                       asapInstance->StateMachine,
                       asapInstance->RegistrarHuntSocket,
                       FDCE_Read|FDCE_Exception,
                       handleRegistrarConnectionEvent,
                       (void*)asapInstance);

         if(bindplus(asapInstance->RegistrarHuntSocket, NULL, 0) == false) {
            LOG_ERROR
            logerror("Binding registrar hunt socket failed");
            LOG_END
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         setNonBlocking(asapInstance->RegistrarHuntSocket);

         if(ext_listen(asapInstance->RegistrarHuntSocket, 10) < 0) {
            LOG_ERROR
            logerror("Unable to turn registrar hunt socket into listening mode");
            LOG_END
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         memset(&sctpEvents, 0, sizeof(sctpEvents));
         sctpEvents.sctp_data_io_event          = 1;
         sctpEvents.sctp_association_event      = 1;
         sctpEvents.sctp_address_event          = 1;
         sctpEvents.sctp_send_failure_event     = 1;
         sctpEvents.sctp_peer_error_event       = 1;
         sctpEvents.sctp_shutdown_event         = 1;
         sctpEvents.sctp_partial_delivery_event = 1;
         sctpEvents.sctp_adaption_layer_event   = 1;

         if(ext_setsockopt(asapInstance->RegistrarHuntSocket, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
            logerror("setsockopt() for SCTP_EVENTS on registrar hunt socket failed");
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         autoCloseTimeout = 30;
         if(ext_setsockopt(asapInstance->RegistrarHuntSocket, IPPROTO_SCTP, SCTP_AUTOCLOSE, &autoCloseTimeout, sizeof(autoCloseTimeout)) < 0) {
            logerror("setsockopt() for SCTP_AUTOCLOSE on registrar hunt socket failed");
            exit(1);
         }
      }
   }
   return(asapInstance);
}


/* ###### Destructor ##################################################### */
void asapInstanceDelete(struct ASAPInstance* asapInstance)
{
   if(asapInstance) {
      asapInstanceDisconnectFromRegistrar(asapInstance, false);
      if(asapInstance->RegistrarHuntSocket >= 0) {
         fdCallbackDelete(&asapInstance->RegistrarHuntFDCallback);
         ext_close(asapInstance->RegistrarHuntSocket);
      }
      ST_CLASS(poolHandlespaceManagementClear)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolHandlespaceManagementDelete)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolHandlespaceManagementClear)(&asapInstance->Cache);
      ST_CLASS(poolHandlespaceManagementDelete)(&asapInstance->Cache);
      if(asapInstance->RegistrarSet) {
         registrarTableDelete(asapInstance->RegistrarSet);
         asapInstance->RegistrarSet = NULL;
      }
      free(asapInstance);
   }
}


/* ###### Connect to registrar ############################################ */
static bool asapInstanceConnectToRegistrar(struct ASAPInstance* asapInstance,
                                           int                  sd)
{
   if(asapInstance->RegistrarSocket < 0) {
      if(sd < 0) {
         LOG_ACTION
         fputs("Starting registrar hunt...\n", stdlog);
         LOG_END
         if(sd < 0) {
            sd = registrarTableGetRegistrar(asapInstance->RegistrarSet,
                                            asapInstance->RegistrarHuntSocket,
                                            &asapInstance->RegistrarIdentifier);
         }
         if(sd < 0) {
            LOG_ACTION
            fputs("Unable to connect to a registrar\n", stdlog);
            LOG_END
            return(false);
         }
      }

      asapInstance->RegistrarSocket = sd;
      fdCallbackNew(&asapInstance->RegistrarFDCallback,
                    asapInstance->StateMachine,
                    asapInstance->RegistrarSocket,
                    FDCE_Read|FDCE_Exception,
                    handleRegistrarConnectionEvent,
                    (void*)asapInstance);

      LOG_NOTE
      fprintf(stdlog, "Connected to registrar $%08x\n", asapInstance->RegistrarIdentifier);
      LOG_END
   }
   return(true);
}


/* ###### Disconnect from registrar #################################### */
static void asapInstanceDisconnectFromRegistrar(struct ASAPInstance* asapInstance,
                                                bool                 sendAbort)
{
   if(asapInstance->RegistrarSocket >= 0) {
      fdCallbackDelete(&asapInstance->RegistrarFDCallback);
      if(sendAbort) {
         /* Abort association to current registrar */
         sendtoplus(asapInstance->RegistrarSocket, NULL, 0, SCTP_ABORT,
                    NULL, 0,
                    0, 0, 0, 0xffffffff, 0);
      }
      ext_close(asapInstance->RegistrarSocket);
      asapInstance->RegistrarSocket              = -1;
      asapInstance->RegistrarConnectionTimeStamp = 0;
      asapInstance->RegistrarIdentifier          = UNDEFINED_REGISTRAR_IDENTIFIER;

      LOG_ACTION
      fputs("Disconnected from registrar\n", stdlog);
      LOG_END
   }
}


/* ###### Handle SCTP notification on registrar socket ################### */
static void handleNotificationOnRegistrarSocket(struct ASAPInstance*           asapInstance,
                                                const union sctp_notification* notification)
{
   if( (notification->sn_header.sn_type == SCTP_ASSOC_CHANGE) &&
       (notification->sn_assoc_change.sac_state == SCTP_COMM_LOST) ) {
      LOG_WARNING
      fputs("Registrar connection lost\n", stdlog);
      LOG_END
      asapInstanceDisconnectFromRegistrar(asapInstance, true);
   }
   else if(notification->sn_header.sn_type == SCTP_SHUTDOWN_EVENT) {
      LOG_WARNING
      fputs("Registrar connection is shutting down\n", stdlog);
      LOG_END
      asapInstanceDisconnectFromRegistrar(asapInstance, true);
   }
}


/* ###### Send request to registrar #################################### */
static unsigned int asapInstanceSendRequest(struct ASAPInstance*     asapInstance,
                                            struct RSerPoolMessage*  request)
{
   bool result;

   if(asapInstanceConnectToRegistrar(asapInstance, -1) == false) {
      return(RSPERR_NO_REGISTRAR);
   }
   result = rserpoolMessageSend(IPPROTO_SCTP,
                                asapInstance->RegistrarSocket,
                                0,
                                MSG_NOSIGNAL,
                                asapInstance->RegistrarRequestTimeout,
                                request);
   if(result) {
      return(RSPERR_OKAY);
   }

   return(RSPERR_WRITE_ERROR);
}


/* ###### Send request to registrar and wait for response ############## */
static unsigned int asapInstanceDoIO(struct ASAPInstance*     asapInstance,
                                     struct RSerPoolMessage*  message,
                                     struct RSerPoolMessage** responsePtr,
                                     uint16_t*                error)
{
   struct RSerPoolMessage* response;
   ssize_t                 received;
   union sockaddr_union    sourceAddress;
   socklen_t               sourceAddressLength;
   uint32_t                ppid;
   int                     flags;
   char                    buffer[65536];
   unsigned int            result = RSPERR_OKAY;
   size_t                  i;

   *responsePtr = NULL;
   *error = RSPERR_OKAY;
   for(i = 0;i < asapInstance->RegistrarRequestMaxTrials;i++) {
      LOG_VERBOSE2
      fprintf(stdlog, "Request trial #%u - sending request...\n", (unsigned int)i + 1);
      LOG_END

      /* ====== Send request ============================================= */
      result = asapInstanceSendRequest(asapInstance, message);
      if(result == RSPERR_OKAY) {
         LOG_VERBOSE2
         fprintf(stdlog, "Request trial #%u - waiting for response...\n", (unsigned int)i + 1);
         LOG_END

         /* ====== Receive response ====================================== */
         flags               = MSG_NOSIGNAL;
         sourceAddressLength = sizeof(sourceAddress);
         received = recvfromplus(asapInstance->RegistrarSocket,
                                 (char*)&buffer, sizeof(buffer),
                                 &flags,
                                 (struct sockaddr*)&sourceAddress, &sourceAddressLength,
                                 &ppid, 0, NULL,
                                 asapInstance->RegistrarResponseTimeout);
         while(received > 0) {
            /* ====== Handle notification ================================ */
            if(flags & MSG_NOTIFICATION) {
               handleNotificationOnRegistrarSocket(asapInstance, (union sctp_notification*)&buffer);
            }

            /* ====== Handle response ==================================== */
            else {
               if(rserpoolPacket2Message((char*)&buffer,
                                         &sourceAddress,
                                         0,
                                         ppid,
                                         received, sizeof(buffer),
                                         &response) == RSPERR_OKAY) {
                  *error = response->Error;

                  if(response->Type == AHT_ENDPOINT_KEEP_ALIVE) {
                     asapInstanceHandleEndpointKeepAlive(asapInstance, response, asapInstance->RegistrarSocket);
                  }
                  else if(response->Type == AHT_ENDPOINT_KEEP_ALIVE_ACK) {
                  }
                  else if( ((response->Type == AHT_REGISTRATION_RESPONSE)    && (message->Type == AHT_REGISTRATION))   ||
                           ((response->Type == AHT_DEREGISTRATION_RESPONSE)  && (message->Type == AHT_DEREGISTRATION)) ||
                           ((response->Type == AHT_HANDLE_RESOLUTION_RESPONSE) && (message->Type == AHT_HANDLE_RESOLUTION)) ) {
                     LOG_VERBOSE2
                     fprintf(stdlog, "Request trial #%u - Success\n", (unsigned int)i + 1);
                     LOG_END
                     *responsePtr = response;
                     return(RSPERR_OKAY);
                  }
                  else {
                     LOG_WARNING
                     fprintf(stdlog, "Bad request/response type pair: %02x/%02x\n",
                             message->Type, response->Type);
                     LOG_END
                     rserpoolMessageDelete(response);
                     return(RSPERR_INVALID_VALUES);
                  }

                  rserpoolMessageDelete(response);
               }
            }

            /* ====== Continue receiving response ======================== */
            flags               = MSG_NOSIGNAL;
            sourceAddressLength = sizeof(sourceAddress);
            received = recvfromplus(asapInstance->RegistrarSocket,
                                    (char*)&buffer, sizeof(buffer),
                                    &flags,
                                    (struct sockaddr*)&sourceAddress, &sourceAddressLength,
                                    &ppid, 0, NULL,
                                    asapInstance->RegistrarResponseTimeout);
         }

         LOG_VERBOSE2
         fprintf(stdlog, "Request trial #%u failed - no valid response from registrar\n", (unsigned int)i + 1);
         LOG_END
         asapInstanceDisconnectFromRegistrar(asapInstance, true);
         result = RSPERR_READ_ERROR;
      }
      else {
         LOG_VERBOSE2
         fprintf(stdlog, "Request trial #%u failed - unable to send request\n", (unsigned int)i + 1);
         LOG_END
         asapInstanceDisconnectFromRegistrar(asapInstance, true);
      }
   }

   return(result);
}


/* ###### Register pool element ########################################## */
unsigned int asapInstanceRegister(struct ASAPInstance*              asapInstance,
                                  struct PoolHandle*                poolHandle,
                                  struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct RSerPoolMessage*           message;
   struct RSerPoolMessage*           response;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   unsigned int                      result;
   uint16_t                          registrarResult;
   unsigned int                      handlespaceMgtResult;

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE
   fputs("Trying to register to pool ", stdlog);
   poolHandlePrint(poolHandle, stdlog);
   fputs(" pool element ", stdlog);
   ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
   fputs("\n", stdlog);
   LOG_END

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type           = AHT_REGISTRATION;
      message->Flags          = 0x00;
      message->Handle         = *poolHandle;
      message->PoolElementPtr = poolElementNode;

      result = asapInstanceDoIO(asapInstance, message, &response, &registrarResult);
      if(result == RSPERR_OKAY) {
         if( (registrarResult == RSPERR_OKAY) &&
             (!(response->Flags & AHF_REGISTRATION_REJECT)) ) {
            handlespaceMgtResult = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                                      &asapInstance->OwnPoolElements,
                                      poolHandle,
                                      message->PoolElementPtr->HomeRegistrarIdentifier,
                                      message->PoolElementPtr->Identifier,
                                      message->PoolElementPtr->RegistrationLife,
                                      &message->PoolElementPtr->PolicySettings,
                                      message->PoolElementPtr->UserTransport,
                                      NULL,
                                      -1, 0,
                                      getMicroTime(),
                                      &newPoolElementNode);
            if(handlespaceMgtResult == RSPERR_OKAY) {
               newPoolElementNode->UserData = (void*)asapInstance;
               if(response->Identifier != poolElementNode->Identifier) {
                  LOG_ERROR
                  fprintf(stdlog, "Tried to register PE $%08x, got response for PE $%08x\n",
                          poolElementNode->Identifier,
                          response->Identifier);
                  LOG_END_FATAL
               }
            }
            else {
               LOG_ERROR
               fprintf(stdlog, "Unable to register pool element $%08x of pool ",
                       poolElementNode->Identifier);
               poolHandlePrint(poolHandle, stdlog);
               fputs(" to OwnPoolElements\n", stdlog);
               LOG_END_FATAL
            }
         }
         else {
            result = (unsigned int)registrarResult;
         }
         if(response) {
            rserpoolMessageDelete(response);
         }
      }

      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_NO_RESOURCES;
   }

   LOG_VERBOSE
   fputs("Registration result is: ", stdlog);
   rserpoolErrorPrint(result, stdlog);
   fputs("\n", stdlog);
   LOG_END

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}


/* ###### Deregister pool element ######################################## */
unsigned int asapInstanceDeregister(struct ASAPInstance*            asapInstance,
                                    struct PoolHandle*              poolHandle,
                                    const PoolElementIdentifierType identifier)
{
   struct RSerPoolMessage* message;
   struct RSerPoolMessage* response;
   unsigned int            result;
   uint16_t                registrarResult;
   unsigned int            handlespaceMgtResult;

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE
   fprintf(stdlog, "Trying to deregister $%08x from pool ",identifier);
   poolHandlePrint(poolHandle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type       = AHT_DEREGISTRATION;
      message->Flags      = 0x00;
      message->Handle     = *poolHandle;
      message->Identifier = identifier;

      result = asapInstanceDoIO(asapInstance, message, &response, &registrarResult);
      if((result == RSPERR_OKAY) && (registrarResult == RSPERR_OKAY)) {
         if(identifier != response->Identifier) {
            LOG_ERROR
            fprintf(stdlog, "Tried to deregister PE $%08x, got response for PE $%08x\n",
                    identifier,
                    message->Identifier);
            LOG_END_FATAL
         }

         handlespaceMgtResult = ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
                                   &asapInstance->OwnPoolElements,
                                   poolHandle,
                                   identifier);
         if(handlespaceMgtResult != RSPERR_OKAY) {
            LOG_ERROR
            fprintf(stdlog, "Unable to deregister pool element $%08x of pool ",
                    identifier);
            poolHandlePrint(poolHandle, stdlog);
            fputs(" from OwnPoolElements\n", stdlog);
            LOG_END_FATAL
         }
         if(response) {
            rserpoolMessageDelete(response);
         }
      }

      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_NO_RESOURCES;
   }

   LOG_VERBOSE
   fputs("Deregistration result is: ", stdlog);
   rserpoolErrorPrint(result, stdlog);
   fputs("\n", stdlog);
   LOG_END

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}



/* ###### Report pool element failure ####################################### */
unsigned int asapInstanceReportFailure(struct ASAPInstance*            asapInstance,
                                       struct PoolHandle*              poolHandle,
                                       const PoolElementIdentifierType identifier)
{
   struct RSerPoolMessage*           message;
   struct ST_CLASS(PoolElementNode)* found;
   unsigned int                      result;

   LOG_VERBOSE
   fprintf(stdlog, "Failure reported for pool element $%08x of pool ",
           (unsigned int)identifier);
   poolHandlePrint(poolHandle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   /* ====== Remove pool element from cache ================================= */
   dispatcherLock(asapInstance->StateMachine);

   found = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
              &asapInstance->Cache,
              poolHandle,
              identifier);
   if(found != NULL) {
      result = ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
                          &asapInstance->Cache,
                          poolHandle,
                          identifier);
      CHECK(result == RSPERR_OKAY);
   }
   else {
      LOG_VERBOSE
      fputs("Pool element does not exist in cache\n", stdlog);
      LOG_END
   }

   /* ====== Report unreachability ========================================== */
   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type       = AHT_ENDPOINT_UNREACHABLE;
      message->Flags      = 0x00;
      message->Handle     = *poolHandle;
      message->Identifier = identifier;
      result = asapInstanceSendRequest(asapInstance, message);
      if(result != RSPERR_OKAY) {
         LOG_VERBOSE2
         fprintf(stdlog, "Failed to send failure report for pool element $%08x of pool ",
               (unsigned int)identifier);
         poolHandlePrint(poolHandle, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_OUT_OF_MEMORY;
   }

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}


/* ###### Do name lookup ################################################# */
static unsigned int asapInstanceDoHandleResolution(struct ASAPInstance* asapInstance,
                                                   struct PoolHandle*   poolHandle)
{
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct RSerPoolMessage*           message;
   struct RSerPoolMessage*           response;
   unsigned int                      result;
   uint16_t                          registrarResult;
   size_t                            i;

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type   = AHT_HANDLE_RESOLUTION;
      message->Flags  = 0x00;
      message->Handle = *poolHandle;

      result = asapInstanceDoIO(asapInstance, message, &response, &registrarResult);
      if(result == RSPERR_OKAY) {
         if(registrarResult == RSPERR_OKAY) {
            LOG_VERBOSE
            fprintf(stdlog, "Got %u elements in handle resolution response\n",
                    (unsigned int)response->PoolElementPtrArraySize);
            LOG_END
            for(i = 0;i < response->PoolElementPtrArraySize;i++) {
               LOG_VERBOSE2
               fputs("Adding pool element to cache: ", stdlog);
               ST_CLASS(poolElementNodePrint)(response->PoolElementPtrArray[i], stdlog, PENPO_FULL);
               fputs("\n", stdlog);
               LOG_END
               result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                           &asapInstance->Cache,
                           poolHandle,
                           response->PoolElementPtrArray[i]->HomeRegistrarIdentifier,
                           response->PoolElementPtrArray[i]->Identifier,
                           response->PoolElementPtrArray[i]->RegistrationLife,
                           &response->PoolElementPtrArray[i]->PolicySettings,
                           response->PoolElementPtrArray[i]->UserTransport,
                           NULL,
                           -1, 0,
                           getMicroTime(),
                           &newPoolElementNode);
               if(result != RSPERR_OKAY) {
                  LOG_WARNING
                  fputs("Failed to add pool element to cache: ", stdlog);
                  ST_CLASS(poolElementNodePrint)(response->PoolElementPtrArray[i], stdlog, PENPO_FULL);
                  fputs(": ", stdlog);
                  rserpoolErrorPrint(result, stdlog);
                  fputs("\n", stdlog);
                  LOG_END
               }
               ST_CLASS(poolHandlespaceManagementRestartPoolElementExpiryTimer)(
                  &asapInstance->Cache,
                  newPoolElementNode,
                  asapInstance->CacheElementTimeout);
            }
         }
         else {
            LOG_VERBOSE2
            fprintf(stdlog, "Handle Resolution at registrar for pool ");
            poolHandlePrint(poolHandle, stdlog);
            fputs(" failed: ", stdlog);
            rserpoolErrorPrint(registrarResult, stdlog);
            fputs("\n", stdlog);
            LOG_END
            result = registrarResult;
         }
         rserpoolMessageDelete(response);
      }
      else {
         LOG_VERBOSE2
         fprintf(stdlog, "Handle Resolution at registrar for pool ");
         poolHandlePrint(poolHandle, stdlog);
         fputs(" failed: ", stdlog);
         rserpoolErrorPrint(registrarResult, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_NO_RESOURCES;
   }

   return(result);
}


/* ###### Do name lookup from cache ###################################### */
static unsigned int asapInstanceHandleResolutionFromCache(
                       struct ASAPInstance*               asapInstance,
                       struct PoolHandle*                 poolHandle,
                       struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                       size_t*                            poolElementNodes)
{
   unsigned int result;
   size_t       i;

   LOG_VERBOSE
   fprintf(stdlog, "Handle Resolution for pool ");
   poolHandlePrint(poolHandle, stdlog);
   fputs(":\n", stdlog);
   ST_CLASS(poolHandlespaceManagementPrint)(&asapInstance->Cache,
                                            stdlog, PENPO_ONLY_POLICY);
   LOG_END

   i = ST_CLASS(poolHandlespaceManagementPurgeExpiredPoolElements)(
          &asapInstance->Cache,
          getMicroTime());
   LOG_VERBOSE
   fprintf(stdlog, "Purged %u out-of-date elements\n", (unsigned int)i);
   LOG_END

   if(ST_CLASS(poolHandlespaceManagementHandleResolution)(
         &asapInstance->Cache,
         poolHandle,
         poolElementNodeArray,
         poolElementNodes,
         *poolElementNodes,
         1000000000) == RSPERR_OKAY) {
      LOG_VERBOSE
      fprintf(stdlog, "Got %u items:\n", (unsigned int)*poolElementNodes);
      for(i = 0;i < *poolElementNodes;i++) {
         fprintf(stdlog, "#%u: ", (unsigned int)i + 1);
         ST_CLASS(poolElementNodePrint)(poolElementNodeArray[i],
                                        stdlog, PENPO_FULL);
      }
      fputs("\n", stdlog);
      LOG_END
      result = RSPERR_OKAY;
   }
   else {
      result = RSPERR_NOT_FOUND;
   }

   return(result);
}


/* ###### Do handle resolution for given pool handle ##################### */
unsigned int asapInstanceHandleResolution(
                struct ASAPInstance*               asapInstance,
                struct PoolHandle*                 poolHandle,
                struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                size_t*                            poolElementNodes)
{
   unsigned int result;
   const size_t originalPoolElementNodes = *poolElementNodes;

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE
   fputs("Trying handle resolution from cache...\n", stdlog);
   LOG_END

   result = asapInstanceHandleResolutionFromCache(
               asapInstance, poolHandle,
               poolElementNodeArray,
               poolElementNodes);
   if(result != RSPERR_OKAY) {
      LOG_VERBOSE
      fputs("No results in cache. Trying handle resolution at registrar...\n", stdlog);
      LOG_END

      /* The amount is now 0 (since handle resolution was not successful).
         Set it to its original value. */
      *poolElementNodes = originalPoolElementNodes;

      result = asapInstanceDoHandleResolution(asapInstance, poolHandle);
      if(result == RSPERR_OKAY) {
         result = asapInstanceHandleResolutionFromCache(
                     asapInstance, poolHandle,
                     poolElementNodeArray,
                     poolElementNodes);
      }
      else {
         LOG_VERBOSE
         fputs("Handle resolution not succesful\n", stdlog);
         LOG_END
      }
   }

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}


/* ###### Handle endpoint keepalive ###################################### */
static void asapInstanceHandleEndpointKeepAlive(
               struct ASAPInstance*    asapInstance,
               struct RSerPoolMessage* message,
               int                     fd)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolNode)*        poolNode;
   unsigned int                      result;
   int                               sd;

   LOG_VERBOSE2
   fprintf(stdlog, "EndpointKeepAlive from registrar $%08x via assoc %u for pool ",
           message->RegistrarIdentifier, (unsigned int)message->AssocID);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   /* Does message come via association on registrar *hunt* socket
      instead of peeled-off registrar socket? */
   if(fd == asapInstance->RegistrarHuntSocket) {
      if(message->Flags & AHF_ENDPOINT_KEEP_ALIVE_HOME) {
         LOG_NOTE
         fprintf(stdlog, "EndpointKeepAlive from $%08x (assoc %u) instead of home-registrar assoc $%08x -> replacing home registrar\n",
               message->RegistrarIdentifier,
               (unsigned int)message->AssocID,
               asapInstance->RegistrarIdentifier);
         LOG_END

         sd = registrarTablePeelOffRegistrarAssocID(asapInstance->RegistrarSet,
                                                    asapInstance->RegistrarHuntSocket,
                                                    message->AssocID);
         if(sd >= 0) {
            asapInstanceDisconnectFromRegistrar(asapInstance, true);
            if(asapInstanceConnectToRegistrar(asapInstance, sd) == true) {
               /* The new registrar ID is already known */
               asapInstance->RegistrarIdentifier          = message->RegistrarIdentifier;
               asapInstance->RegistrarConnectionTimeStamp = getMicroTime();
            }
         }
         else {
            LOG_ERROR
            logerror("sctp_peeloff() for incoming registrar association from new registrar failed");
            LOG_END
         }
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "EndpointKeepAlive from $%08x (assoc %u) instead of home-registrar assoc $%08x. The H bit is not set, therefore ignoring it.",
               message->RegistrarIdentifier,
               (unsigned int)message->AssocID,
               asapInstance->RegistrarIdentifier);
         LOG_END
      }
   }
   else {
      if(asapInstance->RegistrarIdentifier == UNDEFINED_REGISTRAR_IDENTIFIER) {
         asapInstance->RegistrarIdentifier = message->RegistrarIdentifier;
      }
   }


   /* ====== Send ENDPOINT_KEEP_ALIVE_ACK for each PE ==================== */
   poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(&asapInstance->OwnPoolElements.Handlespace);
   while(poolNode != NULL) {
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
      while(poolElementNode != NULL) {

         message->Type       = AHT_ENDPOINT_KEEP_ALIVE_ACK;
         message->Flags      = 0x00;
         message->Identifier = poolElementNode->Identifier;

         LOG_VERBOSE2
         fprintf(stdlog, "Sending KeepAliveAck for pool element $%08x\n",message->Identifier);
         LOG_END
         result = asapInstanceSendRequest(asapInstance, message);
         if(result != RSPERR_OKAY) {
            LOG_WARNING
            fprintf(stdlog, "Failed to send KeepAliveAck for pool element $%08x\n", message->Identifier);
            LOG_END
         }

         poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)(poolNode, poolElementNode);
      }

      poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(&asapInstance->OwnPoolElements.Handlespace, poolNode);
   }
}


/* ###### Handle incoming requests from registrar (keep-alives) ########## */
static void handleRegistrarConnectionEvent(
               struct Dispatcher* dispatcher,
               int                fd,
               unsigned int       eventMask,
               void*              userData)
{
   struct ASAPInstance*    asapInstance = (struct ASAPInstance*)userData;
   struct RSerPoolMessage* message;
   ssize_t                 received;
   union sockaddr_union    sourceAddress;
   socklen_t               sourceAddressLength;
   uint32_t                ppid;
   sctp_assoc_t            assocID;
   int                     flags;
   char                    buffer[65536];

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE2
   fputs("Entering Connection Handler...\n", stdlog);
   LOG_END

   if( ((fd == asapInstance->RegistrarHuntSocket) || (fd == asapInstance->RegistrarSocket)) &&
       (eventMask & (FDCE_Read|FDCE_Exception)) ) {
      flags               = 0;
      sourceAddressLength = sizeof(sourceAddress);
      received = recvfromplus(fd,
                              (char*)&buffer, sizeof(buffer),
                              &flags,
                              (struct sockaddr*)&sourceAddress, &sourceAddressLength,
                              &ppid, &assocID, NULL,
                              asapInstance->RegistrarResponseTimeout);
      if(received > 0) {
         /* ====== Handle notification ================================ */
         if(flags & MSG_NOTIFICATION) {
            if(fd == asapInstance->RegistrarSocket) {
               handleNotificationOnRegistrarSocket(asapInstance,
                                                   (union sctp_notification*)&buffer);
            }
            else if(fd == asapInstance->RegistrarHuntSocket) {
               registrarTableHandleNotificationOnRegistrarHuntSocket(asapInstance->RegistrarSet,
                                                                     asapInstance->RegistrarHuntSocket,
                                                                     (union sctp_notification*)&buffer);
            }
         }

         /* ====== Handle message ===================================== */
         else {
            if(rserpoolPacket2Message((char*)&buffer,
                                       &sourceAddress,
                                       assocID,
                                       ppid,
                                       received, sizeof(buffer),
                                       &message) == RSPERR_OKAY) {
               if(message->Type == AHT_ENDPOINT_KEEP_ALIVE) {
                  asapInstanceHandleEndpointKeepAlive(asapInstance, message, fd);
               }
               else {
                  LOG_WARNING
                  fprintf(stdlog, "Received unexpected message type $%04x\n",
                           message->Type);
                  LOG_END
               }
               rserpoolMessageDelete(message);
            }
            else {
               LOG_WARNING
               fputs("Disconnecting from registrar due to failure\n", stdlog);
               LOG_END
               asapInstanceDisconnectFromRegistrar(asapInstance, true);
            }
         }
      }
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "Event for unknown socket %d\n", fd);
      LOG_END
   }

   LOG_VERBOSE2
   fputs("Leaving Connection Handler\n", stdlog);
   LOG_END

   dispatcherUnlock(asapInstance->StateMachine);
}
