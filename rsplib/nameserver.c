/*
 *  $Id: nameserver.c,v 1.5 2004/07/20 08:47:38 dreibh Exp $
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
   fputs("*************** Namespace Dump ***************\n", stdlog);
   printTimeStamp(stdlog);
   fputs("\n", stdlog);
   ST_CLASS(poolNamespaceManagementPrint)(&nameServer->Namespace, stdlog,
            PNNPO_POOLS_INDEX|PNNPO_POOLS_SELECTION|PNNPO_POOLS_TIMER|PENPO_USERTRANSPORT|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PENPO_HOME_NS);
   fputs("**********************************************\n", stdlog);
}


/* ###### Send announcement messages ###################################### */
static void announceTimerCallback(struct Dispatcher* dispatcher,
                                  struct Timer*      timer,
                                  void*              userData)
{
   struct NameServer*  nameServer = (struct NameServer*)userData;
   struct ASAPMessage* message;
   size_t              messageLength;

   CHECK(nameServer->SendAnnounces == true);
   CHECK(nameServer->AnnounceSocket >= 0);

   message = asapMessageNew(NULL, 65536);
   if(message) {
      message->Type                         = AHT_SERVER_ANNOUNCE;
      message->NSIdentifier                 = nameServer->ServerID;
      message->TransportAddressBlockListPtr = nameServer->NameServerAddress;
      messageLength = asapMessage2Packet(message);
      if(messageLength > 0) {
         if(nameServer->AnnounceSocket) {
            LOG_VERBOSE2
            fputs("Sending announce to address ", stdlog);
            fputaddress((struct sockaddr*)&nameServer->AnnounceAddress, true, stdlog);
            fputs("\n", stdlog);
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



/* ###### Filter address array ########################################### */
static size_t filterValidAddresses(
                 const struct TransportAddressBlock* sourceAddressBlock,
                 const struct sockaddr_storage*      assocAddressArray,
                 const size_t                        assocAddresses,
                 struct TransportAddressBlock*       validAddressBlock)
{
   bool   selectionArray[MAX_PE_TRANSPORTADDRESSES];
   size_t selected = 0;
   size_t i, j;

   for(i = 0;i < sourceAddressBlock->Addresses;i++) {
      selectionArray[i] = false;
      for(j = 0;j < assocAddresses;j++) {
         if(addresscmp((const struct sockaddr*)&sourceAddressBlock->AddressArray[i],
                       (const struct sockaddr*)&assocAddressArray[j],
                       false) == 0) {
            selectionArray[i] = true;
            selected++;
            break;
         }
      }
   }

   if(selected > 0) {
      validAddressBlock->Next      = NULL;
      validAddressBlock->Protocol  = sourceAddressBlock->Protocol;
      validAddressBlock->Flags     = 0;
      validAddressBlock->Addresses = selected;
      j = 0;
      for(i = 0;i < sourceAddressBlock->Addresses;i++) {
         if(selectionArray[i]) {
            memcpy(&validAddressBlock->AddressArray[j],
                   (const struct sockaddr*)&sourceAddressBlock->AddressArray[i],
                   sizeof(validAddressBlock->AddressArray[j]));
            j++;
         }
      }
   }

   return(selected);
}


/* ###### Handle registration request #################################### */
static void handleRegistrationRequest(struct NameServer*  nameServer,
                                      int                 fd,
                                      sctp_assoc_t        assocID,
                                      struct ASAPMessage* message)
{
   char                              validAddressBlockBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     validAddressBlock = (struct TransportAddressBlock*)&validAddressBlockBuffer;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct sockaddr_storage*          addressArray;
   int                               addresses;
   int                               i;

   LOG_ACTION
   fprintf(stdlog, "Registration request to pool ");
   poolHandlePrint(&message->Handle, stdlog);
   fputs(" of pool element ", stdlog);
   ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
   fputs("\n", stdlog);
   LOG_END


   message->Type       = AHT_REGISTRATION_RESPONSE;
   message->Error      = RSPERR_OKAY;
   message->Identifier = message->PoolElementPtr->Identifier;


   /* ====== Filter addresses ========================================= */
   addresses = getpaddrsplus(fd, assocID, &addressArray);
   if(addresses > 0) {
      LOG_VERBOSE1
      fputs("SCTP association's peer addresses:\n", stdlog);
      for(i = 0;i < addresses;i++) {
         fprintf(stdlog, "%d/%d: ",i + 1,addresses);
         fputaddress((struct sockaddr*)&addressArray[i],false, stdlog);
         fputs("\n", stdlog);
      }
      LOG_END

      if(filterValidAddresses(message->PoolElementPtr->AddressBlock,
                              addressArray, addresses,
                              validAddressBlock) > 0) {
         LOG_VERBOSE1
         fputs("Valid addresses: ", stdlog);
         transportAddressBlockPrint(validAddressBlock, stdlog);
         fputs("\n", stdlog);
         LOG_END

         message->Error = ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
                           &nameServer->Namespace,
                           &message->Handle,
                           nameServer->ServerID,
                           message->PoolElementPtr->Identifier,
                           message->PoolElementPtr->RegistrationLife,
                           &message->PoolElementPtr->PolicySettings,
                           validAddressBlock,
                           getMicroTime(),
                           &poolElementNode);
         if(message->Error == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Successfully registered to pool ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fputs(" pool element ", stdlog);
            ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
            fputs("\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Namespace content:\n", stdlog);
            nameServerDump(nameServer);
            LOG_END
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
         fprintf(stdlog, "Registration request to pool ");
         poolHandlePrint(&message->Handle, stdlog);
         fputs(" of pool element ", stdlog);
         ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
         fputs(" was not possible, since no valid addresses are available\n", stdlog);
         LOG_END
         message->Error = RSPERR_NO_USABLE_ADDRESSES;
      }
      free(addressArray);
      addressArray = NULL;

      if(asapMessageSend(fd, assocID, 0, message) == false) {
         LOG_WARNING
         logerror("Sending registration response failed");
         LOG_END
      }
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "Unable to obtain peer addresses of FD %d, AssocID %u\n",
              fd, assocID);
      LOG_END
   }
}


/* ###### Handle deregistration request ################################## */
static void handleDeregistrationRequest(struct NameServer*  nameServer,
                                        int                 fd,
                                        sctp_assoc_t        assocID,
                                        struct ASAPMessage* message)
{
   LOG_ACTION
   fprintf(stdlog, "Deregistration request for pool element $%08x of pool ",
           message->Identifier);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message->Type  = AHT_DEREGISTRATION_RESPONSE;
   message->Error = ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
                       &nameServer->Namespace,
                       &message->Handle,
                       message->Identifier);
   if(message->Error == RSPERR_OKAY) {
      LOG_ACTION
      fprintf(stdlog, "Successfully deregistered pool element $%08x of pool ",
              message->Identifier);
      poolHandlePrint(&message->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END
      LOG_VERBOSE3
      fputs("Namespace content:\n", stdlog);
      nameServerDump(nameServer);
      LOG_END
   }
   else {
      LOG_WARNING
      fprintf(stdlog, "Failed to deregister for pool element $%08x of pool ",
            message->Identifier);
      poolHandlePrint(&message->Handle, stdlog);
      fputs(": ", stdlog);
      rserpoolErrorPrint(message->Error, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }

   if(asapMessageSend(fd, assocID, 0, message) == false) {
      LOG_WARNING
      logerror("Sending deregistration response failed");
      LOG_END
   }
}


/* ###### Handle name resolution request ################################# */
#define NAMERESOLUTION_MAX_NAME_RESOLUTION_ITEMS 16
#define NAMERESOLUTION_MAX_INCREMENT              1
static void handleNameResolutionRequest(struct NameServer*  nameServer,
                                        int                 fd,
                                        sctp_assoc_t        assocID,
                                        struct ASAPMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNodeArray[NAMERESOLUTION_MAX_NAME_RESOLUTION_ITEMS],
   size_t                            poolElementNodes = NAMERESOLUTION_MAX_NAME_RESOLUTION_ITEMS;
   GList*                            poolElementList  = NULL;

   LOG_ACTION
   fprintf(stdlog, "Name Resolution request for pool ");
   poolHandlePrint(message->PoolHandlePtr, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message->Type  = AHT_NAME_RESOLUTION_RESPONSE;
   message->Error = ST_CLASS(poolNamespaceManagementNameResolution)(
                       &nameServer->Namespace,
                       &message->Handle,
                       (struct ST_CLASS(PoolElementNode)**)&poolElementNodeArray,
                       &poolElementNodes,
                       NAMERESOLUTION_MAX_NAME_RESOLUTION_ITEMS,
                       NAMERESOLUTION_MAX_INCREMENT);
   if(message->Error == RSPERR_OKAY) {
      LOG_VERBOSE
      fprintf(stdlog, "Got %u elements:\n", poolElementNodes);
      for(i = 0;i < poolElementNodes;i++) {
         fprintf(stdlog, "#%02u: ", i + 1);
         ST_CLASS(poolElementNodePrint)(poolElementNodeArray[i],
                  stdlog,
                  PENPO_USERTRANSPORT|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PENPO_HOME_NS);
         fputs("\n", stdlog);
      }
      LOG_END

      message->PoolElementPtrArrayAutoDelete = false;
      message->PoolElementPtrArraySize       = poolElementNodes;
      for(i = 0;i < poolElementNodes;i++) {
         message->PoolElementPtrArray[i] = poolElementNodeArray[i];
      }

      if(asapMessageSend(fd, assocID, 0, message) == false) {
         LOG_WARNING
         logerror("Sending deregistration response failed");
         LOG_END
      }
   }
   else {
      LOG_WARNING
      fprintf(stdlog, "Name Resolution request for pool ");
      poolHandlePrint(message->PoolHandlePtr, stdlog);
      fputs(" failed: ", stdlog);
      rserpoolErrorPrint(message->Error, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }
}


#if 0
/* ###### Handle endpoint keepalive acknowledgement ###################### */
static void endpointKeepAliveAck(struct NameServer*     nameServer,
                                 struct NameServerUser* user,
                                 int                    fd,
                                 struct ASAPMessage*    message)
{
   struct PoolElement* poolElement;

   LOG_VERBOSE
   fprintf(stdlog, "Endpoint KeepAliveAck for $%08x in ",message->Identifier);
   poolHandlePrint(message->PoolHandlePtr, stdlog);
   fputs("\n", stdlog);
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
      fputs("Pool element not found!\n", stdlog);
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
   fprintf(stdlog, "KeepAlive-Timer for socket #%d.\n",user->FD);
   LOG_END

   message = asapMessageNew(NULL,1024);
   if(message != NULL) {
      list = g_list_first(user->PoolElementList);
      while(list != NULL) {
         poolElement = (struct PoolElement*)list->data;

         LOG_VERBOSE2
         fprintf(stdlog, "KeepAlive for pool element $%08x in pool ",poolElement->Identifier);
         poolHandlePrint(poolElement->OwnerPool->Handle, stdlog);
         fputs("\n", stdlog);
         LOG_END

         message->PoolElementPtr = poolElement;
         message->PoolHandlePtr  = poolElement->OwnerPool->Handle;
         message->Type           = AHT_ENDPOINT_KEEP_ALIVE;
         if(asapMessageSend(user->FD,0,message) == false) {
            LOG_ERROR
            fputs("Sending KeepAlive failed!\n", stdlog);
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
   fprintf(stdlog, "Timeout-Timer for socket #%d -> removing user\n",user->FD);
   LOG_END

   removeUser(nameServer,user);
}

#endif



/* ###### Handle incoming message ######################################## */
static void handleMessage(struct NameServer*  nameServer,
                          struct ASAPMessage* message,
                          int                 sd)
{
      switch(message->Type) {
         case AHT_ENDPOINT_KEEP_ALIVE:
            puts("KeepAlive request received!");
          break;
         case AHT_REGISTRATION:
            handleRegistrationRequest(nameServer, sd, message->AssocID, message);
          break;
         case AHT_DEREGISTRATION:
            handleDeregistrationRequest(nameServer, sd, message->AssocID, message);
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
            fprintf(stdlog, "Unsupported message type $%02x!\n",
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
//      asapMessageDelete(message);
//   }
}


/* ###### Handle events on sockets ####################################### */
static void nameServerSocketCallback(struct Dispatcher* dispatcher,
                                     int                fd,
                                     int                eventMask,
                                     void*              userData)
{
   struct NameServer*      nameServer = (struct NameServer*)userData;
   struct ASAPMessage*     message;
   struct sockaddr_storage remoteAddress;
   socklen_t               remoteAddressLength;

   if(fd == nameServer->NameServerSocket) {
      LOG_ACTION
      fputs("Event on name server socket...\n", stdlog);
      LOG_END

      message = asapMessageReceive(nameServer->NameServerSocket,
                                   0, 0,
                                   65536,
                                   (struct sockaddr*)&remoteAddress,
                                   &remoteAddressLength);
      if(message) {
/*


struct ASAPMessage* asapMessageReceive(int              fd,
                                       const card64     peekTimeout,
                                       const card64     totalTimeout,
                                       const size_t     minBufferSize,
                                       struct sockaddr* senderAddress,
                                       socklen_t*       senderAddressLength)

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
*/
         LOG_VERBOSE
         fprintf(stdlog, "Got %u bytes message from ", message->BufferSize);
         fputaddress((struct sockaddr*)&remoteAddress, true, stdlog);
         fprintf(stdlog, ", assoc #%u, PPID $%x\n",
                 message->AssocID, message->PPID);
         LOG_END

         handleMessage(nameServer, message, nameServer->NameServerSocket);
         asapMessageDelete(message);
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
      if(!(strncasecmp(argv[i], "-tcp=",5))) {
         fputs("ERROR: TCP mapping is not supported -> Use SCTP!", stderr);
         exit(1);
      }
      else if(!(strncasecmp(argv[i], "-sctp=",6))) {
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

         sctpSocket = ext_socket(checkIPv6() ? AF_INET6 : AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
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
      else if(!(strncasecmp(argv[i], "-announce=", 10))) {
         address = (char*)&argv[i][10];
         if(!string2address(address, &announceAddress)) {
            printf("ERROR: Bad announce address %s! Use format <address:port>.\n",address);
            exit(1);
         }
         hasAnnounceAddress = true;
      }
      else if(!(strncmp(argv[i], "-log",4))) {
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
