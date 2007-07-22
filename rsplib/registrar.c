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
#include "takeoverprocess.h"
#include "messagebuffer.h"
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


#define MIN_ENDPOINT_ADDRESS_SCOPE                                          6
#define RSERPOOL_MESSAGE_BUFFER_SIZE                                    65536
#define REGISTRAR_DEFAULT_DISTANCE_STEP                                    50
#define REGISTRAR_DEFAULT_MAX_BAD_PE_REPORTS                                3
#define REGISTRAR_DEFAULT_SERVER_ANNOUNCE_CYCLE                       1000000
#define REGISTRAR_DEFAULT_ENDPOINT_MONITORING_HEARTBEAT_INTERVAL      1000000
#define REGISTRAR_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL   5000000
#define REGISTRAR_DEFAULT_ENDPOINT_KEEP_ALIVE_TIMEOUT_INTERVAL        5000000
#define REGISTRAR_DEFAULT_MAX_ELEMENTS_PER_HANDLE_TABLE_REQUEST           128
#define REGISTRAR_DEFAULT_MAX_INCREMENT                                     0
#define REGISTRAR_DEFAULT_MAX_HANDLE_RESOLUTION_ITEMS                       3
#define REGISTRAR_DEFAULT_PEER_HEARTBEAT_CYCLE                        2444444
#define REGISTRAR_DEFAULT_PEER_MAX_TIME_LAST_HEARD                    5000000
#define REGISTRAR_DEFAULT_PEER_MAX_TIME_NO_RESPONSE                   5000000
#define REGISTRAR_DEFAULT_MENTOR_DISCOVERY_TIMEOUT                         5000000
#define REGISTRAR_DEFAULT_TAKEOVER_EXPIRY_INTERVAL                    5000000
#define REGISTRAR_DEFAULT_AUTOCLOSE_TIMEOUT                         300000000

unsigned long long randomizeCycle(const unsigned long long interval)
{
   const double originalInterval = (double)interval;
   const double variation    = 0.250 * originalInterval;
   const double nextInterval = originalInterval - (variation / 2.0) +
                                  variation * ((double)rand() / (double)RAND_MAX);
   return((unsigned long long)nextInterval);
}



struct Registrar
{
   RegistrarIdentifierType                    ServerID;

   struct Dispatcher                          StateMachine;
   struct ST_CLASS(PoolHandlespaceManagement) Handlespace;
   struct ST_CLASS(PeerListManagement)        Peers;
   struct Timer                               HandlespaceActionTimer;
   struct Timer                               PeerActionTimer;

   struct MessageBuffer*                      UDPMessageBuffer;

   int                                        ASAPAnnounceSocket;
   int                                        ASAPAnnounceSocketFamily;
   struct FDCallback                          ASAPSocketFDCallback;
   union sockaddr_union                       ASAPAnnounceAddress;
   bool                                       ASAPSendAnnounces;
   int                                        ASAPSocket;
   struct MessageBuffer*                      ASAPMessageBuffer;
   struct Timer                               ASAPAnnounceTimer;

   int                                        ENRPMulticastOutputSocket;
   int                                        ENRPMulticastOutputSocketFamily;
   int                                        ENRPMulticastInputSocket;
   struct FDCallback                          ENRPMulticastInputSocketFDCallback;
   union sockaddr_union                       ENRPMulticastAddress;
   int                                        ENRPUnicastSocket;
   struct FDCallback                          ENRPUnicastSocketFDCallback;
   struct MessageBuffer*                      ENRPUnicastMessageBuffer;
   bool                                       ENRPAnnounceViaMulticast;
   struct Timer                               ENRPAnnounceTimer;

   bool                                       InStartupPhase;
   RegistrarIdentifierType                    MentorServerID;

   size_t                                     DistanceStep;
   size_t                                     MaxBadPEReports;
   unsigned long long                         AutoCloseTimeout;
   unsigned long long                         ServerAnnounceCycle;
   unsigned long long                         EndpointMonitoringHeartbeatInterval;
   unsigned long long                         EndpointKeepAliveTransmissionInterval;
   unsigned long long                         EndpointKeepAliveTimeoutInterval;
   size_t                                     MaxElementsPerHTRequest;
   size_t                                     MaxIncrement;
   size_t                                     MaxHandleResolutionItems;
   unsigned long long                         PeerHeartbeatCycle;
   unsigned long long                         PeerMaxTimeLastHeard;
   unsigned long long                         PeerMaxTimeNoResponse;
   unsigned long long                         MentorDiscoveryTimeout;
   unsigned long long                         TakeoverExpiryInterval;

   FILE*                                      StatsFile;
   struct Timer                               StatsTimer;
   unsigned long long                         StatsLine;
   unsigned long long                         StatsStartTime;
   int                                        StatsInterval;
   unsigned long long                         RegistrationCount;
   unsigned long long                         ReregistrationCount;
   unsigned long long                         DeregistrationCount;
   unsigned long long                         HandleResolutionCount;
   unsigned long long                         FailureReportCount;
   unsigned long long                         SynchronizationCount;
   unsigned long long                         UpdateCount;

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
                 uint64_t*                     identifier,
                 struct ComponentAssociation** caeArray,
                 char*                         statusText,
                 char*                         componentLocation,
                 double*                       workload);
#endif


struct Registrar* registrarNew(const RegistrarIdentifierType  serverID,
                               int                            asapUnicastSocket,
                               int                            asapAnnounceSocket,
                               int                            enrpUnicastSocket,
                               int                            enrpMulticastOutputSocket,
                               int                            enrpMulticastInputSocket,
                               const bool                     asapSendAnnounces,
                               const union sockaddr_union*    asapAnnounceAddress,
                               const bool                     enrpAnnounceViaMulticast,
                               const union sockaddr_union*    enrpMulticastAddress,
                               FILE*                          statsFile,
                               unsigned int                   statsInterval
#ifdef ENABLE_CSP
                               , const unsigned int           cspReportInterval,
                               const union sockaddr_union*    cspReportAddress
#endif
                               );
void registrarDelete(struct Registrar* registrar);

static void sendENRPHandleUpdate(struct Registrar*                registrar,
                             struct ST_CLASS(PoolElementNode)* poolElementNode,
                             const uint16_t                    action);
static void sendENRPInitTakeover(struct Registrar*       registrar,
                             const RegistrarIdentifierType targetID);
static void sendENRPInitTakeoverAck(struct Registrar*       registrar,
                                const int                sd,
                                const sctp_assoc_t       assocID,
                                const RegistrarIdentifierType receiverID,
                                const RegistrarIdentifierType targetID);
static void statisticsCallback(struct Dispatcher* dispatcher,
                               struct Timer*      timer,
                               void*              userData);
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
                               int                            asapUnicastSocket,
                               int                            asapAnnounceSocket,
                               int                            enrpUnicastSocket,
                               int                            enrpMulticastOutputSocket,
                               int                            enrpMulticastInputSocket,
                               const bool                     asapSendAnnounces,
                               const union sockaddr_union*    asapAnnounceAddress,
                               const bool                     enrpAnnounceViaMulticast,
                               const union sockaddr_union*    enrpMulticastAddress,
                               FILE*                          statsFile,
                               unsigned int                   statsInterval
#ifdef ENABLE_CSP
                               , const unsigned int           cspReportInterval,
                               const union sockaddr_union*    cspReportAddress
#endif
                               )
{
   struct Registrar*       registrar;
   int                     autoCloseTimeout;
#ifdef SCTP_DELAYED_ACK_TIME
   struct sctp_assoc_value sctpAssocValue;
#endif

   registrar = (struct Registrar*)malloc(sizeof(struct Registrar));
   if(registrar != NULL) {
      registrar->UDPMessageBuffer = messageBufferNew(RSERPOOL_MESSAGE_BUFFER_SIZE, false);
      if(registrar->UDPMessageBuffer == NULL) {
         free(registrar);
         return(NULL);
      }
      registrar->ASAPMessageBuffer = messageBufferNew(RSERPOOL_MESSAGE_BUFFER_SIZE, true);
      if(registrar->ASAPMessageBuffer == NULL) {
         messageBufferDelete(registrar->UDPMessageBuffer);
         free(registrar);
         return(NULL);
      }
      registrar->ENRPUnicastMessageBuffer = messageBufferNew(RSERPOOL_MESSAGE_BUFFER_SIZE, true);
      if(registrar->ENRPUnicastMessageBuffer == NULL) {
         messageBufferDelete(registrar->ASAPMessageBuffer);
         messageBufferDelete(registrar->UDPMessageBuffer);
         free(registrar);
         return(NULL);
      }

      registrar->ServerID = serverID;
      if(registrar->ServerID == 0) {
         registrar->ServerID = random32();
      }

      dispatcherNew(&registrar->StateMachine, NULL, NULL, NULL);
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

      registrar->InStartupPhase            = true;
      registrar->MentorServerID            = 0;

      registrar->ASAPSocket                = asapUnicastSocket;
      registrar->ASAPAnnounceSocket        = asapAnnounceSocket;
      registrar->ASAPSendAnnounces         = asapSendAnnounces;

      registrar->ENRPUnicastSocket         = enrpUnicastSocket;
      registrar->ENRPMulticastInputSocket  = enrpMulticastOutputSocket;
      registrar->ENRPMulticastOutputSocket = enrpMulticastInputSocket;
      registrar->ENRPAnnounceViaMulticast  = enrpAnnounceViaMulticast;


      registrar->DistanceStep                          = REGISTRAR_DEFAULT_DISTANCE_STEP;
      registrar->MaxBadPEReports                       = REGISTRAR_DEFAULT_MAX_BAD_PE_REPORTS;
      registrar->ServerAnnounceCycle                   = REGISTRAR_DEFAULT_SERVER_ANNOUNCE_CYCLE;
      registrar->EndpointMonitoringHeartbeatInterval   = REGISTRAR_DEFAULT_ENDPOINT_MONITORING_HEARTBEAT_INTERVAL;
      registrar->EndpointKeepAliveTransmissionInterval = REGISTRAR_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL;
      registrar->EndpointKeepAliveTimeoutInterval      = REGISTRAR_DEFAULT_ENDPOINT_KEEP_ALIVE_TIMEOUT_INTERVAL;
      registrar->AutoCloseTimeout                      = REGISTRAR_DEFAULT_AUTOCLOSE_TIMEOUT;
      registrar->MaxIncrement                          = REGISTRAR_DEFAULT_MAX_INCREMENT;
      registrar->MaxHandleResolutionItems              = REGISTRAR_DEFAULT_MAX_HANDLE_RESOLUTION_ITEMS;
      registrar->MaxElementsPerHTRequest               = REGISTRAR_DEFAULT_MAX_ELEMENTS_PER_HANDLE_TABLE_REQUEST;
      registrar->PeerMaxTimeLastHeard                  = REGISTRAR_DEFAULT_PEER_MAX_TIME_LAST_HEARD;
      registrar->PeerMaxTimeNoResponse                 = REGISTRAR_DEFAULT_PEER_MAX_TIME_NO_RESPONSE;
      registrar->PeerHeartbeatCycle                    = REGISTRAR_DEFAULT_PEER_HEARTBEAT_CYCLE;
      registrar->MentorDiscoveryTimeout                = REGISTRAR_DEFAULT_MENTOR_DISCOVERY_TIMEOUT;
      registrar->TakeoverExpiryInterval                = REGISTRAR_DEFAULT_TAKEOVER_EXPIRY_INTERVAL;

      registrar->StatsFile             = statsFile;
      registrar->StatsInterval         = statsInterval;
      registrar->StatsStartTime        = 0;
      registrar->StatsLine             = 0;
      registrar->RegistrationCount     = 0;
      registrar->ReregistrationCount   = 0;
      registrar->DeregistrationCount   = 0;
      registrar->HandleResolutionCount = 0;
      registrar->FailureReportCount    = 0;
      registrar->SynchronizationCount  = 0;
      registrar->UpdateCount           = 0;

      autoCloseTimeout = (registrar->AutoCloseTimeout / 1000000);
      if(ext_setsockopt(registrar->ASAPSocket, IPPROTO_SCTP, SCTP_AUTOCLOSE, &autoCloseTimeout, sizeof(autoCloseTimeout)) < 0) {
         LOG_ERROR
         logerror("setsockopt() for SCTP_AUTOCLOSE failed");
         LOG_END
      }
      if(ext_setsockopt(registrar->ENRPUnicastSocket, IPPROTO_SCTP, SCTP_AUTOCLOSE, &autoCloseTimeout, sizeof(autoCloseTimeout)) < 0) {
         LOG_ERROR
         logerror("setsockopt() for SCTP_AUTOCLOSE failed");
         LOG_END
      }
#ifdef SCTP_DELAYED_ACK_TIME
      /* ====== Tune SACK handling ======================================= */
      /* Without this tuning, the PE would wait 200ms to acknowledge a
         Endpoint Unreachable. The usually immediately following
         Handle Resolution transmission would also be delayed ... */
      sctpAssocValue.assoc_id    = 0;
      sctpAssocValue.assoc_value = 1;
      if(ext_setsockopt(registrar->ASAPSocket,
                        IPPROTO_SCTP, SCTP_DELAYED_ACK_TIME,
                        &sctpAssocValue, sizeof(sctpAssocValue)) < 0) {
         LOG_WARNING
         logerror("Unable to set SCTP_DELAYED_ACK_TIME");
         LOG_END
      }
#else
#warning SCTP_DELAYED_ACK_TIME is not defined - Unable to tune SACK handling!
#endif

      memcpy(&registrar->ASAPAnnounceAddress, asapAnnounceAddress, sizeof(registrar->ASAPAnnounceAddress));
      if(registrar->ASAPAnnounceSocket >= 0) {
         setNonBlocking(registrar->ASAPAnnounceSocket);
      }
      registrar->ASAPAnnounceSocketFamily = getFamily(&registrar->ASAPAnnounceAddress.sa);

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
      registrar->ENRPMulticastOutputSocketFamily = getFamily(&registrar->ENRPMulticastAddress.sa);

      timerStart(&registrar->ENRPAnnounceTimer, getMicroTime() + registrar->MentorDiscoveryTimeout);
      if(registrar->ENRPAnnounceViaMulticast) {
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

      if(registrar->StatsFile) {
         timerNew(&registrar->StatsTimer,
                  &registrar->StateMachine,
                  statisticsCallback,
                  (void*)registrar);
         timerStart(&registrar->StatsTimer, 0);
      }
   }

   return(registrar);
}


/* ###### Destructor ###################################################### */
void registrarDelete(struct Registrar* registrar)
{
   if(registrar) {
      if(registrar->StatsFile) {
         timerDelete(&registrar->StatsTimer);
      }
      fdCallbackDelete(&registrar->ENRPUnicastSocketFDCallback);
      fdCallbackDelete(&registrar->ASAPSocketFDCallback);
      ST_CLASS(peerListManagementDelete)(&registrar->Peers);
      ST_CLASS(poolHandlespaceManagementDelete)(&registrar->Handlespace);
#ifdef ENABLE_CSP
      if(registrar->CSPReportInterval > 0) {
         cspReporterDelete(&registrar->CSPReporter);
      }
#endif
      timerDelete(&registrar->ASAPAnnounceTimer);
      timerDelete(&registrar->ENRPAnnounceTimer);
      timerDelete(&registrar->HandlespaceActionTimer);
      timerDelete(&registrar->PeerActionTimer);
      if(registrar->ENRPMulticastOutputSocket >= 0) {
         ext_close(registrar->ENRPMulticastOutputSocket);
         registrar->ENRPMulticastOutputSocket = -1;
      }
      if(registrar->ENRPMulticastInputSocket >= 0) {
         fdCallbackDelete(&registrar->ENRPMulticastInputSocketFDCallback);
         ext_close(registrar->ENRPMulticastInputSocket);
         registrar->ENRPMulticastInputSocket = -1;
      }
      if(registrar->ENRPUnicastSocket >= 0) {
         ext_close(registrar->ENRPUnicastSocket);
         registrar->ENRPUnicastSocket = -1;
      }
      if(registrar->ASAPAnnounceSocket >= 0) {
         ext_close(registrar->ASAPAnnounceSocket);
         registrar->ASAPAnnounceSocket = -1;
      }
      dispatcherDelete(&registrar->StateMachine);
      messageBufferDelete(registrar->ENRPUnicastMessageBuffer);
      registrar->ENRPUnicastMessageBuffer = NULL;
      messageBufferDelete(registrar->ASAPMessageBuffer);
      registrar->ASAPMessageBuffer = NULL;
      messageBufferDelete(registrar->UDPMessageBuffer);
      registrar->UDPMessageBuffer = NULL;
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


#ifdef LINUX
/* ###### Get system uptime (in microseconds) ############################ */
static unsigned long long getUptime()
{
   double             dblUptime;
   unsigned long long uptime = 0;

   FILE* fh = fopen("/proc/uptime", "r");
   if(fh != NULL) {
      if(fscanf(fh, "%lf", &dblUptime) == 1) {
         uptime = (unsigned long long)floor(dblUptime * 1000000.0);
      }
      fclose(fh);
   }
   return(uptime);
}


/* ###### Get process times (in microseconds) ############################ */
static bool getProcessTimes(unsigned long long* startupTime,
                            unsigned long long* userTime,
                            unsigned long long* systemTime)
{
   char          statusFile[256];
   int           pid;
   char          tcomm[128];
   char          state[16];
   int           ppid;
   int           pgid;
   int           sid;
   int           tty_nr;
   int           tty_pgrp;
   unsigned long flags;
   unsigned long min_flt;
   unsigned long cmin_flt;
   unsigned long maj_flt;
   unsigned long cmaj_flt;
   long          priority;
   long          nice;
   int           num_threads;
   unsigned long long start_time;

   unsigned long utime;
   unsigned long stime;
   long          cutime;
   long          cstime;
   bool          success = false;

   snprintf((char*)&statusFile, sizeof(statusFile), "/proc/%u/stat", getpid());

   FILE* fh = fopen(statusFile, "r");
   if(fh != NULL) {
      int result = fscanf(fh, "%d %127s %1s   %d %d %d %d %d   %lu %lu %lu %lu %lu   %lu %lu %ld %ld   %ld %ld %d 0 %llu",
                          &pid, (char*)&tcomm, (char*)&state,
                          &ppid, &pgid, &sid, &tty_nr, &tty_pgrp,
                          &flags, &min_flt, &cmin_flt, &maj_flt, &cmaj_flt,
                          &utime, &stime, &cutime, &cstime,
                          &priority, &nice, &num_threads, &start_time);

      if(result == 21) {
         *startupTime = start_time * 10000ULL;
         *userTime    = utime * 10000ULL;
         *systemTime  = stime * 10000ULL;
         success = true;
      }
      fclose(fh);
   }
   return(success);
}
#endif


/* ###### Statistics dump callback ########################################## */
static void statisticsCallback(struct Dispatcher* dispatcher,
                               struct Timer*      timer,
                               void*              userData)
{
   struct Registrar*        registrar   = (struct Registrar*)userData;
   const unsigned long long now         = getMicroTime();
   unsigned long long       runtime     = 0;
   unsigned long long       startupTime = 0;
   unsigned long long       userTime    = 0;
   unsigned long long       systemTime  = 0;
   unsigned long long       uptime;

   if(registrar->StatsLine == 0) {
      fputs("AbsTime RelTime Runtime UserTime SystemTime Registrations Reregistrations Deregistrations HandleResolutions FailureReports Synchronizations Updates\n",
            registrar->StatsFile);
      registrar->StatsLine      = 1;
      registrar->StatsStartTime = now;
   }

#ifdef LINUX
   if( (getProcessTimes(&startupTime, &userTime, &systemTime)) &&
       ( (uptime = getUptime()) > 0 ) ) {

      runtime = uptime - startupTime;
   }
   else {
      LOG_WARNING
      fputs("Unable to obtain process performance data!\n", stdlog);
      LOG_END
   }
#endif

   fprintf(registrar->StatsFile,
           "%06llu %1.6f %1.6f   %1.6f %1.6f %1.6f   %llu %llu %llu %llu %llu %llu %llu\n",
           registrar->StatsLine++,
           now / 1000000.0,
           (now - registrar->StatsStartTime) / 1000000.0,

           runtime / 1000000.0,
           userTime / 1000000.0,
           systemTime / 1000000.0,

           registrar->RegistrationCount,
           registrar->ReregistrationCount,
           registrar->DeregistrationCount,
           registrar->HandleResolutionCount,
           registrar->FailureReportCount,
           registrar->SynchronizationCount,
           registrar->UpdateCount);
   fflush(registrar->StatsFile);

   timerStart(timer, getMicroTime() + (1000ULL * registrar->StatsInterval));
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
         if(sendMulticastOverAllInterfaces(
               registrar->ASAPAnnounceSocket,
               registrar->ASAPAnnounceSocketFamily,
               message->Buffer,
               messageLength,
               0,
               (struct sockaddr*)&registrar->ASAPAnnounceAddress,
               getSocklen((struct sockaddr*)&registrar->ASAPAnnounceAddress)) <= 0) {
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
            sendENRPHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
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
static void sendENRPPresence(struct Registrar*             registrar,
                             int                           sd,
                             const sctp_assoc_t            assocID,
                             int                           msgSendFlags,
                             const union sockaddr_union*   destinationAddressList,
                             const size_t                  destinationAddresses,
                             const RegistrarIdentifierType receiverID,
                             const bool                    replyRequired)
{
   struct RSerPoolMessage*       message;
   size_t                        messageLength;
   struct ST_CLASS(PeerListNode) peerListNode;
   char                          localAddressArrayBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
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
                                                         MAX_PE_TRANSPORTADDRESSES,
                                                         true) > 0) {
         ST_CLASS(peerListNodeNew)(&peerListNode,
                                   registrar->ServerID, 0,
                                   localAddressArray);

         LOG_VERBOSE3
         fputs("Sending Presence using peer list entry: \n", stdlog);
         ST_CLASS(peerListNodePrint)(&peerListNode, stdlog, ~0);
         fputs("\n", stdlog);
         LOG_END

         messageLength = rserpoolMessage2Packet(message);
         if(messageLength > 0) {
            if(sd == registrar->ENRPMulticastOutputSocket) {
               if(sendMulticastOverAllInterfaces(
                     registrar->ENRPMulticastOutputSocket,
                     registrar->ENRPMulticastOutputSocketFamily,
                     message->Buffer,
                     messageLength,
                     0,
                     &destinationAddressList->sa,
                     getSocklen(&destinationAddressList->sa)) <= 0) {
                  LOG_WARNING
                  fputs("Sending Presence via multicast failed\n", stdlog);
                  LOG_END
               }
            }
            else {
               if(rserpoolMessageSend(IPPROTO_SCTP,
                                      sd, assocID, msgSendFlags, 0, 0, message) == false) {
                  LOG_WARNING
                  fputs("Sending Presence via unicast failed\n", stdlog);
                  LOG_END
               }
            }
            rserpoolMessageDelete(message);
         }
      }
      else {
         LOG_ERROR
         fputs("Unable to obtain local ASAP socket addresses\n", stdlog);
         LOG_END
      }
   }
}


/* ###### Send ENRP peer presence message to all peers ################### */
static void sendENRPPresenceToAllPeers(struct Registrar* registrar)
{
   struct ST_CLASS(PeerListNode)* peerListNode;

   peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
   while(peerListNode != NULL) {
      /* If MSG_SEND_TO_ALL is available, unicast messages must only be sent
         to static peers not yet connected. That is, their ID is 0. */
#ifdef MSG_SEND_TO_ALL
      if(peerListNode->Identifier == UNDEFINED_REGISTRAR_IDENTIFIER) {
#else
      {
#endif
         LOG_VERBOSE2
         fprintf(stdlog, "Sending Presence to unicast peer $%08x...\n",
                 peerListNode->Identifier);
         LOG_END
         sendENRPPresence(registrar,
                          registrar->ENRPUnicastSocket, 0, 0,
                          peerListNode->AddressBlock->AddressArray,
                          peerListNode->AddressBlock->Addresses,
                          peerListNode->Identifier,
                          false);
      }
      peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                        &registrar->Peers, peerListNode);
   }
#ifdef MSG_SEND_TO_ALL
#warning Using MSG_SEND_TO_ALL!
   sendENRPPresence(registrar,
                    registrar->ENRPUnicastSocket, 0,
                    MSG_SEND_TO_ALL,
                    NULL, 0, 0, false);
#endif
}


/* ###### Send ENRP peer list request message ############################ */
static void sendENRPListRequest(struct Registrar*           registrar,
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
                             sd, assocID, msgSendFlags, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending ListRequest failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Send ENRP peer handle table request message ###################### */
static void sendENRPHandleTableRequest(struct Registrar*           registrar,
                                       int                         sd,
                                       const sctp_assoc_t          assocID,
                                       int                         msgSendFlags,
                                       const union sockaddr_union* destinationAddressList,
                                       const size_t                destinationAddresses,
                                       RegistrarIdentifierType     receiverID,
                                       unsigned int                flags)
{
   struct RSerPoolMessage* message;

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {
      message->Type         = EHT_HANDLE_TABLE_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = assocID;
      message->AddressArray = (union sockaddr_union*)destinationAddressList;
      message->Addresses    = destinationAddresses;
      message->Flags        = flags;
      message->SenderID     = registrar->ServerID;
      message->ReceiverID   = receiverID;
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending HandleTableRequest failed\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
}


/* ###### Initialization is complete ##################################### */
static void beginNormalOperation(struct Registrar* registrar, const bool initializedFromMentor)
{
   LOG_NOTE
   if(initializedFromMentor) {
      fputs("Initialization phase ended after obtaining handlespace from mentor server. The registrar is ready!\n", stdlog);
   }
   else {
      fputs("Initialization phase ended after ENRP mentor discovery timeout. The registrar is ready!\n", stdlog);
   }
   LOG_END

   registrar->InStartupPhase = false;
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
   struct Registrar* registrar = (struct Registrar*)userData;

   /* ====== Leave startup phase ========================================= */
   if(registrar->InStartupPhase) {
      beginNormalOperation(registrar, false);
   }

   /* ====== Send Presence as multicast announce ========================= */
   if(registrar->ENRPAnnounceViaMulticast) {
      if(joinOrLeaveMulticastGroup(registrar->ENRPMulticastInputSocket,
                                   &registrar->ENRPMulticastAddress,
                                   true) == false) {
         LOG_WARNING
         fputs("Unable to join multicast group ", stdlog);
         fputaddress(&registrar->ENRPMulticastAddress.sa, true, stdlog);
         fputs(". Registrar will probably be unable to detect peers!\n", stdlog);
         LOG_END;
      }
      sendENRPPresence(registrar, registrar->ENRPMulticastOutputSocket, 0, 0,
                       (union sockaddr_union*)&registrar->ENRPMulticastAddress, 1,
                       0, false);
   }

   /* ====== Send Presence to peers ====================================== */
   sendENRPPresenceToAllPeers(registrar);

   /* ====== Restart heartbeat cycle timer =============================== */
   unsigned long long n = getMicroTime() + randomizeCycle(registrar->PeerHeartbeatCycle);
   timerStart(timer, n);
}


/* ###### Send Endpoint Keep Alive ####################################### */
static void sendASAPEndpointKeepAlive(struct Registrar*                 registrar,
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
                                      0, 0, 0,
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
                                      0, 0, 0,
                                      message);
      }
      if(result == false) {
         LOG_WARNING
         fprintf(stdlog, "Sending EndpointKeepAlive for pool element $%08x in pool ",
                 poolElementNode->Identifier);
         poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
         fputs(" failed\n", stdlog);
         LOG_END
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

         sendASAPEndpointKeepAlive(registrar, poolElementNode, false);

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
            sendENRPHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
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
   size_t                         outstandingAcks;
   size_t                         i;

   peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromTimerStorage)(
                     &registrar->Peers);
   while((peerListNode != NULL) &&
         (peerListNode->TimerTimeStamp <= getMicroTime())) {
      nextPeerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromTimerStorage)(
                            &registrar->Peers,
                            peerListNode);

      /* ====== Max-time-last-heard timer ================================ */
      if(peerListNode->TimerCode == PLNT_MAX_TIME_LAST_HEARD) {
         LOG_ACTION
         fputs("Peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fprintf(stdlog, " not head since MaxTimeLastHeard=%lluus -> requesting immediate Presence\n",
               registrar->PeerMaxTimeLastHeard);
         LOG_END
         sendENRPPresence(registrar,
                          registrar->ENRPUnicastSocket,
                          0, 0,
                          peerListNode->AddressBlock->AddressArray,
                          peerListNode->AddressBlock->Addresses,
                          peerListNode->Identifier,
                          true);
         ST_CLASS(peerListManagementDeactivateTimer)(
            &registrar->Peers, peerListNode);
         ST_CLASS(peerListManagementActivateTimer)(
            &registrar->Peers, peerListNode, PLNT_MAX_TIME_NO_RESPONSE,
            getMicroTime() + registrar->PeerMaxTimeNoResponse);
      }

      /* ====== Max-time-no-response timer =============================== */
      else if(peerListNode->TimerCode == PLNT_MAX_TIME_NO_RESPONSE) {
         LOG_ACTION
         fputs("Peer ", stdlog);
         ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
         fprintf(stdlog, " did not answer in MaxTimeNoResponse=%lluus -> removing it\n",
               registrar->PeerMaxTimeLastHeard);
         LOG_END

         CHECK(peerListNode->TakeoverProcess == NULL);
         if(peerListNode->TakeoverRegistrarID == UNDEFINED_REGISTRAR_IDENTIFIER) {
            /* Nobody else seems to have started a takeover yet ... */
            if(ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &registrar->Handlespace.Handlespace, peerListNode->Identifier) != NULL) {
               peerListNode->TakeoverProcess = takeoverProcessNew(
                                                  peerListNode->Identifier,
                                                  &registrar->Peers);
               if(peerListNode->TakeoverProcess) {
                  LOG_ACTION
                  fprintf(stdlog, "Initiating takeover of dead peer $%08x\n",
                        peerListNode->Identifier);
                  LOG_END
                  sendENRPInitTakeover(registrar, peerListNode->Identifier);
                  ST_CLASS(peerListManagementDeactivateTimer)(
                     &registrar->Peers, peerListNode);
                  ST_CLASS(peerListManagementActivateTimer)(
                     &registrar->Peers, peerListNode, PLNT_TAKEOVER_EXPIRY,
                     getMicroTime() + registrar->TakeoverExpiryInterval);
               }
            }
            else {
               LOG_ACTION
               fprintf(stdlog, "Dead peer $%08x has no PEs -> no takeover procedure necessary\n",
                     peerListNode->Identifier);
               LOG_END
               ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                        &registrar->Peers,
                        peerListNode);
            }
         }
         else {
            LOG_ACTION
            fprintf(stdlog, "Dead peer $%08x. Removing it from the peer list\n",
                  peerListNode->Identifier);
            LOG_END
            ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                     &registrar->Peers,
                     peerListNode);
         }
      }

      /* ====== Takeover expiry timer ==================================== */
      else if(peerListNode->TimerCode == PLNT_TAKEOVER_EXPIRY) {
         CHECK(peerListNode->TakeoverProcess != NULL);

         outstandingAcks = takeoverProcessGetOutstandingAcks(peerListNode->TakeoverProcess);
         LOG_WARNING
         fprintf(stdlog, "Takeover of peer $%08x has expired, %u outstanding acknowledgements:\n",
                 peerListNode->Identifier,
                 (unsigned int)outstandingAcks);
         for(i = 0;i < outstandingAcks;i++) {
            fprintf(stdlog, "- $%08x (%s)\n",
                    peerListNode->TakeoverProcess->PeerIDArray[i],
                    (ST_CLASS(peerListManagementFindPeerListNode)(
                              &registrar->Peers,
                              peerListNode->TakeoverProcess->PeerIDArray[i],
                              NULL) == NULL) ? "not found!" : "still available");
         }
         LOG_END

         takeoverProcessDelete(peerListNode->TakeoverProcess);
         peerListNode->TakeoverProcess = NULL;
         ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                  &registrar->Peers,
                  peerListNode);
      }

      /* ====== Bad timer ================================================ */
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
static void sendENRPInitTakeover(struct Registrar*             registrar,
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

      peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
      while(peerListNode != NULL) {
         message->ReceiverID   = peerListNode->Identifier;
         message->AddressArray = peerListNode->AddressBlock->AddressArray;
         message->Addresses    = peerListNode->AddressBlock->Addresses;
         LOG_VERBOSE
         fprintf(stdlog, "Sending InitTakeover to unicast peer $%08x...\n",
                 peerListNode->Identifier);
         LOG_END
         rserpoolMessageSend(IPPROTO_SCTP,
                             registrar->ENRPUnicastSocket,
                             0, 0, 0, 0,
                             message);
         peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers, peerListNode);
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Send peer init takeover acknowledgement ######################## */
static void sendENRPInitTakeoverAck(struct Registrar*             registrar,
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
                             sd, assocID, 0, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending InitTakeoverAck failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Send peer name update ########################################## */
static void sendENRPHandleUpdate(struct Registrar*                 registrar,
                                 struct ST_CLASS(PoolElementNode)* poolElementNode,
                                 const uint16_t                    action)
{
#ifndef MSG_SEND_TO_ALL
   struct ST_CLASS(PeerListNode)* peerListNode;
#endif
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

#ifndef MSG_SEND_TO_ALL
      peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
      while(peerListNode != NULL) {
         message->ReceiverID   = peerListNode->Identifier;
         message->AddressArray = peerListNode->AddressBlock->AddressArray;
         message->Addresses    = peerListNode->AddressBlock->Addresses;
         LOG_VERBOSE
         fprintf(stdlog, "Sending HandleUpdate to unicast peer $%08x...\n",
                 peerListNode->Identifier);
         LOG_END
         rserpoolMessageSend(IPPROTO_SCTP,
                             registrar->ENRPUnicastSocket,
                             0, 0, 0, 0,
                             message);
         peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers, peerListNode);
      }
#else
#warning Using MSG_SEND_TO_ALL!
      rserpoolMessageSend(IPPROTO_SCTP,
                          registrar->ENRPUnicastSocket,
                          0,
                          MSG_SEND_TO_ALL, 0, 0, message);
#endif

      rserpoolMessageDelete(message);
   }
}


/* ###### Round distance to nearest step ################################# */
static unsigned int roundDistance(const unsigned int distance,
                                  const unsigned int step)
{
   const unsigned int d = (unsigned int)rint((double)distance / step) * step;
   return(d);
}


/* ###### Update distance for distance-sensitive policies ################ */
static void updateDistance(struct Registrar*                       registrar,
                           int                                     fd,
                           sctp_assoc_t                            assocID,
                           const struct ST_CLASS(PoolElementNode)* poolElementNode,
                           struct PoolPolicySettings*              updatedPolicySettings,
                           bool                                    addDistance,
                           unsigned int*                           distance)
{
   struct sctp_status assocStatus;
   socklen_t          assocStatusLength;

   *updatedPolicySettings = poolElementNode->PolicySettings;

   /* ====== Set distance for distance-sensitive policies ======= */
   if((poolElementNode->PolicySettings.PolicyType == PPT_LEASTUSED_DPF) ||
      (poolElementNode->PolicySettings.PolicyType == PPT_LEASTUSED_DEGRADATION_DPF) ||
      (poolElementNode->PolicySettings.PolicyType == PPT_WEIGHTED_RANDOM_DPF)) {
      if(*distance == 0xffffffff) {
         assocStatusLength = sizeof(assocStatus);
         assocStatus.sstat_assoc_id = assocID;
         if(ext_getsockopt(fd, IPPROTO_SCTP, SCTP_STATUS, (char*)&assocStatus, &assocStatusLength) == 0) {
            *distance = roundDistance(assocStatus.sstat_primary.spinfo_srtt / 2,
                                      registrar->DistanceStep);
            LOG_VERBOSE
            fprintf(stdlog, " FD %d, assoc %u: primary=", fd, (unsigned int)assocID);
            fputaddress((struct sockaddr*)&assocStatus.sstat_primary.spinfo_address,
                        false, stdlog);
            fprintf(stdlog, " cwnd=%u srtt=%u rto=%u mtu=%u\n",
                    assocStatus.sstat_primary.spinfo_cwnd,
                    assocStatus.sstat_primary.spinfo_srtt,
                    assocStatus.sstat_primary.spinfo_rto,
                    assocStatus.sstat_primary.spinfo_mtu);
            LOG_END
         }
         else {
            LOG_WARNING
            logerror("Unable to obtain SCTP_STATUS");
            LOG_END
            *distance = 0;
         }
      }

      if(addDistance) {
         updatedPolicySettings->Distance += *distance;
      }
      else {
         updatedPolicySettings->Distance = *distance;
      }
   }
   else {
      *distance = 0;
   }
}


/* ###### Handle registration request #################################### */
static void handleASAPRegistration(struct Registrar*       registrar,
                                   int                     fd,
                                   sctp_assoc_t            assocID,
                                   struct RSerPoolMessage* message)
{

   char                              remoteAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     remoteAddressBlock = (struct TransportAddressBlock*)&remoteAddressBlockBuffer;
   char                              asapTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     asapTransportAddressBlock = (struct TransportAddressBlock*)&asapTransportAddressBlockBuffer;
   char                              userTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     userTransportAddressBlock = (struct TransportAddressBlock*)&userTransportAddressBlockBuffer;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct PoolPolicySettings         updatedPolicySettings;
   unsigned int                      distance;

   LOG_ACTION
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
                                                      MAX_PE_TRANSPORTADDRESSES,
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
                                     MAX_PE_TRANSPORTADDRESSES, false, MIN_ENDPOINT_ADDRESS_SCOPE) > 0) {
         LOG_VERBOSE2
         fputs("Filtered user transport addresses: ", stdlog);
         transportAddressBlockPrint(userTransportAddressBlock, stdlog);
         fputs("\n", stdlog);
         LOG_END
         if(transportAddressBlockFilter(remoteAddressBlock,
                                        NULL,
                                        asapTransportAddressBlock,
                                        MAX_PE_TRANSPORTADDRESSES, true, MIN_ENDPOINT_ADDRESS_SCOPE) > 0) {
            LOG_VERBOSE2
            fputs("Filtered ASAP transport addresses: ", stdlog);
            transportAddressBlockPrint(asapTransportAddressBlock, stdlog);
            fputs("\n", stdlog);
            LOG_END

            /* ====== Set distance for distance-sensitive policies ======= */
            distance = 0xffffffff;
            updateDistance(registrar,
                           fd, assocID, message->PoolElementPtr,
                           &updatedPolicySettings, false, &distance);

            message->Error = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                              &registrar->Handlespace,
                              &message->Handle,
                              registrar->ServerID,
                              message->PoolElementPtr->Identifier,
                              message->PoolElementPtr->RegistrationLife,
                              &updatedPolicySettings,
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

                  registrar->ReregistrationCount++;   /* We have a re-registration here */
               }
               else {
                  registrar->RegistrationCount++;   /* We have a new registration here */
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
               sendENRPHandleUpdate(registrar, poolElementNode, PNUP_ADD_PE);
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
      if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, 0, message) == false) {
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
static void handleASAPDeregistration(struct Registrar*       registrar,
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

            delPoolElementNode.HomeRegistrarIdentifier = registrar->ServerID;
            sendENRPHandleUpdate(registrar, &delPoolElementNode, PNUP_DEL_PE);

            LOG_ACTION
            fputs("Deregistration successfully completed\n", stdlog);
            LOG_END
            LOG_VERBOSE3
            fputs("Handlespace content:\n", stdlog);
            registrarDumpHandlespace(registrar);
            LOG_END

            registrar->DeregistrationCount++;
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

   if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, 0, message) == false) {
      LOG_WARNING
      logerror("Sending DeregistrationResponse failed");
      LOG_END
   }
}


/* ###### Handle handle resolution request ################################# */
static void handleASAPHandleResolution(struct Registrar*       registrar,
                                       int                     fd,
                                       sctp_assoc_t            assocID,
                                       struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNodeArray[MAX_MAX_HANDLE_RESOLUTION_ITEMS];
   size_t                            poolElementNodes = MAX_MAX_HANDLE_RESOLUTION_ITEMS;
   size_t                            items;
   size_t                            i;

   items = message->Addresses;
   if(items == 0) {
      items = registrar->MaxHandleResolutionItems;
   }
   items = min(items, MAX_MAX_HANDLE_RESOLUTION_ITEMS);

   LOG_ACTION
   fputs("Handle Resolution request for pool ", stdlog);
   poolHandlePrint(&message->Handle, stdlog);
   fprintf(stdlog, " for %u items (%u requested)\n",
           (unsigned int)items, (unsigned int)message->Addresses);
   LOG_END


   registrar->HandleResolutionCount++;

   message->Type  = AHT_HANDLE_RESOLUTION_RESPONSE;
   message->Flags = 0x00;
   message->Error = ST_CLASS(poolHandlespaceManagementHandleResolution)(
                       &registrar->Handlespace,
                       &message->Handle,
                       (struct ST_CLASS(PoolElementNode)**)&poolElementNodeArray,
                       &poolElementNodes,
                       items,
                       registrar->MaxIncrement);
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

   if(rserpoolMessageSend(IPPROTO_SCTP, fd, assocID, 0, 0, 0, message) == false) {
      LOG_WARNING
      logerror("Sending handle resolution response failed");
      LOG_END
   }
}


/* ###### Handle endpoint keepalive acknowledgement ###################### */
static void handleASAPEndpointKeepAliveAck(struct Registrar*       registrar,
                                            int                     fd,
                                            sctp_assoc_t            assocID,
                                            struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   LOG_VERBOSE2
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
static void handleASAPEndpointUnreachable(struct Registrar*       registrar,
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

   registrar->FailureReportCount++;

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
            sendENRPHandleUpdate(registrar, poolElementNode, PNUP_DEL_PE);
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
static void handleENRPHandleUpdate(struct Registrar*       registrar,
                                   int                     fd,
                                   sctp_assoc_t            assocID,
                                   struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct PoolPolicySettings         updatedPolicySettings;
   unsigned int                      distance;
   int                               result;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own HandleUpdate message\n", stdlog);
      LOG_END
      return;
   }

   registrar->UpdateCount++;

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
         updateDistance(registrar,
                        fd, assocID, message->PoolElementPtr,
                        &updatedPolicySettings, true, &distance);

         result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                     &registrar->Handlespace,
                     &message->Handle,
                     message->PoolElementPtr->HomeRegistrarIdentifier,
                     message->PoolElementPtr->Identifier,
                     message->PoolElementPtr->RegistrationLife,
                     &updatedPolicySettings,
                     message->PoolElementPtr->UserTransport,
                     message->PoolElementPtr->RegistratorTransport,
                     -1, 0,
                     getMicroTime(),
                     &newPoolElementNode);
         if(result == RSPERR_OKAY) {
            LOG_VERBOSE
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
         LOG_WARNING
         fprintf(stdlog, "PR $%08x sent me a HandleUpdate for a PE owned by myself!\n",
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
static void handleENRPPresence(struct Registrar*       registrar,
                               int                     fd,
                               sctp_assoc_t            assocID,
                               struct RSerPoolMessage* message)
{
   HandlespaceChecksumType        checksum;
   struct ST_CLASS(PeerListNode)* peerListNode;
   char                           remoteAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*  remoteAddressBlock = (struct TransportAddressBlock*)&remoteAddressBlockBuffer;
   char                           enrpTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*  enrpTransportAddressBlock = (struct TransportAddressBlock*)&enrpTransportAddressBlockBuffer;
   int                            result;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own Presence message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE2
   fprintf(stdlog, "Got Presence from peer $%08x",
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
                               1, MAX_PE_TRANSPORTADDRESSES);
   }
   else {
      if(transportAddressBlockGetAddressesFromSCTPSocket(remoteAddressBlock,
                                                         fd, assocID,
                                                         MAX_PE_TRANSPORTADDRESSES,
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
                                        MAX_PE_TRANSPORTADDRESSES, true, MIN_ENDPOINT_ADDRESS_SCOPE) == 0) {
            LOG_WARNING
            fprintf(stdlog, "Presence from peer $%08x contains no usable ENRP endpoint addresses\n",
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
         return;
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
      result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                  &registrar->Peers,
                  message->PeerListNodePtr->Identifier,
                  PLNF_DYNAMIC,
                  enrpTransportAddressBlock,
                  getMicroTime(),
                  &peerListNode);

      if(result == RSPERR_OKAY) {
         /* ====== Send Presence to new peer ============================= */
         if(assocID == 0) {
            LOG_ACTION
            fputs("Sending Presence to new peer ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
            fputs("\n", stdlog);
            LOG_END
            sendENRPPresence(registrar,
                             registrar->ENRPUnicastSocket,
                             0, 0,
                             peerListNode->AddressBlock->AddressArray,
                             peerListNode->AddressBlock->Addresses,
                             peerListNode->Identifier,
                             true);
         }

         /* ====== Mentor query ========================================== */
         if( (registrar->InStartupPhase) &&
             (registrar->MentorServerID == UNDEFINED_REGISTRAR_IDENTIFIER) ) {
            LOG_ACTION
            fputs("Trying ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
            fputs(" as mentor server...\n", stdlog);
            LOG_END
            peerListNode->Status |= PLNS_LISTSYNC|PLNS_HTSYNC|PLNS_MENTOR;
            registrar->MentorServerID = peerListNode->Identifier;
            sendENRPListRequest(registrar,
                                registrar->ENRPUnicastSocket,
                                0, 0,
                                peerListNode->AddressBlock->AddressArray,
                                peerListNode->AddressBlock->Addresses,
                                peerListNode->Identifier);
            sendENRPHandleTableRequest(registrar,
                                       registrar->ENRPUnicastSocket,
                                       0, 0,
                                       peerListNode->AddressBlock->AddressArray,
                                       peerListNode->AddressBlock->Addresses,
                                       peerListNode->Identifier,
                                       0x00);
         }

         /* ====== Check if synchronization is necessary ================= */
         if(assocID != 0) {
            if(!(peerListNode->Status & PLNS_HTSYNC)) {
               /* Attention: We only synchronize on SCTP-received Presence messages! */
               checksum = ST_CLASS(peerListNodeGetOwnershipChecksum)(
                             peerListNode);
               if(checksum != message->Checksum) {
                  LOG_ACTION
                  fprintf(stdlog, "Handle Table synchronization with peer $%08x is necessary -> requesting it...\n",
                          peerListNode->Identifier);
                  LOG_END
                  LOG_VERBOSE
                  fprintf(stdlog, "Checksum is $%x, should be $%x\n",
                          checksum, message->Checksum);
                  LOG_END

                  peerListNode->Status |= PLNS_HTSYNC;
                  sendENRPHandleTableRequest(registrar, fd, assocID, 0,
                                             NULL, 0,
                                             peerListNode->Identifier,
                                             EHF_HANDLE_TABLE_REQUEST_OWN_CHILDREN_ONLY);
                  ST_CLASS(poolHandlespaceManagementMarkPoolElementNodes)(&registrar->Handlespace,
                                                                          message->SenderID);
               }
            }
            else {
               LOG_ACTION
               fprintf(stdlog, "Synchronization with peer $%08x is still in progress -> no new synchronization\n",
                       peerListNode->Identifier);
               LOG_END
            }
            if(!(peerListNode->Status & PLNS_LISTSYNC)) {
               LOG_ACTION
               fprintf(stdlog, "Peer List synchronization with peer $%08x is necessary -> requesting it...\n",
                        peerListNode->Identifier);
               LOG_END
               peerListNode->Status |= PLNS_LISTSYNC;
               sendENRPListRequest(registrar, fd, assocID, 0,
                                   NULL, 0,
                                   peerListNode->Identifier);
            }
         }

         /* ====== Activate keep alive timer ============================= */
         if(STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode)) {
            ST_CLASS(peerListManagementDeactivateTimer)(
               &registrar->Peers, peerListNode);
         }
         ST_CLASS(peerListManagementActivateTimer)(
            &registrar->Peers, peerListNode, PLNT_MAX_TIME_LAST_HEARD,
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
            (message->SenderID != UNDEFINED_REGISTRAR_IDENTIFIER)) {
            LOG_VERBOSE
            fputs("Presence with ReplyRequired flag set -> Sending reply...\n", stdlog);
            LOG_END
            sendENRPPresence(registrar,
                             fd, message->AssocID, 0,
                             NULL, 0,
                             message->SenderID,
                             false);
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

      /* ====== Takeover handling ======================================== */
      peerListNode->TakeoverRegistrarID = UNDEFINED_REGISTRAR_IDENTIFIER;
      if(peerListNode->TakeoverProcess) {
         takeoverProcessDelete(peerListNode->TakeoverProcess);
         peerListNode->TakeoverProcess = NULL;
      }
   }
}


/* ###### Handle peer init takeover ###################################### */
static void handleENRPInitTakeover(struct Registrar*       registrar,
                                   int                     fd,
                                   sctp_assoc_t            assocID,
                                   struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;

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

   /* ====== We are the takeover target -> try to stop it by Peer Presence */
   if(message->RegistrarIdentifier == registrar->ServerID) {
      LOG_WARNING
      fprintf(stdlog, "Peer $%08x has initiated takeover -> trying to stop this by Presences\n",
              message->SenderID);
      LOG_END
      /* ====== Send presence as multicast announce ====================== */
      if(registrar->ENRPAnnounceViaMulticast) {
         sendENRPPresence(registrar, registrar->ENRPMulticastOutputSocket, 0, 0,
                          (union sockaddr_union*)&registrar->ENRPMulticastAddress, 1,
                          0, false);
      }
      /* ====== Send presence to all peers =============================== */
      sendENRPPresenceToAllPeers(registrar);
   }

   /* ====== Another peer is the takeover target ========================= */
   else {
      peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                              &registrar->Peers, message->RegistrarIdentifier, NULL);
      if(peerListNode) {
         /* ====== We have also started a takeover -> negotiation required */
         if(peerListNode->TakeoverProcess != NULL) {
            if(message->SenderID < registrar->ServerID) {
               LOG_ACTION
               fprintf(stdlog, "Peer $%08x, also trying takeover of $%08x, has lower ID -> we ($%08x) abort our takeover\n",
                     message->SenderID,
                     message->RegistrarIdentifier,
                     registrar->ServerID);
               LOG_END
               /* Acknowledge peer's takeover */
               peerListNode->TakeoverRegistrarID = message->SenderID;
               sendENRPInitTakeoverAck(registrar,
                                       fd, assocID,
                                       message->SenderID, message->RegistrarIdentifier);
               /* Cancel our takeover process */
               takeoverProcessDelete(peerListNode->TakeoverProcess);
               peerListNode->TakeoverProcess = NULL;
               /* Schedule a max-time-no-response timer */
               ST_CLASS(peerListManagementDeactivateTimer)(
                  &registrar->Peers, peerListNode);
               ST_CLASS(peerListManagementActivateTimer)(
                  &registrar->Peers, peerListNode, PLNT_MAX_TIME_NO_RESPONSE,
                  getMicroTime() + registrar->PeerMaxTimeNoResponse);
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

         /* ====== Acknowledge takeover ================================== */
         else {
            LOG_ACTION
            fprintf(stdlog, "Acknowledging peer $%08x's takeover of $%08x\n",
                  message->SenderID,
                  message->RegistrarIdentifier);
            LOG_END
            peerListNode->TakeoverRegistrarID = message->SenderID;
            sendENRPInitTakeoverAck(registrar,
                                    fd, assocID,
                                    message->SenderID, message->RegistrarIdentifier);
         }
      }

      /* ====== The target is unknown. Agree to takeover. ================ */
      else {
         LOG_WARNING
         fprintf(stdlog, "Acknowledging peer $%08x's takeover of $%08x (which is unknown for us!)\n",
               message->SenderID,
               message->RegistrarIdentifier);
         LOG_END
         sendENRPInitTakeoverAck(registrar,
                                 fd, assocID,
                                 message->SenderID, message->RegistrarIdentifier);
      }
   }
}


/* ###### Send ENRP takeover server message ############################## */
static void sendENRPTakeoverServer(struct Registrar*             registrar,
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

      if(rserpoolMessageSend(IPPROTO_SCTP,
                             sd, assocID, msgSendFlags, 0, 0, message) == false) {
         LOG_WARNING
         fputs("Sending TakeoverServer failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(message);
   }
}


/* ###### Send ENRP takeover server message to all peers ################# */
static void sendENRPTakeoverServerToAllPeers(struct Registrar*             registrar,
                                             const RegistrarIdentifierType targetID)
{
#ifndef MSG_SEND_TO_ALL
   struct ST_CLASS(PeerListNode)* peerListNode;

   peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
   while(peerListNode != NULL) {
      if(peerListNode->Identifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
         sendENRPTakeoverServer(registrar,
                                registrar->ENRPUnicastSocket, 0, 0,
                                NULL, 0,
                                peerListNode->Identifier,
                                targetID);
      }
      peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                        &registrar->Peers, peerListNode);
   }
#else
#warning Using MSG_SEND_TO_ALL!
   sendENRPTakeoverServer(registrar,
                          registrar->ENRPUnicastSocket, 0,
                          MSG_SEND_TO_ALL,
                          NULL, 0,
                          UNDEFINED_REGISTRAR_IDENTIFIER,
                          targetID);
#endif
}


/* ###### Do takeover of peer server ##################################### */
static void finishTakeover(struct Registrar*             registrar,
                           const RegistrarIdentifierType targetID,
                           struct TakeoverProcess*       takeoverProcess)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;

   LOG_WARNING
   fprintf(stdlog, "Taking over peer $%08x...\n", targetID);
   LOG_END

   /* ====== Send Takeover Servers ======================================= */
   sendENRPTakeoverServerToAllPeers(registrar, targetID);

   /* ====== Update PEs' home PR identifier ============================== */
   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &registrar->Handlespace.Handlespace, targetID);
   while(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                               &registrar->Handlespace.Handlespace, poolElementNode);

      LOG_ACTION
      fprintf(stdlog, "Taking ownership of pool element $%08x in pool ",
              poolElementNode->Identifier);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END

      /* Deactivate expiry timer */
      ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
         &registrar->Handlespace.Handlespace,
         poolElementNode);

      /* Update handlespace */
      ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
         &registrar->Handlespace.Handlespace,
         poolElementNode,
         registrar->ServerID);

      /* Tell node about new home PR */
      sendASAPEndpointKeepAlive(registrar, poolElementNode, true);

      /* Schedule endpoint keep-alive timeout */
      ST_CLASS(poolHandlespaceNodeActivateTimer)(
         &registrar->Handlespace.Handlespace,
         poolElementNode,
         PENT_KEEPALIVE_TIMEOUT,
         getMicroTime() + registrar->EndpointKeepAliveTimeoutInterval);

      poolElementNode = nextPoolElementNode;
   }

   /* ====== Restart the handlespace action timer ======================== */
   timerRestart(&registrar->HandlespaceActionTimer,
                ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                   &registrar->Handlespace));

   /* ====== Remove takeover process ===================================== */
   LOG_WARNING
   fprintf(stdlog, "Takeover of peer $%08x completed\n", targetID);
   LOG_END
}


/* ###### Handle peer init takeover ack ################################## */
static void handleENRPInitTakeoverAck(struct Registrar*       registrar,
                                      int                     fd,
                                      sctp_assoc_t            assocID,
                                      struct RSerPoolMessage* message)
{
   struct ST_CLASS(PeerListNode)* peerListNode;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own InitTakeoverAck message\n", stdlog);
      LOG_END
      return;
   }

   peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                     &registrar->Peers, message->RegistrarIdentifier, NULL);
   if((peerListNode) && (peerListNode->TakeoverProcess)) {
      LOG_VERBOSE
      fprintf(stdlog, "Got InitTakeoverAck from peer $%08x for target $%08x\n",
            message->SenderID,
            message->RegistrarIdentifier);
      LOG_END

      if(takeoverProcessAcknowledge(peerListNode->TakeoverProcess,
                                    message->RegistrarIdentifier,
                                    message->SenderID) > 0) {
         LOG_ACTION
         fprintf(stdlog, "Peer $%08x acknowledges takeover of target $%08x. %u acknowledges to go.\n",
               message->SenderID,
               message->RegistrarIdentifier,
               (unsigned int)takeoverProcessGetOutstandingAcks(peerListNode->TakeoverProcess));
         LOG_END
      }
      else {
         LOG_ACTION
         fprintf(stdlog, "Takeover of target $%08x confirmed. Taking it over now.\n",
                 message->RegistrarIdentifier);
         LOG_END
         finishTakeover(registrar, message->RegistrarIdentifier, peerListNode->TakeoverProcess);
         takeoverProcessDelete(peerListNode->TakeoverProcess);
         peerListNode->TakeoverProcess = NULL;
         ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                  &registrar->Peers,
                  peerListNode);
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
static void handleENRPTakeoverServer(struct Registrar*       registrar,
                                     int                     fd,
                                     sctp_assoc_t            assocID,
                                     struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;

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

   /* ====== Update PEs' home PR identifier ============================== */
   poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                        &registrar->Handlespace.Handlespace, message->RegistrarIdentifier);
   while(poolElementNode) {
      nextPoolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                               &registrar->Handlespace.Handlespace, poolElementNode);

      LOG_ACTION
      fprintf(stdlog, "Changing ownership of pool element $%08x in pool ",
              poolElementNode->Identifier);
      poolHandlePrint(&poolElementNode->OwnerPoolNode->Handle, stdlog);
      fputs("\n", stdlog);
      LOG_END

      /* Update handlespace */
      ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
         &registrar->Handlespace.Handlespace,
         poolElementNode,
         message->SenderID);

      poolElementNode = nextPoolElementNode;
   }
}


/* ###### Handle peer list request ####################################### */
static void handleENRPListRequest(struct Registrar*       registrar,
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
      fprintf(stdlog, "Sending ListResponse to peer $%08x...\n",
              message->SenderID);
      LOG_END
      if(rserpoolMessageSend(IPPROTO_SCTP,
                             fd, assocID, 0, 0, 0, response) == false) {
         LOG_WARNING
         fputs("Sending ListResponse failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(response);
   }
}


/* ###### Handle peer list response ###################################### */
static void handleENRPListResponse(struct Registrar*       registrar,
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
      fputs("Skipping our own ListResponse message\n", stdlog);
      LOG_END
      return;
   }

   LOG_VERBOSE
   fprintf(stdlog, "Got ListResponse from peer $%08x\n",
           message->SenderID);
   LOG_END

   /* ====== Handle response ============================================= */
   if(!(message->Flags & EHF_LIST_RESPONSE_REJECT)) {
      if(message->PeerListPtr) {
         peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                           &message->PeerListPtr->List);
         while(peerListNode) {
            LOG_VERBOSE2
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
                  ST_CLASS(peerListManagementActivateTimer)(
                     &registrar->Peers, newPeerListNode, PLNT_MAX_TIME_LAST_HEARD,
                     getMicroTime() + registrar->PeerMaxTimeLastHeard);

                  /* ====== New peer -> Send Peer Presence =============== */
                  sendENRPPresence(registrar,
                                   registrar->ENRPUnicastSocket, 0, 0,
                                   newPeerListNode->AddressBlock->AddressArray,
                                   newPeerListNode->AddressBlock->Addresses,
                                   newPeerListNode->Identifier,
                                   false);
               }
            }
            else {
               LOG_ACTION
               fputs("Skipping unknown peer (due to ID=0) from ListResponse: ", stdlog);
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
      fprintf(stdlog, "Rejected ListResponse from peer $%08x\n",
              message->SenderID);
      LOG_END
   }
}


/* ###### Handle peer handle table request ################################# */
static void handleENRPHandleTableRequest(struct Registrar*       registrar,
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

   LOG_VERBOSE2
   fprintf(stdlog, "Got HandleTableRequest from peer $%08x\n",
           message->SenderID);
   LOG_END

   registrar->SynchronizationCount++;

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
      response->MaxElementsPerHTRequest   = registrar->MaxElementsPerHTRequest;

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
                             fd, assocID, 0, 0, 0, response) == false) {
         LOG_WARNING
         fputs("Sending HandleTableResponse failed\n", stdlog);
         LOG_END
      }

      rserpoolMessageDelete(response);
   }
}


/* ###### Handle handle table response ##################################### */
static void handleENRPHandleTableResponse(struct Registrar*       registrar,
                                          int                     fd,
                                          sctp_assoc_t            assocID,
                                          struct RSerPoolMessage* message)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct ST_CLASS(PeerListNode)*    peerListNode;
   struct PoolPolicySettings         updatedPolicySettings;
   unsigned int                      distance;
   unsigned int                      result;
   size_t                            purged;

   if(message->SenderID == registrar->ServerID) {
      /* This is our own message -> skip it! */
      LOG_VERBOSE5
      fputs("Skipping our own HandleTableResponse message\n", stdlog);
      LOG_END
      return;
   }

   LOG_ACTION
   fprintf(stdlog, "Got HandleTableResponse from peer $%08x\n",
           message->SenderID);
   LOG_END


   /* ====== Find peer list node ========================================= */
   peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                           &registrar->Peers, message->SenderID, NULL);
   if(peerListNode == NULL) {
      LOG_WARNING
      fprintf(stdlog, "Got HandleTableResponse from peer $%08x which is not in peer list\n",
            message->SenderID);
      LOG_END
      return;
   }

   /* ====== Propagate response data into the handlespace ================ */
   if(!(message->Flags & EHF_HANDLE_TABLE_RESPONSE_REJECT)) {
      if(message->HandlespacePtr) {
         distance = 0xffffffff;
         poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(&message->HandlespacePtr->Handlespace);
         while(poolElementNode != NULL) {
            /* ====== Set distance for distance-sensitive policies ======= */
            updateDistance(registrar,
                           fd, assocID, poolElementNode,
                           &updatedPolicySettings, true, &distance);

            if(poolElementNode->HomeRegistrarIdentifier != registrar->ServerID) {
               result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                           &registrar->Handlespace,
                           &poolElementNode->OwnerPoolNode->Handle,
                           poolElementNode->HomeRegistrarIdentifier,
                           poolElementNode->Identifier,
                           poolElementNode->RegistrationLife,
                           &updatedPolicySettings,
                           poolElementNode->UserTransport,
                           poolElementNode->RegistratorTransport,
                           -1, 0,
                           getMicroTime(),
                           &newPoolElementNode);
               if(result == RSPERR_OKAY) {
                  LOG_VERBOSE
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
               LOG_WARNING
               fprintf(stdlog, "PR $%08x sent me a HandleTableResponse containing a PE owned by myself!\n",
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
            LOG_ACTION
            fprintf(stdlog, "HandleTableResponse has MoreToSend flag set -> sending HandleTableRequest to peer $%08x to get more data\n",
                    message->SenderID);
            LOG_END
            sendENRPHandleTableRequest(registrar, fd, message->AssocID, 0, NULL, 0, message->SenderID,
                                       (peerListNode->Status & PLNS_MENTOR) ? 0x00 : EHF_HANDLE_TABLE_REQUEST_OWN_CHILDREN_ONLY);
         }
         else {
            peerListNode->Status &= ~(PLNS_MENTOR|PLNS_HTSYNC);   /* Synchronization completed */
            purged = ST_CLASS(poolHandlespaceManagementPurgeMarkedPoolElementNodes)(
                        &registrar->Handlespace, message->SenderID);
            if(purged) {
               LOG_ACTION
               fprintf(stdlog, "Purged %u PEs after last Handle Table Response from $%08x\n",
                       (unsigned int)purged, message->SenderID);
               LOG_END
            }
            if( (registrar->InStartupPhase) &&
                (registrar->MentorServerID == message->SenderID) ) {
               beginNormalOperation(registrar, true);
            }
         }
      }
      else {
         peerListNode->Status &= ~(PLNS_MENTOR|PLNS_HTSYNC);   /* Synchronization completed */
      }
   }
   else {
      LOG_ACTION
      fprintf(stdlog, "Peer $%08x has rejected the HandleTableRequest\n",
              message->SenderID);
      LOG_END
      peerListNode->Status &= ~(PLNS_MENTOR|PLNS_HTSYNC);   /* Synchronization completed */
   }
}


/* ###### Handle incoming message ######################################## */
static void handleMessage(struct Registrar*       registrar,
                          struct RSerPoolMessage* message,
                          int                     sd)
{
   switch(message->Type) {
      case AHT_REGISTRATION:
         handleASAPRegistration(registrar, sd, message->AssocID, message);
       break;
      case AHT_HANDLE_RESOLUTION:
         handleASAPHandleResolution(registrar, sd, message->AssocID, message);
       break;
      case AHT_DEREGISTRATION:
         handleASAPDeregistration(registrar, sd, message->AssocID, message);
       break;
      case AHT_ENDPOINT_KEEP_ALIVE_ACK:
         handleASAPEndpointKeepAliveAck(registrar, sd, message->AssocID, message);
       break;
      case AHT_ENDPOINT_UNREACHABLE:
         handleASAPEndpointUnreachable(registrar, sd, message->AssocID, message);
       break;
      case EHT_PRESENCE:
         handleENRPPresence(registrar, sd, message->AssocID, message);
       break;
      case EHT_HANDLE_UPDATE:
         handleENRPHandleUpdate(registrar, sd, message->AssocID, message);
       break;
      case EHT_LIST_REQUEST:
         handleENRPListRequest(registrar, sd, message->AssocID, message);
       break;
      case EHT_LIST_RESPONSE:
         handleENRPListResponse(registrar, sd, message->AssocID, message);
       break;
      case EHT_HANDLE_TABLE_REQUEST:
         handleENRPHandleTableRequest(registrar, sd, message->AssocID, message);
       break;
      case EHT_HANDLE_TABLE_RESPONSE:
         handleENRPHandleTableResponse(registrar, sd, message->AssocID, message);
       break;
      case EHT_INIT_TAKEOVER:
         handleENRPInitTakeover(registrar, sd, message->AssocID, message);
       break;
      case EHT_INIT_TAKEOVER_ACK:
         handleENRPInitTakeoverAck(registrar, sd, message->AssocID, message);
       break;
      case EHT_TAKEOVER_SERVER:
         handleENRPTakeoverServer(registrar, sd, message->AssocID, message);
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
   struct MessageBuffer*    messageBuffer;
   int                      flags;
   uint32_t                 ppid;
   sctp_assoc_t             assocID;
   unsigned short           streamID;
   ssize_t                  received;
   unsigned int             result;

   CHECK((fd == registrar->ASAPSocket) ||
         (fd == registrar->ENRPUnicastSocket) ||
         ((registrar->ENRPMulticastInputSocket >= 0) && (fd == registrar->ENRPMulticastInputSocket)));
   LOG_VERBOSE3
   fprintf(stdlog, "Event on socket %d...\n", fd);
   LOG_END

   if(fd == registrar->ASAPSocket) {
      messageBuffer = registrar->ASAPMessageBuffer;
   }
   else if(fd == registrar->ENRPUnicastSocket) {
      messageBuffer = registrar->ENRPUnicastMessageBuffer;
   }
   else {
      messageBuffer = registrar->UDPMessageBuffer;
   }

   flags               = 0;
   remoteAddressLength = sizeof(remoteAddress);
   received = messageBufferRead(messageBuffer, fd, &flags,
                                (struct sockaddr*)&remoteAddress,
                                &remoteAddressLength,
                                &ppid, &assocID, &streamID, 0);
   if(received > 0) {
      if(!(flags & MSG_NOTIFICATION)) {
         if(!( (((ppid == PPID_ASAP) && (fd != registrar->ASAPSocket)) ||
                ((ppid == PPID_ENRP) && (fd != registrar->ENRPUnicastSocket))) )) {

            if(fd == registrar->ENRPMulticastInputSocket) {
               /* ENRP via UDP -> Set PPID so that rserpoolPacket2Message can
                  correctly decode the packet */
               ppid = PPID_ENRP;
            }

            result = rserpoolPacket2Message(messageBuffer->Buffer,
                                            &remoteAddress, assocID, ppid,
                                            received, messageBuffer->BufferSize, &message);
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
               else if(message->Error != RSPERR_UNRECOGNIZED_PARAMETER_SILENT) {
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
                                         fd, assocID, 0, 0, 0, message);
                  }
               }
               rserpoolMessageDelete(message);
            }
         }
         else {
            LOG_WARNING
            fprintf(stdlog, "Received PPID $%08x on wrong socket -> Sending ABORT to assoc %u!\n",
                    ppid, (unsigned int)assocID);
            LOG_END
            sendabort(fd, assocID);
         }
      }
      else {
         notification = (union sctp_notification*)messageBuffer->Buffer;
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
   else if(received != MBRead_Partial) {
      LOG_WARNING
      logerror("Unable to read from registrar socket");
      LOG_END
   }
}


#ifdef ENABLE_CSP
/* ###### Get CSP report ################################################# */
static size_t registrarGetReportFunction(
                 void*                         userData,
                 uint64_t*                     identifier,
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
   size_t                         ownedPoolElements;

   LOG_VERBOSE4
   fputs("Sending a Component Status Protocol report...\n", stdlog);
   LOG_END
   peers             = ST_CLASS(peerListManagementGetPeers)(&registrar->Peers);
   pools             = ST_CLASS(poolHandlespaceManagementGetPools)(&registrar->Handlespace);
   poolElements      = ST_CLASS(poolHandlespaceManagementGetPoolElements)(&registrar->Handlespace);
   ownedPoolElements = ST_CLASS(poolHandlespaceManagementGetOwnedPoolElements)(&registrar->Handlespace);
   snprintf(statusText, CSPR_STATUS_SIZE,
            "%u[%u] PEs in %u Pool%s, %u Peer%s",
            (unsigned int)poolElements,
            (unsigned int)ownedPoolElements,
            (unsigned int)pools, (pools == 1) ? "" : "s",
            (unsigned int)peers, (peers == 1) ? "" : "s");
   getComponentLocation(componentLocation, registrar->ASAPSocket, 0);

   *workload    = -1.0;
   *caeArray    = createComponentAssociationArray(peers);
   caeArraySize = 0;
   if(*caeArray) {
      peerListNode = ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(&registrar->Peers);
      while(peerListNode != NULL) {

         (*caeArray)[caeArraySize].ReceiverID = CID_COMPOUND(CID_GROUP_REGISTRAR, peerListNode->Identifier);
         (*caeArray)[caeArraySize].Duration   = ~0;
         (*caeArray)[caeArraySize].Flags      = 0;
         (*caeArray)[caeArraySize].ProtocolID = IPPROTO_SCTP;
         (*caeArray)[caeArraySize].PPID       = PPID_ENRP;
         caeArraySize++;

         peerListNode = ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                           &registrar->Peers, peerListNode);
      }
   }
   return(caeArraySize);
}
#endif


/* ###### Add static peer ################################################ */
unsigned int registrarAddStaticPeer(
                struct Registrar*                   registrar,
                const RegistrarIdentifierType       identifier,
                const struct TransportAddressBlock* transportAddressBlock)
{
   struct ST_CLASS(PeerListNode)* peerListNode;
   int                            result;
   char                           localAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*  localAddressBlock = (struct TransportAddressBlock*)&localAddressBlockBuffer;

   /* ====== Skip own address ============================================ */
   if(transportAddressBlockGetAddressesFromSCTPSocket(localAddressBlock,
                                                      registrar->ENRPUnicastSocket, 0,
                                                      MAX_PE_TRANSPORTADDRESSES,
                                                      true) > 0) {
      if(transportAddressBlockOverlapComparison(localAddressBlock, transportAddressBlock) == 0) {
         return(RSPERR_OKAY);
      }
   }

   /* ====== Add static peer ============================================= */
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
   union sockaddr_union          addressArray[MAX_PE_TRANSPORTADDRESSES];
   size_t                        addresses;
   char*                         address;
   char*                         idx;

   addresses = 0;
   address = arg;
   while(addresses < MAX_PE_TRANSPORTADDRESSES) {
      idx = strindex(address, ',');
      if(idx) {
         *idx = 0x00;
      }
      if(!string2address(address, &addressArray[addresses])) {
         fprintf(stderr, "ERROR: Bad address %s! Use format <address:port>.\n",address);
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
                            addresses, MAX_PE_TRANSPORTADDRESSES);
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
                          const bool                    requiresUDPSocket,
                          const char*                   udpGroupAddressParameter,
                          struct TransportAddressBlock* udpGroupTransportAddress,
                          int*                          udpSocket,
                          const bool                    useIPv6)
{
   union sockaddr_union        sctpAddressArray[MAX_PE_TRANSPORTADDRESSES];
   union sockaddr_union        groupAddress;
   union sockaddr_union        udpAddress;
   uint16_t                    localPort;
   size_t                      i;
   int                         family;
   const char*                 address;
   char*                       idx;
   struct sctp_event_subscribe sctpEvents;
   size_t                      sctpAddresses;
   uint16_t                    configuredPort;
   size_t                      trials;

   sctpAddresses = 0;
   family        = useIPv6 ? AF_INET6 : AF_INET;

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
      while(sctpAddresses < MAX_PE_TRANSPORTADDRESSES) {
         idx = strindex((char*)address, ',');
         if(idx) {
            *idx = 0x00;
         }
         if(!string2address(address, &sctpAddressArray[sctpAddresses])) {
            fprintf(stderr, "ERROR: Bad local address %s! Use format <address:port>.\n", address);
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
      else {
         setPort((struct sockaddr*)&udpAddress, configuredPort);
         trials = 1024;
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

      if(!requiresUDPSocket) {
         break;
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

      if(configuredPort != 0) {
         break;
      }
   }

   if((*sctpSocket < 0) || (requiresUDPSocket && (*udpSocket < 0))) {
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
   /* sctpEvents.sctp_adaptation_layer_event = 1; */
   if(ext_setsockopt(*sctpSocket, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
      perror("setsockopt() for SCTP_EVENTS failed");
      exit(1);
   }

   transportAddressBlockNew(sctpTransportAddress,
                            IPPROTO_SCTP,
                            getPort((struct sockaddr*)&sctpAddressArray[0]),
                            0,
                            (union sockaddr_union*)&sctpAddressArray,
                            sctpAddresses, MAX_PE_TRANSPORTADDRESSES);
   transportAddressBlockNew(udpGroupTransportAddress,
                            IPPROTO_UDP,
                            getPort((struct sockaddr*)&groupAddress),
                            0,
                            (union sockaddr_union*)&groupAddress,
                            1, 1);
}



/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct Registrar*             registrar;
   uint32_t                      serverID                      = 0;

   char                          asapUnicastAddressBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* asapUnicastAddress            = (struct TransportAddressBlock*)&asapUnicastAddressBuffer;
   const char*                   asapUnicastAddressParameter   = "auto";
   int                           asapUnicastSocket             = -1;

   char                          asapAnnounceAddressBuffer[transportAddressBlockGetSize(1)];
   struct TransportAddressBlock* asapAnnounceAddress           = (struct TransportAddressBlock*)&asapAnnounceAddressBuffer;
   const char*                   asapAnnounceAddressParameter  = "0.0.0.0:0";
   int                           asapAnnounceSocket            = -1;
   bool                          asapSendAnnounces             = false;

   char                          enrpUnicastAddressBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* enrpUnicastAddress            = (struct TransportAddressBlock*)&enrpUnicastAddressBuffer;
   const char*                   enrpUnicastAddressParameter   = "auto";
   int                           enrpUnicastSocket             = -1;

   char                          enrpMulticastAddressBuffer[transportAddressBlockGetSize(1)];
   struct TransportAddressBlock* enrpMulticastAddress          = (struct TransportAddressBlock*)&enrpMulticastAddressBuffer;
   const char*                   enrpMulticastAddressParameter = "0.0.0.0:0";
   int                           enrpMulticastOutputSocket     = -1;
   int                           enrpMulticastInputSocket      = -1;
   bool                          enrpAnnounceViaMulticast      = false;
   bool                          useIPv6                       = checkIPv6();

#ifdef ENABLE_CSP
   union sockaddr_union          cspReportAddress;
   unsigned long long            cspReportInterval = 0;
#endif

   FILE*                         statsFile     = NULL;
   int                           statsInterval = -1;

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
      if(!(strncmp(argv[i], "-identifier=", 12))) {
         if(sscanf((const char*)&argv[i][12], "0x%x", &serverID) == 0) {
            if(sscanf((const char*)&argv[i][12], "%u", &serverID) == 0) {
               fputs("ERROR: Bad registrar ID given!\n", stderr);
               exit(1);
            }
         }
      }
      else if(!(strncmp(argv[i], "-peer=",6))) {
         /* to be handled later */
      }
      else if( (!(strncmp(argv[i], "-maxbadpereports=", 17))) ||
               (!(strncmp(argv[i], "-distancestep=", 14))) ||
               (!(strncmp(argv[i], "-autoclosetimeout=", 18))) ||
               (!(strncmp(argv[i], "-serverannouncecycle=", 21))) ||
               (!(strncmp(argv[i], "-endpointkeepalivetransmissioninterval=", 39))) ||
               (!(strncmp(argv[i], "-endpointkeepalivetimeoutinterval=", 34))) ||
               (!(strncmp(argv[i], "-peerheartbeatcycle=", 20))) ||
               (!(strncmp(argv[i], "-peermaxtimelastheard=", 22))) ||
               (!(strncmp(argv[i], "-peermaxtimenoresponse=", 23))) ||
               (!(strncmp(argv[i], "-mentordiscoverytimeout=", 19))) ||
               (!(strncmp(argv[i], "-takeoverexpiryinterval=", 24))) ||
               (!(strncmp(argv[i], "-maxincrement=", 14))) ||
               (!(strncmp(argv[i], "-maxhresitems=", 14))) ||
               (!(strncmp(argv[i], "-maxelementsperhtrequest=", 25))) ) {
         /* to be handled later */
      }
      else if(!(strncmp(argv[i], "-asap=",6))) {
         asapUnicastAddressParameter = (const char*)&argv[i][6];
      }
      else if(!(strncmp(argv[i], "-asapannounce=", 14))) {
         if( (!(strcasecmp((const char*)&argv[i][14], "off"))) ||
             (!(strcasecmp((const char*)&argv[i][14], "none"))) ) {
            asapAnnounceAddressParameter = "0.0.0.0:0";
            asapSendAnnounces            = false;
         }
         else {
            asapAnnounceAddressParameter = (const char*)&argv[i][14];
            asapSendAnnounces            = true;
         }
      }
      else if(!(strncmp(argv[i], "-enrp=",6))) {
         enrpUnicastAddressParameter = (const char*)&argv[i][6];
      }
      else if(!(strncmp(argv[i], "-enrpmulticast=", 15))) {
         if( (!(strcasecmp((const char*)&argv[i][15], "off"))) ||
             (!(strcasecmp((const char*)&argv[i][15], "none"))) ) {
            enrpMulticastAddressParameter = "0.0.0.0:0";
            enrpAnnounceViaMulticast      = false;
         }
         else {
            enrpMulticastAddressParameter = (const char*)&argv[i][15];
            enrpAnnounceViaMulticast      = true;
         }
      }
      else if(!(strcmp(argv[i], "-disable-ipv6"))) {
         useIPv6 = false;
      }
      else if(!(strncmp(argv[i], "-statsfile=", 11))) {
         if(statsFile) {
            fclose(statsFile);
         }
         statsFile = fopen((const char*)&argv[i][11], "w");
         if(statsFile == NULL) {
            fprintf(stderr, "ERROR: Unable to create statistics file \"%s\"!\n",
                    (char*)&argv[i][11]);
         }
      }
      else if(!(strncmp(argv[i], "-statsinterval=", 15))) {
         statsInterval = atol((const char*)&argv[i][15]);
         if(statsInterval < 10) {
            statsInterval = 10;
         }
         else if(statsInterval > 3600000) {
            statsInterval = 36000000;
         }
      }
      else if(!(strncmp(argv[i], "-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
#ifdef ENABLE_CSP
      else if(!(strncmp(argv[i], "-cspinterval=", 13))) {
         cspReportInterval = 1000ULL * atol((char*)&argv[i][13]);
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
         fprintf(stderr, "ERROR: Invalid argument <%s>!\n", argv[i]);
         fprintf(stderr, "Usage: %s {-asap=auto|address:port{,address}...} {[-asapannounce=auto|address:port}]} {-enrp=auto|address:port{,address}...} {[-enrpmulticast=auto|address:port}]} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} "
#ifdef ENABLE_CSP
            "{-cspserver=address} {-cspinterval=microseconds} "
#endif
            "{-identifier=registrar identifier} "
            "{-disable-ipv6} "
            "{-autoclosetimeout=seconds} {-serverannouncecycle=milliseconds} "
            "{-maxbadpereports=reports} "
            "{-endpointkeepalivetransmissioninterval=milliseconds} {-endpointkeepalivetimeoutinterval=milliseconds} "
            "{-peerheartbeatcycle=milliseconds} {-peermaxtimelastheard=milliseconds} {-peermaxtimenoresponse=milliseconds} "
            "{-takeoverexpiryinterval=milliseconds} {-mentorhuntinterval=milliseconds} "
            "{-statsfile=file} {-statsinterval=millisecs}"
            "\n",argv[0]);
         exit(1);
      }
   }
   beginLogging();

   if(!strcmp(asapAnnounceAddressParameter, "auto")) {
      asapAnnounceAddressParameter = "239.0.0.1:3863";
      asapSendAnnounces = true;
   }
   if(!strcmp(enrpMulticastAddressParameter, "auto")) {
      enrpMulticastAddressParameter = "239.0.0.1:9901";
      enrpAnnounceViaMulticast = true;
   }
   getSocketPair(asapUnicastAddressParameter, asapUnicastAddress, &asapUnicastSocket,
                 asapSendAnnounces,
                 asapAnnounceAddressParameter, asapAnnounceAddress, &asapAnnounceSocket, useIPv6);
   getSocketPair(enrpUnicastAddressParameter, enrpUnicastAddress, &enrpUnicastSocket,
                 enrpAnnounceViaMulticast,
                 enrpMulticastAddressParameter, enrpMulticastAddress, &enrpMulticastOutputSocket, useIPv6);

   if(enrpAnnounceViaMulticast) {
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
   }


   /* ====== Initialize Registrar ======================================= */
   registrar = registrarNew(serverID,
                            asapUnicastSocket,
                            asapAnnounceSocket,
                            enrpUnicastSocket,
                            enrpMulticastInputSocket,
                            enrpMulticastOutputSocket,
                            asapSendAnnounces, (const union sockaddr_union*)&asapAnnounceAddress->AddressArray[0],
                            enrpAnnounceViaMulticast, (const union sockaddr_union*)&enrpMulticastAddress->AddressArray[0],
                            statsFile, statsInterval
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
      else if(!(strncmp(argv[i], "-distancestep=", 14))) {
         registrar->DistanceStep = atol((char*)&argv[i][14]);
         if(registrar->DistanceStep < 1) {
            registrar->DistanceStep = 1;
         }
      }
      else if(!(strncmp(argv[i], "-maxbadpereports=", 17))) {
         registrar->MaxBadPEReports = atol((char*)&argv[i][17]);
         if(registrar->MaxBadPEReports < 1) {
            registrar->MaxBadPEReports = 1;
         }
      }
      else if(!(strncmp(argv[i], "-maxincrement=", 14))) {
         registrar->MaxIncrement = atol((char*)&argv[i][14]);
      }
      else if(!(strncmp(argv[i], "-maxhresitems=", 14))) {
         registrar->MaxHandleResolutionItems = atol((char*)&argv[i][14]);
         if(registrar->MaxHandleResolutionItems < 1) {
            registrar->MaxHandleResolutionItems = 1;
         }
      }
      else if(!(strncmp(argv[i], "-maxelementsperhtrequest=", 25))) {
         registrar->MaxElementsPerHTRequest = atol((char*)&argv[i][25]);
         if(registrar->MaxElementsPerHTRequest < 1) {
            registrar->MaxElementsPerHTRequest = 1;
         }
      }
      else if(!(strncmp(argv[i], "-autoclosetimeout=", 18))) {
         registrar->AutoCloseTimeout = 1000000 * atol((char*)&argv[i][18]);
         if(registrar->AutoCloseTimeout < 5000000) {
            registrar->AutoCloseTimeout = 5000000;
         }
      }
      else if(!(strncmp(argv[i], "-serverannouncecycle=", 21))) {
         registrar->ServerAnnounceCycle = 1000 * atol((char*)&argv[i][21]);
         if(registrar->ServerAnnounceCycle < 100000) {
            registrar->ServerAnnounceCycle = 100000;
         }
         else if(registrar->ServerAnnounceCycle > 60000000) {
            registrar->ServerAnnounceCycle = 60000000;
         }
      }
      else if(!(strncmp(argv[i], "-endpointkeepalivetransmissioninterval=", 39))) {
         registrar->EndpointKeepAliveTransmissionInterval = 1000 * atol((char*)&argv[i][39]);
         if(registrar->EndpointKeepAliveTransmissionInterval < 300000) {
            registrar->EndpointKeepAliveTransmissionInterval = 300000;
         }
         else if(registrar->EndpointKeepAliveTransmissionInterval > 300000000) {
            registrar->EndpointKeepAliveTransmissionInterval = 300000000;
         }
      }
      else if(!(strncmp(argv[i], "-endpointkeepalivetimeoutinterval=", 34))) {
         registrar->EndpointKeepAliveTimeoutInterval = 1000 * atol((char*)&argv[i][34]);
         if(registrar->EndpointKeepAliveTimeoutInterval < 100000) {
            registrar->EndpointKeepAliveTimeoutInterval = 100000;
         }
         if(registrar->EndpointKeepAliveTimeoutInterval > 60000000) {
            registrar->EndpointKeepAliveTimeoutInterval = 60000000;
         }
      }
      else if(!(strncmp(argv[i], "-peerheartbeatcycle=", 20))) {
         registrar->PeerHeartbeatCycle = 1000 * atol((char*)&argv[i][20]);
         if(registrar->PeerHeartbeatCycle < 1000000) {
            registrar->PeerHeartbeatCycle = 1000000;
         }
         else if(registrar->PeerHeartbeatCycle > 300000000) {
            registrar->PeerHeartbeatCycle = 300000000;
         }
      }
      else if(!(strncmp(argv[i], "-peermaxtimelastheard=", 22))) {
         registrar->PeerMaxTimeLastHeard = 1000 * atol((char*)&argv[i][22]);
         if(registrar->PeerMaxTimeLastHeard < 1000000) {
            registrar->PeerMaxTimeLastHeard = 1000000;
         }
         else if(registrar->PeerMaxTimeLastHeard > 300000000) {
            registrar->PeerMaxTimeLastHeard = 300000000;
         }
      }
      else if(!(strncmp(argv[i], "-peermaxtimenoresponse=", 23))) {
         registrar->PeerMaxTimeNoResponse = 1000 * atol((char*)&argv[i][23]);
         if(registrar->PeerMaxTimeNoResponse < 1000000) {
            registrar->PeerMaxTimeNoResponse = 1000000;
         }
         else if(registrar->PeerMaxTimeNoResponse > 60000000) {
            registrar->PeerMaxTimeNoResponse = 60000000;
         }
      }
      else if(!(strncmp(argv[i], "-mentordiscoverytimeout=", 24))) {
         registrar->MentorDiscoveryTimeout = 1000 * atol((char*)&argv[i][24]);
         if(registrar->MentorDiscoveryTimeout < 1000000) {
            registrar->MentorDiscoveryTimeout = 1000000;
         }
         else if(registrar->MentorDiscoveryTimeout > 60000000) {
            registrar->MentorDiscoveryTimeout = 60000000;
         }
      }
      else if(!(strncmp(argv[i], "-takeoverexpiryinterval=", 24))) {
         registrar->TakeoverExpiryInterval = 1000 * atol((char*)&argv[i][24]);
         if(registrar->TakeoverExpiryInterval < 1000000) {
            registrar->TakeoverExpiryInterval = 1000000;
         }
         else if(registrar->TakeoverExpiryInterval > 60000000) {
            registrar->TakeoverExpiryInterval = 60000000;
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
   transportAddressBlockPrint(asapUnicastAddress, stdout);
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
   printf("ENRP Announce Address:  ");
   if(enrpAnnounceViaMulticast) {
      transportAddressBlockPrint(enrpMulticastAddress, stdout);
      puts("");
   }
   else {
      puts("(none)");
   }
#ifdef ENABLE_CSP
   if(cspReportInterval > 0) {
      printf("CSP Report Address:     ");
      fputaddress((struct sockaddr*)&cspReportAddress, true, stdout);
      puts("");
      printf("CSP Report Interval:    %lldms\n", cspReportInterval / 1000);
   }
#endif
   printf("Auto-Close Timeout:     %Lus\n", registrar->AutoCloseTimeout / 1000000);
   if(statsFile) {
      printf("Statistics Interval:    %ums\n", statsInterval);
   }

   puts("\nASAP Parameters:");
   printf("   Distance Step:                               %ums\n",   (unsigned int)registrar->DistanceStep);
   printf("   Max Bad PE Reports:                          %u\n",     (unsigned int)registrar->MaxBadPEReports);
   printf("   Server Announce Cycle:                       %lldms\n", registrar->ServerAnnounceCycle / 1000);
   printf("   Endpoint Monitoring SCTP Heartbeat Interval: %lldms\n", registrar->EndpointMonitoringHeartbeatInterval / 1000);
   printf("   Endpoint Keep Alive Transmission Interval:   %lldms\n", registrar->EndpointKeepAliveTransmissionInterval / 1000);
   printf("   Endpoint Keep Alive Timeout Interval:        %lldms\n", registrar->EndpointKeepAliveTimeoutInterval / 1000);
   printf("   Max Increment:                               %u\n",     (unsigned int)registrar->MaxIncrement);
   printf("   Max Handle Resolution Items (MaxHResItems):  %u\n",     (unsigned int)registrar->MaxHandleResolutionItems);
   puts("ENRP Parameters:");
   printf("   Peer Heartbeat Cylce:                        %lldms\n", registrar->PeerHeartbeatCycle / 1000);
   printf("   Peer Max Time Last Heard:                    %lldms\n", registrar->PeerMaxTimeLastHeard / 1000);
   printf("   Peer Max Time No Response:                   %lldms\n", registrar->PeerMaxTimeNoResponse / 1000);
   printf("   Max Elements per Handle Table Request:       %u\n",     (unsigned int)registrar->MaxElementsPerHTRequest);
   printf("   Mentor Hunt Timeout:                         %lldms\n", registrar->MentorDiscoveryTimeout / 1000);
   printf("   Takeover Expiry Interval:                    %lldms\n", registrar->TakeoverExpiryInterval / 1000);
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
   if(statsFile) {
      fclose(statsFile);
   }
   puts("\nTerminated!");
   return(0);
}
