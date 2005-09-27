/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include "tdtypes.h"
#include "rserpool.h"
#include "debug.h"
#include "dispatcher.h"
#include "identifierbitmap.h"
#include "netutilities.h"
#include "threadsafety.h"
#include "rserpoolmessage.h"
#include "asapinstance.h"
#include "leaflinkedredblacktree.h"
#include "loglevel.h"
#include "sessioncontrol.h"
#include "componentstatusreporter.h"


extern struct ASAPInstance*          gAsapInstance;
extern struct Dispatcher             gDispatcher;
extern struct LeafLinkedRedBlackTree gRSerPoolSocketSet;
extern struct ThreadSafety           gRSerPoolSocketSetMutex;
extern struct IdentifierBitmap*      gRSerPoolSocketAllocationBitmap;


#define GET_RSERPOOL_SOCKET(rserpoolSocket, sd) \
   rserpoolSocket = getRSerPoolSocketForDescriptor(sd); \
   if(rserpoolSocket == NULL) { \
      errno = EBADF; \
      return(-1); \
   }


/* ###### Configure notifications of SCTP socket ############################# */
static bool configureSCTPSocket(int sd, sctp_assoc_t assocID)
{
   struct sctp_event_subscribe events;

   memset((char*)&events, 0 ,sizeof(events));
   events.sctp_data_io_event     = 1;
   events.sctp_association_event = 1;
   events.sctp_shutdown_event    = 1;
   if(ext_setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events)) < 0) {
      LOG_ERROR
      perror("setsockopt failed for SCTP_EVENTS");
      LOG_END
      return(false);
   }
   return(true);
}


/* ###### Create RSerPool socket ######################################### */
int rsp_socket_internal(int domain, int type, int protocol, int customFD)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    fd;

   /* ====== Check for problems ========================================== */
   if(gAsapInstance == NULL) {
      LOG_ERROR
      fputs("rsplib is not initialized\n", stdlog);
      LOG_END
      errno = EACCES;
      return(-1);
   }
   if(protocol != IPPROTO_SCTP) {
      LOG_ERROR
      fputs("Only SCTP is supported for the Enhanced Mode API\n", stdlog);
      LOG_END
      errno = EPROTONOSUPPORT;
      return(-1);
   }

   /* ====== Create socket =============================================== */
   domain = (domain == 0) ? (checkIPv6() ? AF_INET6 : AF_INET) : domain;
   if(customFD < 0) {
      fd = ext_socket(domain, type, protocol);
   }
   else {
      fd = customFD;
   }
   if(fd <= 0) {
      LOG_ERROR
      logerror("Unable to create socket for RSerPool socket");
      LOG_END
      return(-1);
   }
   setNonBlocking(fd);

   /* ====== Configure SCTP socket ======================================= */
   if(!configureSCTPSocket(fd, 0)) {
      LOG_ERROR
      fputs("Failed to configure SCTP parameters for socket\n", stdlog);
      LOG_END
      close(fd);
      return(-1);
   }

   /* ====== Initialize RSerPool socket entry ============================ */
   rserpoolSocket = (struct RSerPoolSocket*)malloc(sizeof(struct RSerPoolSocket));
   if(rserpoolSocket == NULL) {
      close(fd);
      errno = ENOMEM;
      return(-1);
   }

   /* ====== Allocate message buffer ===================================== */
   rserpoolSocket->MessageBuffer = (char*)malloc(RSERPOOL_MESSAGE_BUFFER_SIZE);
   if(rserpoolSocket->MessageBuffer == NULL) {
      free(rserpoolSocket);
      close(fd);
      errno = ENOMEM;
      return(-1);
   }

   /* ====== Create session ID allocation bitmap ========================= */
   rserpoolSocket->SessionAllocationBitmap = identifierBitmapNew(SESSION_SETSIZE);
   if(rserpoolSocket->SessionAllocationBitmap == NULL) {
      free(rserpoolSocket->MessageBuffer);
      free(rserpoolSocket);
      close(fd);
      errno = ENOMEM;
      return(-1);
   }
   CHECK(identifierBitmapAllocateSpecificID(rserpoolSocket->SessionAllocationBitmap, 0) == 0);

   threadSafetyNew(&rserpoolSocket->Mutex, "RSerPoolSocket");
   leafLinkedRedBlackTreeNodeNew(&rserpoolSocket->Node);
   sessionStorageNew(&rserpoolSocket->SessionSet);
   rserpoolSocket->Socket           = fd;
   rserpoolSocket->SocketDomain     = domain;
   rserpoolSocket->SocketType       = type;
   rserpoolSocket->SocketProtocol   = protocol;
   rserpoolSocket->Descriptor       = -1;
   rserpoolSocket->PoolElement      = NULL;
   rserpoolSocket->ConnectedSession = NULL;

   notificationQueueNew(&rserpoolSocket->Notifications);
   if(rserpoolSocket->SocketType == SOCK_STREAM) {
      rserpoolSocket->Notifications.EventMask = 0;
   }
   else {
      rserpoolSocket->Notifications.EventMask = NET_NOTIFICATION_MASK;
   }

   /* ====== Find available RSerPool socket descriptor =================== */
   threadSafetyLock(&gRSerPoolSocketSetMutex);
   rserpoolSocket->Descriptor = identifierBitmapAllocateID(gRSerPoolSocketAllocationBitmap);
   if(rserpoolSocket->Descriptor >= 0) {
      /* ====== Add RSerPool socket entry ================================ */
      CHECK(leafLinkedRedBlackTreeInsert(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
   }
   threadSafetyUnlock(&gRSerPoolSocketSetMutex);

   /* ====== Has there been a problem? =================================== */
   if(rserpoolSocket->Descriptor < 0) {
      identifierBitmapDelete(rserpoolSocket->SessionAllocationBitmap);
      free(rserpoolSocket->MessageBuffer);
      free(rserpoolSocket);
      close(fd);
      errno = EMFILE;
      return(-1);
   }
   return(rserpoolSocket->Descriptor);
}


/* ###### Create RSerPool socket ######################################### */
int rsp_socket(int domain, int type, int protocol)
{
   return(rsp_socket_internal(domain, type, protocol, -1));
}


/* ###### Delete RSerPool socket ######################################### */
int rsp_close(int sd)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   struct Session*        nextSession;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd)

   threadSafetyLock(&rserpoolSocket->Mutex);

   /* ====== Close sessions ============================================== */
   if(rserpoolSocket->PoolElement) {
      rsp_deregister(sd);
   }
   session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
   while(session != NULL) {
      nextSession = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
      LOG_ACTION
      fprintf(stdlog, "RSerPool socket %d, socket %d has open session %u -> closing it\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              session->SessionID);
      LOG_END
      /* Send ABORT to peer */
      sendabort(rserpoolSocket->Socket, session->AssocID);
      deleteSession(rserpoolSocket, session);
      session = nextSession;
   }

   /* ====== Delete RSerPool socket entry ================================ */
   threadSafetyLock(&gRSerPoolSocketSetMutex);
   CHECK(leafLinkedRedBlackTreeRemove(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
   identifierBitmapFreeID(gRSerPoolSocketAllocationBitmap, sd);
   rserpoolSocket->Descriptor = -1;
   threadSafetyUnlock(&gRSerPoolSocketSetMutex);

   notificationQueueDelete(&rserpoolSocket->Notifications);
   sessionStorageDelete(&rserpoolSocket->SessionSet);

   if(rserpoolSocket->Socket >= 0) {
      ext_close(rserpoolSocket->Socket);
      rserpoolSocket->Socket = -1;
   }
   if(rserpoolSocket->SessionAllocationBitmap) {
      identifierBitmapDelete(rserpoolSocket->SessionAllocationBitmap);
      rserpoolSocket->SessionAllocationBitmap = NULL;
   }
   if(rserpoolSocket->MessageBuffer) {
      free(rserpoolSocket->MessageBuffer);
      rserpoolSocket->MessageBuffer = NULL;
   }
   threadSafetyDelete(&rserpoolSocket->Mutex);
   free(rserpoolSocket);
   return(0);
}


/* ###### Map socket descriptor into RSerPool socket ##################### */
int rsp_mapsocket(int sd, int toSD)
{
   struct RSerPoolSocket* rserpoolSocket;

   /* ====== Check for problems ========================================== */
   if((sd < 0) || (sd >= FD_SETSIZE)) {
      errno = EINVAL;
      return(-1);
   }

   /* ====== Initialize RSerPool socket entry ============================ */
   rserpoolSocket = (struct RSerPoolSocket*)malloc(sizeof(struct RSerPoolSocket));
   if(rserpoolSocket == NULL) {
      errno = ENOMEM;
      return(-1);
   }
   memset(rserpoolSocket, 0, sizeof(struct RSerPoolSocket));
   rserpoolSocket->Socket = sd;

   /* ====== Find available RSerPool socket descriptor =================== */
   threadSafetyLock(&gRSerPoolSocketSetMutex);
   if(toSD >= 0) {
      rserpoolSocket->Descriptor = identifierBitmapAllocateSpecificID(gRSerPoolSocketAllocationBitmap, toSD);
   }
   else {
      rserpoolSocket->Descriptor = identifierBitmapAllocateID(gRSerPoolSocketAllocationBitmap);
   }
   if(rserpoolSocket->Descriptor >= 0) {
      /* ====== Add RSerPool socket entry ================================ */
      CHECK(leafLinkedRedBlackTreeInsert(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
   }
   threadSafetyUnlock(&gRSerPoolSocketSetMutex);

   /* ====== Has there been a problem? =================================== */
   if(rserpoolSocket->Descriptor < 0) {
      free(rserpoolSocket);
      errno = EMFILE;
      return(-1);
   }
   return(rserpoolSocket->Descriptor);
}


/* ###### Unmap socket descriptor from RSerPool socket ################### */
int rsp_unmapsocket(int sd)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    result = 0;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd)

   if(rserpoolSocket->SessionAllocationBitmap == NULL) {
      threadSafetyLock(&gRSerPoolSocketSetMutex);
      CHECK(leafLinkedRedBlackTreeRemove(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
      identifierBitmapFreeID(gRSerPoolSocketAllocationBitmap, sd);
      rserpoolSocket->Descriptor = -1;
      threadSafetyUnlock(&gRSerPoolSocketSetMutex);
      free(rserpoolSocket);
   }
   else {
      errno = EBADF;
      result = -1;
   }
   return(result);
}


/* ###### RSerPool socket poll() implementation ########################## */
int rsp_poll(struct pollfd* ufds, unsigned int nfds, int timeout)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    fdbackup[FD_SETSIZE];
   int                    result;
   unsigned int           i;

   /* ====== Check for problems ========================================== */
   if(nfds > FD_SETSIZE) {
      errno = EINVAL;
      return(-1);
   }

   /* ====== Collect data for poll() call ================================ */
   result = 0;
   for(i = 0;i < nfds;i++) {
      fdbackup[i] = ufds[i].fd;
      rserpoolSocket = getRSerPoolSocketForDescriptor(ufds[i].fd);
      if(rserpoolSocket != NULL) {
         threadSafetyLock(&rserpoolSocket->Mutex);
         ufds[i].fd      = rserpoolSocket->Socket;
         ufds[i].revents = 0;
         if((ufds[i].events & POLLIN) &&
            (notificationQueueHasData(&rserpoolSocket->Notifications))) {
            result++;
            ufds[i].revents = POLLIN;
         }
         threadSafetyUnlock(&rserpoolSocket->Mutex);
      }
      else {
         ufds[i].fd = -1;
      }
   }

   // ====== Do poll() =================================================== */
   if(result == 0) {
      /* Only call poll() when there are no notifications */
      result = ext_poll(ufds, nfds, timeout);
   }

   // ====== Handle results ============================================== */
   for(i = 0;i < nfds;i++) {
      rserpoolSocket = getRSerPoolSocketForDescriptor(fdbackup[i]);
      if((rserpoolSocket != NULL) && (rserpoolSocket->SessionAllocationBitmap != NULL)) {
         threadSafetyLock(&rserpoolSocket->Mutex);

         /* ======= Check for control channel data ======================= */
         if(ufds[i].revents & POLLIN) {
            LOG_VERBOSE4
            fprintf(stdlog, "RSerPool socket %d (socket %d) has <read> flag set -> Check, if it has to be handled by rsplib...\n",
                     rserpoolSocket->Descriptor, rserpoolSocket->Socket);
            LOG_END
            if(handleControlChannelAndNotifications(rserpoolSocket)) {
               LOG_VERBOSE4
               fprintf(stdlog, "RSerPool socket %d (socket %d) had <read> event for rsplib only. Clearing <read> flag\n",
                        rserpoolSocket->Descriptor, rserpoolSocket->Socket);
               LOG_END
               ufds[i].revents &= ~POLLIN;
            }
         }

         /* ====== Set <read> flag for RSerPool notifications? =========== */
         if((ufds[i].events & POLLIN) &&
            (notificationQueueHasData(&rserpoolSocket->Notifications))) {
            ufds[i].revents |= POLLIN;
         }

         threadSafetyUnlock(&rserpoolSocket->Mutex);
      }
      ufds[i].fd = fdbackup[i];
   }

   return(result);
}


/* ###### RSerPool socket select() implementation ######################## */
int rsp_select(int n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
               struct timeval* timeout)
{
   struct pollfd ufds[FD_SETSIZE];
   unsigned int  nfds;
   int           waitingTime;
   int           result;
   int           i;

   /* ====== Check for problems ========================================== */
   if(n > FD_SETSIZE) {
      errno = EINVAL;
      return(-1);
   }

   /* ====== Prepare pollfd array ======================================== */
   nfds = 0;
   for(i = 0;i < n;i++) {
      ufds[nfds].events = 0;
      if((readfds) && (FD_ISSET(i, readfds))) {
         ufds[nfds].fd = i;
         ufds[nfds].events |= POLLIN;
      }
      if((writefds) && (FD_ISSET(i, writefds))) {
         ufds[nfds].fd = i;
         ufds[nfds].events |= POLLOUT;
      }
      if((exceptfds) && (FD_ISSET(i, exceptfds))) {
         ufds[nfds].fd = i;
         ufds[nfds].events |= ~(POLLIN|POLLOUT);
      }
      if(ufds[nfds].events) {
         nfds++;
      }
   }

   /* ====== Call poll() and propagate results to fdsets ================= */
   waitingTime = (1000 * timeout->tv_sec) + (timeout->tv_usec / 1000);
   result = rsp_poll((struct pollfd*)&ufds, nfds, waitingTime);
   if(result > 0) {
      for(i = 0;i < n;i++) {
         if( (!(ufds[nfds].events & POLLIN)) && (readfds) ) {
            FD_CLR(i, readfds);
         }
         if( (!(ufds[nfds].events & POLLOUT)) && (writefds) ) {
            FD_CLR(i, writefds);
         }
         if( (!(ufds[nfds].events & (POLLIN|POLLHUP|POLLNVAL))) && (exceptfds) ) {
            FD_CLR(i, exceptfds);
         }
      }
   }

   return(result);
}


/* ###### Bind RSerPool socket ########################################### */
int rsp_bind(int sd, struct sockaddr* addrs, int addrcnt)
{
   struct RSerPoolSocket* rserpoolSocket;
   union sockaddr_union   localAddress;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   /* ====== Bind socket ================================================= */
   if(addrs == NULL) {
      addrs   = (struct sockaddr*)&localAddress;
      addrcnt = 1;
      memset((void*)&localAddress, 0, sizeof(localAddress));
      ((struct sockaddr*)&localAddress)->sa_family = rserpoolSocket->SocketDomain;
   }
   if(bindplus(rserpoolSocket->Socket, (union sockaddr_union*)addrs, addrcnt) == false) {
      LOG_ERROR
      logerror("Unable to bind socket for new pool element");
      LOG_END
      return(-1);
   }

   return(0);
}


/* ###### Register pool element ########################################## */
int rsp_register_tags(int                        sd,
                      const unsigned char*       poolHandle,
                      const size_t               poolHandleSize,
                      const struct rsp_loadinfo* loadinfo,
                      unsigned int               reregistrationInterval,
                      struct TagItem*            tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct PoolHandle      cmpPoolHandle;
   union sockaddr_union   socketName;
   socklen_t              socketNameLen;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   threadSafetyLock(&rserpoolSocket->Mutex);

   /* ====== Check for problems ========================================== */
   socketNameLen = sizeof(socketName);
   if((ext_getsockname(rserpoolSocket->Socket, (struct sockaddr*)&socketName, &socketNameLen) < 0) ||
      (getPort(&socketName.sa) == 0)) {
      LOG_VERBOSE
      fprintf(stdlog, "RSerPool socket %d, socket %d is not bound -> trying to bind it to the ANY address\n",
              sd, rserpoolSocket->Socket);
      LOG_END
      if(rsp_bind(sd, NULL, 0) < 0) {
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         return(-1);
      }

   }

   if(poolHandleSize > MAX_POOLHANDLESIZE) {
      LOG_ERROR
      fputs("Pool handle too long\n", stdlog);
      LOG_END
      errno = ENAMETOOLONG;
      threadSafetyUnlock(&rserpoolSocket->Mutex);
      return(-1);
   }

   /* ====== Reregistration ============================================== */
   if(rserpoolSocket->PoolElement != NULL) {
      /* ====== Check for incompatibilities ============================== */
      poolHandleNew(&cmpPoolHandle, poolHandle, poolHandleSize);
      if(poolHandleComparison(&cmpPoolHandle, &rserpoolSocket->PoolElement->Handle) != 0) {
         LOG_ERROR
         fprintf(stdlog, "RSerPool socket %d already has a pool element; use same PH for update\n", sd);
         LOG_END
         errno = EBADF;
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         return(-1);
      }

      /* ====== Update registration information ========================== */
      threadSafetyLock(&rserpoolSocket->PoolElement->Mutex);
      rserpoolSocket->PoolElement->LoadInfo               = *loadinfo;
      rserpoolSocket->PoolElement->ReregistrationInterval = reregistrationInterval;
      rserpoolSocket->PoolElement->RegistrationLife       = 3 * rserpoolSocket->PoolElement->ReregistrationInterval;

      /* ====== Schedule reregistration as soon as possible ============== */
      timerStart(&rserpoolSocket->PoolElement->ReregistrationTimer, 0);
      threadSafetyUnlock(&rserpoolSocket->PoolElement->Mutex);
   }

   /* ====== Registration of a new pool element ========================== */
   else {
      if(ext_listen(rserpoolSocket->Socket, 10) < 0) {
         LOG_ERROR
         logerror("Unable to set socket for new pool element to listen mode");
         LOG_END
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         return(-1);
      }

      /* ====== Create pool element ====================================== */
      rserpoolSocket->PoolElement = (struct PoolElement*)malloc(sizeof(struct PoolElement));
      if(rserpoolSocket->PoolElement == NULL) {
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         return(-1);
      }
      threadSafetyNew(&rserpoolSocket->PoolElement->Mutex, "RspPoolElement");
      poolHandleNew(&rserpoolSocket->PoolElement->Handle, poolHandle, poolHandleSize);
      timerNew(&rserpoolSocket->PoolElement->ReregistrationTimer,
               &gDispatcher,
               reregistrationTimer,
               (void*)rserpoolSocket);

      rserpoolSocket->PoolElement->Identifier             = tagListGetData(tags, TAG_PoolElement_Identifier,
                                                               0x00000000);
      rserpoolSocket->PoolElement->LoadInfo               = *loadinfo;
      rserpoolSocket->PoolElement->ReregistrationInterval = reregistrationInterval;
      rserpoolSocket->PoolElement->RegistrationLife       = 3 * rserpoolSocket->PoolElement->ReregistrationInterval;
      rserpoolSocket->PoolElement->HasControlChannel      = tagListGetData(tags, TAG_UserTransport_HasControlChannel, false);

      /* ====== Do registration ============================================= */
      if(doRegistration(rserpoolSocket, true) == false) {
         LOG_ERROR
         fputs("Unable to obtain registration information -> Creating pool element not possible\n", stdlog);
         LOG_END
         deletePoolElement(rserpoolSocket->PoolElement, tags);
         rserpoolSocket->PoolElement = NULL;
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         return(-1);
      }

      /* ====== start reregistration timer ================================== */
      timerStart(&rserpoolSocket->PoolElement->ReregistrationTimer,
                  getMicroTime() + ((unsigned long long)1000 *
                                      (unsigned long long)rserpoolSocket->PoolElement->ReregistrationInterval));
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(0);
}


/* ###### Register pool element ########################################## */
int rsp_register(int                        sd,
                 const unsigned char*       poolHandle,
                 const size_t               poolHandleSize,
                 const struct rsp_loadinfo* loadinfo,
                 unsigned int               reregistrationInterval)
{
   return(rsp_register_tags(sd, poolHandle, poolHandleSize, loadinfo,
                            reregistrationInterval, NULL));
}


/* ###### Deregister pool element ######################################## */
int rsp_deregister_tags(int sd, struct TagItem* tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    result = 0;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   /* ====== Delete pool element ========================================= */
   threadSafetyLock(&rserpoolSocket->Mutex);
   if(rserpoolSocket->PoolElement != NULL) {
      deletePoolElement(rserpoolSocket->PoolElement, tags);
      rserpoolSocket->PoolElement = NULL;
   }
   else {
      result = -1;
      errno  = EBADF;
   }
   threadSafetyUnlock(&rserpoolSocket->Mutex);

   return(result);
}


/* ###### Deregister pool element ######################################## */
int rsp_deregister(int sd)
{
   return(rsp_deregister_tags(sd, NULL));
}


/* ###### RSerPool socket connect() implementation ####################### */
int rsp_connect_tags(int                  sd,
                     const unsigned char* poolHandle,
                     const size_t         poolHandleSize,
                     struct TagItem*      tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   unsigned int           oldEventMask;
   int                    result = -1;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   threadSafetyLock(&rserpoolSocket->Mutex);

   /* ====== Check for problems ========================================== */
   if(sessionStorageGetElements(&rserpoolSocket->SessionSet) == 0) {
      /* ====== Create session =========================================== */
      session = addSession(rserpoolSocket, 0, false, poolHandle, poolHandleSize, tags);
      if(session != NULL) {
         rserpoolSocket->ConnectedSession = session;

         /* ====== Connect to a PE ======================================= */
         /* Do not signalize successful failover, since this is the first
            connection establishment */
         oldEventMask = rserpoolSocket->Notifications.EventMask;
         rserpoolSocket->Notifications.EventMask = 0;
         if(rsp_forcefailover_tags(rserpoolSocket->Descriptor, tags) == 0) {
            result = 0;
         }
         else {
            deleteSession(rserpoolSocket, session);
            errno = EIO;
         }
         rserpoolSocket->Notifications.EventMask = oldEventMask;

      }
      else {
         errno = ENOMEM;
      }
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "RSerPool socket %d, socket %d already has a session; cannot connect it again\n",
              sd, rserpoolSocket->Socket);
      LOG_END
      errno = EBADF;
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(result);
}


/* ###### RSerPool socket connect() implementation ####################### */
int rsp_connect(int                  sd,
                const unsigned char* poolHandle,
                const size_t         poolHandleSize)
{
   return(rsp_connect_tags(sd, poolHandle, poolHandleSize, NULL));
}


/* ###### RSerPool socket accept() implementation ######################## */
int rsp_accept_tags(int                sd,
                    unsigned long long timeout,
                    struct TagItem*    tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct RSerPoolSocket* newRSerPoolSocket;
   struct Session*        session;
   int                    result = -1;
   int                    newSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   if(waitForRead(rserpoolSocket, timeout)) {
      newSocket = ext_accept(rserpoolSocket->Socket, NULL, 0);
      if(newSocket >= 0) {
         LOG_VERBOSE
         fprintf(stdlog, "Accepted new socket %d on RSerPool socket %u, socket %d\n",
               newSocket, rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
         result = rsp_socket_internal(0, SOCK_STREAM, IPPROTO_SCTP, newSocket);
         if(result >= 0) {
            newRSerPoolSocket = getRSerPoolSocketForDescriptor(result);
            CHECK(newRSerPoolSocket);
            session = addSession(newRSerPoolSocket, 0, true, NULL, 0, tags);
            if(session == NULL) {
               errno = ENOMEM;
               ext_close(result);
               result = -1;
            }
            else {
               newRSerPoolSocket->ConnectedSession = session;

               LOG_ACTION
               fprintf(stdlog, "Accepted new session %u (RSerPool socket %u, socket %u) on RSerPool socket %u, socket %d\n",
                     session->SessionID,
                     newRSerPoolSocket->Descriptor, newRSerPoolSocket->Socket,
                     rserpoolSocket->Descriptor, rserpoolSocket->Socket);
               LOG_END
            }
         }
      }
   }

   return(result);
}


/* ###### RSerPool socket accept() implementation ######################## */
int rsp_accept(int                sd,
               unsigned long long timeout)
{
   return(rsp_accept_tags(sd, timeout, NULL));
}


/* ###### Make failover ################################################## */
int rsp_forcefailover_tags(int             sd,
                           struct TagItem* tags)
{
   struct RSerPoolSocket*   rserpoolSocket;
   struct rsp_addrinfo*     rspAddrInfo;
   struct rsp_addrinfo*     rspAddrInfo2;
   struct NotificationNode* notificationNode;
   union sctp_notification  notification;
   struct timeval           timeout;
   ssize_t                  received;
   fd_set                   readfds;
   int                      result;
   int                      flags;
   bool                     success = false;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   threadSafetyLock(&rserpoolSocket->Mutex);

   /* ====== Check for problems ========================================== */
   if(rserpoolSocket->ConnectedSession == NULL) {
      LOG_ERROR
      fprintf(stdlog, "RSerPool socket %u, socket %d has no connected session\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket);
      LOG_END
      errno = EBADF;
      threadSafetyUnlock(&rserpoolSocket->Mutex);
      return(-1);
   }

   /* When next rsp_sendmsg() fails, a new FAILOVER_NECESSARY notification
      has to be sent. */
   rserpoolSocket->ConnectedSession->IsFailed = false;

   /* ====== Report failure ============================================== */
   if((rserpoolSocket->ConnectedSession->ConnectedPE != 0) &&
      (rserpoolSocket->ConnectedSession->Handle.Size > 0)) {
      LOG_ACTION
      fputs("Reporting failure of ", stdlog);
      poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
      fprintf(stdlog, "/$%08x on RSerPool socket %u, socket %d, session %u\n",
              rserpoolSocket->ConnectedSession->ConnectedPE,
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              rserpoolSocket->ConnectedSession->SessionID);
      LOG_END
      rsp_pe_failure_tags((unsigned char*)&rserpoolSocket->ConnectedSession->Handle.Handle,
                          rserpoolSocket->ConnectedSession->Handle.Size,
                          rserpoolSocket->ConnectedSession->ConnectedPE,
                          tags);
      rserpoolSocket->ConnectedSession->ConnectedPE = 0;
      if(rserpoolSocket->ConnectedSession->AssocID != 0) {
         sendabort(rserpoolSocket->Descriptor,
                   rserpoolSocket->ConnectedSession->AssocID);
      }
      rserpoolSocket->ConnectedSession->AssocID = 0;
   }

   /* ====== Do handle resolution ======================================== */
   if(rserpoolSocket->ConnectedSession->Handle.Size > 0) {
      LOG_ACTION
      fputs("Doing handle resolution for pool handle ", stdlog);
      poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
      fprintf(stdlog, " of RSerPool socket %u, socket %d, session %u\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              rserpoolSocket->ConnectedSession->SessionID);
      LOG_END
      result = rsp_getaddrinfo_tags((unsigned char*)&rserpoolSocket->ConnectedSession->Handle.Handle,
                                    rserpoolSocket->ConnectedSession->Handle.Size,
                                    &rspAddrInfo,
                                    tags);
      if(result == 0) {
         if(rspAddrInfo->ai_protocol == rserpoolSocket->SocketProtocol) {

            /* ====== Establish connection ================================== */
            rspAddrInfo2 = rspAddrInfo;
            while((rspAddrInfo2 != NULL) && (success == false)) {

               LOG_VERBOSE
               fprintf(stdlog, "Trying connection to pool element $%08x (", rspAddrInfo2->ai_pe_id);
               poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
               fprintf(stdlog, "/$%08x on RSerPool socket %u, socket %d, session %u)\n",
                       rspAddrInfo2->ai_pe_id,
                       rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                       rserpoolSocket->ConnectedSession->SessionID);
               LOG_END
               result = sctp_connectx(rserpoolSocket->Socket,
                                      (struct sockaddr*)rspAddrInfo2->ai_addr,
                                      rspAddrInfo2->ai_addrs);
               if((result == 0) || (errno == EINPROGRESS)) {
                  FD_ZERO(&readfds);
                  FD_SET(rserpoolSocket->Socket, &readfds);
                  timeout.tv_sec  = rserpoolSocket->ConnectedSession->ConnectTimeout / 1000000;
                  timeout.tv_usec = rserpoolSocket->ConnectedSession->ConnectTimeout % 1000000;
                  ext_select(rserpoolSocket->Socket + 1, &readfds, NULL, NULL, &timeout);

                  flags = 0;
                  received = recvfromplus(rserpoolSocket->Socket,
                                          (char*)&notification, sizeof(notification),
                                          &flags, NULL, 0, NULL, NULL, NULL,0);
                  if(received > 0) {
                     if(flags & MSG_NOTIFICATION) {
                        if(notification.sn_header.sn_type == SCTP_ASSOC_CHANGE) {
                           if(notification.sn_assoc_change.sac_state == SCTP_COMM_UP) {
                              LOG_VERBOSE
                              fprintf(stdlog, "Successfully established connection to pool element $%08x (", rspAddrInfo2->ai_pe_id);
                              poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
                              fprintf(stdlog, "/$%08x on RSerPool socket %u, socket %d, session %u, assoc %u)\n",
                                      rspAddrInfo2->ai_pe_id,
                                      rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                                      rserpoolSocket->ConnectedSession->SessionID,
                                      (unsigned int)notification.sn_assoc_change.sac_assoc_id);
                              LOG_END
                              success = true;
                              rserpoolSocket->ConnectedSession->ConnectionTimeStamp = getMicroTime();
                              rserpoolSocket->ConnectedSession->ConnectedPE         = rspAddrInfo2->ai_pe_id;

                              sessionStorageUpdateSession(&rserpoolSocket->SessionSet,
                                                         rserpoolSocket->ConnectedSession,
                                                         notification.sn_assoc_change.sac_assoc_id);
                              break;
                           }
                           else if(notification.sn_assoc_change.sac_state == SCTP_COMM_LOST) {
                              LOG_VERBOSE
                              fprintf(stdlog, "Successfully established connection to pool element $%08x (", rspAddrInfo2->ai_pe_id);
                              poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
                              fprintf(stdlog, "/$%08x on RSerPool socket %u, socket %d, session %u, assoc %u)\n",
                                    rspAddrInfo2->ai_pe_id,
                                    rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                                    rserpoolSocket->ConnectedSession->SessionID,
                                    (unsigned int)notification.sn_assoc_change.sac_assoc_id);
                              LOG_END
                              break;
                           }
                        }
                     }
                     else {
                        LOG_ERROR
                        fputs("Received data before COMM_UP notification?!\n", stdlog);
                        LOG_END
                        break;
                     }
                  }
                  else {
                     LOG_ERROR
                     logerror("Error while trying to wait for COMM_UP notification");
                     LOG_END
                     break;
                  }
               }
               rspAddrInfo2 = rspAddrInfo2->ai_next;
            }


            /* ====== Free handle resolution result ====================== */
            rsp_freeaddrinfo(rspAddrInfo);


            /* ====== Failover procedue ================================== */
            if(success) {
               /* ====== Do failover by client-based state sharing ======= */
               if(rserpoolSocket->ConnectedSession->Cookie) {
                  success = sendCookieEcho(rserpoolSocket, rserpoolSocket->ConnectedSession);
               }
            }
#if 0
            if(success) {
               /* ====== Send business card ============================== */
               if( (!rserpoolSocket->ConnectedSession->IsIncoming) &&
                   (rserpoolSocket->ConnectedSession->PoolElement)) {
                  sendBusinessCard(rserpoolSocket, rserpoolSocket->ConnectedSession);
               }
            }
#endif

            /* ====== Notify application of successful failover ========== */
            if(success) {
               notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                                       true, RSERPOOL_FAILOVER);
               if(notificationNode) {
                  notificationNode->Content.rn_failover.rf_state      = RSERPOOL_FAILOVER_COMPLETE;
                  notificationNode->Content.rn_failover.rf_session    = rserpoolSocket->ConnectedSession->SessionID;
                  notificationNode->Content.rn_failover.rf_has_cookie = (rserpoolSocket->ConnectedSession->CookieEchoSize > 0);
               }
            }
            else {
               LOG_ACTION
               fputs("Connection establishment was not possible\n", stdlog);
               LOG_END
            }
         }
         else {
            LOG_ERROR
            fputs("Pool elements in pool ", stdlog);
            poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
            fprintf(stdlog, " do not use the same transport protocol as RSerPool socket %u, socket %d, session %u\n",
                    rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                    rserpoolSocket->ConnectedSession->SessionID);
            LOG_END
         }
      }
      else if(result == EAI_NONAME) {
         LOG_ACTION
         fprintf(stdlog,
                 "Handle resolution did not find new pool element. Waiting %lluus...\n",
                 rserpoolSocket->ConnectedSession->HandleResolutionRetryDelay);
         LOG_END
         usleep((unsigned int)rserpoolSocket->ConnectedSession->HandleResolutionRetryDelay);
         errno = ENOENT;
      }
      else {
         LOG_WARNING
         fputs("Handle resolution failed\n", stdlog);
         LOG_END
         errno = EIO;
      }
   }
   else {
      LOG_WARNING
      fputs("No pool handle for failover\n", stdlog);
      LOG_END
      errno = EINVAL;
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return((success == true) ? 0 : -1);
}


/* ###### Make failover ################################################## */
int rsp_forcefailover(int sd)
{
   return(rsp_forcefailover_tags(sd, NULL));
}


/* ###### RSerPool socket sendmsg() implementation ####################### */
ssize_t rsp_sendmsg(int                sd,
                    const void*        data,
                    size_t             dataLength,
                    unsigned int       msg_flags,
                    rserpool_session_t sessionID,
                    uint32_t           sctpPPID,
                    uint16_t           sctpStreamID,
                    uint32_t           sctpTimeToLive,
                    unsigned long long timeout)
{
   struct RSerPoolSocket*   rserpoolSocket;
   struct Session*          session;
   struct NotificationNode* notificationNode;
   ssize_t                  result;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   threadSafetyLock(&rserpoolSocket->Mutex);

   session = findSession(rserpoolSocket, sessionID, 0);
   if(session != NULL) {
      if(!session->IsFailed) {
         LOG_VERBOSE1
         fprintf(stdlog, "Trying to send message via session %u of RSerPool socket %d, socket %d\n",
                 session->SessionID,
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
         result = sendtoplus(rserpoolSocket->Socket, data, dataLength,
#ifdef MSG_NOSIGNAL
                             msg_flags|MSG_NOSIGNAL,
#else
                             msg_flags,
#endif
                             NULL, 0,
                             ntohl(sctpPPID), session->AssocID, sctpStreamID, sctpTimeToLive,
                             timeout);
         if((result < 0) && (errno != EAGAIN)) {
            LOG_ACTION
            fprintf(stdlog, "Session failure during send on RSerPool socket %d, session %u. Failover necessary\n",
                    rserpoolSocket->Descriptor, session->SessionID);
            LOG_END

            /* ====== Terminate association and notify application ======= */
            sendabort(rserpoolSocket->Socket, session->AssocID);
            sessionStorageUpdateSession(&rserpoolSocket->SessionSet, session, 0);

            notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                                    false, RSERPOOL_FAILOVER);
            if(notificationNode) {
               notificationNode->Content.rn_failover.rf_state      = RSERPOOL_FAILOVER_NECESSARY;
               notificationNode->Content.rn_failover.rf_session    = session->SessionID;
               notificationNode->Content.rn_failover.rf_has_cookie = (session->CookieEchoSize > 0);
            }
            /* =========================================================== */

// ????            sendabort(rserpoolSocket->Socket, session->AssocID);
            result = -1;
         }
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "Session %u of RSerPool socket %d, socket %d requires failover\n",
                 session->SessionID,
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
         result = -1;
         errno = EIO;
      }
   }
   else {
      result = -1;
      errno  = EBADF;
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(result);
}


/* ###### RSerPool socket recvmsg() implementation ####################### */
ssize_t rsp_recvmsg(int                    sd,
                    void*                  buffer,
                    size_t                 bufferLength,
                    struct rsp_sndrcvinfo* rinfo,
                    int*                   msg_flags,
                    unsigned long long     timeout)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   struct rsp_sndrcvinfo  rinfoDummy;
   sctp_assoc_t           assocID;
   int                    flags;
   ssize_t                received;
   ssize_t                received2;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   if(rinfo == NULL) {
      // User is not interested in values.
      rinfo = &rinfoDummy;
      rinfo->rinfo_session = 0;
   }
   else {
      memset(rinfo, 0, sizeof(struct rsp_sndrcvinfo));
   }


   LOG_VERBOSE1
   fprintf(stdlog, "Trying to read message from RSerPool socket %d, socket %d\n",
           rserpoolSocket->Descriptor, rserpoolSocket->Socket);
   LOG_END


   /* ====== Give back Cookie Echo and notifications ===================== */
   if(buffer != NULL) {
      received = getCookieEchoOrNotification(rserpoolSocket, buffer, bufferLength, msg_flags, true);
      if(received > 0) {
         return(received);
      }
   }


   /* ====== Read from socket ========================================= */
   LOG_VERBOSE1
   fprintf(stdlog, "Trying to read from RSerPool socket %d, socket %d with timeout %Ldus\n",
           rserpoolSocket->Descriptor, rserpoolSocket->Socket, timeout);
   LOG_END
   flags = 0;
   received = recvfromplus(rserpoolSocket->Socket,
                           (char*)rserpoolSocket->MessageBuffer,
                           RSERPOOL_MESSAGE_BUFFER_SIZE,
                           &flags, NULL, 0,
                           &rinfo->rinfo_ppid,
                           &assocID,
                           &rinfo->rinfo_stream,
                           timeout);
   LOG_VERBOSE
   fprintf(stdlog, "received=%d\n", received);
   LOG_END


   /* ====== Handle result =============================================== */
   if(received > 0) {
      threadSafetyLock(&rserpoolSocket->Mutex);

      rinfo->rinfo_ppid = htonl(rinfo->rinfo_ppid);

      /* ====== Handle notification ====================================== */
      if(flags & MSG_NOTIFICATION) {
         handleNotification(rserpoolSocket,
                            (const union sctp_notification*)rserpoolSocket->MessageBuffer);
         received = -1;
      }

      /* ====== Handle ASAP control channel message ====================== */
      else if(ntohl(rinfo->rinfo_ppid) == PPID_ASAP) {
         handleControlChannelMessage(rserpoolSocket, assocID,
                                     (char*)rserpoolSocket->MessageBuffer, received);
         received = -1;
      }

      /* ====== User Data ================================================ */
      else {
         /* ====== Find session ========================================== */
         if(rserpoolSocket->ConnectedSession) {
            session              = rserpoolSocket->ConnectedSession;
            rinfo->rinfo_session = session->SessionID;
            rinfo->rinfo_pe_id   = session->ConnectedPE;
         }
         else {
            session = sessionStorageFindSessionByAssocID(&rserpoolSocket->SessionSet, assocID);
            if(session) {
               rinfo->rinfo_session = session->SessionID;
            }
            else {
               LOG_ERROR
               fprintf(stdlog, "Received data on RSerPool socket %d, socket %d via unknown assoc %u\n",
                     rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                     (unsigned int)assocID);
               LOG_END
            }
         }

         /* ====== Copy user data ======================================== */
         if(buffer != NULL) {
            if((ssize_t)bufferLength < received) {
               LOG_ERROR
               fputs("Buffer is too small to keep full message\n", stdlog);
               LOG_END
               errno = ENOMEM;
               return(-1);
            }
         }
         received = min(received, (ssize_t)bufferLength);
         memcpy(buffer, (const char*)rserpoolSocket->MessageBuffer, received);
      }

      threadSafetyUnlock(&rserpoolSocket->Mutex);
   }


   /* ====== Nothing read from socket, but there may be a notification === */
   else {
      /* ====== Give back Cookie Echo and notifications ================== */
      if(buffer != NULL) {
         received2 = getCookieEchoOrNotification(rserpoolSocket, buffer, bufferLength, msg_flags, false);
         if(received2 > 0) {
            received = received2;
         }
      }
   }

   return(received);
}


/* ###### Send cookie ######################################################## */
ssize_t rsp_send_cookie(int                  sd,
                        const unsigned char* cookie,
                        const size_t         cookieSize,
                        rserpool_session_t   sessionID,
                        unsigned long long   timeout)
{
   struct RSerPoolSocket*  rserpoolSocket;
   struct Session*         session;
   struct RSerPoolMessage* message;
   bool                    result = false;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   threadSafetyLock(&rserpoolSocket->Mutex);

   session = findSession(rserpoolSocket, sessionID, 0);
   if(session != NULL) {
      LOG_VERBOSE1
      fprintf(stdlog, "Trying to send ASAP_COOKIE via session %u of RSerPool socket %d, socket %d\n",
              session->SessionID,
              rserpoolSocket->Descriptor, rserpoolSocket->Socket);
      LOG_END

      message = rserpoolMessageNew(NULL,256 + cookieSize);
      if(message != NULL) {
         message->Type       = AHT_COOKIE;
         message->CookiePtr  = (char*)cookie;
         message->CookieSize = (size_t)cookieSize;
         threadSafetyLock(&rserpoolSocket->Mutex);
         result = rserpoolMessageSend(IPPROTO_SCTP,
                                      rserpoolSocket->Socket,
                                      session->AssocID,
#ifdef MSG_NOSIGNAL
                                      MSG_NOSIGNAL,
#else
                                      0,
#endif
                                      timeout,
                                      message);
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         rserpoolMessageDelete(message);
      }
   }
   else {
      errno = EINVAL;
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return((result == true) ? (ssize_t)cookieSize : -1);
}


/* ###### RSerPool socket read() implementation ########################## */
ssize_t rsp_read(int fd, void* buffer, size_t bufferLength)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    flags = 0;
   GET_RSERPOOL_SOCKET(rserpoolSocket, fd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      return(rsp_recvmsg(fd, buffer, bufferLength, NULL, &flags, ~0));
   }
   return(ext_read(rserpoolSocket->Socket, buffer, bufferLength));
}


/* ###### RSerPool socket write() implementation ######################### */
ssize_t rsp_write(int fd, const char* buffer, size_t bufferLength)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, fd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      return(rsp_sendmsg(fd, buffer, bufferLength, 0, 0, 0, 0, 0, ~0));
   }
   return(ext_write(rserpoolSocket->Socket, buffer, bufferLength));
}


/* ###### RSerPool socket recv() implementation ########################## */
ssize_t rsp_recv(int sd, void* buffer, size_t bufferLength, int flags)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      return(rsp_recvmsg(sd, buffer, bufferLength, NULL, &flags, ~0));
   }
   return(ext_read(rserpoolSocket->Socket, buffer, bufferLength));
}


/* ###### RSerPool socket send() implementation ########################## */
ssize_t rsp_send(int sd, const void* buffer, size_t bufferLength, int flags)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      return(rsp_sendmsg(sd, buffer, bufferLength, flags, 0, 0, 0, 0, ~0));
   }
   return(ext_send(rserpoolSocket->Socket, buffer, bufferLength, flags));
}


/* ###### RSerPool socket getsockopt() implementation #################### */
int rsp_getsockopt(int sd, int level, int optname, void* optval, socklen_t* optlen)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    result = -1;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      threadSafetyLock(&rserpoolSocket->Mutex);
      // ...
      threadSafetyUnlock(&rserpoolSocket->Mutex);
      return(result);
   }
   return(ext_getsockopt(rserpoolSocket->Socket, level, optname, optval, optlen));
}


/* ###### RSerPool socket setsockopt() implementation #################### */
int rsp_setsockopt(int sd, int level, int optname, const void* optval, socklen_t optlen)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    result = -1;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      threadSafetyLock(&rserpoolSocket->Mutex);
      // ...
      threadSafetyUnlock(&rserpoolSocket->Mutex);
      return(result);
   }
   return(ext_setsockopt(rserpoolSocket->Socket, level, optname, optval, optlen));
}


/* ###### Set session's CSP status text ################################## */
int rsp_csp_setstatus(int                sd,
                      rserpool_session_t sessionID,
                      const char*        statusText)
{
#ifdef ENABLE_CSP
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   int                    result = 0;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   threadSafetyLock(&rserpoolSocket->Mutex);
   session = findSession(rserpoolSocket, sessionID, 0);
   if(session != NULL) {
      safestrcpy((char*)&session->StatusText,
                 statusText,
                 sizeof(session->StatusText));
   }
   else {
      errno  = EINVAL;
      result = -1;
   }
   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(result);
#else
   errno = EPROTONOSUPPORT;
   return(-1);
#endif
}


#ifdef ENABLE_CSP
/* ###### Get CSP status ################################################# */
size_t getSessionStatus(struct ComponentAssociation** caeArray,
                        unsigned long long*           identifier,
                        char*                         statusText,
                        char*                         componentLocation,
                        double*                       workload,
                        const int                     registrarSocket,
                        const uint32_t                registrarID,
                        const unsigned long long      registrarConnectionTimeStamp)
{
   size_t                 caeArraySize;
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   size_t                 sessions;

   LOG_VERBOSE3
   fputs("Getting Component Status...\n", stdlog);
   LOG_END

   threadSafetyLock(&gRSerPoolSocketSetMutex);

   sessions = 0;
   rserpoolSocket = (struct RSerPoolSocket*)leafLinkedRedBlackTreeGetFirst(&gRSerPoolSocketSet);
   while(rserpoolSocket != NULL) {
      sessions += sessionStorageGetElements(&rserpoolSocket->SessionSet);
      rserpoolSocket = (struct RSerPoolSocket*)leafLinkedRedBlackTreeGetNext(&gRSerPoolSocketSet, &rserpoolSocket->Node);
   }

   *workload    = -1.0;
   *caeArray    = createComponentAssociationArray(1 + sessions);
   caeArraySize = 0;
   if(*caeArray) {
      statusText[0]        = 0x00;
      componentLocation[0] = 0x00;
      if(registrarSocket >= 0) {
         (*caeArray)[caeArraySize].ReceiverID = CID_COMPOUND(CID_GROUP_REGISTRAR, registrarID);
         (*caeArray)[caeArraySize].Duration   = getMicroTime() - registrarConnectionTimeStamp;
         (*caeArray)[caeArraySize].Flags      = 0;
         (*caeArray)[caeArraySize].ProtocolID = IPPROTO_SCTP;
         (*caeArray)[caeArraySize].PPID       = PPID_ASAP;
         caeArraySize++;
      }
      getComponentLocation(componentLocation, -1, 0);

      rserpoolSocket = (struct RSerPoolSocket*)leafLinkedRedBlackTreeGetFirst(&gRSerPoolSocketSet);
      while(rserpoolSocket != NULL) {
         session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
         while(session != NULL) {
            if((!session->IsIncoming) &&
               (session->ConnectedPE != 0)) {
               (*caeArray)[caeArraySize].ReceiverID = CID_COMPOUND(CID_GROUP_POOLELEMENT, session->ConnectedPE);
               (*caeArray)[caeArraySize].Duration   = (session->ConnectionTimeStamp > 0) ? (getMicroTime() - session->ConnectionTimeStamp) : ~0ULL;
               (*caeArray)[caeArraySize].Flags      = 0;
               (*caeArray)[caeArraySize].ProtocolID = rserpoolSocket->SocketProtocol;
               (*caeArray)[caeArraySize].PPID       = 0;
               caeArraySize++;
               getComponentLocation(componentLocation, rserpoolSocket->Socket, session->AssocID);
            }
            if(session->StatusText[0] != 0x00) {
               safestrcpy(statusText,
                          session->StatusText,
                          CSPR_STATUS_SIZE);
            }
            session = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
         }
         if((rserpoolSocket->PoolElement) &&
            (CID_GROUP(*identifier) == CID_GROUP_POOLELEMENT) &&
            (CID_OBJECT(*identifier) == 0)) {
            *identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, rserpoolSocket->PoolElement->Identifier);
         }
         rserpoolSocket = (struct RSerPoolSocket*)leafLinkedRedBlackTreeGetNext(&gRSerPoolSocketSet, &rserpoolSocket->Node);
      }

      if((statusText[0] == 0x00) || (sessions != 1)) {
         snprintf(statusText, CSPR_STATUS_SIZE,
                  "%u Session%s", (unsigned int)sessions,
                  (sessions == 1) ? "" : "s");
      }
   }

   threadSafetyUnlock(&gRSerPoolSocketSetMutex);

   return(caeArraySize);
}
#endif


// ###### Print RSerPool notification #######################################
void rsp_print_notification(const union rserpool_notification* notification, FILE* fd)
{
   switch(notification->rn_header.rn_type) {
      case RSERPOOL_SESSION_CHANGE:
         fprintf(fd, "RSERPOOL_SESSION_CHANGE for session %d, state=",
                 notification->rn_session_change.rsc_session);
         switch(notification->rn_session_change.rsc_state) {
            case RSERPOOL_SESSION_ADD:
               fputs("RSERPOOL_SESSION_ADD", fd);
             break;
            case RSERPOOL_SESSION_REMOVE:
               fputs("RSERPOOL_SESSION_REMOVE", fd);
             break;
            default:
               fprintf(fd, "Unknown state %d!\n", notification->rn_session_change.rsc_state);
             break;
         }
       break;

      case RSERPOOL_FAILOVER:
         fprintf(fd, "RSERPOOL_FAILOVER for session %d, state=",
                 notification->rn_failover.rf_session);
         switch(notification->rn_failover.rf_state) {
            case RSERPOOL_FAILOVER_NECESSARY:
               fputs("RSERPOOL_FAILOVER_NECESSARY", fd);
             break;
            case RSERPOOL_FAILOVER_COMPLETE:
               fputs("RSERPOOL_FAILOVER_COMPLETE", fd);
             break;
            default:
               fprintf(fd, "Unknown state %d!\n", notification->rn_failover.rf_state);
             break;
          }
        break;

      case RSERPOOL_SHUTDOWN_EVENT:
         fprintf(fd, "RSERPOOL_SHUTDOWN_EVENT for session %d",
                 notification->rn_shutdown_event.rse_session);
       break;

      default:
         fprintf(fd, "Unknown type %d!\n", notification->rn_header.rn_type);
       break;
   }
}
