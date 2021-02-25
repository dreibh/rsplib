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
#include "rserpool-internals.h"
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


extern struct ASAPInstance*      gAsapInstance;
extern struct Dispatcher         gDispatcher;
extern struct SimpleRedBlackTree gRSerPoolSocketSet;
extern struct ThreadSafety       gRSerPoolSocketSetMutex;
extern struct IdentifierBitmap*  gRSerPoolSocketAllocationBitmap;


#define GET_RSERPOOL_SOCKET(rserpoolSocket, sd) \
   rserpoolSocket = getRSerPoolSocketForDescriptor(sd); \
   if(rserpoolSocket == NULL) { \
      errno = EBADF; \
      return(-1); \
   }


/* ###### Configure notifications of SCTP socket ############################# */
static bool configureSCTPSocket(struct RSerPoolSocket* rserpoolSocket,
                                int                    sd,
                                sctp_assoc_t           assocID)
{
   struct sctp_event_subscribe events;

   memset((char*)&events, 0 ,sizeof(events));
   events.sctp_data_io_event     = 1;
   events.sctp_association_event = 1;
   events.sctp_shutdown_event    = 1;
   if(ext_setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events)) < 0) {
      LOG_ERROR
      logerror("setsockopt failed for SCTP_EVENTS");
      LOG_END
      return(false);
   }

   if(tuneSCTP(sd, 0, &rserpoolSocket->AssocParameters) == false) {
      LOG_WARNING
      fputs("Unable to tune association parameters\n", stdout);
      LOG_END
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

   /* ====== Initialize RSerPool socket entry ============================ */
   rserpoolSocket = (struct RSerPoolSocket*)malloc(sizeof(struct RSerPoolSocket));
   if(rserpoolSocket == NULL) {
      close(fd);
      errno = ENOMEM;
      return(-1);
   }

   /* ====== Allocate message buffer ===================================== */
   rserpoolSocket->MsgBuffer = messageBufferNew(RSERPOOL_MESSAGE_BUFFER_SIZE, true);
   if(rserpoolSocket->MsgBuffer == NULL) {
      free(rserpoolSocket);
      close(fd);
      errno = ENOMEM;
      return(-1);
   }

   /* ====== Create session ID allocation bitmap ========================= */
   rserpoolSocket->SessionAllocationBitmap = identifierBitmapNew(SESSION_SETSIZE);
   if(rserpoolSocket->SessionAllocationBitmap == NULL) {
      free(rserpoolSocket->MsgBuffer);
      free(rserpoolSocket);
      close(fd);
      errno = ENOMEM;
      return(-1);
   }
   CHECK(identifierBitmapAllocateSpecificID(rserpoolSocket->SessionAllocationBitmap, 0) == 0);

   threadSafetyNew(&rserpoolSocket->Mutex, "RSerPoolSocket");
   threadSafetyNew(&rserpoolSocket->SessionSetMutex, "SessionSet");
   simpleRedBlackTreeNodeNew(&rserpoolSocket->Node);
   sessionStorageNew(&rserpoolSocket->SessionSet);
   rserpoolSocket->Socket                             = fd;
   rserpoolSocket->SocketDomain                       = domain;
   rserpoolSocket->SocketType                         = type;
   rserpoolSocket->SocketProtocol                     = protocol;
   rserpoolSocket->Descriptor                         = -1;
   rserpoolSocket->PoolElement                        = NULL;
   rserpoolSocket->ConnectedSession                   = NULL;
   rserpoolSocket->WaitingForFirstMsg                 = false;

   rserpoolSocket->AssocParameters.InitialRTO         = 2000;
   rserpoolSocket->AssocParameters.MinRTO             = 1000;
   rserpoolSocket->AssocParameters.MaxRTO             = 5000;
   rserpoolSocket->AssocParameters.AssocMaxRxt        = 8;
   rserpoolSocket->AssocParameters.PathMaxRxt         = 3;
   rserpoolSocket->AssocParameters.HeartbeatInterval  = 15000;

   notificationQueueNew(&rserpoolSocket->Notifications);
   if(rserpoolSocket->SocketType == SOCK_STREAM) {
      rserpoolSocket->Notifications.EventMask = 0;
   }
   else {
      rserpoolSocket->Notifications.EventMask = NET_NOTIFICATION_MASK;
   }

   /* ====== Configure SCTP socket ======================================= */
   if(!configureSCTPSocket(rserpoolSocket, fd, 0)) {
      free(rserpoolSocket->MsgBuffer);
      free(rserpoolSocket);
      close(fd);
      return(-1);
   }

   /* ====== Find available RSerPool socket descriptor =================== */
   threadSafetyLock(&gRSerPoolSocketSetMutex);
   rserpoolSocket->Descriptor = identifierBitmapAllocateID(gRSerPoolSocketAllocationBitmap);
   if(rserpoolSocket->Descriptor >= 0) {
      /* ====== Add RSerPool socket entry ================================ */
      CHECK(simpleRedBlackTreeInsert(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
   }
   threadSafetyUnlock(&gRSerPoolSocketSetMutex);

   /* ====== Has there been a problem? =================================== */
   if(rserpoolSocket->Descriptor < 0) {
      identifierBitmapDelete(rserpoolSocket->SessionAllocationBitmap);
      free(rserpoolSocket->MsgBuffer);
      free(rserpoolSocket);
      close(fd);
      errno = EMFILE;
      return(-1);
   }
   return(rserpoolSocket->Descriptor);
}


/* ###### Update session parameters ###################################### */
int rsp_update_session_parameters(int sd,
                                  struct rserpool_session_parameters* params)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd)

#define CHECK_AND_SET(x,y) if(y != 0) { x = y; } else { y = x; }

   threadSafetyLock(&rserpoolSocket->Mutex);
   CHECK_AND_SET(rserpoolSocket->AssocParameters.InitialRTO,        params->sp_rto_initial);
   CHECK_AND_SET(rserpoolSocket->AssocParameters.MinRTO,            params->sp_rto_min);
   CHECK_AND_SET(rserpoolSocket->AssocParameters.MaxRTO,            params->sp_rto_max);
   CHECK_AND_SET(rserpoolSocket->AssocParameters.AssocMaxRxt,       params->sp_assoc_max_rxt);
   CHECK_AND_SET(rserpoolSocket->AssocParameters.PathMaxRxt,        params->sp_path_max_rxt);
   CHECK_AND_SET(rserpoolSocket->AssocParameters.HeartbeatInterval, params->sp_hbinterval);
   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(0);
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
      rsp_deregister(sd, 0);
   }
   session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
   while(session != NULL) {
      nextSession = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
      LOG_ACTION
      fprintf(stdlog, "RSerPool socket %d, socket %d has open session %u -> closing it\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              session->SessionID);
      LOG_END
      /* Send SHUTDOWN to peer */
      sendshutdown(rserpoolSocket->Socket, session->AssocID);
      deleteSession(rserpoolSocket, session);
      session = nextSession;
   }

   /* ====== Delete RSerPool socket entry ================================ */
   threadSafetyLock(&gRSerPoolSocketSetMutex);
   CHECK(simpleRedBlackTreeRemove(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
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
   if(rserpoolSocket->MsgBuffer) {
      messageBufferDelete(rserpoolSocket->MsgBuffer);
      rserpoolSocket->MsgBuffer = NULL;
   }
   threadSafetyDelete(&rserpoolSocket->SessionSetMutex);
   threadSafetyUnlock(&rserpoolSocket->Mutex);
   threadSafetyDelete(&rserpoolSocket->Mutex);
   free(rserpoolSocket);
   return(0);
}


/* ###### Map socket descriptor into RSerPool socket ##################### */
int rsp_mapsocket(int sd, int toSD)
{
   struct RSerPoolSocket* rserpoolSocket;

   /* ====== Check for problems ========================================== */
   if((sd < 0) || (sd >= (int)FD_SETSIZE)) {
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
   sessionStorageNew(&rserpoolSocket->SessionSet);
   notificationQueueNew(&rserpoolSocket->Notifications);

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
      CHECK(simpleRedBlackTreeInsert(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
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
      CHECK(simpleRedBlackTreeRemove(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
      identifierBitmapFreeID(gRSerPoolSocketAllocationBitmap, sd);
      rserpoolSocket->Descriptor = -1;
      threadSafetyUnlock(&gRSerPoolSocketSetMutex);
      sessionStorageDelete(&rserpoolSocket->SessionSet);
      notificationQueueDelete(&rserpoolSocket->Notifications);
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

   /* ====== Do poll() =================================================== */
   if(result == 0) {
      /* Only call poll() when there are no notifications */
      result = ext_poll(ufds, nfds, timeout);
   }

   /* ====== Handle results ============================================== */
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
   int           nfds;
   int           waitingTime;
   int           result;
   int           i;

   /* ====== Check for problems ========================================== */
   if(n > (int)FD_SETSIZE) {
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
   waitingTime = (1000 * timeout->tv_sec) + (int)ceil((double)timeout->tv_usec / 1000.0);
   result = rsp_poll((struct pollfd*)&ufds, nfds, waitingTime);
   if(result > 0) {
      for(i = 0;i < nfds;i++) {
         if( (!(ufds[i].events & POLLIN)) && (readfds) ) {
            FD_CLR(ufds[i].fd, readfds);
         }
         if( (!(ufds[i].events & POLLOUT)) && (writefds) ) {
            FD_CLR(ufds[i].fd, writefds);
         }
         if( (!(ufds[i].events & (POLLIN|POLLHUP|POLLNVAL))) && (exceptfds) ) {
            FD_CLR(ufds[i].fd, exceptfds);
         }
      }
   }

   return(result);
}


/* ###### Bind RSerPool socket ########################################### */
int rsp_bind(int sd, const struct sockaddr* addrs, int addrcnt)
{
   struct RSerPoolSocket* rserpoolSocket;
   union sockaddr_union*  unpackedAddresses;
   union sockaddr_union   localAddress;
   int                    result;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   /* ====== Bind socket ================================================= */
   if(addrcnt < 1) {
      addrs   = (struct sockaddr*)&localAddress;
      addrcnt = 1;
      memset((void*)&localAddress, 0, sizeof(localAddress));
      ((struct sockaddr*)&localAddress)->sa_family = rserpoolSocket->SocketDomain;
   }
   unpackedAddresses = unpack_sockaddr(addrs, addrcnt);
   if(unpackedAddresses == NULL) {
      errno = ENOMEM;
      return(-1);
   }
   result = bindplus(rserpoolSocket->Socket, unpackedAddresses, addrcnt);
   free(unpackedAddresses);
   if(result == false) {
      LOG_ERROR
      logerror("Unable to bind socket for new pool element");
      LOG_END
      return(-1);
   }
   return(0);
}


/* ###### Put PE socket into listening mode ############################## */
int rsp_listen(int sd, int backlog)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   /* printf("backlog(%d)=%d\n", sd, backlog); */
   return(ext_listen(rserpoolSocket->Socket, backlog));
}


/* ###### Register pool element ########################################## */
int rsp_register_tags(int                        sd,
                      const unsigned char*       poolHandle,
                      const size_t               poolHandleSize,
                      const struct rsp_loadinfo* loadinfo,
                      unsigned int               reregistrationInterval,
                      int                        flags,
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
      threadSafetyUnlock(&rserpoolSocket->PoolElement->Mutex);

      /* ====== Schedule reregistration as soon as possible ============== */
      timerRestart(&rserpoolSocket->PoolElement->ReregistrationTimer, 0);
   }

   /* ====== Registration of a new pool element ========================== */
   else {
      /* ====== Create pool element ====================================== */
      rserpoolSocket->PoolElement = (struct PoolElement*)malloc(sizeof(struct PoolElement));
      if(rserpoolSocket->PoolElement == NULL) {
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         return(-1);
      }
      threadSafetyNew(&rserpoolSocket->PoolElement->Mutex, "PoolElement");
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
      rserpoolSocket->PoolElement->HasControlChannel      = true;
      rserpoolSocket->PoolElement->InDaemonMode           = (flags & REGF_DAEMONMODE);

      /* ====== Do registration ============================================= */
      if(doRegistration(rserpoolSocket, true) == false) {
         LOG_ERROR
         fputs("Unable to obtain registration information -> Creating pool element not possible\n", stdlog);
         LOG_END
         if(!(flags & REGF_DAEMONMODE)) {
            deletePoolElement(rserpoolSocket->PoolElement, flags, tags);
            rserpoolSocket->PoolElement = NULL;
            threadSafetyUnlock(&rserpoolSocket->Mutex);
            return(-1);
         }
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
                 unsigned int               reregistrationInterval,
                 int                        flags)
{
   return(rsp_register_tags(sd, poolHandle, poolHandleSize, loadinfo,
                            reregistrationInterval, flags, NULL));
}


/* ###### Deregister pool element ######################################## */
int rsp_deregister_tags(int sd, int flags, struct TagItem* tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    result = 0;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   /* ====== Delete pool element ========================================= */
   threadSafetyLock(&rserpoolSocket->Mutex);
   if(rserpoolSocket->PoolElement != NULL) {
      deletePoolElement(rserpoolSocket->PoolElement, flags, tags);
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
int rsp_deregister(int sd, int flags)
{
   return(rsp_deregister_tags(sd, flags, NULL));
}


/* ###### RSerPool socket connect() implementation ####################### */
int rsp_connect_tags(int                  sd,
                     const unsigned char* poolHandle,
                     const size_t         poolHandleSize,
                     const unsigned int   staleCacheValue,
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
         /* Session Change events are only useful for handling
            multiple incoming sessions. */
         rserpoolSocket->Notifications.EventMask &= ~NET_SESSION_CHANGE;

         /* ====== Connect to a PE ======================================= */
         /* Do not signalize successful failover, since this is the first
            connection establishment */
         oldEventMask = rserpoolSocket->Notifications.EventMask;
         rserpoolSocket->Notifications.EventMask = 0;
         rsp_forcefailover_tags(rserpoolSocket->Descriptor, FFF_NONE, staleCacheValue, tags);
         rserpoolSocket->Notifications.EventMask = oldEventMask;
         result = 0;
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
                const size_t         poolHandleSize,
                const unsigned int   staleCacheValue)
{
   return(rsp_connect_tags(sd, poolHandle, poolHandleSize, staleCacheValue, NULL));
}


/* ###### RSerPool socket accept() implementation ######################## */
int rsp_accept_tags(int             sd,
                    int             timeout,
                    struct TagItem* tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct RSerPoolSocket* newRSerPoolSocket;
   struct Session*        session;
   union sockaddr_union   remoteAddress;
   socklen_t              remoteAddressSize;
   int                    result = -1;
   int                    newSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   if(waitForRead(rserpoolSocket, timeout)) {
      remoteAddressSize = sizeof(remoteAddress);
      newSocket = ext_accept(rserpoolSocket->Socket, &remoteAddress.sa, &remoteAddressSize);
      if(newSocket >= 0) {
         LOG_VERBOSE
         fprintf(stdlog, "Accepted new socket %d on RSerPool socket %u, socket %d (",
                newSocket, rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         fputaddress(&remoteAddress.sa, true, stdlog);
         fputs(")\n", stdlog);
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
               fprintf(stdlog, "Accepted new session %u from ", session->SessionID);
               fputaddress(&remoteAddress.sa, true, stdlog);
               fprintf(stdlog, " (RSerPool socket %u, socket %u) on RSerPool socket %u, socket %d\n",
                     newRSerPoolSocket->Descriptor, newRSerPoolSocket->Socket,
                     rserpoolSocket->Descriptor, rserpoolSocket->Socket);
               LOG_END

/*
               struct sctp_setpeerprim setPeerPrimary;
               setPeerPrimary.sspp_assoc_id = 0;
               memcpy(&setPeerPrimary.sspp_addr, &remoteAddress, sizeof(union sockaddr_union));
               if(ext_setsockopt(newRSerPoolSocket->Socket,
                                 IPPROTO_SCTP, SCTP_SET_PEER_PRIMARY_ADDR,
                                 &setPeerPrimary, sizeof(setPeerPrimary)) < 0) {
                  LOG_WARNING
                  logerror("setsockopt SCTP_SET_PEER_PRIMARY_ADDR failed");
                  LOG_END
               }
*/
            }
         }
      }
   }

   return(result);
}


/* ###### RSerPool socket accept() implementation ######################## */
int rsp_accept(int sd,
               int timeout)
{
   return(rsp_accept_tags(sd, timeout, NULL));
}


/* ###### Establish association to a new PE ############################## */
static int connectToPE(struct RSerPoolSocket* rserpoolSocket,
                       const struct sockaddr* destinationAddressArray,
                       const size_t           destinationAddresses)
{
   struct pollfd        ufds;
   union sockaddr_union peerAddress;
   socklen_t            peerAddressLength;
   int                  result;
   int                  sd;

   sd = ext_socket(rserpoolSocket->SocketDomain,
                   SOCK_STREAM,
                   rserpoolSocket->SocketProtocol);
   if(sd >= 0) {
      setNonBlocking(sd);
      if(configureSCTPSocket(rserpoolSocket, sd, 0)) {
         LOG_VERBOSE
         fprintf(stdlog, "Trying connection to pool element $%08x (", rserpoolSocket->ConnectedSession->ConnectedPE);
         poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
         fprintf(stdlog, "/$%08x on RSerPool socket %u, socket %d, session %u)\n",
                 rserpoolSocket->ConnectedSession->ConnectedPE,
                 rserpoolSocket->Descriptor, sd,
                 rserpoolSocket->ConnectedSession->SessionID);
         LOG_END

         result = sctp_connectx(sd, (struct sockaddr*)destinationAddressArray, destinationAddresses, NULL);
         if((result == 0) || (errno == EINPROGRESS)) {
            ufds.fd     = sd;
            ufds.events = POLLIN;
            if(ext_poll(&ufds, 1, (int)(rserpoolSocket->ConnectedSession->ConnectTimeout / 1000)) > 0) {
               if(ufds.revents & POLLIN) {
                  peerAddressLength = sizeof(peerAddress);
                  if(ext_getsockname(sd, &peerAddress.sa, &peerAddressLength) == 0) {
                     return(sd);
                  }
               }
            }
         }

         LOG_VERBOSE
         fprintf(stdlog, "Failed to establish connection to pool element $%08x (", rserpoolSocket->ConnectedSession->ConnectedPE);
         poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
         fprintf(stdlog, "/$%08x on RSerPool socket %u, socket %d, session %u)\n",
                 rserpoolSocket->ConnectedSession->ConnectedPE,
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                 rserpoolSocket->ConnectedSession->SessionID);
         LOG_END
      }
      ext_close(sd);
   }
   return(-1);
}


/* ###### Check if cookie is available ################################### */
int rsp_has_cookie(int sd)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    result;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   threadSafetyLock(&rserpoolSocket->Mutex);
   result = (rserpoolSocket->ConnectedSession->CookieSize > 0);
   threadSafetyUnlock(&rserpoolSocket->Mutex);

   return(result);
}


/* ###### Report PE as failed ############################################ */
static void rsp_send_failure_report(struct RSerPoolSocket* rserpoolSocket,
                                    struct TagItem*        tags)
{
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
}


/* ###### Make failover ################################################## */
int rsp_forcefailover_tags(int                sd,
                           const unsigned int flags,
                           const unsigned int staleCacheValue,
                           struct TagItem*    tags)
{
   struct RSerPoolSocket*   rserpoolSocket;
   struct rsp_addrinfo*     rspAddrInfo;
   struct NotificationNode* notificationNode;
   int                      result;
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

   LOG_NOTE
   fprintf(stdlog, "Starting failover for RSerPool socket %u, socket %d, assoc %u\n",
           sd, rserpoolSocket->Socket, (unsigned int)rserpoolSocket->ConnectedSession->AssocID);
   LOG_END

   /* When next rsp_sendmsg() fails, a new FAILOVER_NECESSARY notification
      has to be sent. */
   rserpoolSocket->ConnectedSession->IsFailed = false;
   /* But for now, remove all queued notifications - they are out of date! */
   notificationQueueClear(&rserpoolSocket->Notifications);

   /* ====== Report failure ============================================== */
   if((flags & FFF_UNREACHABLE) &&
      (rserpoolSocket->ConnectedSession->ConnectedPE != 0) &&
      (rserpoolSocket->ConnectedSession->Handle.Size > 0)) {
      rsp_send_failure_report(rserpoolSocket, tags);
   }

   /* ====== Abort association =========================================== */
   if(rserpoolSocket->Socket >= 0) {
      LOG_ACTION
      fprintf(stdlog, "Aborting association on RSerPool socket %u, socket %d, session %u\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              (unsigned int)rserpoolSocket->ConnectedSession->AssocID);
      LOG_END
      sendabort(rserpoolSocket->Socket,
                rserpoolSocket->ConnectedSession->AssocID);
      rserpoolSocket->ConnectedSession->AssocID = 0;
      ext_close(rserpoolSocket->Socket);
      rserpoolSocket->Socket = -1;
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
                                    &rspAddrInfo, 1, staleCacheValue,
                                    tags);
      if(result > 0) {
         if(rspAddrInfo->ai_protocol == rserpoolSocket->SocketProtocol) {
            /* ====== Establish connection ================================== */
            rserpoolSocket->ConnectedSession->ConnectedPE = rspAddrInfo->ai_pe_id;
            rserpoolSocket->Socket = connectToPE(rserpoolSocket,
                                                 rspAddrInfo->ai_addr,
                                                 rspAddrInfo->ai_addrs);
            if(rserpoolSocket->Socket >= 0) {
               success = true;
               rserpoolSocket->WaitingForFirstMsg = true;
               sessionStorageUpdateSession(&rserpoolSocket->SessionSet,
                                           rserpoolSocket->ConnectedSession,
                                           0);
            }

            /* ====== Free handle resolution result ====================== */
            rsp_freeaddrinfo(rspAddrInfo);

            /* ====== Failover procedure ================================= */
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
                  notificationNode->Content.rn_failover.rf_has_cookie = (rserpoolSocket->ConnectedSession->CookieSize > 0);
               }
            }
            else {
               LOG_ACTION
               fputs("Connection establishment was not possible\n", stdlog);
               LOG_END
               rsp_send_failure_report(rserpoolSocket, tags);
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
      else if(result == REAI_NONAME) {
         LOG_ACTION
         fprintf(stdlog,
                 "Handle resolution did not find new pool element. Waiting %lluus...\n",
                 rserpoolSocket->ConnectedSession->HandleResolutionRetryDelay);
         LOG_END
         errno = ENOENT;
         // Release lock before waiting with usleep()!
#ifdef ENABLE_CSP
         syncSessionStatus(rserpoolSocket, rserpoolSocket->ConnectedSession);
#endif
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         usleep((unsigned int)rserpoolSocket->ConnectedSession->HandleResolutionRetryDelay);
         return(false);
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

#ifdef ENABLE_CSP
   syncSessionStatus(rserpoolSocket, rserpoolSocket->ConnectedSession);
#endif

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return((success == true) ? 0 : -1);
}


/* ###### Make failover ################################################## */
int rsp_forcefailover(int                sd,
                      const unsigned int flags,
                      const unsigned int staleCacheValue)
{
   return(rsp_forcefailover_tags(sd, flags, staleCacheValue, NULL));
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
                    uint16_t           sctpFlags,
                    int                timeout)
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
         session->PPID = ntohl(sctpPPID);
         result = sendtoplus(rserpoolSocket->Socket, data, dataLength,
#ifdef MSG_NOSIGNAL
                             msg_flags|MSG_NOSIGNAL,
#else
                             msg_flags,
#endif
                             NULL, 0,
                             ntohl(sctpPPID), session->AssocID, sctpStreamID, sctpTimeToLive, sctpFlags,
                             (timeout >= 0) ? (1000ULL * timeout) : 0);
         if((result < 0) && (errno != EAGAIN)) {
            LOG_ACTION
            fprintf(stdlog, "Session failure during send on RSerPool socket %d, session %u: %s. Failover necessary\n",
                    rserpoolSocket->Descriptor, session->SessionID, strerror(errno));
            LOG_END

            /* ====== Terminate association and notify application ======= */
            notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                                    false, RSERPOOL_FAILOVER);
            if(notificationNode) {
               notificationNode->Content.rn_failover.rf_state      = RSERPOOL_FAILOVER_NECESSARY;
               notificationNode->Content.rn_failover.rf_session    = session->SessionID;
               notificationNode->Content.rn_failover.rf_has_cookie = (session->CookieSize > 0);
            }

            session->IsFailed = true;
            /* =========================================================== */

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
                    int                    timeout)
{
   struct RSerPoolSocket*         rserpoolSocket;
   struct Session*                session;
   struct rsp_sndrcvinfo          rinfoDummy;
   const union sctp_notification* notification;
   union sockaddr_union           remoteAddress;
   socklen_t                      remoteAddressSize;
   sctp_assoc_t                   assocID;
   int                            flags;
   int                            errorCode;
   ssize_t                        received;
   ssize_t                        received2;
   unsigned long long             startTimeStamp;
   unsigned long long             currentTimeout;
   unsigned long long             now;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   if(rinfo == NULL) {
      /* User is not interested in values. */
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
      received = getCookieEchoOrNotification(rserpoolSocket, buffer, bufferLength,
                                             rinfo, msg_flags, true);
      if(received > 0) {
         return(received);
      }
   }

   /* ====== Read from socket ========================================= */
   startTimeStamp = getMicroTime();
   do {
      if(rserpoolSocket->Socket >= 0) {
         now = getMicroTime();
         if((timeout >= 0) && (startTimeStamp + (1000ULL * timeout) >= now)) {
            currentTimeout = startTimeStamp + (1000ULL * timeout) - now;
         }
         else {
            currentTimeout = 0;
         }
         LOG_VERBOSE1
         fprintf(stdlog, "Trying to read from RSerPool socket %d, socket %d with timeout %Ldus\n",
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket, currentTimeout);
         LOG_END
         flags = 0;
         remoteAddressSize = sizeof(remoteAddress);
         received = messageBufferRead(rserpoolSocket->MsgBuffer,
                                      rserpoolSocket->Socket,
                                      &flags, &remoteAddress.sa, &remoteAddressSize,
                                      &rinfo->rinfo_ppid,
                                      &assocID,
                                      &rinfo->rinfo_stream,
                                      currentTimeout);
         errorCode = errno;
         LOG_VERBOSE
         fprintf(stdlog, "received=%d errno=%d\n", (int)received, errorCode);
         LOG_END

         /* Note, messageBufferRead()'s PPID byte order is host byte order! */
         rinfo->rinfo_ppid = htonl(rinfo->rinfo_ppid);

         if( (received == 0) ||
             ((received < 0) && (errorCode != EAGAIN)) ) {
            /* A failover is necessary due to error or shutdown. */
            threadSafetyLock(&rserpoolSocket->Mutex);
            if(rserpoolSocket->ConnectedSession) {
               struct NotificationNode* notificationNode;
               notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                                       false, RSERPOOL_FAILOVER);
               if(notificationNode) {
                  notificationNode->Content.rn_failover.rf_state      = RSERPOOL_FAILOVER_NECESSARY;
                  notificationNode->Content.rn_failover.rf_session    = rserpoolSocket->ConnectedSession->SessionID;
                  notificationNode->Content.rn_failover.rf_has_cookie = (rserpoolSocket->ConnectedSession->CookieSize > 0);
               }
               rserpoolSocket->ConnectedSession->IsFailed = true;
            }
            threadSafetyUnlock(&rserpoolSocket->Mutex);
         }
      }
      else {
         LOG_VERBOSE
         fputs("No socket -> nothing to read from\n", stdlog);
         LOG_END

         received = 0;

         threadSafetyLock(&rserpoolSocket->Mutex);
         if(rserpoolSocket->ConnectedSession) {
            if(!rserpoolSocket->ConnectedSession->IsFailed) {
               struct NotificationNode* notificationNode;
               notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                                       false, RSERPOOL_FAILOVER);
               if(notificationNode) {
                  notificationNode->Content.rn_failover.rf_state      = RSERPOOL_FAILOVER_NECESSARY;
                  notificationNode->Content.rn_failover.rf_session    = rserpoolSocket->ConnectedSession->SessionID;
                  notificationNode->Content.rn_failover.rf_has_cookie = (rserpoolSocket->ConnectedSession->CookieSize > 0);
               }
               rserpoolSocket->ConnectedSession->IsFailed = true;
            }
         }
         threadSafetyUnlock(&rserpoolSocket->Mutex);
      }

      /* ====== Handle result =============================================== */
      if(received > 0) {
         threadSafetyLock(&rserpoolSocket->Mutex);

         /* ====== Handle notification ====================================== */
         if(flags & MSG_NOTIFICATION) {
            notification = (const union sctp_notification*)rserpoolSocket->MsgBuffer->Buffer;
            if(handleNotification(rserpoolSocket, notification) == true) {
               /* The association is closed here. Therefore, we return length 0
                  to signalize the disconnect. If there is a corresponding
                  notification, we will copy it later; otherwise, for
                  SOCK_STREAM, the 0 is returned back. */
               received = 0;
            }
            else {
               received = -1;
            }
         }

         /* ====== Handle ASAP control channel message =================== */
         else {
            if(rserpoolSocket->WaitingForFirstMsg) {
               rserpoolSocket->WaitingForFirstMsg = false;
#if defined(__LINUX__)
#ifdef HAVE_KERNEL_SCTP
#warning Using lksctp fix for possible bad primary path after connectx()!
/*
lksctp has a bug in connectx():
Using connectx() to connect to {A1, A2, A3}, the primary will always be A1;
even if A1 is bad and A2 or A3 had been used for 4-way handshake! Using
default SCTP settings of RTO.Max, Path.Max.Retrans, etc., it will take
*minutes* to drop the bad primary path.
Work-around: use the source address of the first incoming message as
             primary path.
*/

               LOG_NOTE
               fprintf(stdlog, "Setting primary for RSerPool socket %d, socket %d: ",
                       rserpoolSocket->Descriptor, rserpoolSocket->Socket);
               fputaddress(&remoteAddress.sa, true, stdlog);
               fputs("\n", stdlog);
               LOG_END

               struct sctp_setprim setPrimary;
               memset(&setPrimary, 0, sizeof(setPrimary));
               setPrimary.ssp_assoc_id = assocID;
               memcpy(&setPrimary.ssp_addr, &remoteAddress, sizeof(remoteAddress));
               if(ext_setsockopt(rserpoolSocket->Socket, IPPROTO_SCTP, SCTP_PRIMARY_ADDR,
                  &setPrimary, (socklen_t)sizeof(setPrimary)) < 0) {
                  LOG_VERBOSE
                  logerror("setsockopt SCTP_PRIMARY_ADDR failed");
                  LOG_END
               }
#endif
#endif
            }

            if( (flags & MSG_EOR) &&
                     (ntohl(rinfo->rinfo_ppid) == PPID_ASAP) ) {
               handleControlChannelMessage(rserpoolSocket, assocID,
                                           rserpoolSocket->MsgBuffer->Buffer, received);
               received = -1;
            }

            /* ====== User Data ========================================== */
            else {
               /* ====== Find session ==================================== */
               if(rserpoolSocket->ConnectedSession) {
                  rinfo->rinfo_session = rserpoolSocket->ConnectedSession->SessionID;
                  rinfo->rinfo_pe_id   = rserpoolSocket->ConnectedSession->ConnectedPE;
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

               /* ====== Copy user data ================================== */
               if(buffer != NULL) {
                  if((ssize_t)bufferLength < received) {
                     LOG_ERROR
                     fputs("Buffer is too small to keep full message\n", stdlog);
                     LOG_END
                     errno = ENOMEM;
                     return(-1);
                  }
                  received = min(received, (ssize_t)bufferLength);
                  memcpy(buffer, rserpoolSocket->MsgBuffer->Buffer, received);
               }
               *msg_flags |= MSG_EOR;
            }
         }

         threadSafetyUnlock(&rserpoolSocket->Mutex);
      }


      /* ====== Nothing read from socket, but there may be a notification === */
      if(received <= 0) {
         /* A cookie or notification may have been received. In this case,
            the if-blocks above have set received to -1! */

         /* ====== Give back Cookie Echo and notifications =============== */
         if(buffer != NULL) {
            received2 = getCookieEchoOrNotification(rserpoolSocket, buffer, bufferLength, rinfo, msg_flags, false);
            if(received2 > 0) {
               received = received2;
            }
         }
      }

      /* ====== Partial message ========================================== */
      if( (received < 0) &&
          (messageBufferHasPartial(rserpoolSocket->MsgBuffer)) &&
          (ntohl(rinfo->rinfo_ppid) != PPID_ASAP) ) {
         if(buffer != NULL) {
            if(bufferLength < rserpoolSocket->MsgBuffer->BufferPos) {
               LOG_ERROR
               fputs("Buffer is too small to keep even partial message\n", stdlog);
               LOG_END
               errno = ENOMEM;
               return(-1);
            }
            received = min(rserpoolSocket->MsgBuffer->BufferPos, bufferLength);
            memcpy(buffer, rserpoolSocket->MsgBuffer->Buffer, received);
         }
         messageBufferReset(rserpoolSocket->MsgBuffer);
         *msg_flags &= ~MSG_EOR;
         break;
      }
   } while((received < 0) &&
           (timeout >= 0) && (getMicroTime() < startTimeStamp + (1000ULL * timeout)));

   return(received);
}


/* ###### RSerPool socket read() implementation ########################## */
ssize_t rsp_read(int fd, void* buffer, size_t bufferLength)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    flags = 0;
   GET_RSERPOOL_SOCKET(rserpoolSocket, fd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      return(rsp_recvmsg(fd, buffer, bufferLength, NULL, &flags, -1));
   }
   return(ext_read(rserpoolSocket->Socket, buffer, bufferLength));
}


/* ###### Receive full message (until MSG_EOR) ########################### */
ssize_t rsp_recvfullmsg(int                    sd,
                        void*                  buffer,
                        size_t                 bufferLength,
                        struct rsp_sndrcvinfo* rinfo,
                        int*                   msg_flags,
                        int                    timeout)
{
   unsigned long long       now          = getMicroTime();
   const unsigned long long endTimeStamp = now  + (1000ULL * timeout);
   size_t                   offset       = 0;
   ssize_t                  received;

   while( ((received = rsp_recvmsg(sd, (char*)&((char*)buffer)[offset], bufferLength - offset,
                                   rinfo, msg_flags,
                                   ((endTimeStamp - now) > 0) ? (int)((endTimeStamp - now) / 1000) : 0)) > 0) &&
          (offset < bufferLength) ) {
      offset += received;
      if(*msg_flags & MSG_EOR) {
         return(offset);
      }
      now = getMicroTime();
   }
   return(received);
}


/* ###### RSerPool socket recv() implementation ########################## */
ssize_t rsp_recv(int sd, void* buffer, size_t bufferLength, int flags)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      return(rsp_recvmsg(sd, buffer, bufferLength, NULL, &flags, -1));
   }
   return(ext_read(rserpoolSocket->Socket, buffer, bufferLength));
}


/* ###### RSerPool socket send() implementation ########################## */
ssize_t rsp_send(int sd, const void* buffer, size_t bufferLength, int flags)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
#ifdef MSG_NOSIGNAL
   flags |= MSG_NOSIGNAL;
#endif
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      return(rsp_sendmsg(sd, buffer, bufferLength, flags, 0, 0, 0, 0, 0, -1));
   }
   return(ext_send(rserpoolSocket->Socket, buffer, bufferLength, flags));
}


/* ###### RSerPool socket write() implementation ######################### */
ssize_t rsp_write(int fd, const char* buffer, size_t bufferLength)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, fd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      return(rsp_sendmsg(fd, buffer, bufferLength, 0, 0, 0, 0, 0, 0, -1));
   }
   return(ext_write(rserpoolSocket->Socket, buffer, bufferLength));
}


/* ###### Send cookie ######################################################## */
ssize_t rsp_send_cookie(int                  sd,
                        const unsigned char* cookie,
                        const size_t         cookieSize,
                        rserpool_session_t   sessionID,
                        int                  timeout)
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
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         result = rserpoolMessageSend(IPPROTO_SCTP,
                                      rserpoolSocket->Socket,
                                      session->AssocID,
                                      0, 0,
                                      (1000ULL * timeout),
                                      message);
         threadSafetyLock(&rserpoolSocket->Mutex);
         rserpoolMessageDelete(message);
      }
   }
   else {
      errno = EINVAL;
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return((result == true) ? (ssize_t)cookieSize : -1);
}


/* ###### RSerPool socket getsockopt() implementation #################### */
int rsp_getsockopt(int sd, int level, int optname, void* optval, socklen_t* optlen)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    result = -1;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   if(rserpoolSocket->SessionAllocationBitmap != NULL) {
      threadSafetyLock(&rserpoolSocket->Mutex);
      /* ... */
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
      /* ... */
      threadSafetyUnlock(&rserpoolSocket->Mutex);
      return(result);
   }
   return(ext_setsockopt(rserpoolSocket->Socket, level, optname, optval, optlen));
}


/* ###### Get PE's pool handle and identifier ############################ */
int rsp_getsockname(int            sd,
                    unsigned char* poolHandle,
                    size_t*        poolHandleSize,
                    uint32_t*      identifier)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   if(rserpoolSocket->PoolElement == NULL) {
      errno = EBADF;
      return(-1);
   }
   if(poolHandleSize) {
      if(*poolHandleSize >= rserpoolSocket->PoolElement->Handle.Size) {
         memcpy(poolHandle,
               rserpoolSocket->PoolElement->Handle.Handle,
               rserpoolSocket->PoolElement->Handle.Size);
         *poolHandleSize = rserpoolSocket->PoolElement->Handle.Size;
      }
      else {
         errno = ENOBUFS;
         return(-1);
      }
   }
   if(identifier) {
      *identifier = rserpoolSocket->PoolElement->Identifier;
   }
   return(0);
}


/* ###### Get peer PE's pool handle and identifier ####################### */
int rsp_getpeername(int            sd,
                    unsigned char* poolHandle,
                    size_t*        poolHandleSize,
                    uint32_t*      identifier)
{
   struct RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   if(rserpoolSocket->ConnectedSession == NULL) {
      errno = EBADF;
      return(-1);
   }
   if(poolHandleSize) {
      if(*poolHandleSize >= rserpoolSocket->ConnectedSession->Handle.Size) {
         memcpy(poolHandle,
               rserpoolSocket->ConnectedSession->Handle.Handle,
               rserpoolSocket->ConnectedSession->Handle.Size);
         *poolHandleSize = rserpoolSocket->ConnectedSession->Handle.Size;
      }
      else {
         errno = ENOBUFS;
         return(-1);
      }
   }
   if(identifier) {
      *identifier = rserpoolSocket->ConnectedSession->ConnectedPE;
   }
   return(0);
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
                        uint64_t*                     identifier,
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
   bool                   gotLocation;

   LOG_VERBOSE3
   fputs("Getting Component Status...\n", stdlog);
   LOG_END

   threadSafetyLock(&gRSerPoolSocketSetMutex);

   /* ====== Obtain locks ================================================ */
   sessions = 0;
   rserpoolSocket = (struct RSerPoolSocket*)simpleRedBlackTreeGetFirst(&gRSerPoolSocketSet);
   while(rserpoolSocket != NULL) {
      threadSafetyLock(&rserpoolSocket->SessionSetMutex);
      sessions += sessionStorageGetElements(&rserpoolSocket->SessionSet);
      rserpoolSocket = (struct RSerPoolSocket*)simpleRedBlackTreeGetNext(&gRSerPoolSocketSet, &rserpoolSocket->Node);
   }

   /* ====== Get status ================================================== */
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
      gotLocation = false;

      rserpoolSocket = (struct RSerPoolSocket*)simpleRedBlackTreeGetFirst(&gRSerPoolSocketSet);
      while(rserpoolSocket != NULL) {
         session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
         while(session != NULL) {
            threadSafetyLock(&session->Status.Mutex);

            if((!session->Status.IsIncoming) &&
               (!session->Status.IsFailed)) {
               (*caeArray)[caeArraySize].ReceiverID = CID_COMPOUND(CID_GROUP_POOLELEMENT, session->Status.ConnectedPE);
               (*caeArray)[caeArraySize].Duration   = (session->Status.ConnectionTimeStamp > 0) ? (getMicroTime() - session->Status.ConnectionTimeStamp) : ~0ULL;
               (*caeArray)[caeArraySize].Flags      = 0;
               (*caeArray)[caeArraySize].ProtocolID = rserpoolSocket->SocketProtocol;
               (*caeArray)[caeArraySize].PPID       = session->Status.PPID;
               caeArraySize++;
               getComponentLocation(componentLocation, session->Status.Socket, session->Status.AssocID);
            }
            if(session->StatusText[0] != 0x00) {
               safestrcpy(statusText,
                          session->Status.StatusText,
                          CSPR_STATUS_SIZE);
            }

            threadSafetyUnlock(&session->Status.Mutex);
            session = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
         }
         if((rserpoolSocket->PoolElement) &&
            (CID_GROUP(*identifier) == CID_GROUP_POOLELEMENT)) {
            if(CID_OBJECT(*identifier) == 0) {
               *identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, rserpoolSocket->PoolElement->Identifier);
            }
            if(PPT_IS_ADAPTIVE(rserpoolSocket->PoolElement->LoadInfo.rli_policy)) {
               *workload = rserpoolSocket->PoolElement->LoadInfo.rli_load / (double)PPV_MAX_LOAD;
            }
            if(gotLocation == false) {
               getComponentLocation(componentLocation, rserpoolSocket->Socket, 0);
               gotLocation = true;
            }
         }
         rserpoolSocket = (struct RSerPoolSocket*)simpleRedBlackTreeGetNext(&gRSerPoolSocketSet, &rserpoolSocket->Node);
      }

      if(gotLocation == false) {
         getComponentLocation(componentLocation, -1, 0);
      }

      if((statusText[0] == 0x00) || (sessions != 1)) {
         snprintf(statusText, CSPR_STATUS_SIZE,
                  "%u Session%s", (unsigned int)sessions,
                  (sessions == 1) ? "" : "s");
      }
   }

   /* ====== Release locks ===============================================*/
   rserpoolSocket = (struct RSerPoolSocket*)simpleRedBlackTreeGetFirst(&gRSerPoolSocketSet);
   while(rserpoolSocket != NULL) {
      threadSafetyUnlock(&rserpoolSocket->SessionSetMutex);
      rserpoolSocket = (struct RSerPoolSocket*)simpleRedBlackTreeGetNext(&gRSerPoolSocketSet, &rserpoolSocket->Node);
   }
   threadSafetyUnlock(&gRSerPoolSocketSetMutex);

   return(caeArraySize);
}
#endif


/* ###### Print RSerPool notification #################################### */
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
               fprintf(fd, "Unknown state %d!", notification->rn_session_change.rsc_state);
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
               fprintf(fd, "Unknown state %d!", notification->rn_failover.rf_state);
             break;
          }
          fprintf(fd, ", cookie=%s",
                  (notification->rn_failover.rf_has_cookie) ? "yes" : "no");
        break;

      case RSERPOOL_SHUTDOWN_EVENT:
         fprintf(fd, "RSERPOOL_SHUTDOWN_EVENT for session %d",
                 notification->rn_shutdown_event.rse_session);
       break;

      default:
         fprintf(fd, "Unknown type %d!", notification->rn_header.rn_type);
       break;
   }
}
