/*
 *  $Id: servertable.c,v 1.13 2004/08/04 01:02:39 dreibh Exp $
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
#include "asapinstance.h"
#include "rserpoolmessage.h"
#include "rsplib.h"
#include "rsplib-tags.h"

#include <ext_socket.h>
#include <sys/time.h>



/* Maximum number of simultaneous nameserver connection trials */
#define MAX_SIMULTANEOUS_REQUESTS 3



/* ###### Server announce callback  ######################################### */
static void handleServerAnnounceCallback(struct Dispatcher* dispatcher,
                                         int                sd,
                                         int                eventMask,
                                         void*              userData)
{
   struct ServerTable*            serverTable = (struct ServerTable*)userData;
   struct RSerPoolMessage*        message;
   struct ST_CLASS(PeerListNode)* peerListNode;
   union sockaddr_union           senderAddress;
   socklen_t                      senderAddressLength;
   char                           buffer[1024];
   ssize_t                        received;
   unsigned int                   result;
   size_t                         i;

   LOG_VERBOSE2
   fputs("Trying to receive name server announce...\n",  stdlog);
   LOG_END

   senderAddressLength = sizeof(senderAddress);
   received = ext_recvfrom(sd,
                           (char*)&buffer , sizeof(buffer), 0,
                           (struct sockaddr*)&senderAddress,
                           &senderAddressLength);
   if(received > 0) {
      message = rserpoolPacket2Message((char*)&buffer,
                                       PPID_ASAP,
                                       received, sizeof(buffer));
      if(message != NULL) {
         if(message->Type == AHT_SERVER_ANNOUNCE) {
            if(message->Error == RSPERR_OKAY) {
               LOG_VERBOSE2
               fputs("ServerAnnounce from ",  stdlog);
               address2string((struct sockaddr*)&senderAddress,
                              (char*)&buffer, sizeof(buffer), true);
               fputs(buffer, stdlog);
               fputs(" received\n",  stdlog);
               LOG_END

               result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                           &serverTable->List,
                           message->NSIdentifier,
                           PLNF_DYNAMIC,
                           message->TransportAddressBlockListPtr,
                           getMicroTime(),
                           &peerListNode);
               if(result == RSPERR_OKAY) {
ST_CLASS(peerListManagementPrint)(&serverTable->List, stdlog, PLPO_FULL);
// ??????????

                  serverTable->LastAnnounce = getMicroTime();
                  ST_CLASS(peerListManagementRestartPeerListNodeExpiryTimer)(
                     &serverTable->List,
                     peerListNode,
                     serverTable->NameServerAnnounceTimeout);
               }
               else {
                  LOG_ERROR
                  fputs("Unable to add new peer: ",  stdlog);
                  rserpoolErrorPrint(result, stdlog);
                  fputs("\n",  stdlog);
                  LOG_END
               }

               i = ST_CLASS(peerListManagementPurgeExpiredPeerListNodes)(
                      &serverTable->List,
                      getMicroTime());
               LOG_VERBOSE3
               fprintf(stdlog, "Purged %u out-of-date peer list nodes. Peer List:\n",  i);
               ST_CLASS(peerListManagementPrint)(&serverTable->List, stdlog, PLPO_FULL);
               LOG_END
            }
         }
         rserpoolMessageDelete(message);
      }
   }
}


/* ###### Constructor ####################################################### */
struct ServerTable* serverTableNew(struct Dispatcher* dispatcher,
                                   struct TagItem*    tags)
{
   union sockaddr_union* announceAddress;
   union sockaddr_union  defaultAnnounceAddress;
   struct ServerTable*   serverTable = (struct ServerTable*)malloc(sizeof(struct ServerTable));
   if(serverTable != NULL) {
      serverTable->Dispatcher   = dispatcher;
      serverTable->LastAnnounce = 0;
      ST_CLASS(peerListManagementNew)(&serverTable->List, 0, NULL, NULL);

      /* ====== ASAP Instance settings ==================================== */
      serverTable->NameServerConnectMaxTrials = tagListGetData(tags, TAG_RspLib_NameServerConnectMaxTrials,
                                                               ASAP_DEFAULT_NAMESERVER_CONNECT_MAXTRIALS);
      serverTable->NameServerConnectTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerConnectTimeout,
                                                                     ASAP_DEFAULT_NAMESERVER_CONNECT_TIMEOUT);
      serverTable->NameServerAnnounceTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerAnnounceTimeout,
                                                                      ASAP_DEFAULT_NAMESERVER_ANNOUNCE_TIMEOUT);

      CHECK(string2address(ASAP_DEFAULT_NAMESERVER_ANNOUNCE_ADDRESS, &defaultAnnounceAddress) == true);
      announceAddress = (union sockaddr_union*)tagListGetData(tags, TAG_RspLib_NameServerAnnounceAddress,
                                                                 (tagdata_t)&defaultAnnounceAddress);
      memcpy(&serverTable->AnnounceAddress, announceAddress, sizeof(serverTable->AnnounceAddress));

      /* ====== Show results ================================================ */
      LOG_VERBOSE3
      fputs("New ServerTable's configuration:\n",  stdlog);
      fprintf(stdlog, "nameserver.announce.timeout  = %Lu [µs]\n",  serverTable->NameServerAnnounceTimeout);
      fputs("nameserver.announce.address  = ",  stdlog);
      fputaddress((struct sockaddr*)&serverTable->AnnounceAddress, true, stdlog);
      fputs("\n",  stdlog);
      fprintf(stdlog, "nameserver.connect.maxtrials = %u\n",        serverTable->NameServerConnectMaxTrials);
      fprintf(stdlog, "nameserver.connect.timeout   = %Lu [µs]\n",  serverTable->NameServerConnectTimeout);
      LOG_END


      /* ====== Join announce multicast groups ============================ */
      serverTable->AnnounceSocket = ext_socket(serverTable->AnnounceAddress.sa.sa_family,
                                               SOCK_DGRAM, IPPROTO_UDP);
      if(serverTable->AnnounceSocket >= 0) {
         if(joinOrLeaveMulticastGroup(serverTable->AnnounceSocket,
                                      &serverTable->AnnounceAddress,
                                      true)) {
            dispatcherAddFDCallback(serverTable->Dispatcher,
                                    serverTable->AnnounceSocket,
                                    FDCE_Read,
                                    handleServerAnnounceCallback,
                                    (void*)serverTable);
         }
         else {
            LOG_ERROR
            fputs("Joining multicast group ",  stdlog);
            fputaddress((struct sockaddr*)&serverTable->AnnounceAddress, true, stdlog);
            fputs(" failed. Check routing (is default route set?) and firewall settings!\n",  stdlog);
            LOG_END
            ext_close(serverTable->AnnounceSocket);
            serverTable->AnnounceSocket = -1;
         }
      }
      else {
         LOG_ERROR
         fputs("Creating a socket for name server announces failed\n",  stdlog);
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


/* ###### Add static name server entry ################################### */
unsigned int serverTableAddStaticEntry(struct ServerTable*   serverTable,
                                       union sockaddr_union* addressArray,
                                       size_t                addresses)
{
   char                           transportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*  transportAddressBlock = (struct TransportAddressBlock*)&transportAddressBlockBuffer;
   struct ST_CLASS(PeerListNode)* peerListNode;
   int                            result;

   transportAddressBlockNew(transportAddressBlock,
                            IPPROTO_SCTP,
                            getPort((struct sockaddr*)&addressArray[0]),
                            0,
                            addressArray,
                            addresses);
   result = ST_CLASS(peerListManagementRegisterPeerListNode)(
               &serverTable->List,
               0,
               PLNF_STATIC,
               transportAddressBlock,
               getMicroTime(),
               &peerListNode);
   if(result == RSPERR_OKAY) {
      LOG_VERBOSE
      fputs("Added static entry to server table: ", stdlog);
      ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
      fputs("\n", stdlog);
      LOG_END
   }
   else {
      LOG_WARNING
      fputs("Unable to add static entry to server table: ", stdlog);
      transportAddressBlockPrint(transportAddressBlock, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }
   return(result);
}


/* ###### Try more nameservers ########################################### */
static void tryNextBlock(struct ServerTable*     serverTable,
                         ENRPIdentifierType*     lastNSIdentifier,
                         TransportAddressBlock** lastTransportAddressBlock,
                         int*                    sd,
                         card64*                 timeout)
{
   struct TransportAddressBlock*  transportAddressBlock;
   struct ST_CLASS(PeerListNode)* peerListNode;
   union sockaddr_union           addressArray[MAX_PE_TRANSPORTADDRESSES];
   size_t                         addresses;
   int                            status;
   size_t                         i, j;
   void*                          a;

   for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
      if(sd[i] < 0) {
         peerListNode = ST_CLASS(peerListManagementFindNearestNextPeerListNode)(
                           &serverTable->List,
                           *lastNSIdentifier,
                           *lastTransportAddressBlock);

         if(peerListNode != NULL) {
            *lastNSIdentifier = peerListNode->Identifier;
            if(*lastTransportAddressBlock) {
               transportAddressBlockDelete(*lastTransportAddressBlock);
               free(*lastTransportAddressBlock);
            }
            *lastTransportAddressBlock = transportAddressBlockDuplicate(peerListNode->AddressBlock);

            transportAddressBlock = peerListNode->AddressBlock;
            while(transportAddressBlock != NULL) {
               if(transportAddressBlock->Protocol == IPPROTO_SCTP) {
                  break;
               }
               transportAddressBlock = transportAddressBlock->Next;
            }
            if(transportAddressBlock != NULL) {
               LOG_VERBOSE2
               fputs("Trying name server at ",  stdlog);
               transportAddressBlockPrint(transportAddressBlock, stdlog);
               fputs("\n",  stdlog);
               LOG_END

               addresses = 0;
               a = &addressArray;
               CHECK(transportAddressBlock->Addresses <= MAX_PE_TRANSPORTADDRESSES);
               for(j = 0;j < transportAddressBlock->Addresses;j++) {
                  switch(transportAddressBlock->AddressArray[j].sa.sa_family) {
                     case AF_INET:
                        memcpy((void*)a, (void*)&transportAddressBlock->AddressArray[j],
                               sizeof(struct sockaddr_in));
                        a = (void*)((long)a + (long)sizeof(struct sockaddr_in));
                     break;
                     case AF_INET6:
                        memcpy((void*)a, (void*)&transportAddressBlock->AddressArray[j],
                               sizeof(struct sockaddr_in6));
                        a = (void*)((long)a + (long)sizeof(struct sockaddr_in6));
                     break;
                  }
                  addresses++;
               }

               if(addresses == transportAddressBlock->Addresses) {
                  sd[i] = ext_socket(checkIPv6() ? AF_INET6 : AF_INET,
                                    SOCK_STREAM,
                                    transportAddressBlock->Protocol);
                  if(sd[i] >= 0) {
                     timeout[i] = getMicroTime() + serverTable->NameServerConnectTimeout;
                     setNonBlocking(sd[i]);

                     LOG_VERBOSE2
                     fprintf(stdlog, "Connecting socket %d to ", sd[i]);
                     transportAddressBlockPrint(transportAddressBlock, stdlog);
                     fputs("...\n", stdlog);
                     LOG_END

                     status = sctp_connectx(sd[i], (struct sockaddr*)addressArray, addresses);
                     if( ((status < 0) && (errno == EINPROGRESS)) || (status == 0) ) {
                        break;
                     }

                     ext_close(sd[i]);
                     sd[i]      = -1;
                     timeout[i] = (card64)-1;
                  }
               }
               else {
                  LOG_ERROR
                  fputs("Unknown address type in TransportAddressBlock\n", stdlog);
                  LOG_END
               }
            }
         }

         *lastNSIdentifier = 0;
         *lastTransportAddressBlock = NULL;
         break;
      }
   }
}


/* ###### Find nameserver ################################################### */
int serverTableFindServer(struct ServerTable* serverTable)
{
   struct timeval          selectTimeout;
   union sockaddr_union    peerAddress;
   socklen_t               peerAddressLength;
   int                     sd[MAX_SIMULTANEOUS_REQUESTS];
   card64                  timeout[MAX_SIMULTANEOUS_REQUESTS];
   ENRPIdentifierType      lastNSIdentifier;
   TransportAddressBlock*  lastTransportAddressBlock;
   card64                  start;
   card64                  nextTimeout;
   fd_set                  rfdset;
   fd_set                  wfdset;
   unsigned int            trials;
   unsigned int            i, j;
   int                     n, result;

   if(serverTable == NULL) {
      return(-1);
   }
   for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
      sd[i]      = -1;
      timeout[i] = (card64)-1;
   }


   LOG_ACTION
   fputs("Looking for nameserver...\n",  stdlog);
   LOG_END

   trials                    = 0;
   lastNSIdentifier          = 0;
   lastTransportAddressBlock = NULL;
   start                     = 0;

   for(;;) {
      if((lastNSIdentifier == 0) && (lastTransportAddressBlock == NULL)) {
         /* Start new trial, when
            - First time
            - A new server announce has been added to the list
            - The current trial has been running for at least serverTable->NameServerConnectTimeout */
         if( (start == 0) ||
             (serverTable->LastAnnounce >= start) ||
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
               if(lastTransportAddressBlock) {
                  transportAddressBlockDelete(lastTransportAddressBlock);
                  free(lastTransportAddressBlock);
               }
               return(-1);
            }

            start = getMicroTime();
            LOG_ACTION
            fprintf(stdlog, "Trial #%u...\n", trials);
            LOG_END
         }
      }

      /* Try next block of addresses */
      tryNextBlock(serverTable,
                   &lastNSIdentifier,
                   &lastTransportAddressBlock,
                   (int*)&sd, (card64*)&timeout);

      /* Wait for event */
      n = 0;
      FD_ZERO(&rfdset);
      FD_ZERO(&wfdset);
      nextTimeout = serverTable->NameServerConnectTimeout;
      for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
         if(sd[i] >= 0) {
            FD_SET(sd[i], &wfdset);
            n           = max(n, sd[i]);
            nextTimeout = min(nextTimeout, timeout[i]);
         }
      }
      if(n == 0) {
         nextTimeout = 500000;
      }
      if(serverTable->AnnounceSocket >= 0) {
         FD_SET(serverTable->AnnounceSocket, &rfdset);
         n = max(n, serverTable->AnnounceSocket);
      }
      selectTimeout.tv_sec  = nextTimeout / (card64)1000000;
      selectTimeout.tv_usec = nextTimeout % (card64)1000000;
      LOG_VERBOSE3
      fprintf(stdlog, "select() with timeout %Ld\n", nextTimeout);
      LOG_END
      result = rspSelect(n + 1,
                         &rfdset, &wfdset, NULL,
                         &selectTimeout);
      LOG_VERBOSE3
      fprintf(stdlog, "select() result=%d\n", result);
      LOG_END
      if((result < 0) && (errno == EINTR)) {
         for(j = 0;j < MAX_SIMULTANEOUS_REQUESTS;j++) {
            if(sd[j] >= 0) {
               ext_close(sd[j]);
            }
         }
         LOG_ACTION
         fputs("Interrupted select() call -> returning immediately!\n",  stdlog);
         LOG_END
         if(lastTransportAddressBlock) {
            transportAddressBlockDelete(lastTransportAddressBlock);
            free(lastTransportAddressBlock);
         }
         return(-1);
      }

      /* Handle events */
      if(result != 0) {
         for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
            if(sd[i] >= 0) {
               if(FD_ISSET(sd[i], &wfdset)) {
                  peerAddressLength = sizeof(peerAddress);
                  if(ext_getpeername(sd[i], (struct sockaddr*)&peerAddress, &peerAddressLength) >= 0) {
                     LOG_ACTION
                     fputs("Successfully found nameserver at ",  stdlog);
                     fputaddress((struct sockaddr*)&peerAddress, true, stdlog);
                     fprintf(stdlog, ",  socket %d\n",  sd[i]);
                     LOG_END
                     for(j = 0;j < MAX_SIMULTANEOUS_REQUESTS;j++) {
                        if((j != i) && (sd[j] >= 0)) {
                           ext_close(sd[j]);
                        }
                     }
                     if(lastTransportAddressBlock) {
                        transportAddressBlockDelete(lastTransportAddressBlock);
                        free(lastTransportAddressBlock);
                     }
                     return(sd[i]);
                  }
                  else {
                     LOG_VERBOSE3
                     fprintf(stdlog, "Connection trial of socket %d failed: %s\n",
                             sd[i], strerror(errno));
                     LOG_END
                     ext_close(sd[i]);
                     sd[i] = -1;
                  }
               }
               else if((sd[i] >= 0) && (timeout[i] < getMicroTime())) {
                  LOG_VERBOSE3
                  fprintf(stdlog, "Connection trial of socket %d timed out\n",  sd[i]);
                  LOG_END
                  ext_close(sd[i]);
                  sd[i] = -1;
               }
            }
         }
      }
   }
}
