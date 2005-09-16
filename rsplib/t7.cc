/*
 *  $Id: api.c 733 2005-08-26 13:32:44Z dreibh $
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
 * Purpose: RSerPool API
 *
 */

#include "rsplib.h"
#include "loglevel.h"
#include "dispatcher.h"
#include "netutilities.h"
#include "threadsafety.h"
#include "rserpoolmessage.h"
#include "messagebuffer.h"
#include "componentstatusprotocol.h"
#include "leaflinkedredblacktree.h"

#include <ext_socket.h>



struct IdentifierBitmap
{
   size_t Entries;
   size_t Available;

   size_t Slots;
   size_t Bitmap[0];
};

#define IdentifierBitmapSlotsize (sizeof(size_t) * 8)


/* ###### Constructor #################################################### */
struct IdentifierBitmap* identifierBitmapNew(const size_t entries)
{
   const size_t slots = (entries + (IdentifierBitmapSlotsize - (entries % IdentifierBitmapSlotsize))) /
                           IdentifierBitmapSlotsize;
   struct IdentifierBitmap* identifierBitmap = (struct IdentifierBitmap*)malloc(sizeof(IdentifierBitmap) + (slots + 1) * sizeof(size_t));
   if(identifierBitmap) {
      memset(&identifierBitmap->Bitmap, 0, (slots + 1) * sizeof(size_t));
      identifierBitmap->Entries   = entries;
      identifierBitmap->Available = entries;
      identifierBitmap->Slots     = slots;
   }
   return(identifierBitmap);
}


/* ###### Destructor ##################################################### */
void identifierBitmapDelete(struct IdentifierBitmap* identifierBitmap)
{
   identifierBitmap->Entries = 0;
   free(identifierBitmap);
}


/* ###### Allocate ID #################################################### */
int identifierBitmapAllocateID(struct IdentifierBitmap* identifierBitmap)
{
   size_t i, j;
   int    id = -1;

   if(identifierBitmap->Available > 0) {
      i = 0;
      while(identifierBitmap->Bitmap[i] == ~((size_t)0)) {
         i++;
      }
      id = i * IdentifierBitmapSlotsize;

      j = 0;
      while((j < IdentifierBitmapSlotsize) &&
            (id < (int)identifierBitmap->Entries) &&
            (identifierBitmap->Bitmap[i] & (1 << j))) {
         j++;
         id++;
      }
      CHECK(id < (int)identifierBitmap->Entries);

      identifierBitmap->Bitmap[i] |= (1 << j);
      identifierBitmap->Available--;
   }

   return(id);
}


/* ###### Allocate specific ID ########################################### */
int identifierBitmapAllocateSpecificID(struct IdentifierBitmap* identifierBitmap,
                                       const int                id)
{
   size_t i, j;

   CHECK((id >= 0) && (id < (int)identifierBitmap->Entries));
   i = id / IdentifierBitmapSlotsize;
   j = id % IdentifierBitmapSlotsize;
   if(identifierBitmap->Bitmap[i] & (1 << j)) {
      return(-1);
   }
   identifierBitmap->Bitmap[i] |= (1 << j);
   identifierBitmap->Available--;
   return(id);
}


/* ###### Free ID ######################################################## */
void identifierBitmapFreeID(struct IdentifierBitmap* identifierBitmap, const int id)
{
   size_t i, j;

   CHECK((id >= 0) && (id < (int)identifierBitmap->Entries));
   i = id / IdentifierBitmapSlotsize;
   j = id % IdentifierBitmapSlotsize;
   CHECK(identifierBitmap->Bitmap[i] & (1 << j));
   identifierBitmap->Bitmap[i] &= ~(1 << j);
   identifierBitmap->Available++;
}





typedef unsigned int rserpool_session_t;

#define SESSION_SETSIZE 16384


struct rserpool_failover
{
   uint16_t rf_type;
   uint16_t rf_flags;
   uint32_t rf_length;
};

union rserpool_notification
{
   struct {
      uint16_t rn_type;
      uint16_t rn_flags;
      uint32_t rn_length;
   }                        rn_header;
   struct rserpool_failover rn_failover;
};


// ????????????
int rsp_deregister(int sd);
int rsp_close(int sd);
static bool doRegistration(struct RSerPoolSocket* rserpoolSocket);
static void deletePoolElement(struct PoolElement* poolElement);
static bool handleControlChannelAndNotifications(struct RSerPoolSocket* rserpoolSocket);
static void handleControlChannelMessage(struct RSerPoolSocket* rserpoolSocket,
                                        const char*            buffer,
                                        size_t                 bufferSize);
static void handleNotification(struct RSerPoolSocket* rserpoolSocket,
                               const char*            buffer,
                               size_t                 bufferSize);

// ????????????


/* ###### Configure notifications of SCTP socket ############################# */
static bool configureSCTPSocket(int sd, sctp_assoc_t assocID, struct TagItem* tags)
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



extern struct Dispatcher      gDispatcher;
struct LeafLinkedRedBlackTree gRSerPoolSocketSet;
struct IdentifierBitmap*      gRSerPoolSocketAllocationBitmap;


struct rsp_info
{

};

struct rsp_sndrcvinfo
{
};

struct rsp_loadinfo
{
   uint32_t rli_policy;
   uint32_t rli_weight;
   uint32_t rli_load;
   uint32_t rli_load_degradation;
};


#define GET_RSERPOOL_SOCKET(rserpoolSocket, sd) \
   rserpoolSocket = getRSerPoolSocketForDescriptor(sd); \
   if(rserpoolSocket == NULL) { \
      errno = EBADF; \
      return(-1); \
   }

#define CHECK_RSERPOOL_POOLELEMENT(rserpoolSocket) \
   if(rserpoolSocket->PoolElement == NULL) { \
      LOG_ERROR \
      fprintf(stdlog, "RSerPool socket %d is no pool element\n", sd); \
      LOG_END \
      errno = EBADF; \
      return(-1); \
   }

#define GET_RSERPOOL_SESSION(session, rserpoolSocket, sessionID, assocID) \
   session = sessionFind(rserpoolSocket, sessionID, assocID); \
   if(session == NULL) { \
      errno = EINVAL; \
      return(-1); \
   }


struct PoolElement
{
   struct PoolHandle                 Handle;
   uint32_t                          Identifier;
   struct ThreadSafety               Mutex;

   unsigned int                      PolicyType;
   uint32_t                          PolicyParameterWeight;
   uint32_t                          PolicyParameterLoad;

   struct Timer                      ReregistrationTimer;
   unsigned int                      RegistrationLife;
   unsigned int                      ReregistrationInterval;

   bool                              HasControlChannel;
};




struct SessionStorage
{
   struct LeafLinkedRedBlackTree AssocIDSet;
   struct LeafLinkedRedBlackTree SessionIDSet;
};


struct Session
{
   struct LeafLinkedRedBlackTreeNode SessionIDNode;
   struct LeafLinkedRedBlackTreeNode AssocIDNode;

   sctp_assoc_t                      AssocID;
   rserpool_session_t                SessionID;

   bool                              IsIncoming;

   void*                             Cookie;
   size_t                            CookieSize;
   void*                             CookieEcho;
   size_t                            CookieEchoSize;

   unsigned long long                ConnectionTimeStamp;
   unsigned long long                ConnectTimeout;
   unsigned long long                HandleResolutionRetryDelay;

   struct TagItem*                   Tags;

   char                              StatusText[128];
};


struct RSerPoolSocket
{
   struct LeafLinkedRedBlackTreeNode Node;

   int                               Descriptor;
   struct ThreadSafety               Mutex;

   int                               SocketDomain;
   int                               SocketType;
   int                               SocketProtocol;
   int                               Socket;

   struct PoolElement*               PoolElement;        /* PE mode                      */
   struct SessionStorage             SessionSet;         /* UDP-like PU mode and PE mode */
   struct Session*                   ConnectedSession;   /* TCP-like PU mode             */

   struct IdentifierBitmap*          SessionAllocationBitmap;
   char                              MessageBuffer[65536];
};





/* ###### Get session from assoc ID storage node ######################### */
inline static struct Session* getSessionFromAssocIDStorageNode(void* node)
{
   return((struct Session*)((long)node -
             ((long)&((struct Session *)node)->AssocIDNode - (long)node)));
}


/* ###### Get session from session ID storage node ####################### */
inline static struct Session* getSessionFromSessionIDStorageNode(void* node)
{
   return((struct Session*)((long)node -
             ((long)&((struct Session *)node)->SessionIDNode - (long)node)));
}


/* ###### Print assoc ID storage node #################################### */
static void sessionAssocIDPrint(const void* node, FILE* fd)
{
   const struct Session* session = (const struct Session*)getSessionFromAssocIDStorageNode((void*)node);

   fprintf(fd, "A%u[S%u] ", session->AssocID, session->SessionID);
}


/* ###### Compare assoc ID storage nodes ################################# */
static int sessionAssocIDComparison(const void* node1, const void* node2)
{
   const struct Session* session1 = (const struct Session*)getSessionFromAssocIDStorageNode((void*)node1);
   const struct Session* session2 = (const struct Session*)getSessionFromAssocIDStorageNode((void*)node2);

   if(session1->AssocID < session2->AssocID) {
      return(-1);
   }
   else if(session1->AssocID > session2->AssocID) {
      return(1);
   }
   return(0);
}


/* ###### Print session ID storage node ################################## */
static void sessionSessionIDPrint(const void* node, FILE* fd)
{
   const struct Session* session = (const struct Session*)getSessionFromSessionIDStorageNode((void*)node);

   fprintf(fd, "%u[A%u] ", session->SessionID, (unsigned int)session->AssocID);
}


/* ###### Compare session ID storage nodes ############################### */
static int sessionSessionIDComparison(const void* node1, const void* node2)
{
   const struct Session* session1 = (const struct Session*)getSessionFromSessionIDStorageNode((void*)node1);
   const struct Session* session2 = (const struct Session*)getSessionFromSessionIDStorageNode((void*)node2);

   if(session1->SessionID < session2->SessionID) {
      return(-1);
   }
   else if(session1->SessionID > session2->SessionID) {
      return(1);
   }
   return(0);
}


/* ###### Constructor #################################################### */
void sessionStorageNew(struct SessionStorage* sessionStorage)
{
   leafLinkedRedBlackTreeNew(&sessionStorage->AssocIDSet, sessionAssocIDPrint, sessionAssocIDComparison);
   leafLinkedRedBlackTreeNew(&sessionStorage->SessionIDSet, sessionSessionIDPrint, sessionSessionIDComparison);
}


/* ###### Destructor ##################################################### */
void sessionStorageDelete(struct SessionStorage* sessionStorage)
{
   CHECK(leafLinkedRedBlackTreeIsEmpty(&sessionStorage->AssocIDSet));
   CHECK(leafLinkedRedBlackTreeIsEmpty(&sessionStorage->SessionIDSet));
   leafLinkedRedBlackTreeDelete(&sessionStorage->AssocIDSet);
   leafLinkedRedBlackTreeDelete(&sessionStorage->SessionIDSet);
}


/* ###### Add session #################################################### */
void sessionStorageAddSession(struct SessionStorage* sessionStorage,
                              struct Session*        session)
{
   CHECK(leafLinkedRedBlackTreeInsert(&sessionStorage->SessionIDSet, &session->SessionIDNode) == &session->SessionIDNode);
   CHECK(leafLinkedRedBlackTreeInsert(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Delete session ################################################# */
void sessionStorageDeleteSession(struct SessionStorage* sessionStorage,
                                 struct Session*        session)
{
   CHECK(leafLinkedRedBlackTreeRemove(&sessionStorage->SessionIDSet, &session->SessionIDNode) == &session->SessionIDNode);
   CHECK(leafLinkedRedBlackTreeRemove(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Update session's assoc ID ###################################### */
void sessionStorageUpdateSession(struct SessionStorage* sessionStorage,
                                 struct Session*        session,
                                 sctp_assoc_t           newAssocID)
{
   CHECK(leafLinkedRedBlackTreeRemove(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
   session->AssocID = newAssocID;
   CHECK(leafLinkedRedBlackTreeInsert(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Is session storage empty? ###################################### */
bool sessionStorageIsEmpty(struct SessionStorage* sessionStorage)
{
   return(leafLinkedRedBlackTreeIsEmpty(&sessionStorage->SessionIDSet));
}


/* ###### Print session storage ########################################## */
void sessionStoragePrint(struct SessionStorage* sessionStorage,
                         FILE*                  fd)
{
   fputs("SessionStorage:\n", fd);
   fputs(" by Session ID: ", fd);
   leafLinkedRedBlackTreePrint(&sessionStorage->SessionIDSet, fd);
   fputs(" by Assoc ID:   ", fd);
   leafLinkedRedBlackTreePrint(&sessionStorage->AssocIDSet, fd);
}


/* ###### Find session by session ID ##################################### */
struct Session* sessionStorageFindSessionBySessionID(struct SessionStorage* sessionStorage,
                                                     rserpool_session_t     sessionID)
{
   struct Session                     cmpNode;
   struct LeafLinkedRedBlackTreeNode* node;

   cmpNode.SessionID = sessionID;

   node = leafLinkedRedBlackTreeFind(&sessionStorage->SessionIDSet, &cmpNode.SessionIDNode);
   if(node != NULL) {
      return(getSessionFromSessionIDStorageNode(node));
   }
   return(NULL);
}


/* ###### Find session by assoc ID ####################################### */
struct Session* sessionStorageFindSessionByAssocID(struct SessionStorage* sessionStorage,
                                                   sctp_assoc_t           assocID)
{
   struct Session                     cmpNode;
   struct LeafLinkedRedBlackTreeNode* node;

   cmpNode.AssocID = assocID;

   node = leafLinkedRedBlackTreeFind(&sessionStorage->AssocIDSet, &cmpNode.AssocIDNode);
   if(node != NULL) {
      return(getSessionFromAssocIDStorageNode(node));
   }
   return(NULL);
}


/* ###### Get first session ############################################## */
struct Session* sessionStorageGetFirstSession(struct SessionStorage* sessionStorage)
{
   struct LeafLinkedRedBlackTreeNode* node;
   node = leafLinkedRedBlackTreeGetFirst(&sessionStorage->SessionIDSet);
   if(node != NULL) {
      return(getSessionFromSessionIDStorageNode(node));
   }
   return(NULL);
}


/* ###### Get next session ############################################### */
struct Session* sessionStorageGetNextSession(struct SessionStorage* sessionStorage,
                                             struct Session*        session)
{
   struct LeafLinkedRedBlackTreeNode* node;
   node = leafLinkedRedBlackTreeGetNext(&sessionStorage->SessionIDSet, &session->SessionIDNode);
   if(node != NULL) {
      return(getSessionFromSessionIDStorageNode(node));
   }
   return(NULL);
}



/* ###### Create new session ############################################# */
static struct Session* addSession(struct RSerPoolSocket* rserpoolSocket,
                                  const sctp_assoc_t     assocID,
                                  const bool             isIncoming,
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
      session->Cookie                     = NULL;
      session->CookieSize                 = 0;
      session->CookieEcho                 = NULL;
      session->CookieEchoSize             = 0;
      session->StatusText[0]              = 0x00;
      session->ConnectionTimeStamp        = 0;
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
static void deleteSession(struct RSerPoolSocket* rserpoolSocket,
                          struct Session*        session)
{
   if(session) {
      LOG_ACTION
      fprintf(stdlog, "Removing %s session %u on RSerPool socket %d, socket %d\n",
               session->IsIncoming ? "incoming" : "outgoing", session->SessionID,
               rserpoolSocket->Descriptor, rserpoolSocket->Socket);
      LOG_END

      threadSafetyLock(&rserpoolSocket->Mutex);
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




#if 0
/* ###### Read from buffer ################################################ */
ssize_t new_messageBufferRead(
                          struct MessageBuffer*    messageBuffer,
                          int                      sd,
                          union sockaddr_union*    sourceAddress,
                          socklen_t*               sourceAddressLength,
                          const unsigned long long timeout)
{
   struct TLVHeader header;
   ssize_t          received=0;
   size_t           tlvLength;
   uint32_t         ppid;
   sctp_assoc_t     assocID;
   unsigned short   streamID;
   int64            timeout;
   int              flags;

   if(messageBuffer->Position == 0) {

      LOG_VERBOSE4
      fprintf(stdlog, "Trying to peek TLV header from socket %d, peek timeout %lld [us], total timeout %lld [us]\n",
              sd, peekTimeout, totalTimeout);
      LOG_END
      messageBuffer->StartTimeStamp = getMicroTime();
      flags = MSG_PEEK;
      received = recvfromplus(sd, (char*)&header, sizeof(header), &flags,
                              (struct sockaddr*)sourceAddress, sourceAddressLength,
                              &ppid, &assocID, &streamID, peekTimeout);
      if(received > 0) {
         if(received == sizeof(header)) {
            tlvLength = (size_t)ntohs(header.Length);
            if(tlvLength < messageBuffer->Size) {
               messageBuffer->ToGo = tlvLength;
            }
            else {
               LOG_ERROR
               fprintf(stdlog, "Message buffer size %d of socket %d is too small to read TLV of length %d\n",
                        (int)messageBuffer->Size, sd, (int)tlvLength);
               LOG_END
               return(RspRead_TooBig);
            }
         }
      }
      else if(received == 0) {
         LOG_VERBOSE3
         fputs("Nothing to read (connection closed?)\n", stdlog);
         LOG_END
         return(0);
      }
      else if(errno == EAGAIN) {
         LOG_VERBOSE3
         fputs("Timeout while trying to read data\n", stdlog);
         LOG_END
         return(RspRead_Timeout);
      }
      else {
         LOG_VERBOSE3
         logerror("Unable to read data");
         LOG_END
         return(RspRead_ReadError);
      }
   }

   if(messageBuffer->ToGo > 0) {
      timeout = (int64)totalTimeout - ((int64)getMicroTime() - (int64)messageBuffer->StartTimeStamp);
      if(timeout < 0) {
         timeout = 0;
      }
      LOG_VERBOSE4
      fprintf(stdlog, "Trying to read remaining %d bytes from message of length %d from socket %d, timeout %lld [us]\n",
              (int)messageBuffer->ToGo, (int)(messageBuffer->Position + messageBuffer->ToGo), sd, timeout);
      LOG_END
      flags    = 0;
      received = recvfromplus(sd, (char*)&messageBuffer->Buffer[messageBuffer->Position], messageBuffer->ToGo, &flags,
                              (struct sockaddr*)sourceAddress, sourceAddressLength,
                              &ppid, &assocID, &streamID, (unsigned long long)timeout);
      if(received > 0) {
         messageBuffer->ToGo     -= received;
         messageBuffer->Position += received;
         LOG_VERBOSE4
         fprintf(stdlog, "Successfully read %d bytes from socket %d, %d bytes to go\n",
                 received, sd, (int)messageBuffer->ToGo);
         LOG_END
         if(messageBuffer->ToGo == 0) {
            LOG_VERBOSE3
            fprintf(stdlog, "Successfully read message of %d bytes from socket %d\n",
                    (int)messageBuffer->Position, sd);
            LOG_END
            received      = messageBuffer->Position;
            messageBuffer->Position = 0;
         }
         else {
            return(RspRead_PartialRead);
         }
      }
   }

   if(received < 0) {
      received = RspRead_ReadError;
   }
   return(received);
}
#endif





/* ###### Reregistration timer ########################################### */
static void reregistrationTimer(struct Dispatcher* dispatcher,
                                struct Timer*      timer,
                                void*              userData)
{
   struct RSerPoolSocket* rserpoolSocket = (struct RSerPoolSocket*)userData;

   LOG_VERBOSE3
   fputs("Starting reregistration\n", stdlog);
   LOG_END

   /* ====== Do reregistration =========================================== */
   threadSafetyLock(&rserpoolSocket->PoolElement->Mutex);
   doRegistration(rserpoolSocket);
   timerStart(&rserpoolSocket->PoolElement->ReregistrationTimer,
              getMicroTime() + ((unsigned long long)1000 * (unsigned long long)rserpoolSocket->PoolElement->ReregistrationInterval));
   threadSafetyUnlock(&rserpoolSocket->PoolElement->Mutex);

   LOG_VERBOSE3
   fputs("Reregistration completed\n", stdlog);
   LOG_END
}



/* ###### Delete pool element ############################################ */
static void deletePoolElement(struct PoolElement* poolElement)
{
   int result;

   timerDelete(&poolElement->ReregistrationTimer);
   if(poolElement->Identifier != 0x00000000) {
      result = rspDeregister((unsigned char*)&poolElement->Handle.Handle,
                             poolElement->Handle.Size,
                             poolElement->Identifier,
                             NULL);
      if(result != RSPERR_OKAY) {
         LOG_ERROR
         fprintf(stdlog, "Deregistration failed: ");
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
   }
   threadSafetyDestroy(&poolElement->Mutex);
   free(poolElement);
}




/* ###### Reregistration ##################################################### */
static bool doRegistration(struct RSerPoolSocket* rserpoolSocket)
{
   struct TagItem              tags[16];
   struct EndpointAddressInfo* eai;
   struct EndpointAddressInfo* eai2;
   struct EndpointAddressInfo* next;
   union sockaddr_union*       sctpLocalAddressArray = NULL;
   union sockaddr_union*       localAddressArray     = NULL;
   union sockaddr_union        socketName;
   socklen_t                   socketNameLen;
   unsigned int                localAddresses;
   unsigned int                result;
   unsigned int                i;

   CHECK(rserpoolSocket->PoolElement != NULL);

   /* ====== Create EndpointAddressInfo structure =========================== */
   eai = (struct EndpointAddressInfo*)malloc(sizeof(struct EndpointAddressInfo));
   if(eai == NULL) {
      LOG_ERROR
      fputs("Out of memory\n", stdlog);
      LOG_END
      return(false);
   }
   threadSafetyLock(&rserpoolSocket->PoolElement->Mutex);

   eai->ai_family   = rserpoolSocket->SocketDomain;
   eai->ai_socktype = rserpoolSocket->SocketType;
   eai->ai_protocol = rserpoolSocket->SocketProtocol;
   eai->ai_next     = NULL;
   eai->ai_addr     = NULL;
   eai->ai_addrs    = 0;
   eai->ai_addrlen  = sizeof(union sockaddr_union);
   eai->ai_pe_id    = rserpoolSocket->PoolElement->Identifier;

   /* ====== Get local addresses for SCTP socket ========================= */
   if(rserpoolSocket->SocketProtocol == IPPROTO_SCTP) {
      eai->ai_addrs = getladdrsplus(rserpoolSocket->Socket, 0, &eai->ai_addr);
      if(eai->ai_addrs <= 0) {
         LOG_WARNING
         fputs("Unable to obtain socket's local addresses\n", stdlog);
         LOG_END
      }
      else {
         sctpLocalAddressArray = eai->ai_addr;

         /* --- Check for buggy sctplib/socketapi ----- */
         if( (getPort((struct sockaddr*)eai->ai_addr) == 0)                  ||
             ( (((struct sockaddr*)eai->ai_addr)->sa_family == AF_INET) &&
               (((struct sockaddr_in*)eai->ai_addr)->sin_addr.s_addr == 0) ) ||
             ( (((struct sockaddr*)eai->ai_addr)->sa_family == AF_INET6) &&
                (IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6*)eai->ai_addr)->sin6_addr))) ) {
            LOG_ERROR
            fputs("getladdrsplus() replies INADDR_ANY or port 0\n", stdlog);
            for(i = 0;i < eai->ai_addrs;i++) {
               fprintf(stdlog, "Address[%d] = ", i);
               fputaddress((struct sockaddr*)&eai->ai_addr[i], true, stdlog);
               fputs("\n", stdlog);
            }
            LOG_END_FATAL
            free(eai->ai_addr);
            eai->ai_addr   = NULL;
            eai->ai_addrs  = 0;
         }
         /* ------------------------------------------- */
      }
   }

   /* ====== Get local addresses for TCP/UDP socket ====================== */
   if(eai->ai_addrs <= 0) {
      /* Get addresses of all local interfaces */
      localAddresses = gatherLocalAddresses(&localAddressArray);
      if(localAddresses == 0) {
         LOG_ERROR
         fputs("Unable to find out local addresses -> No registration possible\n", stdlog);
         LOG_END
         free(eai);
         return(false);
      }

      /* Get local port number */
      socketNameLen = sizeof(socketName);
printf("so=%d\n",rserpoolSocket->Socket);
      if(ext_getsockname(rserpoolSocket->Socket, (struct sockaddr*)&socketName, &socketNameLen) == 0) {
         for(i = 0;i < localAddresses;i++) {
            setPort((struct sockaddr*)&localAddressArray[i], getPort((struct sockaddr*)&socketName));
         }
      }
      else {
         LOG_ERROR
         logerror("getsockname() failed");
         fputs("Unable to find out local port -> No registration possible\n", stdlog);
         LOG_END
         free(localAddressArray);
         free(eai);
         return(false);
      }

      /* Add addresses to EndpointAddressInfo structure */
      if(rserpoolSocket->SocketProtocol == IPPROTO_SCTP) {
         /* SCTP endpoints are described by a list of their
            transport addresses. */
         eai->ai_addrs = localAddresses;
         eai->ai_addr  = localAddressArray;
      }
      else {
         /* TCP/UDP endpoints are described by an own EndpointAddressInfo
            for each transport address. */
         eai2 = eai;
         for(i = 0;i < localAddresses;i++) {
            eai2->ai_addrs = 1;
            eai2->ai_addr  = &localAddressArray[i];
            if(i < localAddresses - 1) {
               next = (struct EndpointAddressInfo*)malloc(sizeof(struct EndpointAddressInfo));
               if(next == NULL) {
                  LOG_ERROR
                  fputs("Out of memory\n", stdlog);
                  LOG_END
                  free(localAddressArray);
                  while(eai != NULL) {
                     next = eai->ai_next;
                     free(eai);
                     eai = next;
                  }
                  return(false);
               }
               *next = *eai2;
               next->ai_next = NULL;
               eai2->ai_next    = next;
               eai2             = next;
            }
         }
      }
   }

   /* ====== Set policy type and parameters ================================= */
   tags[0].Tag  = TAG_PoolPolicy_Type;
   tags[0].Data = rserpoolSocket->PoolElement->LoadInfo.rli_policy;
   tags[1].Tag  = TAG_PoolPolicy_Parameter_Weight;
   tags[1].Data = rserpoolSocket->PoolElement->LoadInfo.rli_weight;
   tags[2].Tag  = TAG_PoolPolicy_Parameter_Load;
   tags[2].Data = rserpoolSocket->PoolElement->LoadInfo.rli_load;
   tags[3].Tag  = TAG_PoolPolicy_Parameter_LoadDegradation;
   tags[3].Data = rserpoolSocket->PoolElement->LoadInfo.rli_load_degradation;
   tags[4].Tag  = TAG_UserTransport_HasControlChannel;
   tags[4].Data = (tagdata_t)rserpoolSocket->PoolElement->HasControlChannel;
   tags[5].Tag  = TAG_END;

   /* ====== Do registration ================================================ */
   result = rspRegister((unsigned char*)&rserpoolSocket->PoolElement->Handle.Handle,
                        rserpoolSocket->PoolElement->Handle.Size,
                        eai, (struct TagItem*)&tags);
   if(result == RSPERR_OKAY) {
      rserpoolSocket->PoolElement->Identifier = eai->ai_pe_id;
      LOG_VERBOSE2
      fprintf(stdlog, "(Re-)Registration successful, ID is $%08x\n",
              rserpoolSocket->PoolElement->Identifier);
      LOG_END

      printf("(Re-)Registration successful, ID is $%08x\n",
             rserpoolSocket->PoolElement->Identifier);
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "(Re-)Registration failed: ");
      rserpoolErrorPrint(result, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }


   /* ====== Clean up ======================================================= */
   threadSafetyUnlock(&rserpoolSocket->PoolElement->Mutex);
   if(sctpLocalAddressArray) {
      free(sctpLocalAddressArray);
   }
   if(localAddressArray) {
      free(localAddressArray);
   }
   while(eai != NULL) {
      next = eai->ai_next;
      free(eai);
      eai = next;
   }

   return(true);
}



















static void rserpoolSocketPrint(const void* node, FILE* fd)
{
   const struct RSerPoolSocket* rserpoolSocket = (const struct RSerPoolSocket*)node;

   fprintf(fd, "%d ", rserpoolSocket->Descriptor);
}


static int rserpoolSocketComparison(const void* node1, const void* node2)
{
   const struct RSerPoolSocket* rserpoolSocket1 = (const struct RSerPoolSocket*)node1;
   const struct RSerPoolSocket* rserpoolSocket2 = (const struct RSerPoolSocket*)node2;

   if(rserpoolSocket1->Descriptor < rserpoolSocket2->Descriptor) {
      return(-1);
   }
   else if(rserpoolSocket1->Descriptor > rserpoolSocket2->Descriptor) {
      return(1);
   }
   return(0);
}


static struct Session* sessionFind(struct RSerPoolSocket* rserpoolSocket,
                                   rserpool_session_t     sessionID,
                                   sctp_assoc_t           assocID)
{
   struct Session* session;

   if(rserpoolSocket->ConnectedSession) {
      if(sessionID != 0) {
         LOG_WARNING
         fputs("Session ID given for connected RSerPool socket (there is only one session)\n", stdlog);
         LOG_END
      }
      if(assocID != 0) {
         LOG_WARNING
         fputs("Assoc ID given for connected RSerPool socket (there is only one session)\n", stdlog);
         LOG_END
      }
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
   LOG_ERROR
   fputs("What session are you looking for?\n", stdlog);
   LOG_END
   return(NULL);
}


static struct RSerPoolSocket* getRSerPoolSocketForDescriptor(int sd)
{
   struct RSerPoolSocket cmpSocket;
   cmpSocket.Descriptor = sd;

   struct RSerPoolSocket* rserpoolSocket = (struct RSerPoolSocket*)leafLinkedRedBlackTreeFind(&gRSerPoolSocketSet, &cmpSocket.Node);
   if(rserpoolSocket == NULL) {
      LOG_ERROR
      fprintf(stdlog, "Bad RSerPool socket descriptor %d\n", sd);
      LOG_END
   }
   return(rserpoolSocket);
}









/* ###### Initialize RSerPool API Library ################################ */
int rsp_initialize(struct rsp_info* info)
{
   rspInitialize(NULL);  // ???????????

   /* ====== Initialize session storage ================================== */
   leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);

   /* ====== Initialize RSerPool Socket descriptor storage =============== */
   leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);
   leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);
   gRSerPoolSocketAllocationBitmap = identifierBitmapNew(FD_SETSIZE);
   if(gRSerPoolSocketAllocationBitmap == NULL) {
      errno = ENOMEM;
      return(-1);
   }
   CHECK(identifierBitmapAllocateSpecificID(gRSerPoolSocketAllocationBitmap, 0) == 0);

   return(0);
}


/* ###### Clean-up RSerPool API Library ################################## */
void rsp_cleanup()
{
   leafLinkedRedBlackTreeDelete(&gRSerPoolSocketSet);


   /* ====== Clean-up RSerPool Socket descriptor storage ================= */
   leafLinkedRedBlackTreeDelete(&gRSerPoolSocketSet);
   for(int i = 1;i < FD_SETSIZE;i++) {
      if(identifierBitmapAllocateSpecificID(gRSerPoolSocketAllocationBitmap, i) < 0) {
         LOG_WARNING
         fprintf(stdlog, "RSerPool socket %d is still registered -> closing it\n", i);
         LOG_END
         rsp_close(i);
      }
   }
   identifierBitmapDelete(gRSerPoolSocketAllocationBitmap);
   gRSerPoolSocketAllocationBitmap = NULL;

   rspCleanUp();
}


/* ###### Create RSerPool socket ######################################### */
int rsp_socket(int domain, int type, int protocol, struct TagItem* tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    fd;

   /* ====== Check for problems ========================================== */
   if(protocol != IPPROTO_SCTP) {
      LOG_ERROR
      fputs("Only SCTP is supported for the Enhanced Mode API\n", stdlog);
      LOG_END
      errno = EPROTONOSUPPORT;
      return(-1);
   }

   /* ====== Create socket =============================================== */
   domain = (domain == 0) ? (checkIPv6() ? AF_INET6 : AF_INET) : domain;
   fd = ext_socket(domain, SOCK_SEQPACKET, protocol);
   if(fd <= 0) {
      LOG_ERROR
      logerror("Unable to create socket for RSerPool socket");
      LOG_END
      return(-1);
   }
   setNonBlocking(fd);

   /* ====== Configure SCTP socket ======================================= */
   if(!configureSCTPSocket(fd, 0, tags)) {
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

   /* ====== Create session ID allocation bitmap ========================= */
   rserpoolSocket->SessionAllocationBitmap = identifierBitmapNew(SESSION_SETSIZE);
   if(rserpoolSocket->SessionAllocationBitmap == NULL) {
      free(rserpoolSocket);
      close(fd);
      errno = ENOMEM;
      return(-1);
   }
   CHECK(identifierBitmapAllocateSpecificID(rserpoolSocket->SessionAllocationBitmap, 0) == 0);

   threadSafetyInit(&rserpoolSocket->Mutex, "RSerPoolSocket");
   leafLinkedRedBlackTreeNodeNew(&rserpoolSocket->Node);
   rserpoolSocket->Socket           = fd;
printf("SO=%d\n",fd);
   rserpoolSocket->SocketDomain     = domain;
   rserpoolSocket->SocketType       = type;
   rserpoolSocket->SocketProtocol   = protocol;
   rserpoolSocket->Descriptor       = -1;
   rserpoolSocket->PoolElement      = NULL;
   rserpoolSocket->ConnectedSession = NULL;
   sessionStorageNew(&rserpoolSocket->SessionSet);

   /* ====== Find available RSerPool socket descriptor =================== */
   dispatcherLock(&gDispatcher);
   rserpoolSocket->Descriptor = identifierBitmapAllocateID(gRSerPoolSocketAllocationBitmap);
   if(rserpoolSocket->Descriptor >= 0) {
      /* ====== Add RSerPool socket entry ================================ */
      CHECK(leafLinkedRedBlackTreeInsert(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
   }
   dispatcherUnlock(&gDispatcher);

   /* ====== Has there been a problem? =================================== */
   if(rserpoolSocket->Descriptor < 0) {
      identifierBitmapDelete(rserpoolSocket->SessionAllocationBitmap);
      free(rserpoolSocket);
      close(fd);
      errno = ENOSR;
      return(-1);
   }
   return(rserpoolSocket->Descriptor);
}


/* ###### Bind RSerPool socket ########################################### */
int rsp_bind(int sd, struct sockaddr* addrs, int addrcnt, struct TagItem* tags)
{
   RSerPoolSocket*      rserpoolSocket;
   union sockaddr_union localAddress;

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
      sendtoplus(rserpoolSocket->Socket, NULL, 0, SCTP_ABORT, NULL, 0, 0,
                 session->AssocID, 0, 0xffffffff, 0);
      deleteSession(rserpoolSocket, session);
      session = nextSession;
   }

   /* ====== Delete RSerPool socket entry ================================ */
   dispatcherLock(&gDispatcher);
   CHECK(leafLinkedRedBlackTreeRemove(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
   identifierBitmapFreeID(gRSerPoolSocketAllocationBitmap, sd);
   rserpoolSocket->Descriptor = -1;
   dispatcherUnlock(&gDispatcher);

   sessionStorageDelete(&rserpoolSocket->SessionSet);
   identifierBitmapDelete(rserpoolSocket->SessionAllocationBitmap);
   if(rserpoolSocket->Socket >= 0) {
      ext_close(rserpoolSocket->Socket);
      rserpoolSocket->Socket = -1;
   }
   threadSafetyDestroy(&rserpoolSocket->Mutex);
   free(rserpoolSocket);
   return(0);
}


/* ###### Register pool element ########################################## */
int rsp_register(int                        sd,
                 const unsigned char*       poolHandle,
                 const size_t               poolHandleSize,
                 const struct rsp_loadinfo* loadinfo,
                 struct TagItem*            tags)
{
   RSerPoolSocket*      rserpoolSocket;
   union sockaddr_union socketName;
   socklen_t            socketNameLen;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   /* ====== Check for problems ========================================== */
   socketNameLen = sizeof(socketName);
   if(ext_getsockname(rserpoolSocket->Socket, (struct sockaddr*)&socketName, &socketNameLen) < 0) {
      LOG_VERBOSE
      fprintf(stdlog, "RSerPool socket %d, socket %d is not bound -> trying to bind it to the ANY address\n",
              sd, rserpoolSocket->Socket);
      LOG_END
      if(rsp_bind(sd, NULL, 0, NULL) < 0) {
         return(-1);
      }
   }
   if(rserpoolSocket->PoolElement != NULL) {
      LOG_ERROR
      fprintf(stdlog, "RSerPool socket %d already is a pool element\n", sd);
      LOG_END
      errno = EBADF;
      return(-1);
   }
   if(poolHandleSize > MAX_POOLHANDLESIZE) {
      LOG_ERROR
      fputs("Pool handle too long\n", stdlog);
      LOG_END
      errno = ENAMETOOLONG;
      return(-1);
   }
   if(ext_listen(rserpoolSocket->Socket, 10) < 0) {
      LOG_ERROR
      logerror("Unable to set socket for new pool element to listen mode");
      LOG_END
      return(-1);
   }

   /* ====== Create pool element ========================================= */
   rserpoolSocket->PoolElement = (struct PoolElement*)malloc(sizeof(struct PoolElement));
   if(rserpoolSocket->PoolElement == NULL) {
      return(-1);
   }
   threadSafetyInit(&rserpoolSocket->PoolElement->Mutex, "RspPoolElement");
   poolHandleNew(&rserpoolSocket->PoolElement->Handle, poolHandle, poolHandleSize);
   timerNew(&rserpoolSocket->PoolElement->ReregistrationTimer,
            &gDispatcher,
            reregistrationTimer,
            (void*)rserpoolSocket);

   rserpoolSocket->PoolElement->Identifier             = tagListGetData(tags, TAG_PoolElement_Identifier,
                                                            0x00000000);
   rserpoolSocket->PoolElement->ReregistrationInterval = tagListGetData(tags, TAG_PoolElement_ReregistrationInterval,
                                                            60000);
   rserpoolSocket->PoolElement->RegistrationLife       = tagListGetData(tags, TAG_PoolElement_RegistrationLife,
                                                            (rserpoolSocket->PoolElement->ReregistrationInterval * 3) + 3000);
   rserpoolSocket->PoolElement->HasControlChannel      = tagListGetData(tags, TAG_UserTransport_HasControlChannel, false);

   rserpoolSocket->PoolElement->LoadInfo = *loadinfo;
   PolicyType                     = loadinfo->rli_policy;
   rserpoolSocket->PoolElement->PolicyParameterWeight          = loadinfo->rli_weight;
   rserpoolSocket->PoolElement->PolicyParameterLoad            = loadinfo->rli_load;
   rserpoolSocket->PoolElement->PolicyParameterLoadDeg = loadinfo->rli_load_degradation;

   /* ====== Do registration ============================================= */
   if(doRegistration(rserpoolSocket) == false) {
      LOG_ERROR
      fputs("Unable to obtain registration information -> Creating pool element not possible\n", stdlog);
      LOG_END
      deletePoolElement(rserpoolSocket->PoolElement);
      rserpoolSocket->PoolElement = NULL;
      return(-1);
   }

   /* ====== start reregistration timer ================================== */
   timerStart(&rserpoolSocket->PoolElement->ReregistrationTimer,
               getMicroTime() + ((unsigned long long)1000 * (unsigned long long)rserpoolSocket->PoolElement->ReregistrationInterval));

   return(0);
}


/* ###### Deregister pool element ######################################## */
int rsp_deregister(int sd)
{
   RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   CHECK_RSERPOOL_POOLELEMENT(rserpoolSocket);

   /* ====== Delete pool element ========================================= */
   deletePoolElement(rserpoolSocket->PoolElement);
   rserpoolSocket->PoolElement = NULL;

   return(0);
}



/* ###### Send cookie ######################################################## */
ssize_t rsp_send_cookie(int                  sd,
                        const unsigned char* cookie,
                        const size_t         cookieSize,
                        rserpool_session_t   sessionID,
                        unsigned long long   timeout,
                        struct TagItem*      tags)
{
   struct RSerPoolSocket*  rserpoolSocket;
   struct Session*         session;
   struct RSerPoolMessage* message;
   bool                    result = false;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   GET_RSERPOOL_SESSION(session, rserpoolSocket, sessionID, 0);

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
                                   (unsigned int)tagListGetData(tags, TAG_RspIO_Flags, MSG_NOSIGNAL),
                                   timeout,
                                   message);
      threadSafetyUnlock(&rserpoolSocket->Mutex);
      rserpoolMessageDelete(message);
   }

   return((result == true) ? (ssize_t)cookieSize : -1);
}


/* ###### RSerPool socket sendmsg() implementation ####################### */
ssize_t rsp_sendmsg(int                  sd,
                    const void*          data,
                    size_t               dataLength,
                    unsigned int         rserpoolFlags,
                    rserpool_session_t   sessionID,
                    const unsigned char* poolHandle,
                    size_t               poolHandleSize,
                    uint32_t             sctpPPID,
                    uint16_t             sctpStreamID,
                    uint32_t             sctpTimeToLive,
                    unsigned long long   timeout,
                    struct TagItem*      tags)
{
   struct RSerPoolSocket*  rserpoolSocket;
   struct Session*         session;
   ssize_t                 result;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   GET_RSERPOOL_SESSION(session, rserpoolSocket, sessionID, 0);

   LOG_VERBOSE1
   fprintf(stdlog, "Trying to send message via session %u of RSerPool socket %d, socket %d\n",
           session->SessionID,
           rserpoolSocket->Descriptor, rserpoolSocket->Socket);
   LOG_END
   result = sendtoplus(rserpoolSocket->Socket, data, dataLength,
                       MSG_NOSIGNAL,
                       NULL, 0,
                       sctpPPID, session->AssocID, sctpStreamID, sctpTimeToLive,
                       timeout);
   if((result < 0) && (errno != EAGAIN)) {
      LOG_ACTION
      fprintf(stdlog, "Session failure during send on RSerPool socket %d, session %u. Failover necessary\n",
              rserpoolSocket->Descriptor, session->SessionID);
      LOG_END
      return(-1);
/*      rspSessionFailover(session);
      return(RspRead_Failover);*/
   }
   tagListSetData(tags, TAG_RspIO_PE_ID, session->SessionID);
   return(result);
}


#define MSG_RSERPOOL_NOTIFICATION (1 << 0)
#define MSG_RSERPOOL_COOKIE_ECHO  (1 << 1)


/* ###### RSerPool socket recvmsg() implementation ####################### */
ssize_t rsp_recvmsg(int                  sd,
                    void*                buffer,
                    size_t               bufferLength,
                    unsigned int*        rserpoolFlags,
                    rserpool_session_t*  sessionID,
                    uint32_t*            poolElementID,
                    uint32_t*            sctpPPID,
                    uint16_t*            sctpStreamID,
                    unsigned long long   timeout,
                    struct TagItem*      tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   struct sctp_sndrcvinfo sinfo;
   sctp_assoc_t           assocID;
   int                    flags;
   ssize_t                received;

   unsigned long long     startTimeStamp;
   unsigned long long     now;
   long long              readTimeout;

//    uint32_t                 ppid;
//    sctp_assoc_t             assocID;
//    unsigned short           streamID;
//    int                      flags;
//    unsigned int             type;
//    size_t                   cookieLength;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   LOG_VERBOSE3
   fprintf(stdlog, "Trying to read message from RSerPool socket %d, socket %d\n",
           rserpoolSocket->Descriptor, rserpoolSocket->Socket);
   LOG_END

#if 0
   /* ====== Cookie Echo ================================================= */
   if((session->CookieEcho) && (bufferLength > 0)) {
      /* A cookie echo has been stored (during rspSessionSelect(). Now,
         the application calls this function. We now return the cookie
         and free its storage space. */
      LOG_ACTION
      fputs("There is a cookie echo. Giving it back first\n", stdlog);
      LOG_END
      tagListSetData(tags, TAG_RspIO_MsgIsCookieEcho, 1);
      cookieLength = min(length, session->CookieEchoSize);
      memcpy(buffer, session->CookieEcho, cookieLength);
      free(session->CookieEcho);
      session->CookieEcho     = NULL;
      session->CookieEchoSize = 0;
      return(cookieLength);
   }
#endif

   /* ====== Notification ================================================ */

   /* ====== Really read from socket ===================================== */
   do {
      /* ====== Check for timeout ======================================== */
      now = startTimeStamp = getMicroTime();
      readTimeout = (long long)timeout - ((long long)now - (long long)startTimeStamp);
      if(readTimeout < 0) {
         LOG_VERBOSE
         fprintf(stdlog, "Reading from RSerPool socket %d, socket %d timed out\n",
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
         errno = EAGAIN;
         return(0);
      }

      /* ====== Read from socket ========================================= */
      LOG_VERBOSE2
      fprintf(stdlog, "Trying to read from RSerPool socket %d, socket %d with timeout %Ldus\n",
              rserpoolSocket->Descriptor, rserpoolSocket->Socket,
              readTimeout);
      LOG_END
      received = recvfromplus(rserpoolSocket->Socket,
                              (char*)&rserpoolSocket->MessageBuffer,
                              sizeof(rserpoolSocket->MessageBuffer),
                              &flags, NULL, 0,
                              sctpPPID, &assocID, sctpStreamID,
                              readTimeout);
      LOG_VERBOSE2
      fprintf(stdlog, "received=%d\n", received);
      LOG_END
      if(received > 0) {
         /* ====== Handle notification =================================== */
         if(flags & MSG_NOTIFICATION) {
            handleNotification(rserpoolSocket,
                               (const char*)&rserpoolSocket->MessageBuffer, received);
         }

         /* ====== Handle ASAP control channel message =================== */
         else if(sinfo.sinfo_ppid == PPID_ASAP) {
            handleControlChannelMessage(rserpoolSocket,
                                        (const char*)&rserpoolSocket->MessageBuffer, received);
         }

         /* ====== User Data ============================================= */
         else {
            break;
         }
      }
   } while(received > 0);

   if(received > 0) {
      /* ====== Find session ============================================= */
//  ???      threadSafetyLock(&rserpoolSocket->Mutex);
      session = sessionStorageFindSessionByAssocID(&rserpoolSocket->SessionSet, assocID);
      if(session) {
         *sessionID = session->SessionID;
      }
//  ???      threadSafetyUnlock(&rserpoolSocket->Mutex);

      /* ====== Copy user data =========================================== */
      if((ssize_t)bufferLength < received) {
         LOG_ERROR
         fputs("Buffer is too small to keep full message\n", stdlog);
         LOG_END
         errno = ENOMEM;
         return(-1);
      }
      received = min(received, (ssize_t)bufferLength);
      memcpy(buffer, (const char*)&rserpoolSocket->MessageBuffer, received);
   }

//       /* ====== Handle ASAP messages ===================================== */
//       if((received > 0) && (sctpPPID == PPID_ASAP)) {
//          handleControlChannelMessage(rserpoolSocket, (char*)&rserpoolSocket->MessageBuffer, received);
//       }
      /* ====== Handle failover ======================================= */
//       else if((result == 0) ||
//               (result == RspRead_ReadError)) {
//          if(tagListGetData(tags, TAG_RspIO_MakeFailover, 1) != 0) {
//             LOG_VERBOSE
//             fprintf(stdlog, "Session failure during read, socket %d. Failover necessary\n",
//                     session->Socket);
//             LOG_END
//             rspSessionFailover(session);
//             return(RspRead_Failover);
//          }
//          LOG_VERBOSE
//          fprintf(stdlog, "Session failure during read, socket %d. Failover turned off -> returning\n",
//                  session->Socket);
//          LOG_END
//          return(RspRead_ReadError);
//       }

/*
   if(result == RspRead_PartialRead) {
      LOG_VERBOSE2
      fprintf(stdlog, "Partially read message data from socket %d\n", session->Socket);
      LOG_END
      errno = EAGAIN;
      return(result);
   }
   else if(result == RspRead_TooBig) {
      LOG_ERROR
      fprintf(stdlog, "Message on %d is too big\n", session->Socket);
      LOG_END
      errno = EIO;
      return(result);
   }
   else if(result == RspRead_WrongPPID) {
      if(length > 0) {
         LOG_VERBOSE2
         fprintf(stdlog, "No message -> Trying to read up to %u bytes of user data on socket %d\n",
                 (int)length, session->Socket);
         LOG_END
         flags  = tagListGetData(tags, TAG_RspIO_Flags, MSG_NOSIGNAL);
         result = recvfromplus(session->Socket, buffer, length,
                               &flags,
                               NULL, 0,
                               &ppid, &assocID, &streamID,
                               timeout);
      }
      else {
         LOG_VERBOSE4
         fputs("Check for control data completed -> returning\n", stdlog);
         LOG_END
         return(result);
      }
   }

   if(result > 0) {
      tagListSetData(tags, TAG_RspIO_SCTP_AssocID,  (tagdata_t)assocID);
      tagListSetData(tags, TAG_RspIO_SCTP_StreamID, streamID);
      tagListSetData(tags, TAG_RspIO_SCTP_PPID,     ppid);
      tagListSetData(tags, TAG_RspIO_PE_ID,         session->Identifier);
   }*/
   return(received);
}



static void handleControlChannelMessage(struct RSerPoolSocket* rserpoolSocket,
                                        const char*            buffer,
                                        size_t                 bufferSize)
{
   LOG_ACTION
   fprintf(stdlog, "ASAP control channel message for RSerPool socket %d, socket %d\n",
            rserpoolSocket->Descriptor, rserpoolSocket->Socket);
   LOG_END

#if 0
         type = handleRSerPoolMessage(session, (char*)&session->MessageBuffer->Buffer, (size_t)result);
         switch(type) {
            case AHT_COOKIE_ECHO:
               if(session->CookieEcho) {
                  tagListSetData(tags, TAG_RspIO_MsgIsCookieEcho, 1);
                  cookieLength = min(length, session->CookieEchoSize);
                  if(cookieLength > 0) {
                     /* The function is called from the user program.
                        Give the cookie echo back now. */
                     memcpy(buffer, session->CookieEcho, cookieLength);
                     free(session->CookieEcho);
                     session->CookieEcho     = NULL;
                     session->CookieEchoSize = 0;
                  }
                  return(cookieLength);
               }
            break;
         }
#endif
}


#define SNT_COMM_LOST      1
#define SNT_SHUTDOWN_EVENT 2

void notifySession(struct RSerPoolSocket* rserpoolSocket,
                   sctp_assoc_t           assocID,
                   unsigned int           notificationType)
{
   struct Session*          session;

   threadSafetyLock(&rserpoolSocket->Mutex);
   session = sessionStorageFindSessionByAssocID(&rserpoolSocket->SessionSet,
                                                assocID);
   if(session) {
      switch(notificationType) {
         case SNT_COMM_LOST:
         case SNT_SHUTDOWN_EVENT:
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


static void handleNotification(struct RSerPoolSocket* rserpoolSocket,
                               const char*            buffer,
                               size_t                 bufferSize)
{
   union sctp_notification* notification;

   notification = (union sctp_notification*)buffer;
   LOG_VERBOSE1
   fprintf(stdlog, "Notification %d for RSerPool socket %d, socket %d\n",
            notification->sn_header.sn_type,
            rserpoolSocket->Descriptor, rserpoolSocket->Socket);
   LOG_END

   if(notification->sn_header.sn_type == SCTP_ASSOC_CHANGE) {
printf("ASSOC STATE=%d\n",notification->sn_assoc_change.sac_state);
      switch(notification->sn_assoc_change.sac_state) {
         case SCTP_COMM_UP:
            LOG_ACTION
            fprintf(stdlog, "SCTP_COMM_UP for RSerPool socket %d, socket %d, assoc %u\n",
                  rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                  notification->sn_assoc_change.sac_assoc_id);
            LOG_END
            if(addSession(rserpoolSocket,
                          notification->sn_assoc_change.sac_assoc_id,
                          true, NULL) == NULL) {
               LOG_WARNING
               fprintf(stderr, "Aborting association %u on RSerPool socket %d, socket %d, due to session creation failure\n",
                       notification->sn_assoc_change.sac_assoc_id,
                       rserpoolSocket->Descriptor, rserpoolSocket->Socket);
               LOG_END
               sendtoplus(rserpoolSocket->Socket, NULL, 0, SCTP_ABORT, NULL, 0, 0,
                          notification->sn_assoc_change.sac_assoc_id, 0, 0xffffffff, 0);
            }
          break;
            LOG_ACTION
            fprintf(stdlog, "SCTP_COMM_LOST for RSerPool socket %d, socket %d, assoc %u\n",
                  rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                  notification->sn_assoc_change.sac_assoc_id);
            LOG_END
            notifySession(rserpoolSocket,
                          notification->sn_assoc_change.sac_assoc_id,
                          SNT_COMM_LOST);
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
                    SNT_SHUTDOWN_EVENT);
   }
printf("TYPE=%d  %d  %d\n",notification->sn_header.sn_type,notification->sn_header.sn_length,notification->sn_header.sn_flags);
}


static bool handleControlChannelAndNotifications(struct RSerPoolSocket* rserpoolSocket)
{
   char                     buffer[65536];
   struct sctp_sndrcvinfo   sinfo;
   ssize_t                  result;
   int                      flags;

   /* ====== Check, if message on socket is notification or ASAP ========= */
puts("peek...");
   flags = MSG_PEEK;
   result = sctp_recvmsg(rserpoolSocket->Socket, (char*)&buffer, 4,
                         NULL, NULL, &sinfo, &flags);
printf("flgs=%d   notif=%d\n",flags, (flags & MSG_NOTIFICATION) != 0);
   if( (result <= 0) ||
       (! ((flags & MSG_NOTIFICATION) ||
          (sinfo.sinfo_ppid == PPID_ASAP)) ) ) {
      /* Message is user data, go back and let user read it. */
      puts("peek ok -> user data");
      return(false);
   }

   LOG_ACTION
   fputs("Handling control channel message or notification...\n", stdlog);
   LOG_END
   rsp_recvmsg(rserpoolSocket->Descriptor, NULL, 0, NULL, NULL, NULL, NULL, NULL, 0, NULL);

// ssize_t rsp_recvmsg(int                  sd,
//                     void*                buffer,
//                     size_t               bufferLength,
//                     unsigned int*        rserpoolFlags,
//                     rserpool_session_t*  sessionID,
//                     uint32_t*            poolElementID,
//                     uint32_t*            sctpPPID,
//                     uint16_t*            sctpStreamID,
//                     unsigned long long   timeout,
//                     struct TagItem*      tags)
//
// puts("NOTIF!");
//    /* ====== Read complete message ======================================= */
//    flags = 0;
//    result = sctp_recvmsg(rserpoolSocket->Socket, (char*)&buffer, sizeof(buffer),
//                          NULL, NULL, &sinfo, &flags);
//    if(result <= 0) {
//       LOG_ERROR
//       logerror("Failed to read notification or ASAP control channel message");
//       LOG_END
//       return(true);
//    }
//
//    /* ====== Handle notification ========================================= */
//    if(flags & MSG_NOTIFICATION) {
//       handleNotification(rserpoolSocket, (const char*)&buffer, result);
//    }
//
//    /* ====== Handle ASAP control channel message ========================= */
//    else if(sinfo.sinfo_ppid == PPID_ASAP) {
//       handleControlChannelMessage(rserpoolSocket, (const char*)&buffer, result);
//    }

   return(true);
}


static int setFDs(fd_set* fds, int sd)
{
   struct RSerPoolSocket* rserpoolSocket;
   int                    n = 0;

   rserpoolSocket = getRSerPoolSocketForDescriptor(sd);
   if(rserpoolSocket != NULL) {
      threadSafetyLock(&rserpoolSocket->Mutex);
      if(rserpoolSocket->Socket >= 0) {
         FD_SET(rserpoolSocket->Socket, fds);
         n = max(n, rserpoolSocket->Socket);
      }
      threadSafetyUnlock(&rserpoolSocket->Mutex);
   }
   return(n);
}


inline static int transferFD(int inSD, const fd_set* inFD, int outSD, fd_set* outFD)
{
   if((inFD != NULL) && (outFD != NULL)) {
      if(FD_ISSET(inSD, inFD)) {
         FD_SET(outSD, outFD);
         return(1);
      }
      FD_CLR(outSD, outFD);
   }
   return(0);
}


/* ###### RSerPool socket select() implementation ######################## */
int rsp_select(int                      rserpoolN,
               fd_set*                  rserpoolReadFDs,
               fd_set*                  rserpoolWriteFDs,
               fd_set*                  rserpoolExceptFDs,
               const unsigned long long timeout,
               struct TagItem*          tags)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   struct timeval         mytimeout;
   fd_set                 myreadfds, mywritefds, myexceptfds;
   fd_set*                readfds    = (fd_set*)tagListGetData(tags, TAG_RspSelect_ReadFDs,   (tagdata_t)&myreadfds);
   fd_set*                writefds   = (fd_set*)tagListGetData(tags, TAG_RspSelect_WriteFDs,  (tagdata_t)&mywritefds);
   fd_set*                exceptfds  = (fd_set*)tagListGetData(tags, TAG_RspSelect_ExceptFDs, (tagdata_t)&myexceptfds);
   struct timeval*        to = (struct timeval*)tagListGetData(tags, TAG_RspSelect_Timeout,   (tagdata_t)&mytimeout);
   int                    result;
   int                    i;
   int                    n;

   FD_ZERO(&myreadfds);
   FD_ZERO(&mywritefds);
   FD_ZERO(&myexceptfds);
   mytimeout.tv_sec  = timeout / 1000000;
   mytimeout.tv_usec = timeout % 1000000;

   /* ====== Collect data for select() call =============================== */
   n = tagListGetData(tags, TAG_RspSelect_MaxFD, 0);
   if(rserpoolReadFDs) {
      for(i = 0;i < rserpoolN;i++) {
         if(FD_ISSET(i, rserpoolReadFDs)) {
            n = max(n, setFDs(readfds, i));
         }
      }
   }
   if(rserpoolWriteFDs) {
      for(i = 0;i < rserpoolN;i++) {
         if(FD_ISSET(i, rserpoolWriteFDs)) {
            n = max(n, setFDs(writefds, i));
         }
      }
   }
   if(rserpoolExceptFDs) {
      for(i = 0;i < rserpoolN;i++) {
         if(FD_ISSET(i, rserpoolExceptFDs)) {
            n = max(n, setFDs(exceptfds, i));
         }
      }
   }

   /* ====== Do select() call ============================================= */
   if(tagListGetData(tags, TAG_RspSelect_RsplibEventLoop, 0) != 0) {
      LOG_VERBOSE2
      fputs("Calling rspSelect()\n", stdlog);
      LOG_END
      result = rspSelect(n + 1, readfds, writefds, exceptfds, to);
   }
   else {
      LOG_VERBOSE3
      fputs("Calling ext_select()\n", stdlog);
      LOG_END
      result = ext_select(n + 1, readfds, writefds, exceptfds, to);
   }
   LOG_VERBOSE3
   fprintf(stdlog, "Select result=%d\n", result);
   LOG_END

   /* ====== Handle results of select() call ============================== */
   if(result > 0) {
      result = 0;
      for(i = 0;i < rserpoolN;i++) {
         if( ((rserpoolReadFDs) && FD_ISSET(i, rserpoolReadFDs)) ||
             ((rserpoolWriteFDs) && FD_ISSET(i, rserpoolWriteFDs)) ||
             ((rserpoolExceptFDs) && FD_ISSET(i, rserpoolExceptFDs)) ) {
            rserpoolSocket = getRSerPoolSocketForDescriptor(i);
            if(rserpoolSocket != NULL) {
               threadSafetyLock(&rserpoolSocket->Mutex);
               if(rserpoolSocket->Socket >= 0) {
puts("------------ EVENT");


                  /* ====== Transfer events ============================== */
                  result +=
                     ((transferFD(rserpoolSocket->Socket, readfds, i, rserpoolReadFDs) +
                       transferFD(rserpoolSocket->Socket, writefds, i, rserpoolWriteFDs) +
                       transferFD(rserpoolSocket->Socket, exceptfds, i, rserpoolExceptFDs)) > 0) ? 1 : 0;

                  /* ======= Check for control channel data ============== */
                  if(FD_ISSET(rserpoolSocket->Socket, readfds)) {
                     LOG_VERBOSE4
                     fprintf(stdlog, "RSerPool socket %d (socket %d) has <read> flag set -> Check, if it has to be handled by rsplib...\n",
                              rserpoolSocket->Descriptor, session->SessionID);
                     LOG_END
                     /* ?????????????
                     if(handleControlChannelAndNotifications(rserpoolSocket)) {
                        LOG_VERBOSE4
                        fprintf(stdlog, "RSerPool socket %d (socket %d) had <read> event for rsplib only. Clearing <read> flag\n",
                                 rserpoolSocket->Descriptor, session->SessionID);
                        LOG_END
                        FD_CLR(i, rserpoolReadFDs);
                     }
                     */
                  }
                  if((rserpoolSocket->Socket >= 0) &&
                     FD_ISSET(rserpoolSocket->Socket, readfds)) {
                     LOG_VERBOSE4
                     fprintf(stdlog, "RSerPool socket %d (socket %d) has <read> flag set -> Checking for ASAP message...\n",
                              rserpoolSocket->Descriptor, session->SessionID);
                     LOG_END
                  }

 puts("------------");
               }
               threadSafetyUnlock(&rserpoolSocket->Mutex);
            }
         }
      }
   }

   return(result);
}






// AF_INET, SOCK_DGRAM, IPPROTO_SCTP);
//
//
//   ssize_t rsp_sendmsg(int sockfd,
//                          struct msghdr *msg,
//                          int flags);
//
//      ssize_t rsp_rcvmsg(int sockfd,
//                         struct msghdr *msg,
//                         int flags);
//
//
// rsp_initialize(&info);
// rsp_socket(AF_INET, SOCK_DGRAM, IPPROTO_SCTP);
// rsp_register(fd, "echo", 4, &linfo);
// rsp_bind(fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));
// rsp_deregister(fd);
// rsp_close(fd);
// rsp_cleanup();


#include "breakdetector.h"

int main(int argc, char** argv)
{
   TagItem tags[16];
//    size_t i;
   int sd;
   fd_set readfds;
   int n;
   struct rsp_loadinfo loadinfo;
   struct rsp_info info;

   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-log" ,4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
   }
   beginLogging();

   rsp_initialize(&info);

//    for(i = 0;i < 1028;i++) {
//       printf("%d: \n",i);
//       sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
//       if(sd > 0) {
//          rsp_register(sd, (const unsigned char*)"ECHO", 4, NULL);
//       }
//       printf("    => sd=%d\n",sd);
//    }
//
//    puts("TREE: ");
//    leafLinkedRedBlackTreePrint(&gRSerPoolSocketSet, stdout);
//    puts("---");
//
//    for(i = 100;i < 200;i+=3) {
//       sd = rsp_close(i);
//    }
//    for(i = 100;i < 215;i+=3) {
//       printf("%d: \n",i);
//       sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
//       printf("    *** sd=%d\n",sd);
//    }

   sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP, NULL);
   CHECK(sd > 0);

   memset(&loadinfo, 0, sizeof(loadinfo));
   loadinfo.rli_policy = PPT_ROUNDROBIN;
   rsp_register(sd, (const unsigned char*)"EchoPool", 8, &loadinfo, NULL);

   installBreakDetector();
   while(!breakDetected()) {
//       puts("sel...");
      FD_ZERO(&readfds);
      FD_SET(sd, &readfds);
      n = sd;

      tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
      tags[0].Data = 1;
      tags[1].Tag  = TAG_DONE;
      int result = rsp_select(n + 1, &readfds, NULL, NULL, 1000000, (struct TagItem*)&tags);

      if(result > 0) {
         printf("result=%d\n", result);
         if(FD_ISSET(sd, &readfds)) {
            puts("READ EVENT!");

            char                buffer[1024];
            unsigned int        rserpoolFlags = 0;
            rserpool_session_t  sessionID     = 0;
            uint32_t            poolElementID = 0;
            uint32_t            sctpPPID      = 0;
            uint16_t            sctpStreamID  = 0;
            ssize_t rv = rsp_recvmsg(sd, (char*)&buffer, sizeof(buffer), &rserpoolFlags,
                                     &sessionID,&poolElementID,&sctpPPID,&sctpStreamID,
                                     0, NULL);
            printf("===========>> rv=%d  session=%u   ppid=%x\n", rv, sessionID, sctpPPID);
            if((rv > 0) && (!(rserpoolFlags & MSG_RSERPOOL_NOTIFICATION))) {
               rserpoolFlags = 0;
               ssize_t snd = rsp_sendmsg(sd, (char*)&buffer, sizeof(buffer), rserpoolFlags,
                                         sessionID, NULL, 0, sctpPPID, sctpStreamID, ~0, 0, NULL);
            }
         }
      }
   }

puts("DEREG...");
   rsp_deregister(sd);
puts("CLOSE...");
   rsp_close(sd);

puts("CLEAN");
   rsp_cleanup();
   finishLogging();
}
