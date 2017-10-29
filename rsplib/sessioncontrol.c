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
 * Copyright (C) 2002-2018 by Thomas Dreibholz
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
#include "sessioncontrol.h"
#include "rserpool.h"
#include "debug.h"
#include "netutilities.h"
#include "threadsafety.h"
#include "rserpoolmessage.h"
#include "loglevel.h"


/* ###### Send cookie echo ################################################### */
bool sendCookieEcho(struct RSerPoolSocket* rserpoolSocket,
                    struct Session*        session)
{
   struct RSerPoolMessage* message;
   bool                    result = false;

   if(session->Cookie) {
      message = rserpoolMessageNew(NULL, 256 + session->CookieSize);
      if(message != NULL) {
         message->Type       = AHT_COOKIE_ECHO;
         message->CookiePtr  = session->Cookie;
         message->CookieSize = session->CookieSize;
         LOG_ACTION
         fputs("Sending Cookie Echo\n", stdlog);
         LOG_END
         result = rserpoolMessageSend(IPPROTO_SCTP,
                                      rserpoolSocket->Socket,
                                      session->AssocID,
                                      0, 0, 0,
                                      message);
         rserpoolMessageDelete(message);
      }
   }
   return(result);
}


/* ###### Create new session ############################################# */
struct Session* addSession(struct RSerPoolSocket* rserpoolSocket,
                           const sctp_assoc_t     assocID,
                           const bool             isIncoming,
                           const unsigned char*   poolHandle,
                           const size_t           poolHandleSize,
                           struct TagItem*        tags)
{
   struct Session* session = (struct Session*)malloc(sizeof(struct Session));
   if(session != NULL) {
      CHECK(rserpoolSocket->ConnectedSession == NULL);
      session->Tags = tagListDuplicate(tags);
      if(session->Tags == NULL) {
         free(session);
         return(NULL);
      }

      simpleRedBlackTreeNodeNew(&session->AssocIDNode);
      simpleRedBlackTreeNodeNew(&session->SessionIDNode);
      session->AssocID                    = assocID;
      session->IsIncoming                 = isIncoming;
      session->IsFailed                   = isIncoming ? false : true;
      if(poolHandleSize > 0) {
         CHECK(poolHandleSize <= MAX_POOLHANDLESIZE);
         poolHandleNew(&session->Handle, poolHandle, poolHandleSize);
      }
      else {
         session->Handle.Size = 0;
      }
      session->Cookie                     = NULL;
      session->CookieSize                 = 0;
      session->CookieEcho                 = NULL;
      session->CookieEchoSize             = 0;
      session->StatusText[0]              = 0x00;
      session->ConnectionTimeStamp        = (isIncoming == true) ? getMicroTime() : 0;
      session->ConnectedPE                = 0;
      session->ConnectTimeout             = (unsigned long long)tagListGetData(tags, TAG_RspSession_ConnectTimeout, 5000000);
      session->HandleResolutionRetryDelay = (unsigned long long)tagListGetData(tags, TAG_RspSession_HandleResolutionRetryDelay, 250000);

      threadSafetyLock(&rserpoolSocket->Mutex);
      session->SessionID = identifierBitmapAllocateID(rserpoolSocket->SessionAllocationBitmap);
      if(session->SessionID >= 0) {
         threadSafetyLock(&rserpoolSocket->SessionSetMutex);
         sessionStorageAddSession(&rserpoolSocket->SessionSet, session);
         threadSafetyUnlock(&rserpoolSocket->SessionSetMutex);
         LOG_ACTION
         fprintf(stdlog, "Added %s session %u on RSerPool socket %d, socket %d\n",
                 session->IsIncoming ? "incoming" : "outgoing", session->SessionID,
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
      }
      threadSafetyUnlock(&rserpoolSocket->Mutex);

      if(session->SessionID < 0) {
         LOG_ERROR
         fprintf(stdlog, "Addeding %s session on RSerPool socket %d, socket %d failed, no more descriptors available\n",
                 session->IsIncoming ? "incoming" : "outgoing",
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
         free(session->Tags);
         free(session);
         session = NULL;
      }
   }
   return(session);
}


/* ###### Delete session ################################################# */
void deleteSession(struct RSerPoolSocket* rserpoolSocket,
                   struct Session*        session)
{
   if(session) {
      LOG_ACTION
      fprintf(stdlog, "Removing %s session %u on RSerPool socket %d, socket %d\n",
               session->IsIncoming ? "incoming" : "outgoing", session->SessionID,
               rserpoolSocket->Descriptor, rserpoolSocket->Socket);
      LOG_END

      threadSafetyLock(&rserpoolSocket->Mutex);
      if(rserpoolSocket->ConnectedSession == session) {
         rserpoolSocket->ConnectedSession = NULL;
      }

      threadSafetyLock(&rserpoolSocket->SessionSetMutex);
      sessionStorageDeleteSession(&rserpoolSocket->SessionSet, session);
      threadSafetyUnlock(&rserpoolSocket->SessionSetMutex);

      identifierBitmapFreeID(rserpoolSocket->SessionAllocationBitmap, session->SessionID);
      session->SessionID = 0;
      threadSafetyUnlock(&rserpoolSocket->Mutex);

      if(session->Tags) {
         tagListFree(session->Tags);
         session->Tags = NULL;
      }
      if(session->Cookie) {
         free(session->Cookie);
         session->Cookie = NULL;
      }
      if(session->CookieEcho) {
         free(session->CookieEcho);
         session->CookieEcho = NULL;
      }

      simpleRedBlackTreeNodeDelete(&session->AssocIDNode);
      simpleRedBlackTreeNodeDelete(&session->SessionIDNode);
      free(session);
   }
}


/* ###### Find session for given assoc ID or session ID ################## */
struct Session* findSession(struct RSerPoolSocket* rserpoolSocket,
                            rserpool_session_t     sessionID,
                            sctp_assoc_t           assocID)
{
   struct Session* session;

   if(rserpoolSocket->ConnectedSession) {
      return(rserpoolSocket->ConnectedSession);
   }
   else if(sessionID != 0) {
      session = sessionStorageFindSessionBySessionID(&rserpoolSocket->SessionSet, sessionID);
      if(session) {
         return(session);
      }
      LOG_VERBOSE
      fprintf(stdlog, "There is no session %u on RSerPool socket %d\n",
              sessionID, rserpoolSocket->Descriptor);
      LOG_END
      errno = EINVAL;
   }
   else if(assocID != 0) {
      session = sessionStorageFindSessionByAssocID(&rserpoolSocket->SessionSet, assocID);
      if(session) {
         return(session);
      }
      LOG_WARNING
      fprintf(stdlog, "There is no session for assoc %u on RSerPool socket %d\n",
              (unsigned int)assocID, rserpoolSocket->Descriptor);
      LOG_END
      errno = EINVAL;
   }
   else {
      LOG_ERROR
      fputs("What session are you looking for?\n", stdlog);
      LOG_END_FATAL
   }
   return(NULL);
}


/* ###### Receive cookie echo or notification from RSerPool socket ####### */
ssize_t getCookieEchoOrNotification(struct RSerPoolSocket* rserpoolSocket,
                                    void*                  buffer,
                                    size_t                 bufferLength,
                                    struct rsp_sndrcvinfo* rinfo,
                                    int*                   msg_flags,
                                    const bool             isPreRead)
{
   struct Session*          session;
   struct NotificationNode* notificationNode;
   ssize_t                  received = -1;

   threadSafetyLock(&rserpoolSocket->Mutex);

   /* ====== Give back notification ====================================== */
   notificationNode = notificationQueueDequeueNotification(&rserpoolSocket->Notifications, isPreRead);
   while(notificationNode != NULL) {
      if((1 << notificationNode->Content.rn_header.rn_type) &
         rserpoolSocket->Notifications.EventMask) {
         if(bufferLength < sizeof(notificationNode->Content)) {
            LOG_ERROR
            fputs("Buffer size is to small for a notification\n", stdlog);
            LOG_END
            errno = EINVAL;
            received = -1;
            notificationNodeDelete(notificationNode);
            break;
         }
         *msg_flags |= MSG_RSERPOOL_NOTIFICATION|MSG_EOR;
         memcpy(buffer, &notificationNode->Content, sizeof(notificationNode->Content));
         received = sizeof(notificationNode->Content);
         notificationNodeDelete(notificationNode);
         break;
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "Got unrequested notification type %u -> skipping\n",
                 notificationNode->Content.rn_header.rn_type);
         LOG_END
      }
      notificationNodeDelete(notificationNode);
      notificationNode = notificationQueueDequeueNotification(&rserpoolSocket->Notifications, isPreRead);
   }

   /* ====== Check every session if there is something to do ============= */
   if(received < 0) {
      session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
      while(session != NULL) {
         /* ====== Give back Cookie Echo ==================================== */
         if((session->CookieEcho) && (bufferLength > 0)) {
            /* A cookie echo has been stored (during rspSessionSelect()). Now,
               the application calls this function. We now return the cookie
               and free its storage space. */
            LOG_ACTION
            fputs("There is a cookie echo. Giving it back first\n", stdlog);
            LOG_END
            *msg_flags |= MSG_RSERPOOL_COOKIE_ECHO|MSG_EOR;
            received = min(bufferLength, session->CookieEchoSize);
            memcpy(buffer, session->CookieEcho, received);
            free(session->CookieEcho);
            session->CookieEcho     = NULL;
            session->CookieEchoSize = 0;
            rinfo->rinfo_session    = session->SessionID;
            rinfo->rinfo_ppid       = PPID_ASAP;
            break;
         }
         session = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
      }
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(received);
}


/* ###### Handle SCTP communication up notification ###################### */
static void handleCommUp(struct RSerPoolSocket*          rserpoolSocket,
                         const struct sctp_assoc_change* notification)
{
   struct Session*          session;
   struct NotificationNode* notificationNode;

   LOG_ACTION
   fprintf(stdlog, "SCTP_COMM_UP for RSerPool socket %d, socket %d, assoc %u\n",
         rserpoolSocket->Descriptor, rserpoolSocket->Socket,
         (unsigned int)notification->sac_assoc_id);
   LOG_END
   if(rserpoolSocket->ConnectedSession == NULL) {
      session = addSession(rserpoolSocket,
                           notification->sac_assoc_id,
                           true, NULL, 0, NULL);
      if(session != NULL) {
         notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                                 true, RSERPOOL_SESSION_CHANGE);
         if(notificationNode) {
            notificationNode->Content.rn_session_change.rsc_state   = RSERPOOL_SESSION_ADD;
            notificationNode->Content.rn_session_change.rsc_session = session->SessionID;
         }
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "Aborting association %u on RSerPool socket %d, socket %d, due to session creation failure\n",
               (unsigned int)notification->sac_assoc_id,
               rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
         sendabort(rserpoolSocket->Socket, notification->sac_assoc_id);
      }
   }
}


/* ###### Handle SCTP communication lost notification #################### */
static void handleCommLost(struct RSerPoolSocket*          rserpoolSocket,
                           const struct sctp_assoc_change* notification)
{
   struct Session*          session = NULL;
   struct NotificationNode* notificationNode;

   LOG_ACTION
   fprintf(stdlog, "SCTP_COMM_LOST for RSerPool socket %d, socket %d, assoc %u\n",
           rserpoolSocket->Descriptor, rserpoolSocket->Socket,
           (unsigned int)notification->sac_assoc_id);
   LOG_END

   if(rserpoolSocket->ConnectedSession) {
      LOG_ACTION
      fprintf(stdlog, "Removing Assoc ID %u from session %u of RSerPool socket %d, socket %d\n",
            (unsigned int)rserpoolSocket->ConnectedSession->AssocID, rserpoolSocket->ConnectedSession->SessionID,
            rserpoolSocket->Descriptor, rserpoolSocket->Socket);

      LOG_END
      if(!rserpoolSocket->ConnectedSession->IsFailed) {
         sendabort(rserpoolSocket->Socket, rserpoolSocket->ConnectedSession->AssocID);
         sessionStorageUpdateSession(&rserpoolSocket->SessionSet, rserpoolSocket->ConnectedSession, 0);

         notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                                 false, RSERPOOL_FAILOVER);
         if(notificationNode) {
            notificationNode->Content.rn_failover.rf_state      = RSERPOOL_FAILOVER_NECESSARY;
            notificationNode->Content.rn_failover.rf_session    = rserpoolSocket->ConnectedSession->SessionID;
            notificationNode->Content.rn_failover.rf_has_cookie = (rserpoolSocket->ConnectedSession->CookieSize > 0);
         }
      }
      else {
         LOG_ACTION
         fputs("Session has already been removed. Okay.\n", stdlog);
         LOG_END
      }
   }
   else {
      session = sessionStorageFindSessionByAssocID(&rserpoolSocket->SessionSet,
                                                   notification->sac_assoc_id);
      if(session) {
         if(rserpoolSocket->ConnectedSession != session) {
            LOG_ACTION
            fprintf(stdlog, "Removing session %u of RSerPool socket %d, socket %d after closure of assoc %u\n",
                    session->SessionID,
                    rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                    (unsigned int)session->AssocID);
            LOG_END

            notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                                    true, RSERPOOL_SESSION_CHANGE);
            if(notificationNode) {
               notificationNode->Content.rn_session_change.rsc_state   = RSERPOOL_SESSION_REMOVE;
               notificationNode->Content.rn_session_change.rsc_session = session->SessionID;
            }
            deleteSession(rserpoolSocket, session);
         }
      }
      else {
         LOG_ERROR
         fprintf(stdlog, "SCTP_COMM_LOST for RSerPool socket %d, socket %d, assoc %u, but there is no session for it!\n",
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                 (unsigned int)notification->sac_assoc_id);
         LOG_END
      }
   }
}


/* ###### Handle SCTP shutdown event notification ######################## */
static void handleShutdown(struct RSerPoolSocket*            rserpoolSocket,
                           const struct sctp_shutdown_event* notification)
{
   struct Session*          session;
   struct NotificationNode* notificationNode;

   LOG_ACTION
   fprintf(stdlog, "SHUTDOWN_EVENT for RSerPool socket %d, socket %d, assoc %u\n",
           rserpoolSocket->Descriptor, rserpoolSocket->Socket,
           (unsigned int)notification->sse_assoc_id);
   LOG_END
   session = sessionStorageFindSessionByAssocID(&rserpoolSocket->SessionSet,
                                                notification->sse_assoc_id);
   if(session) {
      LOG_ACTION
      fprintf(stdlog, "Shutdown event for session %u of RSerPool socket %d, socket %d, assoc %u\n",
              session->SessionID,
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              (unsigned int)session->AssocID);
      LOG_END

      notificationNode = notificationQueueEnqueueNotification(&rserpoolSocket->Notifications,
                                                              false, RSERPOOL_SHUTDOWN_EVENT);
      if(notificationNode) {
         notificationNode->Content.rn_shutdown_event.rse_session = session->SessionID;
      }
   }
}


/* ###### Handle SCTP notification ####################################### */
/* Returns true, if association has been disconnected; false otherwise. */
bool handleNotification(struct RSerPoolSocket*         rserpoolSocket,
                        const union sctp_notification* notification)
{

   LOG_VERBOSE1
   fprintf(stdlog, "SCTP notification %d for RSerPool socket %d, socket %d\n",
            notification->sn_header.sn_type,
            rserpoolSocket->Descriptor, rserpoolSocket->Socket);
   LOG_END

   if(rserpoolSocket->SocketType != SOCK_STREAM) {
      if(notification->sn_header.sn_type == SCTP_ASSOC_CHANGE) {
         switch(notification->sn_assoc_change.sac_state) {
            case SCTP_COMM_UP:
               handleCommUp(rserpoolSocket, &notification->sn_assoc_change);
             break;
            case SCTP_COMM_LOST:
            case SCTP_SHUTDOWN_COMP:
               handleCommLost(rserpoolSocket, &notification->sn_assoc_change);
               return(true);
         }
      }
      else if(notification->sn_header.sn_type == SCTP_SHUTDOWN_EVENT) {
         handleShutdown(rserpoolSocket, &notification->sn_shutdown_event);
      }
   }
   else {
      if( (notification->sn_header.sn_type == SCTP_ASSOC_CHANGE) &&
          ( (notification->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP) ||
            (notification->sn_assoc_change.sac_state == SCTP_COMM_LOST) ) ) {
         return(true);
      }
   }
   return(false);
}


/* ###### Handle control channel message ################################# */
void handleControlChannelMessage(struct RSerPoolSocket* rserpoolSocket,
                                 const sctp_assoc_t     assocID,
                                 char*                  buffer,
                                 size_t                 bufferSize)
{
   struct RSerPoolMessage* message;
   unsigned int            result;

   struct Session* session = findSession(rserpoolSocket, 0, assocID);
   if(session != NULL) {
      LOG_ACTION
      fprintf(stdlog, "ASAP control channel message for RSerPool socket %d, socket %d, session %u, assoc %u\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              session->SessionID, (unsigned int)assocID);
      LOG_END

      result = rserpoolPacket2Message(buffer, NULL, 0, PPID_ASAP, bufferSize, bufferSize, &message);
      if(message != NULL) {
         if(result == RSPERR_OKAY) {
            switch(message->Type) {
               case AHT_COOKIE:
                  LOG_VERBOSE
                  fputs("Got cookie\n", stdlog);
                  LOG_END
                  if(session->Cookie) {
                     LOG_VERBOSE2
                     fputs("Replacing existing Cookie with new one\n", stdlog);
                     LOG_END
                     free(session->Cookie);
                  }
                  message->CookiePtrAutoDelete = false;
                  session->Cookie              = message->CookiePtr;
                  session->CookieSize          = message->CookieSize;
               break;
               case AHT_COOKIE_ECHO:
                  if(session->CookieEcho == NULL) {
                     LOG_ACTION
                     fputs("Got cookie echo\n", stdlog);
                     LOG_END
                     message->CookiePtrAutoDelete = false;
                     session->CookieEcho          = message->CookiePtr;
                     session->CookieEchoSize      = message->CookieSize;
                  }
                  else {
                     LOG_ERROR
                     fputs("Got additional cookie echo. Ignoring it.\n", stdlog);
                     LOG_END
                  }
               break;
               case AHT_BUSINESS_CARD:
                  LOG_ACTION
                  fputs("Got business card\n", stdlog);
                  LOG_END
               break;
               default:
                  LOG_WARNING
                  fprintf(stdlog, "Do not know what to do with ASAP type %u\n", message->Type);
                  LOG_END
               break;
            }
         }
         rserpoolMessageDelete(message);
      }
   }
}


/* ###### Handle control channel message or notification ################# */
bool handleControlChannelAndNotifications(struct RSerPoolSocket* rserpoolSocket)
{
   char     buffer[4];
   ssize_t  result;
   uint32_t ppid;
   int      flags;

   /* ====== Check, if message on socket is notification or ASAP ========= */
   flags   = MSG_PEEK;   /* Only peek to check if data is of right type! */
   result  = recvfromplus(rserpoolSocket->Socket, (char*)&buffer, sizeof(buffer),
                          &flags, NULL, NULL, &ppid, NULL, NULL, 0);
   if( (result > 0) &&
       ( (ppid == PPID_ASAP) || (flags & MSG_NOTIFICATION) ) ) {
      /* Handle control channel data or notification */
      LOG_VERBOSE
      fprintf(stdlog, "Handling control channel data or notification of RSerPool socket %u, socket %u\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket);
      LOG_END
      rsp_recvmsg(rserpoolSocket->Descriptor, NULL, 0, NULL, &flags, 0);
      return(true);
   }

   return(false);
}
