/*
 *  $Id: asapinstance.c,v 1.30 2004/11/19 16:42:46 dreibh Exp $
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


static void handleRegistrarConnectionEvent(struct Dispatcher* dispatcher,
                                            int                fd,
                                            unsigned int       eventMask,
                                            void*              userData);
static void asapInstanceHandleEndpointKeepAlive(
               struct ASAPInstance*    asapInstance,
               struct RSerPoolMessage* message);
static void asapInstanceDisconnectFromRegistrar(struct ASAPInstance* asapInstance);


/* ###### Get configuration from file #################################### */
static void asapInstanceConfigure(struct ASAPInstance* asapInstance, struct TagItem* tags)
{
   /* ====== ASAP Instance settings ======================================= */
   asapInstance->RegistrarRequestMaxTrials = tagListGetData(tags, TAG_RspLib_RegistrarRequestMaxTrials,
                                                             ASAP_DEFAULT_NAMESERVER_REQUEST_MAXTRIALS);
   asapInstance->RegistrarRequestTimeout = (card64)tagListGetData(tags, TAG_RspLib_RegistrarRequestTimeout,
                                                                   ASAP_DEFAULT_NAMESERVER_REQUEST_TIMEOUT);
   asapInstance->RegistrarResponseTimeout = (card64)tagListGetData(tags, TAG_RspLib_RegistrarResponseTimeout,
                                                                    ASAP_DEFAULT_NAMESERVER_RESPONSE_TIMEOUT);
   asapInstance->CacheElementTimeout = (card64)tagListGetData(tags, TAG_RspLib_CacheElementTimeout,
                                                              ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT);


   /* ====== Show results =================================================== */
   LOG_VERBOSE3
   fputs("New ASAP instance's configuration:\n", stdlog);
   fprintf(stdlog, "registrar.request.maxtrials   = %u\n",     (unsigned int)asapInstance->RegistrarRequestMaxTrials);
   fprintf(stdlog, "registrar.request.timeout     = %lluus\n", asapInstance->RegistrarRequestTimeout);
   fprintf(stdlog, "registrar.response.timeout    = %lluus\n", asapInstance->RegistrarResponseTimeout);
   fprintf(stdlog, "cache.elementtimeout           = %lluus\n", asapInstance->CacheElementTimeout);
   LOG_END
}


/* ###### Constructor #################################################### */
struct ASAPInstance* asapInstanceNew(struct Dispatcher* dispatcher,
                                     struct TagItem*    tags)
{
   struct ASAPInstance* asapInstance = NULL;
   if(dispatcher != NULL) {
      asapInstance = (struct ASAPInstance*)malloc(sizeof(struct ASAPInstance));
      if(asapInstance != NULL) {
         asapInstance->StateMachine = dispatcher;

         asapInstanceConfigure(asapInstance, tags);

         asapInstance->RegistrarConnectionTimeStamp = 0;
         asapInstance->RegistrarSocket              = -1;
         asapInstance->RegistrarID                  = 0;
         asapInstance->RegistrarSocketProtocol      = 0;
         ST_CLASS(poolHandlespaceManagementNew)(&asapInstance->Cache,
                                              0x00000000,
                                              NULL, NULL, NULL);
         ST_CLASS(poolHandlespaceManagementNew)(&asapInstance->OwnPoolElements,
                                              0x00000000,
                                              NULL, NULL, NULL);
         asapInstance->Buffer          = messageBufferNew(65536);
         asapInstance->RegistrarTable = serverTableNew(asapInstance->StateMachine, tags);
         if((asapInstance->Buffer == NULL) || (asapInstance->RegistrarTable == NULL)) {
            asapInstanceDelete(asapInstance);
            asapInstance = NULL;
         }
      }
   }
   return(asapInstance);
}


/* ###### Destructor ##################################################### */
void asapInstanceDelete(struct ASAPInstance* asapInstance)
{
   if(asapInstance) {
      asapInstanceDisconnectFromRegistrar(asapInstance);
      ST_CLASS(poolHandlespaceManagementClear)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolHandlespaceManagementDelete)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolHandlespaceManagementClear)(&asapInstance->Cache);
      ST_CLASS(poolHandlespaceManagementDelete)(&asapInstance->Cache);
      if(asapInstance->RegistrarTable) {
         serverTableDelete(asapInstance->RegistrarTable);
         asapInstance->RegistrarTable = NULL;
      }
      if(asapInstance->AsapServerAnnounceConfigFile) {
         free(asapInstance->AsapServerAnnounceConfigFile);
         asapInstance->AsapServerAnnounceConfigFile = NULL;
      }
      if(asapInstance->AsapRegistrarsConfigFile) {
         free(asapInstance->AsapRegistrarsConfigFile);
         asapInstance->AsapRegistrarsConfigFile = NULL;
      }
      if(asapInstance->Buffer) {
         messageBufferDelete(asapInstance->Buffer);
         asapInstance->Buffer = NULL;
      }
      free(asapInstance);
   }
}


/* ###### Connect to registrar ############################################ */
static bool asapInstanceConnectToRegistrar(struct ASAPInstance* asapInstance,
                                            const int            protocol)
{
   struct sctp_event_subscribe events;
   bool                        result = true;

   if(asapInstance->RegistrarSocket < 0) {
      asapInstance->RegistrarSocket =
         serverTableFindServer(asapInstance->RegistrarTable,
                               &asapInstance->RegistrarID);
      if(asapInstance->RegistrarSocket >= 0) {
         asapInstance->RegistrarConnectionTimeStamp = getMicroTime();
         asapInstance->RegistrarSocketProtocol      = protocol;

         /* ====== Enable data IO events ================================= */
         if(asapInstance->RegistrarSocketProtocol == IPPROTO_SCTP) {
            memset((char*)&events, 0 ,sizeof(events));
            events.sctp_data_io_event = 1;
            if(ext_setsockopt(asapInstance->RegistrarSocket,
                              IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events)) < 0) {
               LOG_ERROR
               perror("setsockopt() failed for SCTP_EVENTS on registrar socket");
               LOG_END
            }
         }

         fdCallbackNew(&asapInstance->RegistrarFDCallback,
                       asapInstance->StateMachine,
                       asapInstance->RegistrarSocket,
                       FDCE_Read|FDCE_Exception,
                       handleRegistrarConnectionEvent,
                       (void*)asapInstance);

         LOG_NOTE
         fputs("Connection to registrar successfully established\n", stdlog);
         LOG_END
      }
      else {
         LOG_ERROR
         fputs("Unable to connect to a registrar\n", stdlog);
         LOG_END
         result = false;
      }
   }

   return(result);
}


/* ###### Disconnect from registrar #################################### */
static void asapInstanceDisconnectFromRegistrar(struct ASAPInstance* asapInstance)
{
   if(asapInstance->RegistrarSocket >= 0) {
      fdCallbackDelete(&asapInstance->RegistrarFDCallback);
      ext_close(asapInstance->RegistrarSocket);
      LOG_NOTE
      fputs("Disconnected from registrar\n", stdlog);
      LOG_END
      asapInstance->RegistrarConnectionTimeStamp = 0;
      asapInstance->RegistrarSocket              = -1;
      asapInstance->RegistrarID                  = 0;
      asapInstance->RegistrarSocketProtocol      = 0;
   }
}


/* ###### Receive response from registrar ############################## */
static unsigned int asapInstanceReceiveResponse(struct ASAPInstance*     asapInstance,
                                                struct RSerPoolMessage** message)
{
   ssize_t              received;
   unsigned int         result;
   union sockaddr_union sourceAddress;
   socklen_t            sourceAddressLength;

   LOG_VERBOSE2
   fputs("Waiting for response...\n",stdlog);
   LOG_END

   do {
      sourceAddressLength = sizeof(sourceAddress);
      received = messageBufferRead(asapInstance->Buffer, asapInstance->RegistrarSocket,
                                   &sourceAddress, &sourceAddressLength,
                                   (asapInstance->RegistrarSocketProtocol == IPPROTO_SCTP) ? PPID_ASAP : 0,
                                   asapInstance->RegistrarResponseTimeout,
                                   asapInstance->RegistrarResponseTimeout);
      LOG_VERBOSE2
      fprintf(stdlog, "received=%d, timeout=%Ld\n", received, asapInstance->RegistrarResponseTimeout);
      LOG_END
   } while(received == RspRead_PartialRead);

   if(received > 0) {
      result = rserpoolPacket2Message((char*)&asapInstance->Buffer->Buffer,
                                      &sourceAddress,
                                      PPID_ASAP,
                                      received, asapInstance->Buffer->Size,
                                      message);
      if(result != RSPERR_OKAY) {
         if(*message) {
            rserpoolMessageDelete(*message);
            *message = NULL;
         }
      }
   }
   else {
      *message = NULL;
   }

   if(*message != NULL) {
      LOG_VERBOSE2
      fputs("Response successfully received\n",stdlog);
      LOG_END
      return(RSPERR_OKAY);
   }

   LOG_WARNING
   fputs("Receiving response failed\n", stdlog);
   LOG_END
   return(RSPERR_READ_ERROR);
}


/* ###### Send request to registrar #################################### */
static unsigned int asapInstanceSendRequest(struct ASAPInstance*     asapInstance,
                                            struct RSerPoolMessage*  request)
{
   bool result;

   if(asapInstanceConnectToRegistrar(asapInstance, IPPROTO_SCTP) == false) {
      LOG_ERROR
      fputs("No registrar available\n", stdlog);
      LOG_END
      return(RSPERR_NO_REGISTRAR);
   }

   result = rserpoolMessageSend(asapInstance->RegistrarSocketProtocol,
                                asapInstance->RegistrarSocket,
                                0, 0,
                                asapInstance->RegistrarRequestTimeout,
                                request);
   if(result) {
      return(RSPERR_OKAY);
   }

   LOG_ERROR
   fputs("Sending request failed\n", stdlog);
   LOG_END
   return(RSPERR_WRITE_ERROR);
}


/* ###### Send request to registrar and wait for response ############## */
static unsigned int asapInstanceDoIO(struct ASAPInstance*     asapInstance,
                                     struct RSerPoolMessage*  message,
                                     struct RSerPoolMessage** responsePtr,
                                     uint16_t*                error)
{
   struct RSerPoolMessage* response;
   unsigned int            result = RSPERR_OKAY;
   size_t                  i;

   *responsePtr = NULL;
   *error = RSPERR_OKAY;

   for(i = 0;i < asapInstance->RegistrarRequestMaxTrials;i++) {
      LOG_VERBOSE2
      fprintf(stdlog, "Request trial #%u - sending request...\n", (unsigned int)i + 1);
      LOG_END

      result = asapInstanceSendRequest(asapInstance, message);
      if(result == RSPERR_OKAY) {
         LOG_VERBOSE2
         fprintf(stdlog, "Request trial #%u - waiting for response...\n", (unsigned int)i + 1);
         LOG_END
         result = asapInstanceReceiveResponse(asapInstance, &response);
         while(result == RSPERR_OKAY) {
            *error = response->Error;

            if(response->Type == AHT_ENDPOINT_KEEP_ALIVE) {
               asapInstanceHandleEndpointKeepAlive(asapInstance, response);
            }
            else if(response->Type == AHT_ENDPOINT_KEEP_ALIVE_ACK) {
            }
            else if( ((response->Type == AHT_REGISTRATION_RESPONSE)    && (message->Type == AHT_REGISTRATION))   ||
                     ((response->Type == AHT_DEREGISTRATION_RESPONSE)  && (message->Type == AHT_DEREGISTRATION)) ||
                     ((response->Type == AHT_NAME_RESOLUTION_RESPONSE) && (message->Type == AHT_NAME_RESOLUTION)) ) {
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
            result = asapInstanceReceiveResponse(asapInstance, &response);
         }
      }

      LOG_ERROR
      fprintf(stdlog, "Request trial #%u failed\n", (unsigned int)i + 1);
      LOG_END
      asapInstanceDisconnectFromRegistrar(asapInstance);
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

   LOG_ACTION
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
         if(registrarResult == RSPERR_OKAY) {
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

   LOG_ACTION
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

   LOG_ACTION
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

   LOG_ACTION
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

   LOG_ACTION
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
      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_OUT_OF_MEMORY;
   }

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}


/* ###### Do name lookup ################################################# */
static unsigned int asapInstanceDoNameResolution(struct ASAPInstance* asapInstance,
                                                 struct PoolHandle*   poolHandle)
{
   struct ST_CLASS(PoolElementNode)* newPoolElementNodeNode;
   struct RSerPoolMessage*           message;
   struct RSerPoolMessage*           response;
   unsigned int                      result;
   uint16_t                          registrarResult;
   size_t                            i;

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type   = AHT_NAME_RESOLUTION;
      message->Flags  = 0x00;
      message->Handle = *poolHandle;

      result = asapInstanceDoIO(asapInstance, message, &response, &registrarResult);
      if(result == RSPERR_OKAY) {
         if(registrarResult == RSPERR_OKAY) {
            LOG_VERBOSE
            fprintf(stdlog, "Got %u elements in name resolution response\n",
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
                           &newPoolElementNodeNode);
               if(result != RSPERR_OKAY) {
                  LOG_WARNING
                  fputs("Failed to add pool element to cache: ", stdlog);
                  ST_CLASS(poolElementNodePrint)(response->PoolElementPtrArray[i], stdlog, PENPO_FULL);
                  fputs(": ", stdlog);
                  rserpoolErrorPrint(result, stdlog);
                  fputs("\n", stdlog);
                  LOG_END
               }
            }
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Name Resolution at registrar for pool ");
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
         LOG_WARNING
         fprintf(stdlog, "Name Resolution at registrar for pool ");
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
static unsigned int asapInstanceNameResolutionFromCache(
                       struct ASAPInstance*               asapInstance,
                       struct PoolHandle*                 poolHandle,
                       struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                       size_t*                            poolElementNodes)
{
   unsigned int result;
   size_t       i;

   dispatcherLock(asapInstance->StateMachine);

   LOG_ACTION
   fprintf(stdlog, "Name Resolution for pool ");
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

   if(ST_CLASS(poolHandlespaceManagementNameResolution)(
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
      LOG_END
      result = RSPERR_OKAY;
   }
   else {
      result = RSPERR_NOT_FOUND;
   }

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}


/* ###### Do name resolution for given pool handle ####################### */
unsigned int asapInstanceNameResolution(
                struct ASAPInstance*               asapInstance,
                struct PoolHandle*                 poolHandle,
                struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                size_t*                            poolElementNodes)
{
   unsigned int result;
   const size_t originalPoolElementNodes = *poolElementNodes;

   LOG_VERBOSE
   fputs("Trying name resolution from cache...\n", stdlog);
   LOG_END

   result = asapInstanceNameResolutionFromCache(
               asapInstance, poolHandle,
               poolElementNodeArray,
               poolElementNodes);
   if(result != RSPERR_OKAY) {
      LOG_VERBOSE
      fputs("No results in cache. Trying name resolution at registrar...\n", stdlog);
      LOG_END

      /* The amount is now 0 (since name resolution was not successful).
         Set it to its original value. */
      *poolElementNodes = originalPoolElementNodes;

      result = asapInstanceDoNameResolution(asapInstance, poolHandle);
      if(result == RSPERR_OKAY) {
         result = asapInstanceNameResolutionFromCache(
                     asapInstance, poolHandle,
                     poolElementNodeArray,
                     poolElementNodes);
      }
      else {
         LOG_VERBOSE
         fputs("Name resolution not succesful\n", stdlog);
         LOG_END
      }
   }

   return(result);
}


/* ###### Handle endpoint keepalive ###################################### */
static void asapInstanceHandleEndpointKeepAlive(
               struct ASAPInstance*    asapInstance,
               struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   LOG_VERBOSE2
   fprintf(stdlog, "Endpoint KeepAlive for $%08x of pool ", message->Identifier);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   poolElementNode = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                        &asapInstance->OwnPoolElements,
                        &message->Handle,
                        message->Identifier);
   if(poolElementNode) {
      message->Type       = AHT_ENDPOINT_KEEP_ALIVE_ACK;
      message->Flags      = 0x00;
      message->Identifier = poolElementNode->Identifier;

      LOG_VERBOSE2
      fprintf(stdlog, "Sending KeepAliveAck for pool element $%08x\n",message->Identifier);
      LOG_END
      asapInstanceSendRequest(asapInstance, message);
   }
   else {
      LOG_WARNING
      fprintf(stdlog, "Endpoint KeepAlive for $%08x of pool ",
              message->Identifier);
      poolHandlePrint(&message->Handle, stdlog);
      fputs(" received. We do *not* own this PE\n", stdlog);
      LOG_END
   }
}


/* ###### Handle incoming requests from registrar (keepalives) ######### */
static void handleRegistrarConnectionEvent(
               struct Dispatcher* dispatcher,
               int                fd,
               unsigned int       eventMask,
               void*              userData)
{
   struct ASAPInstance*     asapInstance = (struct ASAPInstance*)userData;
   struct RSerPoolMessage*  message;
   unsigned int             result;

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE2
   fputs("Entering Connection Handler...\n", stdlog);
   LOG_END

   if(fd == asapInstance->RegistrarSocket) {
      if(eventMask & (FDCE_Read|FDCE_Exception)) {
         result = asapInstanceReceiveResponse(asapInstance, &message);
         if(result == RSPERR_OKAY) {
            if(message->Type == AHT_ENDPOINT_KEEP_ALIVE) {
               asapInstanceHandleEndpointKeepAlive(asapInstance, message);
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
            asapInstanceDisconnectFromRegistrar(asapInstance);
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
