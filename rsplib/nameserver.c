/*
 *  $Id: nameserver.c,v 1.16 2004/07/26 16:36:38 dreibh Exp $
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

#include "rserpoolmessage.h"
#include "poolnamespacemanagement.h"
#include "randomizer.h"
#include "breakdetector.h"

#include <ext_socket.h>
#include <net/if.h>
#include <sys/ioctl.h>



/* Exit immediately on Ctrl-C. No clean shutdown. */
// #define FAST_BREAK

#define MAX_NS_TRANSPORTADDRESSES                                   16
#define NAMESERVER_DEFAULT_ASAP_ANNOUNCE_INTERVAL              2111111
#define NAMESERVER_DEFAULT_ENRP_ANNOUNCE_INTERVAL              2444444
#define NAMESERVER_DEFAULT_HEARTBEAT_INTERVAL                   100000
#define NAMESERVER_DEFAULT_KEEP_ALIVE_TRANSMISSION_INTERVAL    5000000
#define NAMESERVER_DEFAULT_KEEP_ALIVE_TIMEOUT_INTERVAL         5000000


struct NameServerUserNode
{
   struct STN_CLASSNAME      Node;
   struct ST_CLASSNAME       PoolElementNodeReferenceStorage;

   int                       Socket;
   sctp_assoc_t              AssocID;
};

int nameServerUserNodeComparison(const void* nodePtr1, const void* nodePtr2)
{
   const struct NameServerUserNode* node1 = (const struct NameServerUserNode*)nodePtr1;
   const struct NameServerUserNode* node2 = (const struct NameServerUserNode*)nodePtr2;
   if(node1->AssocID < node2->AssocID) {
      return(-1);
   }
   else if(node1->AssocID > node2->AssocID) {
      return(1);
   }
   if(node1->Socket < node2->Socket) {
      return(-1);
   }
   else if(node1->Socket > node2->Socket) {
      return(1);
   }
   return(0);
}

void nameServerUserNodePrint(const void* nodePtr, FILE* fd)
{
   const struct NameServerUserNode* node = (const struct NameServerUserNode*)nodePtr;
   fprintf(fd, "User socket %d, assoc %u has %u pool elements:\n",
           node->Socket, node->AssocID,
           ST_METHOD(GetElements)(&node->PoolElementNodeReferenceStorage));
   ST_METHOD(Print)((struct ST_CLASSNAME*)&node->PoolElementNodeReferenceStorage, fd);
}


struct NameServer
{
   ENRPIdentifierType                       ServerID;

   struct Dispatcher*                       StateMachine;
   struct ST_CLASS(PoolNamespaceManagement) Namespace;
   struct ST_CLASS(PeerListManagement)      Peers;
   struct Timer*                            NamespaceActionTimer;
   struct Timer*                            PeerActionTimer;
   ST_CLASSNAME                             UserStorage;

   int                                      ASAPAnnounceSocket;
   union sockaddr_union                     ASAPAnnounceAddress;
   bool                                     ASAPSendAnnounces;
   int                                      ASAPSocket;
   struct TransportAddressBlock*            ASAPAddress;
   struct Timer*                            ASAPAnnounceTimer;

   int                                      ENRPMulticastSocket;
   union sockaddr_union                     ENRPMulticastAddress;
   int                                      ENRPUnicastSocket;
   struct TransportAddressBlock*            ENRPUnicastAddress;
   bool                                     ENRPUseMulticast;
   struct Timer*                            ENRPAnnounceTimer;

   card64                                   ASAPAnnounceInterval;
   card64                                   ENRPAnnounceInterval;
   card64                                   HeartbeatInterval;
   card64                                   KeepAliveTransmissionInterval;
   card64                                   KeepAliveTimeoutInterval;
   card64                                   MessageReceptionTimeout;
};


struct PoolElementNodeReference
{
   struct STN_CLASSNAME              Node;
   struct ST_CLASS(PoolElementNode)* Reference;
};


void nameServerDumpNamespace(struct NameServer* nameServer);
void nameServerDumpUsers(struct NameServer* nameServer);


static int poolElementNodeReferenceComparison(const void* nodePtr1, const void* nodePtr2)
{
   const struct PoolElementNodeReference* node1 =
      (const struct PoolElementNodeReference*)nodePtr1;
   const struct PoolElementNodeReference* node2 =
      (const struct PoolElementNodeReference*)nodePtr2;

   if((long)node1->Reference < (long)node2->Reference) {
      return(-1);
   }
   else if((long)node1->Reference > (long)node2->Reference) {
      return(1);
   }
   return(0);
}

static void poolElementNodeReferencePrint(const void* nodePtr, FILE* fd)
{
   struct PoolElementNodeReference* node =
      (struct PoolElementNodeReference*)nodePtr;
   ST_CLASS(poolElementNodePrint)(node->Reference, fd,
            PENPO_USERTRANSPORT|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PENPO_HOME_NS);
}



struct NameServerUserNode* nameServerAddUser(
                              struct NameServer*                nameServer,
                              const int                         sd,
                              const sctp_assoc_t                assocID,
                              struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct NameServerUserNode*       nameServerUserNode;
   struct PoolElementNodeReference* poolElementNodeReference;
   struct NameServerUserNode        cmpNameServerUserNode;
   struct PoolElementNodeReference  cmpPoolElementNodeReference;

   cmpNameServerUserNode.Socket  = sd;
   cmpNameServerUserNode.AssocID = assocID;
   nameServerUserNode = (struct NameServerUserNode*)ST_METHOD(Find)(
                                                       &nameServer->UserStorage,
                                                       &cmpNameServerUserNode.Node);
   if(nameServerUserNode == NULL) {
      nameServerUserNode = (struct NameServerUserNode*)malloc(sizeof(struct NameServerUserNode));
      if(nameServerUserNode != NULL) {
         STN_METHOD(New)(&nameServerUserNode->Node);
         ST_METHOD(New)(&nameServerUserNode->PoolElementNodeReferenceStorage,
                         poolElementNodeReferencePrint,
                         poolElementNodeReferenceComparison);
         nameServerUserNode->Socket  = sd;
         nameServerUserNode->AssocID = assocID;
         ST_METHOD(Insert)(&nameServer->UserStorage,
                           &nameServerUserNode->Node);
      }
   }

   if(nameServerUserNode != NULL) {
      poolElementNodeReference = (struct PoolElementNodeReference*)ST_METHOD(Find)(
                                    &nameServerUserNode->PoolElementNodeReferenceStorage,
                                    &cmpPoolElementNodeReference.Node);
      if(poolElementNodeReference == NULL) {
         poolElementNodeReference = (struct PoolElementNodeReference*)malloc(sizeof(struct PoolElementNodeReference));
         if(poolElementNodeReference != NULL) {
            STN_METHOD(New)(&poolElementNodeReference->Node);
            poolElementNodeReference->Reference = poolElementNode;
            ST_METHOD(Insert)(&nameServerUserNode->PoolElementNodeReferenceStorage,
                              &poolElementNodeReference->Node);
         }
         else {
            nameServerUserNode = NULL;
         }
      }
   }

   return(nameServerUserNode);
}

void nameServerRemoveUser(struct NameServer*                nameServer,
                          const int                         sd,
                          const sctp_assoc_t                assocID,
                          struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   /*
      WARNING: Do not use poolElementNode here,
               it may point to already deallocated memory!
   */
   struct NameServerUserNode*       nameServerUserNode;
   struct PoolElementNodeReference* poolElementNodeReference;
   struct NameServerUserNode        cmpNameServerUserNode;
   struct PoolElementNodeReference  cmpPoolElementNodeReference;

   cmpNameServerUserNode.Socket  = sd;
   cmpNameServerUserNode.AssocID = assocID;
   nameServerUserNode = (struct NameServerUserNode*)ST_METHOD(Find)(
                                                      &nameServer->UserStorage,
                                                      &cmpNameServerUserNode.Node);
   if(nameServerUserNode != NULL) {
      cmpPoolElementNodeReference.Reference = poolElementNode;
      poolElementNodeReference = (struct PoolElementNodeReference*)ST_METHOD(Find)(
                                    &nameServerUserNode->PoolElementNodeReferenceStorage,
                                    &cmpPoolElementNodeReference.Node);
      if(poolElementNodeReference != NULL) {
         ST_METHOD(Remove)(&nameServerUserNode->PoolElementNodeReferenceStorage,
                           &poolElementNodeReference->Node);
         free(poolElementNodeReference);
      }
      if(ST_METHOD(GetElements)(&nameServerUserNode->PoolElementNodeReferenceStorage) == 0) {
         ST_METHOD(Remove)(&nameServer->UserStorage,
                           &nameServerUserNode->Node);
         free(nameServerUserNode);
      }
   }
}

void nameServerCleanUser(struct NameServer* nameServer,
                         const int          sd,
                         const sctp_assoc_t assocID)
{
   struct PoolElementNodeReference* poolElementNodeReference;
   struct PoolElementNodeReference* nextPoolElementNodeReference;
   struct NameServerUserNode*       nameServerUserNode;
   struct NameServerUserNode        cmpNameServerUserNode;
   unsigned int                     result;

   cmpNameServerUserNode.Socket  = sd;
   cmpNameServerUserNode.AssocID = assocID;
   nameServerUserNode = (struct NameServerUserNode*)ST_METHOD(Find)(
                                                      &nameServer->UserStorage,
                                                      &cmpNameServerUserNode.Node);
   if(nameServerUserNode != NULL) {
      LOG_ACTION
      fprintf(stdlog,
              "Removing all pool elements registered by user socket %u, assoc %u...\n",
              sd, assocID);
      LOG_END

      poolElementNodeReference = (struct PoolElementNodeReference*)ST_METHOD(GetFirst)(
                                    &nameServerUserNode->PoolElementNodeReferenceStorage);
      while(poolElementNodeReference != NULL) {
         nextPoolElementNodeReference = (struct PoolElementNodeReference*)ST_METHOD(GetNext)(
                                      &nameServerUserNode->PoolElementNodeReferenceStorage,
                                      &poolElementNodeReference->Node);
         LOG_VERBOSE
         fprintf(stdlog, "Removing pool element $%08x of pool ",
                 &poolElementNodeReference->Reference->Identifier);
         poolHandlePrint(&poolElementNodeReference->Reference->OwnerPoolNode->Handle, stdlog);
         fputs("\n", stdlog);
         LOG_END
         result = ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
                     &nameServer->Namespace,
                     &poolElementNodeReference->Reference->OwnerPoolNode->Handle,
                     poolElementNodeReference->Reference->Identifier);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("User clean-up successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Namespace content:\n", stdlog);
            nameServerDumpNamespace(nameServer);
            fputs("Users:\n", stdlog);
            nameServerDumpUsers(nameServer);
            LOG_END
         }
         else {
            LOG_ERROR
            fprintf(stdlog, "Failed to deregister for pool element $%08x of pool ",
                    poolElementNodeReference->Reference->Identifier);
            poolHandlePrint(&poolElementNodeReference->Reference->OwnerPoolNode->Handle, stdlog);
            fputs(": ", stdlog);
            rserpoolErrorPrint(result, stdlog);
            fputs("\n", stdlog);
            LOG_END_FATAL
         }

         poolElementNodeReference = nextPoolElementNodeReference;
      }

      LOG_ACTION
      fprintf(stdlog,
               "All pool elements registered by user socket %u, assoc %u have been removed\n",
               sd, assocID);
      LOG_END
   }
   else {
      LOG_VERBOSE
      fprintf(stdlog,
              "No pool elements are registered by user socket %u, assoc %u -> nothing to do\n",
              sd, assocID);
      LOG_END
   }
}

void nameServerDumpUsers(struct NameServer* nameServer)
{
   fputs("*************** User Dump ********************\n", stdlog);
   ST_METHOD(Print)(&nameServer->UserStorage, stdlog);
   fputs("**********************************************\n", stdlog);

}




struct NameServer* nameServerNew(const ENRPIdentifierType      serverID,
                                 int                           asapSocket,
                                 struct TransportAddressBlock* asapAddress,
                                 int                           enrpUnicastSocket,
                                 struct TransportAddressBlock* enrpUnicastAddress,
                                 const bool                    asapSendAnnounces,
                                 const union sockaddr_union*   asapAnnounceAddress,
                                 const bool                    enrpUseMulticast,
                                 const union sockaddr_union*   enrpMulticastAddress);
void nameServerDelete(struct NameServer* nameServer);

static void sendPeerNameUpdate(struct NameServer*                nameServer,
                               struct ST_CLASS(PoolElementNode)* poolElementNode,
                               const uint16_t                    action);

static void asapAnnounceTimerCallback(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData);
static void enrpAnnounceTimerCallback(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData);
static void namespaceActionTimerCallback(struct Dispatcher* dispatcher,
                                         struct Timer*      timer,
                                         void*              userData);
static void peerActionTimerCallback(struct Dispatcher* dispatcher,
                                    struct Timer*      timer,
                                    void*              userData);
static void nameServerSocketCallback(struct Dispatcher* dispatcher,
                                     int                fd,
                                     int                eventMask,
                                     void*              userData);




static void poolElementNodeDisposer(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                    void*                             userData)
{
   struct NameServer*         nameServer         = (struct NameServer*)userData;
   struct NameServerUserNode* nameServerUserNode = (struct NameServerUserNode*)poolElementNode->UserData;

   if(nameServerUserNode) {

      /* ====== Send update to peers =============================== */
      if(poolElementNode->HomeNSIdentifier == nameServer->ServerID) {
         sendPeerNameUpdate(nameServer, poolElementNode, PNUP_DEL_PE);
      }

      nameServerRemoveUser(nameServer,
                           nameServerUserNode->Socket,
                           nameServerUserNode->AssocID,
                           poolElementNode);

      LOG_VERBOSE3
      fputs("Users:\n", stdlog);
      nameServerDumpUsers(nameServer);
      LOG_END
   }
}


static void peerListNodeDisposer(struct ST_CLASS(PeerListNode)* peerListNode,
                                     void*                          userData)
{
   struct NameServer* nameServer = (struct NameServer*)userData;

}




/* ###### Constructor #################################################### */
struct NameServer* nameServerNew(const ENRPIdentifierType      serverID,
                                 int                           asapSocket,
                                 struct TransportAddressBlock* asapAddress,
                                 int                           enrpUnicastSocket,
                                 struct TransportAddressBlock* enrpUnicastAddress,
                                 const bool                    asapSendAnnounces,
                                 const union sockaddr_union*   asapAnnounceAddress,
                                 const bool                    enrpUseMulticast,
                                 const union sockaddr_union*   enrpMulticastAddress)
{
   struct NameServer* nameServer = (struct NameServer*)malloc(sizeof(struct NameServer));
   if(nameServer != NULL) {
      nameServer->ServerID = serverID;
      if(nameServer->ServerID == 0) {
         nameServer->ServerID = random32();
      }
      nameServer->StateMachine = dispatcherNew(dispatcherDefaultLock, dispatcherDefaultUnlock, NULL);
      if(nameServer->StateMachine == NULL) {
         free(nameServer);
         return(NULL);
      }
      ST_CLASS(poolNamespaceManagementNew)(&nameServer->Namespace,
                                           nameServer->ServerID,
                                           NULL,
                                           poolElementNodeDisposer,
                                           nameServer);
      ST_CLASS(peerListManagementNew)(&nameServer->Peers,
                                      nameServer->ServerID,
                                      peerListNodeDisposer,
                                      nameServer);
      ST_METHOD(New)(&nameServer->UserStorage,
                     nameServerUserNodePrint,
                     nameServerUserNodeComparison);

      nameServer->ASAPSocket         = asapSocket;
      nameServer->ASAPAddress        = asapAddress;
      nameServer->ASAPSendAnnounces  = asapSendAnnounces;
      nameServer->ENRPUnicastSocket  = enrpUnicastSocket;
      nameServer->ENRPUnicastAddress = enrpUnicastAddress;
      nameServer->ENRPUseMulticast   = enrpUseMulticast;

      nameServer->KeepAliveTransmissionInterval = NAMESERVER_DEFAULT_KEEP_ALIVE_TRANSMISSION_INTERVAL;
      nameServer->KeepAliveTimeoutInterval      = NAMESERVER_DEFAULT_KEEP_ALIVE_TIMEOUT_INTERVAL;
      nameServer->ASAPAnnounceInterval          = NAMESERVER_DEFAULT_ASAP_ANNOUNCE_INTERVAL;
      nameServer->ENRPAnnounceInterval          = NAMESERVER_DEFAULT_ENRP_ANNOUNCE_INTERVAL;
      nameServer->HeartbeatInterval             = NAMESERVER_DEFAULT_HEARTBEAT_INTERVAL;

      memcpy(&nameServer->ASAPAnnounceAddress, asapAnnounceAddress, sizeof(union sockaddr_union));
      if(nameServer->ASAPAnnounceAddress.in6.sin6_family == AF_INET6) {
         nameServer->ASAPAnnounceSocket = ext_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
      }
      else if(nameServer->ASAPAnnounceAddress.in.sin_family == AF_INET) {
         nameServer->ASAPAnnounceSocket = ext_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      }
      else {
         nameServer->ASAPAnnounceSocket = -1;
      }
      if(nameServer->ASAPAnnounceSocket >= 0) {
         setNonBlocking(nameServer->ASAPAnnounceSocket);
      }

      memcpy(&nameServer->ENRPMulticastAddress, enrpMulticastAddress, sizeof(union sockaddr_union));
      if(nameServer->ENRPMulticastAddress.in6.sin6_family == AF_INET6) {
         nameServer->ENRPMulticastSocket = ext_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
      }
      else if(nameServer->ENRPMulticastAddress.in.sin_family == AF_INET) {
         nameServer->ENRPMulticastSocket = ext_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      }
      else {
         nameServer->ENRPMulticastSocket = -1;
      }
      if(nameServer->ENRPMulticastSocket >= 0) {
         setNonBlocking(nameServer->ENRPMulticastSocket);
      }

      nameServer->ASAPAnnounceTimer = timerNew(nameServer->StateMachine,
                                               asapAnnounceTimerCallback,
                                               (void*)nameServer);
      nameServer->ENRPAnnounceTimer = timerNew(nameServer->StateMachine,
                                               enrpAnnounceTimerCallback,
                                               (void*)nameServer);
      nameServer->NamespaceActionTimer = timerNew(nameServer->StateMachine,
                                                  namespaceActionTimerCallback,
                                                  (void*)nameServer);
      nameServer->PeerActionTimer = timerNew(nameServer->StateMachine,
                                             peerActionTimerCallback,
                                             (void*)nameServer);
      if((nameServer->ASAPAnnounceTimer == NULL) ||
         (nameServer->ENRPAnnounceTimer == NULL) ||
         (nameServer->NamespaceActionTimer == NULL) ||
         (nameServer->PeerActionTimer == NULL)) {
         nameServerDelete(nameServer);
         return(NULL);
      }
      if(nameServer->ASAPSendAnnounces) {
         timerStart(nameServer->ASAPAnnounceTimer, 0);
      }
      if(nameServer->ENRPUseMulticast) {
         timerStart(nameServer->ENRPAnnounceTimer, 0);
      }
      if(nameServer->ENRPUseMulticast) {
         if(joinOrLeaveMulticastGroup(nameServer->ENRPMulticastSocket,
                                      &nameServer->ENRPMulticastAddress,
                                      true) == false) {
            LOG_ERROR
            logerror("Unable to join ENRP multicast group");
            LOG_END
            nameServerDelete(nameServer);
            return(NULL);
         }
         dispatcherAddFDCallback(nameServer->StateMachine,
                                 nameServer->ENRPMulticastSocket,
                                 FDCE_Read|FDCE_Exception,
                                 nameServerSocketCallback,
                                 (void*)nameServer);
      }

      dispatcherAddFDCallback(nameServer->StateMachine,
                              nameServer->ASAPSocket,
                              FDCE_Read|FDCE_Exception,
                              nameServerSocketCallback,
                              (void*)nameServer);
      dispatcherAddFDCallback(nameServer->StateMachine,
                              nameServer->ENRPUnicastSocket,
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
      dispatcherRemoveFDCallback(nameServer->StateMachine, nameServer->ASAPSocket);
      ST_CLASS(peerListManagementClear)(&nameServer->Peers);
      ST_CLASS(peerListManagementDelete)(&nameServer->Peers);
      ST_CLASS(poolNamespaceManagementClear)(&nameServer->Namespace);
      ST_CLASS(poolNamespaceManagementDelete)(&nameServer->Namespace);
      if(nameServer->ASAPAnnounceTimer) {
         timerDelete(nameServer->ASAPAnnounceTimer);
         nameServer->ASAPAnnounceTimer = NULL;
      }
      if(nameServer->ENRPAnnounceTimer) {
         timerDelete(nameServer->ENRPAnnounceTimer);
         nameServer->ENRPAnnounceTimer = NULL;
      }
      if(nameServer->NamespaceActionTimer) {
         timerDelete(nameServer->NamespaceActionTimer);
         nameServer->NamespaceActionTimer = NULL;
      }
      if(nameServer->PeerActionTimer) {
         timerDelete(nameServer->PeerActionTimer);
         nameServer->PeerActionTimer = NULL;
      }
      if(nameServer->ASAPAnnounceSocket >= 0) {
         ext_close(nameServer->ASAPAnnounceSocket >= 0);
         nameServer->ASAPAnnounceSocket = -1;
      }
      if(nameServer->ENRPMulticastSocket >= 0) {
         ext_close(nameServer->ENRPMulticastSocket >= 0);
         nameServer->ENRPMulticastSocket = -1;
      }
      ST_METHOD(Delete)(&nameServer->UserStorage);
      if(nameServer->StateMachine) {
         dispatcherDelete(nameServer->StateMachine);
         nameServer->StateMachine = NULL;
      }
      free(nameServer);
   }
}


/* ###### Dump namespace ################################################# */
void nameServerDumpNamespace(struct NameServer* nameServer)
{
   fputs("*************** Namespace Dump ***************\n", stdlog);
   printTimeStamp(stdlog);
   fputs("\n", stdlog);
   ST_CLASS(poolNamespaceManagementPrint)(&nameServer->Namespace, stdlog,
            PNNPO_POOLS_INDEX|PNNPO_POOLS_SELECTION|PNNPO_POOLS_TIMER|PENPO_USERTRANSPORT|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PENPO_HOME_NS);
   fputs("**********************************************\n", stdlog);
}


/* ###### Send ASAP announcement message ################################# */
static void asapAnnounceTimerCallback(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData)
{
   struct NameServer*      nameServer = (struct NameServer*)userData;
   struct RSerPoolMessage* message;
   size_t                  messageLength;

   CHECK(nameServer->ASAPSendAnnounces == true);
   CHECK(nameServer->ASAPAnnounceSocket >= 0);

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type                         = AHT_SERVER_ANNOUNCE;
      message->Flags                        = 0x00;
      message->NSIdentifier                 = nameServer->ServerID;
      message->TransportAddressBlockListPtr = nameServer->ASAPAddress;
      messageLength = rserpoolMessage2Packet(message);
      if(messageLength > 0) {
         if(nameServer->ASAPAnnounceSocket) {
            LOG_VERBOSE2
            fputs("Sending announce to address ", stdlog);
            fputaddress((struct sockaddr*)&nameServer->ASAPAnnounceAddress, true, stdlog);
            fputs("\n", stdlog);
            LOG_END
            if(ext_sendto(nameServer->ASAPAnnounceSocket,
                          message->Buffer,
                          messageLength,
                          0,
                          (struct sockaddr*)&nameServer->ASAPAnnounceAddress,
                          getSocklen((struct sockaddr*)&nameServer->ASAPAnnounceAddress)) < (ssize_t)messageLength) {
               LOG_WARNING
               logerror("Unable to send announce");
               LOG_END
            }
         }
      }
      rserpoolMessageDelete(message);
   }
   timerStart(timer, getMicroTime() + nameServer->ASAPAnnounceInterval);
}


/* ###### Send ENRP peer presence message ################################ */
static void enrpAnnounceTimerCallback(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData)
{
   struct NameServer*      nameServer = (struct NameServer*)userData;
   struct RSerPoolMessage* message;
   size_t                  messageLength;
   ST_CLASS(PeerListNode)  peerListNode;

   CHECK(nameServer->ENRPUseMulticast == true);
   CHECK(nameServer->ENRPMulticastSocket >= 0);

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type                      = EHT_PEER_PRESENCE;
      message->Flags                     = 0x00;
      message->PeerListNodePtr           = &peerListNode;
      message->PeerListNodePtrAutoDelete = false;
      message->SenderID                  = nameServer->ServerID;
      message->ReceiverID                = 0;

      ST_CLASS(peerListNodeNew)(&peerListNode,
                                nameServer->ServerID,
                                0x00,
                                nameServer->ENRPUnicastAddress);

      messageLength = rserpoolMessage2Packet(message);
      if(messageLength > 0) {
         if(nameServer->ENRPMulticastSocket) {
            LOG_VERBOSE2
            fputs("Sending peer presence to address ", stdlog);
            fputaddress((struct sockaddr*)&nameServer->ENRPMulticastAddress, true, stdlog);
            fputs("\n", stdlog);
            LOG_END
            if(ext_sendto(nameServer->ENRPMulticastSocket,
                          message->Buffer,
                          messageLength,
                          0,
                          (struct sockaddr*)&nameServer->ENRPMulticastAddress,
                          getSocklen((struct sockaddr*)&nameServer->ENRPMulticastAddress)) < (ssize_t)messageLength) {
               LOG_WARNING
               logerror("Unable to send peer presence");
               LOG_END
            }
         }
      }
      rserpoolMessageDelete(message);
   }

   timerStart(timer, getMicroTime() + nameServer->ENRPAnnounceInterval);
}


/* ###### Send Endpoint Keep Alive ####################################### */
static void sendEndpointKeepAlive(struct NameServer*                nameServer,
                                  struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct NameServerUserNode* nameServerUserNode = (struct NameServerUserNode*)poolElementNode->UserData;
   struct RSerPoolMessage*    message;

   if(nameServerUserNode) {
      message = rserpoolMessageNew(NULL, 1024);
      if(message != NULL) {
         LOG_VERBOSE2
         fprintf(stdlog, "Sending KeepAlive for pool element $%08x in pool ",
                 poolElementNode->Identifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
         fputs("\n", stdlog);
         LOG_END

         message->Handle     = poolElementNode->OwnerPoolNode->Handle;
         message->Identifier = poolElementNode->Identifier;
         message->Type       = AHT_ENDPOINT_KEEP_ALIVE;
         message->Flags      = 0x00;
         if(rserpoolMessageSend(nameServerUserNode->Socket,
                                nameServerUserNode->AssocID,
                                0,
                                message) == false) {
            LOG_ERROR
            fprintf(stdlog, "Sending KeepAlive for pool element $%08x in pool ",
                     poolElementNode->Identifier);
            poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
            fputs(" failed\n", stdlog);
            LOG_END
            return;
         }

         rserpoolMessageDelete(message);
      }
   }
}


/* ###### Handle namespace management timers ############################# */
static void namespaceActionTimerCallback(struct Dispatcher* dispatcher,
                                         struct Timer*      timer,
                                         void*              userData)
{
   struct NameServer*                nameServer = (struct NameServer*)userData;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   unsigned int                      result;

   poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementTimerNode)(
                        &nameServer->Namespace.Namespace);
   while((poolElementNode != NULL) &&
         (poolElementNode->TimerTimeStamp <= getMicroTime())) {
      nextPoolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementTimerNode)(
                               &nameServer->Namespace.Namespace,
                               poolElementNode);

      if(poolElementNode->TimerCode == PENT_KEEPALIVE_TRANSMISSION) {
         ST_CLASS(poolNamespaceNodeDeactivateTimer)(
            &nameServer->Namespace.Namespace,
            poolElementNode);

         sendEndpointKeepAlive(nameServer, poolElementNode);

         ST_CLASS(poolNamespaceNodeActivateTimer)(
            &nameServer->Namespace.Namespace,
            poolElementNode,
            PENT_KEEPALIVE_TIMEOUT,
            getMicroTime() + nameServer->KeepAliveTimeoutInterval);
      }

      else if( (poolElementNode->TimerCode == PENT_KEEPALIVE_TIMEOUT) ||
               (poolElementNode->TimerCode == PENT_EXPIRY) ) {
         if(poolElementNode->TimerCode == PENT_KEEPALIVE_TIMEOUT) {
            LOG_ACTION
            fprintf(stdlog, "Keep alive timeout expired pool element $%08x of pool ",
                  poolElementNode->Identifier);
            poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
            fputs(" -> removing it\n", stdlog);
            LOG_END
         }
         else {
            LOG_ACTION
            fprintf(stdlog, "Expiry timeout expired pool element $%08x of pool ",
                  poolElementNode->Identifier);
            poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
            fputs(" -> removing it\n", stdlog);
            LOG_END
         }
         result = ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
                     &nameServer->Namespace,
                     &poolElementNode->OwnerPoolNode->Handle,
                     poolElementNode->Identifier);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Deregistration successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Namespace content:\n", stdlog);
            nameServerDumpNamespace(nameServer);
            fputs("Users:\n", stdlog);
            nameServerDumpUsers(nameServer);
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

   timerRestart(nameServer->NamespaceActionTimer,
               ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                  &nameServer->Namespace));
}


/* ###### Handle peer management timers ################################## */
static void peerActionTimerCallback(struct Dispatcher* dispatcher,
                                    struct Timer*      timer,
                                    void*              userData)
{
   // struct NameServer* nameServer = (struct NameServer*)userData;
}


/* ###### Send peer name update ########################################## */
static void sendPeerNameUpdate(struct NameServer*                nameServer,
                               struct ST_CLASS(PoolElementNode)* poolElementNode,
                               const uint16_t                    action)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct RSerPoolMessage*        message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type                     = EHT_PEER_NAME_UPDATE;
      message->Flags                    = action;
      message->SenderID                 = nameServer->ServerID;
      message->Handle                   = poolElementNode->OwnerPoolNode->Handle;
      message->PoolElementPtr           = poolElementNode;
      message->PoolElementPtrAutoDelete = false;

      LOG_VERBOSE
      fputs("Sending PeerNameUpdate for pool element ", stdlog);
      ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL),
      fputs(" of pool ", stdlog);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fprintf(stdlog, ", action $%04...\n", action);
      LOG_END

      if(nameServer->ENRPUseMulticast) {
         LOG_VERBOSE2
         fputs("Sending PeerNameUpdate via ENRP multicast socket...\n", stdlog);
         LOG_END
         message->ReceiverID    = 0;
         message->Address       = (struct sockaddr*)&nameServer->ENRPMulticastAddress;
         message->AddressLength = getSocklen(message->Address);
         if(rserpoolMessageSend(nameServer->ENRPMulticastSocket,
                                0, 0,
                                message) == false) {
            LOG_WARNING
            fputs("Sending PeerNameUpdate via ENRP multicast socket failed\n", stdlog);
            LOG_END
         }
      }

      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&nameServer->Peers.List);
      while(peerListNode != NULL) {
         if(!(peerListNode->Flags & PLNF_MULTICAST)) {
            message->ReceiverID = peerListNode->Identifier;
            LOG_VERBOSE
            fprintf(stdlog, "Sending PeerNameUpdate to unicast peer $%08x...\n",
                    peerListNode->Identifier);
            LOG_END

            // ???????????
         }
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                           &nameServer->Peers.List,
                           peerListNode);
      }

      rserpoolMessageDelete(message);
   }
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
      validAddressBlock->Port      = sourceAddressBlock->Port;
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
                                      struct RSerPoolMessage* message)
{
   char                              validAddressBlockBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     validAddressBlock = (struct TransportAddressBlock*)&validAddressBlockBuffer;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct NameServerUserNode*        nameServerUserNode;
   struct sockaddr_storage*          addressArray;
   struct TagItem                    tags[8];
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
   message->Flags      = AHF_REGISTRATION_REJECT;
   message->Error      = RSPERR_OKAY;
   message->Identifier = message->PoolElementPtr->Identifier;


   /* ====== Get peer addresses ========================================== */
   addresses = getpaddrsplus(fd, assocID, &addressArray);
   if(addresses > 0) {
      LOG_VERBOSE1
      fputs("SCTP association's valid peer addresses:\n", stdlog);
      for(i = 0;i < addresses;i++) {
         fprintf(stdlog, "%d/%d: ",i + 1,addresses);
         fputaddress((struct sockaddr*)&addressArray[i],false, stdlog);
         fputs("\n", stdlog);
      }
      LOG_END

      /* ====== Filter addresses ========================================= */
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
            /* ====== Successful registration ============================ */
            message->Flags = 0x00;

            LOG_ACTION
            fputs("Successfully registered to pool ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fputs(" pool element ", stdlog);
            ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
            fputs("\n", stdlog);
            LOG_END

            /* ====== Add user =========================================== */
            nameServerUserNode = nameServerAddUser(nameServer, fd, assocID, poolElementNode);
            if((poolElementNode->UserData != NULL) &&
               (poolElementNode->UserData != nameServerUserNode)) {
               LOG_WARNING
               fputs("NameServerUserNode changed for pool element ", stdlog);
               ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
               fputs(" in pool ", stdlog);
               ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
               fputs("\n", stdlog);
               LOG_END
            }
            poolElementNode->UserData = nameServerUserNode;

            /* ====== Tune SCTP association ============================== */
            tags[0].Tag = TAG_TuneSCTP_MinRTO;
            tags[0].Data = 100;
            tags[1].Tag = TAG_TuneSCTP_MaxRTO;
            tags[1].Data = 500;
            tags[2].Tag = TAG_TuneSCTP_InitialRTO;
            tags[2].Data = 250;
            tags[3].Tag = TAG_TuneSCTP_Heartbeat;
            tags[3].Data = (nameServer->HeartbeatInterval / 1000);
            tags[4].Tag = TAG_TuneSCTP_PathMaxRXT;
            tags[4].Data = 3;
            tags[5].Tag = TAG_TuneSCTP_AssocMaxRXT;
            tags[5].Data = 9;
            tags[6].Tag = TAG_DONE;
            if(!tuneSCTP(fd, assocID, (struct TagItem*)&tags)) {
               LOG_WARNING
               fprintf(stdlog, "Unable to tune SCTP association %u's parameters\n",
                       assocID);
               LOG_END
            }

            /* ====== Activate keep alive timer ========================== */
            if(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
               ST_CLASS(poolNamespaceNodeActivateTimer)(
                  &nameServer->Namespace.Namespace,
                  poolElementNode,
                  PENT_KEEPALIVE_TRANSMISSION,
                  getMicroTime() + nameServer->KeepAliveTransmissionInterval);
            }
            timerRestart(nameServer->NamespaceActionTimer,
                         ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                            &nameServer->Namespace));

            /* ====== Print debug information ============================ */
            LOG_VERBOSE3
            fputs("Namespace content:\n", stdlog);
            nameServerDumpNamespace(nameServer);
            fputs("Users:\n", stdlog);
            nameServerDumpUsers(nameServer);
            LOG_END

            /* ====== Send update to peers =============================== */
            sendPeerNameUpdate(nameServer, poolElementNode, PNUP_ADD_PE);
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

      if(rserpoolMessageSend(fd, assocID, 0, message) == false) {
         LOG_WARNING
         logerror("Sending registration response failed");
         LOG_END
      }
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "Unable to obtain peer addresses of FD %d, assoc %u\n",
              fd, assocID);
      LOG_END
   }
}


/* ###### Handle deregistration request ################################## */
static void handleDeregistrationRequest(struct NameServer*      nameServer,
                                        int                     fd,
                                        sctp_assoc_t            assocID,
                                        struct RSerPoolMessage* message)
{
   LOG_ACTION
   fprintf(stdlog, "Deregistration request for pool element $%08x of pool ",
           message->Identifier);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message->Type  = AHT_DEREGISTRATION_RESPONSE;
   message->Flags = AHF_DEREGISTRATION_REJECT;
   message->Error = ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
                       &nameServer->Namespace,
                       &message->Handle,
                       message->Identifier);
   if(message->Error == RSPERR_OKAY) {
      message->Flags = 0x00;

      LOG_ACTION
      fputs("Deregistration successfully completed\n", stdlog);
      LOG_END
      LOG_VERBOSE3
      fputs("Namespace content:\n", stdlog);
      nameServerDumpNamespace(nameServer);
      fputs("Users:\n", stdlog);
      nameServerDumpUsers(nameServer);
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

   if(rserpoolMessageSend(fd, assocID, 0, message) == false) {
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
                                        struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNodeArray[NAMERESOLUTION_MAX_NAME_RESOLUTION_ITEMS];
   size_t                            poolElementNodes = NAMERESOLUTION_MAX_NAME_RESOLUTION_ITEMS;
   size_t                            i;

   LOG_ACTION
   fprintf(stdlog, "Name Resolution request for pool ");
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message->Type  = AHT_NAME_RESOLUTION_RESPONSE;
   message->Flags = 0x00;
   message->Error = ST_CLASS(poolNamespaceManagementNameResolution)(
                       &nameServer->Namespace,
                       &message->Handle,
                       (struct ST_CLASS(PoolElementNode)**)&poolElementNodeArray,
                       &poolElementNodes,
                       NAMERESOLUTION_MAX_NAME_RESOLUTION_ITEMS,
                       NAMERESOLUTION_MAX_INCREMENT);
   if(message->Error == RSPERR_OKAY) {
      LOG_VERBOSE
      fprintf(stdlog, "Got %u element(s):\n", poolElementNodes);
      for(i = 0;i < poolElementNodes;i++) {
         fprintf(stdlog, "#%02u: ", i + 1);
         ST_CLASS(poolElementNodePrint)(poolElementNodeArray[i],
                  stdlog,
                  PENPO_USERTRANSPORT|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PENPO_HOME_NS);
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
      fprintf(stdlog, "Name Resolution request for pool ");
      poolHandlePrint(&message->Handle, stdlog);
      fputs(" failed: ", stdlog);
      rserpoolErrorPrint(message->Error, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }

   if(rserpoolMessageSend(fd, assocID, 0, message) == false) {
      LOG_WARNING
      logerror("Sending deregistration response failed");
      LOG_END
   }
}


/* ###### Handle endpoint keepalive acknowledgement ###################### */
static void handleEndpointKeepAliveAck(struct NameServer*      nameServer,
                                       int                     fd,
                                       sctp_assoc_t            assocID,
                                       struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   LOG_VERBOSE
   fprintf(stdlog, "Got EndpointKeepAliveAck for pool element $%08x of pool ",
           message->Identifier);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   poolElementNode = ST_CLASS(poolNamespaceManagementFindPoolElement)(
                        &nameServer->Namespace,
                        &message->Handle,
                        message->Identifier);
   if(poolElementNode != NULL) {
      LOG_VERBOSE2
      fputs("KeepAlive successful, resetting timer\n", stdlog);
      LOG_END

      ST_CLASS(poolNamespaceNodeDeactivateTimer)(
         &nameServer->Namespace.Namespace,
         poolElementNode);
      ST_CLASS(poolNamespaceNodeActivateTimer)(
         &nameServer->Namespace.Namespace,
         poolElementNode,
         PENT_KEEPALIVE_TRANSMISSION,
         getMicroTime() + nameServer->KeepAliveTransmissionInterval);
      timerRestart(nameServer->NamespaceActionTimer,
                  ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                     &nameServer->Namespace));
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


/* ###### Handle endpoint unreachable #################################### */
static void handleEndpointUnreachable(struct NameServer*      nameServer,
                                      int                     fd,
                                      sctp_assoc_t            assocID,
                                      struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   LOG_ACTION
   fprintf(stdlog, "Got EndpointUnreachable for pool element $%08x of pool ",
           message->Identifier);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   poolElementNode = ST_CLASS(poolNamespaceManagementFindPoolElement)(
                        &nameServer->Namespace,
                        &message->Handle,
                        message->Identifier);
   if(poolElementNode != NULL) {
      poolElementNode->UnreachabilityReports++;
      // ??????
   }
   else {
      LOG_WARNING
      fprintf(stdlog,
              "EndpointUnreachable for not-existing pool element $%08x of pool ",
              message->Identifier);
      poolHandlePrint(&message->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }
}


/* ###### Handle peer name update ######################################## */
static void handlePeerNameUpdate(struct NameServer*      nameServer,
                                 int                     fd,
                                 sctp_assoc_t            assocID,
                                 struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   int                               result;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerNameUpdate message\n", stdlog);
      LOG_END
      return;
   }

   LOG_ACTION
   fputs("Got PeerNameUpdate for pool element ", stdlog);
   ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
   fputs(" of pool ", stdlog);
   poolHandlePrint(&message->Handle, stdlog);
   fprintf(stdlog, ", action $%04x\n", message->Action);
   LOG_END

   if(message->Action == PNUP_ADD_PE) {
      result = ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
                  &nameServer->Namespace,
                  &message->Handle,
                  message->PoolElementPtr->HomeNSIdentifier,
                  message->PoolElementPtr->Identifier,
                  message->PoolElementPtr->RegistrationLife,
                  &message->PoolElementPtr->PolicySettings,
                  message->PoolElementPtr->AddressBlock,
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

         if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
            ST_CLASS(poolNamespaceNodeDeactivateTimer)(
               &nameServer->Namespace.Namespace,
               poolElementNode);
         }
         ST_CLASS(poolNamespaceNodeActivateTimer)(
            &nameServer->Namespace.Namespace,
            poolElementNode,
            PENT_EXPIRY,
            getMicroTime() + nameServer->KeepAliveTransmissionInterval);
         timerRestart(nameServer->NamespaceActionTimer,
                     ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                        &nameServer->Namespace));

         LOG_VERBOSE3
         fputs("Namespace content:\n", stdlog);
         nameServerDumpNamespace(nameServer);
         fputs("Users:\n", stdlog);
         nameServerDumpUsers(nameServer);
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

   else if(message->Action == PNUP_DEL_PE) {
   }

   else {
      LOG_WARNING
      fprintf(stdlog, "Got PeerNameUpdate with invalid action $%04x\n",
              message->Action);
      LOG_END
   }
}


/* ###### Handle peer presence ########################################### */
static void handlePeerPresence(struct NameServer*      nameServer,
                               int                     fd,
                               sctp_assoc_t            assocID,
                               struct RSerPoolMessage* message)
{
   ST_CLASS(PeerListNode)* peerListNode;
   int                     result;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerPresence message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got PeerPresence from peer $%08x",
           message->PeerListNodePtr->Identifier);
   if(message->PeerListNodePtr) {
      fputs(" at address ", stdlog);
      transportAddressBlockPrint(message->PeerListNodePtr->AddressBlock, stdlog);
   }
   fputs("\n", stdlog);
   LOG_END

   if(message->PeerListNodePtr) {
      int flags = PLNF_DYNAMIC;
      if(fd == nameServer->ENRPMulticastSocket) {
         flags |= PLNF_MULTICAST;
      }
      result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                  &nameServer->Peers,
                  message->PeerListNodePtr->Identifier,
                  flags,
                  message->PeerListNodePtr->AddressBlock,
                  getMicroTime(),
                  &peerListNode);

      if(result == RSPERR_OKAY) {
         LOG_VERBOSE2
         fputs("Successfully added peer ", stdlog);
         ST_CLASS(peerListNodePrint)(message->PeerListNodePtr, stdlog, PLPO_FULL);
         fputs("\n", stdlog);
         LOG_END
#if 0
         /* ====== Activate keep alive timer ========================== */
         if(!STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode)) {
            ST_CLASS(peerListActivateTimer)(
               &nameServer->Peers.List,
               peerListNode,
               PLNT_EXPIRY,
               getMicroTime() + nameServer->KeepAliveTransmissionInterval);
         }
         timerRestart(nameServer->NamespaceActionTimer,
                        ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                           &nameServer->Namespace));
#endif
         /* ====== Print debug information ============================ */
         LOG_VERBOSE3
         fputs("Peers:\n", stdlog);
         ST_CLASS(peerListManagementPrint)(&nameServer->Peers, stdlog, PLPO_FULL);
         LOG_END
      }
      else {
         LOG_WARNING
         fputs("Failed to add to peer ", stdlog);
         ST_CLASS(peerListNodePrint)(message->PeerListNodePtr, stdlog, PLPO_FULL);
         fputs(": ", stdlog);
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
   }
}


/* ###### Handle incoming message ######################################## */
static void handleMessage(struct NameServer*  nameServer,
                          struct RSerPoolMessage* message,
                          int                 sd)
{
   switch(message->Type) {
      case AHT_REGISTRATION:
         handleRegistrationRequest(nameServer, sd, message->AssocID, message);
       break;
      case AHT_NAME_RESOLUTION:
         handleNameResolutionRequest(nameServer, sd, message->AssocID, message);
       break;
      case AHT_DEREGISTRATION:
         handleDeregistrationRequest(nameServer, sd, message->AssocID, message);
       break;
      case AHT_ENDPOINT_KEEP_ALIVE_ACK:
         handleEndpointKeepAliveAck(nameServer, sd, message->AssocID, message);
       break;
      case AHT_ENDPOINT_UNREACHABLE:
         handleEndpointUnreachable(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_PRESENCE:
         handlePeerPresence(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_NAME_UPDATE:
         handlePeerNameUpdate(nameServer, sd, message->AssocID, message);
       break;
      default:
         LOG_WARNING
         fprintf(stdlog, "Unsupported message type $%02x\n",
                  message->Type);
         LOG_END
       break;
   }
}


/* ###### Handle events on sockets ####################################### */
static void nameServerSocketCallback(struct Dispatcher* dispatcher,
                                     int                fd,
                                     int                eventMask,
                                     void*              userData)
{
   struct NameServer*       nameServer = (struct NameServer*)userData;
   struct RSerPoolMessage*  message;
   union sctp_notification* notification;
   struct sockaddr_storage  remoteAddress;
   socklen_t                remoteAddressLength;
   char                     buffer[65536];
   int                      flags;
   uint32_t                 ppid;
   sctp_assoc_t             assocID;
   unsigned short           streamID;
   ssize_t                  received;

   if((fd == nameServer->ASAPSocket) ||
      (fd == nameServer->ENRPUnicastSocket) ||
      ((nameServer->ENRPUseMulticast) && (fd == nameServer->ENRPMulticastSocket))) {
      LOG_VERBOSE
      fprintf(stdlog, "Event on socket %d...\n", fd);
      LOG_END

      flags               = 0;
      remoteAddressLength = sizeof(remoteAddress);
      received = recvfromplus(fd,
                              (char*)&buffer, sizeof(buffer), &flags,
                              (struct sockaddr*)&remoteAddress,
                              &remoteAddressLength,
                              &ppid, &assocID, &streamID,
                              0);
      if(received > 0) {
         if(!(flags & MSG_NOTIFICATION)) {
            if(fd == nameServer->ENRPMulticastSocket) {
               /* ENRP via UDP -> Set PPID so that rserpoolPacket2Message can
                  correctly decode the packet */
               ppid = PPID_ENRP;
            }

            message = rserpoolPacket2Message(buffer, ppid, received, sizeof(buffer));
            if(message != NULL) {
               message->BufferAutoDelete = false;
               message->PPID             = ppid;
               message->AssocID          = assocID;
               message->StreamID         = streamID;
               LOG_VERBOSE
               fprintf(stdlog, "Got %u bytes message from ", message->BufferSize);
               fputaddress((struct sockaddr*)&remoteAddress, true, stdlog);
               fprintf(stdlog, ", assoc #%u, PPID $%x\n",
                       message->AssocID, message->PPID);
               LOG_END

               handleMessage(nameServer, message, fd);

               rserpoolMessageDelete(message);
            }
         }
         else {
            notification = (union sctp_notification*)&buffer;
            switch(notification->sn_header.sn_type) {
               case SCTP_ASSOC_CHANGE:
                  if(notification->sn_assoc_change.sac_state == SCTP_COMM_LOST) {
                     LOG_ACTION
                     fprintf(stdlog, "Association communication lost for socket %d, assoc %u\n",
                             nameServer->ASAPSocket,
                             notification->sn_assoc_change.sac_assoc_id);

                     LOG_END
                     nameServerCleanUser(nameServer, fd,
                                         notification->sn_assoc_change.sac_assoc_id);
                  }
                  else if(notification->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP) {
                     LOG_ACTION
                     fprintf(stdlog, "Association shutdown completed for socket %d, assoc %u\n",
                             nameServer->ASAPSocket,
                             notification->sn_assoc_change.sac_assoc_id);

                     LOG_END
                     nameServerCleanUser(nameServer, fd,
                                         notification->sn_assoc_change.sac_assoc_id);
                  }
                break;
               case SCTP_SHUTDOWN_EVENT:
                     LOG_ACTION
                     fprintf(stdlog, "Shutdown event for socket %d, assoc %u\n",
                             nameServer->ASAPSocket,
                             notification->sn_assoc_change.sac_assoc_id);

                     LOG_END
                  nameServerCleanUser(nameServer, fd,
                                      notification->sn_shutdown_event.sse_assoc_id);
                break;
            }
         }
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


unsigned int nameServerAddStaticPeer(
                struct NameServer*                  nameServer,
                const ENRPIdentifierType            identifier,
                const struct TransportAddressBlock* transportAddressBlock)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   int                            result;

   result = ST_CLASS(peerListManagementRegisterPeerListNode)(
               &nameServer->Peers,
               identifier,
               PLNF_STATIC,
               transportAddressBlock,
               getMicroTime(),
               &peerListNode);

   return(result);
}


static void addPeer(struct NameServer* nameServer, char* arg)
{
   char                          transportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* transportAddressBlock = (struct TransportAddressBlock*)&transportAddressBlockBuffer;
   struct sockaddr_storage       addressArray[MAX_NS_TRANSPORTADDRESSES];
   size_t                        addresses;
   char*                         address;
   char*                         idx;

   addresses = 0;
   address = arg;
   while(addresses < MAX_NS_TRANSPORTADDRESSES) {
      idx = index(address, ',');
      if(idx) {
         *idx = 0x00;
      }
      if(!string2address(address, &addressArray[addresses])) {
         printf("ERROR: Bad address %s! Use format <address:port>.\n",address);
         exit(1);
      }
      addresses++;
      if(idx) {
         address = idx;
         address++;
      }
      else {
         break;
      }
   }
   if(addresses < 1) {
      fprintf(stderr, "ERROR: At least one peer address must be specified (at %s)!\n", arg);
      exit(1);
   }
   transportAddressBlockNew(transportAddressBlock,
                            IPPROTO_SCTP,
                            getPort((struct sockaddr*)&addressArray[0]),
                            0,
                            (struct sockaddr_storage*)&addressArray,
                            addresses);
   if(nameServerAddStaticPeer(nameServer, 0, transportAddressBlock) != RSPERR_OKAY) {
      fputs("ERROR: Unable to add static peer ", stderr);
      transportAddressBlockPrint(transportAddressBlock, stderr);
      fputs("\n", stderr);
      exit(1);
   }
}


static int getSCTPSocket(char* arg, struct TransportAddressBlock* sctpAddress)
{
   struct sockaddr_storage  sctpAddressArray[MAX_NS_TRANSPORTADDRESSES];
   struct sockaddr_storage* localAddressArray;
   char*                    address;
   char*                    idx;
   sctp_event_subscribe     sctpEvents;
   size_t                   sctpAddresses;
   int                      sctpSocket;
   int                      autoCloseTimeout;
   int                      sockFD;
   size_t                   i;

    sctpAddresses = 0;
    if(!(strncasecmp(arg, "auto", 4))) {
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

          for(i = 0; i < sctpAddresses;i++) {
             setPort((struct sockaddr*)&sctpAddressArray[i], 0);
          }
       }
       else {
          puts("ERROR: SCTP is unavailable. Install SCTP!");
          exit(1);
       }
    }
    else {
       address = arg;
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
       bzero(&sctpEvents, sizeof(sctpEvents));
       sctpEvents.sctp_data_io_event          = 1;
       sctpEvents.sctp_association_event      = 1;
       sctpEvents.sctp_address_event          = 1;
       sctpEvents.sctp_send_failure_event     = 1;
       sctpEvents.sctp_peer_error_event       = 1;
       sctpEvents.sctp_shutdown_event         = 1;
       sctpEvents.sctp_partial_delivery_event = 1;
       sctpEvents.sctp_adaption_layer_event   = 1;
       if(ext_setsockopt(sctpSocket, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
          perror("setsockopt() for SCTP_EVENTS failed");
          exit(1);
       }
       autoCloseTimeout = 2 * (NAMESERVER_DEFAULT_KEEP_ALIVE_TRANSMISSION_INTERVAL / 1000000);
       if(ext_setsockopt(sctpSocket, IPPROTO_SCTP, SCTP_AUTOCLOSE, &autoCloseTimeout, sizeof(autoCloseTimeout)) < 0) {
          perror("setsockopt() for SCTP_AUTOCLOSE failed");
          exit(1);
       }

       transportAddressBlockNew(sctpAddress,
                                IPPROTO_SCTP,
                                getPort((struct sockaddr*)&sctpAddressArray[0]),
                                0,
                                (struct sockaddr_storage*)&sctpAddressArray,
                                sctpAddresses);
    }
    else {
       puts("ERROR: Unable to create SCTP socket!");
    }
    return(sctpSocket);
}



/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct NameServer*            nameServer;
   uint32_t                      serverID   = 0;
   int                           asapSocket = -1;
   char                          asapAddressBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* asapAddress = (struct TransportAddressBlock*)&asapAddressBuffer;
   struct sockaddr_storage       asapAnnounceAddress;
   bool                          asapSendAnnounces = false;
   int                           enrpUnicastSocket = -1;
   char                          enrpUnicastAddressBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* enrpUnicastAddress = (struct TransportAddressBlock*)&enrpUnicastAddressBuffer;
   struct sockaddr_storage       enrpMulticastAddress;
   bool                          enrpUseMulticast = false;
   int                           result;
   struct timeval                timeout;
   fd_set                        readfdset;
   fd_set                        writefdset;
   fd_set                        exceptfdset;
   fd_set                        testfdset;
   card64                        testTS;
   int                           i, n;


   /* ====== Get arguments =============================================== */
   for(i = 1;i < argc;i++) {
      if(!(strncasecmp(argv[i], "-tcp=",5))) {
         fputs("ERROR: TCP mapping is not supported -> Use SCTP!", stderr);
         exit(1);
      }
      else if(!(strncasecmp(argv[i], "-asap=",6))) {
         if(asapSocket < 0) {
            asapSocket = getSCTPSocket((char*)&argv[i][6], asapAddress);
         }
         else {
            fputs("ERROR: ASAP address given twice!", stdlog);
            exit(1);
         }
      }
      else if(!(strncasecmp(argv[i], "-serverid=", 10))) {
         serverID = atol((char*)&argv[i][10]);
      }
      else if(!(strncasecmp(argv[i], "-peer=",6))) {
         /* to be handled later */
      }
      else if(!(strcasecmp(argv[i], "-asapannounce=auto"))) {
         string2address("239.0.0.1:3863", &asapAnnounceAddress);
         asapSendAnnounces = true;
      }
      else if(!(strncasecmp(argv[i], "-asapannounce=", 14))) {
         if(!string2address((char*)&argv[i][14], &asapAnnounceAddress)) {
            fprintf(stderr,
                    "ERROR: Bad ASAP announce address %s! Use format <address:port>.\n",
                    (char*)&argv[i][10]);
            exit(1);
         }
         asapSendAnnounces = true;
      }
      else if(!(strncasecmp(argv[i], "-enrp=",6))) {
         if(enrpUnicastSocket < 0) {
            enrpUnicastSocket = getSCTPSocket((char*)&argv[i][6], enrpUnicastAddress);
         }
         else {
            fputs("ERROR: ENRP unicast address given twice!", stdlog);
            exit(1);
         }
      }
      else if(!(strcasecmp(argv[i], "-enrpmulticast=auto"))) {
         string2address("239.0.0.1:3864", &enrpMulticastAddress);
         enrpUseMulticast = true;
      }
      else if(!(strncasecmp(argv[i], "-enrpmulticast=", 15))) {
         if(!string2address((char*)&argv[i][15], &enrpMulticastAddress)) {
            fprintf(stderr,
                    "ERROR: Bad ENRP announce address %s! Use format <address:port>.\n",
                    (char*)&argv[i][10]);
            exit(1);
         }
         enrpUseMulticast = true;
      }
      else if(!(strncmp(argv[i], "-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else {
         printf("Usage: %s {-asap=auto|address:port{,address}...} {[-asapannounce=address:port}]} {-enrp=auto|address:port{,address}...} {[-enrpmulticast=address:port}]} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}\n",argv[0]);
         exit(1);
      }
   }
   if(asapSocket <= 0) {
      fprintf(stderr, "ERROR: No ASAP socket opened. Use parameter \"-asap=...\"!\n");
      exit(1);
   }
   if(enrpUnicastSocket <= 0) {
      fprintf(stderr, "ERROR: No ENRP socket opened. Use parameter \"-enrp=...\"!\n");
      exit(1);
   }


   /* ====== Initialize NameServer ======================================= */
   nameServer = nameServerNew(serverID,
                              asapSocket, asapAddress,
                              enrpUnicastSocket, enrpUnicastAddress,
                              asapSendAnnounces, (union sockaddr_union*)&asapAnnounceAddress,
                              enrpUseMulticast, (union sockaddr_union*)&enrpMulticastAddress);
   if(nameServer == NULL) {
      fprintf(stderr, "ERROR: Unable to initialize NameServer object!\n");
      exit(1);
   }
   for(i = 1;i < argc;i++) {
      if(!(strncasecmp(argv[i], "-peer=",6))) {
         addPeer(nameServer, (char*)&argv[i][6]);
      }
   }
#ifndef FAST_BREAK
   installBreakDetector();
#endif

   /* ====== Print information =========================================== */
   puts("The rsplib Name Server - Version 1.00");
   puts("=====================================\n");
   printf("ASAP Address:           ");
   transportAddressBlockPrint(asapAddress, stdout);
   puts("");
   printf("ENRP Address:           ");
   transportAddressBlockPrint(enrpUnicastAddress, stdout);
   puts("");
   printf("ASAP Announce Address:  ");
   if(asapSendAnnounces) {
      fputaddress((struct sockaddr*)&asapAnnounceAddress, true, stdout);
      puts("");
   }
   else {
      puts("(none)");
   }
   printf("ENRP Multicast Address: ");
   if(enrpUseMulticast) {
      fputaddress((struct sockaddr*)&enrpMulticastAddress, true, stdout);
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
