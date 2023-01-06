/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2023 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: thomas.dreibholz@gmail.com
 */

#include "rspregistrar.h"


static void poolElementNodeDisposer(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                    void*                             userData);
static void peerListNodeDisposer(struct ST_CLASS(PeerListNode)* peerListNode,
                                 void*                          userData);
#ifdef ENABLE_REGISTRAR_STATISTICS
static void statisticsCallback(struct Dispatcher* dispatcher,
                               struct Timer*      timer,
                               void*              userData);
#endif
#ifdef ENABLE_CSP
static size_t registrarGetReportFunction(
                 void*                         userData,
                 uint64_t*                     identifier,
                 struct ComponentAssociation** caeArray,
                 char*                         statusText,
                 char*                         componentLocation,
                 double*                       workload);
#endif


/* ###### Constructor #################################################### */
struct Registrar* registrarNew(const RegistrarIdentifierType serverID,
                               int                           asapUnicastSocket,
                               int                           asapAnnounceSocket,
                               int                           enrpUnicastSocket,
                               int                           enrpMulticastOutputSocket,
                               int                           enrpMulticastInputSocket,
                               const bool                    asapSendAnnounces,
                               const union sockaddr_union*   asapAnnounceAddress,
                               const bool                    enrpAnnounceViaMulticast,
                               const union sockaddr_union*   enrpMulticastAddress
#ifdef ENABLE_REGISTRAR_STATISTICS
                             , FILE*                         actionLogFile,
                               BZFILE*                       actionLogBZFile,
                               FILE*                         statsFile,
                               BZFILE*                       statsBZFile,
                               const unsigned int            statsInterval,
                               const bool                    needsWeightedStatValues
#endif
#ifdef ENABLE_CSP
                             , const unsigned int            cspReportInterval,
                               const union sockaddr_union*   cspReportAddress
#endif
                               )
{
   struct Registrar*     registrar;
   int                   autoCloseTimeout;
   int                   noDelayOn;
#ifdef HAVE_SCTP_DELAYED_SACK
   struct sctp_sack_info sctpSACKInfo;
#endif

   registrar = (struct Registrar*)malloc(sizeof(struct Registrar));
   if(registrar != NULL) {
      registrar->UDPMessageBuffer = messageBufferNew(REGISTRAR_RSERPOOL_MESSAGE_BUFFER_SIZE, false);
      if(registrar->UDPMessageBuffer == NULL) {
         free(registrar);
         return(NULL);
      }
      registrar->ASAPMessageBuffer = messageBufferNew(REGISTRAR_RSERPOOL_MESSAGE_BUFFER_SIZE, true);
      if(registrar->ASAPMessageBuffer == NULL) {
         messageBufferDelete(registrar->UDPMessageBuffer);
         free(registrar);
         return(NULL);
      }
      registrar->ENRPUnicastMessageBuffer = messageBufferNew(REGISTRAR_RSERPOOL_MESSAGE_BUFFER_SIZE, true);
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
      ST_CLASS(poolUserListNew)(&registrar->PoolUsers);
      ST_CLASS(peerListManagementNew)(&registrar->Peers,
                                      &registrar->Handlespace,
                                      registrar->ServerID,
                                      peerListNodeDisposer,
                                      registrar);
      timerNew(&registrar->ASAPAnnounceTimer,
               &registrar->StateMachine,
               registrarHandleASAPAnnounceTimer,
               (void*)registrar);
      timerNew(&registrar->ENRPAnnounceTimer,
               &registrar->StateMachine,
               registrarHandleENRPAnnounceTimer,
               (void*)registrar);
      timerNew(&registrar->HandlespaceActionTimer,
               &registrar->StateMachine,
               registrarHandlePoolElementEvent,
               (void*)registrar);
      timerNew(&registrar->PeerActionTimer,
               &registrar->StateMachine,
               registrarHandlePeerEvent,
               (void*)registrar);

      registrar->InStartupPhase                = true;
      registrar->MentorServerID                = 0;

      registrar->ASAPSocket                    = asapUnicastSocket;
      registrar->ASAPAnnounceSocket            = asapAnnounceSocket;
      registrar->ASAPSendAnnounces             = asapSendAnnounces;

      registrar->ENRPUnicastSocket             = enrpUnicastSocket;
      registrar->ENRPMulticastInputSocket      = enrpMulticastOutputSocket;
      registrar->ENRPMulticastOutputSocket     = enrpMulticastInputSocket;
      registrar->ENRPAnnounceViaMulticast      = enrpAnnounceViaMulticast;
      registrar->ENRPSupportTakeoverSuggestion = REGISTRAR_DEFAULT_SUPPORT_TAKEOVER_SUGGESTION;

      registrar->DistanceStep                          = REGISTRAR_DEFAULT_DISTANCE_STEP;
      registrar->MaxBadPEReports                       = REGISTRAR_DEFAULT_MAX_BAD_PE_REPORTS;
      registrar->ServerAnnounceCycle                   = REGISTRAR_DEFAULT_SERVER_ANNOUNCE_CYCLE;
      registrar->EndpointMonitoringHeartbeatInterval   = REGISTRAR_DEFAULT_ENDPOINT_MONITORING_HEARTBEAT_INTERVAL;
      registrar->EndpointKeepAliveTransmissionInterval = REGISTRAR_DEFAULT_ENDPOINT_KEEP_ALIVE_TRANSMISSION_INTERVAL;
      registrar->EndpointKeepAliveTimeoutInterval      = REGISTRAR_DEFAULT_ENDPOINT_KEEP_ALIVE_TIMEOUT_INTERVAL;
      registrar->MinEndpointAddressScope               = REGISTRAR_DEFAULT_MIN_ENDPOINT_ADDRESS_SCOPE;
      registrar->AutoCloseTimeout                      = REGISTRAR_DEFAULT_AUTOCLOSE_TIMEOUT;
      registrar->MaxIncrement                          = REGISTRAR_DEFAULT_MAX_INCREMENT;
      registrar->MaxHandleResolutionItems              = REGISTRAR_DEFAULT_MAX_HANDLE_RESOLUTION_ITEMS;
      registrar->MaxElementsPerHTRequest               = REGISTRAR_DEFAULT_MAX_ELEMENTS_PER_HANDLE_TABLE_REQUEST;
      registrar->PeerMaxTimeLastHeard                  = REGISTRAR_DEFAULT_PEER_MAX_TIME_LAST_HEARD;
      registrar->PeerMaxTimeNoResponse                 = REGISTRAR_DEFAULT_PEER_MAX_TIME_NO_RESPONSE;
      registrar->PeerHeartbeatCycle                    = REGISTRAR_DEFAULT_PEER_HEARTBEAT_CYCLE;
      registrar->MentorDiscoveryTimeout                = REGISTRAR_DEFAULT_MENTOR_DISCOVERY_TIMEOUT;
      registrar->TakeoverExpiryInterval                = REGISTRAR_DEFAULT_TAKEOVER_EXPIRY_INTERVAL;
      registrar->AnnounceTTL                           = REGISTRAR_DEFAULT_ANNOUNCE_TTL;

      registrar->MaxHRRate                             = REGISTRAR_DEFAULT_MAX_HR_RATE;
      registrar->MaxEURate                             = REGISTRAR_DEFAULT_MAX_EU_RATE;

#ifdef ENABLE_REGISTRAR_STATISTICS
      registrar->ActionLogFile                         = actionLogFile;
      registrar->ActionLogBZFile                       = actionLogBZFile;
      registrar->StatsFile                             = statsFile;
      registrar->StatsBZFile                           = statsBZFile;
      registrar->Stats.StatsInterval                   = statsInterval;
      registrar->Stats.ActionLogLine                   = 0;
      registrar->Stats.ActionLogLastActivity           = 0;
      registrar->Stats.ActionLogStartTime              = getMicroTime();
      registrar->Stats.StatsStartTime                  = 0;
      registrar->Stats.StatsLine                       = 0;
      registrar->Stats.RegistrationCount               = 0;
      registrar->Stats.ReregistrationCount             = 0;
      registrar->Stats.DeregistrationCount             = 0;
      registrar->Stats.HandleResolutionCount           = 0;
      registrar->Stats.FailureReportCount              = 0;
      registrar->Stats.SynchronizationCount            = 0;
      registrar->Stats.HandleUpdateCount               = 0;
      registrar->Stats.EndpointKeepAliveCount          = 0;
      registrar->Stats.NeedsWeightedStatValues         = needsWeightedStatValues;
      initWeightedStatValue(&registrar->Stats.PoolsCount, registrar->Stats.ActionLogStartTime);
      initWeightedStatValue(&registrar->Stats.PoolElementsCount, registrar->Stats.ActionLogStartTime);
      initWeightedStatValue(&registrar->Stats.OwnedPoolElementsCount, registrar->Stats.ActionLogStartTime);
      initWeightedStatValue(&registrar->Stats.PeersCount, registrar->Stats.ActionLogStartTime);
#endif

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
      noDelayOn = 1;
      if(ext_setsockopt(registrar->ASAPSocket, IPPROTO_SCTP, SCTP_NODELAY, &noDelayOn, sizeof(noDelayOn)) < 0) {
         LOG_ERROR
         logerror("setsockopt() for SCTP_NODELAY failed");
         LOG_END
      }
      if(ext_setsockopt(registrar->ENRPUnicastSocket, IPPROTO_SCTP, SCTP_NODELAY, &noDelayOn, sizeof(noDelayOn)) < 0) {
         LOG_ERROR
         logerror("setsockopt() for SCTP_NODELAY failed");
         LOG_END
      }
#ifdef HAVE_SCTP_DELAYED_SACK
      /* ====== Tune SACK handling ======================================= */
      /* Without this tuning, the PE would wait 200ms to acknowledge a
         Endpoint Unreachable. The usually immediately following
         Handle Resolution transmission would also be delayed ... */
      sctpSACKInfo.sack_assoc_id = 0;
      sctpSACKInfo.sack_delay    = 0;
      sctpSACKInfo.sack_freq     = 1;
      if(ext_setsockopt(registrar->ASAPSocket,
                        IPPROTO_SCTP, SCTP_DELAYED_SACK,
                        &sctpSACKInfo, sizeof(sctpSACKInfo)) < 0) {
         LOG_WARNING
         logerror("Unable to set SCTP_DELAYED_SACK");
         LOG_END
      }
#else
#warning SCTP_DELAYED_SACK/sctp_sack_info is not supported - Unable to tune SACK handling!
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
                    registrarHandleSocketEvent,
                    (void*)registrar);
      fdCallbackNew(&registrar->ENRPUnicastSocketFDCallback,
                    &registrar->StateMachine,
                    registrar->ENRPUnicastSocket,
                    FDCE_Read|FDCE_Exception,
                    registrarHandleSocketEvent,
                    (void*)registrar);

      memcpy(&registrar->ENRPMulticastAddress, enrpMulticastAddress, sizeof(registrar->ENRPMulticastAddress));
      if(registrar->ENRPMulticastInputSocket >= 0) {
         setNonBlocking(registrar->ENRPMulticastInputSocket);
         fdCallbackNew(&registrar->ENRPMulticastInputSocketFDCallback,
                       &registrar->StateMachine,
                       registrar->ENRPMulticastInputSocket,
                       FDCE_Read|FDCE_Exception,
                       registrarHandleSocketEvent,
                       (void*)registrar);
      }
      registrar->ENRPMulticastOutputSocketFamily = getFamily(&registrar->ENRPMulticastAddress.sa);

      timerStart(&registrar->ENRPAnnounceTimer, getMicroTime() + registrar->MentorDiscoveryTimeout);
      if(registrar->ENRPAnnounceViaMulticast) {
         if(multicastGroupControl(registrar->ENRPMulticastInputSocket,
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

#ifdef ENABLE_REGISTRAR_STATISTICS
      if(registrar->Stats.StatsInterval > 0) {
         timerNew(&registrar->StatsTimer,
                  &registrar->StateMachine,
                  statisticsCallback,
                  (void*)registrar);
         timerStart(&registrar->StatsTimer, 0);
      }
      registrarBeginActionLog(registrar);
#endif
   }

   return(registrar);
}


/* ###### Destructor ###################################################### */
void registrarDelete(struct Registrar* registrar)
{
   if(registrar) {
#ifdef ENABLE_REGISTRAR_STATISTICS
      if(registrar->StatsFile) {
         timerDelete(&registrar->StatsTimer);
      }
#endif
      fdCallbackDelete(&registrar->ENRPUnicastSocketFDCallback);
      fdCallbackDelete(&registrar->ASAPSocketFDCallback);
      ST_CLASS(peerListManagementDelete)(&registrar->Peers);
      ST_CLASS(poolUserListDelete)(&registrar->PoolUsers);
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
      if(registrar->ASAPSocket >= 0) {
         ext_close(registrar->ASAPSocket);
         registrar->ASAPSocket = -1;
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


/* ###### Disposer function for PoolElementNodes ######################### */
static void poolElementNodeDisposer(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                    void*                             userData)
{
}


/* ###### Disposer function for PeerListNodes ############################ */
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


#ifdef ENABLE_REGISTRAR_STATISTICS
#if defined(__LINUX__)
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
   unsigned long long       userTime    = 0;
   unsigned long long       systemTime  = 0;
   int                      bzerror;
   const char*              header;
   char                     str[1024];
#if defined(__LINUX__)
   unsigned long long       startupTime = 0;
   unsigned long long       uptime;
#endif
   size_t                   peers;
   size_t                   pools;
   size_t                   poolElements;
   size_t                   ownedPoolElements;

   if(registrar->Stats.StatsLine == 0) {
      header = "AbsTime RelTime Runtime UserTime SystemTime   Peers Pools PoolElements OwnedPoolElements   Registrations Reregistrations Deregistrations HandleResolutions FailureReports Synchronizations HandleUpdates EndpointKeepAlives\n";
      if(registrar->StatsBZFile) {
         BZ2_bzWrite(&bzerror, registrar->StatsBZFile, (char*)header, strlen(header));
      }
      else {
         fputs(header, registrar->StatsFile);
      }
      registrar->Stats.StatsLine      = 1;
      registrar->Stats.StatsStartTime = now;
   }

#if defined(__LINUX__)
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

   peers             = ST_CLASS(peerListManagementGetPeers)(&registrar->Peers);
   pools             = ST_CLASS(poolHandlespaceManagementGetPools)(&registrar->Handlespace);
   poolElements      = ST_CLASS(poolHandlespaceManagementGetPoolElements)(&registrar->Handlespace);
   ownedPoolElements = ST_CLASS(poolHandlespaceManagementGetOwnedPoolElements)(&registrar->Handlespace);

   snprintf((char*)&str, sizeof(str),
            "%06llu %1.6f %1.6f   %1.6f %1.6f %1.6f   %llu %llu %llu %llu   %llu %llu %llu %llu %llu %llu %llu %llu\n",
            registrar->Stats.StatsLine++,
            now / 1000000.0,
            (now - registrar->Stats.StatsStartTime) / 1000000.0,

            runtime / 1000000.0,
            userTime / 1000000.0,
            systemTime / 1000000.0,

            (unsigned long long)peers,
            (unsigned long long)pools,
            (unsigned long long)poolElements,
            (unsigned long long)ownedPoolElements,

            registrar->Stats.RegistrationCount,
            registrar->Stats.ReregistrationCount,
            registrar->Stats.DeregistrationCount,
            registrar->Stats.HandleResolutionCount,
            registrar->Stats.FailureReportCount,
            registrar->Stats.SynchronizationCount,
            registrar->Stats.HandleUpdateCount,
            registrar->Stats.EndpointKeepAliveCount);
   if(registrar->StatsBZFile) {
      BZ2_bzWrite(&bzerror, registrar->StatsBZFile, str, strlen(str));
   }
   else {
      fputs(str, registrar->StatsFile);
      fflush(registrar->StatsFile);
   }

   timerStart(timer, getMicroTime() + (1000ULL * registrar->Stats.StatsInterval));
}


/* ###### Print scalar statistics ######################################## */
void registrarPrintScalarStatistics(struct Registrar* registrar,
                                    FILE*             fh,
                                    const char*       objectName)
{
   const unsigned long long now = getMicroTime();

   fputs("run 1 \"scenario\"\n", fh);
   fprintf(fh, "scalar \"%s\" \"Registrar Total Registrations\"        %8llu\n", objectName, registrar->Stats.RegistrationCount);
   fprintf(fh, "scalar \"%s\" \"Registrar Total Reregistrations\"      %8llu\n", objectName, registrar->Stats.ReregistrationCount);
   fprintf(fh, "scalar \"%s\" \"Registrar Total Deregistrations\"      %8llu\n", objectName, registrar->Stats.DeregistrationCount);
   fprintf(fh, "scalar \"%s\" \"Registrar Total Handle Resolutions\"   %8llu\n", objectName, registrar->Stats.HandleResolutionCount);
   fprintf(fh, "scalar \"%s\" \"Registrar Total Failure Reports\"      %8llu\n", objectName, registrar->Stats.FailureReportCount);
   fprintf(fh, "scalar \"%s\" \"Registrar Total Synchronizations\"     %8llu\n", objectName, registrar->Stats.SynchronizationCount);
   fprintf(fh, "scalar \"%s\" \"Registrar Total Handle Updates\"       %8llu\n", objectName, registrar->Stats.HandleUpdateCount);
   fprintf(fh, "scalar \"%s\" \"Registrar Total Endpoint Keep Alives\" %8llu\n", objectName, registrar->Stats.EndpointKeepAliveCount);

   fprintf(fh, "scalar \"%s\" \"Registrar Average Number Of Pools\"               %1.6f\n", objectName, averageWeightedStatValue(&registrar->Stats.PoolsCount, now));
   fprintf(fh, "scalar \"%s\" \"Registrar Average Number Of Pool Elements\"       %1.6f\n", objectName, averageWeightedStatValue(&registrar->Stats.PoolElementsCount, now));
   fprintf(fh, "scalar \"%s\" \"Registrar Average Number Of Owned Pool Elements\" %1.6f\n", objectName, averageWeightedStatValue(&registrar->Stats.OwnedPoolElementsCount, now));
   fprintf(fh, "scalar \"%s\" \"Registrar Average Number Of Peers\"               %1.6f\n", objectName, averageWeightedStatValue(&registrar->Stats.PeersCount, now));
}


/* ###### Write action log header line ################################### */
void registrarBeginActionLog(struct Registrar* registrar)
{
   const char* header;
   int         bzerror;

   if(registrar->ActionLogFile) {
      header = "AbsTime RelTime   Direction Protocol Action Reason   Flags Counter Time   PoolHandle PoolElementID   SenderID ReceiverID TargetID   ErrorCode\n";
      if(registrar->ActionLogBZFile) {
         BZ2_bzWrite(&bzerror, registrar->ActionLogBZFile, (char*)header, strlen(header));
      }
      else {
         fputs(header, registrar->ActionLogFile);
         fflush(registrar->ActionLogFile);
      }
   }
}


/* ###### Write action log entry ######################################### */
void registrarWriteActionLog(struct Registrar*         registrar,
                             const char*               direction,
                             const char*               protocol,
                             const char*               action,
                             const char*               reason,
                             const uint32_t            flags,
                             const unsigned long long  counter,
                             const unsigned long long  timeValue,
                             const struct PoolHandle*  poolHandle,
                             PoolElementIdentifierType poolElementID,
                             RegistrarIdentifierType   senderID,
                             RegistrarIdentifierType   receiverID,
                             RegistrarIdentifierType   targetID,
                             unsigned int              errorCode)
{
   char str1[1024];
   char str2[1024];
   char str3[256];
   int  bzerror;

   if(registrar->ActionLogFile) {
      registrar->Stats.ActionLogLastActivity = getMicroTime();
      registrar->Stats.ActionLogLine++;
      snprintf((char*)&str1, sizeof(str1),
               "%06llu   %1.6f %1.6f   \"%s\" \"%s\" \"%s\" \"%s\"   0x%x %llu %1.6f   \"",
               registrar->Stats.ActionLogLine,
               registrar->Stats.ActionLogLastActivity / 1000000.0,
               (registrar->Stats.ActionLogLastActivity - registrar->Stats.ActionLogStartTime) / 1000000.0,
               direction, protocol, action, reason, flags, counter, timeValue / 1000000.0);
      if(poolHandle) {
         poolHandleGetDescription(poolHandle, (char*)&str2, sizeof(str2));
      }
      else {
         str2[0] = 0x00;
      }
      snprintf((char*)&str3, sizeof(str3),
               "\" 0x%x   0x%x 0x%x 0x%x   %x\n",
               poolElementID, senderID, receiverID, targetID, errorCode);

      if(registrar->ActionLogBZFile) {
         BZ2_bzWrite(&bzerror, registrar->ActionLogBZFile, str1, strlen(str1));
         BZ2_bzWrite(&bzerror, registrar->ActionLogBZFile, str2, strlen(str2));
         BZ2_bzWrite(&bzerror, registrar->ActionLogBZFile, str3, strlen(str3));
      }
      else {
         fputs(str1, registrar->ActionLogFile);
         fputs(str2, registrar->ActionLogFile);
         fputs(str3, registrar->ActionLogFile);
         fflush(registrar->ActionLogFile);
      }
   }
}
#endif


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


/* ###### Randomize heartbeat cycle ###################################### */
unsigned long long registrarRandomizeCycle(const unsigned long long interval)
{
   const double originalInterval = (double)interval;
   const double variation    = 0.250 * originalInterval;
   const double nextInterval = originalInterval - (variation / 2.0) +
                                  variation * ((double)rand() / (double)RAND_MAX);
   return((unsigned long long)nextInterval);
}


/* ###### Round distance to nearest step ################################# */
unsigned int registrarRoundDistance(const unsigned int distance,
                                    const unsigned int step)
{
   const unsigned int d = (unsigned int)rint((double)distance / step) * step;
   return(d);
}


/* ###### Update distance for distance-sensitive policies ################ */
void registrarUpdateDistance(struct Registrar*                       registrar,
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
            *distance = registrarRoundDistance(assocStatus.sstat_primary.spinfo_srtt / 2,
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
