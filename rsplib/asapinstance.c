/*
 *  $Id: asapinstance.c,v 1.12 2004/07/25 16:55:03 dreibh Exp $
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
#include "rsplib-tags.h"

#include <ext_socket.h>


static void handleNameServerConnectionEvent(struct Dispatcher* dispatcher,
                                            int                fd,
                                            int                eventMask,
                                            void*              userData);


/* ###### Get configuration from file #################################### */
static void asapInstanceConfigure(struct ASAPInstance* asapInstance, struct TagItem* tags)
{
   /* ====== ASAP Instance settings ======================================= */
   asapInstance->NameServerRequestMaxTrials = tagListGetData(tags, TAG_RspLib_NameServerRequestMaxTrials,
                                                     ASAP_DEFAULT_NAMESERVER_REQUEST_MAXTRIALS);
   asapInstance->NameServerRequestTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerRequestTimeout,
                                                           ASAP_DEFAULT_NAMESERVER_REQUEST_TIMEOUT);
   asapInstance->NameServerResponseTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerResponseTimeout,
                                                            ASAP_DEFAULT_NAMESERVER_RESPONSE_TIMEOUT);
   asapInstance->CacheElementTimeout = (card64)tagListGetData(tags, TAG_RspLib_CacheElementTimeout,
                                                      ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT);

   /* ====== Show results =================================================== */
   LOG_VERBOSE3
   fputs("New ASAP instance's configuration:\n", stdlog);
   fprintf(stdlog, "nameserver.request.maxtrials   = %u\n",       asapInstance->NameServerRequestMaxTrials);
   fprintf(stdlog, "nameserver.request.timeout     = %Lu [µs]\n", asapInstance->NameServerRequestTimeout);
   fprintf(stdlog, "nameserver.response.timeout    = %Lu [µs]\n", asapInstance->NameServerResponseTimeout);
   fprintf(stdlog, "cache.elementtimeout           = %Lu [µs]\n", asapInstance->CacheElementTimeout);
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

         asapInstance->NameServerSocket         = -1;
         asapInstance->NameServerSocketProtocol = 0;
         ST_CLASS(poolNamespaceManagementNew)(&asapInstance->Cache,
                                              0x00000000,
                                              NULL, NULL, NULL);
         ST_CLASS(poolNamespaceManagementNew)(&asapInstance->OwnPoolElements,
                                              0x00000000,
                                              NULL, NULL, NULL);
         asapInstance->Buffer          = messageBufferNew(65536);
         asapInstance->NameServerTable = serverTableNew(asapInstance->StateMachine, tags);
         if((asapInstance->Buffer == NULL) || (asapInstance->NameServerTable == NULL)) {
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
      ST_CLASS(poolNamespaceManagementClear)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolNamespaceManagementDelete)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolNamespaceManagementClear)(&asapInstance->Cache);
      ST_CLASS(poolNamespaceManagementDelete)(&asapInstance->Cache);
      if(asapInstance->NameServerTable) {
         serverTableDelete(asapInstance->NameServerTable);
         asapInstance->NameServerTable = NULL;
      }
      if(asapInstance->AsapServerAnnounceConfigFile) {
         free(asapInstance->AsapServerAnnounceConfigFile);
         asapInstance->AsapServerAnnounceConfigFile = NULL;
      }
      if(asapInstance->AsapNameServersConfigFile) {
         free(asapInstance->AsapNameServersConfigFile);
         asapInstance->AsapNameServersConfigFile = NULL;
      }
      if(asapInstance->Buffer) {
         messageBufferDelete(asapInstance->Buffer);
         asapInstance->Buffer = NULL;
      }
      free(asapInstance);
   }
}


/* ###### Connect to name server ######################################### */
static bool asapInstanceConnectToNameServer(struct ASAPInstance* asapInstance,
                                            const int            protocol)
{
   struct sctp_event_subscribe events;
   bool                        result = true;

   if(asapInstance->NameServerSocket < 0) {
      asapInstance->NameServerSocket =
         serverTableFindServer(asapInstance->NameServerTable);
      if(asapInstance->NameServerSocket >= 0) {
         asapInstance->NameServerSocketProtocol = protocol;

         /* ====== Enable data IO events ================================= */
         if(asapInstance->NameServerSocketProtocol == IPPROTO_SCTP) {
            memset((char*)&events, 0 ,sizeof(events));
            events.sctp_data_io_event = 1;
            if(ext_setsockopt(asapInstance->NameServerSocket,
                              IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events)) < 0) {
               LOG_ERROR
               perror("setsockopt() failed for SCTP_EVENTS on name server socket");
               LOG_END
            }
         }

         dispatcherAddFDCallback(asapInstance->StateMachine,
                                 asapInstance->NameServerSocket,
                                 FDCE_Read|FDCE_Exception,
                                 handleNameServerConnectionEvent,
                                 (gpointer)asapInstance);
         LOG_ACTION
         fputs("Connection to name server server successfully established\n", stdlog);
         LOG_END
      }
      else {
         LOG_ERROR
         fputs("Unable to connect to a name server server\n", stdlog);
         LOG_END
         result = false;
      }
   }

   return(result);
}


/* ###### Disconnect from name server #################################### */
static void asapInstanceDisconnectFromNameServer(struct ASAPInstance* asapInstance)
{
   if(asapInstance->NameServerSocket >= 0) {
      dispatcherRemoveFDCallback(asapInstance->StateMachine, asapInstance->NameServerSocket);
      ext_close(asapInstance->NameServerSocket);
      LOG_ACTION
      fputs("Disconnected from nameserver\n", stdlog);
      LOG_END
      asapInstance->NameServerSocket         = -1;
      asapInstance->NameServerSocketProtocol = 0;
   }
}


/* ###### Receive response from name server ############################## */
static unsigned int asapInstanceReceiveResponse(struct ASAPInstance*     asapInstance,
                                                struct RSerPoolMessage** message)
{
   ssize_t received;

   LOG_VERBOSE2
   fputs("Waiting for response...\n",stdlog);
   LOG_END

   do {
      received = messageBufferRead(asapInstance->Buffer, asapInstance->NameServerSocket,
                                   (asapInstance->NameServerSocketProtocol == IPPROTO_SCTP) ? PPID_ASAP : 0,
                                   asapInstance->NameServerResponseTimeout,
                                   asapInstance->NameServerResponseTimeout);
      LOG_VERBOSE2
      fprintf(stdlog, "received=%d\n", received);
      LOG_END
   } while((received == RspRead_PartialRead) || (received == RspRead_Timeout));

   if(received > 0) {
      *message = rserpoolPacket2Message((char*)&asapInstance->Buffer->Buffer,
                                        PPID_ASAP,
                                        received, asapInstance->Buffer->Size);
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


/* ###### Send request to name server #################################### */
static unsigned int asapInstanceSendRequest(struct ASAPInstance*     asapInstance,
                                            struct RSerPoolMessage*  request)
{
   bool result;

   if(asapInstanceConnectToNameServer(asapInstance, IPPROTO_SCTP) == false) {
      LOG_ERROR
      fputs("No name server server available\n", stdlog);
      LOG_END
      return(RSPERR_NO_NAMESERVER);
   }

   result = rserpoolMessageSend(asapInstance->NameServerSocket,
                            0,
                            asapInstance->NameServerRequestTimeout,
                            request);
   if(result) {
      return(RSPERR_OKAY);
   }

   LOG_ERROR
   fputs("Sending request failed\n", stdlog);
   LOG_END
   return(RSPERR_WRITE_ERROR);
}


/* ###### Send request to name server and wait for response ############## */
static unsigned int asapInstanceDoIO(struct ASAPInstance*     asapInstance,
                                     struct RSerPoolMessage*  message,
                                     struct RSerPoolMessage** responsePtr,
                                     uint16_t*                error)
{
   struct RSerPoolMessage* response;
   unsigned int        result = RSPERR_OKAY;
   cardinal            i;

   if(responsePtr != NULL) {
      *responsePtr = NULL;
   }
   *error = RSPERR_OKAY;


   for(i = 0;i < asapInstance->NameServerRequestMaxTrials;i++) {
      LOG_VERBOSE2
      fprintf(stdlog, "Request trial #%d - sending request...\n",i + 1);
      LOG_END

      result = asapInstanceSendRequest(asapInstance, message);
      if(result == RSPERR_OKAY) {
         LOG_VERBOSE2
         fprintf(stdlog, "Request trial #%d - waiting for response...\n",i + 1);
         LOG_END
         result = asapInstanceReceiveResponse(asapInstance, &response);
         while(response != NULL) {
            *error = response->Error;

            if(response->Type == AHT_ENDPOINT_KEEP_ALIVE) {
            }
            else if(response->Type == AHT_ENDPOINT_KEEP_ALIVE_ACK) {
            }
            else if( ((response->Type == AHT_REGISTRATION_RESPONSE)    && (message->Type == AHT_REGISTRATION))   ||
                     ((response->Type == AHT_DEREGISTRATION_RESPONSE)  && (message->Type == AHT_DEREGISTRATION)) ||
                     ((response->Type == AHT_NAME_RESOLUTION_RESPONSE) && (message->Type == AHT_NAME_RESOLUTION)) ) {
               if(responsePtr != NULL) {
                  *responsePtr = response;
               }
               else {
                  rserpoolMessageDelete(response);
               }
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
            if(result == RSPERR_OKAY) {
               LOG_VERBOSE2
               fprintf(stdlog, "Request trial #%d - Success\n",i + 1);
               LOG_END
               return(result);
            }
         }
      }

      LOG_ERROR
      fprintf(stdlog, "Request trial #%d failed\n",i + 1);
      LOG_END
      asapInstanceDisconnectFromNameServer(asapInstance);
   }

   return(result);
}


/* ###### Register pool element ########################################## */
unsigned int asapInstanceRegister(struct ASAPInstance*              asapInstance,
                                  struct PoolHandle*                poolHandle,
                                  struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct RSerPoolMessage*           message;
   struct ST_CLASS(PoolElementNode)* newPoolElement;
   unsigned int                      result;
   uint16_t                          nameServerResult;
   unsigned int                      namespaceMgtResult;

//puts("WAITING...");
   dispatcherLock(asapInstance->StateMachine);
//   puts("WAITING... OK!!!");

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

      result = asapInstanceDoIO(asapInstance, message, NULL, &nameServerResult);
      if(result == RSPERR_OKAY) {
         if(nameServerResult == RSPERR_OKAY) {
            namespaceMgtResult = ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
                                    &asapInstance->OwnPoolElements,
                                    poolHandle,
                                    message->PoolElementPtr->HomeNSIdentifier,
                                    message->PoolElementPtr->Identifier,
                                    message->PoolElementPtr->RegistrationLife,
                                    &message->PoolElementPtr->PolicySettings,
                                    message->PoolElementPtr->AddressBlock,
                                    getMicroTime(),
                                    &newPoolElement);
            if(namespaceMgtResult == RSPERR_OKAY) {
               newPoolElement->UserData = (void*)asapInstance;
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
            result = (unsigned int)nameServerResult;
         }
      }

      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_NO_RESOURCES;
   }

   LOG_ACTION
   fputs("Registration result is ", stdlog);
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
   unsigned int        result;
   uint16_t            nameServerResult;
   unsigned int        namespaceMgtResult;

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

      result = asapInstanceDoIO(asapInstance, message, NULL, &nameServerResult);
      if((result == RSPERR_OKAY) && (nameServerResult == RSPERR_OKAY)) {
         namespaceMgtResult = ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
                                 &asapInstance->OwnPoolElements,
                                 poolHandle,
                                 identifier);
         LOG_ERROR
         fprintf(stdlog, "Unable to deregister pool element $%08x of pool ",
                 identifier);
         poolHandlePrint(poolHandle, stdlog);
         fputs(" from OwnPoolElements\n", stdlog);
         LOG_END_FATAL
      }

      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_NO_RESOURCES;
   }

   LOG_ACTION
   fputs("Deregistration result is ", stdlog);
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
   struct RSerPoolMessage*               message;
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

   found = ST_CLASS(poolNamespaceManagementFindPoolElement)(
              &asapInstance->Cache,
              poolHandle,
              identifier);
   if(found != NULL) {
      result = ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
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

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}


/* ###### Do name lookup ################################################# */
static unsigned int asapInstanceDoNameResolution(struct ASAPInstance* asapInstance,
                                                 struct PoolHandle*   poolHandle)
{
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct RSerPoolMessage*           message;
   struct RSerPoolMessage*           response;
   unsigned int                      result;
   uint16_t                          nameServerResult;
   size_t                            i;

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type   = AHT_NAME_RESOLUTION;
      message->Flags  = 0x00;
      message->Handle = *poolHandle;

      result = asapInstanceDoIO(asapInstance, message, &response, &nameServerResult);
      if(result == RSPERR_OKAY) {
         if(nameServerResult == RSPERR_OKAY) {
            LOG_VERBOSE
            fprintf(stdlog, "Got %u elements in name resolution response\n",
                    response->PoolElementPtrArraySize);
            LOG_END
            for(i = 0;i < response->PoolElementPtrArraySize;i++) {
               LOG_VERBOSE2
               fputs("Adding pool element to cache: ", stdlog);
               ST_CLASS(poolElementNodePrint)(response->PoolElementPtrArray[i], stdlog, PENPO_FULL);
               fputs("\n", stdlog);
               LOG_END
               result = ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
                           &asapInstance->Cache,
                           poolHandle,
                           response->PoolElementPtrArray[i]->HomeNSIdentifier,
                           response->PoolElementPtrArray[i]->Identifier,
                           response->PoolElementPtrArray[i]->RegistrationLife,
                           &response->PoolElementPtrArray[i]->PolicySettings,
                           response->PoolElementPtrArray[i]->AddressBlock,
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
            }
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Name Resolution at name server for pool ");
            poolHandlePrint(poolHandle, stdlog);
            fputs("failed: ", stdlog);
            rserpoolErrorPrint(nameServerResult, stdlog);
            fputs("\n", stdlog);
            LOG_END
            result = nameServerResult;
         }
         rserpoolMessageDelete(response);
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "Name Resolution at name server for pool ");
         poolHandlePrint(poolHandle, stdlog);
         fputs("failed: ", stdlog);
         rserpoolErrorPrint(nameServerResult, stdlog);
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
   ST_CLASS(poolNamespaceManagementPrint)(&asapInstance->Cache,
                                          stdlog, PENPO_ONLY_POLICY);
   LOG_END

   i = ST_CLASS(poolNamespaceManagementPurgeExpiredPoolElements)(
          &asapInstance->Cache,
          getMicroTime());
   LOG_VERBOSE
   fprintf(stdlog, "Purged %u out-of-date elements\n", i);
   LOG_END

   if(ST_CLASS(poolNamespaceManagementNameResolution)(
         &asapInstance->Cache,
         poolHandle,
         poolElementNodeArray,
         poolElementNodes,
         *poolElementNodes,
         1000000000) == RSPERR_OKAY) {
      LOG_VERBOSE
      fprintf(stdlog, "Got %u items:\n", *poolElementNodes);
      for(i = 0;i < *poolElementNodes;i++) {
         fprintf(stdlog, "#%u: ", i + 1);
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
      fputs("No results in cache. Trying name resolution at name server...\n", stdlog);
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

   poolElementNode = ST_CLASS(poolNamespaceManagementFindPoolElement)(
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


/* ###### Handle incoming requests from name server (keepalives) ######### */
static void handleNameServerConnectionEvent(
               struct Dispatcher* dispatcher,
               int                fd,
               int                eventMask,
               void*              userData)
{
   struct ASAPInstance* asapInstance = (struct ASAPInstance*)userData;
   struct RSerPoolMessage*  message;
   unsigned int       result;

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE2
   fputs("Entering Connection Handler...\n", stdlog);
   LOG_END

   if(eventMask & (FDCE_Read|FDCE_Exception)) {
      result = asapInstanceReceiveResponse(asapInstance, &message);
      if(result == RSPERR_OKAY) {
         if(message->Type == AHT_ENDPOINT_KEEP_ALIVE) {
            asapInstanceHandleEndpointKeepAlive(asapInstance, message);
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Received unexpected message type #%d\n",
                    message->Type);
            LOG_END
         }
         rserpoolMessageDelete(message);
      }
      else {
         LOG_ACTION
         fputs("Disconnecting from name server server due to failure\n", stdlog);
         LOG_END
         asapInstanceDisconnectFromNameServer(asapInstance);
      }
   }

   LOG_VERBOSE2
   fputs("Leaving Connection Handler\n", stdlog);
   LOG_END

   dispatcherUnlock(asapInstance->StateMachine);
}
