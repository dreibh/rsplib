/*
 *  $Id: nameserver.c,v 1.2 2004/07/18 15:30:43 dreibh Exp $
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
 * Purpose: Name Server
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "dispatcher.h"
#include "timer.h"
#include "messagebuffer.h"
#include "rsplib-tags.h"

#include "asapmessage.h"
#include "poolnamespacemanagement.h"
#include "randomizer.h"
#include "breakdetector.h"

#include <ext_socket.h>
#include <net/if.h>
#include <sys/ioctl.h>



/* Exit immediately on Ctrl-C. No clean shutdown. */
#define FAST_BREAK

#define MAX_NS_TRANSPORTADDRESSES 16



struct NameServer
{
   ENRPIdentifierType                       ServerID;
   struct Dispatcher*                       StateMachine;
   struct ST_CLASS(PoolNamespaceManagement) Namespace;

   int                                      AnnounceSocket;
   union sockaddr_union                     AnnounceAddress;
   bool                                     SendAnnounces;

   int                                      NameServerSocket;
   struct TransportAddressBlock*            NameServerAddress;

   struct Timer*                            AnnounceTimer;
   struct Timer*                            NamespaceActionTimer;

   card64                                   AnnounceInterval;
   card64                                   KeepAliveInterval;
   card64                                   KeepAliveTimeout;
   card64                                   MessageReceptionTimeout;
};


#define NAMESERVER_DEFAULT_ANNOUNCE_INTERVAL          2000000
#define NAMESERVER_DEFAULT_KEEP_ALIVE_INTERVAL        5000000
#define NAMESERVER_DEFAULT_KEEP_ALIVE_TIMEOUT        12000000
#define NAMESERVER_DEFAULT_MESSAGE_RECEPTION_TIMEOUT  3000000


struct NameServer* nameServerNew(int                           nameServerSocket,
                                 struct TransportAddressBlock* nameServerAddress,
                                 const bool                    sendAnnounces,
                                 const union sockaddr_union*   announceAddress);
void nameServerDelete(struct NameServer* nameServer);

static void announceTimerCallback(struct Dispatcher* dispatcher,
                                  struct Timer*      timer,
                                  void*              userData);
static void namespaceActionTimerCallback(struct Dispatcher* dispatcher,
                                         struct Timer*      timer,
                                         void*              userData);
static void nameServerSocketCallback(struct Dispatcher* dispatcher,
                                     int                fd,
                                     int                eventMask,
                                     void*              userData);



/* ###### Constructor #################################################### */
struct NameServer* nameServerNew(int                           nameServerSocket,
                                 struct TransportAddressBlock* nameServerAddress,
                                 const bool                    sendAnnounces,
                                 const union sockaddr_union*   announceAddress)
{
   struct NameServer* nameServer = (struct NameServer*)malloc(sizeof(struct NameServer));
   if(nameServer != NULL) {
      nameServer->ServerID     = random32();
      nameServer->StateMachine = dispatcherNew(dispatcherDefaultLock,dispatcherDefaultUnlock,NULL);
      if(nameServer->StateMachine == NULL) {
         free(nameServer);
         return(NULL);
      }
      ST_CLASS(poolNamespaceManagementNew)(&nameServer->Namespace,
                                           nameServer->ServerID,
                                           NULL, NULL);

      nameServer->NameServerSocket        = nameServerSocket;
      nameServer->NameServerAddress       = nameServerAddress;

      nameServer->KeepAliveInterval       = NAMESERVER_DEFAULT_KEEP_ALIVE_INTERVAL;
      nameServer->KeepAliveTimeout        = NAMESERVER_DEFAULT_KEEP_ALIVE_TIMEOUT;
      nameServer->AnnounceInterval        = NAMESERVER_DEFAULT_ANNOUNCE_INTERVAL;
      nameServer->MessageReceptionTimeout = NAMESERVER_DEFAULT_MESSAGE_RECEPTION_TIMEOUT;

      nameServer->SendAnnounces           = sendAnnounces;
      memcpy(&nameServer->AnnounceAddress, announceAddress, sizeof(union sockaddr_union));
      if(nameServer->AnnounceAddress.in6.sin6_family == AF_INET6) {
         nameServer->AnnounceSocket = ext_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
puts("v6-ann.");
      }
      else if(nameServer->AnnounceAddress.in.sin_family == AF_INET) {
         nameServer->AnnounceSocket = ext_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      }
      else {
         nameServer->AnnounceSocket = -1;
      }
      if(nameServer->AnnounceSocket >= 0) {
         setNonBlocking(nameServer->AnnounceSocket);
      }

      nameServer->AnnounceTimer = timerNew(nameServer->StateMachine,
                                           announceTimerCallback,
                                           (void*)nameServer);
      nameServer->NamespaceActionTimer = timerNew(nameServer->StateMachine,
                                                  namespaceActionTimerCallback,
                                                  (void*)nameServer);
      if((nameServer->AnnounceTimer == NULL) ||
         (nameServer->NamespaceActionTimer == NULL)) {
         nameServerDelete(nameServer);
         return(NULL);
      }
      if(nameServer->SendAnnounces) {
puts("ANNOUNCE TIMER...");
         timerStart(nameServer->AnnounceTimer, 0);
      }

      dispatcherAddFDCallback(nameServer->StateMachine,
                              nameServer->NameServerSocket,
                              FDCE_Read|FDCE_Exception,
                              nameServerSocketCallback,
                              (void*)nameServer);
   }
   return(nameServer);
}


/* ###### Destructor ###################################################### */
void nameServerDelete(struct NameServer* nameServer)
{
   if(nameServer) {
      dispatcherRemoveFDCallback(nameServer->StateMachine, nameServer->NameServerSocket);
      if(nameServer->AnnounceTimer) {
         timerDelete(nameServer->AnnounceTimer);
         nameServer->AnnounceTimer = NULL;
      }
      if(nameServer->NamespaceActionTimer) {
         timerDelete(nameServer->NamespaceActionTimer);
         nameServer->NamespaceActionTimer = NULL;
      }
      if(nameServer->AnnounceSocket >= 0) {
         ext_close(nameServer->AnnounceSocket >= 0);
         nameServer->AnnounceSocket = -1;
      }
      ST_CLASS(poolNamespaceManagementClear)(&nameServer->Namespace);
      ST_CLASS(poolNamespaceManagementDelete)(&nameServer->Namespace);
      if(nameServer->StateMachine) {
         dispatcherDelete(nameServer->StateMachine);
         nameServer->StateMachine = NULL;
      }
      free(nameServer);
   }
}


/* ###### Dump namespace ################################################# */
void nameServerDump(struct NameServer* nameServer)
{
   fputs("*************** Namespace Dump ***************\n",stdlog);
   printTimeStamp(stdlog);
   fputs("\n",stdlog);
   ST_CLASS(poolNamespaceManagementPrint)(&nameServer->Namespace, stdlog, PENPO_FULL);
   fputs("**********************************************\n",stdlog);
}


/* ###### Send announcement messages ###################################### */
static void announceTimerCallback(struct Dispatcher* dispatcher,
                                  struct Timer*      timer,
                                  void*              userData)
{
   struct NameServer*  nameServer = (struct NameServer*)userData;
   GList*              transportAddressBlockList;
   struct ASAPMessage* message;
   size_t              messageLength;

   CHECK(nameServer->SendAnnounces == true);
   CHECK(nameServer->AnnounceSocket >= 0);

   transportAddressBlockList = g_list_append(NULL, nameServer->NameServerAddress);
   if(transportAddressBlockList) {
      message = asapMessageNew(NULL, 65536);
      if(message) {
         message->Type                         = AHT_SERVER_ANNOUNCE;
         message->TransportAddressBlockListPtr = transportAddressBlockList;
         messageLength = asapMessage2Packet(message);
         if(messageLength > 0) {
            if(nameServer->AnnounceSocket) {
               LOG_VERBOSE2
               fputs("Sending announce to address ",stdlog);
               fputaddress((struct sockaddr*)&nameServer->AnnounceAddress, true, stdlog);
               fputs("\n",stdlog);
               LOG_END
               if(ext_sendto(nameServer->AnnounceSocket,
                             message->Buffer,
                             messageLength,
                             0,
                             (struct sockaddr*)&nameServer->AnnounceAddress,
                             getSocklen((struct sockaddr*)&nameServer->AnnounceAddress)) < (ssize_t)messageLength) {
                  LOG_WARNING
                  logerror("Unable to send announce");
                  LOG_END
               }
            }
         }
         asapMessageDelete(message);
      }
   }
   timerStart(timer, nameServer->AnnounceInterval);
}


/* ###### Handle namespace management timers ############################# */
static void namespaceActionTimerCallback(struct Dispatcher* dispatcher,
                                         struct Timer*      timer,
                                         void*              userData)
{
   struct NameServer*  nameServer = (struct NameServer*)userData;
puts("action CB...");
}



#if 0
/* ###### Filter address array ########################################### */
static bool filterAddresses(struct PoolElement*            poolElement,
                            const struct sockaddr_storage* addressArray,
                            const size_t                   addresses)
{
   struct TransportAddress* transportAddress;
   GList*                   list;
   bool                     found;
   size_t                   i, j;
   bool                     valid;

   valid = false;
   list = poolElement->TransportAddressList;
   while(list != NULL) {
      transportAddress = (struct TransportAddress*)list->data;
      for(i = 0;i < transportAddress->Addresses;i++) {
         found = false;
         for(j = 0;j < addresses;j++) {
            if(addresscmp((const struct sockaddr*)&transportAddress->AddressArray[i],
                          (const struct sockaddr*)&addressArray[j],
                          false) == 0) {
               found = true;
               break;
            }
         }

         if(!found) {
            LOG_VERBOSE3
            fputs("Rejected address ",stdlog);
            fputaddress((struct sockaddr*)&transportAddress->AddressArray[i],
                        false, stdlog);
            fputs("\n",stdlog);
            LOG_END

            if(i + 1 < transportAddress->Addresses) {
               for(j = i;j < transportAddress->Addresses - 1;j++) {
                  transportAddress->AddressArray[j] = transportAddress->AddressArray[j + 1];
               }
               i--;
            }
            transportAddress->Addresses--;
         }
         else {
            LOG_VERBOSE2
            fputs("Accepted address ",stdlog);
            fputaddress((struct sockaddr*)&transportAddress->AddressArray[i],
                        false, stdlog);
            fputs("\n",stdlog);
            LOG_END
         }
      }

      list = g_list_next(list);
      if(transportAddress->Addresses < 1) {
         LOG_VERBOSE3
         fputs("Removed empty TransportAddress structure\n",stdlog);
         LOG_END
         poolElementRemoveTransportAddress(poolElement,transportAddress);
         transportAddressDelete(transportAddress);
      }
      else {
         valid = true;
      }
   }

   return(valid);
}


/* ###### Handle registration request #################################### */
static bool registrationRequest(struct NameServer*     nameServer,
                                struct NameServerUser* user,
                                int                    fd,
                                struct ASAPMessage*    message)
{
   struct PoolElement*      existing;
   struct Pool*             pool;
   bool                     poolAdded;
   struct sockaddr_storage* addressArray;
   int                      addresses;
   int                      i;
   bool                     addressesValid;

   LOG_ACTION
   fprintf(stdlog,"Registration Request: ");
   poolElementPrint(message->PoolElementPtr,stdlog);
   LOG_END


   message->Type       = AHT_REGISTRATION_RESPONSE;
   message->Error      = AEC_OKAY;
   message->Identifier = message->PoolElementPtr->Identifier;


   /* ====== Filter addresses ========================================= */
   addresses = getpaddrsplus(fd, 0, &addressArray);
   if(addresses > 0) {
      LOG_VERBOSE
      fputs("SCTP association local addresses:\n",stdlog);
      for(i = 0;i < addresses;i++) {
         fprintf(stdlog,"%d/%d: ",i + 1,addresses);
         fputaddress((struct sockaddr*)&addressArray[i],false,stdlog);
         fputs("\n",stdlog);
      }
      LOG_END

      addressesValid = filterAddresses(message->PoolElementPtr,addressArray,addresses);

      LOG_VERBOSE
      fputs("Filtered Registration Request: ",stdlog);
      poolElementPrint(message->PoolElementPtr,stdlog);
      LOG_END

      free(addressArray);
   }
   else {
      LOG_WARNING
      fputs("Cannot obtain peer addresses. Assuming addresses to be okay\n", stdlog);
      LOG_END
      addressesValid = true;
   }

   if(addressesValid == false) {
      LOG_ERROR
      fputs("No valid addresses for registration\n",stdlog);
      LOG_END
      message->Error = AEC_AUTHORIZATION_FAILURE;
      return(true);
   }


   /* ====== Check, if pool element already exists ======================= */
   existing = poolNamespaceFindPoolElement(
                  nameServer->Namespace,
                  message->PoolHandlePtr,
                  message->PoolElementPtr->Identifier);
   if(existing == NULL) {
      /* ====== Check, if pool already exists ============================ */
      pool = poolNamespaceFindPool(nameServer->Namespace,message->PoolHandlePtr);
      if(pool != NULL) {
         poolAdded = false;

         /* Try to adapt requested policy to pool's policy. */
         if(poolPolicyAdapt(message->PoolElementPtr->Policy,pool->Policy) == false) {
            LOG_ACTION
            fputs("Pool policy inconsistent -> Registration rejected.\n",stdlog);
            LOG_END
            message->Error = AEC_POLICY_INCONSISTENT;
            pool = NULL;
         }
      }

      /* ====== Pool does not exist -> add it. =========================== */
      else {
         poolAdded = true;

         /* Create pool. */
         pool = poolNew(message->PoolHandlePtr,message->PoolElementPtr->Policy);
         if(pool != NULL) {
            poolNamespaceAddPool(nameServer->Namespace,pool);
         }
         else {
            message->Error = AEC_OUT_OF_MEMORY;
            pool           = NULL;
         }
      }


      /* ====== Add pool element to pool ================================= */
      if(pool != NULL) {
         /* Take pool element from ASAPMessage structure. */
         message->PoolElementPtrAutoDelete = false;

         message->PoolElementPtr->HomeENRPServerID = nameServer->ServerID;
         message->PoolElementPtr->UserData         = (void*)user;
         poolAddPoolElement(pool,message->PoolElementPtr);
         user->PoolElementList = g_list_append(user->PoolElementList,message->PoolElementPtr);

         user->Registrations++;
         if(user->Registrations == 1) {
            LOG_ACTION
            fputs("Starting KeepAlive timers\n",stdlog);
            LOG_END
            timerStart(user->KeepAliveTimer,nameServer->KeepAliveInterval);
            timerStart(user->UserTimeoutTimer,nameServer->KeepAliveTimeout);
         }


         LOG_ACTION
         fputs("Registration successful!\n",stdlog);
         LOG_END
      }
   }


   /* ====== Pool element already exists -> try re-registration ========== */
   else {
      pool = existing->OwnerPool;

      /* Existing pool element has been created by another user.
         Probably non-unique PE-ID. */
      if(existing->UserData != user) {
         message->Error = AEC_NONUNIQUE_PE_ID;

         LOG_ACTION
         fputs("Non-unique pool element identifier -> Registration rejected.\n",stdlog);
         LOG_END
      }

      /* Re-registration */
      else {
         /* Try to adapt requested policy to pool's policy. */
         if(poolPolicyAdapt(message->PoolElementPtr->Policy,pool->Policy) == false) {
            message->Error = AEC_POLICY_INCONSISTENT;
            LOG_ACTION
            fputs("Pool policy for re-registration inconsistent -> Registration rejected.\n",stdlog);
            LOG_END
         }

         /* Update pool element's settings. */
         else {
            message->Error = poolElementAdapt(existing,message->PoolElementPtr);
            LOG_ACTION
            fputs("Re-registration successful!\n",stdlog);
            nameServerDump(nameServer);
            LOG_END
         }
      }
   }

   return(true);
}


/* ###### Handle deregistration request ################################## */
static bool deregistrationRequest(struct NameServer*      nameServer,
                                  struct NameServerUser*  user,
                                  int                     fd,
                                  struct ASAPMessage*     message)
{
   struct PoolElement* poolElement;

   LOG_ACTION
   fprintf(stdlog,"Deregistration Request for pool element $%08x of pool ",message->Identifier);
   poolHandlePrint(message->PoolHandlePtr, stdlog);
   fputs("\n",stdlog);
   LOG_END

   message->Type  = AHT_DEREGISTRATION_RESPONSE;
   message->Error = AEC_OKAY;


   /* ====== Check, if pool element still exists ========================= */
   poolElement = poolNamespaceFindPoolElement(
                    nameServer->Namespace, message->PoolHandlePtr, message->Identifier);
   if(poolElement != NULL) {

      /* ====== Pool element may only be removed by its creator ========== */
      if(poolElement->UserData == user) {
         message->PoolElementPtr = poolElementDuplicate(poolElement);
         removePoolElement(nameServer,user,poolElement);

         user->Registrations--;
         if(user->Registrations == 0) {
            LOG_ACTION
            fputs("Stopping KeepAlive timers\n",stdlog);
            LOG_END
            timerStop(user->KeepAliveTimer);
            timerStop(user->UserTimeoutTimer);
         }

         LOG_ACTION
         fputs("Deregistration successful!\n",stdlog);
         nameServerDump(nameServer);
         LOG_END
      }

      else {
         message->Error = AEC_AUTHORIZATION_FAILURE;
         LOG_ACTION
         fputs("Deregistration not by creator -> Deregistration rejected!\n",stdlog);
         nameServerDump(nameServer);
         LOG_END
      }
   }

   /* ====== Pool element does not exist -> okay. ======================== */
   else {
      LOG_ACTION
      fputs("Pool element to deregister is not in the namespace!\n",stdlog);
      LOG_END
   }

   return(true);
}


/* ###### Handle name resolution request ################################# */
static bool nameResolutionRequest(struct NameServer*  nameServer,
                                  struct NameServerUser*  user,
                                  int                 fd,
                                  struct ASAPMessage* message)
{
   struct Pool* pool;

   LOG_ACTION
   fprintf(stdlog,"Name Resolution Request: ");
   poolHandlePrint(message->PoolHandlePtr,stdlog);
   fputs("\n",stdlog);
   LOG_END

   pool = poolNamespaceFindPool(nameServer->Namespace,message->PoolHandlePtr);
   if(pool != NULL) {
      message->Type              = AHT_NAME_RESOLUTION_RESPONSE;
      message->Error             = 0;
      message->PoolPtrAutoDelete = false;
      message->PoolPtr           = pool;

      LOG_ACTION
      fprintf(stdlog,"Result: ");
      poolPrint(message->PoolPtr,stdlog);
      LOG_END
   }
   else {
      LOG_ACTION
      fputs("Pool is not in the namespace!\n",stdlog);
      LOG_END
      message->Type  = AHT_NAME_RESOLUTION_RESPONSE;
      message->Error = AEC_NOT_FOUND;
   }

   return(true);
}


/* ###### Handle endpoint keepalive acknowledgement ###################### */
static void endpointKeepAliveAck(struct NameServer*     nameServer,
                                 struct NameServerUser* user,
                                 int                    fd,
                                 struct ASAPMessage*    message)
{
   struct PoolElement* poolElement;

   LOG_VERBOSE
   fprintf(stdlog,"Endpoint KeepAliveAck for $%08x in ",message->Identifier);
   poolHandlePrint(message->PoolHandlePtr,stdlog);
   fputs("\n",stdlog);
   LOG_END

   poolElement = poolNamespaceFindPoolElement(nameServer->Namespace,
                                              message->PoolHandlePtr,
                                              message->Identifier);
   if(poolElement != NULL) {
      poolElement->TimeStamp            = getMicroTime();
      poolElement->OwnerPool->TimeStamp = poolElement->TimeStamp;

      timerRestart(user->UserTimeoutTimer,nameServer->KeepAliveTimeout);
   }
   else {
      LOG_VERBOSE
      fputs("Pool element not found!\n",stdlog);
      LOG_END
   }
}


/* ###### Send endpoint keepalive messages ############################### */
static void keepAliveTimerCallback(struct Dispatcher* dispatcher,
                                   struct Timer*      timer,
                                   void*              userData)
{
   struct NameServerUser* user       = (struct NameServerUser*)userData;
   struct NameServer*     nameServer = user->Owner;
   GList*                 list;
   struct PoolElement*    poolElement;
   struct ASAPMessage*    message;

   LOG_VERBOSE
   fprintf(stdlog,"KeepAlive-Timer for socket #%d.\n",user->FD);
   LOG_END

   message = asapMessageNew(NULL,1024);
   if(message != NULL) {
      list = g_list_first(user->PoolElementList);
      while(list != NULL) {
         poolElement = (struct PoolElement*)list->data;

         LOG_VERBOSE2
         fprintf(stdlog,"KeepAlive for pool element $%08x in pool ",poolElement->Identifier);
         poolHandlePrint(poolElement->OwnerPool->Handle,stdlog);
         fputs("\n",stdlog);
         LOG_END

         message->PoolElementPtr = poolElement;
         message->PoolHandlePtr  = poolElement->OwnerPool->Handle;
         message->Type           = AHT_ENDPOINT_KEEP_ALIVE;
         if(asapMessageSend(user->FD,0,message) == false) {
            LOG_ERROR
            fputs("Sending KeepAlive failed!\n",stdlog);
            LOG_END
            return;
         }

         asapMessageClearAll(message);

         list = g_list_next(list);
      }
      asapMessageDelete(message);
   }

   timerStart(timer,nameServer->KeepAliveInterval);
}


/* ###### Handle user timeout ########################################## */
static void userTimeoutTimerCallback(struct Dispatcher* dispatcher,
                                     struct Timer*      timer,
                                     void*              userData)
{
   struct NameServerUser* user       = (struct NameServerUser*)userData;
   struct NameServer*     nameServer = user->Owner;

   LOG_ACTION
   fprintf(stdlog,"Timeout-Timer for socket #%d -> removing user\n",user->FD);
   LOG_END

   removeUser(nameServer,user);
}

#endif



/* ###### Handle incoming message ######################################## */
static void handleMessage(struct NameServer* nameServer,
                          char*              buffer,
                          const size_t       receivedSize,
                          const size_t       bufferSize,
                          int                sockFD,
                          const sctp_assoc_t assocID,
                          const uint32_t     ppid)
{
   struct ASAPMessage* message;
   bool                reply;  // ????????????????????

   CHECK(receivedSize <= bufferSize);
   message = asapPacket2Message(buffer, receivedSize, bufferSize);
   if(message != NULL) {
      reply = false;
      switch(message->Type) {
         case AHT_ENDPOINT_KEEP_ALIVE:
            puts("KeepAlive request received!");
          break;
/*
         case AHT_ENDPOINT_KEEP_ALIVE_ACK:
            endpointKeepAliveAck(nameServer,user,fd,message);
          break;
         case AHT_NAME_RESOLUTION:
            reply = nameResolutionRequest(nameServer,user,fd,message);
          break;
         case AHT_NAME_RESOLUTION_RESPONSE:
            break;
         case AHT_REGISTRATION:
            reply = registrationRequest(nameServer,user,fd,message);
          break;
         case AHT_DEREGISTRATION:
            reply = deregistrationRequest(nameServer,user,fd,message);
          break;
*/
         default:
            LOG_WARNING
            fprintf(stdlog,"Unsupported message type $%02x!\n",
                     message->Type);
            LOG_END
          break;
      }
/* ??????????????
      if(reply) {
         if(asapMessageSend(fd, 0, message) == false) {
            LOG_WARNING
            logerror("Sending reply failed. Closing connection");
            LOG_END
            removeUser(nameServer,user);
         }
      }
*/
      asapMessageDelete(message);
   }
}


/* ###### Handle events on sockets ####################################### */
static void nameServerSocketCallback(struct Dispatcher* dispatcher,
                                     int                fd,
                                     int                eventMask,
                                     void*              userData)
{
   struct NameServer*      nameServer = (struct NameServer*)userData;
   struct sockaddr_storage remoteAddress;
   socklen_t               remoteAddressLength;
   sctp_assoc_t            assocID;
   uint16_t                streamID;
   uint32_t                ppid;
   char                    buffer[65536];
   ssize_t                 received;

   if(fd == nameServer->NameServerSocket) {
      LOG_ACTION
      fputs("Event on name server socket...\n", stdlog);
      LOG_END

      received = recvfromplus(nameServer->NameServerSocket,
                              &buffer,
                              sizeof(buffer),
                              0,
                              (struct sockaddr*)&remoteAddress,
                              &remoteAddressLength,
                              &ppid,
                              &assocID,
                              &streamID,
                              0);
      if(received > 0) {
         LOG_VERBOSE
         fprintf(stdlog, "Got %u bytes from ", received);
         fputaddress((struct sockaddr*)&remoteAddress, true, stdlog);
         fprintf(stdlog, ", assoc #%u, PPID $%x\n", assocID, ppid);
         LOG_END

         handleMessage(nameServer,
                       (char*)&buffer, received, sizeof(buffer),
                       nameServer->NameServerSocket, assocID, ppid);
      }
      else {
         LOG_WARNING
         logerror("Unable to read from name server socket");
         LOG_END
      }

   }
   else {
      CHECK(false);
   }
}



/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct NameServer*       nameServer;
/*
   struct TransportAddress* transportAddress;
   bool                     autoSCTP         = false;
   size_t                   sctpAddresses;
   struct sockaddr_storage  sctpAddressArray[MAX_NS_TRANSPORTADDRESSES];
   fd_set                   testfdset;
   card64                   testTS;
   int                      i;
   int                      n;
   char*                    address=NULL;
   char*                    idx;
   unsigned int             interfaces;
*/
   int                           sockFD;
   int                           sctpSocket = -1;
   size_t                        sctpAddresses;
   struct sockaddr_storage       sctpAddressArray[MAX_NS_TRANSPORTADDRESSES];
   char                          sctpAddressBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* sctpAddress = (struct TransportAddressBlock*)&sctpAddressBuffer;
   struct sockaddr_storage       announceAddress;
   bool                          hasAnnounceAddress = false;

   struct sockaddr_storage* localAddressArray;
   char*                    address;
   char*                    idx;

   int                      result;
   struct timeval           timeout;
   fd_set                   readfdset;
   fd_set                   writefdset;
   fd_set                   exceptfdset;
   fd_set                   testfdset;
   card64                   testTS;
   int                      i, n;


   for(i = 1;i < argc;i++) {
      if(!(strncasecmp(argv[i],"-tcp=",5))) {
         fputs("ERROR: TCP mapping is not supported -> Use SCTP!", stderr);
         exit(1);
      }
      else if(!(strncasecmp(argv[i],"-sctp=",6))) {
         sctpAddresses = 0;
         if(!(strncasecmp((const char*)&argv[i][6], "auto", 4))) {
            sockFD = ext_socket(checkIPv6() ? AF_INET6 : AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
            if(sockFD >= 0) {
               if(bindplus(sockFD, NULL, 0) == false) {
                  puts("ERROR: Unable to bind SCTP socket!");
                  exit(1);
               }
               sctpAddresses = getladdrsplus(sockFD, 0, (struct sockaddr_storage**)&localAddressArray);
               if(sctpAddresses > MAX_NS_TRANSPORTADDRESSES) {
                  puts("ERROR: Too many local addresses -> specify only a subset!");
                  exit(1);
               }
               memcpy(&sctpAddressArray, localAddressArray, sctpAddresses * sizeof(struct sockaddr_storage));
               free(localAddressArray);
               ext_close(sockFD);
            }
            else {
               puts("ERROR: SCTP is unavailable. Install SCTP!");
               exit(1);
            }
         }
         else {
            address = (char*)&argv[i][6];
            while(sctpAddresses < MAX_NS_TRANSPORTADDRESSES) {
               idx = index(address, ',');
               if(idx) {
                  *idx = 0x00;
               }
               if(!string2address(address,&sctpAddressArray[sctpAddresses])) {
                  printf("ERROR: Bad local address %s! Use format <address:port>.\n",address);
                  exit(1);
               }
               sctpAddresses++;
               if(idx) {
                  address = idx;
                  address++;
               }
               else {
                  break;
               }
            }
         }
         if(sctpAddresses < 1) {
            puts("ERROR: At least one SCTP address must be specified!\n");
            exit(1);
         }
         transportAddressBlockNew(sctpAddress,
                                  IPPROTO_SCTP,
                                  getPort((struct sockaddr*)&sctpAddressArray[0]),
                                  0,
                                  (struct sockaddr_storage*)&sctpAddressArray,
                                  sctpAddresses);
         if(sctpAddress == NULL) {
            puts("ERROR: Unable to handle socket address(es)!");
            exit(1);
         }

         sctpSocket = ext_socket(AF_INET,SOCK_STREAM,IPPROTO_SCTP);
         if(sctpSocket >= 0) {
            if(bindplus(sctpSocket,
                        (struct sockaddr_storage*)&sctpAddressArray,
                        sctpAddresses) == false) {
               perror("bindx() call failed");
               exit(1);
            }
            if(ext_listen(sctpSocket, 10) < 0) {
               perror("listen() call failed");
               exit(1);
            }
         }
         else {
            puts("ERROR: Unable to create SCTP socket!");
         }
      }
      else if(!(strncasecmp(argv[i],"-announce=", 10))) {
         address = (char*)&argv[i][10];
         if(!string2address(address, &announceAddress)) {
            printf("ERROR: Bad announce address %s! Use format <address:port>.\n",address);
            exit(1);
         }
         hasAnnounceAddress = true;
      }
      else if(!(strncmp(argv[i],"-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else {
         printf("Usage: %s {-sctp=auto|address:port{,address}...} {[-announce=address:port}]} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}\n",argv[0]);
         exit(1);
      }
   }
   if(sctpSocket <= 0) {
      fprintf(stderr, "ERROR: No SCTP socket opened. Use parameter \"-sctp=...\"!\n");
      exit(1);
   }


   /* ====== Initialize NameServer ======================================= */
   nameServer = nameServerNew(sctpSocket, sctpAddress,
                              hasAnnounceAddress, (union sockaddr_union*)&announceAddress);
   if(nameServer == NULL) {
      fprintf(stderr, "ERROR: Unable to initialize NameServer object!\n");
      exit(1);
   }
#ifndef FAST_BREAK
   installBreakDetector();
#endif

   /* ====== Print information =========================================== */
   puts("The rsplib Name Server - Version 1.00");
   puts("=====================================\n");
   printf("Local Address:    ");
   transportAddressBlockPrint(sctpAddress, stdout);
   puts("");
   printf("Announce Address: ");
   if(hasAnnounceAddress) {
      fputaddress((struct sockaddr*)&announceAddress, true, stdout);
      puts("");
   }
   else {
      puts("(none)");
   }
   puts("\n");


   /* ====== Main loop =================================================== */
   beginLogging();
   while(!breakDetected()) {
      dispatcherGetSelectParameters(nameServer->StateMachine, &n, &readfdset, &writefdset, &exceptfdset, &testfdset, &testTS, &timeout);
      timeout.tv_sec  = min(0, timeout.tv_sec);
      timeout.tv_usec = min(250000, timeout.tv_usec);
      result = ext_select(n, &readfdset, &writefdset, &exceptfdset, &timeout);
      dispatcherHandleSelectResult(nameServer->StateMachine, result, &readfdset, &writefdset, &exceptfdset, &testfdset, testTS);
      if(errno == EINTR) {
         break;
      }
      if(result < 0) {
         perror("select() failed");
         break;
      }
   }


   // ====== Clean up ========================================================
   nameServerDelete(nameServer);
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
