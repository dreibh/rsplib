/*
 *  $Id: servertable.c,v 1.3 2004/07/19 09:06:54 dreibh Exp $
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
 * Purpose: Server Table
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "servertable.h"
#include "netutilities.h"
#include "asapcreator.h"
#include "asapparser.h"
#include "asapinstance.h"
#include "rsplib-tags.h"

#include <ext_socket.h>
#include <sys/time.h>



/* Maximum number of simultaneous nameserver connection trials */
#define MAX_SIMULTANEOUS_REQUESTS 3



/* ###### Add nameserver announcement source ################################ */
static bool joinAnnounceMulticastGroup(int                         sd,
                                       const union sockaddr_union* groupAddress,
                                       const bool                  join)
{
   union sockaddr_union localAddress;
   int                  on;

   memset((char*)&localAddress, 0, sizeof(localAddress));
   localAddress.sa.sa_family = groupAddress->sa.sa_family;
   if(groupAddress->in6.sin6_family == AF_INET6) {
      localAddress.in6.sin6_port = groupAddress->in6.sin6_port;
   }
   else {
      CHECK(groupAddress->in.sin_family == AF_INET);
      localAddress.in.sin_port = groupAddress->in.sin_port;
   }

   on = 1;
   if(ext_setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
      return(false);
   }
#if !defined (LINUX)
   if(ext_setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) != 0) {
      return(false);
   }
#endif

   if(ext_bind(sd, (struct sockaddr*)&localAddress,
               getSocklen((struct sockaddr*)&localAddress)) != 0) {
      LOG_ERROR
      fputs("Unable to bind multicast socket to address ", stdlog);
      fputaddress((struct sockaddr*)&localAddress, true, stdlog);
      fputs("\n", stdlog);
      LOG_END
      return(false);
   }
   if(multicastGroupMgt(sd, (struct sockaddr*)&groupAddress, NULL, join) == false) {
      return(false);
   }
   return(true);
}


/* ###### Server announce callback  ######################################### */
static void handleServerAnnounceCallback(struct Dispatcher* dispatcher,
                                         int                sd,
                                         int                eventMask,
                                         void*              userData)
{
   struct ServerTable*            serverTable = (struct ServerTable*)userData;
   struct ASAPMessage*            message;
   struct ST_CLASS(PeerListNode)* peerListNode;
   struct sockaddr_storage        senderAddress;
   socklen_t                      senderAddressLength;
   char                           buffer[1024];
   ssize_t                        received;
   unsigned int                   result;
   size_t                         i;

   LOG_VERBOSE
   fputs("Trying to receive name server announce...\n", stdlog);
   LOG_END

   senderAddressLength = sizeof(senderAddress);
   received = ext_recvfrom(sd,
                           (char*)&buffer , sizeof(buffer), 0,
                           (struct sockaddr*)&senderAddress,
                           &senderAddressLength);
   if(received > 0) {
      message = asapPacket2Message((char*)&buffer, received, sizeof(buffer));
      if(message != NULL) {
         if(message->Type == AHT_SERVER_ANNOUNCE) {
            if(message->Error == RSPERR_OKAY) {
               LOG_VERBOSE
               fputs("ServerAnnounce from ", stdlog);
               address2string((struct sockaddr*)&senderAddress,
                              (char*)&buffer, sizeof(buffer), true);
               fputs(buffer, stdlog);
               fputs(" received\n", stdlog);
               LOG_END

               result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                           &serverTable->List,
                           message->NSIdentifier,
                           PLNF_DYNAMIC,
                           message->TransportAddressBlockListPtr,
                           getMicroTime(),
                           &peerListNode);
               if(result == RSPERR_OKAY) {
                  ST_CLASS(peerListManagementRestartPeerListNodeExpiryTimer)(
                     &serverTable->List,
                     peerListNode,
                     serverTable->NameServerAnnounceTimeout);
               }
               else {
                  LOG_ERROR
                  fputs("Unable to add new peer: ", stdlog);
                  rserpoolErrorPrint(result, stdlog);
                  fputs("\n", stdlog);
                  LOG_END
               }

               i = ST_CLASS(peerListManagementPurgeExpiredPeerListNodes)(
                      &serverTable->List,
                      getMicroTime());
               LOG_VERBOSE
               fprintf(stdlog, "Purged %u out-of-date peer list nodes\n", i);
               LOG_END
            }
         }
         asapMessageDelete(message);
      }
   }
}


/* ###### Constructor ####################################################### */
struct ServerTable* serverTableNew(struct Dispatcher* dispatcher,
                                   struct TagItem*    tags)
{
   struct sockaddr_storage* announceAddress;
   struct sockaddr_storage  defaultAnnounceAddress;
   struct ServerTable*      serverTable = (struct ServerTable*)malloc(sizeof(struct ServerTable));
   if(serverTable != NULL) {
      serverTable->Dispatcher = dispatcher;
      ST_CLASS(peerListManagementNew)(&serverTable->List, 0, NULL);

      /* ====== ASAP Instance settings ==================================== */
      serverTable->NameServerConnectMaxTrials = tagListGetData(tags, TAG_RspLib_NameServerConnectMaxTrials,
                                                               ASAP_DEFAULT_NAMESERVER_CONNECT_MAXTRIALS);
      serverTable->NameServerConnectTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerConnectTimeout,
                                                                     ASAP_DEFAULT_NAMESERVER_CONNECT_TIMEOUT);
      serverTable->NameServerAnnounceTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerAnnounceTimeout,
                                                                      ASAP_DEFAULT_NAMESERVER_ANNOUNCE_TIMEOUT);

      CHECK(string2address(ASAP_DEFAULT_NAMESERVER_ANNOUNCE_ADDRESS, &defaultAnnounceAddress) == true);
      announceAddress = (struct sockaddr_storage*)tagListGetData(tags, TAG_RspLib_NameServerAnnounceAddress,
                                                                 (tagdata_t)&defaultAnnounceAddress);
      memcpy(&serverTable->AnnounceAddress, announceAddress, sizeof(serverTable->AnnounceAddress));

      /* ====== Show results ================================================ */
      LOG_VERBOSE3
      fputs("New ServerTable's configuration:\n", stdlog);
      fprintf(stdlog, "nameserver.announce.timeout  = %Lu [µs]\n", serverTable->NameServerAnnounceTimeout);
      fputs("nameserver.announce.address  = ", stdlog);
      fputaddress((struct sockaddr*)&serverTable->AnnounceAddress, true, stdlog);
      fputs("\n", stdlog);
      fprintf(stdlog, "nameserver.connect.maxtrials = %u\n",       serverTable->NameServerConnectMaxTrials);
      fprintf(stdlog, "nameserver.connect.timeout   = %Lu [µs]\n", serverTable->NameServerConnectTimeout);
      LOG_END


      /* ====== Join announce multicast groups ============================ */
      serverTable->AnnounceSocket = ext_socket(serverTable->AnnounceAddress.sa.sa_family,
                                               SOCK_DGRAM, IPPROTO_UDP);
      if(serverTable->AnnounceSocket >= 0) {
         if(joinAnnounceMulticastGroup(serverTable->AnnounceSocket,
                                       &serverTable->AnnounceAddress,
                                       true)) {
            dispatcherAddFDCallback(serverTable->Dispatcher,
                                    serverTable->AnnounceSocket,
                                    FDCE_Read,
                                    handleServerAnnounceCallback,
                                    (void*)serverTable);
printf("************* MCAST=%d\n",serverTable->AnnounceSocket);
         }
         else {
            LOG_ERROR
            fputs("Joining multicast group ", stdlog);
            fputaddress((struct sockaddr*)&serverTable->AnnounceAddress, true, stdlog);
            fputs(" failed. Check routing (is default route set?) and firewall settings!\n", stdlog);
            LOG_END
            ext_close(serverTable->AnnounceSocket);
            serverTable->AnnounceSocket = -1;
         }
      }
      else {
         LOG_ERROR
         fputs("Creating a socket for name server announces failed\n", stdlog);
         LOG_END
      }
   }
   return(serverTable);
}


/* ###### Destructor ######################################################## */
void serverTableDelete(struct ServerTable* serverTable)
{
   if(serverTable != NULL) {
      if(serverTable->AnnounceSocket >= 0) {
         dispatcherRemoveFDCallback(serverTable->Dispatcher,
                                    serverTable->AnnounceSocket);
         ext_close(serverTable->AnnounceSocket);
         serverTable->AnnounceSocket = -1;
      }
      ST_CLASS(peerListManagementClear)(&serverTable->List);
      ST_CLASS(peerListManagementDelete)(&serverTable->List);
      free(serverTable);
   }
}


/* ###### Try more nameservers ############################################## */
static void tryNextBlock(struct ServerTable*      serverTable,
                         ST_CLASS(PeerListNode)** nextPeerListNode,
                         int*                     sd,
                         card64*                  timeout)
{
/*
   struct ServerAnnounce*   serverAnnounce;
   struct TransportAddress* transportAddress;
   int                      status;
   unsigned int             i;

   for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
      if(sd[i] < 0) {
         if(*serverAnnounceList) {
            serverAnnounce = (struct ServerAnnounce*)((*serverAnnounceList)->data);
            if(*serverTransportAddressList == NULL) {
               *serverTransportAddressList = g_list_first(serverAnnounce->TransportAddressList);
            }

            while(*serverTransportAddressList != NULL) {
               transportAddress = (struct TransportAddress*)(*serverTransportAddressList)->data;
               if((protocol == 0) || ((protocol == transportAddress->Protocol))) {
                  sd[i] = ext_socket(transportAddress->AddressArray[0].sa.sa_family,
                                     SOCK_STREAM,
                                     transportAddress->Protocol);
                  if(sd[i] >= 0) {
                     timeout[i] = getMicroTime() + serverTable->NameServerConnectTimeout;
                     setNonBlocking(sd[i]);

                     LOG_VERBOSE
                     fprintf(stdlog,"Connecting socket %d to ", sd[i]);
                     fputaddress((struct sockaddr*)&transportAddress->AddressArray[0],
                                 true, stdlog);
                     fprintf(stdlog," via %s...\n",(transportAddress->Protocol == IPPROTO_SCTP) ? "SCTP" : "TCP");
                     LOG_END
                     status = ext_connect(sd[i],
                                          (struct sockaddr*)&transportAddress->AddressArray[0],
                                          getSocklen((struct sockaddr*)&transportAddress->AddressArray[0]));
                     if( ((status < 0) && (errno == EINPROGRESS)) || (status == 0) ) {
                        break;
                     }

                     ext_close(sd[i]);
                     sd[i]      = -1;
                     timeout[i] = (card64)-1;
                  }
               }
               *serverTransportAddressList = g_list_next(*serverTransportAddressList);
            }
         }

         if(*serverTransportAddressList != NULL) {
            *serverTransportAddressList = g_list_next(*serverTransportAddressList);
         }
         if(*serverTransportAddressList == NULL) {
            *serverAnnounceList         = g_list_next(*serverAnnounceList);
            *serverTransportAddressList = NULL;
         }
      }
   }
*/
}


/* ###### Find nameserver ################################################### */
int serverTableFindServer(struct ServerTable* serverTable)
{
   struct timeval          selectTimeout;
   struct sockaddr_storage peerAddress;
   socklen_t               peerAddressLength;
   int                     sd[MAX_SIMULTANEOUS_REQUESTS];
   card64                  timeout[MAX_SIMULTANEOUS_REQUESTS];
   card64                  start;
   card64                  nextTimeout;
   ST_CLASS(PeerListNode)* nextPeerListNode;
   fd_set                  rfdset;
   fd_set                  wfdset;
   unsigned int            trials;
   unsigned int            i, j;
   int                     n, result;

   if(serverTable == NULL) {
      return(0);
   }
   for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
      sd[i]      = -1;
      timeout[i] = (card64)-1;
   }


   LOG_ACTION
   fputs("Looking for nameserver...\n", stdlog);
   LOG_END

   trials       = 0;
   nextPeerListNode = NULL;
   start        = 0;

   for(;;) {
      if(nextPeerListNode == NULL) {
         /* Start new trial, when
            - First time
            - A new server announce has been added to the list
            - The current trial has been running for at least serverTable->NameServerConnectTimeout */
         if( (start == 0)                                           ||
/* ?????             (serverTable->NameServerAnnounceListAddition >= start) || */
             (start + serverTable->NameServerConnectTimeout < getMicroTime()) ) {
            trials++;
            if(trials > serverTable->NameServerConnectMaxTrials) {
               LOG_ERROR
               fputs("No nameserver found!\n", stdlog);
               LOG_END
               for(j = 0;j < MAX_SIMULTANEOUS_REQUESTS;j++) {
                  if(sd[j] >= 0) {
                     ext_close(sd[j]);
                  }
               }
               return(0);
            }
            nextPeerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                                  &serverTable->List.List);
            start = getMicroTime();
            LOG_ACTION
            fprintf(stdlog,"Trial #%u...\n",trials);
            LOG_END
         }
      }

      /* Try next block of addresses */
      tryNextBlock(serverTable,
                   &nextPeerListNode,
                   (int*)&sd, (card64*)&timeout);

      /* Wait for event */
      n = 0;
      FD_ZERO(&rfdset);
      FD_ZERO(&wfdset);
      if(serverTable->AnnounceSocket >= 0) {
printf("mcast=%d\n",serverTable->AnnounceSocket);
         FD_SET(serverTable->AnnounceSocket, &rfdset);
         n = max(n, serverTable->AnnounceSocket);
      }
      nextTimeout = serverTable->NameServerConnectTimeout;
      for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
         if(sd[i] >= 0) {
            FD_SET(sd[i], &wfdset);
            n           = max(n, sd[i]);
            nextTimeout = min(nextTimeout, timeout[i]);
         }
      }
      selectTimeout.tv_sec  = nextTimeout / (card64)1000000;
      selectTimeout.tv_usec = nextTimeout % (card64)1000000;
      LOG_VERBOSE3
      fprintf(stdlog,"select() with timeout %Ld\n",nextTimeout);
      LOG_END
      result = ext_select(n + 1,
                          &rfdset, &wfdset, NULL,
                          &selectTimeout);
      LOG_VERBOSE3
      fprintf(stdlog,"select() result=%d\n",result);
      LOG_END

      /* Handle events */
      if(result != 0) {
         for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
            if(sd[i] >= 0) {
               if(FD_ISSET(sd[i], &wfdset)) {
                  peerAddressLength = sizeof(peerAddress);
                  if(ext_getpeername(sd[i], (struct sockaddr*)&peerAddress, &peerAddressLength) >= 0) {
                     LOG_ACTION
                     fputs("Successfully found nameserver at ", stdlog);
                     fputaddress((struct sockaddr*)&peerAddress, true, stdlog);
                     fprintf(stdlog,", socket %d\n", sd[i]);
                     LOG_END
                     for(j = 0;j < MAX_SIMULTANEOUS_REQUESTS;j++) {
                        if((j != i) && (sd[j] >= 0)) {
                           ext_close(sd[j]);
                        }
                     }
                     return(sd[i]);
                  }
                  else {
                     LOG_VERBOSE3
                     fprintf(stdlog,"Connection trial of socket %d failed: %s\n",
                             sd[i], strerror(errno));
                     LOG_END
                     ext_close(sd[i]);
                     sd[i] = -1;
                  }
               }
               else if((sd[i] >= 0) && (timeout[i] < getMicroTime())) {
                  LOG_VERBOSE3
                  fprintf(stdlog,"Connection trial of socket %d timed out\n", sd[i]);
                  LOG_END
                  ext_close(sd[i]);
                  sd[i] = -1;
               }
            }
         }
         if((serverTable->AnnounceSocket >= 0) &&
            FD_ISSET(serverTable->AnnounceSocket, &rfdset)) {
puts("ACHTUNG: Hier kein PURGE durchführen!!!");
            handleServerAnnounceCallback(serverTable->Dispatcher,
                                         serverTable->AnnounceSocket,
                                         FDCE_Read,
                                         serverTable);
         }
      }
   }
}
