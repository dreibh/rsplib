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
 * Purpose: Registrar
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "dispatcher.h"
#include "timer.h"

#include "rserpoolmessage.h"
#include "poolhandlespacemanagement.h"
#include "randomizer.h"
#include "breakdetector.h"
#ifdef ENABLE_CSP
#include "componentstatusreporter.h"
#endif

#include <ext_socket.h>
#include <net/if.h>
#include <sys/ioctl.h>



/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */


#define MAX_NS_TRANSPORTADDRESSES                                           16
#define MIN_ENDPOINT_ADDRESS_SCOPE                                           4
#define NAMESERVER_DEFAULT_MAX_BAD_PE_REPORTS                                3
#define NAMESERVER_DEFAULT_SERVER_ANNOUNCE_CYCLE                       1000000
#define NAMESERVER_DEFAULT_ENDPOINT_MONITORING_HEARTBEAT_INTERVAL      1000000
#define NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL   5000000
#define NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TIMEOUT_INTERVAL        5000000
#define NAMESERVER_DEFAULT_PEER_HEARTBEAT_CYCLE                        2444444
#define NAMESERVER_DEFAULT_PEER_MAX_TIME_LAST_HEARD                    5000000
#define NAMESERVER_DEFAULT_PEER_MAX_TIME_NO_RESPONSE                   2000000
#define NAMESERVER_DEFAULT_MENTOR_HUNT_TIMEOUT                         5000000
#define NAMESERVER_DEFAULT_TAKEOVER_EXPIRY_INTERVAL                    5000000


unsigned long long randomizeCycle(const unsigned long long interval)
{
   const double originalInterval = (double)interval;
   const double variation    = 0.250 * originalInterval;
   const double nextInterval = originalInterval - (variation / 2.0) +
                               variation * ((double)rand() / (double)RAND_MAX);
   return((unsigned long long)nextInterval);
}



struct TakeoverProcess
{
   struct STN_CLASSNAME    IndexStorageNode;
   struct STN_CLASSNAME    TimerStorageNode;

   RegistrarIdentifierType TargetID;
   unsigned long long      ExpiryTimeStamp;

   size_t                  OutstandingAcknowledgements;
   RegistrarIdentifierType PeerIDArray[0];
};


/* ###### Get takeover process from index storage node ################### */
struct TakeoverProcess* getTakeoverProcessFromIndexStorageNode(struct STN_CLASSNAME* node)
{
   const struct TakeoverProcess* dummy = (struct TakeoverProcess*)node;
   long n = (long)node - ((long)&dummy->IndexStorageNode - (long)dummy);
   return((struct TakeoverProcess*)n);
}


/* ###### Get takeover process from timer storage node ################### */
struct TakeoverProcess* getTakeoverProcessFromTimerStorageNode(struct STN_CLASSNAME* node)
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
   fprintf(fd, "   - Takeover for $%08x (expiry in %lldus)\n",
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
           (unsigned int)ST_METHOD(GetElements)(&takeoverProcessList->TakeoverProcessIndexStorage));
   ST_METHOD(Print)(&takeoverProcessList->TakeoverProcessIndexStorage, fd);
}


/* ###### Get next takeover process's expiry time stamp ################## */
unsigned long long takeoverProcessListGetNextTimerTimeStamp(
                             struct TakeoverProcessList* takeoverProcessList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&takeoverProcessList->TakeoverProcessTimerStorage);
   if(node) {
      return(getTakeoverProcessFromTimerStorageNode(node)->ExpiryTimeStamp);
   }
   return(~0);
}


/* ###### Get earliest takeover process from list ######################## */
struct TakeoverProcess* takeoverProcessListGetEarliestTakeoverProcess(
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
struct TakeoverProcess* takeoverProcessListGetFirstTakeoverProcess(
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
struct TakeoverProcess* takeoverProcessListGetLastTakeoverProcess(
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
struct TakeoverProcess* takeoverProcessListGetNextTakeoverProcess(
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
struct TakeoverProcess* takeoverProcessListGetPrevTakeoverProcess(
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
                           const RegistrarIdentifierType    targetID)
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
                const RegistrarIdentifierType        targetID,
                struct ST_CLASS(PeerListManagement)* peerList,
                const unsigned long long             expiryTimeStamp)
{
   struct TakeoverProcess*        takeoverProcess;
   struct ST_CLASS(PeerListNode)* peerListNode;
   const size_t                   peers = ST_CLASS(peerListManagementGetPeers)(peerList);

   CHECK(targetID != 0);
   CHECK(targetID != peerList->List.OwnIdentifier);
   if(takeoverProcessListFindTakeoverProcess(takeoverProcessList, targetID)) {
      return(RSPERR_DUPLICATE_ID);
   }

   takeoverProcess = (struct TakeoverProcess*)malloc(sizeof(struct TakeoverProcess) +
                                                     sizeof(RegistrarIdentifierType) * peers);
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
                           const RegistrarIdentifierType    targetID,
                           const RegistrarIdentifierType    acknowledgerID)
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







struct Registrar
{
   RegistrarIdentifierType                    ServerID;

   struct Dispatcher                          StateMachine;
   struct ST_CLASS(PoolHandlespaceManagement) Handlespace;
   struct ST_CLASS(PeerListManagement)        Peers;
   struct Timer                               HandlespaceActionTimer;
   struct Timer                               PeerActionTimer;

   int                                        ASAPAnnounceSocket;
   struct FDCallback                          ASAPSocketFDCallback;
   union sockaddr_union                       ASAPAnnounceAddress;
   bool                                       ASAPSendAnnounces;
   int                                        ASAPSocket;
   struct Timer                               ASAPAnnounceTimer;

   int                                        ENRPMulticastOutputSocket;
   int                                        ENRPMulticastInputSocket;
   struct FDCallback                          ENRPMulticastInputSocketFDCallback;
   union sockaddr_union                       ENRPMulticastAddress;
   int                                        ENRPUnicastSocket;
   struct FDCallback                          ENRPUnicastSocketFDCallback;
   bool                                       ENRPAnnounceViaMulticast;
   bool                                       ENRPUseMulticast;
   struct Timer                               ENRPAnnounceTimer;

   bool                                       InInitializationPhase;
   RegistrarIdentifierType                    MentorServerID;

   struct TakeoverProcessList                 Takeovers;
   struct Timer                               TakeoverExpiryTimer;

   size_t                                     MaxBadPEReports;
   unsigned long long                         ServerAnnounceCycle;
   unsigned long long                         EndpointMonitoringHeartbeatInterval;
   unsigned long long                         EndpointKeepAliveTransmissionInterval;
   unsigned long long                         EndpointKeepAliveTimeoutInterval;
   unsigned long long                         PeerHeartbeatCycle;
   unsigned long long                         PeerMaxTimeLastHeard;
   unsigned long long                         PeerMaxTimeNoResponse;
   unsigned long long                         MentorHuntTimeout;
   unsigned long long                         TakeoverExpiryInterval;

#ifdef ENABLE_CSP
   struct CSPReporter                         CSPReporter;
   unsigned int                               CSPReportInterval;
   union sockaddr_union                       CSPReportAddress;
#endif
};


void registrarDumpHandlespace(struct Registrar* registrar);
void registrarDumpPeers(struct Registrar* registrar);
#ifdef ENABLE_CSP
static size_t registrarGetReportFunction(
                 void*                         userData,
                 unsigned long long*           identifier,
                 struct ComponentAssociation** caeArray,
                 char*                         statusText,
                 char*                         componentLocation,
                 double*                       workload);
#endif


struct Registrar* registrarNew(const RegistrarIdentifierType  serverID,
                               int                            asapSocket,
                               int                            asapAnnounceSocket,
                               int                            enrpUnicastSocket,
                               int                            enrpMulticastOutputSocket,
                               int                            enrpMulticastInputSocket,
                               const bool                     asapSendAnnounces,
                               const union sockaddr_union*    asapAnnounceAddress,
                               const bool                     enrpUseMulticast,
                               const bool                     enrpAnnounceViaMulticast,
                               const union sockaddr_union*    enrpMulticastAddress
#ifdef ENABLE_CSP
                               , const unsigned int           cspReportInterval,
                               const union sockaddr_union*    cspReportAddress
#endif
                               );
void registrarDelete(struct Registrar* registrar);

static void sendHandleUpdate(struct Registrar*                registrar,
                             struct ST_CLASS(PoolElementNode)* poolElementNode,
                             const uint16_t                    action);
static void sendInitTakeover(struct Registrar*       registrar,
                             const RegistrarIdentifierType targetID);
static void sendInitTakeoverAck(struct Registrar*       registrar,
                                const int                sd,
                                const sctp_assoc_t       assocID,
                                const RegistrarIdentifierType receiverID,
                                const RegistrarIdentifierType targetID);
static void asapAnnounceTimerCallback(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData);
static void enrpAnnounceTimerCallback(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData);
static void handlespaceActionTimerCallback(struct Dispatcher* dispatcher,
                                           struct Timer*      timer,
                                           void*              userData);
static void peerActionTimerCallback(struct Dispatcher* dispatcher,
                                    struct Timer*      timer,
                                    void*              userData);
static void takeoverExpiryTimerCallback(struct Dispatcher* dispatcher,
                                        struct Timer*      timer,
                                        void*              userData);
static void registrarSocketCallback(struct Dispatcher* dispatcher,
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
   /* struct Registrar* registrar = (struct Registrar*)userData; */
   if(peerListNode->UserData) {
      /* A peer handle table request state is still saved. Free its memory. */
      free(peerListNode->UserData);
      peerListNode->UserData = NULL;
   }
}




/* ###### Constructor #################################################### */
struct Registrar* registrarNew(const RegistrarIdentifierType  serverID,
                               int                            asapSocket,
                               int                            asapAnnounceSocket,
                               int                            enrpUnicastSocket,
                               int                            enrpMulticastOutputSocket,
                               int                            enrpMulticastInputSocket,
                               const bool                     asapSendAnnounces,
                               const union sockaddr_union*    asapAnnounceAddress,
                               const bool                     enrpUseMulticast,
                               const bool                     enrpAnnounceViaMulticast,
                               const union sockaddr_union*    enrpMulticastAddress
#ifdef ENABLE_CSP
                               , const unsigned int           cspReportInterval,
                               const union sockaddr_union*    cspReportAddress
#endif
                               )
{
   struct Registrar* registrar = (struct Registrar*)malloc(sizeof(struct Registrar));
   if(registrar != NULL) {
      registrar->ServerID = serverID;
      if(registrar->ServerID == 0) {
         registrar->ServerID = random32();
      }

      dispatcherNew(&registrar->StateMachine,
                    dispatcherDefaultLock, dispatcherDefaultUnlock, NULL);
      ST_CLASS(poolHandlespaceManagementNew)(&registrar->Handlespace,
                                             registrar->ServerID,
                                             NULL,
                                             poolElementNodeDisposer,
                                             registrar);
      ST_CLASS(peerListManagementNew)(&registrar->Peers,
                                      &registrar->Handlespace,
                                      registrar->ServerID,
                                      peerListNodeDisposer,
                                      registrar);
      timerNew(&registrar->ASAPAnnounceTimer,
               &registrar->StateMachine,
               asapAnnounceTimerCallback,
               (void*)registrar);
      timerNew(&registrar->ENRPAnnounceTimer,
               &registrar->StateMachine,
               enrpAnnounceTimerCallback,
               (void*)registrar);
      timerNew(&registrar->HandlespaceActionTimer,
               &registrar->StateMachine,
               handlespaceActionTimerCallback,
               (void*)registrar);
      timerNew(&registrar->PeerActionTimer,
               &registrar->StateMachine,
               peerActionTimerCallback,
               (void*)registrar);
      timerNew(&registrar->TakeoverExpiryTimer,
               &registrar->StateMachine,
               takeoverExpiryTimerCallback,
               (void*)registrar);
      takeoverProcessListNew(&registrar->Takeovers);

      registrar->InInitializationPhase     = true;
      registrar->MentorServerID            = 0;

      registrar->ASAPSocket                = asapSocket;
      registrar->ASAPAnnounceSocket        = asapAnnounceSocket;
      registrar->ASAPSendAnnounces         = asapSendAnnounces;

      registrar->ENRPUnicastSocket         = enrpUnicastSocket;
      registrar->ENRPMulticastInputSocket  = enrpMulticastOutputSocket;
      registrar->ENRPMulticastOutputSocket = enrpMulticastInputSocket;
      registrar->ENRPUseMulticast          = enrpUseMulticast;
      registrar->ENRPAnnounceViaMulticast  = enrpAnnounceViaMulticast;


      registrar->MaxBadPEReports                       = NAMESERVER_DEFAULT_MAX_BAD_PE_REPORTS;
      registrar->ServerAnnounceCycle                   = NAMESERVER_DEFAULT_SERVER_ANNOUNCE_CYCLE;
      registrar->EndpointMonitoringHeartbeatInterval   = NAMESERVER_DEFAULT_ENDPOINT_MONITORING_HEARTBEAT_INTERVAL;
      registrar->EndpointKeepAliveTransmissionInterval = NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL;
      registrar->EndpointKeepAliveTimeoutInterval      = NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TIMEOUT_INTERVAL;
      registrar->PeerMaxTimeLastHeard                  = NAMESERVER_DEFAULT_PEER_MAX_TIME_LAST_HEARD;
      registrar->PeerMaxTimeNoResponse                 = NAMESERVER_DEFAULT_PEER_MAX_TIME_NO_RESPONSE;
      registrar->PeerHeartbeatCycle                    = NAMESERVER_DEFAULT_PEER_HEARTBEAT_CYCLE;
      registrar->MentorHuntTimeout                     = NAMESERVER_DEFAULT_MENTOR_HUNT_TIMEOUT;
      registrar->TakeoverExpiryInterval                = NAMESERVER_DEFAULT_TAKEOVER_EXPIRY_INTERVAL;

      memcpy(&registrar->ASAPAnnounceAddress, asapAnnounceAddress, sizeof(registrar->ASAPAnnounceAddress));
      if(registrar->ASAPAnnounceSocket >= 0) {
         setNonBlocking(registrar->ASAPAnnounceSocket);
      }

      fdCallbackNew(&registrar->ASAPSocketFDCallback,
                    &registrar->StateMachine,
                    registrar->ASAPSocket,
                    FDCE_Read|FDCE_Exception,
                    registrarSocketCallback,
                    (void*)registrar);
      fdCallbackNew(&registrar->ENRPUnicastSocketFDCallback,
                    &registrar->StateMachine,
                    registrar->ENRPUnicastSocket,
                    FDCE_Read|FDCE_Exception,
                    registrarSocketCallback,
                    (void*)registrar);

      memcpy(&registrar->ENRPMulticastAddress, enrpMulticastAddress, sizeof(registrar->ENRPMulticastAddress));
      if(registrar->ENRPMulticastInputSocket >= 0) {
         setNonBlocking(registrar->ENRPMulticastInputSocket);
         fdCallbackNew(&registrar->ENRPMulticastInputSocketFDCallback,
                       &registrar->StateMachine,
                       registrar->ENRPMulticastInputSocket,
                       FDCE_Read|FDCE_Exception,
                       registrarSocketCallback,
                       (void*)registrar);
      }

      timerStart(&registrar->ENRPAnnounceTimer, getMicroTime() + registrar->MentorHuntTimeout);
      if((registrar->ENRPUseMulticast) || (registrar->ENRPAnnounceViaMulticast)) {
         if(joinOrLeaveMulticastGroup(registrar->ENRPMulticastInputSocket,
                                      &registrar->ENRPMulticastAddress,
                                      true) == false) {
            LOG_WARNING
            fputs("Unable to join multicast group ", stdlog);
            fputaddress(&registrar->ENRPMulticastAddress.sa, true, stdlog);
            fputs(". Registrar will probably be unable to detect peers!\n", stdlog);
            LOG_END
         }
      }

      if(registrar->ENRPMulticastOutputSocket >= 0) {
         setNonBlocking(registrar->ENRPMulticastOutputSocket);
      }

#ifdef ENABLE_CSP
      registrar->CSPReportInterval = cspReportInterval;
      if(registrar->CSPReportInterval > 0) {
         memcpy(&registrar->CSPReportAddress,
                cspReportAddress,
                sizeof(registrar->CSPReportAddress));
         cspReporterNew(&registrar->CSPReporter, &registrar->StateMachine,
                        CID_COMPOUND(CID_GROUP_REGISTRAR, registrar->ServerID),
                        &registrar->CSPReportAddress.sa,
                        registrar->CSPReportInterval,
                        registrarGetReportFunction,
                        registrar);
      }
#endif
   }

   return(registrar);
}


/* ###### Destructor ###################################################### */
void registrarDelete(struct Registrar* registrar)
{
   if(registrar) {
      fdCallbackDelete(&registrar->ENRPUnicastSocketFDCallback);
      fdCallbackDelete(&registrar->ASAPSocketFDCallback);
      ST_CLASS(peerListManagementDelete)(&registrar->Peers);
      ST_CLASS(poolHandlespaceManagementDelete)(&registrar->Handlespace);
#ifdef ENABLE_CSP
      if(registrar->CSPReportInterval > 0) {
         cspReporterDelete(&registrar->CSPReporter);
      }
#endif
      timerDelete(&registrar->TakeoverExpiryTimer);
      timerDelete(&registrar->ASAPAnnounceTimer);
      timerDelete(&registrar->ENRPAnnounceTimer);
      timerDelete(&registrar->HandlespaceActionTimer);
      timerDelete(&registrar->PeerActionTimer);
      takeoverProcessListDelete(&registrar->Takeovers);
      if(registrar->ENRPMulticastOutputSocket >= 0) {
         fdCallbackDelete(&registrar->ENRPMulticastInputSocketFDCallback);
         ext_close(registrar->ENRPMulticastOutputSocket >= 0);
         registrar->ENRPMulticastOutputSocket = -1;
      }
      if(registrar->ENRPMulticastInputSocket >= 0) {
         ext_close(registrar->ENRPMulticastInputSocket >= 0);
         registrar->ENRPMulticastInputSocket = -1;
      }
      if(registrar->ENRPUnicastSocket >= 0) {
         ext_close(registrar->ENRPUnicastSocket >= 0);
         registrar->ENRPUnicastSocket = -1;
      }
      if(registrar->ASAPAnnounceSocket >= 0) {
         ext_close(registrar->ASAPAnnounceSocket >= 0);
         registrar->ASAPAnnounceSocket = -1;
      }
      dispatcherDelete(&registrar->StateMachine);
      free(registrar);
   }
}


/* ###### Dump handlespace ################################################# */
void registrarDumpHandlespace(struct Registrar* registrar)
{
   fputs("*************** Handlespace Dump ***************\n", stdlog);
   printTimeStamp(stdlog);
   fputs("\n", stdlog);
   ST_CLASS(poolHandlespaceManagementPrint)(&registrar->Handlespace, stdlog,
            PNNPO_POOLS_INDEX|PNNPO_POOLS_SELECTION|PNNPO_POOLS_TIMER|PENPO_USERTRANSPORT|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PENPO_HOME_PR);
   fputs("**********************************************\n", stdlog);
}


/* ###### Dump peers ##################################################### */
void registrarDumpPeers(struct Registrar* registrar)
{
   fputs("*************** Peers ************************\n", stdlog);
   ST_CLASS(peerListManagementPrint)(&registrar->Peers, stdlog, PLPO_FULL);
   fputs("**********************************************\n", stdlog);
}


/* ###### ASAP announcement timer callback ############################### */
static void asapAnnounceTimerCallback(struct Dispatcher* dispatcher,
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
      message->Type         = AHT_SERVER_ANNOUNCE;
      message->Flags        = 0x00;
      message->RegistrarIdentifier = registrar->ServerID;
      messageLength = rserpoolMessage2Packet(message);
      if(messageLength > 0) {
         if(ext_sendto(registrar->ASAPAnnounceSocket,
                       message->Buffer,
                       messageLength,
                       0,
                       (struct sockaddr*)&registrar->ASAPAnnounceAddress,
                       getSocklen((struct sockaddr*)&registrar->ASAPAnnounceAddress)) < (ssize_t)messageLength) {
            LOG_WARNING
            logerror("Unable to send announce");
            LOG_END
         }
      }
      rserpoolMessageDelete(message);
   }
   timerStart(timer, getMicroTime() + randomizeCycle(registrar->ServerAnnounceCycle));
}


/* ###### Remove all PEs registered via a given connection ############### */
static void registrarRemovePoolElementsOfConnection(struct Registrar*  registrar,
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
            sendHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
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


/* ###### Send ENRP peer presence message ################################ */
static void sendPeerPresence(struct Registrar*             registrar,
                             int                           sd,
                             const sctp_assoc_t            assocID,
                             int                           msgSendFlags,
                             const union sockaddr_union*   destinationAddressList,
                             const size_t                  destinationAddresses,
                             const RegistrarIdentifierType receiverID,
                             const bool                    replyRequired)
{
   struct RSerPoolMessage*       message;
   struct ST_CLASS(PeerListNode) peerListNode;
   char                          localAddressArrayBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* localAddressArray = (struct TransportAddressBlock*)&localAddressArrayBuffer;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type                      = EHT_PRESENCE;
      message->PPID                      = PPID_ENRP;
      message->AssocID                   = assocID;
      message->AddressArray              = (union sockaddr_union*)destinationAddressList;
      message->Addresses                 = destinationAddresses;
      message->Flags                     = replyRequired ? EHF_PRESENCE_REPLY_REQUIRED : 0x00;
      message->PeerListNodePtr           = &peerListNode;
      message->PeerListNodePtrAutoDelete = false;
      message->SenderID                  = registrar->ServerID;
      message->ReceiverID                = receiverID;
      message->Checksum                  = ST_CLASS(poolHandlespaceManagementGetOwnershipChecksum)(&registrar->Handlespace);

      if(transportAddressBlockGetAddressesFromSCTPSocket(localAddressArray,
                                                         registrar->ENRPUnicastSocket,
                                                         assocID,
                                                         MAX_NS_TRANSPORTADDRESSES,
                                                         true) > 0) {
         ST_CLASS(peerListNodeNew)(&peerListNode,
                                   registrar->ServerID,
                                   registrar->ENRPUseMulticast ? PLNF_MULTICAST : 0,
                                   localAddressArray);

         LOG_VERBOSE3
         fputs("Sending PeerPresence using peer list entry: \n", stdlog);
         ST_CLASS(peerListNodePrint)(&peerListNode, stdlog, ~0);
         fputs("\n", stdlog);
         LOG_END
         if(rserpoolMessageSend((sd == registrar->ENRPMulticastOutputSocket) ? IPPROTO_UDP : IPPROTO_SCTP,
                                sd, assocID, msgSendFlags, 0, message) == false) {
            LOG_WARNING
            fputs("Sending PeerPresence failed\n", stdlog);
            LOG_END
         }
         rserpoolMessageDelete(message);
      }
      else {
         LOG_ERROR
         fputs("Unable to obtain local ASAP socket addresses\n", stdlog);
         LOG_END
      }
   }
}


/* ###### Send ENRP peer list request message ############################ */
static void sendListRequest(struct Registrar*           registrar,
                            int                         sd,
                            const sctp_assoc_t          assocID,
                            int                         msgSendFlags,
                            const union sockaddr_union* destinationAddressList,
                            const size_t                destinationAddresses,
                            RegistrarIdentifierType     receiverID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type         = EHT_LIST_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = assocID;
      message->AddressArray = (union sockaddr_union*)destinationAddressList;
      message->Addresses    = destinationAddresses;
      message->Flags        = 0x00;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = receiverID;
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, message) == false) {
         LOG_WARNING
         fputs("Sending ListRequest failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Send ENRP peer handle table request message ###################### */
static void sendHandleTableRequest(struct Registrar*           registrar,
                                   int                         sd,
                                   const sctp_assoc_t          assocID,
                                   int                         msgSendFlags,
                                   const union sockaddr_union* destinationAddressList,
                                   const size_t                destinationAddresses,
                                   RegistrarIdentifierType     receiverID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type         = EHT_HANDLE_TABLE_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = assocID;
      message->AddressArray = (union sockaddr_union*)destinationAddressList;
      message->Addresses    = destinationAddresses;
      message->Flags        = 0x00;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = receiverID;
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, message) == false) {
         LOG_WARNING
         fputs("Sending HandleTableRequest failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Initialization is complete ##################################### */
static void registrarInitializationComplete(struct Registrar* registrar)
{
   registrar->InInitializationPhase = false;
   if(registrar->ASAPSendAnnounces) {
      LOG_ACTION
      fputs("Starting to send ASAP announcements\n", stdlog);
      LOG_END
      timerStart(&registrar->ASAPAnnounceTimer, 0);
   }
}


/* ###### ENRP peer presence timer callback ############################## */
static void enrpAnnounceTimerCallback(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData)
{
   struct Registrar*              registrar = (struct Registrar*)userData;
   struct ST_CLASS(PeerListNode)* peerListNode;

   if(registrar->InInitializationPhase) {
      LOG_NOTE
      fputs("Initialization phase ended after ENRP mentor discovery timeout. The registrar is ready!\n", stdlog);
      LOG_END
      registrarInitializationComplete(registrar);
   }

   if((registrar->ENRPUseMulticast) || (registrar->ENRPAnnounceViaMulticast)) {
      if(joinOrLeaveMulticastGroup(registrar->ENRPMulticastInputSocket,
                                   &registrar->ENRPMulticastAddress,
                                   true) == false) {
         LOG_WARNING
         fputs("Unable to join multicast group ", stdlog);
         fputaddress(&registrar->ENRPMulticastAddress.sa, true, stdlog);
         fputs(". Registrar will probably be unable to detect peers!\n", stdlog);
         LOG_END;
      }
      sendPeerPresence(registrar, registrar->ENRPMulticastOutputSocket, 0, 0,
                       (union sockaddr_union*)&registrar->ENRPMulticastAddress, 1,
                       0, false);
   }

   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers.List);
   while(peerListNode != NULL) {
      if(!(peerListNode->Flags & PLNF_MULTICAST)) {
         LOG_VERBOSE1
         fprintf(stdlog, "Sending PeerPresence to unicast peer $%08x...\n",
                  peerListNode->Identifier);
         LOG_END
         sendPeerPresence(registrar,
                          registrar->ENRPUnicastSocket, 0, 0,
                          peerListNode->AddressBlock->AddressArray,
                          peerListNode->AddressBlock->Addresses,
                          peerListNode->Identifier,
                          false);
      }
      peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                        &registrar->Peers.List,
                        peerListNode);
   }

   unsigned long long n = getMicroTime() + randomizeCycle(registrar->PeerHeartbeatCycle);
   timerStart(timer, n);
}


/* ###### Takeover expiry timer callback ################################# */
static void takeoverExpiryTimerCallback(struct Dispatcher* dispatcher,
                                        struct Timer*      timer,
                                        void*              userData)
{
   struct Registrar*             registrar = (struct Registrar*)userData;
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct TakeoverProcess*        takeoverProcess =
      takeoverProcessListGetEarliestTakeoverProcess(&registrar->Takeovers);
   size_t                         i;

   CHECK(takeoverProcess != NULL);

   LOG_WARNING
   fprintf(stdlog, "Takeover of peer $%08x has expired, %u outstanding acknowledgements:\n",
           takeoverProcess->TargetID,
           (unsigned int)takeoverProcess->OutstandingAcknowledgements);
   for(i = 0;i < takeoverProcess->OutstandingAcknowledgements;i++) {
      peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                        &registrar->Peers,
                        takeoverProcess->PeerIDArray[i],
                        NULL);
      fprintf(stdlog, "- $%08x (%s)\n",
              takeoverProcess->PeerIDArray[i],
              (peerListNode == NULL) ? "not found!" : "still available");
   }
   LOG_END
   takeoverProcessListDeleteTakeoverProcess(&registrar->Takeovers, takeoverProcess);
}


/* ###### Send Endpoint Keep Alive ####################################### */
static void sendEndpointKeepAlive(struct Registrar*                 registrar,
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
                                      0, 0,
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
                                      0, 0,
                                      message);
      }
      if(result == false) {
         LOG_WARNING
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


/* ###### Handle handlespace management timers ############################# */
static void handlespaceActionTimerCallback(struct Dispatcher* dispatcher,
                                           struct Timer*      timer,
                                           void*              userData)
{
   struct Registrar*                registrar = (struct Registrar*)userData;
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

         sendEndpointKeepAlive(registrar, poolElementNode, false);

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
            sendHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
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


/* ###### Handle peer management timers ################################## */
static void peerActionTimerCallback(struct Dispatcher* dispatcher,
                                    struct Timer*      timer,
                                    void*              userData)
{
   struct Registrar*              registrar = (struct Registrar*)userData;
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct ST_CLASS(PeerListNode)* nextPeerListNode;
   unsigned int                   result;

   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(
                     &registrar->Peers.List);
   while((peerListNode != NULL) &&
         (peerListNode->TimerTimeStamp <= getMicroTime())) {
      nextPeerListNode = ST_CLASS(peerListGetNextPeerListNodeFromTimerStorage)(
                            &registrar->Peers.List,
                            peerListNode);

      if(peerListNode->TimerCode == PLNT_MAX_TIME_LAST_HEARD) {
         LOG_ACTION
         fputs("Peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fprintf(stdlog, " not head since MaxTimeLastHeard=%lluus -> requesting immediate PeerPresence\n",
               registrar->PeerMaxTimeLastHeard);
         LOG_END
         sendPeerPresence(registrar,
                        registrar->ENRPUnicastSocket,
                        0, 0,
                        peerListNode->AddressBlock->AddressArray,
                        peerListNode->AddressBlock->Addresses,
                        peerListNode->Identifier,
                        true);
         ST_CLASS(peerListDeactivateTimer)(
            &registrar->Peers.List,
            peerListNode);
         ST_CLASS(peerListActivateTimer)(
            &registrar->Peers.List,
            peerListNode,
            PLNT_MAX_TIME_NO_RESPONSE,
            getMicroTime() + registrar->PeerMaxTimeNoResponse);
      }
      else if(peerListNode->TimerCode == PLNT_MAX_TIME_NO_RESPONSE) {
         LOG_ACTION
         fputs("Peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fprintf(stdlog, " did not answer in MaxTimeNoResponse=%lluus -> removing it\n",
               registrar->PeerMaxTimeLastHeard);
         LOG_END

         if(ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                     &registrar->Handlespace.Handlespace, peerListNode->Identifier) != NULL) {
            if(takeoverProcessListCreateTakeoverProcess(
                  &registrar->Takeovers,
                  peerListNode->Identifier,
                  &registrar->Peers,
                  getMicroTime() + registrar->TakeoverExpiryInterval) == RSPERR_OKAY) {
               timerRestart(&registrar->TakeoverExpiryTimer,
                           takeoverProcessListGetNextTimerTimeStamp(&registrar->Takeovers));
               LOG_ACTION
               fprintf(stdlog, "Initiating takeover of dead peer $%08x\n",
                       peerListNode->Identifier);
               LOG_END
               sendInitTakeover(registrar, peerListNode->Identifier);
            }
         }
         else {
            LOG_ACTION
            fprintf(stdlog, "Dead peer $%08x has no PEs -> no takeover procedure necessary\n",
                    peerListNode->Identifier);
            LOG_END
         }

         result = ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                     &registrar->Peers,
                     peerListNode);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Peer removal successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Peers:\n", stdlog);
            registrarDumpPeers(registrar);
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

   timerRestart(&registrar->PeerActionTimer,
                ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                   &registrar->Peers));
}


/* ###### Send peer init takeover ######################################## */
static void sendInitTakeover(struct Registrar*             registrar,
                             const RegistrarIdentifierType targetID)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct RSerPoolMessage*        message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type         = EHT_INIT_TAKEOVER;
      message->Flags        = 0x00;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = 0;
      message->RegistrarIdentifier = targetID;

      if(registrar->ENRPUseMulticast) {
         LOG_VERBOSE2
         fputs("Sending InitTakeover via ENRP multicast socket...\n", stdlog);
         LOG_END
         message->ReceiverID    = 0;
         message->AddressArray  = &registrar->ENRPMulticastAddress;
         message->Addresses     = 1;
         if(rserpoolMessageSend(IPPROTO_UDP,
                                registrar->ENRPMulticastOutputSocket,
                                0, 0, 0,
                                message) == false) {
            LOG_WARNING
            fputs("Sending HandleUpdate via ENRP multicast socket failed\n", stdlog);
            LOG_END
         }
      }

      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers.List);
      while(peerListNode != NULL) {
         if(!(peerListNode->Flags & PLNF_MULTICAST)) {
            message->ReceiverID   = peerListNode->Identifier;
            message->AddressArray = peerListNode->AddressBlock->AddressArray;
            message->Addresses    = peerListNode->AddressBlock->Addresses;
            LOG_VERBOSE
            fprintf(stdlog, "Sending InitTakeover to unicast peer $%08x...\n",
                    peerListNode->Identifier);
            LOG_END
            rserpoolMessageSend(IPPROTO_SCTP,
                                registrar->ENRPUnicastSocket,
                                0, 0, 0,
                                message);
         }
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers.List,
                           peerListNode);
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Send peer init takeover acknowledgement ######################## */
static void sendInitTakeoverAck(struct Registrar*             registrar,
                                const int                     sd,
                                const sctp_assoc_t            assocID,
                                const RegistrarIdentifierType receiverID,
                                const RegistrarIdentifierType targetID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type         = EHT_INIT_TAKEOVER_ACK;
      message->AssocID      = assocID;
      message->Flags        = 0x00;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = receiverID;
      message->RegistrarIdentifier = targetID;

      LOG_ACTION
      fprintf(stdlog, "Sending InitTakeoverAck for target $%08x to peer $%08x...\n",
              message->RegistrarIdentifier,
              message->ReceiverID);
      LOG_END
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending InitTakeoverAck failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Send peer name update ########################################## */
static void sendHandleUpdate(struct Registrar*                 registrar,
                             struct ST_CLASS(PoolElementNode)* poolElementNode,
                             const uint16_t                    action)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct RSerPoolMessage*        message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message != NULL) {
      message->Type                     = EHT_HANDLE_UPDATE;
      message->Flags                    = 0x00;
      message->Action                   = action;
      message->SenderID                 = registrar->ServerID;
      message->Handle                   = poolElementNode->OwnerPoolNode->Handle;
      message->PoolElementPtr           = poolElementNode;
      message->PoolElementPtrAutoDelete = false;

      LOG_VERBOSE
      fputs("Sending HandleUpdate for ", stdlog);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fprintf(stdlog, "/$%08x, action $%04x\n", poolElementNode->Identifier, action);
      LOG_END
      LOG_VERBOSE2
      fputs("Updated pool element: ", stdlog);
      ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
      fputs("\n", stdlog);
      LOG_END

      if(registrar->ENRPUseMulticast) {
         LOG_VERBOSE2
         fputs("Sending HandleUpdate via ENRP multicast socket...\n", stdlog);
         LOG_END
         message->ReceiverID    = 0;
         message->AddressArray  = &registrar->ENRPMulticastAddress;
         message->Addresses     = 1;
         if(rserpoolMessageSend(IPPROTO_UDP,
                                registrar->ENRPMulticastOutputSocket,
                                0, 0, 0,
                                message) == false) {
            LOG_WARNING
            fputs("Sending HandleUpdate via ENRP multicast socket failed\n", stdlog);
            LOG_END
         }
      }

      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers.List);
      while(peerListNode != NULL) {
         if(!(peerListNode->Flags & PLNF_MULTICAST)) {
            message->ReceiverID   = peerListNode->Identifier;
            message->AddressArray = peerListNode->AddressBlock->AddressArray;
            message->Addresses    = peerListNode->AddressBlock->Addresses;
            LOG_VERBOSE
            fprintf(stdlog, "Sending HandleUpdate to unicast peer $%08x...\n",
                    peerListNode->Identifier);
            LOG_END
            rserpoolMessageSend(IPPROTO_SCTP,
                                registrar->ENRPUnicastSocket,
                                0, 0, 0,
                                message);
         }
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers.List,
                           peerListNode);
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Update distance for distance-sensitive policies ################ */
static void updateDistance(int                               fd,
                           sctp_assoc_t                      assocID,
                           struct ST_CLASS(PoolElementNode)* poolElementNode,
                           bool                              addDistance,
                           unsigned int*                     distance)
{
   struct sctp_status assocStatus;
   socklen_t          assocStatusLength;

   /* ====== Set distance for distance-sensitive policies ======= */
   if((poolElementNode->PolicySettings.PolicyType == PPT_LEASTUSED_DPF) ||
       (poolElementNode->PolicySettings.PolicyType == PPT_WEIGHTED_RANDOM_DPF)) {
      if(*distance == 0xffffffff) {
         assocStatusLength = sizeof(assocStatus);
         assocStatus.sstat_assoc_id = assocID;
         if(ext_getsockopt(fd, IPPROTO_SCTP, SCTP_STATUS, (char*)&assocStatus, &assocStatusLength) == 0) {
            printf("SRTT-HUPDT=%d\n", assocStatus.sstat_primary.spinfo_srtt);
            *distance = assocStatus.sstat_primary.spinfo_srtt / 2;
         }
         else {
            LOG_WARNING
                  logerror("Unable to obtain SCTP_STATUS");
            LOG_END
            *distance = 0;
         }
      }
      else printf("----REUSE!!!!!!!!!!!\n");

      if(addDistance) {
         poolElementNode->PolicySettings.Distance += *distance;
      }
      else {
         poolElementNode->PolicySettings.Distance = *distance;
      }
      printf("DIST=%d meas=%d\n", poolElementNode->PolicySettings.Distance, *distance);
   }
   else {
      *distance = 0;
   }
}


/* ###### Handle registration request #################################### */
static void handleRegistrationRequest(struct Registrar*       registrar,
                                      int                     fd,
                                      sctp_assoc_t            assocID,
                                      struct RSerPoolMessage* message)
{

   char                              remoteAddressBlockBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     remoteAddressBlock = (struct TransportAddressBlock*)&remoteAddressBlockBuffer;
   char                              asapTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     asapTransportAddressBlock = (struct TransportAddressBlock*)&asapTransportAddressBlockBuffer;
   char                              userTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     userTransportAddressBlock = (struct TransportAddressBlock*)&userTransportAddressBlockBuffer;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   unsigned int                      distance;

   LOG_VERBOSE
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
                                                      MAX_NS_TRANSPORTADDRESSES,
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
                                     MAX_NS_TRANSPORTADDRESSES, false, MIN_ENDPOINT_ADDRESS_SCOPE) > 0) {
         LOG_VERBOSE2
         fputs("Filtered user transport addresses: ", stdlog);
         transportAddressBlockPrint(userTransportAddressBlock, stdlog);
         fputs("\n", stdlog);
         LOG_END
         if(transportAddressBlockFilter(remoteAddressBlock,
                                        NULL,
                                        asapTransportAddressBlock,
                                        MAX_NS_TRANSPORTADDRESSES, true, MIN_ENDPOINT_ADDRESS_SCOPE) > 0) {
            LOG_VERBOSE2
            fputs("Filtered ASAP transport addresses: ", stdlog);
            transportAddressBlockPrint(asapTransportAddressBlock, stdlog);
            fputs("\n", stdlog);
            LOG_END

            /* ====== Set distance for distance-sensitive policies ======= */
            distance = 0xffffffff;
            updateDistance(fd, assocID, message->PoolElementPtr, false, &distance);

            message->Error = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                              &registrar->Handlespace,
                              &message->Handle,
                              registrar->ServerID,
                              message->PoolElementPtr->Identifier,
                              message->PoolElementPtr->RegistrationLife,
                              &message->PoolElementPtr->PolicySettings,
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
               sendHandleUpdate(registrar, poolElementNode, PNUP_ADD_PE);
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
      if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, message) == false) {
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


/* ###### Handle deregistration request ################################## */
static void handleDeregistrationRequest(struct Registrar*       registrar,
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

            if(delPoolElementNode.HomeRegistrarIdentifier == registrar->ServerID) {
               /* We own this PE -> send HandleUpdate for its removal. */
               sendHandleUpdate(registrar, &delPoolElementNode, PNUP_DEL_PE);
            }

            LOG_ACTION
            fputs("Deregistration successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Handlespace content:\n", stdlog);
            registrarDumpHandlespace(registrar);
            LOG_END
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

   if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, message) == false) {
      LOG_WARNING
      logerror("Sending DeregistrationResponse failed");
      LOG_END
   }
}


/* ###### Handle handle resolution request ################################# */
#define HANDLERESOLUTION_MAX_HANDLE_RESOLUTION_ITEMS 16
#define HANDLERESOLUTION_MAX_INCREMENT              1
static void handleHandleResolutionRequest(struct Registrar*       registrar,
                                          int                     fd,
                                          sctp_assoc_t            assocID,
                                          struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNodeArray[HANDLERESOLUTION_MAX_HANDLE_RESOLUTION_ITEMS];
   size_t                            poolElementNodes = HANDLERESOLUTION_MAX_HANDLE_RESOLUTION_ITEMS;
   size_t                            i;

   LOG_ACTION
   fprintf(stdlog, "Handle Resolution request for pool ");
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message->Type  = AHT_HANDLE_RESOLUTION_RESPONSE;
   message->Flags = 0x00;
   message->Error = ST_CLASS(poolHandlespaceManagementHandleResolution)(
                       &registrar->Handlespace,
                       &message->Handle,
                       (struct ST_CLASS(PoolElementNode)**)&poolElementNodeArray,
                       &poolElementNodes,
                       HANDLERESOLUTION_MAX_HANDLE_RESOLUTION_ITEMS,
                       HANDLERESOLUTION_MAX_INCREMENT);
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

   if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, message) == false) {
      LOG_WARNING
      logerror("Sending handle resolution response failed");
      LOG_END
   }
}


/* ###### Handle endpoint keepalive acknowledgement ###################### */
static void handleEndpointKeepAliveAck(struct Registrar*       registrar,
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


/* ###### Handle endpoint unreachable #################################### */
static void handleEndpointUnreachable(struct Registrar*       registrar,
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
            sendHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
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


/* ###### Handle peer name update ######################################## */
static void handleHandleUpdate(struct Registrar*       registrar,
                               int                     fd,
                               sctp_assoc_t            assocID,
                               struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   unsigned int                      distance;
   int                               result;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own HandleUpdate message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fputs("Got HandleUpdate for ", stdlog);
   poolHandlePrint(&message->Handle, stdlog);
   fprintf(stdlog, "/$%08x, action $%04x\n",
           message->PoolElementPtr->Identifier, message->Action);
   LOG_END
   LOG_VERBOSE2
   fputs("Updated pool element: ", stdlog);
   ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
   fputs("\n", stdlog);
   LOG_END

   if(message->Action == PNUP_ADD_PE) {
      if(message->PoolElementPtr->HomeRegistrarIdentifier != registrar->ServerID) {
         /* ====== Set distance for distance-sensitive policies ======= */
         distance = 0xffffffff;
         updateDistance(fd, assocID, message->PoolElementPtr, true, &distance);

         result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                     &registrar->Handlespace,
                     &message->Handle,
                     message->PoolElementPtr->HomeRegistrarIdentifier,
                     message->PoolElementPtr->Identifier,
                     message->PoolElementPtr->RegistrationLife,
                     &message->PoolElementPtr->PolicySettings,
                     message->PoolElementPtr->UserTransport,
                     message->PoolElementPtr->RegistratorTransport,
                     -1, 0,
                     getMicroTime(),
                     &newPoolElementNode);
         if(result == RSPERR_OKAY) {
            LOG_ACTION
            fputs("Successfully registered ", stdlog);
            poolHandlePrint(&message->Handle, stdlog);
            fprintf(stdlog, "/$%08x\n", newPoolElementNode->Identifier);
            LOG_END
            LOG_VERBOSE2
            fputs("Registered pool element: ", stdlog);
            ST_CLASS(poolElementNodePrint)(newPoolElementNode, stdlog, PENPO_FULL);
            fputs("\n", stdlog);
            LOG_END

            if(STN_METHOD(IsLinked)(&newPoolElementNode->PoolElementTimerStorageNode)) {
               ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
                  &registrar->Handlespace.Handlespace,
                  newPoolElementNode);
            }
            ST_CLASS(poolHandlespaceNodeActivateTimer)(
               &registrar->Handlespace.Handlespace,
               newPoolElementNode,
               PENT_EXPIRY,
               getMicroTime() + (1000ULL * newPoolElementNode->RegistrationLife));
            timerRestart(&registrar->HandlespaceActionTimer,
                         ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                            &registrar->Handlespace));

            LOG_VERBOSE3
            fputs("Handlespace content:\n", stdlog);
            registrarDumpHandlespace(registrar);
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
         fprintf(stdlog, "PR $%08x sent me a HandleUpdate for a PE owned by myself!",
                 message->SenderID);
         LOG_END
      }
   }

   else if(message->Action == PNUP_DEL_PE) {
      result = ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
                  &registrar->Handlespace,
                  &message->Handle,
                  message->PoolElementPtr->Identifier);
      if(message->Error == RSPERR_OKAY) {
         LOG_ACTION
         fputs("Successfully deregistered ", stdlog);
         poolHandlePrint(&message->Handle, stdlog);
         fprintf(stdlog, "/$%08x\n", message->PoolElementPtr->Identifier);
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
      fprintf(stdlog, "Got HandleUpdate with invalid action $%04x\n",
              message->Action);
      LOG_END
   }
}


/* ###### Handle presence ################################################ */
static void handlePeerPresence(struct Registrar*       registrar,
                               int                     fd,
                               sctp_assoc_t            assocID,
                               struct RSerPoolMessage* message)
{
   HandlespaceChecksumType        checksum;
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct TakeoverProcess*        takeoverProcess;
   char                           remoteAddressBlockBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*  remoteAddressBlock = (struct TransportAddressBlock*)&remoteAddressBlockBuffer;
   char                           enrpTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*  enrpTransportAddressBlock = (struct TransportAddressBlock*)&enrpTransportAddressBlockBuffer;
   int                            result;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own PeerPresence message\n", stdlog);
      LOG_END
      return;
   }

   LOG_ACTION
   fprintf(stdlog, "Got PeerPresence from peer $%08x",
           message->PeerListNodePtr->Identifier);
   if(message->PeerListNodePtr) {
      fputs(" at address ", stdlog);
      transportAddressBlockPrint(message->PeerListNodePtr->AddressBlock, stdlog);
   }
   fputs("\n", stdlog);
   LOG_END


   /* ====== Filter addresses ============================================ */
   if(fd == registrar->ENRPMulticastInputSocket) {
      transportAddressBlockNew(enrpTransportAddressBlock,
                               IPPROTO_SCTP,
                               getPort(&message->SourceAddress.sa),
                               0,
                               &message->SourceAddress,
                               1);
   }
   else {
      if(transportAddressBlockGetAddressesFromSCTPSocket(remoteAddressBlock,
                                                         fd, assocID,
                                                         MAX_NS_TRANSPORTADDRESSES,
                                                         false) > 0) {
         LOG_VERBOSE3
         fputs("SCTP association's valid peer addresses: ", stdlog);
         transportAddressBlockPrint(remoteAddressBlock, stdlog);
         fputs("\n", stdlog);
         LOG_END

         /* ====== Filter addresses ====================================== */
         if(transportAddressBlockFilter(message->PeerListNodePtr->AddressBlock,
                                        remoteAddressBlock,
                                        enrpTransportAddressBlock,
                                        MAX_NS_TRANSPORTADDRESSES, true, MIN_ENDPOINT_ADDRESS_SCOPE) == 0) {
            LOG_WARNING
            fprintf(stdlog, "PeerPresence from peer $%08x contains no usable ENRP endpoint addresses\n",
                    message->PeerListNodePtr->Identifier);
            fputs("Addresses from message: ", stdlog);
            transportAddressBlockPrint(message->PeerListNodePtr->AddressBlock, stdlog);
            fputs("\n", stdlog);
            fputs("Addresses from association: ", stdlog);
            transportAddressBlockPrint(remoteAddressBlock, stdlog);
            fputs("\n", stdlog);
            LOG_END
            return;
         }
      }
      else {
         LOG_ERROR
         fprintf(stdlog, "Unable to obtain peer addresses of FD %d, assoc %u\n",
               fd, (unsigned int)assocID);
         LOG_END
      }
   }
   LOG_VERBOSE2
   fprintf(stdlog, "Using the following ENRP endpoint addresses for peer $%08x: ",
           message->PeerListNodePtr->Identifier);
   transportAddressBlockPrint(enrpTransportAddressBlock, stdlog);
   fputs("\n", stdlog);
   LOG_END


   /* ====== Add/update peer ============================================= */
   if(message->PeerListNodePtr) {
      int flags = PLNF_DYNAMIC;
      if((registrar->ENRPUseMulticast) && (fd == registrar->ENRPMulticastOutputSocket)) {
         flags |= PLNF_MULTICAST;
      }
      result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                  &registrar->Peers,
                  message->PeerListNodePtr->Identifier,
                  flags,
                  enrpTransportAddressBlock,
                  getMicroTime(),
                  &peerListNode);

      if(result == RSPERR_OKAY) {
         /* ====== Checksum handling ===================================== */
         LOG_VERBOSE2
         fputs("Successfully added peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fputs("\n", stdlog);
         LOG_END

         /* ====== Check if synchronization is necessary ================= */
         if(assocID != 0) {
            /* Attention: We only synchronize on SCTP-received Presence messages! */
            checksum = ST_CLASS(peerListNodeGetOwnershipChecksum)(
                          peerListNode);
            if(checksum != message->Checksum) {
               LOG_ACTION
               fprintf(stdlog, "Synchronization with peer $%08x is necessary -> requesting handle table...\n",
                       peerListNode->Identifier);
               LOG_END
               sendHandleTableRequest(registrar, fd, assocID, 0,
                                      NULL, 0,
                                      peerListNode->Identifier);
            }
         }

         /* ====== Activate keep alive timer ============================= */
         if(STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode)) {
            ST_CLASS(peerListDeactivateTimer)(
               &registrar->Peers.List,
               peerListNode);
         }
         ST_CLASS(peerListActivateTimer)(
            &registrar->Peers.List,
            peerListNode,
            PLNT_MAX_TIME_LAST_HEARD,
            getMicroTime() + registrar->PeerMaxTimeLastHeard);
         timerRestart(&registrar->PeerActionTimer,
                      ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                         &registrar->Peers));

         /* ====== Print debug information =============================== */
         LOG_VERBOSE3
         fputs("Peers:\n", stdlog);
         registrarDumpPeers(registrar);
         LOG_END

         if((message->Flags & EHF_PRESENCE_REPLY_REQUIRED) &&
            (fd != registrar->ENRPMulticastOutputSocket) &&
            (message->SenderID != 0)) {
            LOG_VERBOSE
            fputs("PeerPresence with ReplyRequired flag set -> Sending reply...\n", stdlog);
            LOG_END
            sendPeerPresence(registrar,
                             fd, message->AssocID, 0,
                             NULL, 0,
                             message->SenderID,
                             false);
         }

         /* ====== Initialization phase ================================== */
         if((registrar->InInitializationPhase) &&
            (registrar->MentorServerID == 0)) {
            LOG_ACTION
            fputs("Trying ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
            fputs(" as mentor server...\n", stdlog);
            LOG_END
            registrar->MentorServerID = peerListNode->Identifier;
            sendPeerPresence(registrar,
                             registrar->ENRPUnicastSocket,
                             0, 0,
                             peerListNode->AddressBlock->AddressArray,
                             peerListNode->AddressBlock->Addresses,
                             peerListNode->Identifier,
                             false);
            sendListRequest(registrar,
                            registrar->ENRPUnicastSocket,
                            0, 0,
                            peerListNode->AddressBlock->AddressArray,
                            peerListNode->AddressBlock->Addresses,
                            peerListNode->Identifier);
            sendHandleTableRequest(registrar,
                                   registrar->ENRPUnicastSocket,
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


   /* ====== Takeover handling =========================================== */
   if((takeoverProcess = takeoverProcessListFindTakeoverProcess(&registrar->Takeovers,
                                                                message->SenderID))) {
      LOG_ACTION
      fprintf(stdlog, "Peer $%08x is alive again. Aborting its takeover.\n",
              message->SenderID);
      LOG_END
      takeoverProcessListDeleteTakeoverProcess(&registrar->Takeovers,
                                                takeoverProcess);
      timerRestart(&registrar->TakeoverExpiryTimer,
                   takeoverProcessListGetNextTimerTimeStamp(&registrar->Takeovers));
   }
}


/* ###### Handle peer init takeover ###################################### */
static void handleInitTakeover(struct Registrar*       registrar,
                               int                     fd,
                               sctp_assoc_t            assocID,
                               struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct TakeoverProcess*        takeoverProcess;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own InitTakeover message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got InitTakeover from peer $%08x for target $%08x\n",
           message->SenderID,
           message->RegistrarIdentifier);
   LOG_END

   /* ====== This server is target -> try to stop it by peer presences === */
   if(message->RegistrarIdentifier == registrar->ServerID) {
      LOG_WARNING
      fprintf(stdlog, "Peer $%08x has initiated takeover -> trying to stop this by PeerPresences\n",
              message->SenderID);
      LOG_END
      if((registrar->ENRPUseMulticast) || (registrar->ENRPAnnounceViaMulticast)) {
         sendPeerPresence(registrar, registrar->ENRPMulticastOutputSocket, 0, 0,
                          (union sockaddr_union*)&registrar->ENRPMulticastAddress, 1,
                          0, false);
      }

      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers.List);
      while(peerListNode != NULL) {
         if(!(peerListNode->Flags & PLNF_MULTICAST)) {
            LOG_VERBOSE
            fprintf(stdlog, "Sending PeerPresence to unicast peer $%08x...\n",
                     peerListNode->Identifier);
            LOG_END
            sendPeerPresence(registrar,
                             registrar->ENRPUnicastSocket, 0, 0,
                             peerListNode->AddressBlock->AddressArray,
                             peerListNode->AddressBlock->Addresses,
                             peerListNode->Identifier,
                             false);
         }
         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers.List,
                           peerListNode);
      }
   }

   /* ====== Another peer tries takeover, too ============================ */
   else if((takeoverProcess = takeoverProcessListFindTakeoverProcess(
                                 &registrar->Takeovers,
                                 message->RegistrarIdentifier))) {
      if(message->SenderID < registrar->ServerID) {
         LOG_ACTION
         fprintf(stdlog, "Peer $%08x, also trying takeover of $%08x, has lower ID -> we ($%08x) abort our takeover\n",
                 message->SenderID,
                 message->RegistrarIdentifier,
                 registrar->ServerID);
         LOG_END
         /* Acknowledge peer's takeover */
         sendInitTakeoverAck(registrar,
                                 fd, assocID,
                                 message->SenderID, message->RegistrarIdentifier);
         /* Cancel our takeover process */
         takeoverProcessListDeleteTakeoverProcess(&registrar->Takeovers,
                                                  takeoverProcess);
         timerRestart(&registrar->TakeoverExpiryTimer,
                      takeoverProcessListGetNextTimerTimeStamp(&registrar->Takeovers));
      }
      else {
         LOG_ACTION
         fprintf(stdlog, "Peer $%08x, also trying takeover of $%08x, has higher ID -> we ($%08x) continue our takeover\n",
                 message->SenderID,
                 message->RegistrarIdentifier,
                 registrar->ServerID);
         LOG_END
      }
   }

   /* ====== Acknowledge takeover ======================================== */
   else {
      LOG_ACTION
      fprintf(stdlog, "Acknowledging peer $%08x's takeover of $%08x\n",
              message->SenderID,
              message->RegistrarIdentifier);
      LOG_END
      sendInitTakeoverAck(registrar,
                              fd, assocID,
                              message->SenderID, message->RegistrarIdentifier);
   }
}


/* ###### Send ENRP peer takeover server message ######################### */
static void sendTakeoverServer(struct Registrar*             registrar,
                               int                           sd,
                               const sctp_assoc_t            assocID,
                               int                           msgSendFlags,
                               const union sockaddr_union*   destinationAddressList,
                               const size_t                  destinationAddresses,
                               const RegistrarIdentifierType receiverID,
                               const RegistrarIdentifierType targetID)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type                = EHT_TAKEOVER_SERVER;
      message->PPID                = PPID_ENRP;
      message->AssocID             = assocID;
      message->AddressArray        = (union sockaddr_union*)destinationAddressList;
      message->Addresses           = destinationAddresses;
      message->Flags               = 0x00;
      message->SenderID            = registrar->ServerID;
      message->ReceiverID          = receiverID;
      message->RegistrarIdentifier = targetID;

      if(rserpoolMessageSend((sd == registrar->ENRPMulticastOutputSocket) ? IPPROTO_UDP : IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, message) == false) {
         LOG_WARNING
         fputs("Sending TakeoverServer failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Do takeover of peer server ##################################### */
static void doTakeover(struct Registrar*       registrar,
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
   if((registrar->ENRPUseMulticast) || (registrar->ENRPAnnounceViaMulticast)) {
      sendTakeoverServer(registrar, registrar->ENRPMulticastOutputSocket, 0, 0,
                             (union sockaddr_union*)&registrar->ENRPMulticastAddress, 1,
                             0,
                             takeoverProcess->TargetID);
   }

   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers.List);
   while(peerListNode != NULL) {
      if(!(peerListNode->Flags & PLNF_MULTICAST)) {
         LOG_VERBOSE
         fprintf(stdlog, "Sending TakeoverServer to unicast peer $%08x...\n",
                  peerListNode->Identifier);
         LOG_END
         sendTakeoverServer(registrar,
                                registrar->ENRPUnicastSocket, 0, 0,
                                peerListNode->AddressBlock->AddressArray,
                                peerListNode->AddressBlock->Addresses,
                                peerListNode->Identifier,
                                takeoverProcess->TargetID);
      }
      peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                        &registrar->Peers.List,
                        peerListNode);
   }

   /* ====== Update PEs' home PR identifier ============================== */
   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &registrar->Handlespace.Handlespace, takeoverProcess->TargetID);
   while(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                               &registrar->Handlespace.Handlespace, poolElementNode);

      LOG_ACTION
      fprintf(stdlog, "Taking ownership of pool element $%08x in pool ",
              poolElementNode->Identifier);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END

      /* Update handlespace */
      ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
          &registrar->Handlespace.Handlespace,
          poolElementNode,
          registrar->ServerID);

      /* Tell node about new home PR */
      sendEndpointKeepAlive(registrar, poolElementNode, true);

      poolElementNode = nextPoolElementNode;
   }

   /* ====== Remove takeover process ===================================== */
   LOG_WARNING
   fprintf(stdlog, "Takeover of peer $%08x completed\n",
           takeoverProcess->TargetID);
   LOG_END
   takeoverProcessListDeleteTakeoverProcess(&registrar->Takeovers,
                                            takeoverProcess);
   timerRestart(&registrar->TakeoverExpiryTimer,
                takeoverProcessListGetNextTimerTimeStamp(&registrar->Takeovers));
}


/* ###### Handle peer init takeover ack ################################## */
static void handleInitTakeoverAck(struct Registrar*       registrar,
                                  int                     fd,
                                  sctp_assoc_t            assocID,
                                  struct RSerPoolMessage* message)
{
   struct TakeoverProcess* takeoverProcess;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own InitTakeoverAck message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got InitTakeoverAck from peer $%08x for target $%08x\n",
           message->SenderID,
           message->RegistrarIdentifier);
   LOG_END

   takeoverProcess = takeoverProcessListAcknowledgeTakeoverProcess(
                        &registrar->Takeovers,
                        message->RegistrarIdentifier,
                        message->SenderID);
   if(takeoverProcess) {
      LOG_ACTION
      fprintf(stdlog, "Peer $%08x acknowledges takeover of target $%08x. %u acknowledges to go.\n",
              message->SenderID,
              message->RegistrarIdentifier,
              (unsigned int)takeoverProcess->OutstandingAcknowledgements);
      LOG_END
      if(takeoverProcess->OutstandingAcknowledgements == 0) {
         LOG_ACTION
         fprintf(stdlog, "Takeover of target $%08x confirmed. Taking it over now.\n",
                 message->RegistrarIdentifier);
         LOG_END

         doTakeover(registrar, takeoverProcess);
      }
   }
   else {
      LOG_VERBOSE
      fprintf(stdlog, "Ignoring InitTakeoverAck from peer $%08x for target $%08x\n",
              message->SenderID,
              message->RegistrarIdentifier);
      LOG_END
   }
}


/* ###### Handle peer takeover server #################################### */
static void handleTakeoverServer(struct Registrar*       registrar,
                                 int                     fd,
                                 sctp_assoc_t            assocID,
                                 struct RSerPoolMessage* message)
{
   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own TakeoverServer message\n", stdlog);
      LOG_END
      return;
   }

   LOG_ACTION
   fprintf(stdlog, "Got TakeoverServer from peer $%08x for target $%08x\n",
           message->SenderID,
           message->RegistrarIdentifier);
   LOG_END
}


/* ###### Handle peer list request ####################################### */
static void handleListRequest(struct Registrar*       registrar,
                              int                     fd,
                              sctp_assoc_t            assocID,
                              struct RSerPoolMessage* message)
{
   struct RSerPoolMessage* response;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own ListRequest message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got ListRequest from peer $%08x\n",
           message->SenderID);
   LOG_END

   response = rserpoolMessageNew(NULL, 65536);
   if(response != NULL) {
      response->Type       = EHT_LIST_RESPONSE;
      response->AssocID    = assocID;
      response->Flags      = 0x00;
      response->SenderID   = registrar->ServerID;
      response->ReceiverID = message->SenderID;

      response->PeerListPtr           = &registrar->Peers;
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
static void handleListResponse(struct Registrar*       registrar,
                               int                     fd,
                               sctp_assoc_t            assocID,
                               struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct ST_CLASS(PeerListNode)* newPeerListNode;
   int                            result;

   if(message->SenderID == registrar->ServerID) {
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

   if(!(message->Flags & EHF_LIST_RESPONSE_REJECT)) {
      if(message->PeerListPtr) {
         peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                           &message->PeerListPtr->List);
         while(peerListNode) {
            LOG_ACTION
            fputs("Trying to add peer ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, ~0);
            fputs(" to peer list\n", stdlog);
            LOG_END
            if(peerListNode->Identifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
               result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                           &registrar->Peers,
                           peerListNode->Identifier,
                           peerListNode->Flags,
                           peerListNode->AddressBlock,
                           getMicroTime(),
                           &newPeerListNode);
               if((result == RSPERR_OKAY) &&
                  (!STN_METHOD(IsLinked)(&newPeerListNode->PeerListTimerStorageNode))) {
                  /* ====== Activate keep alive timer ==================== */
                  ST_CLASS(peerListActivateTimer)(
                     &registrar->Peers.List,
                     newPeerListNode,
                     PLNT_MAX_TIME_LAST_HEARD,
                     getMicroTime() + registrar->PeerMaxTimeLastHeard);

                  /* ====== New peer -> Send Peer Presence =============== */
                  sendPeerPresence(registrar,
                                   registrar->ENRPUnicastSocket, 0, 0,
                                   newPeerListNode->AddressBlock->AddressArray,
                                   newPeerListNode->AddressBlock->Addresses,
                                   newPeerListNode->Identifier,
                                   false);
               }
            }
            else {
               LOG_ACTION
               fputs("Skipping unknown peer (due to ID=0) from PeerListResponse: ", stdlog);
               ST_CLASS(peerListNodePrint)(peerListNode, stdlog, ~0);
               fputs("\n", stdlog);
               LOG_END
            }

            peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                              &message->PeerListPtr->List, peerListNode);
         }
      }

      timerRestart(&registrar->PeerActionTimer,
                   ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                      &registrar->Peers));

      /* ====== Print debug information ============================ */
      LOG_VERBOSE3
      fputs("Peers:\n", stdlog);
      registrarDumpPeers(registrar);
      LOG_END
   }
   else {
      LOG_ACTION
      fprintf(stdlog, "Rejected PeerListResponse from peer $%08x\n",
              message->SenderID);
      LOG_END
   }
}


/* ###### Handle peer handle table request ################################# */
static void handleHandleTableRequest(struct Registrar*       registrar,
                                     int                     fd,
                                     sctp_assoc_t            assocID,
                                     struct RSerPoolMessage* message)
{
   struct RSerPoolMessage*        response;
   struct ST_CLASS(PeerListNode)* peerListNode;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own HandleTableRequest message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got HandleTableRequest from peer $%08x\n",
           message->SenderID);
   LOG_END

   peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                     &registrar->Peers,
                     message->SenderID,
                     NULL);

   /* We allow only 1400 bytes per HandleTableResponse... */
   response = rserpoolMessageNew(NULL, 1400);
   if(response != NULL) {
      response->Type                      = EHT_HANDLE_TABLE_RESPONSE;
      response->AssocID                   = assocID;
      response->Flags                     = 0x00;
      response->SenderID                  = registrar->ServerID;
      response->ReceiverID                = message->SenderID;
      response->Action                    = message->Flags;

      response->PeerListNodePtr           = peerListNode;
      response->PeerListNodePtrAutoDelete = false;
      response->HandlespacePtr            = &registrar->Handlespace;
      response->HandlespacePtrAutoDelete  = false;

      if(peerListNode == NULL) {
         response->Flags |= EHF_HANDLE_TABLE_RESPONSE_REJECT;
         LOG_WARNING
         fprintf(stdlog, "HandleTableRequest from peer $%08x -> This peer is unknown, rejecting request!\n",
                 message->SenderID);
         LOG_END
      }

      LOG_VERBOSE
      fprintf(stdlog, "Sending HandleTableResponse to peer $%08x...\n",
              message->SenderID);
      LOG_END
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             fd, assocID, 0, 0, response) == false) {
         LOG_WARNING
         fputs("Sending HandleTableResponse failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(response);
   }
}


/* ###### Handle handle table response ##################################### */
static void handleHandleTableResponse(struct Registrar*       registrar,
                                      int                     fd,
                                      sctp_assoc_t            assocID,
                                      struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   unsigned int                      distance;
   unsigned int                      result;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own HandleTableResponse message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got HandleTableResponse from peer $%08x\n",
           message->SenderID);
   LOG_END

   if(!(message->Flags & EHF_HANDLE_TABLE_RESPONSE_REJECT)) {
      if(message->HandlespacePtr) {
         distance = 0xffffffff;
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(&message->HandlespacePtr->Handlespace);
         while(poolElementNode != NULL) {
            /* ====== Set distance for distance-sensitive policies ======= */
            updateDistance(fd, assocID, poolElementNode, true, &distance);

            if(poolElementNode->HomeRegistrarIdentifier != registrar->ServerID) {
               result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                           &registrar->Handlespace,
                           &poolElementNode->OwnerPoolNode->Handle,
                           poolElementNode->HomeRegistrarIdentifier,
                           poolElementNode->Identifier,
                           poolElementNode->RegistrationLife,
                           &poolElementNode->PolicySettings,
                           poolElementNode->UserTransport,
                           poolElementNode->RegistratorTransport,
                           -1, 0,
                           getMicroTime(),
                           &newPoolElementNode);
               if(result == RSPERR_OKAY) {
                  LOG_ACTION
                  fputs("Successfully registered ", stdlog);
                  poolHandlePrint(&message->Handle, stdlog);
                  fprintf(stdlog, "/$%08x\n", poolElementNode->Identifier);
                  LOG_END
                  LOG_VERBOSE2
                  fputs("Registered pool element: ", stdlog);
                  ST_CLASS(poolElementNodePrint)(newPoolElementNode, stdlog, PENPO_FULL);
                  fputs("\n", stdlog);
                  LOG_END

                  if(!STN_METHOD(IsLinked)(&newPoolElementNode->PoolElementTimerStorageNode)) {
                     ST_CLASS(poolHandlespaceNodeActivateTimer)(
                        &registrar->Handlespace.Handlespace,
                        newPoolElementNode,
                        PENT_EXPIRY,
                        getMicroTime() + (1000ULL * newPoolElementNode->RegistrationLife));
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
               fprintf(stdlog, "PR $%08x sent me a HandleTableResponse containing a PE owned by myself: ",
                       message->SenderID);
               ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
               fputs("\n", stdlog);
               LOG_END
            }
            poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(&message->HandlespacePtr->Handlespace, poolElementNode);
         }

         timerRestart(&registrar->HandlespaceActionTimer,
                      ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                         &registrar->Handlespace));


         LOG_VERBOSE3
         fputs("Handlespace content:\n", stdlog);
         registrarDumpHandlespace(registrar);
         fputs("Peers:\n", stdlog);
         registrarDumpPeers(registrar);
         LOG_END


         if(message->Flags & EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND) {
            LOG_VERBOSE
            fprintf(stdlog, "HandleTableResponse has MoreToSend flag set -> sending HandleTableRequest to peer $%08x to get more data\n",
                    message->SenderID);
            LOG_END
            sendHandleTableRequest(registrar, fd, message->AssocID, 0, NULL, 0, message->SenderID);
         }
         else {
            if((registrar->InInitializationPhase) &&
               (registrar->MentorServerID == message->SenderID)) {
               registrar->InInitializationPhase = false;
               LOG_NOTE
               fputs("Initialization phase ended after obtaining handlespace from mentor server. The registrar is ready!\n", stdlog);
               LOG_END
               registrarInitializationComplete(registrar);
            }
         }
      }
   }
   else {
      LOG_ACTION
      fprintf(stdlog, "Rejected HandleTableResponse from peer $%08x\n",
              message->SenderID);
      LOG_END
   }
}


/* ###### Handle incoming message ######################################## */
static void handleMessage(struct Registrar*       registrar,
                          struct RSerPoolMessage* message,
                          int                     sd)
{
   switch(message->Type) {
      case AHT_REGISTRATION:
         handleRegistrationRequest(registrar, sd, message->AssocID, message);
       break;
      case AHT_HANDLE_RESOLUTION:
         handleHandleResolutionRequest(registrar, sd, message->AssocID, message);
       break;
      case AHT_DEREGISTRATION:
         handleDeregistrationRequest(registrar, sd, message->AssocID, message);
       break;
      case AHT_ENDPOINT_KEEP_ALIVE_ACK:
         handleEndpointKeepAliveAck(registrar, sd, message->AssocID, message);
       break;
      case AHT_ENDPOINT_UNREACHABLE:
         handleEndpointUnreachable(registrar, sd, message->AssocID, message);
       break;
      case EHT_PRESENCE:
         handlePeerPresence(registrar, sd, message->AssocID, message);
       break;
      case EHT_HANDLE_UPDATE:
         handleHandleUpdate(registrar, sd, message->AssocID, message);
       break;
      case EHT_LIST_REQUEST:
         handleListRequest(registrar, sd, message->AssocID, message);
       break;
      case EHT_LIST_RESPONSE:
         handleListResponse(registrar, sd, message->AssocID, message);
       break;
      case EHT_HANDLE_TABLE_REQUEST:
         handleHandleTableRequest(registrar, sd, message->AssocID, message);
       break;
      case EHT_HANDLE_TABLE_RESPONSE:
         handleHandleTableResponse(registrar, sd, message->AssocID, message);
       break;
      case EHT_INIT_TAKEOVER:
         handleInitTakeover(registrar, sd, message->AssocID, message);
       break;
      case EHT_INIT_TAKEOVER_ACK:
         handleInitTakeoverAck(registrar, sd, message->AssocID, message);
       break;
      case EHT_TAKEOVER_SERVER:
         handleTakeoverServer(registrar, sd, message->AssocID, message);
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
static void registrarSocketCallback(struct Dispatcher* dispatcher,
                                    int                fd,
                                    unsigned int       eventMask,
                                    void*              userData)
{
   struct Registrar*        registrar = (struct Registrar*)userData;
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
   unsigned int             result;

   if((fd == registrar->ASAPSocket) ||
      (fd == registrar->ENRPUnicastSocket) ||
      (((registrar->ENRPUseMulticast) || (registrar->ENRPAnnounceViaMulticast)) &&
        (fd == registrar->ENRPMulticastInputSocket))) {
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
            if(fd == registrar->ENRPMulticastInputSocket) {
               /* ENRP via UDP -> Set PPID so that rserpoolPacket2Message can
                  correctly decode the packet */
               ppid = PPID_ENRP;
            }

            result = rserpoolPacket2Message(buffer, &remoteAddress, assocID, ppid,
                                            received, sizeof(buffer), &message);
            if(message != NULL) {
               if(result == RSPERR_OKAY) {
                  message->BufferAutoDelete = false;
                  LOG_VERBOSE3
                  fprintf(stdlog, "Got %u bytes message from ", (unsigned int)message->BufferSize);
                  fputaddress((struct sockaddr*)&remoteAddress, true, stdlog);
                  fprintf(stdlog, ", assoc #%u, PPID $%x\n",
                          (unsigned int)message->AssocID, message->PPID);
                  LOG_END

                  handleMessage(registrar, message, fd);
               }
               else {
                  if((ppid == PPID_ASAP) || (ppid == PPID_ENRP)) {
                     /* For ASAP or ENRP messages, we can reply
                        error message */
                     if(message->PPID == PPID_ASAP) {
                        message->Type = AHT_ERROR;
                     }
                     else if(message->PPID == PPID_ENRP) {
                        message->Type = EHT_ERROR;
                     }
                     rserpoolMessageSend(IPPROTO_SCTP,
                                         fd, assocID, 0, 0, message);
                  }
               }
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
                             registrar->ASAPSocket,
                             (unsigned int)notification->sn_assoc_change.sac_assoc_id);

                     LOG_END
                     registrarRemovePoolElementsOfConnection(registrar, fd,
                                                             notification->sn_assoc_change.sac_assoc_id);
                  }
                  else if(notification->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP) {
                     LOG_ACTION
                     fprintf(stdlog, "Association shutdown completed for socket %d, assoc %u\n",
                             registrar->ASAPSocket,
                             (unsigned int)notification->sn_assoc_change.sac_assoc_id);

                     LOG_END
                     registrarRemovePoolElementsOfConnection(registrar, fd,
                                                             notification->sn_assoc_change.sac_assoc_id);
                  }
                break;
               case SCTP_SHUTDOWN_EVENT:
                  LOG_ACTION
                  fprintf(stdlog, "Shutdown event for socket %d, assoc %u\n",
                          registrar->ASAPSocket,
                          (unsigned int)notification->sn_shutdown_event.sse_assoc_id);

                  LOG_END
                  registrarRemovePoolElementsOfConnection(registrar, fd,
                                                          notification->sn_shutdown_event.sse_assoc_id);
                break;
            }
         }
      }
      else {
         LOG_WARNING
         logerror("Unable to read from registrar socket");
         LOG_END
      }
   }
   else {
      CHECK(false);
   }
}


#ifdef ENABLE_CSP
/* ###### Get CSP report ################################################# */
static size_t registrarGetReportFunction(
                 void*                         userData,
                 unsigned long long*           identifier,
                 struct ComponentAssociation** caeArray,
                 char*                         statusText,
                 char*                         componentLocation,
                 double*                       workload)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct Registrar*              registrar = (struct Registrar*)userData;
   size_t                         caeArraySize;
   size_t                         peers;
   size_t                         pools;
   size_t                         poolElements;

   LOG_VERBOSE2
   fputs("Sending a Component Status Protocol report...\n", stdlog);
   LOG_END
   peers        = ST_CLASS(peerListManagementGetPeers)(&registrar->Peers);
   pools        = ST_CLASS(poolHandlespaceManagementGetPools)(&registrar->Handlespace);
   poolElements = ST_CLASS(poolHandlespaceManagementGetPoolElements)(&registrar->Handlespace);
   snprintf(statusText, CSPR_STATUS_SIZE,
            "%u PEs in %u Pool%s, %u Peer%s",
            (unsigned int)poolElements,
            (unsigned int)pools, (pools == 1) ? "" : "s",
            (unsigned int)peers, (peers == 1) ? "" : "s");
   getComponentLocation(componentLocation, registrar->ASAPSocket, 0);

   *workload    = -1.0;
   *caeArray    = createComponentAssociationArray(peers);
   caeArraySize = 0;
   if(*caeArray) {
      peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers.List);
      while(peerListNode != NULL) {
         if( (peerListNode->Identifier != 0) &&
             (!(peerListNode->Flags & PLNF_MULTICAST)) ) {
         }

         (*caeArray)[caeArraySize].ReceiverID = CID_COMPOUND(CID_GROUP_REGISTRAR, peerListNode->Identifier);
         (*caeArray)[caeArraySize].Duration   = ~0;
         (*caeArray)[caeArraySize].Flags      = 0;
         (*caeArray)[caeArraySize].ProtocolID = IPPROTO_SCTP;
         (*caeArray)[caeArraySize].PPID       = PPID_ENRP;
         caeArraySize++;

         peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers.List,
                           peerListNode);
      }
   }
   return(caeArraySize);
}
#endif


/* ###### Add static peer ################################################ */
unsigned int registrarAddStaticPeer(
                struct Registrar*                  registrar,
                const RegistrarIdentifierType            identifier,
                const struct TransportAddressBlock* transportAddressBlock)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   int                            result;

   result = ST_CLASS(peerListManagementRegisterPeerListNode)(
               &registrar->Peers,
               identifier,
               PLNF_STATIC,
               transportAddressBlock,
               getMicroTime(),
               &peerListNode);

   return(result);
}


/* ###### Add peer ####################################################### */
static void addPeer(struct Registrar* registrar, char* arg)
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
      idx = strindex(address, ',');
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
   if(registrarAddStaticPeer(registrar, 0, transportAddressBlock) != RSPERR_OKAY) {
      fputs("ERROR: Unable to add static peer ", stderr);
      transportAddressBlockPrint(transportAddressBlock, stderr);
      fputs("\n", stderr);
      exit(1);
   }
}


/* ###### Create and bind SCTP and UDP sockets ########################### */
static void getSocketPair(const char*                   sctpAddressParameter,
                          struct TransportAddressBlock* sctpTransportAddress,
                          int*                          sctpSocket,
                          const char*                   udpGroupAddressParameter,
                          struct TransportAddressBlock* udpGroupTransportAddress,
                          int*                          udpSocket)
{
   union sockaddr_union        sctpAddressArray[MAX_NS_TRANSPORTADDRESSES];
   union sockaddr_union        groupAddress;
   union sockaddr_union        udpAddress;
   uint16_t                    localPort;
   size_t                      i;
   int                         family;
   const char*                 address;
   char*                       idx;
   struct sctp_event_subscribe sctpEvents;
   size_t                      sctpAddresses;
   int                         autoCloseTimeout;
   uint16_t                    configuredPort;
   size_t                      trials;

   sctpAddresses = 0;
   family        = checkIPv6() ? AF_INET6 : AF_INET;

   if(string2address(udpGroupAddressParameter, &groupAddress) == false) {
      fprintf(stderr, "ERROR: Bad multicast group address <%s>\n", udpGroupAddressParameter);
      exit(1);
   }

   memset(&udpAddress, 0, sizeof(udpAddress));
   udpAddress.sa.sa_family = groupAddress.sa.sa_family;

   if(!(strncmp(sctpAddressParameter, "auto", 4))) {
      string2address((family == AF_INET6) ? "[::]" : "0.0.0.0", &sctpAddressArray[0]);
      sctpAddresses = 1;
   }
   else {
      address = sctpAddressParameter;
      while(sctpAddresses < MAX_NS_TRANSPORTADDRESSES) {
         idx = strindex((char*)address, ',');
         if(idx) {
            *idx = 0x00;
         }
         if(!string2address(address, &sctpAddressArray[sctpAddresses])) {
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


   configuredPort = getPort((struct sockaddr*)&sctpAddressArray[0]);
   *sctpSocket = -1;
   *udpSocket  = -1;

   trials = 0;
   while(trials++ < 1024) {
      if(configuredPort == 0) {
         localPort = 10000 + (random16() % 50000);
         setPort((struct sockaddr*)&udpAddress, localPort);
         for(i = 0;i < sctpAddresses;i++) {
            setPort((struct sockaddr*)&sctpAddressArray[i], localPort);
         }
      }
      *sctpSocket = ext_socket(family, SOCK_SEQPACKET, IPPROTO_SCTP);
      if(*sctpSocket < 0) {
         continue;
      }
      if(bindplus(*sctpSocket,
                  (union sockaddr_union*)&sctpAddressArray,
                  sctpAddresses) == false) {
         ext_close(*sctpSocket);
         *sctpSocket = -1;
         continue;
      }

      *udpSocket = ext_socket(udpAddress.sa.sa_family, SOCK_DGRAM, IPPROTO_UDP);
      if(*udpSocket < 0) {
         puts("ERROR: Unable to create UDP socket!");
         exit(1);
      }
      setReusable(*udpSocket, 1);
      if(bindplus(*udpSocket,
                  (union sockaddr_union*)&udpAddress,
                  1) == true) {
         break;
      }

      ext_close(*sctpSocket);
      *sctpSocket = -1;
      ext_close(*udpSocket);
      *udpSocket = -1;
   }

   if((*sctpSocket < 0) || (*udpSocket < 0)) {
      perror("Unable to find suitable SCTP/UDP socket pair");
      exit(1);
   }


   if(ext_listen(*sctpSocket, 10) < 0) {
      perror("listen() call failed");
      exit(1);
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
   if(ext_setsockopt(*sctpSocket, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
      perror("setsockopt() for SCTP_EVENTS failed");
      exit(1);
   }
   autoCloseTimeout = 5 + (2 * (NAMESERVER_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL / 1000000));
   if(ext_setsockopt(*sctpSocket, IPPROTO_SCTP, SCTP_AUTOCLOSE, &autoCloseTimeout, sizeof(autoCloseTimeout)) < 0) {
      perror("setsockopt() for SCTP_AUTOCLOSE failed");
      exit(1);
   }

   transportAddressBlockNew(sctpTransportAddress,
                            IPPROTO_SCTP,
                            getPort((struct sockaddr*)&sctpAddressArray[0]),
                            0,
                            (union sockaddr_union*)&sctpAddressArray,
                            sctpAddresses);
   transportAddressBlockNew(udpGroupTransportAddress,
                            IPPROTO_UDP,
                            getPort((struct sockaddr*)&groupAddress),
                            0,
                            (union sockaddr_union*)&groupAddress,
                            1);
}



/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct Registrar*             registrar;
   uint32_t                      serverID                      = 0;

   int                           asapSocket                    = -1;
   char                          asapAddressBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   const char*                   asapAddressParameter          = "auto";
   struct TransportAddressBlock* asapAddress                   = (struct TransportAddressBlock*)&asapAddressBuffer;

   bool                          asapSendAnnounces             = false;
   int                           asapAnnounceSocket            = -1;
   char                          asapAnnounceAddressBuffer[transportAddressBlockGetSize(1)];
   const char*                   asapAnnounceAddressParameter  = "auto";
   struct TransportAddressBlock* asapAnnounceAddress           = (struct TransportAddressBlock*)&asapAnnounceAddressBuffer;

   int                           enrpUnicastSocket             = -1;
   const char*                   enrpUnicastAddressParameter   = "auto";
   char                          enrpUnicastAddressBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* enrpUnicastAddress            = (struct TransportAddressBlock*)&enrpUnicastAddressBuffer;

   bool                          enrpUseMulticast              = false;
   bool                          enrpAnnounceViaMulticast      = false;
   int                           enrpMulticastOutputSocket     = -1;
   int                           enrpMulticastInputSocket      = -1;
   const char*                   enrpMulticastAddressParameter = "auto";
   char                          enrpMulticastAddressBuffer[transportAddressBlockGetSize(1)];
   struct TransportAddressBlock* enrpMulticastAddress          = (struct TransportAddressBlock*)&enrpMulticastAddressBuffer;

#ifdef ENABLE_CSP
   union sockaddr_union          cspReportAddress;
   unsigned long long            cspReportInterval = 0;
#endif

   unsigned long long            pollTimeStamp;
   struct pollfd                 ufds[FD_SETSIZE];
   unsigned int                  nfds;
   int                           timeout;
   int                           result;
   int                           i;


   /* ====== Get arguments =============================================== */
#ifdef ENABLE_CSP
   string2address("127.0.0.1:2960", &cspReportAddress);
#endif
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-tcp=",5))) {
         fputs("ERROR: TCP mapping is not supported -> Use SCTP!", stderr);
         exit(1);
      }
      else if(!(strncmp(argv[i], "-identifier=", 12))) {
         serverID = atol((char*)&argv[i][12]);
      }
      else if(!(strncmp(argv[i], "-peer=",6))) {
         /* to be handled later */
      }
      else if(!(strncmp(argv[i], "-maxbadpereports=",17))) {
         /* to be handled later */
      }
      else if(!(strncmp(argv[i], "-asap=",6))) {
         asapAddressParameter = (const char*)&argv[i][6];
      }
      else if(!(strncmp(argv[i], "-asapannounce=", 14))) {
         asapAnnounceAddressParameter = (const char*)&argv[i][14];
         asapSendAnnounces = true;
      }
      else if(!(strncmp(argv[i], "-enrp=",6))) {
         enrpUnicastAddressParameter = (const char*)&argv[i][6];
      }
      else if(!(strncmp(argv[i], "-enrpmulticast=", 15))) {
         enrpMulticastAddressParameter = (const char*)&argv[i][15];
         enrpAnnounceViaMulticast = true;
      }
      else if(!(strcmp(argv[i], "-multicast=on"))) {
         enrpUseMulticast = true;
      }
      else if(!(strcmp(argv[i], "-multicast=off"))) {
         enrpUseMulticast = false;
      }
      else if(!(strncmp(argv[i], "-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
#ifdef ENABLE_CSP
      else if(!(strncmp(argv[i], "-cspinterval=", 13))) {
         cspReportInterval = atol((char*)&argv[i][13]);
      }
      else if(!(strncmp(argv[i], "-cspserver=", 11))) {
         if(!string2address((char*)&argv[i][11], &cspReportAddress)) {
            fprintf(stderr,
                    "ERROR: Bad CSP report address %s! Use format <address:port>.\n",
                    (char*)&argv[i][11]);
            exit(1);
         }
         if(cspReportInterval <= 0) {
            cspReportInterval = 250000;
         }
      }
#else
      else if((!(strncmp(argv[i], "-cspinterval=", 13))) ||
              (!(strncmp(argv[i], "-cspserver=", 11)))) {
         fprintf(stderr, "ERROR: CSP support not compiled in! Ignoring argument %s\n", argv[i]);
         exit(1);
      }
#endif
      else {
         printf("Usage: %s {-asap=auto|address:port{,address}...} {[-asapannounce=address:port}]} {-enrp=auto|address:port{,address}...} {[-enrpmulticast=address:port}]} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} "
#ifdef ENABLE_CSP
          "{-cspserver=address} {-cspinterval=microseconds}"
#endif
          "{-identifier=registrar identifier}\n",argv[0]);
         exit(1);
      }
   }
   beginLogging();


   if(!strcmp(asapAnnounceAddressParameter, "auto")) {
      asapAnnounceAddressParameter = "239.0.0.1:3863";
   }
   if(!strcmp(enrpMulticastAddressParameter, "auto")) {
      enrpMulticastAddressParameter = "239.0.0.1:3864";
   }
   getSocketPair(asapAddressParameter, asapAddress, &asapSocket,
                 asapAnnounceAddressParameter, asapAnnounceAddress, &asapAnnounceSocket);
   getSocketPair(enrpUnicastAddressParameter, enrpUnicastAddress, &enrpUnicastSocket,
                 enrpMulticastAddressParameter, enrpMulticastAddress, &enrpMulticastOutputSocket);
   enrpMulticastInputSocket = ext_socket(enrpMulticastAddress->AddressArray[0].sa.sa_family,
                                         SOCK_DGRAM, IPPROTO_UDP);
   if(enrpMulticastInputSocket < 0) {
      fputs("ERROR: Unable to create input UDP socket for ENRP!\n", stderr);
      exit(1);
   }
   setReusable(enrpMulticastInputSocket, 1);
   if(bindplus(enrpMulticastInputSocket,
               (union sockaddr_union*)&enrpMulticastAddress->AddressArray[0],
               1) == false) {
      fputs("ERROR: Unable to bind input UDP socket for ENRP to address ", stderr);
      fputaddress(&enrpMulticastAddress->AddressArray[0].sa, true, stderr);
      fputs("\n", stderr);
      exit(1);
   }


   /* ====== Initialize Registrar ======================================= */
   registrar = registrarNew(serverID,
                            asapSocket,
                            asapAnnounceSocket,
                            enrpUnicastSocket,
                            enrpMulticastInputSocket,
                            enrpMulticastOutputSocket,
                            asapSendAnnounces, (const union sockaddr_union*)&asapAnnounceAddress->AddressArray[0],
                            enrpUseMulticast, enrpAnnounceViaMulticast, (const union sockaddr_union*)&enrpMulticastAddress->AddressArray[0]
#ifdef ENABLE_CSP
                            , cspReportInterval, &cspReportAddress
#endif
                            );
   if(registrar == NULL) {
      fprintf(stderr, "ERROR: Unable to initialize Registrar object!\n");
      exit(1);
   }
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-peer=",6))) {
         addPeer(registrar, (char*)&argv[i][6]);
      }
      else if(!(strncmp(argv[i], "-maxbadpereports=",17))) {
         registrar->MaxBadPEReports = atol((char*)&argv[i][17]);
         if(registrar->MaxBadPEReports < 1) {
            registrar->MaxBadPEReports = 1;
         }
      }
   }
#ifndef FAST_BREAK
   installBreakDetector();
#endif

   /* ====== Print information =========================================== */
   puts("The rsplib Registrar - Version 1.00");
   puts("===================================\n");
   printf("Server ID:              $%08x\n", registrar->ServerID);
   printf("ASAP Address:           ");
   transportAddressBlockPrint(asapAddress, stdout);
   puts("");
   printf("ENRP Address:           ");
   transportAddressBlockPrint(enrpUnicastAddress, stdout);
   puts("");
   printf("ASAP Announce Address:  ");
   if(asapSendAnnounces) {
      transportAddressBlockPrint(asapAnnounceAddress, stdout);
      puts("");
   }
   else {
      puts("(none)");
   }
   printf("ENRP Multicast Address: ");
   if(enrpAnnounceViaMulticast) {
      transportAddressBlockPrint(enrpMulticastAddress, stdout);
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
#ifdef ENABLE_CSP
   if(cspReportInterval > 0) {
      printf("CSP Report Address:     ");
      fputaddress((struct sockaddr*)&cspReportAddress, true, stdout);
      puts("");
      printf("CSP Report Interval:    %lldus\n", cspReportInterval);
   }
#endif

   puts("\nASAP Parameters:");
   printf("   Max Bad PE Reports:                          %u\n",     (unsigned int)registrar->MaxBadPEReports);
   printf("   Server Announce Cycle:                       %lldus\n", registrar->ServerAnnounceCycle);
   printf("   Endpoint Monitoring SCTP Heartbeat Interval: %lldus\n", registrar->EndpointMonitoringHeartbeatInterval);
   printf("   Endpoint Keep Alive Transmission Interval:   %lldus\n", registrar->EndpointKeepAliveTransmissionInterval);
   printf("   Endpoint Keep Alive Timeout Interval:        %lldus\n", registrar->EndpointKeepAliveTimeoutInterval);
   puts("ENRP Parameters:");
   printf("   Peer Heartbeat Cylce:                        %lldus\n", registrar->PeerHeartbeatCycle);
   printf("   Peer Max Time Last Heard:                    %lldus\n", registrar->PeerMaxTimeLastHeard);
   printf("   Peer Max Time No Response:                   %lldus\n", registrar->PeerMaxTimeNoResponse);
   printf("   Mentor Hunt Timeout:                         %lldus\n", registrar->MentorHuntTimeout);
   printf("   Takeover Expiry Interval:                    %lldus\n", registrar->TakeoverExpiryInterval);
   puts("");


   /* ====== We are ready! =============================================== */
   LOG_NOTE
   fputs("Registrar started. Going into initialization phase...\n", stdlog);
   LOG_END


   /* ====== Main loop =================================================== */
   while(!breakDetected()) {
      dispatcherGetPollParameters(&registrar->StateMachine,
                                  (struct pollfd*)&ufds, &nfds, &timeout,
                                  &pollTimeStamp);
      if((timeout < 0) || (timeout > 250)) {
         timeout = 500;
      }
      result = ext_poll((struct pollfd*)&ufds, nfds, timeout);
      if(result < 0) {
         if(errno != EINTR) {
            perror("poll() failed");
         }
         break;
      }
      dispatcherHandlePollResult(&registrar->StateMachine, result,
                                 (struct pollfd*)&ufds, nfds, timeout,
                                 pollTimeStamp);
   }


   /* ====== Clean up ==================================================== */
   registrarDelete(registrar);
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
