/*
 *  $Id: servertable.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
#include "utilities.h"
#include "netutilities.h"
#include "asapcreator.h"
#include "asapparser.h"
#include "asapinstance.h"
#include "rsplib-tags.h"

#include <ext_socket.h>
#include <sys/time.h>



/* Maximum number of simultaneos nameserver connection trials */
#define MAX_SIMULTANEOUS_REQUESTS 3



/* ###### Add nameserver announcement source ################################ */
static bool addAnnounceSource(int         sockFD,
                              const char* address)
{
   struct sockaddr_storage multicastAddress;
   struct sockaddr_storage localAddress;
   int                     on;

   if(!string2address(address,&multicastAddress)) {
      return(false);
   }
   memset((char*)&localAddress,0,sizeof(localAddress));
   ((struct sockaddr*)&localAddress)->sa_family = ((struct sockaddr*)&multicastAddress)->sa_family;
   switch(((struct sockaddr*)&multicastAddress)->sa_family) {
      case AF_INET:
         ((struct sockaddr_in*)&localAddress)->sin_port = ((struct sockaddr_in*)&multicastAddress)->sin_port;
       break;
#ifdef HAVE_IPV6
      case AF_INET6:
         ((struct sockaddr_in6*)&localAddress)->sin6_port = ((struct sockaddr_in6*)&multicastAddress)->sin6_port;
       break;
#endif
      default:
         LOG_ERROR
         fprintf(stdlog,"Multicast address %s must be IPv4 or IPv6\n",address);
         LOG_END
       break;
   }

   on = 1;
   if(ext_setsockopt(sockFD,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) != 0) {
      return(false);
   }
#if !defined (LINUX)
   if(ext_setsockopt(sockFD,SOL_SOCKET,SO_REUSEPORT,&on,sizeof(on)) != 0) {
      return(false);
   }
#endif
   if(ext_bind(sockFD,(struct sockaddr*)&localAddress,getSocklen((struct sockaddr*)&localAddress)) != 0) {
      LOG_ERROR
      fprintf(stdlog,"Unable to bind multicast socket to address %s\n",address);
      LOG_END
      return(false);
   }
   if(multicastGroupMgt(sockFD,&multicastAddress,NULL,true) == false) {
      return(false);
   }
   return(true);
}


/* ###### Add nameserver to list ############################################ */
static void addServer(struct ServerTable* serverTable,
                      GList*              transportAddressList,
                      const unsigned int  flags)
{
   struct ServerAnnounce* serverAnnounce;
   GList*                 found;

   serverAnnounce = serverAnnounceNew(transportAddressList,flags);
   if(serverAnnounce) {
      LOG_VERBOSE5
      fputs("Try adding ServerAnnounce:\n",stdlog);
      serverAnnouncePrint(serverAnnounce,stdlog);
      LOG_END

      found = g_list_find_custom(serverTable->NameServerAnnounceList,
                                 serverAnnounce,
                                 serverAnnounceCompareFunc);
      if(found) {
         serverAnnounceDelete(serverAnnounce);
         serverAnnounce = (struct ServerAnnounce*)found->data;
         LOG_VERBOSE3
         fputs("ServerAnnounce already in list -> updating timestamp\n",stdlog);
         LOG_END
         serverAnnounce->LastUpdate = getMicroTime();
      }
      else {
         serverTable->NameServerAnnounceList = g_list_prepend(serverTable->NameServerAnnounceList,
                                                          serverAnnounce);
         serverTable->NameServerAnnounceListAddition = getMicroTime();
         LOG_VERBOSE2
         fputs("New ServerAnnounce successfully added:\n",stdlog);
         serverAnnouncePrint(serverAnnounce,stdlog);
         LOG_END
      }
   }
   else {
      transportAddressListDelete(transportAddressList);
   }
}


/* ###### Receive server announce ########################################### */
static void receiveServerAnnounce(struct ServerTable* serverTable,
                           int                 sd)
{
   struct ASAPMessage*      message;
   struct TransportAddress* transportAddress;
   GList*                   transportAddressList;
   struct sockaddr_storage  senderAddress;
   socklen_t                senderAddressLength;
   char                     buffer[1024];
   ssize_t                  received;

   LOG_VERBOSE2
   fprintf(stdlog,"Reading response from socket %d...\n",sd);
   LOG_END

   senderAddressLength = sizeof(senderAddress);
   received = ext_recvfrom(sd,
                           (char*)&buffer ,sizeof(buffer), 0,
                           (struct sockaddr*)&senderAddress,
                           &senderAddressLength);
   if(received > 0) {
      message = asapPacket2Message((char*)&buffer,received,sizeof(buffer));
      if(message != NULL) {
         if(message->Type == AHT_SERVER_ANNOUNCE) {
            if(message->Error == 0) {

               /* For TCP socket: Try to get peer address if not specified. */
               if(senderAddressLength == 0) {
                  senderAddressLength = sizeof(senderAddress);
                  ext_getpeername(sd,(struct sockaddr*)&senderAddress,
                                  &senderAddressLength);
               }

               LOG_VERBOSE
               fputs("ServerAnnounce from ",stdlog);
               address2string((struct sockaddr*)&senderAddress,
                              (char*)&buffer, sizeof(buffer), true);
               fputs(buffer,stdlog);
               fputs(" received\n",stdlog);
               LOG_END

               if(message->TransportAddressListPtr) {
                  message->TransportAddressListPtrAutoDelete = false;
                  transportAddressList = message->TransportAddressListPtr;
               }
               else {
                  transportAddressList = NULL;
                  transportAddress = transportAddressNew(IPPROTO_SCTP,
                                                         PORT_ASAP,
                                                         (struct sockaddr_storage*)&senderAddress,
                                                         1);
                  if(transportAddress != NULL) {
                     transportAddressList = g_list_append(transportAddressList,transportAddress);
                  }
               }

               addServer(serverTable,transportAddressList,SIF_DYNAMIC);
            }
         }
         else {
            LOG_WARNING
            fprintf(stdlog,"Unsupported message type $%02x!\n",message->Type);
            LOG_END
         }
         asapMessageDelete(message);
      }
   }
}


/* ###### Server announce callback  ######################################### */
static void receiveServerAnnounceCallback(struct Dispatcher* dispatcher,
                                          int                sd,
                                          int                eventMask,
                                          void*              userData)
{
   struct ServerTable* serverTable = (struct ServerTable*)userData;
   receiveServerAnnounce(serverTable,sd);
}


/* ###### Server maintenance timer ########################################## */
static void serverMaintenanceTimer(struct Dispatcher* dispatcher,
                                       struct Timer*      timer,
                                       void*              userData)
{
   struct ServerTable*    serverTable = (struct ServerTable*)userData;
   struct ServerAnnounce* serverAnnounce;
   GList*                 list;

   dispatcherLock(dispatcher);

   LOG_VERBOSE3
   fputs("Starting ServerAnnounce list maintenance\n",stdlog);
   LOG_END

   list = g_list_first(serverTable->NameServerAnnounceList);
   while(list != NULL) {
      serverAnnounce = (struct ServerAnnounce*)list->data;
      if((serverAnnounce->Flags & SIF_DYNAMIC) &&
         (serverAnnounce->LastUpdate + serverTable->NameServerAnnounceTimeout < getMicroTime())) {
         LOG_VERBOSE2
         fputs("Timeout for this entry:\n",stdlog);
         serverAnnouncePrint(serverAnnounce,stdlog);
         LOG_END
         serverTable->NameServerAnnounceList = g_list_remove(serverTable->NameServerAnnounceList,
                                                         serverAnnounce);
         serverAnnounceDelete(serverAnnounce);
         list = g_list_first(serverTable->NameServerAnnounceList);
      }
      else {
         list = g_list_next(list);
      }
   }

   timerRestart(timer,serverTable->NameServerAnnounceMaintenanceInterval);

   LOG_VERBOSE3
   fputs("Finished ServerAnnounce list maintenance\n",stdlog);
   LOG_END

   dispatcherUnlock(dispatcher);
}


/* ###### Constructor ####################################################### */
struct ServerTable* serverTableNew(struct Dispatcher* dispatcher,
                                   struct TagItem*    tags)
{
   const char*         announceAddress;
   struct ServerTable* serverTable = (struct ServerTable*)malloc(sizeof(struct ServerTable));
   if(serverTable != NULL) {
      serverTable->Dispatcher = dispatcher;

      /* ====== ASAP Instance settings ==================================== */
      serverTable->NameServerConnectMaxTrials = tagListGetData(tags, TAG_RspLib_NameServerConnectMaxTrials,
                                                           ASAP_DEFAULT_NAMESERVER_CONNECT_MAXTRIALS);
      serverTable->NameServerConnectTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerConnectTimeout,
                                                                 ASAP_DEFAULT_NAMESERVER_CONNECT_TIMEOUT);
      serverTable->NameServerAnnounceTimeout = (card64)tagListGetData(tags, TAG_RspLib_NameServerAnnounceTimeout,
                                                                  ASAP_DEFAULT_NAMESERVER_ANNOUNCE_TIMEOUT);
      serverTable->NameServerAnnounceMaintenanceInterval = (card64)tagListGetData(tags, TAG_RspLib_NameServerAnnounceMaintenanceInterval,
                                                                              ASAP_DEFAULT_NAMESERVER_ANNOUNCE_MAINTENANCE_INTERVAL);

      /* ====== Show results ================================================ */
      LOG_VERBOSE3
      fputs("New ServerTable's configuration:\n", stdlog);
      fprintf(stdlog, "nameserver.connect.maxtrials   = %u\n",       serverTable->NameServerConnectMaxTrials);
      fprintf(stdlog, "nameserver.connect.timeout     = %Lu [µs]\n", serverTable->NameServerConnectTimeout);
      fprintf(stdlog, "nameserver.announce.timeout    = %Lu [µs]\n", serverTable->NameServerAnnounceTimeout);
      fprintf(stdlog, "nameserver.announce.mtinterval = %Lu\n",       serverTable->NameServerAnnounceMaintenanceInterval);
      LOG_END


      /* ====== Join announce multicast groups ============================ */
      serverTable->NameServerAnnounceList         = NULL;
      serverTable->NameServerAnnounceListAddition = 0;
      serverTable->NameServerIPv4AnnounceSocket   = ext_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if(serverTable->NameServerIPv4AnnounceSocket >= 0) {
         dispatcherAddFDCallback(serverTable->Dispatcher,
                                 serverTable->NameServerIPv4AnnounceSocket,
                                 FDCE_Read,
                                 receiveServerAnnounceCallback,
                                 (void*)serverTable);
         announceAddress = (const char*)tagListGetData(tags, TAG_RspLib_NameServerAnnounceAddressIPv4,
                                                       (tagdata_t)ASAP_DEFAULT_NAMESERVER_ANNOUNCE_ADDRESS_IPv4);
         if(!addAnnounceSource(serverTable->NameServerIPv4AnnounceSocket,
                               announceAddress)) {
            LOG_ERROR
            fprintf(stdlog,"Unable to set up IPv4 multicast announce socket for %s\n",
                    announceAddress);
            LOG_END
         }
      }
#ifdef HAVE_IPV6
      serverTable->NameServerIPv6AnnounceSocket = ext_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
      if(serverTable->NameServerIPv6AnnounceSocket >= 0) {
         dispatcherAddFDCallback(serverTable->Dispatcher,
                                 serverTable->NameServerIPv6AnnounceSocket,
                                 FDCE_Read,
                                 receiveServerAnnounceCallback,
                                 (void*)serverTable);
         announceAddress = (const char*)tagListGetData(tags, TAG_RspLib_NameServerAnnounceAddressIPv6,
                                                       (tagdata_t)ASAP_DEFAULT_NAMESERVER_ANNOUNCE_ADDRESS_IPv6);
         if(!addAnnounceSource(serverTable->NameServerIPv6AnnounceSocket,
                               announceAddress)) {
            LOG_ERROR
            fprintf(stdlog,"Unable to set up IPv6 multicast announce socket for %s\n",
                    announceAddress);
            LOG_END
         }
      }
#endif
      serverTable->NameServerAnnounceMaintenanceTimer = timerNew(serverTable->Dispatcher,
                                                                 serverMaintenanceTimer,
                                                                 (void*)serverTable);
      if(serverTable->NameServerAnnounceMaintenanceTimer == NULL) {
         serverTableDelete(serverTable);
         return(NULL);
      }
      timerStart(serverTable->NameServerAnnounceMaintenanceTimer,
                 serverTable->NameServerAnnounceMaintenanceInterval);
   }
   return(serverTable);
}


/* ###### Destructor ######################################################## */
void serverTableDelete(struct ServerTable* serverTable)
{
   struct ServerAnnounce* serverAnnounce;
   GList*                 list;

   if(serverTable != NULL) {
      if(serverTable->NameServerAnnounceMaintenanceTimer) {
         timerDelete(serverTable->NameServerAnnounceMaintenanceTimer);
      }
      while(serverTable->NameServerAnnounceList != NULL) {
         list = g_list_first(serverTable->NameServerAnnounceList);
         serverAnnounce = (struct ServerAnnounce*)list->data;
         serverAnnounceDelete(serverAnnounce);
         serverTable->NameServerAnnounceList = g_list_remove(serverTable->NameServerAnnounceList,serverAnnounce);
      }
      if(serverTable->NameServerIPv4AnnounceSocket >= 0) {
         dispatcherRemoveFDCallback(serverTable->Dispatcher,
                                    serverTable->NameServerIPv4AnnounceSocket);
         ext_close(serverTable->NameServerIPv4AnnounceSocket);
         serverTable->NameServerIPv4AnnounceSocket = -1;
      }
#ifdef HAVE_IPV6
      if(serverTable->NameServerIPv6AnnounceSocket >= 0) {
         dispatcherRemoveFDCallback(serverTable->Dispatcher,
                                    serverTable->NameServerIPv6AnnounceSocket);
         ext_close(serverTable->NameServerIPv6AnnounceSocket);
         serverTable->NameServerIPv6AnnounceSocket = -1;
      }
#endif
      free(serverTable);
   }
}


/* ###### Try more nameservers ############################################## */
static void tryNextBlock(struct ServerTable* serverTable,
                         GList**             serverAnnounceList,
                         GList**             serverTransportAddressList,
                         int*                sd,
                         card64*             timeout,
                         const int           protocol)
{
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
                     fprintf(stdlog,"Connecting socket %d to ",sd[i]);
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
}


/* ###### Find nameserver ################################################### */
unsigned int serverTableFindServer(struct ServerTable* serverTable,
                                   const int           protocol)
{
   struct timeval          selectTimeout;
   struct sockaddr_storage peerAddress;
   socklen_t               peerAddressLength;
   int                     sd[MAX_SIMULTANEOUS_REQUESTS];
   card64                  timeout[MAX_SIMULTANEOUS_REQUESTS];
   card64                  start;
   card64                  nextTimeout;
   GList*                  serverAnnounceList;
   GList*                  serverTransportAddressList;
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
   fputs("Looking for nameserver...\n",stdlog);
   LOG_END

   trials             = 0;
   serverAnnounceList = NULL;
   start              = 0;

   for(;;) {
      if(serverAnnounceList == NULL) {
         /* Start new trial, when
            - First time
            - A new server announce has been added to the list
            - The current trial has been running for at least serverTable->NameServerConnectTimeout */
         if( (start == 0)                                       ||
             (serverTable->NameServerAnnounceListAddition >= start) ||
             (start + serverTable->NameServerConnectTimeout < getMicroTime()) ) {
            trials++;
            if(trials > serverTable->NameServerConnectMaxTrials) {
               LOG_ERROR
               fputs("No nameserver found!\n",stdlog);
               LOG_END
               for(j = 0;j < MAX_SIMULTANEOUS_REQUESTS;j++) {
                  if(sd[j] >= 0) {
                     ext_close(sd[j]);
                  }
               }
               return(0);
            }
            serverAnnounceList         = g_list_first(serverTable->NameServerAnnounceList);
            serverTransportAddressList = NULL;
            start                      = getMicroTime();
            LOG_ACTION
            fprintf(stdlog,"Trial #%u...\n",trials);
            LOG_END
         }
      }

      /* Try next block of addresses */
      tryNextBlock(serverTable,
                   &serverAnnounceList,
                   &serverTransportAddressList,
                   (int*)&sd, (card64*)&timeout,
                   protocol);

      /* Wait for event */
      n = 0;
      FD_ZERO(&rfdset);
      FD_ZERO(&wfdset);
      if(serverTable->NameServerIPv4AnnounceSocket >= 0) {
         FD_SET(serverTable->NameServerIPv4AnnounceSocket,&rfdset);
         n = max(n,serverTable->NameServerIPv4AnnounceSocket);
      }
#ifdef HAVE_IPV6
      if(serverTable->NameServerIPv6AnnounceSocket >= 0) {
         FD_SET(serverTable->NameServerIPv6AnnounceSocket,&rfdset);
         n = max(n,serverTable->NameServerIPv6AnnounceSocket);
      }
#endif
      nextTimeout = serverTable->NameServerConnectTimeout;
      for(i = 0;i < MAX_SIMULTANEOUS_REQUESTS;i++) {
         if(sd[i] >= 0) {
            FD_SET(sd[i],&wfdset);
            n           = max(n,sd[i]);
            nextTimeout = min(nextTimeout,timeout[i]);
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
               if(FD_ISSET(sd[i],&wfdset)) {
                  peerAddressLength = sizeof(peerAddress);
                  if(ext_getpeername(sd[i],(struct sockaddr*)&peerAddress,&peerAddressLength) >= 0) {
                     LOG_ACTION
                     fprintf(stdlog,"Successfully found nameserver, socket %d\n",sd[i]);
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
                     fprintf(stdlog,"Connection trial of socket %d failed: %s\n",sd[i],strerror(errno));
                     LOG_END
                     ext_close(sd[i]);
                     sd[i] = -1;
                  }
               }
               else if((sd[i] >= 0) && (timeout[i] < getMicroTime())) {
                  LOG_VERBOSE3
                  fprintf(stdlog,"Connection trial of socket %d timed out\n",sd[i]);
                  LOG_END
                  ext_close(sd[i]);
                  sd[i] = -1;
               }
            }
         }
         if((serverTable->NameServerIPv4AnnounceSocket >= 0) &&
            FD_ISSET(serverTable->NameServerIPv4AnnounceSocket,&rfdset)) {
            receiveServerAnnounce(serverTable,serverTable->NameServerIPv4AnnounceSocket);
         }
#ifdef HAVE_IPV6
         if((serverTable->NameServerIPv6AnnounceSocket >= 0) &&
            FD_ISSET(serverTable->NameServerIPv6AnnounceSocket,&rfdset)) {
            receiveServerAnnounce(serverTable,serverTable->NameServerIPv6AnnounceSocket);
         }
#endif
      }
   }
}
