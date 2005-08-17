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
 * Purpose: Registrar Table
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "registrartable.h"
#include "netutilities.h"
#include "asapinstance.h"
#include "rserpoolmessage.h"
#include "rsplib.h"
#include "rsplib-tags.h"

#include <ext_socket.h>
#include <sys/time.h>



/* Maximum number of simultaneous registrar connection trials */
#define MAX_SIMULTANEOUS_REQUESTS 3



/* ###### Registrar announce callback  ######################################### */
static void handleRegistrarAnnounceCallback(struct RegistrarTable* registrarTable,
                                            int                    sd)
{
   struct RSerPoolMessage*        message;
   struct ST_CLASS(PeerListNode)* peerListNode;
   union sockaddr_union           senderAddress;
   socklen_t                      senderAddressLength;
   char                           buffer[1024];
   ssize_t                        received;
   unsigned int                   result;
   size_t                         i;

   LOG_VERBOSE3
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
                                      0,
                                      PPID_ASAP,
                                      received, sizeof(buffer),
                                      &message);
      if(message != NULL) {
         if(result == RSPERR_OKAY) {
            if(message->Type == AHT_SERVER_ANNOUNCE) {
               if(message->Error == RSPERR_OKAY) {
                  LOG_VERBOSE2
                  fputs("RegistrarAnnounce from ",  stdlog);
                  address2string((struct sockaddr*)&senderAddress,
                                 (char*)&buffer, sizeof(buffer), true);
                  fputs(buffer, stdlog);
                  fputs(" received\n",  stdlog);
                  LOG_END

                  result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                              &registrarTable->RegistrarList,
                              message->RegistrarIdentifier,
                              PLNF_DYNAMIC,
                              message->TransportAddressBlockListPtr,
                              getMicroTime(),
                              &peerListNode);
                  if(result == RSPERR_OKAY) {
                     registrarTable->LastAnnounceHeard = getMicroTime();
                     ST_CLASS(peerListManagementRestartPeerListNodeExpiryTimer)(
                        &registrarTable->RegistrarList,
                        peerListNode,
                        registrarTable->RegistrarAnnounceTimeout);
                  }
                  else {
                     LOG_ERROR
                     fputs("Unable to add new peer: ",  stdlog);
                     rserpoolErrorPrint(result, stdlog);
                     fputs("\n",  stdlog);
                     LOG_END
                  }

                  i = ST_CLASS(peerListManagementPurgeExpiredPeerListNodes)(
                        &registrarTable->RegistrarList,
                        getMicroTime());
                  LOG_VERBOSE3
                  fprintf(stdlog, "Purged %u out-of-date peer list nodes. Peer List:\n",  (unsigned int)i);
                  ST_CLASS(peerListManagementPrint)(&registrarTable->RegistrarList, stdlog, PLPO_PEERS_INDEX|PLNPO_TRANSPORT);
                  LOG_END
               }
            }
         }
         rserpoolMessageDelete(message);
      }
   }
}


/* ###### Registrar announce callback for FDCallback ######################## */
/* This function is only called to "flush" the socket's buffer into the
   cache when there is *no* PR hunt in progress. The PR hunt itself may
   *not* use the Dispatcher's select() procedure -> This may case
   recursive invocation! Therefore, registrarTableFindRegistrar() simply
   calls ext_select() and manually calls handleRegistrarAnnounceCallback()
   to read from the announce socket. */
static void registrarAnnouceFDCallback(struct Dispatcher* dispatcher,
                                       int                fd,
                                       unsigned int       eventMask,
                                       void*              userData)
{
   LOG_VERBOSE3
   fputs("Entering registrarAnnouceFDCallback()...\n", stdlog);
   LOG_END
   handleRegistrarAnnounceCallback((struct RegistrarTable*)userData, fd);
   LOG_VERBOSE3
   fputs("Leaving registrarAnnouceFDCallback()\n", stdlog);
   LOG_END
}


/* ###### Constructor ####################################################### */
struct RegistrarTable* registrarTableNew(struct Dispatcher* dispatcher,
                                         struct TagItem*    tags)
{
   union sockaddr_union*  announceAddress;
   union sockaddr_union   defaultAnnounceAddress;
   struct RegistrarTable* registrarTable = (struct RegistrarTable*)malloc(sizeof(struct RegistrarTable));

   if(registrarTable != NULL) {
      registrarTable->Dispatcher          = dispatcher;
      registrarTable->LastAnnounceHeard   = 0;
      registrarTable->OutstandingConnects = 0;
      registrarTable->AnnounceSocket      = -1;
      ST_CLASS(peerListManagementNew)(&registrarTable->RegistrarList, NULL, 0, NULL, NULL);


      /* ====== ASAP Instance settings ==================================== */
      registrarTable->RegistrarConnectMaxTrials = tagListGetData(tags, TAG_RspLib_RegistrarConnectMaxTrials,
                                                              ASAP_DEFAULT_REGISTRAR_CONNECT_MAXTRIALS);
      registrarTable->RegistrarConnectTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_RegistrarConnectTimeout,
                                                                     ASAP_DEFAULT_REGISTRAR_CONNECT_TIMEOUT);
      registrarTable->RegistrarAnnounceTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_RegistrarAnnounceTimeout,
                                                                      ASAP_DEFAULT_REGISTRAR_ANNOUNCE_TIMEOUT);

      CHECK(string2address(ASAP_DEFAULT_REGISTRAR_ANNOUNCE_ADDRESS, &defaultAnnounceAddress) == true);
      announceAddress = (union sockaddr_union*)tagListGetData(tags, TAG_RspLib_RegistrarAnnounceAddress,
                                                                 (tagdata_t)&defaultAnnounceAddress);
      memcpy(&registrarTable->AnnounceAddress, announceAddress, sizeof(registrarTable->AnnounceAddress));


      /* ====== Show results ================================================ */
      LOG_VERBOSE3
      fputs("New RegistrarTable's configuration:\n",  stdlog);
      fprintf(stdlog, "registrar.announce.timeout  = %llu [us]\n",  registrarTable->RegistrarAnnounceTimeout);
      fputs("registrar.announce.address  = ",  stdlog);
      fputaddress((struct sockaddr*)&registrarTable->AnnounceAddress, true, stdlog);
      fputs("\n",  stdlog);
      fprintf(stdlog, "registrar.connect.maxtrials = %u\n",        registrarTable->RegistrarConnectMaxTrials);
      fprintf(stdlog, "registrar.connect.timeout   = %llu [us]\n",  registrarTable->RegistrarConnectTimeout);
      LOG_END


      /* ====== Join announce multicast groups ============================ */
      registrarTable->AnnounceSocket = ext_socket(registrarTable->AnnounceAddress.sa.sa_family,
                                                  SOCK_DGRAM, IPPROTO_UDP);
      if(registrarTable->AnnounceSocket >= 0) {
         fdCallbackNew(&registrarTable->AnnounceSocketFDCallback,
                     registrarTable->Dispatcher,
                     registrarTable->AnnounceSocket,
                     FDCE_Read,
                     registrarAnnouceFDCallback,
                     registrarTable);

         setReusable(registrarTable->AnnounceSocket, 1);
         if(bindplus(registrarTable->AnnounceSocket,
                     &registrarTable->AnnounceAddress,
                     1) == false) {
            LOG_ERROR
            fputs("Unable to bind multicast socket to address ",  stdlog);
            fputaddress((struct sockaddr*)&registrarTable->AnnounceAddress.sa, true, stdlog);
            fputs("\n",  stdlog);
            LOG_END
            registrarTableDelete(registrarTable);
            return(false);
         }
      }
      else {
         LOG_ERROR
         fputs("Creating a socket for registrar announces failed\n",  stdlog);
         LOG_END
         registrarTableDelete(registrarTable);
         return(false);
      }
   }
   return(registrarTable);
}


/* ###### Destructor ##################################################### */
void registrarTableDelete(struct RegistrarTable* registrarTable)
{
   if(registrarTable != NULL) {
      if(registrarTable->AnnounceSocket >= 0) {
         fdCallbackDelete(&registrarTable->AnnounceSocketFDCallback);
         ext_close(registrarTable->AnnounceSocket);
         registrarTable->AnnounceSocket = -1;
      }
      ST_CLASS(peerListManagementClear)(&registrarTable->RegistrarList);
      ST_CLASS(peerListManagementDelete)(&registrarTable->RegistrarList);
      free(registrarTable);
   }
}


/* ###### Add static registrar entry ################################### */
unsigned int registrarTableAddStaticEntry(struct RegistrarTable* registrarTable,
                                          union sockaddr_union*  addressArray,
                                          size_t                 addresses)
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
               &registrarTable->RegistrarList,
               0,
               PLNF_STATIC,
               transportAddressBlock,
               getMicroTime(),
               &peerListNode);
   if(result == RSPERR_OKAY) {
      LOG_VERBOSE
      fputs("Added static entry to registrar table: ", stdlog);
      ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
      fputs("\n", stdlog);
      LOG_END
   }
   else {
      LOG_WARNING
      fputs("Unable to add static entry to registrar table: ", stdlog);
      transportAddressBlockPrint(transportAddressBlock, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }
   return(result);
}


/* ###### Try more registrars ########################################### */
static void tryNextBlock(struct RegistrarTable*         registrarTable,
                         int                            registrarFD,
                         RegistrarIdentifierType*       lastRegistrarIdentifier,
                         struct TransportAddressBlock** lastTransportAddressBlock)
{
   struct TransportAddressBlock*  transportAddressBlock;
   struct ST_CLASS(PeerListNode)* peerListNode;
   int                            result;
   size_t                         i;

   i = ST_CLASS(peerListManagementPurgeExpiredPeerListNodes)(
         &registrarTable->RegistrarList,
         getMicroTime());
   LOG_VERBOSE2
   fprintf(stdlog, "Purged %u out-of-date peer list nodes. Peer List:\n",  (unsigned int)i);
   ST_CLASS(peerListManagementPrint)(&registrarTable->RegistrarList, stdlog, PLPO_PEERS_INDEX|PLNPO_TRANSPORT);
   LOG_END

   for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
      peerListNode = ST_CLASS(peerListManagementFindNearestNextPeerListNode)(
                        &registrarTable->RegistrarList,
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
            fputs("...\n",  stdlog);
            LOG_END

            result = connectplus(registrarFD,
                                 transportAddressBlock->AddressArray,
                                 transportAddressBlock->Addresses);
            if((result < 0) &&
               (errno != EINPROGRESS) && (errno != EISCONN)) {
               LOG_WARNING
               fputs("Connection to registrar ",  stdlog);
               transportAddressBlockPrint(transportAddressBlock, stdlog);
               fputs(" failed\n",  stdlog);
               LOG_END
            }
            else {
               registrarTable->OutstandingConnects++;
            }
         }
      }
      else {
         if(*lastTransportAddressBlock) {
            free(*lastTransportAddressBlock);
         }
         *lastTransportAddressBlock = NULL;
         *lastRegistrarIdentifier   = UNDEFINED_REGISTRAR_IDENTIFIER;
      }
   }
}


/* ###### Find registrar ################################################### */
sctp_assoc_t registrarTableGetRegistrar(struct RegistrarTable*   registrarTable,
                                        int                      registrarFD,
                                        RegistrarIdentifierType* registrarIdentifier)
{
   struct timeval                 selectTimeout;
   union sockaddr_union*          peerAddressArray;
   union sctp_notification        notification;
   RegistrarIdentifierType        lastRegistrarIdentifier;
   struct TransportAddressBlock*  lastTransportAddressBlock;
   struct ST_CLASS(PeerListNode)* peerListNode;
   sctp_assoc_t                   assocID;
   ssize_t                        received;
   unsigned long long             start;
   unsigned long long             nextTimeout;
   unsigned long long             lastTrialTimeStamp;
   fd_set                         rfdset;
   unsigned int                   trials;
   int                            n, result;
   int                            flags;

   *registrarIdentifier = 0;
   if(registrarTable == NULL) {
      return(0);
   }


   LOG_ACTION
   fputs("Looking for registrar...\n",  stdlog);
   LOG_END

   trials                    = 0;
   lastRegistrarIdentifier   = 0;
   lastTransportAddressBlock = NULL;
   start                     = 0;

   registrarTable->OutstandingConnects = 0;

   peerListNode = ST_CLASS(peerListGetRandomPeerNode)(&registrarTable->RegistrarList.List);
   if(peerListNode) {
      lastRegistrarIdentifier   = peerListNode->Identifier;
      lastTransportAddressBlock = transportAddressBlockDuplicate(peerListNode->AddressBlock);
      LOG_VERBOSE
      fputs("Randomized registrar hunt start: ", stdlog);
      ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
      fputs("\n", stdlog);
      LOG_END
   }

   lastTrialTimeStamp = 0;
   for(;;) {
      if((lastRegistrarIdentifier == 0) && (lastTransportAddressBlock == NULL)) {
         /*
            Start new trial, when
            - First time
            - A new registrar announce has been added to the list AND
                 there are no outstanding connects
            - The current trial has been running for at least
               registrarTable->RegistrarConnectTimeout
         */
         if( (start == 0) ||
             ((registrarTable->LastAnnounceHeard >= start) && (registrarTable->OutstandingConnects == 0)) ||
             (start + registrarTable->RegistrarConnectTimeout < getMicroTime()) ) {
            trials++;
            if(trials > registrarTable->RegistrarConnectMaxTrials) {
               LOG_ACTION
               fputs("Registrar hunt procedure did not find any registrar!\n", stdlog);
               LOG_END
               if(lastTransportAddressBlock) {
                  transportAddressBlockDelete(lastTransportAddressBlock);
                  free(lastTransportAddressBlock);
               }
               return(0);
            }

            lastTrialTimeStamp = 0;
            start = getMicroTime();
            LOG_ACTION
            fprintf(stdlog, "Trial #%u...\n", trials);
            LOG_END

            if(!joinOrLeaveMulticastGroup(registrarTable->AnnounceSocket,
                                          &registrarTable->AnnounceAddress,
                                          true)) {
               LOG_WARNING
               fputs("Joining multicast group ",  stdlog);
               fputaddress(&registrarTable->AnnounceAddress.sa, true, stdlog);
               fputs(" failed. Check routing (is default route set?) and firewall settings!\n",  stdlog);
               LOG_END
            }
         }
      }

      if(lastTrialTimeStamp + registrarTable->RegistrarConnectTimeout < getMicroTime()) {
         /* Try next block of addresses */
         tryNextBlock(registrarTable,
                      registrarFD,
                      &lastRegistrarIdentifier,
                      &lastTransportAddressBlock);
         lastTrialTimeStamp = getMicroTime();
      }

      /* Wait for event */
      FD_ZERO(&rfdset);
      FD_SET(registrarFD, &rfdset);
      n = registrarFD;
      if(registrarTable->AnnounceSocket >= 0) {
         FD_SET(registrarTable->AnnounceSocket, &rfdset);
         n = max(n, registrarTable->AnnounceSocket);
      }
      nextTimeout = registrarTable->RegistrarConnectTimeout;
      selectTimeout.tv_sec  = nextTimeout / (unsigned long long)1000000;
      selectTimeout.tv_usec = nextTimeout % (unsigned long long)1000000;

      LOG_VERBOSE3
      fprintf(stdlog, "select() with timeout %lld\n", nextTimeout);
      LOG_END
      /*
         Important: rspSelect() may *not* be used here!
         registrarTableFindRegistrar() may be called from within a
         session timer + from within registrar socket callback
         => Strange things will happen when these callbacks are
            invoked recursively!
      */
      result = ext_select(n + 1,
                          &rfdset, NULL, NULL,
                          &selectTimeout);
      LOG_VERBOSE3
      fprintf(stdlog, "select() result=%d\n", result);
      LOG_END

      if((result < 0) && (errno == EINTR)) {
         LOG_ACTION
         fputs("Interrupted select() call -> returning immediately!\n",  stdlog);
         LOG_END
         if(lastTransportAddressBlock) {
            transportAddressBlockDelete(lastTransportAddressBlock);
            free(lastTransportAddressBlock);
         }
         return(0);
      }

      /* Handle events */
      if(result != 0) {
         if(FD_ISSET(registrarTable->AnnounceSocket, &rfdset)) {
            handleRegistrarAnnounceCallback(registrarTable, registrarTable->AnnounceSocket);
         }
         if(FD_ISSET(registrarFD, &rfdset)) {
            flags    = MSG_PEEK;
            received = recvfromplus(registrarFD,
                                    &notification, sizeof(notification), &flags,
                                    NULL, 0,
                                    NULL, &assocID, NULL, 0);
            if(received > 0) {
               if(flags & MSG_NOTIFICATION) {
                  /* ====== Notification received ======================== */
                  flags = 0;
                  if( (recvfromplus(registrarFD,
                                    &notification, sizeof(notification), &flags,
                                    NULL, 0,
                                    NULL, NULL, NULL, 0) > 0) &&
                     (flags & MSG_NOTIFICATION) ) {
                     /* ====== Association change notification =========== */
                     if(notification.sn_header.sn_type == SCTP_ASSOC_CHANGE) {
                        if(notification.sn_assoc_change.sac_state == SCTP_COMM_UP) {
                           n = getpaddrsplus(registrarFD,
                                             notification.sn_assoc_change.sac_assoc_id,
                                             &peerAddressArray);
                           if(n > 0) {
                              LOG_ACTION
                              fputs("Successfully found registrar at ",  stdlog);
                              fputaddress((struct sockaddr*)&peerAddressArray[0], true, stdlog);
                              fputs("\n", stdlog);
                              LOG_END

                              /* ====== Get registrar's entry =============== */
                              struct TransportAddressBlock* registrarAddressBlock = (struct TransportAddressBlock*)malloc(transportAddressBlockGetSize(n));
                              if(registrarAddressBlock) {
                                 transportAddressBlockNew(registrarAddressBlock,
                                                          IPPROTO_SCTP,
                                                          getPort(&peerAddressArray[0].sa),
                                                          0,
                                                          peerAddressArray, n);
                                 peerListNode = ST_CLASS(peerListManagementFindPeerListNode)(
                                                   &registrarTable->RegistrarList,
                                                   0,
                                                   registrarAddressBlock);
                                 if(peerListNode) {
                                    *registrarIdentifier = peerListNode->Identifier;
                                 }
                                 else {
                                    *registrarIdentifier = UNDEFINED_REGISTRAR_IDENTIFIER;
                                 }

                                 free(registrarAddressBlock);
                              }

                              if(lastTransportAddressBlock) {
                                 transportAddressBlockDelete(lastTransportAddressBlock);
                                 free(lastTransportAddressBlock);
                              }
                              free(peerAddressArray);
                           }

                           return(notification.sn_assoc_change.sac_assoc_id);
                        }
                     }
                     else if((notification.sn_assoc_change.sac_state == SCTP_COMM_LOST) ||
                             (notification.sn_assoc_change.sac_state == SCTP_CANT_STR_ASSOC) ||
                             (notification.sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP)) {
                        if(registrarTable->OutstandingConnects > 0) {
                           registrarTable->OutstandingConnects--;
                        }
                        LOG_VERBOSE2
                        fprintf(stdlog, "Failed to establish registrar connection, outstanding=%d\n", registrarTable->OutstandingConnects);
                        LOG_END
                     }
                  }
                  else {
                     LOG_ERROR
                     fputs("Peeked notification but failed to read it\n", stdlog);
                     LOG_END_FATAL
                  }
               }
               else {
                  LOG_WARNING
                  fprintf(stdlog, "Received data on assoc %u -> hopefully this is a registrar",
                          (unsigned int)assocID);
                  LOG_END
                  *registrarIdentifier = UNDEFINED_REGISTRAR_IDENTIFIER;
                  if(lastTransportAddressBlock) {
                     transportAddressBlockDelete(lastTransportAddressBlock);
                     free(lastTransportAddressBlock);
                  }
                  return(assocID);
               }
            }
         }
      }
   }
}
