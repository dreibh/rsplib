/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2014 by Thomas Dreibholz
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
 * Contact: dreibh@iem.uni-due.de
 */

#ifndef REGISTRAR_H
#define REGISTRAR_H

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
#ifdef ENABLE_REGISTRAR_STATISTICS
#include <bzlib.h>
#endif


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */


#define REGISTRAR_RSERPOOL_MESSAGE_BUFFER_SIZE                          65536
#define REGISTRAR_DEFAULT_MIN_ENDPOINT_ADDRESS_SCOPE     AS_UNICAST_SITELOCAL
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
#define REGISTRAR_DEFAULT_MENTOR_DISCOVERY_TIMEOUT                    5000000
#define REGISTRAR_DEFAULT_TAKEOVER_EXPIRY_INTERVAL                    5000000
#define REGISTRAR_DEFAULT_AUTOCLOSE_TIMEOUT                         300000000
#define REGISTRAR_DEFAULT_ANNOUNCE_TTL                                     30
#define REGISTRAR_DEFAULT_SUPPORT_TAKEOVER_SUGGESTION                   false
#define REGISTRAR_DEFAULT_MAX_HR_RATE                                    -1.0   /* unlimited */
#define REGISTRAR_DEFAULT_MAX_EU_RATE                                    -1.0   /* unlimited */


#ifdef ENABLE_REGISTRAR_STATISTICS
struct RegistrarStatistics
{
   unsigned long long                         StatsLine;
   unsigned long long                         StatsStartTime;
   unsigned long long                         ActionLogLine;
   unsigned long long                         ActionLogLastActivity;
   unsigned long long                         ActionLogStartTime;
   int                                        StatsInterval;

   unsigned long long                         RegistrationCount;
   unsigned long long                         ReregistrationCount;
   unsigned long long                         DeregistrationCount;
   unsigned long long                         HandleResolutionCount;
   unsigned long long                         FailureReportCount;
   unsigned long long                         SynchronizationCount;
   unsigned long long                         HandleUpdateCount;
   unsigned long long                         EndpointKeepAliveCount;

   bool                                       NeedsWeightedStatValues;
   struct WeightedStatValue                   PoolsCount;
   struct WeightedStatValue                   PoolElementsCount;
   struct WeightedStatValue                   OwnedPoolElementsCount;
   struct WeightedStatValue                   PeersCount;
};
#endif


struct Registrar
{
   RegistrarIdentifierType                    ServerID;

   struct Dispatcher                          StateMachine;
   struct ST_CLASS(PoolHandlespaceManagement) Handlespace;
   struct ST_CLASS(PeerListManagement)        Peers;
   struct Timer                               HandlespaceActionTimer;
   struct Timer                               PeerActionTimer;
   struct ST_CLASS(PoolUserList)              PoolUsers;
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
   bool                                       ENRPSupportTakeoverSuggestion;

   bool                                       InStartupPhase;
   RegistrarIdentifierType                    MentorServerID;

   int                                        AnnounceTTL;
   size_t                                     DistanceStep;
   size_t                                     MaxBadPEReports;
   unsigned long long                         AutoCloseTimeout;
   unsigned long long                         ServerAnnounceCycle;
   unsigned long long                         EndpointMonitoringHeartbeatInterval;
   unsigned long long                         EndpointKeepAliveTransmissionInterval;
   unsigned long long                         EndpointKeepAliveTimeoutInterval;
   unsigned int                               MinEndpointAddressScope;
   size_t                                     MaxElementsPerHTRequest;
   size_t                                     MaxIncrement;
   size_t                                     MaxHandleResolutionItems;
   unsigned long long                         PeerHeartbeatCycle;
   unsigned long long                         PeerMaxTimeLastHeard;
   unsigned long long                         PeerMaxTimeNoResponse;
   unsigned long long                         MentorDiscoveryTimeout;
   unsigned long long                         TakeoverExpiryInterval;
   double                                     MaxHRRate;
   double                                     MaxEURate;

#ifdef ENABLE_CSP
   struct CSPReporter                         CSPReporter;
   unsigned int                               CSPReportInterval;
   union sockaddr_union                       CSPReportAddress;
#endif

#ifdef ENABLE_REGISTRAR_STATISTICS
   struct RegistrarStatistics                 Stats;
   FILE*                                      ActionLogFile;
   BZFILE*                                    ActionLogBZFile;
   FILE*                                      StatsFile;
   BZFILE*                                    StatsBZFile;
   struct Timer                               StatsTimer;
#endif
};


/* ###### Initialization ################################################# */
struct Registrar* registrarNew(const RegistrarIdentifierType  serverID,
                               int                            asapUnicastSocket,
                               int                            asapAnnounceSocket,
                               int                            enrpUnicastSocket,
                               int                            enrpMulticastOutputSocket,
                               int                            enrpMulticastInputSocket,
                               const bool                     asapSendAnnounces,
                               const union sockaddr_union*    asapAnnounceAddress,
                               const bool                     enrpAnnounceViaMulticast,
                               const union sockaddr_union*    enrpMulticastAddress
#ifdef ENABLE_REGISTRAR_STATISTICS
                             , FILE*                          actionLogFile,
                               BZFILE*                        actionLogBZFile,
                               FILE*                          statsFile,
                               BZFILE*                        statsBZFile,
                               const unsigned int             statsInterval,
                               const bool                     needsWeightedStatValues
#endif
#ifdef ENABLE_CSP
                             , const unsigned int             cspReportInterval,
                               const union sockaddr_union*    cspReportAddress
#endif
                               );
void registrarDelete(struct Registrar* registrar);
unsigned int registrarAddStaticPeer(
                struct Registrar*                   registrar,
                const RegistrarIdentifierType       identifier,
                const struct TransportAddressBlock* transportAddressBlock);

unsigned long long registrarRandomizeCycle(const unsigned long long interval);
unsigned int registrarRoundDistance(const unsigned int distance,
                                    const unsigned int step);
void registrarUpdateDistance(struct Registrar*                       registrar,
                             const int                               fd,
                             const sctp_assoc_t                      assocID,
                             const struct ST_CLASS(PoolElementNode)* poolElementNode,
                             struct PoolPolicySettings*              updatedPolicySettings,
                             bool                                    addDistance,
                             unsigned int*                           distance);


/* ###### Statistics ##################################################### */
#ifdef ENABLE_REGISTRAR_STATISTICS
void registrarPrintScalarStatistics(struct Registrar* registrar,
                                    FILE*             fh,
                                    const char*       objectName);
void registrarBeginActionLog(struct Registrar* registrar);
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
                             unsigned int              errorCode);
#endif


/* ###### Core ########################################################### */
void registrarDumpHandlespace(struct Registrar* registrar);
void registrarDumpPeers(struct Registrar* registrar);
void registrarBeginNormalOperation(struct Registrar* registrar,
                                   const bool        initializedFromMentor);
void registrarHandleSocketEvent(struct Dispatcher* dispatcher,
                                int                fd,
                                unsigned int       eventMask,
                                void*              userData);
void registrarHandleMessage(struct Registrar*       registrar,
                            struct RSerPoolMessage* message,
                            int                     sd);


/* ###### ASAP ########################################################### */
void registrarHandleASAPAnnounceTimer(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData);
void registrarHandlePoolElementEvent(struct Dispatcher* dispatcher,
                                     struct Timer*      timer,
                                     void*              userData);

/* ====== Handlespace Operations ======================= */
void registrarRemovePoolElementsOfConnection(struct Registrar*  registrar,
                                             const int          fd,
                                             const sctp_assoc_t assocID);
void registrarHandleASAPRegistration(struct Registrar*       registrar,
                                     const int               fd,
                                     const sctp_assoc_t      assocID,
                                     struct RSerPoolMessage* message);
void registrarHandleASAPDeregistration(struct Registrar*       registrar,
                                       const int               fd,
                                       const sctp_assoc_t      assocID,
                                       struct RSerPoolMessage* message);
void registrarHandleASAPHandleResolution(struct Registrar*       registrar,
                                         const int               fd,
                                         const sctp_assoc_t      assocID,
                                         struct RSerPoolMessage* message);

/* ====== Monitoring =================================== */
void registrarHandleASAPEndpointKeepAliveAck(struct Registrar*       registrar,
                                             const int               fd,
                                             const sctp_assoc_t      assocID,
                                             struct RSerPoolMessage* message);
void registrarSendASAPEndpointKeepAlive(struct Registrar*                 registrar,
                                        struct ST_CLASS(PoolElementNode)* poolElementNode,
                                        const bool                        newHomeRegistrar);
void registrarHandleASAPEndpointUnreachable(struct Registrar*       registrar,
                                            const int               fd,
                                            const sctp_assoc_t      assocID,
                                            struct RSerPoolMessage* message);


/* ###### ENRP ########################################################### */
void registrarHandleENRPAnnounceTimer(struct Dispatcher* dispatcher,
                                      struct Timer*      timer,
                                      void*              userData);
void registrarHandlePeerEvent(struct Dispatcher* dispatcher,
                              struct Timer*      timer,
                              void*              userData);

/* ====== List/Handlespace I/O ========================= */
void registrarHandleENRPHandleUpdate(struct Registrar*       registrar,
                                     const int               fd,
                                     const sctp_assoc_t      assocID,
                                     struct RSerPoolMessage* message);
void registrarSendENRPHandleUpdate(struct Registrar*                 registrar,
                                   struct ST_CLASS(PoolElementNode)* poolElementNode,
                                   const uint16_t                    action);
void registrarHandleENRPListRequest(struct Registrar*       registrar,
                                    const int               fd,
                                    const sctp_assoc_t      assocID,
                                    struct RSerPoolMessage* message);
void registrarSendENRPListRequest(struct Registrar*           registrar,
                                  const int                   fd,
                                  const sctp_assoc_t          assocID,
                                  int                         msgSendFlags,
                                  const union sockaddr_union* destinationAddressList,
                                  const size_t                destinationAddresses,
                                  RegistrarIdentifierType     receiverID);
void registrarHandleENRPListResponse(struct Registrar*       registrar,
                                     const int               fd,
                                     const sctp_assoc_t      assocID,
                                     struct RSerPoolMessage* message);
void registrarHandleENRPHandleTableRequest(struct Registrar*       registrar,
                                           const int               fd,
                                           const sctp_assoc_t      assocID,
                                           struct RSerPoolMessage* message);
void registrarSendENRPHandleTableRequest(struct Registrar*           registrar,
                                         const int                   fd,
                                         const sctp_assoc_t          assocID,
                                         int                         msgSendFlags,
                                         const union sockaddr_union* destinationAddressList,
                                         const size_t                destinationAddresses,
                                         RegistrarIdentifierType     receiverID,
                                         unsigned int                flags);
void registrarHandleENRPHandleTableResponse(struct Registrar*       registrar,
                                            const int               fd,
                                            const sctp_assoc_t      assocID,
                                            struct RSerPoolMessage* message);

/* ====== Handlespace Audit ============================ */
void registrarHandleENRPPresence(struct Registrar*       registrar,
                                 const int               fd,
                                 sctp_assoc_t            assocID,
                                 struct RSerPoolMessage* message);
void registrarSendENRPPresence(struct Registrar*             registrar,
                               const int                     fd,
                               const sctp_assoc_t            assocID,
                               int                           msgSendFlags,
                               const union sockaddr_union*   destinationAddressList,
                               const size_t                  destinationAddresses,
                               const RegistrarIdentifierType receiverID,
                               const bool                    replyRequired);
void registrarSendENRPPresenceToAllPeers(struct Registrar* registrar);

/* ====== Takeover ===================================== */
void registrarHandleENRPInitTakeover(struct Registrar*       registrar,
                                     const int               fd,
                                     sctp_assoc_t            assocID,
                                     struct RSerPoolMessage* message);
void registrarSendENRPInitTakeoverToAllPeers(struct Registrar*             registrar,
                                             const RegistrarIdentifierType targetID);
void registrarHandleENRPInitTakeoverAck(struct Registrar*       registrar,
                                        const int               fd,
                                        sctp_assoc_t            assocID,
                                        struct RSerPoolMessage* message);
void registrarSendENRPInitTakeoverAck(struct Registrar*             registrar,
                                      const int                     fd,
                                      const sctp_assoc_t            assocID,
                                      const RegistrarIdentifierType receiverID,
                                      const RegistrarIdentifierType targetID);

void registrarHandleENRPTakeoverServer(struct Registrar*       registrar,
                                       const int               fd,
                                       sctp_assoc_t            assocID,
                                       struct RSerPoolMessage* message);
void registrarSendENRPTakeoverServerToAllPeers(struct Registrar*             registrar,
                                               const RegistrarIdentifierType targetID);

/* ###### Security ####################################################### */
bool registrarPoolUserHasPermissionFor(struct Registrar*               registrar,
                                       const int                       fd,
                                       const sctp_assoc_t              assocID,
                                       const unsigned int              action,
                                       const struct PoolHandle*        poolHandle,
                                       const PoolElementIdentifierType peIdentifier);

/* ###### Miscellaneous ################################################## */
void registrarRegistrationHook(struct Registrar*                 registrar,
                               struct ST_CLASS(PoolElementNode)* poolElementNode);
void registrarDeregistrationHook(struct Registrar*                 registrar,
                                 struct ST_CLASS(PoolElementNode)* poolElementNode);

#endif
