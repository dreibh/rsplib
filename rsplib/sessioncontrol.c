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
                                      MSG_NOSIGNAL,
                                      0,
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
      session->Tags = tagListDuplicate(tags);
      if(session->Tags == NULL) {
         free(session);
         return(NULL);
      }

      leafLinkedRedBlackTreeNodeNew(&session->AssocIDNode);
      leafLinkedRedBlackTreeNodeNew(&session->SessionIDNode);
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
      session->PendingNotifications       = 0;
      session->EventMask                  = (rserpoolSocket->SocketType == SOCK_STREAM) ? 0 : SNT_NOTIFICATION_MASK;
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
         sessionStorageAddSession(&rserpoolSocket->SessionSet, session);
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
      sessionStorageDeleteSession(&rserpoolSocket->SessionSet, session);
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

      leafLinkedRedBlackTreeNodeDelete(&session->AssocIDNode);
      leafLinkedRedBlackTreeNodeDelete(&session->SessionIDNode);
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
      LOG_WARNING
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
              assocID, rserpoolSocket->Descriptor);
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
                                    int*                   msg_flags)
{
   struct Session*              session;
   union rserpool_notification* notification;
   unsigned int                 pendingNotifications;
   ssize_t                      received = -1;

   threadSafetyLock(&rserpoolSocket->Mutex);

   /* ====== Check every session if there is something to do ============= */
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
         *msg_flags |= MSG_RSERPOOL_COOKIE_ECHO;
         received = min(bufferLength, session->CookieEchoSize);
         memcpy(buffer, session->CookieEcho, received);
         free(session->CookieEcho);
         session->CookieEcho     = NULL;
         session->CookieEchoSize = 0;
         break;
      }

      /* ====== Give back notification =================================== */
      pendingNotifications = session->PendingNotifications & session->EventMask;
      if(pendingNotifications) {
         notification = (union rserpool_notification*)buffer;
         if(bufferLength < sizeof(union rserpool_notification)) {
            LOG_ERROR
            fputs("Buffer size is to small for a notification\n", stdlog);
            LOG_END
            errno = EINVAL;
            received = sizeof(notification);
            break;
         }
         if(pendingNotifications & SNT_SESSION_ADD) {
            session->PendingNotifications &= ~SNT_SESSION_ADD;
            *msg_flags |= MSG_RSERPOOL_NOTIFICATION;
            notification->rn_session_change.rsc_type    = RSERPOOL_SESSION_CHANGE;
            notification->rn_session_change.rsc_flags   = 0x00;
            notification->rn_session_change.rsc_length  = sizeof(notification);
            notification->rn_session_change.rsc_state   = RSERPOOL_SESSION_ADD;
            notification->rn_session_change.rsc_session = session->SessionID;
            return(sizeof(notification));
         }
         if(pendingNotifications & SNT_FAILOVER_NECESSARY) {
            session->PendingNotifications &= ~SNT_FAILOVER_NECESSARY;
            *msg_flags |= MSG_RSERPOOL_NOTIFICATION;
            notification->rn_failover.rf_type    = RSERPOOL_FAILOVER;
            notification->rn_failover.rf_flags   = 0x00;
            notification->rn_failover.rf_length  = sizeof(notification);
            notification->rn_failover.rf_state   = RSERPOOL_FAILOVER_NECESSARY;
            notification->rn_failover.rf_session = session->SessionID;
            received = sizeof(notification);
            break;
         }
         if(pendingNotifications & SNT_FAILOVER_COMPLETE) {
            session->PendingNotifications &= ~SNT_FAILOVER_COMPLETE;
            *msg_flags |= MSG_RSERPOOL_NOTIFICATION;
            notification->rn_failover.rf_type    = RSERPOOL_FAILOVER;
            notification->rn_failover.rf_flags   = 0x00;
            notification->rn_failover.rf_length  = sizeof(notification);
            notification->rn_failover.rf_state   = RSERPOOL_FAILOVER_COMPLETE;
            notification->rn_failover.rf_session = session->SessionID;
            received = sizeof(notification);
            break;
         }
      }
      session = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(received);
}


/* ###### Notify session of an RSerPool event ############################ */
static void notifySession(struct RSerPoolSocket* rserpoolSocket,
                          sctp_assoc_t           assocID,
                          unsigned int           notificationType)
{
   struct Session* session;

   threadSafetyLock(&rserpoolSocket->Mutex);
   session = sessionStorageFindSessionByAssocID(&rserpoolSocket->SessionSet,
                                                assocID);
   if(session) {
      switch(notificationType) {
         case NST_COMM_LOST:
         case NST_SHUTDOWN_EVENT:
            if(rserpoolSocket->ConnectedSession != session) {
               LOG_ACTION
               fprintf(stdlog, "Removing session %u of RSerPool socket %d, socket %d after closure of assoc %u\n",
                       session->SessionID,
                       rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                       session->AssocID);
               LOG_END
               deleteSession(rserpoolSocket, session);
            }
            else {
               LOG_ACTION
               fprintf(stdlog, "Removing Assoc ID %u from session %u of RSerPool socket %d, socket %d\n",
                     session->AssocID, session->SessionID,
                     rserpoolSocket->Descriptor, rserpoolSocket->Socket);

               LOG_END
               sendabort(rserpoolSocket->Socket, session->AssocID);
               session->PendingNotifications |= SNT_FAILOVER_NECESSARY;
               session->IsFailed = true;
               sessionStorageUpdateSession(&rserpoolSocket->SessionSet, session, 0);
            }
          break;
      }
   }
   else {
      LOG_WARNING
      fprintf(stdlog, "There is no session for assoc %u on RSerPool socket %d, socket %d\n",
               assocID, rserpoolSocket->Descriptor, rserpoolSocket->Socket);
      LOG_END
   }
   threadSafetyUnlock(&rserpoolSocket->Mutex);
}


/* ###### Handle SCTP notification ####################################### */
void handleNotification(struct RSerPoolSocket* rserpoolSocket,
                        const char*            buffer,
                        size_t                 bufferSize)
{
   union sctp_notification* notification;
   struct Session*          session;

   notification = (union sctp_notification*)buffer;
   LOG_VERBOSE1
   fprintf(stdlog, "SCTP notification %d for RSerPool socket %d, socket %d\n",
            notification->sn_header.sn_type,
            rserpoolSocket->Descriptor, rserpoolSocket->Socket);
   LOG_END

   if(rserpoolSocket->SocketType != SOCK_STREAM) {
      if(notification->sn_header.sn_type == SCTP_ASSOC_CHANGE) {
         switch(notification->sn_assoc_change.sac_state) {
            case SCTP_COMM_UP:
               LOG_ACTION
               fprintf(stdlog, "SCTP_COMM_UP for RSerPool socket %d, socket %d, assoc %u\n",
                     rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                     notification->sn_assoc_change.sac_assoc_id);
               LOG_END
               session = addSession(rserpoolSocket,
                                    notification->sn_assoc_change.sac_assoc_id,
                                    true, NULL, 0, NULL);
               if(session == NULL) {
                  LOG_WARNING
                  fprintf(stdlog, "Aborting association %u on RSerPool socket %d, socket %d, due to session creation failure\n",
                        notification->sn_assoc_change.sac_assoc_id,
                        rserpoolSocket->Descriptor, rserpoolSocket->Socket);
                  LOG_END
                  sendabort(rserpoolSocket->Socket, notification->sn_assoc_change.sac_assoc_id);
               }
               else {
                  session->PendingNotifications |= SNT_SESSION_ADD;
               }
            break;
            case SCTP_COMM_LOST:
               LOG_ACTION
               fprintf(stdlog, "SCTP_COMM_LOST for RSerPool socket %d, socket %d, assoc %u\n",
                     rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                     notification->sn_assoc_change.sac_assoc_id);
               LOG_END
               notifySession(rserpoolSocket,
                           notification->sn_assoc_change.sac_assoc_id,
                           NST_COMM_LOST);
            break;
         }
      }
      if(notification->sn_header.sn_type == SCTP_SHUTDOWN_EVENT) {
         LOG_ACTION
         fprintf(stdlog, "SCTP_SHUTDOWN_EVENT for RSerPool socket %d, socket %d, assoc %u\n",
               rserpoolSocket->Descriptor, rserpoolSocket->Socket,
               notification->sn_shutdown_event.sse_assoc_id);
         LOG_END
         notifySession(rserpoolSocket,
                     notification->sn_shutdown_event.sse_assoc_id,
                     NST_SHUTDOWN_EVENT);
      }
   }
}


/* ###### Handle control channel message ################################# */
void handleControlChannelMessage(struct RSerPoolSocket* rserpoolSocket,
                                 const sctp_assoc_t     assocID,
                                 char*                  buffer,
                                 size_t                 bufferSize)
{
   struct RSerPoolMessage* message;
   unsigned int            type = 0;
   unsigned int            result;

   struct Session* session = findSession(rserpoolSocket, 0, assocID);
   if(session != NULL) {
      LOG_ACTION
      fprintf(stdlog, "ASAP control channel message for RSerPool socket %d, socket %d, session %u, assoc %u\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              session->SessionID, assocID);
      LOG_END

      result = rserpoolPacket2Message(buffer, NULL, 0, PPID_ASAP, bufferSize, bufferSize, &message);
      if(message != NULL) {
         if(result == RSPERR_OKAY) {
            type = message->Type;
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
   char                   buffer[4];
   struct sctp_sndrcvinfo sinfo;
   ssize_t                result;
   int                    flags;

   /* ====== Check, if message on socket is notification or ASAP ========= */
   flags = MSG_PEEK;
   result = sctp_recvmsg(rserpoolSocket->Socket, (char*)&buffer, sizeof(buffer),
                         NULL, NULL, &sinfo, &flags);
   if( (result > 0) &&
       ( (ntohl(sinfo.sinfo_ppid) == PPID_ASAP) || (flags & MSG_NOTIFICATION) ) ) {
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
