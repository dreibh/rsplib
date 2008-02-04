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
 * Copyright (C) 2002-2008 by Thomas Dreibholz
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

#include "registrar.h"


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
         if(errno == ESOCKTNOSUPPORT) {
            break;
         }
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
   bool                          quiet                         = false;

   char                          asapUnicastAddressBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* asapUnicastAddress            = (struct TransportAddressBlock*)&asapUnicastAddressBuffer;
   const char*                   asapUnicastAddressParameter   = "auto";
   int                           asapUnicastSocket             = -1;

   char                          asapAnnounceAddressBuffer[transportAddressBlockGetSize(1)];
   struct TransportAddressBlock* asapAnnounceAddress           = (struct TransportAddressBlock*)&asapAnnounceAddressBuffer;
   const char*                   asapAnnounceAddressParameter  = "auto";
   int                           asapAnnounceSocket            = -1;
   bool                          asapSendAnnounces             = false;

   char                          enrpUnicastAddressBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* enrpUnicastAddress            = (struct TransportAddressBlock*)&enrpUnicastAddressBuffer;
   const char*                   enrpUnicastAddressParameter   = "auto";
   int                           enrpUnicastSocket             = -1;

   char                          enrpMulticastAddressBuffer[transportAddressBlockGetSize(1)];
   struct TransportAddressBlock* enrpMulticastAddress          = (struct TransportAddressBlock*)&enrpMulticastAddressBuffer;
   const char*                   enrpMulticastAddressParameter = "auto";
   int                           enrpMulticastOutputSocket     = -1;
   int                           enrpMulticastInputSocket      = -1;
   bool                          enrpAnnounceViaMulticast      = false;
   bool                          useIPv6                       = checkIPv6();
   const char*                   daemonPIDFile                 = NULL;
   FILE*                         fh;
   pid_t                         childProcess;

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
   const char* cspServer   = getenv("CSP_SERVER");
   const char* cspInterval = getenv("CSP_INTERVAL");
   if(cspInterval) {
      cspReportInterval = 1000ULL * atol(cspInterval);
      if(cspReportInterval < 250000) {
         cspReportInterval = 250000;
      }
   }
   string2address( (cspServer == NULL) ? "127.0.0.1:2960" : cspServer, &cspReportAddress);
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
               (!(strncmp(argv[i], "-minaddressscope=", 17))) ||
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
      else if(!(strncmp(argv[i], "-enrpannounce=", 14))) {
         if( (!(strcasecmp((const char*)&argv[i][14], "off"))) ||
             (!(strcasecmp((const char*)&argv[i][14], "none"))) ) {
            enrpMulticastAddressParameter = "0.0.0.0:0";
            enrpAnnounceViaMulticast      = false;
         }
         else {
            enrpMulticastAddressParameter = (const char*)&argv[i][14];
            enrpAnnounceViaMulticast      = true;
         }
      }
      else if(!(strncmp(argv[i], "-enrpmulticast=", 15))) {
         fputs("WARNING: -enrpmulticast is deprecated, use -enrpannounce!\n", stderr);
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
      else if(!(strcmp(argv[i], "-quiet"))) {
         quiet = true;
      }
      else if(!(strcmp(argv[i], "-disable-ipv6"))) {
         useIPv6 = false;
      }
      else if(!(strncmp(argv[i], "-daemonpidfile=", 15))) {
         daemonPIDFile = (const char*)&argv[i][15];
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
         fprintf(stderr, "Usage: %s {-asap=auto|address:port{,address}...} {[-asapannounce=auto|address:port}]} {-enrp=auto|address:port{,address}...} {[-enrpannounce=auto|address:port}]} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} "
#ifdef ENABLE_CSP
            "{-cspserver=address} {-cspinterval=microseconds} "
#endif
            "{-identifier=registrar identifier} "
            "{-disable-ipv6} {-quiet} "
            "{-autoclosetimeout=seconds} {-serverannouncecycle=milliseconds} "
            "{-maxbadpereports=reports} "
            "{-endpointkeepalivetransmissioninterval=milliseconds} {-endpointkeepalivetimeoutinterval=milliseconds} "
            "{-minaddressscope=loopback|sitelocal|global} "
            "{-peerheartbeatcycle=milliseconds} {-peermaxtimelastheard=milliseconds} {-peermaxtimenoresponse=milliseconds} "
            "{-takeoverexpiryinterval=milliseconds} {-mentorhuntinterval=milliseconds} "
            "{-statsfile=file} {-statsinterval=millisecs} "
            "{-daemonpidfile=file}"
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
      else if(!(strncmp(argv[i], "-minaddressscope=", 17))) {
         if(!(strcmp((const char*)&argv[i][17], "loopback"))) {
            registrar->MinEndpointAddressScope = AS_LOOPBACK;
         }
         else if(!(strcmp((const char*)&argv[i][17], "sitelocal"))) {
            registrar->MinEndpointAddressScope = AS_UNICAST_SITELOCAL;
         }
         else if(!(strcmp((const char*)&argv[i][17], "global"))) {
            registrar->MinEndpointAddressScope = AS_UNICAST_GLOBAL;
         }
         else {
            fputs("ERROR: Bad argument for -minaddressscope!\n", stderr);
            exit(1);
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
   if(!quiet) {
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
      printf("Auto-Close Timeout:     %llus\n", registrar->AutoCloseTimeout / 1000000);
      printf("Min Address Scope:      ");
      if(registrar->MinEndpointAddressScope <= AS_LOOPBACK) {
         puts("loopback");
      }
      else if(registrar->MinEndpointAddressScope <= AS_UNICAST_SITELOCAL) {
         puts("site-local");
      }
      else {
         puts("global");
      }
      if(statsFile) {
         printf("Statistics Interval:    %ums\n", statsInterval);
      }
      printf("Daemon Mode:            %s\n", (daemonPIDFile == NULL) ? "off" : daemonPIDFile);

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
   }


   /* ====== We are ready! =============================================== */
   LOG_NOTE
   fputs("Registrar started. Going into initialization phase...\n", stdlog);
   LOG_END


   if(daemonPIDFile != NULL) {
      childProcess = fork();
      if(childProcess != 0) {
         fh = fopen(daemonPIDFile, "w");
         if(fh) {
            fprintf(fh, "%d\n", childProcess);
            fclose(fh);
         }
         else {
            fprintf(stderr, "ERROR: Unable to create PID file \"%s\"!\n", daemonPIDFile);
         }
         exit(0);
      }
   }

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
   if(!quiet) {
      puts("\nTerminated!");
   }
   return(0);
}
