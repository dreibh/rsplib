/*
 *  $Id: servertable.c,v 1.29 2004/11/22 18:07:53 dreibh Exp $
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



/* Maximum number of simultaneous registrar connection trials */
#define MAX_SIMULTANEOUS_REQUESTS 3



/* ###### Server announce callback  ######################################### */
static void handleServerAnnounceCallback(struct ServerTable* serverTable,
                                         int                 sd)
{
   struct RSerPoolMessage*        message;
   struct ST_CLASS(PeerListNode)* peerListNode;
   union sockaddr_union           senderAddress;
   socklen_t                      senderAddressLength;
   char                           buffer[1024];
   ssize_t                        received;
   unsigned int                   result;
   size_t                         i;

   LOG_VERBOSE2
   fputs("Trying to receive registrar announce...\n",  stdlog);
   LOG_END

   senderAddressLength = sizeof(senderAddress);
   received = ext_recvfrom(sd,
                           (char*)&buffer , sizeof(buffer), 0,
                           (struct sockaddr*)&senderAddress,
                           &senderAddressLength);
   if(received > 0) {
      result = rserpoolPacket2Message((char*)&buffer,
                                      &senderAddress,
                                      PPID_ASAP,
                                      received, sizeof(buffer),
                                      &message);
      if(message != NULL) {
         if(result == RSPERR_OKAY) {
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
                              &serverTable->ServerList,
                              message->RegistrarIdentifier,
                              PLNF_DYNAMIC,
                              message->TransportAddressBlockListPtr,
                              getMicroTime(),
                              &peerListNode);
                  if(result == RSPERR_OKAY) {
                     serverTable->LastAnnounceHeard = getMicroTime();
                     ST_CLASS(peerListManagementRestartPeerListNodeExpiryTimer)(
                        &serverTable->ServerList,
                        peerListNode,
                        serverTable->RegistrarAnnounceTimeout);
                  }
                  else {
                     LOG_ERROR
                     fputs("Unable to add new peer: ",  stdlog);
                     rserpoolErrorPrint(result, stdlog);
                     fputs("\n",  stdlog);
                     LOG_END
                  }

                  i = ST_CLASS(peerListManagementPurgeExpiredPeerListNodes)(
                        &serverTable->ServerList,
                        getMicroTime());
                  LOG_VERBOSE3
                  fprintf(stdlog, "Purged %u out-of-date peer list nodes. Peer List:\n",  (unsigned int)i);
                  ST_CLASS(peerListManagementPrint)(&serverTable->ServerList, stdlog, PLPO_FULL);
                  LOG_END
               }
            }
         }
         rserpoolMessageDelete(message);
      }
   }
}


/* ###### Server announce callback for FDCallback ######################## */
/* This function is only called to "flush" the socket's buffer into the
   cache when there is *no* NS hunt in progress. The NS hunt itself may
   *not* use the Dispatcher's select() procedure -> This may case
   recursive invocation! Therefore, serverTableFindServer() simply
   calls ext_select() and manually calls handleServerAnnounceCallback()
   to read from the announce socket. */
static void serverAnnouceFDCallback(struct Dispatcher* dispatcher,
                                    int                fd,
                                    unsigned int       eventMask,
                                    void*              userData)
{
   LOG_VERBOSE3
   fputs("Entering serverAnnouceFDCallback()...\n", stdlog);
   LOG_END
   handleServerAnnounceCallback((struct ServerTable*)userData, fd);
   LOG_VERBOSE3
   fputs("Leaving serverAnnouceFDCallback()\n", stdlog);
   LOG_END
}


/* ###### Constructor ####################################################### */
struct ServerTable* serverTableNew(struct Dispatcher* dispatcher,
                                   struct TagItem*    tags)
{
   union sockaddr_union* announceAddress;
   union sockaddr_union  defaultAnnounceAddress;
   struct ServerTable*   serverTable = (struct ServerTable*)malloc(sizeof(struct ServerTable));

   if(serverTable != NULL) {
      serverTable->Dispatcher        = dispatcher;
      serverTable->LastAnnounceHeard = 0;
      ST_CLASS(peerListManagementNew)(&serverTable->ServerList, 0, NULL, NULL);

      /* ====== ASAP Instance settings ==================================== */
      serverTable->RegistrarConnectMaxTrials = tagListGetData(tags, TAG_RspLib_RegistrarConnectMaxTrials,
                                                               ASAP_DEFAULT_NAMESERVER_CONNECT_MAXTRIALS);
      serverTable->RegistrarConnectTimeout = (card64)tagListGetData(tags, TAG_RspLib_RegistrarConnectTimeout,
                                                                     ASAP_DEFAULT_NAMESERVER_CONNECT_TIMEOUT);
      serverTable->RegistrarAnnounceTimeout = (card64)tagListGetData(tags, TAG_RspLib_RegistrarAnnounceTimeout,
                                                                      ASAP_DEFAULT_NAMESERVER_ANNOUNCE_TIMEOUT);

      CHECK(string2address(ASAP_DEFAULT_NAMESERVER_ANNOUNCE_ADDRESS, &defaultAnnounceAddress) == true);
      announceAddress = (union sockaddr_union*)tagListGetData(tags, TAG_RspLib_RegistrarAnnounceAddress,
                                                                 (tagdata_t)&defaultAnnounceAddress);
      memcpy(&serverTable->AnnounceAddress, announceAddress, sizeof(serverTable->AnnounceAddress));

      /* ====== Show results ================================================ */
      LOG_VERBOSE3
      fputs("New ServerTable's configuration:\n",  stdlog);
      fprintf(stdlog, "registrar.announce.timeout  = %llu [us]\n",  serverTable->RegistrarAnnounceTimeout);
      fputs("registrar.announce.address  = ",  stdlog);
      fputaddress((struct sockaddr*)&serverTable->AnnounceAddress, true, stdlog);
      fputs("\n",  stdlog);
      fprintf(stdlog, "registrar.connect.maxtrials = %u\n",        serverTable->RegistrarConnectMaxTrials);
      fprintf(stdlog, "registrar.connect.timeout   = %llu [us]\n",  serverTable->RegistrarConnectTimeout);
      LOG_END


      /* ====== Join announce multicast groups ============================ */
      serverTable->AnnounceSocket = ext_socket(serverTable->AnnounceAddress.sa.sa_family,
                                               SOCK_DGRAM, IPPROTO_UDP);
      if(serverTable->AnnounceSocket >= 0) {
         setReusable(serverTable->AnnounceSocket, 1);
         if(bindplus(serverTable->AnnounceSocket,
                     &serverTable->AnnounceAddress,
                     1) == true) {
            if(joinOrLeaveMulticastGroup(serverTable->AnnounceSocket,
                                         &serverTable->AnnounceAddress,
                                         true)) {
               fdCallbackNew(&serverTable->AnnounceSocketFDCallback,
                           serverTable->Dispatcher,
                           serverTable->AnnounceSocket,
                           FDCE_Read,
                           serverAnnouceFDCallback,
                           serverTable);
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
            fputs("Unable to bind multicast socket to address ",  stdlog);
            fputaddress((struct sockaddr*)&serverTable->AnnounceAddress.sa, true, stdlog);
            fputs("\n",  stdlog);
            LOG_END
            return(false);
         }
      }
      else {
         LOG_ERROR
         fputs("Creating a socket for registrar announces failed\n",  stdlog);
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
         fdCallbackDelete(&serverTable->AnnounceSocketFDCallback);
         ext_close(serverTable->AnnounceSocket);
         serverTable->AnnounceSocket = -1;
      }
      ST_CLASS(peerListManagementClear)(&serverTable->ServerList);
      ST_CLASS(peerListManagementDelete)(&serverTable->ServerList);
      free(serverTable);
   }
}


/* ###### Add static registrar entry ################################### */
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
               &serverTable->ServerList,
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


/* ###### Try more registrars ########################################### */
static void tryNextBlock(struct ServerTable*            serverTable,
                         RegistrarIdentifierType*            lastRegistrarIdentifier,
                         struct TransportAddressBlock** lastTransportAddressBlock,
                         int*                           sd,
                         card64*                        timeout,
                         RegistrarIdentifierType*            identifier)
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
                           &serverTable->ServerList,
                           *lastRegistrarIdentifier,
                           *lastTransportAddressBlock);

         if(peerListNode != NULL) {
            *lastRegistrarIdentifier = peerListNode->Identifier;
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
               fputs("Trying registrar at ",  stdlog);
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
                     timeout[i]    = getMicroTime() + serverTable->RegistrarConnectTimeout;
                     identifier[i] = peerListNode->Identifier;
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
                     sd[i]         = -1;
                     timeout[i]    = (card64)-1;
                     identifier[i] = 0;
                  }
               }
               else {
                  LOG_ERROR
                  fputs("Unknown address type in TransportAddressBlock\n", stdlog);
                  LOG_END
               }
            }
         }

         *lastRegistrarIdentifier = 0;
         *lastTransportAddressBlock = NULL;
         break;
      }
   }
}


/* ###### Find registrar ################################################### */
int serverTableFindServer(struct ServerTable* serverTable,
                          RegistrarIdentifierType* registrarIdentifier)
{
   struct timeval                 selectTimeout;
   union sockaddr_union           peerAddress;
   socklen_t                      peerAddressLength;
   int                            sd[MAX_SIMULTANEOUS_REQUESTS];
   card64                         timeout[MAX_SIMULTANEOUS_REQUESTS];
   RegistrarIdentifierType             identifier[MAX_SIMULTANEOUS_REQUESTS];
   RegistrarIdentifierType             lastRegistrarIdentifier;
   struct TransportAddressBlock*  lastTransportAddressBlock;
   struct ST_CLASS(PeerListNode)* peerListNode;
   card64                         start;
   card64                         nextTimeout;
   fd_set                         rfdset;
   fd_set                         wfdset;
   unsigned int                   trials;
   unsigned int                   i, j;
   int                            n, result;

   *registrarIdentifier = 0;
   if(serverTable == NULL) {
      return(-1);
   }
   for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
      sd[i]         = -1;
      timeout[i]    = (card64)-1;
      identifier[i] = 0;
   }


   LOG_ACTION
   fputs("Looking for registrar...\n",  stdlog);
   LOG_END

   trials                    = 0;
   lastRegistrarIdentifier          = 0;
   lastTransportAddressBlock = NULL;
   start                     = 0;

   peerListNode = ST_CLASS(peerListGetRandomPeerNode)(&serverTable->ServerList.List);
   if(peerListNode) {
      lastRegistrarIdentifier          = peerListNode->Identifier;
      lastTransportAddressBlock = transportAddressBlockDuplicate(peerListNode->AddressBlock);
      LOG_NOTE
      fputs("Randomized server hunt start: ", stdlog);
      ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
      fputs("\n", stdlog);
      LOG_END
   }

   for(;;) {
      if((lastRegistrarIdentifier == 0) && (lastTransportAddressBlock == NULL)) {
         /* Start new trial, when
            - First time
            - A new server announce has been added to the list
            - The current trial has been running for at least serverTable->RegistrarConnectTimeout */
         if( (start == 0) ||
             (serverTable->LastAnnounceHeard >= start) ||
             (start + serverTable->RegistrarConnectTimeout < getMicroTime()) ) {
            trials++;
            if(trials > serverTable->RegistrarConnectMaxTrials) {
               LOG_ERROR
               fputs("No registrar found!\n", stdlog);
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
                   &lastRegistrarIdentifier,
                   &lastTransportAddressBlock,
                   (int*)&sd, (card64*)&timeout, (RegistrarIdentifierType*)&identifier);

      /* Wait for event */
      n = 0;
      FD_ZERO(&rfdset);
      FD_ZERO(&wfdset);
      nextTimeout = serverTable->RegistrarConnectTimeout;
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
      fprintf(stdlog, "select() with timeout %lld\n", nextTimeout);
      LOG_END
      /*
         Important: rspSelect() may *not* be used here!
         serverTableFindServer() may be called from within a
         session timer + from within registrar socket callback
         => Strange things will happen when these callbacks are
            invoked recursively!
      */
      result = ext_select(n + 1,
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
         if(FD_ISSET(serverTable->AnnounceSocket, &rfdset)) {
            handleServerAnnounceCallback(serverTable, serverTable->AnnounceSocket);
         }
         for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
            if(sd[i] >= 0) {
               if(FD_ISSET(sd[i], &wfdset)) {
                  peerAddressLength = sizeof(peerAddress);
                  if(ext_getpeername(sd[i], (struct sockaddr*)&peerAddress, &peerAddressLength) >= 0) {
                     LOG_ACTION
                     fputs("Successfully found registrar at ",  stdlog);
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
                     *registrarIdentifier = identifier[i];
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
