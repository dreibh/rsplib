/*
 *  $Id: nameserver.c,v 1.31 2004/08/30 08:32:41 dreibh Exp $
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

#define MAX_NS_TRANSPORTADDRESSES                                           16
#define NAMESERVER_DEFAULT_MAX_BAD_PE_REPORTS                                3
#define NAMESERVER_DEFAULT_SERVER_ANNOUNCE_CYCLE                       2111111
#define NAMESERVER_DEFAULT_ENDPOINT_MONITORING_HEARTBEAT_INTERVAL      1000000   // ???????
#define NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL   5000000
#define NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TIMEOUT_INTERVAL        5000000   // ???????
#define NAMESERVER_DEFAULT_PEER_HEARTBEAT_CYCLE                        2444444
#define NAMESERVER_DEFAULT_PEER_MAX_TIME_LAST_HEARD                    5000000
#define NAMESERVER_DEFAULT_PEER_MAX_TIME_NO_RESPONSE                   2000000
#define NAMESERVER_DEFAULT_MENTOR_HUNT_TIMEOUT                         5000000
#define NAMESERVER_DEFAULT_TAKEOVER_EXPIRY_INTERVAL                    5000000






struct TakeoverProcess
{
   struct STN_CLASSNAME IndexStorageNode;
   struct STN_CLASSNAME TimerStorageNode;

   ENRPIdentifierType   TargetID;
   unsigned long long   ExpiryTimeStamp;

   size_t               OutstandingAcknowledgements;
   ENRPIdentifierType   PeerIDArray[0];
};


/* ###### Get takeover process from index storage node ################### */
inline struct TakeoverProcess* getTakeoverProcessFromIndexStorageNode(struct STN_CLASSNAME* node)
{
   const struct TakeoverProcess* dummy = (struct TakeoverProcess*)node;
   long n = (long)node - ((long)&dummy->IndexStorageNode - (long)dummy);
   return((struct TakeoverProcess*)n);
}


/* ###### Get takeover process from timer storage node ################### */
inline struct TakeoverProcess* getTakeoverProcessFromTimerStorageNode(struct STN_CLASSNAME* node)
{
   const struct TakeoverProcess* dummy = (struct TakeoverProcess*)node;
   long n = (long)node - ((long)&dummy->TimerStorageNode - (long)dummy);
   return((struct TakeoverProcess*)n);
}


/* ###### Print ########################################################## */
void takeoverProcessIndexPrint(const void* takeoverProcessPtr,
                               FILE*       fd)
{
   size_t i;

   struct TakeoverProcess* takeoverProcess = (struct TakeoverProcess*)takeoverProcessPtr;
   fprintf(fd, "   - Takeover for $%08x (expiry in %Ldµs)\n",
           takeoverProcess->TargetID,
           (long long)takeoverProcess->ExpiryTimeStamp - (long long)getMicroTime());
   for(i = 0;i < takeoverProcess->OutstandingAcknowledgements;i++) {
      fprintf(fd, "      * Ack required by $%08x\n", takeoverProcess->PeerIDArray[i]);
   }
}


/* ###### Comparison ##################################################### */
int takeoverProcessIndexComparison(const void* takeoverProcessPtr1,
                                   const void* takeoverProcessPtr2)
{
   const struct TakeoverProcess* takeoverProcess1 = getTakeoverProcessFromIndexStorageNode((struct STN_CLASSNAME*)takeoverProcessPtr1);
   const struct TakeoverProcess* takeoverProcess2 = getTakeoverProcessFromIndexStorageNode((struct STN_CLASSNAME*)takeoverProcessPtr2);

   if(takeoverProcess1->TargetID < takeoverProcess2->TargetID) {
      return(-1);
   }
   else if(takeoverProcess1->TargetID > takeoverProcess2->TargetID) {
      return(1);
   }
   return(0);
}


/* ###### Comparison ##################################################### */
int takeoverProcessTimerComparison(const void* takeoverProcessPtr1,
                                   const void* takeoverProcessPtr2)
{
   const struct TakeoverProcess* takeoverProcess1 = getTakeoverProcessFromTimerStorageNode((struct STN_CLASSNAME*)takeoverProcessPtr1);
   const struct TakeoverProcess* takeoverProcess2 = getTakeoverProcessFromTimerStorageNode((struct STN_CLASSNAME*)takeoverProcessPtr2);

   if(takeoverProcess1->ExpiryTimeStamp < takeoverProcess2->ExpiryTimeStamp) {
      return(-1);
   }
   else if(takeoverProcess1->ExpiryTimeStamp > takeoverProcess2->ExpiryTimeStamp) {
      return(1);
   }
   return(0);
}




struct TakeoverProcessList
{
   struct ST_CLASSNAME TakeoverProcessIndexStorage;
   struct ST_CLASSNAME TakeoverProcessTimerStorage;
};


/* ###### Initialize ##################################################### */
void takeoverProcessListNew(struct TakeoverProcessList* takeoverProcessList)
{
   ST_METHOD(New)(&takeoverProcessList->TakeoverProcessIndexStorage,
                  takeoverProcessIndexPrint,
                  takeoverProcessIndexComparison);
   ST_METHOD(New)(&takeoverProcessList->TakeoverProcessTimerStorage,
                  NULL,
                  takeoverProcessTimerComparison);
}


/* ###### Invalidate takeover process list ############################### */
void takeoverProcessListDelete(struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessIndexStorage);
   while(node != NULL) {
      ST_METHOD(Remove)(&takeoverProcessList->TakeoverProcessIndexStorage, node);
      free(node);
      node = ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessIndexStorage);
   }
   ST_METHOD(Delete)(&takeoverProcessList->TakeoverProcessIndexStorage);
   ST_METHOD(Delete)(&takeoverProcessList->TakeoverProcessTimerStorage);
}


/* ###### Print takeover process list #################################### */
void takeoverProcessListPrint(struct TakeoverProcessList* takeoverProcessList,
                              FILE*                       fd)
{
   fprintf(fd, "Takeover Process List: (%u entries)\n",
           ST_METHOD(GetElements)(&takeoverProcessList->TakeoverProcessIndexStorage));
   ST_METHOD(Print)(&takeoverProcessList->TakeoverProcessIndexStorage, fd);
}


/* ###### Get next takeover process's expiry time stamp ################## */
inline unsigned long long takeoverProcessListGetNextTimerTimeStamp(
                             struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessTimerStorage);
   if(node) {
      return(getTakeoverProcessFromTimerStorageNode(node)->ExpiryTimeStamp);
   }
   return(~0);
}


/* ###### Get earliest takeover process from list ######################## */
inline struct TakeoverProcess* takeoverProcessListGetEarliestTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessTimerStorage);
   if(node) {
      return(getTakeoverProcessFromTimerStorageNode(node));
   }
   return(NULL);
}


/* ###### Get first takeover process from list ########################### */
inline struct TakeoverProcess* takeoverProcessListGetFirstTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessIndexStorage);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Get last takeover process from list ############################ */
inline struct TakeoverProcess* takeoverProcessListGetLastTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetLast)(&takeoverProcessList->TakeoverProcessIndexStorage);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Get next takeover process from list ############################ */
inline struct TakeoverProcess* takeoverProcessListGetNextTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList,
                                  struct TakeoverProcess*     takeoverProcess)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetNext)(&takeoverProcessList->TakeoverProcessIndexStorage,
                         &takeoverProcess->IndexStorageNode);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Get previous takeover process from list ######################## */
inline struct TakeoverProcess* takeoverProcessListGetPrevTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList,
                                  struct TakeoverProcess*     takeoverProcess)
{
   struct STN_CLASSNAME* node =
      ST_METHOD(GetPrev)(&takeoverProcessList->TakeoverProcessIndexStorage,
                         &takeoverProcess->IndexStorageNode);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Find takeover process ########################################## */
struct TakeoverProcess* takeoverProcessListFindTakeoverProcess(
                           struct TakeoverProcessList* takeoverProcessList,
                           const ENRPIdentifierType    targetID)
{
   struct STN_CLASSNAME*  node;
   struct TakeoverProcess cmpTakeoverProcess;

   cmpTakeoverProcess.TargetID = targetID;
   node = ST_METHOD(Find)(&takeoverProcessList->TakeoverProcessIndexStorage,
                          &cmpTakeoverProcess.IndexStorageNode);
   if(node) {
      return(getTakeoverProcessFromIndexStorageNode(node));
   }
   return(NULL);
}


/* ###### Create takeover process ######################################## */
unsigned int takeoverProcessListCreateTakeoverProcess(
                struct TakeoverProcessList*          takeoverProcessList,
                const ENRPIdentifierType             targetID,
                struct ST_CLASS(PeerListManagement)* peerList,
                const unsigned long long             expiryTimeStamp)
{
   struct TakeoverProcess*        takeoverProcess;
   struct ST_CLASS(PeerListNode)* peerListNode;
   const size_t                   peers = ST_CLASS(peerListManagementGetPeerListNodes)(peerList);

   CHECK(targetID != 0);
   CHECK(targetID != peerList->List.OwnIdentifier);
   if(takeoverProcessListFindTakeoverProcess(takeoverProcessList, targetID)) {
      return(RSPERR_DUPLICATE_ID);
   }

   takeoverProcess = (struct TakeoverProcess*)malloc(sizeof(struct TakeoverProcess) +
                                                     sizeof(ENRPIdentifierType) * peers);
   if(takeoverProcess == NULL) {
      return(RSPERR_OUT_OF_MEMORY);
   }

   STN_METHOD(New)(&takeoverProcess->IndexStorageNode);
   STN_METHOD(New)(&takeoverProcess->TimerStorageNode);
   takeoverProcess->TargetID                    = targetID;
   takeoverProcess->ExpiryTimeStamp             = expiryTimeStamp;
   takeoverProcess->OutstandingAcknowledgements = 0;
   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&peerList->List);
   while(peerListNode != NULL) {
      if((peerListNode->Identifier != targetID) &&
         (peerListNode->Identifier != peerList->List.OwnIdentifier) &&
         (peerListNode->Identifier != 0)) {
         takeoverProcess->PeerIDArray[takeoverProcess->OutstandingAcknowledgements++] =
            peerListNode->Identifier;
      }
      peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(&peerList->List, peerListNode);
   }

   ST_METHOD(Insert)(&takeoverProcessList->TakeoverProcessIndexStorage,
                     &takeoverProcess->IndexStorageNode);
   ST_METHOD(Insert)(&takeoverProcessList->TakeoverProcessTimerStorage,
                     &takeoverProcess->TimerStorageNode);
   return(RSPERR_OKAY);
}


/* ###### Acknowledge takeover process ################################### */
struct TakeoverProcess* takeoverProcessListAcknowledgeTakeoverProcess(
                           struct TakeoverProcessList* takeoverProcessList,
                           const ENRPIdentifierType    targetID,
                           const ENRPIdentifierType    acknowledgerID)
{
   size_t                  i;
   struct TakeoverProcess* takeoverProcess =
      takeoverProcessListFindTakeoverProcess(takeoverProcessList,
                                             targetID);
   if(takeoverProcess != NULL) {
      for(i = 0;i < takeoverProcess->OutstandingAcknowledgements;i++) {
         if(takeoverProcess->PeerIDArray[i] == acknowledgerID) {
            for(   ;i < takeoverProcess->OutstandingAcknowledgements - 1;i++) {
               takeoverProcess->PeerIDArray[i] = takeoverProcess->PeerIDArray[i + 1];
            }
            takeoverProcess->OutstandingAcknowledgements--;
            return(takeoverProcess);
         }
      }
   }
   return(NULL);
}


/* ###### Delete takeover process ######################################## */
void takeoverProcessListDeleteTakeoverProcess(
        struct TakeoverProcessList* takeoverProcessList,
        struct TakeoverProcess*     takeoverProcess)
{
   ST_METHOD(Remove)(&takeoverProcessList->TakeoverProcessIndexStorage,
                     &takeoverProcess->IndexStorageNode);
   ST_METHOD(Remove)(&takeoverProcessList->TakeoverProcessTimerStorage,
                     &takeoverProcess->TimerStorageNode);
   STN_METHOD(Delete)(&takeoverProcess->IndexStorageNode);
   STN_METHOD(Delete)(&takeoverProcess->TimerStorageNode);
   takeoverProcess->TargetID                    = 0;
   takeoverProcess->OutstandingAcknowledgements = 0;
   free(takeoverProcess);
}







struct NameServer
{
   ENRPIdentifierType                       ServerID;

   struct Dispatcher                        StateMachine;
   struct ST_CLASS(PoolNamespaceManagement) Namespace;
   struct ST_CLASS(PeerListManagement)      Peers;
   struct Timer                             NamespaceActionTimer;
   struct Timer                             PeerActionTimer;

   int                                      ASAPAnnounceSocket;
   struct FDCallback                        ASAPSocketFDCallback;
   union sockaddr_union                     ASAPAnnounceAddress;
   bool                                     ASAPSendAnnounces;
   int                                      ASAPSocket;
   struct TransportAddressBlock*            ASAPAddress;
   struct Timer                             ASAPAnnounceTimer;

   int                                      ENRPMulticastSocket;
   struct FDCallback                        ENRPMulticastSocketFDCallback;
   union sockaddr_union                     ENRPMulticastAddress;
   int                                      ENRPUnicastSocket;
   struct FDCallback                        ENRPUnicastSocketFDCallback;
   struct TransportAddressBlock*            ENRPUnicastAddress;
   bool                                     ENRPAnnounceViaMulticast;
   bool                                     ENRPUseMulticast;
   struct Timer                             ENRPAnnounceTimer;

   bool                                     InInitializationPhase;
   ENRPIdentifierType                       MentorServerID;

   struct TakeoverProcessList               Takeovers;
   struct Timer                             TakeoverExpiryTimer;

   size_t                                   MaxBadPEReports;
   unsigned long long                       ServerAnnounceCycle;
   unsigned long long                       EndpointMonitoringHeartbeatInterval;
   unsigned long long                       EndpointKeepAliveTransmissionInterval;
   unsigned long long                       EndpointKeepAliveTimeoutInterval;
   unsigned long long                       PeerHeartbeatCycle;
   unsigned long long                       PeerMaxTimeLastHeard;
   unsigned long long                       PeerMaxTimeNoResponse;
   unsigned long long                       MentorHuntTimeout;
   unsigned long long                       TakeoverExpiryInterval;
};


void nameServerDumpNamespace(struct NameServer* nameServer);

void nameServerDumpPeers(struct NameServer* nameServer)
{
   fputs("*************** Peers ********************\n", stdlog);
   ST_CLASS(peerListManagementPrint)(&nameServer->Peers, stdlog, PLPO_FULL);
   fputs("******************************************\n", stdlog);

}

struct NameServer* nameServerNew(const ENRPIdentifierType      serverID,
                                 int                           asapSocket,
                                 struct TransportAddressBlock* asapAddress,
                                 int                           enrpUnicastSocket,
                                 struct TransportAddressBlock* enrpUnicastAddress,
                                 const bool                    asapSendAnnounces,
                                 const union sockaddr_union*   asapAnnounceAddress,
                                 const bool                    enrpUseMulticast,
                                 const bool                    enrpAnnounceViaMulticast,
                                 const union sockaddr_union*   enrpMulticastAddress);
void nameServerDelete(struct NameServer* nameServer);

static void sendPeerNameUpdate(struct NameServer*                nameServer,
                               struct ST_CLASS(PoolElementNode)* poolElementNode,
                               const uint16_t                    action);
static void sendPeerInitTakeover(struct NameServer*       nameServer,
                                 const ENRPIdentifierType targetID);
static void sendPeerInitTakeoverAck(struct NameServer*       nameServer,
                                    const int                sd,
                                    const sctp_assoc_t       assocID,
                                    const ENRPIdentifierType receiverID,
                                    const ENRPIdentifierType targetID);
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
static void takeoverExpiryTimerCallback(struct Dispatcher* dispatcher,
                                        struct Timer*      timer,
                                        void*              userData);
static void nameServerSocketCallback(struct Dispatcher* dispatcher,
                                     int                fd,
                                     unsigned int       eventMask,
                                     void*              userData);


static void poolElementNodeDisposer(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                    void*                             userData)
{
}

static void peerListNodeDisposer(struct ST_CLASS(PeerListNode)* peerListNode,
                                 void*                          userData)
{
   // struct NameServer* nameServer = (struct NameServer*)userData;
   if(peerListNode->UserData) {
      /* A peer name table request state is still saved. Free its memory. */
      free(peerListNode->UserData);
      peerListNode->UserData = NULL;
   }
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
                                 const bool                    enrpAnnounceViaMulticast,
                                 const union sockaddr_union*   enrpMulticastAddress)
{
   struct NameServer* nameServer = (struct NameServer*)malloc(sizeof(struct NameServer));
   if(nameServer != NULL) {
      nameServer->ServerID = serverID;
      if(nameServer->ServerID == 0) {
         nameServer->ServerID = random32();
      }

      dispatcherNew(&nameServer->StateMachine,
                    dispatcherDefaultLock, dispatcherDefaultUnlock, NULL);
      ST_CLASS(poolNamespaceManagementNew)(&nameServer->Namespace,
                                           nameServer->ServerID,
                                           NULL,
                                           poolElementNodeDisposer,
                                           nameServer);
      ST_CLASS(peerListManagementNew)(&nameServer->Peers,
                                      nameServer->ServerID,
                                      peerListNodeDisposer,
                                      nameServer);
      timerNew(&nameServer->ASAPAnnounceTimer,
               &nameServer->StateMachine,
               asapAnnounceTimerCallback,
               (void*)nameServer);
      timerNew(&nameServer->ENRPAnnounceTimer,
               &nameServer->StateMachine,
               enrpAnnounceTimerCallback,
               (void*)nameServer);
      timerNew(&nameServer->NamespaceActionTimer,
               &nameServer->StateMachine,
               namespaceActionTimerCallback,
               (void*)nameServer);
      timerNew(&nameServer->PeerActionTimer,
               &nameServer->StateMachine,
               peerActionTimerCallback,
               (void*)nameServer);
      timerNew(&nameServer->TakeoverExpiryTimer,
               &nameServer->StateMachine,
               takeoverExpiryTimerCallback,
               (void*)nameServer);
      takeoverProcessListNew(&nameServer->Takeovers);

      nameServer->InInitializationPhase    = true;
      nameServer->MentorServerID           = 0;
      nameServer->ASAPSocket               = asapSocket;
      nameServer->ASAPAddress              = asapAddress;
      nameServer->ASAPSendAnnounces        = asapSendAnnounces;
      nameServer->ENRPUnicastSocket        = enrpUnicastSocket;
      nameServer->ENRPUnicastAddress       = enrpUnicastAddress;
      nameServer->ENRPMulticastSocket      = -1;
      nameServer->ENRPUseMulticast         = enrpUseMulticast;
      nameServer->ENRPAnnounceViaMulticast = enrpAnnounceViaMulticast;

      nameServer->MaxBadPEReports                       = NAMESERVER_DEFAULT_MAX_BAD_PE_REPORTS;
      nameServer->ServerAnnounceCycle                   = NAMESERVER_DEFAULT_SERVER_ANNOUNCE_CYCLE;
      nameServer->EndpointMonitoringHeartbeatInterval   = NAMESERVER_DEFAULT_ENDPOINT_MONITORING_HEARTBEAT_INTERVAL;
      nameServer->EndpointKeepAliveTransmissionInterval = NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL;
      nameServer->EndpointKeepAliveTimeoutInterval      = NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TIMEOUT_INTERVAL;
      nameServer->PeerMaxTimeLastHeard                  = NAMESERVER_DEFAULT_PEER_MAX_TIME_LAST_HEARD;
      nameServer->PeerMaxTimeNoResponse                 = NAMESERVER_DEFAULT_PEER_MAX_TIME_NO_RESPONSE;
      nameServer->PeerHeartbeatCycle                    = NAMESERVER_DEFAULT_PEER_HEARTBEAT_CYCLE;
      nameServer->MentorHuntTimeout                     = NAMESERVER_DEFAULT_MENTOR_HUNT_TIMEOUT;
      nameServer->TakeoverExpiryInterval                = NAMESERVER_DEFAULT_TAKEOVER_EXPIRY_INTERVAL;

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

      fdCallbackNew(&nameServer->ASAPSocketFDCallback,
                    &nameServer->StateMachine,
                    nameServer->ASAPSocket,
                    FDCE_Read|FDCE_Exception,
                    nameServerSocketCallback,
                    (void*)nameServer);
      fdCallbackNew(&nameServer->ENRPUnicastSocketFDCallback,
                    &nameServer->StateMachine,
                    nameServer->ENRPUnicastSocket,
                    FDCE_Read|FDCE_Exception,
                    nameServerSocketCallback,
                    (void*)nameServer);

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
         fdCallbackNew(&nameServer->ENRPMulticastSocketFDCallback,
                       &nameServer->StateMachine,
                       nameServer->ENRPMulticastSocket,
                       FDCE_Read|FDCE_Exception,
                       nameServerSocketCallback,
                       (void*)nameServer);
      }

      timerStart(&nameServer->ENRPAnnounceTimer, getMicroTime() + nameServer->MentorHuntTimeout);
      if((nameServer->ENRPUseMulticast) || (nameServer->ENRPAnnounceViaMulticast)) {
         if(joinOrLeaveMulticastGroup(nameServer->ENRPMulticastSocket,
                                      &nameServer->ENRPMulticastAddress,
                                      true) == false) {
            LOG_ERROR
            logerror("Unable to join ENRP multicast group");
            LOG_END
            nameServerDelete(nameServer);
            return(NULL);
         }
      }
   }

   return(nameServer);
}


/* ###### Destructor ###################################################### */
void nameServerDelete(struct NameServer* nameServer)
{
   if(nameServer) {
      fdCallbackDelete(&nameServer->ENRPUnicastSocketFDCallback);
      fdCallbackDelete(&nameServer->ASAPSocketFDCallback);
      ST_CLASS(peerListManagementClear)(&nameServer->Peers);
      ST_CLASS(peerListManagementDelete)(&nameServer->Peers);
      ST_CLASS(poolNamespaceManagementClear)(&nameServer->Namespace);
      ST_CLASS(poolNamespaceManagementDelete)(&nameServer->Namespace);
      timerDelete(&nameServer->TakeoverExpiryTimer);
      timerDelete(&nameServer->ASAPAnnounceTimer);
      timerDelete(&nameServer->ENRPAnnounceTimer);
      timerDelete(&nameServer->NamespaceActionTimer);
      timerDelete(&nameServer->PeerActionTimer);
      takeoverProcessListDelete(&nameServer->Takeovers);
      if(nameServer->ENRPMulticastSocket >= 0) {
         fdCallbackDelete(&nameServer->ENRPMulticastSocketFDCallback);
         ext_close(nameServer->ENRPMulticastSocket >= 0);
         nameServer->ENRPMulticastSocket = -1;
      }
      if(nameServer->ENRPUnicastSocket >= 0) {
         ext_close(nameServer->ENRPUnicastSocket >= 0);
         nameServer->ENRPUnicastSocket = -1;
      }
      if(nameServer->ASAPAnnounceSocket >= 0) {
         ext_close(nameServer->ASAPAnnounceSocket >= 0);
         nameServer->ASAPAnnounceSocket = -1;
      }
      dispatcherDelete(&nameServer->StateMachine);
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


/* ###### ASAP announcement timer callback ############################### */
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
   timerStart(timer, getMicroTime() + nameServer->ServerAnnounceCycle);
}


/* ###### Remove all PEs registered via a given connection ############### */
static void nameServerRemovePoolElementsOfConnection(struct NameServer* nameServer,
                                                     const int          sd,
                                                     const sctp_assoc_t assocID)
{
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode =
      ST_CLASS(poolNamespaceNodeGetFirstPoolElementConnectionNodeForConnection)(
         &nameServer->Namespace.Namespace,
         sd, assocID);
   unsigned int                      result;

   if(poolElementNode) {
      LOG_ACTION
      fprintf(stdlog,
              "Removing all pool elements registered by user socket %u, assoc %u...\n",
              sd, assocID);
      LOG_END

      do {
         nextPoolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(
                                  &nameServer->Namespace.Namespace,
                                  poolElementNode);
         LOG_VERBOSE
         fprintf(stdlog, "Removing pool element $%08x of pool ",
                 poolElementNode->Identifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
         fputs("\n", stdlog);
         LOG_END

         if(poolElementNode->HomeNSIdentifier == nameServer->ServerID) {
            /* We own this PE -> send PeerNameUpdate for its removal. */
            sendPeerNameUpdate(nameServer, poolElementNode, PNUP_DEL_PE);
         }

         result = ST_CLASS(poolNamespaceManagementDeregisterPoolElementByPtr)(
                     &nameServer->Namespace,
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
      fputs("Namespace content:\n", stdlog);
      nameServerDumpNamespace(nameServer);
      LOG_END
   }
}


/* ###### Send ENRP peer presence message ################################ */
static void sendPeerPresence(struct NameServer*          nameServer,
                             int                         sd,
                             const sctp_assoc_t          assocID,
                             int                         msgSendFlags,
                             const union sockaddr_union* destinationAddressList,
                             const size_t                destinationAddresses,
                             const ENRPIdentifierType    receiverID,
                             const bool                  replyRequired)
{
   struct RSerPoolMessage* message;
   ST_CLASS(PeerListNode)  peerListNode;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type                      = EHT_PEER_PRESENCE;
      message->PPID                      = PPID_ENRP;
      message->AssocID                   = assocID;
      message->AddressArray              = (union sockaddr_union*)destinationAddressList;
      message->Addresses                 = destinationAddresses;
      message->Flags                     = replyRequired ? EHF_PEER_PRESENCE_REPLY_REQUIRED : 0x00;
      message->PeerListNodePtr           = &peerListNode;
      message->PeerListNodePtrAutoDelete = false;
      message->SenderID                  = nameServer->ServerID;
      message->ReceiverID                = receiverID;

      ST_CLASS(peerListNodeNew)(&peerListNode,
                                nameServer->ServerID,
                                nameServer->ENRPUseMulticast ? PLNF_MULTICAST : 0,
                                nameServer->ENRPUnicastAddress);
      if(rserpoolMessageSend((sd == nameServer->ENRPMulticastSocket) ? IPPROTO_UDP : IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, message) == false) {
         LOG_WARNING
         fputs("Sending PeerPresence failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Send ENRP peer list request message ############################ */
static void sendPeerListRequest(struct NameServer*          nameServer,
                                int                         sd,
                                const sctp_assoc_t          assocID,
                                int                         msgSendFlags,
                                const union sockaddr_union* destinationAddressList,
                                const size_t                destinationAddresses,
                                ENRPIdentifierType          receiverID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type         = EHT_PEER_LIST_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = assocID;
      message->AddressArray = (union sockaddr_union*)destinationAddressList;
      message->Addresses    = destinationAddresses;
      message->Flags        = 0x00;
      message->SenderID     = nameServer->ServerID;
      message->ReceiverID   = receiverID;
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, message) == false) {
         LOG_WARNING
         fputs("Sending PeerListRequest failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Send ENRP peer name table request message ###################### */
static void sendPeerNameTableRequest(struct NameServer*          nameServer,
                                     int                         sd,
                                     const sctp_assoc_t          assocID,
                                     int                         msgSendFlags,
                                     const union sockaddr_union* destinationAddressList,
                                     const size_t                destinationAddresses,
                                     ENRPIdentifierType          receiverID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type         = EHT_PEER_NAME_TABLE_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = assocID;
      message->AddressArray = (union sockaddr_union*)destinationAddressList;
      message->Addresses    = destinationAddresses;
      message->Flags        = 0x00;
      message->SenderID     = nameServer->ServerID;
      message->ReceiverID   = receiverID;
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, message) == false) {
         LOG_WARNING
         fputs("Sending PeerNameTableRequest failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


static void nameServerInitializationComplete(struct NameServer* nameServer)
{
   nameServer->InInitializationPhase = false;
   if(nameServer->ASAPSendAnnounces) {
      LOG_ACTION
      fputs("Starting to send ASAP announcements\n", stdlog);
      LOG_END
      timerStart(&nameServer->ASAPAnnounceTimer, 0);
   }
}


/* ###### ENRP peer presence timer callback ############################## */
static void enrpAnnounceTimerCallback(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData)
{
   struct NameServer*             nameServer = (struct NameServer*)userData;
   struct ST_CLASS(PeerListNode)* peerListNode;

   if(nameServer->InInitializationPhase) {
      LOG_NOTE
      fputs("Initialization phase ended after ENRP mentor discovery timeout. The name server is ready!\n", stdlog);
      LOG_END
      nameServerInitializationComplete(nameServer);
   }

   if((nameServer->ENRPUseMulticast) || (nameServer->ENRPAnnounceViaMulticast)) {
      sendPeerPresence(nameServer, nameServer->ENRPMulticastSocket, 0, 0,
                       (union sockaddr_union*)&nameServer->ENRPMulticastAddress, 1,
                       0, false);
   }

   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&nameServer->Peers.List);
   while(peerListNode != NULL) {
      if(!(peerListNode->Flags & PLNF_MULTICAST)) {
         LOG_VERBOSE2
         fprintf(stdlog, "Sending PeerPresence to unicast peer $%08x...\n",
                  peerListNode->Identifier);
         LOG_END
         sendPeerPresence(nameServer,
                          nameServer->ENRPUnicastSocket, 0, 0,
                          peerListNode->AddressBlock->AddressArray,
                          peerListNode->AddressBlock->Addresses,
                          peerListNode->Identifier,
                          false);
      }
      peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                        &nameServer->Peers.List,
                        peerListNode);
   }

   timerStart(timer, getMicroTime() + nameServer->PeerHeartbeatCycle);
}


/* ###### Takeover expiry timer callback ################################# */
static void takeoverExpiryTimerCallback(struct Dispatcher* dispatcher,
                                        struct Timer*      timer,
                                        void*              userData)
{
   struct NameServer*             nameServer = (struct NameServer*)userData;
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct TakeoverProcess*        takeoverProcess =
      takeoverProcessListGetEarliestTakeoverProcess(&nameServer->Takeovers);
   size_t                         i;

   CHECK(takeoverProcess != NULL);

   LOG_WARNING
   fprintf(stdlog, "Takeover of peer $%08x has expired, %u outstanding acknowledgements:\n",
           takeoverProcess->TargetID,
           takeoverProcess->OutstandingAcknowledgements);
   for(i = 0;i < takeoverProcess->OutstandingAcknowledgements;i++) {
      peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                        &nameServer->Peers,
                        takeoverProcess->PeerIDArray[i],
                        NULL);
      fprintf(stdlog, "- $%08x (%s)\n",
              takeoverProcess->PeerIDArray[i],
              (peerListNode == NULL) ? "not found!" : "still available");
   }
   LOG_END
   takeoverProcessListDeleteTakeoverProcess(&nameServer->Takeovers, takeoverProcess);
}


/* ###### Send Endpoint Keep Alive ####################################### */
static void sendEndpointEndpointKeepAlive(struct NameServer*                nameServer,
                                          struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 1024);
   if(message != NULL) {
      LOG_VERBOSE2
      fprintf(stdlog, "Sending EndpointKeepAlive for pool element $%08x in pool ",
              poolElementNode->Identifier);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END

      message->Handle     = poolElementNode->OwnerPoolNode->Handle;
      message->Identifier = poolElementNode->Identifier;
      message->Type       = AHT_ENDPOINT_KEEP_ALIVE;
      message->Flags      = 0x00;
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             poolElementNode->ConnectionSocketDescriptor,
                             poolElementNode->ConnectionAssocID,
                             0, 0,
                             message) == false) {
         LOG_ERROR
         fprintf(stdlog, "Sending EndpointKeepAlive for pool element $%08x in pool ",
                  poolElementNode->Identifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
         fputs(" failed\n", stdlog);
         LOG_END
         return;
      }

      rserpoolMessageDelete(message);
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

         sendEndpointEndpointKeepAlive(nameServer, poolElementNode);

         ST_CLASS(poolNamespaceNodeActivateTimer)(
            &nameServer->Namespace.Namespace,
            poolElementNode,
            PENT_KEEPALIVE_TIMEOUT,
            getMicroTime() + nameServer->EndpointKeepAliveTimeoutInterval);
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

         if(poolElementNode->HomeNSIdentifier == nameServer->ServerID) {
            /* We own this PE -> send PeerNameUpdate for its removal. */
            sendPeerNameUpdate(nameServer, poolElementNode, PNUP_DEL_PE);
         }

         result = ST_CLASS(poolNamespaceManagementDeregisterPoolElementByPtr)(
                     &nameServer->Namespace,
                     poolElementNode);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Deregistration successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Namespace content:\n", stdlog);
            nameServerDumpNamespace(nameServer);
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

   timerRestart(&nameServer->NamespaceActionTimer,
                ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                   &nameServer->Namespace));
}


/* ###### Handle peer management timers ################################## */
static void peerActionTimerCallback(struct Dispatcher* dispatcher,
                                    struct Timer*      timer,
                                    void*              userData)
{
   struct NameServer*             nameServer = (struct NameServer*)userData;
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct ST_CLASS(PeerListNode)* nextPeerListNode;
   unsigned int                   result;

   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(
                     &nameServer->Peers.List);
   while((peerListNode != NULL) &&
         (peerListNode->TimerTimeStamp <= getMicroTime())) {
      nextPeerListNode = ST_CLASS(peerListGetNextPeerListNodeFromTimerStorage)(
                            &nameServer->Peers.List,
                            peerListNode);

      if(peerListNode->TimerCode == PLNT_MAX_TIME_LAST_HEARD) {
         LOG_ACTION
         fputs("Peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fprintf(stdlog, " not head since MaxTimeLastHeard=%Luµs -> requesting immediate PeerPresence\n",
                 nameServer->PeerMaxTimeLastHeard);
         LOG_END
         sendPeerPresence(nameServer,
                          nameServer->ENRPUnicastSocket,
                          0, 0,
                          peerListNode->AddressBlock->AddressArray,
                          peerListNode->AddressBlock->Addresses,
                          peerListNode->Identifier,
                          true);
         ST_CLASS(peerListDeactivateTimer)(
            &nameServer->Peers.List,
            peerListNode);
         ST_CLASS(peerListActivateTimer)(
            &nameServer->Peers.List,
            peerListNode,
            PLNT_MAX_TIME_NO_RESPONSE,
            getMicroTime() + nameServer->PeerMaxTimeNoResponse);
      }
      else if(peerListNode->TimerCode == PLNT_MAX_TIME_NO_RESPONSE) {
         LOG_ACTION
         fputs("Peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fprintf(stdlog, " did not answer in MaxTimeNoResponse=%Luµs -> removing it\n",
                 nameServer->PeerMaxTimeLastHeard);
         LOG_END

         if(takeoverProcessListCreateTakeoverProcess(
               &nameServer->Takeovers,
               peerListNode->Identifier,
               &nameServer->Peers,
               getMicroTime() + nameServer->TakeoverExpiryInterval) == RSPERR_OKAY) {
            timerRestart(&nameServer->TakeoverExpiryTimer,
                         takeoverProcessListGetNextTimerTimeStamp(&nameServer->Takeovers));
            LOG_ACTION
            fprintf(stdlog, "Initiating takeover of dead peer $%08x\n",
                    peerListNode->Identifier);
            LOG_END
            sendPeerInitTakeover(nameServer, peerListNode->Identifier);
         }

         result = ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                     &nameServer->Peers,
                     peerListNode);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Peer removal successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Peers:\n", stdlog);
            nameServerDumpPeers(nameServer);
            LOG_END
         }
         else {
            LOG_ERROR
            fputs("Failed to remove peer ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
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

      peerListNode = nextPeerListNode;
   }

   timerRestart(&nameServer->PeerActionTimer,
                ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                   &nameServer->Peers));
}


/* ###### Send peer init takeover ######################################## */
static void sendPeerInitTakeover(struct NameServer*       nameServer,
                                 const ENRPIdentifierType targetID)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct RSerPoolMessage*        message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type         = EHT_PEER_INIT_TAKEOVER;
      message->Flags        = 0x00;
      message->SenderID     = nameServer->ServerID;
      message->ReceiverID   = 0;
      message->NSIdentifier = targetID;

      if(nameServer->ENRPUseMulticast) {
         LOG_VERBOSE2
         fputs("Sending PeerInitTakeover via ENRP multicast socket...\n", stdlog);
         LOG_END
         message->ReceiverID    = 0;
         message->AddressArray  = &nameServer->ENRPMulticastAddress;
         message->Addresses     = 1;
         if(rserpoolMessageSend(IPPROTO_UDP,
                                nameServer->ENRPMulticastSocket,
                                0, 0, 0,
                                message) == false) {
            LOG_WARNING
            fputs("Sending PeerNameUpdate via ENRP multicast socket failed\n", stdlog);
            LOG_END
         }
      }

      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&nameServer->Peers.List);
      while(peerListNode != NULL) {
         if(!(peerListNode->Flags & PLNF_MULTICAST)) {
            message->ReceiverID   = peerListNode->Identifier;
            message->AddressArray = peerListNode->AddressBlock->AddressArray;
            message->Addresses    = peerListNode->AddressBlock->Addresses;
            LOG_VERBOSE
            fprintf(stdlog, "Sending PeerInitTakeover to unicast peer $%08x...\n",
                    peerListNode->Identifier);
            LOG_END
            rserpoolMessageSend(IPPROTO_SCTP,
                                nameServer->ENRPUnicastSocket,
                                0, 0, 0,
                                message);
         }
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                           &nameServer->Peers.List,
                           peerListNode);
      }
   }
}


/* ###### Send peer init takeover acknowledgement ######################## */
static void sendPeerInitTakeoverAck(struct NameServer*       nameServer,
                                    const int                sd,
                                    const sctp_assoc_t       assocID,
                                    const ENRPIdentifierType receiverID,
                                    const ENRPIdentifierType targetID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type         = EHT_PEER_INIT_TAKEOVER_ACK;
      message->AssocID      = assocID;
      message->Flags        = 0x00;
      message->SenderID     = nameServer->ServerID;
      message->ReceiverID   = receiverID;
      message->NSIdentifier = targetID;

      LOG_ACTION
      fprintf(stdlog, "Sending PeerInitTakeoverAck for target $%08x to peer $%08x...\n",
              message->NSIdentifier,
              message->ReceiverID);
      LOG_END
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending PeerInitTakeoverAck failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(message);
   }
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
      message->Flags                    = 0x00;
      message->Action                   = action;
      message->SenderID                 = nameServer->ServerID;
      message->Handle                   = poolElementNode->OwnerPoolNode->Handle;
      message->PoolElementPtr           = poolElementNode;
      message->PoolElementPtrAutoDelete = false;

      LOG_VERBOSE
      fputs("Sending PeerNameUpdate for pool element ", stdlog);
      ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL),
      fputs(" of pool ", stdlog);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fprintf(stdlog, ", action $%04x...\n", action);
      LOG_END

      if(nameServer->ENRPUseMulticast) {
         LOG_VERBOSE2
         fputs("Sending PeerNameUpdate via ENRP multicast socket...\n", stdlog);
         LOG_END
         message->ReceiverID    = 0;
         message->AddressArray  = &nameServer->ENRPMulticastAddress;
         message->Addresses     = 1;
         if(rserpoolMessageSend(IPPROTO_UDP,
                                nameServer->ENRPMulticastSocket,
                                0, 0, 0,
                                message) == false) {
            LOG_WARNING
            fputs("Sending PeerNameUpdate via ENRP multicast socket failed\n", stdlog);
            LOG_END
         }
      }

      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&nameServer->Peers.List);
      while(peerListNode != NULL) {
         if(!(peerListNode->Flags & PLNF_MULTICAST)) {
            message->ReceiverID   = peerListNode->Identifier;
            message->AddressArray = peerListNode->AddressBlock->AddressArray;
            message->Addresses    = peerListNode->AddressBlock->Addresses;
            LOG_VERBOSE
            fprintf(stdlog, "Sending PeerNameUpdate to unicast peer $%08x...\n",
                    peerListNode->Identifier);
            LOG_END
            rserpoolMessageSend(IPPROTO_SCTP,
                                nameServer->ENRPUnicastSocket,
                                0, 0, 0,
                                message);
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
                 const union sockaddr_union*         assocAddressArray,
                 const size_t                        assocAddresses,
                 struct TransportAddressBlock*       validAddressBlock)
{
   bool   selectionArray[MAX_PE_TRANSPORTADDRESSES];
   size_t selected = 0;
   size_t i, j;

   for(i = 0;i < sourceAddressBlock->Addresses;i++) {
      selectionArray[i] = false;
      if(getScope((const struct sockaddr*)&sourceAddressBlock->AddressArray[i]) >= 4) {
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
static void handleRegistrationRequest(struct NameServer*      nameServer,
                                      int                     fd,
                                      sctp_assoc_t            assocID,
                                      struct RSerPoolMessage* message)
{
   char                              validAddressBlockBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     validAddressBlock = (struct TransportAddressBlock*)&validAddressBlockBuffer;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   union sockaddr_union*             addressArray;
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
                             fd, assocID,
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

            /* ====== Tune SCTP association ============================== */
            tags[0].Tag = TAG_TuneSCTP_MinRTO;
            tags[0].Data = 500;
            tags[1].Tag = TAG_TuneSCTP_MaxRTO;
            tags[1].Data = 1000;
            tags[2].Tag = TAG_TuneSCTP_InitialRTO;
            tags[2].Data = 750;
            tags[3].Tag = TAG_TuneSCTP_Heartbeat;
            tags[3].Data = (nameServer->EndpointMonitoringHeartbeatInterval / 1000);
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
            if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
               ST_CLASS(poolNamespaceNodeDeactivateTimer)(
                  &nameServer->Namespace.Namespace,
                  poolElementNode);
            }
            ST_CLASS(poolNamespaceNodeActivateTimer)(
               &nameServer->Namespace.Namespace,
               poolElementNode,
               PENT_KEEPALIVE_TRANSMISSION,
               getMicroTime() + nameServer->EndpointKeepAliveTransmissionInterval);
            timerRestart(&nameServer->NamespaceActionTimer,
                         ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                            &nameServer->Namespace));

            /* ====== Print debug information ============================ */
            LOG_VERBOSE3
            fputs("Namespace content:\n", stdlog);
            nameServerDumpNamespace(nameServer);
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

      if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, message) == false) {
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
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)  delPoolElementNode;
   struct ST_CLASS(PoolNode)         delPoolNode;

   LOG_ACTION
   fprintf(stdlog, "Deregistration request for pool element $%08x of pool ",
           message->Identifier);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message->Type  = AHT_DEREGISTRATION_RESPONSE;
   message->Flags = AHF_DEREGISTRATION_REJECT;

   poolElementNode = ST_CLASS(poolNamespaceManagementFindPoolElement)(
                        &nameServer->Namespace,
                        &message->Handle,
                        message->Identifier);
   if(poolElementNode != NULL) {
      /*
         The ASAP draft says that PeerNameUpdate has to include a full
         Pool Element Parameter, even if this is unnecessary for
         a removal (ID would be sufficient). Since we cannot guarantee
         that deregistration in the namespace is successful, we have
         to copy all data before!
         Obviously, this is a waste of CPU cycles, memory and bandwidth...
      */
      delPoolNode                      = *(poolElementNode->OwnerPoolNode);
      delPoolElementNode               = *poolElementNode;
      delPoolElementNode.OwnerPoolNode = &delPoolNode;
      delPoolElementNode.AddressBlock  = transportAddressBlockDuplicate(poolElementNode->AddressBlock);
      if(delPoolElementNode.AddressBlock != NULL) {

         message->Error = ST_CLASS(poolNamespaceManagementDeregisterPoolElementByPtr)(
                             &nameServer->Namespace,
                             poolElementNode);
         if(message->Error == RSPERR_OKAY) {
            message->Flags = 0x00;

            if(delPoolElementNode.HomeNSIdentifier == nameServer->ServerID) {
               /* We own this PE -> send PeerNameUpdate for its removal. */
               sendPeerNameUpdate(nameServer, &delPoolElementNode, PNUP_DEL_PE);
            }

            LOG_ACTION
            fputs("Deregistration successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Namespace content:\n", stdlog);
            nameServerDumpNamespace(nameServer);
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

         transportAddressBlockDelete(delPoolElementNode.AddressBlock);
         free(delPoolElementNode.AddressBlock);
         delPoolElementNode.AddressBlock = NULL;
      }
      else {
         message->Error = RSPERR_OUT_OF_MEMORY;
      }
   }
   else {
      message->Flags = 0x00;
      message->Error = RSPERR_OKAY;
      LOG_WARNING
      fprintf(stdlog, "Deregistration request for non-existing pool element $%08x of pool ",
              message->Identifier);
      poolHandlePrint(&message->Handle, stdlog);
      fputs(". Reporting success, since it seems to be already gone.\n", stdlog);
      LOG_END
   }

   if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, message) == false) {
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

   if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, message) == false) {
      LOG_WARNING
      logerror("Sending name resolution response failed");
      LOG_END
   }
}


/* ###### Handle endpoint keepalive acknowledgement ###################### */
static void handleEndpointEndpointKeepAliveAck(struct NameServer*      nameServer,
                                       int                     fd,
                                       sctp_assoc_t            assocID,
                                       struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   LOG_VERBOSE
   fprintf(stdlog, "Got EndpointEndpointKeepAliveAck for pool element $%08x of pool ",
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
      fputs("EndpointKeepAlive successful, resetting timer\n", stdlog);
      LOG_END

      ST_CLASS(poolNamespaceNodeDeactivateTimer)(
         &nameServer->Namespace.Namespace,
         poolElementNode);
      ST_CLASS(poolNamespaceNodeActivateTimer)(
         &nameServer->Namespace.Namespace,
         poolElementNode,
         PENT_KEEPALIVE_TRANSMISSION,
         getMicroTime() + nameServer->EndpointKeepAliveTransmissionInterval);
      timerRestart(&nameServer->NamespaceActionTimer,
                   ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                      &nameServer->Namespace));
   }
   else {
      LOG_WARNING
      fprintf(stdlog,
              "EndpointEndpointKeepAliveAck for not-existing pool element $%08x of pool ",
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
   unsigned int                      result;

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
      if(poolElementNode->UnreachabilityReports >= nameServer->MaxBadPEReports) {
         LOG_WARNING
         fprintf(stdlog, "%u unreachability reports for pool element ",
                 poolElementNode->UnreachabilityReports);
         ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
         fputs(" of pool ", stdlog);
         poolHandlePrint(&message->Handle, stdlog);
         fputs(" -> limit reached, removing it\n", stdlog);
         LOG_END

         if(poolElementNode->HomeNSIdentifier == nameServer->ServerID) {
            /* We own this PE -> send PeerNameUpdate for its removal. */
            sendPeerNameUpdate(nameServer, poolElementNode, PNUP_DEL_PE);
         }

         result = ST_CLASS(poolNamespaceManagementDeregisterPoolElementByPtr)(
                     &nameServer->Namespace,
                     poolElementNode);
         if(message->Error == RSPERR_OKAY) {
            LOG_ACTION
            fprintf(stdlog, "Successfully deregistered pool element $%08x from pool ",
                  message->Identifier);
            poolHandlePrint(&message->Handle, stdlog);
            fputs("\n", stdlog);
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


/* ###### Handle peer name update ######################################## */
static void handlePeerNameUpdate(struct NameServer*      nameServer,
                                 int                     fd,
                                 sctp_assoc_t            assocID,
                                 struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
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
      if(message->PoolElementPtr->HomeNSIdentifier != nameServer->ServerID) {
         result = ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
                     &nameServer->Namespace,
                     &message->Handle,
                     message->PoolElementPtr->HomeNSIdentifier,
                     message->PoolElementPtr->Identifier,
                     message->PoolElementPtr->RegistrationLife,
                     &message->PoolElementPtr->PolicySettings,
                     message->PoolElementPtr->AddressBlock,
                     -1, 0,
                     getMicroTime(),
                     &newPoolElementNode);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Successfully registered to pool ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fputs(" pool element ", stdlog);
            ST_CLASS(poolElementNodePrint)(newPoolElementNode, stdlog, PENPO_FULL);
            fputs("\n", stdlog);
            LOG_END

            if(STN_METHOD(IsLinked)(&newPoolElementNode->PoolElementTimerStorageNode)) {
               ST_CLASS(poolNamespaceNodeDeactivateTimer)(
                  &nameServer->Namespace.Namespace,
                  newPoolElementNode);
            }
            ST_CLASS(poolNamespaceNodeActivateTimer)(
               &nameServer->Namespace.Namespace,
               newPoolElementNode,
               PENT_EXPIRY,
               getMicroTime() + newPoolElementNode->RegistrationLife);
            timerRestart(&nameServer->NamespaceActionTimer,
                         ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                            &nameServer->Namespace));

            LOG_VERBOSE3
            fputs("Namespace content:\n", stdlog);
            nameServerDumpNamespace(nameServer);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("Failed to register to pool ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fputs(" pool element ", stdlog);
            ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
            fputs(": ", stdlog);
            rserpoolErrorPrint(result, stdlog);
            fputs("\n", stdlog);
            LOG_END
         }
      }
      else {
         LOG_ERROR
         fprintf(stdlog, "NS $%08x sent me a PeerNameUpdate for a PE owned by myself!",
                 message->SenderID);
         LOG_END
      }
   }

   else if(message->Action == PNUP_DEL_PE) {
      result = ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
                  &nameServer->Namespace,
                  &message->Handle,
                  message->PoolElementPtr->Identifier);
      if(message->Error == RSPERR_OKAY) {
         LOG_ACTION
         fprintf(stdlog, "Successfully deregistered pool element $%08x from pool ",
                 message->PoolElementPtr->Identifier);
         poolHandlePrint(&message->Handle, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "Failed to deregister pool element $%08x from pool ",
                 message->PoolElementPtr->Identifier);
         poolHandlePrint(&message->Handle, stdlog);
         fputs(": ", stdlog);
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END

      }
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
   struct TakeoverProcess* takeoverProcess;
   int                     result;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerPresence message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE2
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
      if((nameServer->ENRPUseMulticast) && (fd == nameServer->ENRPMulticastSocket)) {
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
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fputs("\n", stdlog);
         LOG_END

         /* ====== Activate keep alive timer ========================== */
         if(STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode)) {
            ST_CLASS(peerListDeactivateTimer)(
               &nameServer->Peers.List,
               peerListNode);
         }
         ST_CLASS(peerListActivateTimer)(
            &nameServer->Peers.List,
            peerListNode,
            PLNT_MAX_TIME_LAST_HEARD,
            getMicroTime() + nameServer->PeerMaxTimeLastHeard);
         timerRestart(&nameServer->PeerActionTimer,
                      ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                         &nameServer->Peers));

         /* ====== Print debug information ============================ */
         LOG_VERBOSE3
         fputs("Peers:\n", stdlog);
         nameServerDumpPeers(nameServer);
         LOG_END

         if((message->Flags & EHF_PEER_PRESENCE_REPLY_REQUIRED) &&
            (fd != nameServer->ENRPMulticastSocket) &&
            (message->SenderID != 0)) {
            LOG_VERBOSE
            fputs("PeerPresence with ReplyRequired flag set -> Sending reply...\n", stdlog);
            LOG_END
            sendPeerPresence(nameServer,
                             fd, message->AssocID, 0,
                             NULL, 0,
                             message->SenderID,
                             false);
         }

         if((nameServer->InInitializationPhase) &&
            (nameServer->MentorServerID == 0)) {
            LOG_ACTION
            fputs("Trying ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
            fputs(" as mentor server...\n", stdlog);
            LOG_END
            nameServer->MentorServerID = peerListNode->Identifier;
            sendPeerPresence(nameServer,
                             nameServer->ENRPUnicastSocket,
                             0, 0,
                             peerListNode->AddressBlock->AddressArray,
                             peerListNode->AddressBlock->Addresses,
                             peerListNode->Identifier,
                             false);
            sendPeerListRequest(nameServer,
                                nameServer->ENRPUnicastSocket,
                                0, 0,
                                peerListNode->AddressBlock->AddressArray,
                                peerListNode->AddressBlock->Addresses,
                                peerListNode->Identifier);
            sendPeerNameTableRequest(nameServer,
                                     nameServer->ENRPUnicastSocket,
                                     0, 0,
                                     peerListNode->AddressBlock->AddressArray,
                                     peerListNode->AddressBlock->Addresses,
                                     peerListNode->Identifier);
         }
      }
      else {
         LOG_WARNING
         fputs("Failed to add peer ", stdlog);
         ST_CLASS(peerListNodePrint)(message->PeerListNodePtr, stdlog, PLPO_FULL);
         fputs(": ", stdlog);
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
   }

   if((takeoverProcess = takeoverProcessListFindTakeoverProcess(&nameServer->Takeovers,
                                                                message->SenderID))) {
      LOG_ACTION
      fprintf(stdlog, "Peer $%08x is alive again. Aborting its takeover.\n",
              message->SenderID);
      LOG_END
      takeoverProcessListDeleteTakeoverProcess(&nameServer->Takeovers,
                                                takeoverProcess);
      timerRestart(&nameServer->TakeoverExpiryTimer,
                   takeoverProcessListGetNextTimerTimeStamp(&nameServer->Takeovers));
   }
}


/* ###### Handle peer init takeover ###################################### */
static void handlePeerInitTakeover(struct NameServer*      nameServer,
                                   int                     fd,
                                   sctp_assoc_t            assocID,
                                   struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct TakeoverProcess*        takeoverProcess;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerInitTakeover message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got PeerInitTakeover from peer $%08x for target $%08x\n",
           message->SenderID,
           message->NSIdentifier);
   LOG_END

   /* ====== This server is target -> try to stop it by peer presences === */
   if(message->NSIdentifier == nameServer->ServerID) {
      LOG_WARNING
      fprintf(stdlog, "Peer $%08x has initiated takeover -> trying to stop this by PeerPresences\n",
              message->SenderID);
      LOG_END
      if((nameServer->ENRPUseMulticast) || (nameServer->ENRPAnnounceViaMulticast)) {
         sendPeerPresence(nameServer, nameServer->ENRPMulticastSocket, 0, 0,
                          (union sockaddr_union*)&nameServer->ENRPMulticastAddress, 1,
                          0, false);
      }

      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&nameServer->Peers.List);
      while(peerListNode != NULL) {
         if(!(peerListNode->Flags & PLNF_MULTICAST)) {
            LOG_VERBOSE
            fprintf(stdlog, "Sending PeerPresence to unicast peer $%08x...\n",
                     peerListNode->Identifier);
            LOG_END
            sendPeerPresence(nameServer,
                             nameServer->ENRPUnicastSocket, 0, 0,
                             peerListNode->AddressBlock->AddressArray,
                             peerListNode->AddressBlock->Addresses,
                             peerListNode->Identifier,
                             false);
         }
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                           &nameServer->Peers.List,
                           peerListNode);
      }
   }

   /* ====== Another peer tries takeover, too ============================ */
   else if((takeoverProcess = takeoverProcessListFindTakeoverProcess(
                                 &nameServer->Takeovers,
                                 message->NSIdentifier))) {
      if(message->SenderID < nameServer->ServerID) {
         LOG_ACTION
         fprintf(stdlog, "Peer $%08x, also trying takeover of $%08x, has lower ID -> we ($%08x) abort our takeover\n",
                 message->SenderID,
                 message->NSIdentifier,
                 nameServer->ServerID);
         LOG_END
         /* Acknowledge peer's takeover */
         sendPeerInitTakeoverAck(nameServer,
                                 fd, assocID,
                                 message->SenderID, message->NSIdentifier);
         /* Cancel our takeover process */
         takeoverProcessListDeleteTakeoverProcess(&nameServer->Takeovers,
                                                  takeoverProcess);
         timerRestart(&nameServer->TakeoverExpiryTimer,
                      takeoverProcessListGetNextTimerTimeStamp(&nameServer->Takeovers));
      }
      else {
         LOG_ACTION
         fprintf(stdlog, "Peer $%08x, also trying takeover of $%08x, has higher ID -> we ($%08x) continue our takeover\n",
                 message->SenderID,
                 message->NSIdentifier,
                 nameServer->ServerID);
         LOG_END
      }
   }

   /* ====== Acknowledge takeover ======================================== */
   else {
      LOG_ACTION
      fprintf(stdlog, "Acknowledging peer $%08x's takeover of $%08x\n",
              message->SenderID,
              message->NSIdentifier);
      LOG_END
      sendPeerInitTakeoverAck(nameServer,
                              fd, assocID,
                              message->SenderID, message->NSIdentifier);
   }
}


/* ###### Send ENRP peer takeover server message ######################### */
static void sendPeerTakeoverServer(struct NameServer*          nameServer,
                                   int                         sd,
                                   const sctp_assoc_t          assocID,
                                   int                         msgSendFlags,
                                   const union sockaddr_union* destinationAddressList,
                                   const size_t                destinationAddresses,
                                   const ENRPIdentifierType    receiverID,
                                   const ENRPIdentifierType    targetID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type         = EHT_PEER_TAKEOVER_SERVER;
      message->PPID         = PPID_ENRP;
      message->AssocID      = assocID;
      message->AddressArray = (union sockaddr_union*)destinationAddressList;
      message->Addresses    = destinationAddresses;
      message->Flags        = 0x00;
      message->SenderID     = nameServer->ServerID;
      message->ReceiverID   = receiverID;
      message->NSIdentifier = targetID;

      if(rserpoolMessageSend((sd == nameServer->ENRPMulticastSocket) ? IPPROTO_UDP : IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, message) == false) {
         LOG_WARNING
         fputs("Sending PeerTakeoverServer failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Send ENRP peer ownership change message(s) ##################### */
static void sendPeerOwnershipChange(struct NameServer*          nameServer,
                                    int                         sd,
                                    const sctp_assoc_t          assocID,
                                    int                         msgSendFlags,
                                    const union sockaddr_union* destinationAddressList,
                                    const size_t                destinationAddresses,
                                    const ENRPIdentifierType    receiverID,
                                    const ENRPIdentifierType    targetID)
{
   struct RSerPoolMessage*           message;
   struct ST_CLASS(NameTableExtract) nte;

   message = rserpoolMessageNew(NULL, 250);  // ??????????????
   if(message) {
      message->Type                   = EHT_PEER_OWNERSHIP_CHANGE;
      message->PPID                   = PPID_ENRP;
      message->AssocID                = assocID;
      message->AddressArray           = (union sockaddr_union*)destinationAddressList;
      message->Addresses              = destinationAddresses;
      message->Flags                  = 0x00;
      message->SenderID               = nameServer->ServerID;
      message->ReceiverID             = receiverID;
      message->NSIdentifier           = targetID;
      message->NamespacePtr           = &nameServer->Namespace;
      message->NamespacePtrAutoDelete = false;
      message->ExtractContinuation    = &nte;
      nte.LastPoolElementIdentifier   = 0;

      do {
         if(rserpoolMessageSend((sd == nameServer->ENRPMulticastSocket) ? IPPROTO_UDP : IPPROTO_SCTP,
                                sd, assocID, msgSendFlags, 0, message) == false) {
            LOG_WARNING
            fputs("Sending PeerOwnershipChange failed\n", stdlog);
            LOG_END
            break;
         }
      } while(nte.LastPoolElementIdentifier != 0);
      rserpoolMessageDelete(message);
   }
}



/* ###### Do takeover of peer server ##################################### */
static void doTakeover(struct NameServer*      nameServer,
                       struct TakeoverProcess* takeoverProcess)
{
   struct ST_CLASS(PeerListNode)*    peerListNode;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;

   LOG_WARNING
   fprintf(stdlog, "Taking over peer $%08x...\n",
           takeoverProcess->TargetID);
   LOG_END

   /* ====== Report takeover to peers ==================================== */
   if((nameServer->ENRPUseMulticast) || (nameServer->ENRPAnnounceViaMulticast)) {
      sendPeerTakeoverServer(nameServer, nameServer->ENRPMulticastSocket, 0, 0,
                             (union sockaddr_union*)&nameServer->ENRPMulticastAddress, 1,
                             0,
                             takeoverProcess->TargetID);
      sendPeerOwnershipChange(nameServer, nameServer->ENRPMulticastSocket, 0, 0,
                              (union sockaddr_union*)&nameServer->ENRPMulticastAddress, 1,
                              0,
                              takeoverProcess->TargetID);
   }

   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&nameServer->Peers.List);
   while(peerListNode != NULL) {
      if(!(peerListNode->Flags & PLNF_MULTICAST)) {
         LOG_VERBOSE
         fprintf(stdlog, "Sending PeerTakeoverServer to unicast peer $%08x...\n",
                  peerListNode->Identifier);
         LOG_END
         sendPeerTakeoverServer(nameServer,
                                nameServer->ENRPUnicastSocket, 0, 0,
                                peerListNode->AddressBlock->AddressArray,
                                peerListNode->AddressBlock->Addresses,
                                peerListNode->Identifier,
                                takeoverProcess->TargetID);
         sendPeerOwnershipChange(nameServer,
                                 nameServer->ENRPUnicastSocket, 0, 0,
                                 peerListNode->AddressBlock->AddressArray,
                                 peerListNode->AddressBlock->Addresses,
                                 peerListNode->Identifier,
                                 takeoverProcess->TargetID);
      }
      peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                        &nameServer->Peers.List,
                        peerListNode);
   }


   /* ====== Update PEs' home NS identifier ============================== */
   poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &nameServer->Namespace.Namespace, takeoverProcess->TargetID);
   while(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                               &nameServer->Namespace.Namespace, poolElementNode);

      LOG_ACTION
      fprintf(stdlog, "Taking ownership of pool element $%08x in pool ",
              poolElementNode->Identifier);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END

      ST_CLASS(poolNamespaceNodeUpdateOwnershipOfPoolElementNode)(
          &nameServer->Namespace.Namespace,
          poolElementNode,
          nameServer->ServerID);

      poolElementNode = nextPoolElementNode;
   }


   /* ====== Remove takeover process ===================================== */
   LOG_WARNING
   fprintf(stdlog, "Takeover of peer $%08x completed\n",
           takeoverProcess->TargetID);
   LOG_END
   takeoverProcessListDeleteTakeoverProcess(&nameServer->Takeovers,
                                            takeoverProcess);
   timerRestart(&nameServer->TakeoverExpiryTimer,
                takeoverProcessListGetNextTimerTimeStamp(&nameServer->Takeovers));
}


/* ###### Handle peer init takeover ack ################################## */
static void handlePeerInitTakeoverAck(struct NameServer*      nameServer,
                                      int                     fd,
                                      sctp_assoc_t            assocID,
                                      struct RSerPoolMessage* message)
{
   struct TakeoverProcess* takeoverProcess;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerInitTakeoverAck message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got PeerInitTakeoverAck from peer $%08x for target $%08x\n",
           message->SenderID,
           message->NSIdentifier);
   LOG_END

   takeoverProcess = takeoverProcessListAcknowledgeTakeoverProcess(
                        &nameServer->Takeovers,
                        message->NSIdentifier,
                        message->SenderID);
   if(takeoverProcess) {
      LOG_ACTION
      fprintf(stdlog, "Peer $%08x acknowledges takeover of target $%08x. %u acknowledges to go.\n",
              message->SenderID,
              message->NSIdentifier,
              takeoverProcess->OutstandingAcknowledgements);
      LOG_END
      if(takeoverProcess->OutstandingAcknowledgements == 0) {
         LOG_ACTION
         fprintf(stdlog, "Takeover of target $%08x confirmed. Taking it over now.\n",
                 message->NSIdentifier);
         LOG_END

         doTakeover(nameServer, takeoverProcess);
      }
   }
   else {
      LOG_VERBOSE
      fprintf(stdlog, "Ignoring PeerInitTakeoverAck from peer $%08x for target $%08x\n",
              message->SenderID,
              message->NSIdentifier);
      LOG_END
   }
}


/* ###### Handle peer takeover server #################################### */
static void handlePeerTakeoverServer(struct NameServer*      nameServer,
                                     int                     fd,
                                     sctp_assoc_t            assocID,
                                     struct RSerPoolMessage* message)
{
   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerTakeoverServer message\n", stdlog);
      LOG_END
      return;
   }

   LOG_ACTION
   fprintf(stdlog, "Got PeerTakeoverServer from peer $%08x for target $%08x\n",
           message->SenderID,
           message->NSIdentifier);
   LOG_END
}


/* ###### Handle peer change ownership ################################### */
static void handlePeerOwnershipChange(struct NameServer*      nameServer,
                                      int                     fd,
                                      sctp_assoc_t            assocID,
                                      struct RSerPoolMessage* message)
{
   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerOwnershipChange message\n", stdlog);
      LOG_END
      return;
   }

   LOG_ACTION
   fprintf(stdlog, "Got PeerOwnershipChange from peer $%08x\n",
           message->SenderID);
   LOG_END
}


/* ###### Handle peer list request ####################################### */
static void handlePeerListRequest(struct NameServer*      nameServer,
                                  int                     fd,
                                  sctp_assoc_t            assocID,
                                  struct RSerPoolMessage* message)
{
   struct RSerPoolMessage* response;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerListRequest message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got PeerListRequest from peer $%08x\n",
           message->SenderID);
   LOG_END

   response = rserpoolMessageNew(NULL, 65536);
   if(response != NULL) {
      response->Type       = EHT_PEER_LIST_RESPONSE;
      response->AssocID    = assocID;
      response->Flags      = 0x00;
      response->SenderID   = nameServer->ServerID;
      response->ReceiverID = message->SenderID;

      response->PeerListPtr           = &nameServer->Peers.List;
      response->PeerListPtrAutoDelete = false;

      LOG_VERBOSE
      fprintf(stdlog, "Sending PeerListResponse to peer $%08x...\n",
              message->SenderID);
      LOG_END
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             fd, assocID, 0, 0, response) == false) {
         LOG_WARNING
         fputs("Sending PeerListResponse failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(response);
   }
}


/* ###### Handle peer list response ###################################### */
static void handlePeerListResponse(struct NameServer*      nameServer,
                                   int                     fd,
                                   sctp_assoc_t            assocID,
                                   struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct ST_CLASS(PeerListNode)* newPeerListNode;
   int                            result;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerListResponse message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got PeerListResponse from peer $%08x\n",
           message->SenderID);
   LOG_END

   if(!(message->Flags & EHT_PEER_LIST_RESPONSE_REJECT)) {
      if(message->PeerListPtr) {
         peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                           message->PeerListPtr);
         while(peerListNode) {


            result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                        &nameServer->Peers,
                        peerListNode->Identifier,
                        peerListNode->Flags,
                        peerListNode->AddressBlock,
                        getMicroTime(),
                        &newPeerListNode);
            if((result == RSPERR_OKAY) &&
               (!STN_METHOD(IsLinked)(&newPeerListNode->PeerListTimerStorageNode))) {
               /* ====== Activate keep alive timer ======================= */
               ST_CLASS(peerListActivateTimer)(
                  &nameServer->Peers.List,
                  newPeerListNode,
                  PLNT_MAX_TIME_LAST_HEARD,
                  getMicroTime() + nameServer->PeerMaxTimeLastHeard);

               /* ====== New peer -> Send Peer Presence ================== */
               sendPeerPresence(nameServer,
                                nameServer->ENRPUnicastSocket, 0, 0,
                                newPeerListNode->AddressBlock->AddressArray,
                                newPeerListNode->AddressBlock->Addresses,
                                newPeerListNode->Identifier,
                                false);
            }

            peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                              message->PeerListPtr, peerListNode);
         }
      }

      timerRestart(&nameServer->PeerActionTimer,
                   ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                      &nameServer->Peers));

      /* ====== Print debug information ============================ */
      LOG_VERBOSE3
      fputs("Peers:\n", stdlog);
      nameServerDumpPeers(nameServer);
      LOG_END
   }
   else {
      LOG_ACTION
      fprintf(stdlog, "Rejected PeerListResponse from peer $%08x\n",
            message->SenderID);
      LOG_END
   }
}


/* ###### Handle peer name table request ################################# */
static void handlePeerNameTableRequest(struct NameServer*      nameServer,
                                       int                     fd,
                                       sctp_assoc_t            assocID,
                                       struct RSerPoolMessage* message)
{
   struct RSerPoolMessage*        response;
   struct ST_CLASS(PeerListNode)* peerListNode;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerNameTableRequest message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got PeerNameTableRequest from peer $%08x\n",
           message->SenderID);
   LOG_END

   peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                     &nameServer->Peers,
                     message->SenderID,
                     NULL);

   /* We allow only 1400 bytes per NameTableResponse... */
   response = rserpoolMessageNew(NULL, 1400);
   if(response != NULL) {
      response->Type                      = EHT_PEER_NAME_TABLE_RESPONSE;
      response->AssocID                   = assocID;
      response->Flags                     = 0x00;
      response->SenderID                  = nameServer->ServerID;
      response->ReceiverID                = message->SenderID;
      response->Action                    = message->Flags;

      response->PeerListNodePtr           = peerListNode;
      response->PeerListNodePtrAutoDelete = false;
      response->NamespacePtr              = &nameServer->Namespace;
      response->NamespacePtrAutoDelete    = false;

      if(peerListNode == NULL) {
         message->Flags |= EHT_PEER_NAME_TABLE_RESPONSE_REJECT;
         LOG_WARNING
         fprintf(stdlog, "PeerNameTableRequest from peer $%08x -> This peer is unknown, rejecting request!\n",
                 message->SenderID);
         LOG_END
      }

      LOG_VERBOSE
      fprintf(stdlog, "Sending PeerNameTableResponse to peer $%08x...\n",
              message->SenderID);
      LOG_END
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             fd, assocID, 0, 0, response) == false) {
         LOG_WARNING
         fputs("Sending PeerNameTableResponse failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(response);
   }
}


/* ###### Handle name table response ##################################### */
static void handlePeerNameTableResponse(struct NameServer*      nameServer,
                                        int                     fd,
                                        sctp_assoc_t            assocID,
                                        struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   unsigned int                      result;

   if(message->SenderID == nameServer->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own NameTableResponse message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got NameTableResponse from peer $%08x\n",
           message->SenderID);
   LOG_END

   if(!(message->Flags & EHT_PEER_NAME_TABLE_RESPONSE_REJECT)) {
      if(message->NamespacePtr) {
         poolElementNode = ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNode)(&message->NamespacePtr->Namespace);
         while(poolElementNode != NULL) {
            if(poolElementNode->HomeNSIdentifier != nameServer->ServerID) {
               result = ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
                           &nameServer->Namespace,
                           &poolElementNode->OwnerPoolNode->Handle,
                           poolElementNode->HomeNSIdentifier,
                           poolElementNode->Identifier,
                           poolElementNode->RegistrationLife,
                           &poolElementNode->PolicySettings,
                           poolElementNode->AddressBlock,
                           -1, 0,
                           getMicroTime(),
                           &newPoolElementNode);
               if(result == RSPERR_OKAY) {
                  LOG_ACTION
                  fputs("Successfully registered to pool ", stdlog);
                  poolHandlePrint(&message->Handle, stdlog);
                  fputs(" pool element ", stdlog);
                  ST_CLASS(poolElementNodePrint)(newPoolElementNode, stdlog, PENPO_FULL);
                  fputs("\n", stdlog);
                  LOG_END

                  if(!STN_METHOD(IsLinked)(&newPoolElementNode->PoolElementTimerStorageNode)) {
                     ST_CLASS(poolNamespaceNodeActivateTimer)(
                        &nameServer->Namespace.Namespace,
                        newPoolElementNode,
                        PENT_EXPIRY,
                        getMicroTime() + newPoolElementNode->RegistrationLife);
                  }
               }
               else {
                  LOG_WARNING
                  fputs("Failed to register to pool ", stdlog);
                  poolHandlePrint(&message->Handle, stdlog);
                  fputs(" pool element ", stdlog);
                  ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
                  fputs(": ", stdlog);
                  rserpoolErrorPrint(result, stdlog);
                  fputs("\n", stdlog);
                  LOG_END
               }
            }
            else {
               LOG_ERROR
               fprintf(stdlog, "NS $%08x sent me a NameTableResponse containing a PE owned by myself: ",
                       message->SenderID);
               ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
               fputs("\n", stdlog);
               LOG_END
            }
            poolElementNode = ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNode)(&message->NamespacePtr->Namespace, poolElementNode);
         }

         timerRestart(&nameServer->NamespaceActionTimer,
                      ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                         &nameServer->Namespace));


         LOG_VERBOSE3
         fputs("Namespace content:\n", stdlog);
         nameServerDumpNamespace(nameServer);
         fputs("Peers:\n", stdlog);
         nameServerDumpPeers(nameServer);
         LOG_END


         if(message->Flags & EHT_PEER_NAME_TABLE_RESPONSE_MORE_TO_SEND) {
            LOG_VERBOSE
            fprintf(stdlog, "NameTableResponse has MoreToSend flag set -> sending NameTableRequest to peer $%08x to get more data\n",
                    message->SenderID);
            LOG_END
            sendPeerNameTableRequest(nameServer, fd, message->AssocID, 0, NULL, 0, message->SenderID);
         }
         else {
            if((nameServer->InInitializationPhase) &&
               (nameServer->MentorServerID == message->SenderID)) {
               nameServer->InInitializationPhase = false;
               LOG_NOTE
               fputs("Initialization phase ended after obtaining namespace from mentor server. The name server is ready!\n", stdlog);
               LOG_END
               nameServerInitializationComplete(nameServer);
            }
         }
      }
   }
   else {
      LOG_ACTION
      fprintf(stdlog, "Rejected PeerNameTableResponse from peer $%08x\n",
              message->SenderID);
      LOG_END
   }
}


/* ###### Handle incoming message ######################################## */
static void handleMessage(struct NameServer*      nameServer,
                          struct RSerPoolMessage* message,
                          int                     sd)
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
         handleEndpointEndpointKeepAliveAck(nameServer, sd, message->AssocID, message);
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
      case EHT_PEER_LIST_REQUEST:
         handlePeerListRequest(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_LIST_RESPONSE:
         handlePeerListResponse(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_NAME_TABLE_REQUEST:
         handlePeerNameTableRequest(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_NAME_TABLE_RESPONSE:
         handlePeerNameTableResponse(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_INIT_TAKEOVER:
         handlePeerInitTakeover(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_INIT_TAKEOVER_ACK:
         handlePeerInitTakeoverAck(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_TAKEOVER_SERVER:
         handlePeerTakeoverServer(nameServer, sd, message->AssocID, message);
       break;
      case EHT_PEER_OWNERSHIP_CHANGE:
         handlePeerOwnershipChange(nameServer, sd, message->AssocID, message);
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
                                     unsigned int       eventMask,
                                     void*              userData)
{
   struct NameServer*       nameServer = (struct NameServer*)userData;
   struct RSerPoolMessage*  message;
   union sctp_notification* notification;
   union sockaddr_union     remoteAddress;
   socklen_t                remoteAddressLength;
   char                     buffer[65536];
   int                      flags;
   uint32_t                 ppid;
   sctp_assoc_t             assocID;
   unsigned short           streamID;
   ssize_t                  received;

   if((fd == nameServer->ASAPSocket) ||
      (fd == nameServer->ENRPUnicastSocket) ||
      (((nameServer->ENRPUseMulticast) || (nameServer->ENRPAnnounceViaMulticast)) &&
        (fd == nameServer->ENRPMulticastSocket))) {
      LOG_VERBOSE3
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
               LOG_VERBOSE3
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
                     nameServerRemovePoolElementsOfConnection(nameServer, fd,
                                                              notification->sn_assoc_change.sac_assoc_id);
                  }
                  else if(notification->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP) {
                     LOG_ACTION
                     fprintf(stdlog, "Association shutdown completed for socket %d, assoc %u\n",
                             nameServer->ASAPSocket,
                             notification->sn_assoc_change.sac_assoc_id);

                     LOG_END
                     nameServerRemovePoolElementsOfConnection(nameServer, fd,
                                                              notification->sn_assoc_change.sac_assoc_id);
                  }
                break;
               case SCTP_SHUTDOWN_EVENT:
                  LOG_ACTION
                  fprintf(stdlog, "Shutdown event for socket %d, assoc %u\n",
                          nameServer->ASAPSocket,
                          notification->sn_shutdown_event.sse_assoc_id);

                  LOG_END
                  nameServerRemovePoolElementsOfConnection(nameServer, fd,
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
   union sockaddr_union          addressArray[MAX_NS_TRANSPORTADDRESSES];
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
                            (union sockaddr_union*)&addressArray,
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
   union sockaddr_union     sctpAddressArray[MAX_NS_TRANSPORTADDRESSES];
   union sockaddr_union*    localAddressArray;
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
          sctpAddresses = getladdrsplus(sockFD, 0, (union sockaddr_union**)&localAddressArray);
          if(sctpAddresses > MAX_NS_TRANSPORTADDRESSES) {
             puts("ERROR: Too many local addresses -> specify only a subset!");
             exit(1);
          }
          memcpy(&sctpAddressArray, localAddressArray, sctpAddresses * sizeof(union sockaddr_union));
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
                   (union sockaddr_union*)&sctpAddressArray,
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
       autoCloseTimeout = 5 + (2 * (NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL / 1000000));
       if(ext_setsockopt(sctpSocket, IPPROTO_SCTP, SCTP_AUTOCLOSE, &autoCloseTimeout, sizeof(autoCloseTimeout)) < 0) {
          perror("setsockopt() for SCTP_AUTOCLOSE failed");
          exit(1);
       }

       transportAddressBlockNew(sctpAddress,
                                IPPROTO_SCTP,
                                getPort((struct sockaddr*)&sctpAddressArray[0]),
                                0,
                                (union sockaddr_union*)&sctpAddressArray,
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
   union sockaddr_union          asapAnnounceAddress;
   bool                          asapSendAnnounces = false;
   int                           enrpUnicastSocket = -1;
   char                          enrpUnicastAddressBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* enrpUnicastAddress = (struct TransportAddressBlock*)&enrpUnicastAddressBuffer;
   union sockaddr_union          enrpMulticastAddress;
   bool                          enrpUseMulticast         = false;
   bool                          enrpAnnounceViaMulticast = false;
   int                           result;
   struct timeval                timeout;
   fd_set                        readfdset;
   fd_set                        writefdset;
   fd_set                        exceptfdset;
   fd_set                        testfdset;
   unsigned long long            testTS;
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
      else if(!(strncasecmp(argv[i], "-maxbadpereports=",17))) {
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
         enrpAnnounceViaMulticast = true;
      }
      else if(!(strncasecmp(argv[i], "-enrpmulticast=", 15))) {
         if(!string2address((char*)&argv[i][15], &enrpMulticastAddress)) {
            fprintf(stderr,
                    "ERROR: Bad ENRP announce address %s! Use format <address:port>.\n",
                    (char*)&argv[i][10]);
            exit(1);
         }
         enrpAnnounceViaMulticast = true;
      }
      else if(!(strcasecmp(argv[i], "-multicast=on"))) {
         enrpUseMulticast = true;
      }
      else if(!(strcasecmp(argv[i], "-multicast=off"))) {
         enrpUseMulticast = false;
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
                              enrpUseMulticast, enrpAnnounceViaMulticast, (union sockaddr_union*)&enrpMulticastAddress);
   if(nameServer == NULL) {
      fprintf(stderr, "ERROR: Unable to initialize NameServer object!\n");
      exit(1);
   }
   for(i = 1;i < argc;i++) {
      if(!(strncasecmp(argv[i], "-peer=",6))) {
         addPeer(nameServer, (char*)&argv[i][6]);
      }
      else if(!(strncasecmp(argv[i], "-maxbadpereports=",17))) {
         nameServer->MaxBadPEReports = atol((char*)&argv[i][17]);
         if(nameServer->MaxBadPEReports < 1) {
            nameServer->MaxBadPEReports = 1;
         }
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
   if(enrpAnnounceViaMulticast) {
      fputaddress((struct sockaddr*)&enrpMulticastAddress, true, stdout);
      puts("");
   }
   else {
      puts("(none)");
   }
   printf("ENRP Use Multicast:     ");
   if((enrpAnnounceViaMulticast) && !(enrpUseMulticast)) {
      printf("Announcements Only\n");
   }
   else if(enrpUseMulticast) {
      printf("Full Mutlicast\n");
   }
   else {
      printf("Off\n");
   }

   puts("\nASAP Parameters:");
   printf("   Max Bad PE Reports:                          %d\n",    nameServer->MaxBadPEReports);
   printf("   Server Announce Cycle:                       %Ldµs\n", nameServer->ServerAnnounceCycle);
   printf("   Endpoint Monitoring SCTP Heartbeat Interval: %Ldµs\n", nameServer->EndpointMonitoringHeartbeatInterval);
   printf("   Endpoint Keep Alive Transmission Interval:   %Ldµs\n", nameServer->EndpointKeepAliveTransmissionInterval);
   printf("   Endpoint Keep Alive Timeout Interval:        %Ldµs\n", nameServer->EndpointKeepAliveTimeoutInterval);
   puts("ENRP Parameters:");
   printf("   Peer Heartbeat Cylce:                        %Ldµs\n", nameServer->PeerHeartbeatCycle);
   printf("   Peer Max Time Last Heard:                    %Ldµs\n", nameServer->PeerMaxTimeLastHeard);
   printf("   Peer Max Time No Response:                   %Ldµs\n", nameServer->PeerMaxTimeNoResponse);
   printf("   Mentor Hunt Timeout:                         %Ldµs\n", nameServer->MentorHuntTimeout);
   printf("   Takeover Expiry Interval:                    %Ldµs\n", nameServer->TakeoverExpiryInterval);
   puts("");


   /* ====== We are ready! =============================================== */
   beginLogging();
   LOG_NOTE
   fputs("Name server startet. Going into initialization phase...\n", stdlog);
   LOG_END


   /* ====== Main loop =================================================== */
   while(!breakDetected()) {
      dispatcherGetSelectParameters(&nameServer->StateMachine, &n, &readfdset, &writefdset, &exceptfdset, &testfdset, &testTS, &timeout);
      timeout.tv_sec  = min(0, timeout.tv_sec);
      timeout.tv_usec = min(250000, timeout.tv_usec);
      result = ext_select(n, &readfdset, &writefdset, &exceptfdset, &timeout);
      dispatcherHandleSelectResult(&nameServer->StateMachine, result, &readfdset, &writefdset, &exceptfdset, &testfdset, testTS);
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
