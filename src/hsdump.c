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
 * Copyright (C) 2002-2021 by Thomas Dreibholz
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

#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "timer.h"

#include "rserpoolmessage.h"
#include "poolhandlespacemanagement.h"
#include "randomizer.h"

#include <ext_socket.h>
#include <net/if.h>
#include <sys/ioctl.h>


#define MAX_NS_TRANSPORTADDRESSES 128

int main(int argc, char** argv)
{
   char                                       localAddressArrayBuffer[transportAddressBlockGetSize(MAX_NS_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*              localAddressArray = (struct TransportAddressBlock*)&localAddressArrayBuffer;
   struct sctp_event_subscribe                sctpEvents;
   struct ST_CLASS(PoolHandlespaceManagement) handlespace;
   struct ST_CLASS(PeerListManagement)        peerList;
   struct ST_CLASS(PoolElementNode)*          poolElementNode;
   struct ST_CLASS(PoolElementNode)*          newPoolElementNode;
   struct ST_CLASS(PeerListNode)*             peerListNodePtr;
   struct ST_CLASS(PeerListNode)*             newPeerListNode;
   unsigned int                               result;
   union sockaddr_union                       registrarAddress;
   struct RSerPoolMessage*                    message;
   uint32_t                                   registrarID;
   int                                        sd;
   char                                       buffer[65536];
   int                                        flags;
   uint32_t                                   ppid;
   sctp_assoc_t                               assocID;
   uint16_t                                   streamID;
   ssize_t                                    received;
   struct ST_CLASS(PeerListNode)              peerListNode;
   bool                                       moreData;
   int                                        i;

   if(argc < 2) {
      fprintf(stderr, "Usage: %s [Registrar] {-loglevel=Level} {-logfile=File} {-logappend=File} {-logcolor=on|off}\n", argv[0]);
      exit(1);
   }

   if(string2address(argv[1], &registrarAddress) == false) {
      fprintf(stderr, "ERROR: Bad registrar address <%s>\n", argv[1]);
      exit(1);
   }
   for(i = 2;i < argc;i++) {
      if(!(strncmp(argv[i], "-log" ,4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else {
         fprintf(stderr, "ERROR: Bad argument <%s>\n", argv[i]);
      }
   }
   beginLogging();

   sd = ext_socket((checkIPv6() == true) ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_SCTP);
   if(sd < 0) {
      perror("Unable to create SCTP socket");
      exit(1);
   }
   memset(&sctpEvents, 0, sizeof(sctpEvents));
   sctpEvents.sctp_data_io_event = 1;
   if(ext_setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
      perror("setsockopt() for SCTP_EVENTS failed");
      ext_close(sd);
      exit(1);
   }
   if(ext_connect(sd, (struct sockaddr*)&registrarAddress, sizeof(registrarAddress)) < 0) {
      perror("Unable to connect to registrar");
      ext_close(sd);
      exit(1);
   }

   message = rserpoolMessageNew(NULL, 65536);
   if(message == NULL) {
      fputs("ERROR: Unable to create message structure!\n", stderr);
      ext_close(sd);
      exit(1);
   }

   registrarID = random32();
   ST_CLASS(poolHandlespaceManagementNew)(&handlespace,
                                          registrarID,
                                          NULL, NULL, NULL);
   ST_CLASS(peerListManagementNew)(&peerList, &handlespace, registrarID, NULL, NULL);

   if(transportAddressBlockGetAddressesFromSCTPSocket(localAddressArray,
                                                      sd,
                                                      0,
                                                      MAX_NS_TRANSPORTADDRESSES,
                                                      true) <= 0) {
      fputs("ERROR: Unable to obtain local address(es)!\n", stderr);
      rserpoolMessageDelete(message);
      ext_close(sd);
      exit(1);
   }
   ST_CLASS(peerListNodeNew)(&peerListNode, registrarID, 0, localAddressArray);

   moreData = false;
   do {
      message->Type                      = EHT_PRESENCE;
      message->PPID                      = PPID_ENRP;
      message->AssocID                   = 0;
      message->AddressArray              = NULL;
      message->Addresses                 = 0;
      message->Flags                     = 0x00;
      message->PeerListNodePtr           = &peerListNode;
      message->PeerListNodePtrAutoDelete = false;
      message->SenderID                  = registrarID;
      message->ReceiverID                = 0;
      message->Checksum                  = ST_CLASS(poolHandlespaceManagementGetOwnershipChecksum)(&handlespace);
      if(rserpoolMessageSend(IPPROTO_SCTP,
                              sd, 0, 0, 0, 0, message) == false) {
         fputs("ERROR: Sending Presence failed\n", stderr);
         rserpoolMessageDelete(message);
         ext_close(sd);
         exit(1);
      }

      message->Type         = EHT_LIST_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = 0;
      message->AddressArray = 0;
      message->Addresses    = 0;
      message->Flags        = 0x00;
      message->SenderID     = registrarID;
      message->ReceiverID   = UNDEFINED_REGISTRAR_IDENTIFIER;
      if(rserpoolMessageSend(IPPROTO_SCTP,
                              sd, 0, 0, 0, 0, message) == false) {
         fputs("ERROR: Sending ListRequest failed\n", stderr);
         rserpoolMessageDelete(message);
         ext_close(sd);
         exit(1);
      }

      message->Type         = EHT_HANDLE_TABLE_REQUEST;
      message->PPID         = PPID_ENRP;
      message->AssocID      = 0;
      message->AddressArray = 0;
      message->Addresses    = 0;
      message->Flags        = 0x00;
      message->SenderID     = registrarID;
      message->ReceiverID   = UNDEFINED_REGISTRAR_IDENTIFIER;
      if(rserpoolMessageSend(IPPROTO_SCTP,
                              sd, 0, 0, 0, 0, message) == false) {
         fputs("ERROR: Sending HandleTableRequest failed\n", stderr);
         rserpoolMessageDelete(message);
         ext_close(sd);
         exit(1);
      }

      for(;;) {
         flags = 0;
         received = recvfromplus(sd,
                                 (char*)&buffer, sizeof(buffer), &flags,
                                 NULL, NULL,
                                 &ppid, &assocID, &streamID,
                                 180000000);
         if(received > 0) {
            rserpoolMessageClearBuffer(message);
            if(rserpoolPacket2Message(buffer, NULL, assocID, ppid,
                                      received, sizeof(buffer), &message) == RSPERR_OKAY) {
               if(message->PPID == PPID_ENRP) {
                  if(message->Type == EHT_LIST_RESPONSE) {
                     if(!(message->Flags & EHF_LIST_RESPONSE_REJECT)) {
                        if(message->PeerListPtr) {
                           peerListNodePtr = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                                             &message->PeerListPtr->List);
                           while(peerListNodePtr) {
                              if(peerListNodePtr->Identifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
                                 result = ST_CLASS(peerListManagementRegisterPeerListNode)(
                                             &peerList,
                                             peerListNodePtr->Identifier,
                                             peerListNodePtr->Flags,
                                             peerListNodePtr->AddressBlock,
                                             getMicroTime(),
                                             &newPeerListNode);
                                 if(result != RSPERR_OKAY) {
                                    fputs("Failed to add peer ", stderr);
                                    ST_CLASS(peerListNodePrint)(peerListNodePtr, stderr, ~0);
                                    fputs(" to peer list\n", stderr);
                                 }
                              }
                              peerListNodePtr = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                                                   &message->PeerListPtr->List, peerListNodePtr);
                           }
                        }
                        else {
                           puts("--- No peers ---");
                        }
                     }
                     else {
                        puts("--- ListRequest has been rejected ---");
                     }
                  }
                  else if(message->Type == EHT_HANDLE_TABLE_RESPONSE) {
                     if(!(message->Flags & EHF_HANDLE_TABLE_RESPONSE_REJECT)) {
                        if(message->HandlespacePtr) {
                           poolElementNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(&message->HandlespacePtr->Handlespace);
                           while(poolElementNode != NULL) {
                              result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                                          &handlespace,
                                          &poolElementNode->OwnerPoolNode->Handle,
                                          poolElementNode->HomeRegistrarIdentifier,
                                          poolElementNode->Identifier,
                                          poolElementNode->RegistrationLife,
                                          &poolElementNode->PolicySettings,
                                          poolElementNode->UserTransport,
                                          poolElementNode->RegistratorTransport,
                                          -1, 0,
                                          0,
                                          &newPoolElementNode);
                              if(result != RSPERR_OKAY) {
                                 fputs("Failed to register to pool ", stderr);
                                 poolHandlePrint(&message->Handle, stderr);
                                 fputs(" pool element ", stderr);
                                 ST_CLASS(poolElementNodePrint)(poolElementNode, stderr, PENPO_FULL);
                                 fputs(": ", stderr);
                                 rserpoolErrorPrint(result, stderr);
                                 fputs("\n", stderr);
                              }
                              poolElementNode = ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(&message->HandlespacePtr->Handlespace, poolElementNode);
                           }

                           moreData = (message->Flags & EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND);
                           printf("Got %u pools, %u PEs => now having %u pools, %u PEs\n",
                              (unsigned int)ST_CLASS(poolHandlespaceManagementGetPools)(message->HandlespacePtr),
                              (unsigned int)ST_CLASS(poolHandlespaceManagementGetPoolElements)(message->HandlespacePtr),
                              (unsigned int)ST_CLASS(poolHandlespaceManagementGetPools)(&handlespace),
                              (unsigned int)ST_CLASS(poolHandlespaceManagementGetPoolElements)(&handlespace));
                        }
                        else {
                           puts("--- No PEs ---");
                        }
                     }
                     else {
                        puts("--- HandleTableRequest has been rejected ---");
                     }
                     break;
                  }
                  else if(message->Type == EHT_PRESENCE) {
                  }
                  else {
                     // fprintf(stderr, "ERROR: Got unexpected ENRP message $%x as response!\n", message->Type);
                  }
               }
               else {
                  fputs("ERROR: Got invalid response!\n", stderr);
                  rserpoolMessageDelete(message);
                  ext_close(sd);
                  exit(1);
               }
            }
         }
         else {
            if(received == 0) {
               fputs("ERROR: Connection closed by peer!\n", stderr);
            }
            else {
               perror("Unable to receive response");
            }
            rserpoolMessageDelete(message);
            ext_close(sd);
            exit(1);
         }
      }
   } while(moreData);

   ST_CLASS(peerListManagementPrint)(&peerList, stdout, ~0);
   ST_CLASS(poolHandlespaceManagementPrint)(&handlespace, stdout, PNNPO_POOLS_INDEX|PNNPO_POOLS_SELECTION|PNNPO_POOLS_OWNERSHIP|PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_HOME_PR|PENPO_REGLIFE|PENPO_UR_REPORTS|PENPO_USERTRANSPORT|PENPO_REGISTRATORTRANSPORT|PENPO_CHECKSUM);

   rserpoolMessageDelete(message);
   ST_CLASS(peerListManagementDelete)(&peerList);
   ST_CLASS(poolHandlespaceManagementDelete)(&handlespace);
   ext_close(sd);
   finishLogging();
   return(0);
}
