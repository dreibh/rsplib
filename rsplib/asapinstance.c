/*
 *  $Id: asapinstance.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
#include "asapmessage.h"
#include "asapcreator.h"
#include "asapparser.h"
#include "asapcache.h"
#include "utilities.h"
#include "asaperror.h"
#include "asapcache.h"
#include "rsplib-tags.h"


#include <ext_socket.h>



static void asapNameServerConnectionHandler(struct Dispatcher* dispatcher,
                                            int                fd,
                                            int                eventMask,
                                            void*              userData);

static void asapDisconnectFromNameServer(struct ASAPInstance* asap);



/* ###### Get configuration from file #################################### */
static void asapConfigure(struct ASAPInstance* asap, struct TagItem* tags)
{
   /* ====== ASAP Instance settings ======================================= */
   asap->NameServerRequestMaxTrials = tagListGetData(tags, TAG_RspLib_NameServerRequestMaxTrials,
                                                     ASAP_DEFAULT_NAMESERVER_REQUEST_MAXTRIALS);
   asap->NameServerRequestTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerRequestTimeout,
                                                           ASAP_DEFAULT_NAMESERVER_REQUEST_TIMEOUT);
   asap->NameServerResponseTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerResponseTimeout,
                                                            ASAP_DEFAULT_NAMESERVER_RESPONSE_TIMEOUT);
   asap->CacheElementTimeout = (card64)tagListGetData(tags, TAG_RspLib_CacheElementTimeout,
                                                      ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT);
   asap->CacheMaintenanceInterval = (card64)tagListGetData(tags, TAG_RspLib_CacheMaintenanceInterval,
                                                           ASAP_DEFAULT_CACHE_MAINTENANCE_INTERVAL);

   /* ====== Show results =================================================== */
   LOG_VERBOSE3
   fputs("New ASAP instance's configuration:\n", stdlog);
   fprintf(stdlog, "nameserver.request.maxtrials   = %u\n",       asap->NameServerRequestMaxTrials);
   fprintf(stdlog, "nameserver.request.timeout     = %Lu [µs]\n", asap->NameServerRequestTimeout);
   fprintf(stdlog, "nameserver.response.timeout    = %Lu [µs]\n", asap->NameServerResponseTimeout);
   fprintf(stdlog, "cache.elementtimeout           = %Lu [µs]\n", asap->CacheElementTimeout);
   fprintf(stdlog, "cache.mtinterval               = %Lu [µs]\n", asap->CacheMaintenanceInterval);
   LOG_END
}


/* ###### Constructor #################################################### */
struct ASAPInstance* asapNew(struct Dispatcher* dispatcher, struct TagItem* tags)
{
   struct ASAPInstance* asap = NULL;
   if(dispatcher != NULL) {
      asap = (struct ASAPInstance*)malloc(sizeof(struct ASAPInstance));
      if(asap != NULL) {
         asap->StateMachine = dispatcher;

         asapConfigure(asap, tags);

         asap->NameServerSocket         = -1;
         asap->NameServerSocketProtocol = 0;
         asap->Buffer           = messageBufferNew(65536);
         asap->Cache            = asapCacheNew(asap->StateMachine,
                                               asap->CacheMaintenanceInterval,
                                               asap->CacheElementTimeout);
         asap->NameServerTable  = serverTableNew(asap->StateMachine, tags);
         if((asap->Buffer == NULL) || (asap->Cache == NULL) || (asap->NameServerTable == NULL)) {
            asapDelete(asap);
            asap = NULL;
         }
      }
   }
   return(asap);
}


/* ###### Destructor ##################################################### */
void asapDelete(struct ASAPInstance* asap)
{
   if(asap) {
      asapDisconnectFromNameServer(asap);
      if(asap->Cache) {
         asapCacheDelete(asap->Cache);
         asap->Cache = NULL;
      }
      if(asap->NameServerTable) {
         serverTableDelete(asap->NameServerTable);
         asap->NameServerTable = NULL;
      }
      if(asap->AsapServerAnnounceConfigFile) {
         free(asap->AsapServerAnnounceConfigFile);
         asap->AsapServerAnnounceConfigFile = NULL;
      }
      if(asap->AsapNameServersConfigFile) {
         free(asap->AsapNameServersConfigFile);
         asap->AsapNameServersConfigFile = NULL;
      }
      if(asap->Buffer) {
         messageBufferDelete(asap->Buffer);
         asap->Buffer = NULL;
      }
      free(asap);
   }
}


/* ###### Connect to name server ######################################### */
static bool asapConnectToNameServer(struct ASAPInstance* asap,
                                    const int            protocol)
{
   bool result = true;

   if(asap->NameServerSocket < 0) {
      asap->NameServerSocket = serverTableFindServer(asap->NameServerTable,
                                                     protocol);
      if(asap->NameServerSocket >= 0) {
         asap->NameServerSocketProtocol = protocol;
         dispatcherAddFDCallback(asap->StateMachine,
                                 asap->NameServerSocket,
                                 FDCE_Read|FDCE_Exception,
                                 asapNameServerConnectionHandler,
                                 (gpointer)asap);
         LOG_ACTION
         fputs("Connection to name server server successfully established\n", stdlog);
         LOG_END
      }
      else {
         result = false;
         LOG_ERROR
         fputs("Unable to connect to an name server server\n", stdlog);
         LOG_END
      }
   }

   return(result);
}


/* ###### Disconnect from name server #################################### */
static void asapDisconnectFromNameServer(struct ASAPInstance* asap)
{
   if(asap->NameServerSocket >= 0) {
      dispatcherRemoveFDCallback(asap->StateMachine, asap->NameServerSocket);
      ext_close(asap->NameServerSocket);
      LOG_ACTION
      fputs("Disconnected from nameserver\n", stdlog);
      LOG_END
      asap->NameServerSocket         = -1;
      asap->NameServerSocketProtocol = 0;
   }
}


/* ###### Receive response from name server ############################## */
enum ASAPError asapReceiveResponse(struct ASAPInstance* asap, struct ASAPMessage** message)
{
   ssize_t received;

   LOG_VERBOSE2
   fputs("Waiting for response...\n",stdlog);
   LOG_END

   do {
      received = messageBufferRead(asap->Buffer, asap->NameServerSocket,
                                   (asap->NameServerSocketProtocol == IPPROTO_SCTP) ? PPID_ASAP : 0,
                                   asap->NameServerResponseTimeout,
                                   asap->NameServerResponseTimeout);
      LOG_VERBOSE2
      fprintf(stdlog, "received=%d\n", received);
      LOG_END
   } while((received == RspRead_PartialRead) || (received == RspRead_Timeout));

   if(received > 0) {
      *message = asapPacket2Message((char*)&asap->Buffer->Buffer, received, asap->Buffer->Size);
   }
   else {
      *message = NULL;
   }

   if(*message != NULL) {
      LOG_VERBOSE2
      fputs("Response successfully received\n",stdlog);
      LOG_END
      return(ASAP_Okay);
   }

   LOG_WARNING
   fputs("Receiving response failed\n", stdlog);
   LOG_END_FATAL
   return(ASAP_ReadError);
}


/* ###### Send request to name server #################################### */
static enum ASAPError asapSendRequest(struct ASAPInstance* asap,
                                      struct ASAPMessage*  request)
{
   bool result;

   if(asapConnectToNameServer(asap,0) == false) {
      LOG_ERROR
      fputs("No name server server available\n", stdlog);
      LOG_END
      return(ASAP_NoNameServerFound);
   }

   result = asapMessageSend(asap->NameServerSocket,
                            asap->NameServerRequestTimeout,
                            request);
   if(result) {
      return(ASAP_Okay);
   }

   LOG_ERROR
   fputs("Sending request failed\n", stdlog);
   LOG_END
   return(ASAP_WriteError);
}


/* ###### Send request to name server and wait for response ############## */
static enum ASAPError asapIO(struct ASAPInstance* asap,
                             struct ASAPMessage*  message,
                             struct ASAPMessage** responsePtr,
                             uint16_t*            error)
{
   struct ASAPMessage* response;
   enum ASAPError      result = ASAP_Okay;
   cardinal            i;

   if(responsePtr != NULL) {
      *responsePtr = NULL;
   }
   *error = ASAP_Okay;


   for(i = 0;i < asap->NameServerRequestMaxTrials;i++) {
      LOG_VERBOSE2
      fprintf(stdlog, "Request trial #%d - sending request...\n",i + 1);
      LOG_END

      result = asapSendRequest(asap, message);
      if(result == ASAP_Okay) {
         LOG_VERBOSE2
         fprintf(stdlog, "Request trial #%d - waiting for response...\n",i + 1);
         LOG_END
         result = asapReceiveResponse(asap, &response);
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
                  asapMessageDelete(response);
               }
               return(ASAP_Okay);
            }
            else {
               LOG_WARNING
               fprintf(stdlog, "Bad request/response type pair: %02x/%02x\n",
                       message->Type, response->Type);
               LOG_END
               asapMessageDelete(response);
               return(ASAP_InvalidValues);
            }

            asapMessageDelete(response);
            result = asapReceiveResponse(asap, &response);
            if(result == ASAP_Okay) {
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
      asapDisconnectFromNameServer(asap);
   }

   return(result);
}


/* ###### Register pool element ########################################## */
enum ASAPError asapRegister(struct ASAPInstance* asap,
                            struct PoolHandle*   poolHandle,
                            struct PoolElement*  poolElement)
{
   struct ASAPMessage* message;
   struct PoolElement* newPoolElement;
   enum ASAPError      result;
   uint16_t            error;

   dispatcherLock(asap->StateMachine);

   LOG_ACTION
   fputs("Trying to register ", stdlog);
   poolElementPrint(poolElement, stdlog);
   fputs("to pool ", stdlog);
   poolHandlePrint(poolHandle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message = asapMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type           = AHT_REGISTRATION;
      message->PoolHandlePtr  = poolHandle;
      message->PoolElementPtr = poolElement;

      result = asapIO(asap, message, NULL, &error);
      if(result == ASAP_Okay) {
         if(error == ASAP_Okay) {
            newPoolElement = asapCacheUpdatePoolElement(asap->Cache, poolHandle, message->PoolElementPtr, false);
            if(newPoolElement != NULL) {
               /* asapCacheUpdatePoolElement() may not directly increment the user counter,
                  since this would also increment it in case of re-registration only. */
               if(newPoolElement->UserCounter == 0) {
                  newPoolElement->UserCounter++;
               }
               newPoolElement->UserData = (void*)asap;
            }
         }
         else {
            result = (enum ASAPError)error;
         }
      }

      asapMessageDelete(message);
   }
   else {
      result = ASAP_OutOfMemory;
   }

   LOG_ACTION
   fputs("Registration result is ", stdlog);
   asapErrorPrint(result, stdlog);
   fputs("\n", stdlog);
   LOG_END

   dispatcherUnlock(asap->StateMachine);
   return(result);
}


/* ###### Deregister pool element ######################################## */
enum ASAPError asapDeregister(struct ASAPInstance*        asap,
                              struct PoolHandle*          poolHandle,
                              const PoolElementIdentifier identifier)
{
   struct PoolElement* poolElement;
   struct ASAPMessage* message;
   enum ASAPError      result;
   uint16_t            error;

   dispatcherLock(asap->StateMachine);

   LOG_ACTION
   fprintf(stdlog, "Trying to deregister $%08x from pool ",identifier);
   poolHandlePrint(poolHandle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message = asapMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type          = AHT_DEREGISTRATION;
      message->PoolHandlePtr = poolHandle;
      message->Identifier    = identifier;

      result = asapIO(asap,message,NULL,&error);
      if((result == ASAP_Okay) && (error == ASAP_Okay)) {
         poolElement = asapCacheFindPoolElement(asap->Cache, poolHandle, identifier);
         if(poolElement) {
            asapCachePurgePoolElement(asap->Cache, poolHandle, poolElement, true);
         }
      }

      asapMessageDelete(message);
   }
   else {
      result = ASAP_OutOfMemory;
   }

   LOG_ACTION
   fputs("Deregistration result is ", stdlog);
   asapErrorPrint(result, stdlog);
   fputs("\n", stdlog);
   LOG_END

   dispatcherUnlock(asap->StateMachine);
   return(result);
}


/* ###### Do name lookup ################################################# */
static enum ASAPError asapDoNameLookup(struct ASAPInstance* asap,
                                       struct PoolHandle*   poolHandle)
{
   struct ASAPMessage* message;
   struct ASAPMessage* response;
   enum ASAPError      result;
   uint16_t            error;

   message = asapMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type          = AHT_NAME_RESOLUTION;
      message->PoolHandlePtr = poolHandle;

      result = asapIO(asap,message,&response,&error);

      if(result == ASAP_Okay) {
         result = (enum ASAPError)response->Error;
         if(result == ASAP_Okay) {
            if(response->PoolPtr != NULL) {
               asapCacheUpdatePool(asap->Cache, response->PoolPtr);
            }
            if(response->PoolElementPtr != NULL) {
               asapCacheUpdatePoolElement(asap->Cache,poolHandle,
                                          response->PoolElementPtr,
                                          false);
            }
         }
         asapMessageDelete(response);
      }

      asapMessageDelete(message);
   }
   else {
      result = ASAP_OutOfMemory;
   }

   return(result);
}


/* ###### Report pool element failure ####################################### */
enum ASAPError asapFailure(struct ASAPInstance*        asap,
                           struct PoolHandle*          poolHandle,
                           const PoolElementIdentifier identifier)
{
   struct ASAPMessage* message;
   struct PoolElement* found;
   enum ASAPError      result = ASAP_Okay;

   LOG_ACTION
   fprintf(stdlog, "Failure reported for pool element $%08x of pool ",
           (unsigned int)identifier);
   poolHandlePrint(poolHandle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   /* ====== Remove pool element from cache ================================= */
   dispatcherLock(asap->StateMachine);
   found = asapCacheFindPoolElement(asap->Cache, poolHandle, identifier);
   if(found != NULL) {
      found->Flags |= PEF_FAILED;
      asapCachePurgePoolElement(asap->Cache, poolHandle, found, false);
   }
   else {
      LOG_VERBOSE
      fputs("Pool element not found\n", stdlog);
      LOG_END
   }

   /* ====== Report unreachability ========================================== */
   message = asapMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type          = AHT_ENDPOINT_UNREACHABLE;
      message->PoolHandlePtr = poolHandleDuplicate(poolHandle);
      if(message->PoolHandlePtr) {
         message->Identifier              = identifier;
         message->PoolHandlePtrAutoDelete = true;

         result = asapSendRequest(asap, message);
      }
      else {
         result = ASAP_OutOfMemory;
      }
   }

   dispatcherUnlock(asap->StateMachine);
   return(ASAP_Okay);
}


/* ###### Do name lookup from cache ###################################### */
static enum ASAPError asapNameResolutionFromCache(struct ASAPInstance* asap,
                                                  struct PoolHandle*   poolHandle,
                                                  struct Pool**        poolPtr)
{
   struct Pool*   pool;
   enum ASAPError result = ASAP_NotFound;

   dispatcherLock(asap->StateMachine);
   pool = asapCacheFindPool(asap->Cache, poolHandle);

   LOG_ACTION
   fprintf(stdlog, "Name Resolution for handle ");
   poolHandlePrint(poolHandle, stdlog);
   fputs(":\n", stdlog);
   poolPrint(pool, stdlog);
   LOG_END

   if(pool != NULL) {
      if(poolPtr != NULL) {
         *poolPtr = poolDuplicate(pool);
      }
      result = ASAP_Okay;
   }
   else {
      if(poolPtr != NULL) {
         *poolPtr = NULL;
      }
   }

   dispatcherUnlock(asap->StateMachine);
   return(result);
}


/* ###### Do name lookup from name server ################################ */
static enum ASAPError asapNameResolutionFromServer(struct ASAPInstance* asap,
                                                   struct PoolHandle*   poolHandle,
                                                   struct Pool**        poolPtr)
{
   enum ASAPError result;

   dispatcherLock(asap->StateMachine);

   result = asapDoNameLookup(asap, poolHandle);
   if(result == ASAP_Okay) {
      result = asapNameResolutionFromCache(asap, poolHandle, poolPtr);
   }
   else {
      if(poolPtr != NULL) {
         *poolPtr = NULL;
      }
   }

   dispatcherUnlock(asap->StateMachine);

   return(result);
}


/* ###### Do name resolution for given pool handle ####################### */
enum ASAPError asapNameResolution(struct ASAPInstance* asap,
                                  struct PoolHandle*   poolHandle,
                                  struct Pool**        poolPtr)
{
   enum ASAPError result;

   LOG_VERBOSE
   fputs("Trying name resolution from cache...\n", stdlog);
   LOG_END

   result = asapNameResolutionFromCache(asap, poolHandle, poolPtr);
   if(result != ASAP_Okay) {
      if(poolPtr != NULL) {
         poolDelete(*poolPtr);
      }

      LOG_VERBOSE
      fputs("Trying name resolution from server...\n", stdlog);
      LOG_END
      result = asapNameResolutionFromServer(asap, poolHandle, poolPtr);

      if(result != ASAP_Okay) {
         LOG_VERBOSE
         fputs("Failed -> Getting old results from cache...\n", stdlog);
         LOG_END
         result = asapNameResolutionFromCache(asap, poolHandle, poolPtr);
      }
   }

   return(result);
}


/* ###### Select pool element by policy ################################## */
struct PoolElement* asapSelectPoolElement(struct ASAPInstance* asap,
                                          struct PoolHandle*   poolHandle,
                                          enum ASAPError*      error)
{
   struct Pool*        pool;
   struct PoolElement* poolElement;
   struct PoolElement* selected;
   enum ASAPError      result;
   card64              now;

   poolElement = NULL;
   dispatcherLock(asap->StateMachine);

   pool = asapCacheFindPool(asap->Cache,poolHandle);
   now  = getMicroTime();
   if(pool == NULL) {
      result = asapDoNameLookup(asap,poolHandle);
   }
   else {
      result = ASAP_Okay;
   }

   pool = asapCacheFindPool(asap->Cache,poolHandle);
   if(pool != NULL) {
      selected = poolSelectByPolicy(pool);
      if(selected != NULL) {
         poolElement = poolElementDuplicate(selected);
      }
      else if(result == ASAP_Okay) {
         result = ASAP_NotFound;
      }
   }
   else if(result == ASAP_Okay) {
      result = ASAP_NotFound;
   }

   dispatcherUnlock(asap->StateMachine);

   if(error) {
      *error = result;
   }
   return(poolElement);
}


/* ###### Handle endpoint keepalive ###################################### */
static void asapHandleEndpointKeepAlive(struct ASAPInstance* asap,
                                        struct ASAPMessage*  message)
{
   struct Pool*        pool;
   struct PoolElement* poolElement;
   GList*              list;

   LOG_VERBOSE2
   fprintf(stdlog, "Endpoint KeepAlive for pool handle ");
   poolHandlePrint(message->PoolHandlePtr, stdlog);
   fputs("\n", stdlog);
   LOG_END

   pool = asapCacheFindPool(asap->Cache, message->PoolHandlePtr);
   if(pool != NULL) {
      list = g_list_first(pool->PoolElementList);
      while(list != NULL) {
         poolElement = (struct PoolElement*)list->data;
         if(poolElement->UserData == (void*)asap) {
            message->Type       = AHT_ENDPOINT_KEEP_ALIVE_ACK;
            message->Identifier = poolElement->Identifier;

            LOG_VERBOSE2
            fprintf(stdlog, "Sending KeepAliveAck for pool element $%08x\n",message->Identifier);
            LOG_END
            asapSendRequest(asap,message);
         }
         list = g_list_next(list);
      }
   }
   else {
      LOG_WARNING
      fputs("Pool not found\n", stdlog);
      LOG_END
   }
}


/* ###### Handle incoming requests from name server (keepalives) ######### */
static void asapNameServerConnectionHandler(struct Dispatcher* dispatcher,
                                      int                fd,
                                      int                eventMask,
                                      void*              userData)
{
   struct ASAPInstance* asap = (struct ASAPInstance*)userData;
   struct ASAPMessage*  message;
   enum ASAPError       result;

   dispatcherLock(asap->StateMachine);

   LOG_VERBOSE2
   fputs("Entering Connection Handler...\n", stdlog);
   LOG_END

   if(eventMask & (FDCE_Read|FDCE_Exception)) {
      result = asapReceiveResponse(asap, &message);
      if(result == ASAP_Okay) {
         if(message->Type == AHT_ENDPOINT_KEEP_ALIVE) {
            asapHandleEndpointKeepAlive(asap, message);
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Received unexpected message type #%d\n",
                    message->Type);
            LOG_END
         }
         asapMessageDelete(message);
      }
      else {
         LOG_ACTION
         fputs("Disconnecting from name server server due to failure\n", stdlog);
         LOG_END
         asapDisconnectFromNameServer(asap);
      }
   }

   LOG_VERBOSE2
   fputs("Leaving Connection Handler\n", stdlog);
   LOG_END

   dispatcherUnlock(asap->StateMachine);
}
