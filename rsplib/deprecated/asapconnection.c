/*
 *  $Id: asapconnection.c,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: ASAP Connection
 *
 */


#include "tdtypes.h"
#include "asapconnection.h"
#include "asapcache.h"
#include "pool.h"
#include "poolelement.h"
#include "utilities.h"
#include "netutilities.h"

#include <ext_socket.h>



static void asapConnectionSocketHandler(struct Dispatcher* dispatcher,
                                        int                fd,
                                        int                eventMask,
                                        void*              userData);

static enum ASAPError asapConnectToPoolElement(struct ASAPInstance*   asap,
                                               struct PoolHandle*     poolHandle,
                                               struct PoolElement*    poolElement,
                                               const int              protocol,
                                               struct ASAPConnection* asapConnection);

static void asapDisconnectFromPoolElement(struct ASAPInstance*   asap,
                                          struct ASAPConnection* asapConnection,
                                          const bool             purge);

static enum ASAPError asapConnectToPool(struct ASAPInstance*   asap,
                                        struct PoolHandle*     poolHandle,
                                        const int              protocol,
                                        struct ASAPConnection* asapConnection);



/* ###### Socket handler for user transport connection ################### */
static void asapConnectionSocketHandler(struct Dispatcher* dispatcher,
                                        int                fd,
                                        int                eventMask,
                                        void*              userData)
{
   struct ASAPConnection* asapConnection = (struct ASAPConnection*)userData;

   if(asapConnection->Callback != NULL) {
      if((eventMask & asapConnection->EventMask) != 0) {
         asapConnection->Callback(asapConnection,eventMask,asapConnection->UserData);
      }
   }
}


/* ###### Connect to pool element ######################################## */
static enum ASAPError asapConnectToPoolElement(struct ASAPInstance*   asap,
                                               struct PoolHandle*     poolHandle,
                                               struct PoolElement*    poolElement,
                                               const int              protocol,
                                               struct ASAPConnection* asapConnection)
{
   GList*                   list;
   struct TransportAddress* transportAddress;
   struct timeval           timeout;
   fd_set                   fdset;
   int                      status;
   int                      domain;
   int                      type;
   int                      result;

   LOG_ACTION
   fprintf(stdlog,"Trying connection to pool element #%Ld...\n",(card64)poolElement->Handle);
   LOG_END

   result = ASAP_ConnectionFailureUnusable;
   list = g_list_first(poolElement->TransportAddressList);
   while(list != NULL) {
      transportAddress = (struct TransportAddress*)list->data;

      LOG_ACTION
      fprintf(stdlog,"Trying address ");
      transportAddressPrint(transportAddress,stdlog);
      fputs("...\n",stdlog);
      LOG_END

      if((protocol == 0) || (transportAddress->Protocol == protocol)) {
         if(transportAddress->Addresses > 0) {
            domain = transportAddress->AddressArray[0].sa.sa_family;
            if((transportAddress->Protocol == IPPROTO_TCP) ||
               (transportAddress->Protocol == IPPROTO_SCTP)) {
               type = SOCK_STREAM;
            }
            else {
               type = SOCK_DGRAM;
            }
            asapConnection->CurrentSocket = ext_socket(
               domain, type,
               transportAddress->Protocol);
            if(asapConnection->CurrentSocket >= 0) {
               setNonBlocking(asapConnection->CurrentSocket);
               status = ext_connect(asapConnection->CurrentSocket,
                                    (struct sockaddr*)&transportAddress->AddressArray[0],
                                    getSocklen((struct sockaddr*)&transportAddress->AddressArray[0]));
               if(errno == EINPROGRESS) {
                  LOG_VERBOSE2
                  fprintf(stdlog,"Waiting for connection establishment, timeout=%Ld [µs]...\n",
                          asap->UserConnectionEstablishmentTimeout);
                  LOG_END
                  FD_ZERO(&fdset);
                  FD_SET(asapConnection->CurrentSocket,&fdset);
                  timeout.tv_sec  = asap->UserConnectionEstablishmentTimeout / (card64)1000000;
                  timeout.tv_usec = asap->UserConnectionEstablishmentTimeout % (card64)1000000;
                  result = ext_select(1 + asapConnection->CurrentSocket,
                                      &fdset, &fdset, &fdset,
                                      &timeout);
               }
               else if(errno == 0) {
                  result = 1;
               }
               else {
                  result = 0;
               }

               if(result > 0) {
                  setBlocking(asapConnection->CurrentSocket);
                  asapConnection->CurrentProtocol = protocol;
                  asapConnection->ASAP            = asap;
                  dispatcherAddFDCallback(asap->StateMachine,
                                          asapConnection->CurrentSocket,
                                          asapConnection->EventMask,
                                          asapConnectionSocketHandler,
                                          (void*)asapConnection);

                  /* If the element is not already in the cache.
                     Further, increment the cache entry's its user count. */
                  asapConnection->CurrentPoolElement = asapCacheUpdatePoolElement(asap,poolHandle,poolElement,true);
                  result = ASAP_Okay;

                  LOG_ACTION
                  fputs("Connection successfully established.\n",stdlog);
                  LOG_END

                  break;
               }
               else {
                  result = ASAP_ConnectionFailureConnect;
               }

               LOG_ERROR
               logerror("Connection failed");
               LOG_END

               asapPoolElementFailure(asap,poolElement->Handle);
               ext_close(asapConnection->CurrentSocket);
               asapConnection->CurrentSocket = -1;
            }
            else {
               asapPoolElementFailure(asap,poolElement->Handle);
               result = ASAP_ConnectionFailureSocket;

               LOG_ACTION
               logerror("Socket creation failed");
               LOG_END
            }
         }
      }
      else {
         LOG_ACTION
         fputs("Skipping address, wrong protocol\n",stdlog);
         LOG_END
      }

      list = g_list_next(list);
   }

   return((enum ASAPError)result);
}


/* ###### Disconnect from pool element ################################### */
static void asapDisconnectFromPoolElement(struct ASAPInstance*   asap,
                                          struct ASAPConnection* asapConnection,
                                          const bool             purge)
{
   if(asapConnection->CurrentPoolElement != NULL) {
      LOG_ACTION
      fputs("Disconnect\n",stdlog);
      LOG_END

      /* Decrement user count of pool element's cache entry */
      if(purge) {
         if(asapConnection->CurrentPoolElement != NULL) {
            asapCachePurgePoolElement(asap,asapConnection->CurrentPoolElement,true);
         }
      }
      else {
         asapConnection->CurrentPoolElement->UserCounter--;
      }
      asapConnection->CurrentPoolElement = NULL;

      dispatcherRemoveFDCallback(asap->StateMachine,
                                 asapConnection->CurrentSocket);
      ext_close(asapConnection->CurrentSocket);
      asapConnection->CurrentSocket = -1;
   }
}


/* ###### Select pool element from pool and connect to pool element ###### */
static enum ASAPError asapConnectToPool(struct ASAPInstance*   asap,
                                        struct PoolHandle*     poolHandle,
                                        const int              protocol,
                                        struct ASAPConnection* asapConnection)
{
   struct PoolElement* poolElement;
   enum ASAPError      result;
   cardinal            i;

   result = asapNameResolution(asap, poolHandle, NULL);
   if(result == ASAP_Okay) {
      for(i = 0;i < asap->UserConnectionMaxTrials;i++) {
         poolElement = asapSelectPoolElement(asap,poolHandle,&result);
         if(poolElement == NULL) {
            result = asapNameResolution(asap, poolHandle, NULL);
            if(result != ASAP_Okay) {
               break;
            }
            poolElement = asapSelectPoolElement(asap,poolHandle,&result);
            if(poolElement == NULL) {
               return(ASAP_NotFound);
            }
         }

         result = asapConnectToPoolElement(asap,poolHandle,poolElement,protocol,asapConnection);
         if(result == ASAP_Okay) {
            break;
         }
      }
   }

   return(result);
}


/* ###### Do name lookup, select pool element from pool and connect to ### */
enum ASAPError asapConnect(struct ASAPInstance*    asap,
                           struct PoolHandle*      poolHandle,
                           const int               protocol,
                           struct ASAPConnection** asapConnection,
                           int                     eventMask,
                           void                    (*callback)(struct ASAPConnection* asapConnection,
                                                               int                    eventMask,
                                                               void*                  userData),
                           void*                   userData)
{
   enum ASAPError result = ASAP_Okay;

   *asapConnection = (struct ASAPConnection*)malloc(sizeof(struct ASAPConnection));
   if(*asapConnection != NULL) {
      (*asapConnection)->Handle = poolHandleDuplicate(poolHandle);
      if((*asapConnection)->Handle != NULL) {
         (*asapConnection)->EventMask = eventMask;
         (*asapConnection)->Callback  = callback;
         (*asapConnection)->UserData  = userData;

         (*asapConnection)->CurrentPoolElement = NULL;

         result = asapConnectToPool(asap,poolHandle,protocol,*asapConnection);
         if(result == ASAP_Okay) {
            return(ASAP_Okay);
         }

         poolHandleDelete((*asapConnection)->Handle);
      }
      free(*asapConnection);
      *asapConnection = NULL;
   }
   return(result);
}


/* ###### Disconnect from pool element ################################### */
void asapDisconnect(struct ASAPInstance*   asap,
                    struct ASAPConnection* asapConnection)
{
   if(asapConnection != NULL) {
      asapDisconnectFromPoolElement(asap,asapConnection,false);
      free(asapConnection);
   }
}


/* ###### Report pool element failure #################################### */
void asapFailure(struct ASAPInstance*   asap,
                 struct ASAPConnection* asapConnection)
{
   if(asapConnection != NULL) {
      if(asapConnection->CurrentPoolElement) {
         asapConnection->CurrentPoolElement->Flags |= PEF_FAILED;
      }
      asapDisconnectFromPoolElement(asap,asapConnection,false);
   }
}


/* ###### Do failover #################################################### */
enum ASAPError asapConnectionFailover(struct ASAPConnection* asapConnection)
{
   struct ASAPInstance* asap;
   enum ASAPError       result;

   if(asapConnection != NULL) {
      asap = asapConnection->ASAP;

      LOG_ACTION
      fputs("Failover!\n",stdlog);
      LOG_END

      asapDisconnectFromPoolElement(asap,asapConnection,true);

      result = asapConnectToPool(asap,
                                 asapConnection->Handle,
                                 asapConnection->CurrentProtocol,
                                 asapConnection);
   }
   else {
      result = ASAP_InvalidValues;
   }

   return(result);
}


/* ###### Write user-specific data to pool element ####################### */
ssize_t asapConnectionWrite(struct ASAPConnection* asapConnection,
                            struct msghdr*         message)
{
   ssize_t result = -1;
   void*   oldName;
   size_t  oldNameLen;

   if(asapConnection != NULL) {
      oldName              = message->msg_name;
      oldNameLen           = message->msg_namelen;
      message->msg_name    = NULL;
      message->msg_namelen = 0;

      result = ext_sendmsg(asapConnection->CurrentSocket,message,message->msg_flags);

      message->msg_name    = oldName;
      message->msg_namelen = oldNameLen;
      if((message->msg_name != NULL)                  &&
         (asapConnection->CurrentPoolElement != NULL) &&
         (message->msg_namelen >= sizeof(PoolElementHandle))) {
         *((PoolElementHandle*)message->msg_name) = asapConnection->CurrentPoolElement->Handle;
         message->msg_namelen = sizeof(PoolElementHandle);
      }
      else {
         message->msg_namelen = 0;
      }

      if((result < 0) && (errno != EWOULDBLOCK)) {
         LOG_ERROR
         logerror("Writing user data failed -> Failover");
         LOG_END

         asapConnectionFailover(asapConnection);
      }
   }
   return(result);
}


/* ###### Read user-specific data from pool element ###################### */
ssize_t asapConnectionRead(struct ASAPConnection* asapConnection,
                           struct msghdr*         message)
{
   ssize_t result = -1;
   void*   oldName;
   size_t  oldNameLen;

   if(asapConnection != NULL) {
      oldName              = message->msg_name;
      oldNameLen           = message->msg_namelen;
      message->msg_name    = NULL;
      message->msg_namelen = 0;

      LOG_VERBOSE2
      fprintf(stdlog,"Waiting for user data on socket %d...\n",asapConnection->CurrentSocket);
      LOG_END

      result = ext_recvmsg(asapConnection->CurrentSocket,message,message->msg_flags);

      LOG_VERBOSE2
      fprintf(stdlog,"recvmsg() result=%d, %s\n",result,strerror(errno));
      LOG_END

      message->msg_name    = oldName;
      message->msg_namelen = oldNameLen;
      if((message->msg_name != NULL)                  &&
         (asapConnection->CurrentPoolElement != NULL) &&
         (message->msg_namelen >= sizeof(PoolElementHandle))) {
         *((PoolElementHandle*)message->msg_name) = asapConnection->CurrentPoolElement->Handle;
         message->msg_namelen = sizeof(PoolElementHandle);
      }
      else {
         message->msg_namelen = 0;
      }

      if((result <= 0) && (errno != EWOULDBLOCK)) {
         LOG_ERROR
         logerror("Reading user data failed -> Failover");
         LOG_END

         asapConnectionFailover(asapConnection);
      }
   }
   return(result);
}
