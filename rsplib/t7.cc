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

#include "asapinstance.h"
extern struct ASAPInstance* gAsapInstance;
extern struct ThreadSafety  gThreadSafety;
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#include <ext_socket.h>



struct IdentifierBitmap
{
   size_t Entries;
   size_t Available;

   size_t Slots;
   size_t Bitmap[0];
};

#define IdentifierBitmapSlotsize (sizeof(size_t) * 8)



struct rserpool_addrinfo {
   int                       ai_family;
   int                       ai_socktype;
   int                       ai_protocol;
   size_t                    ai_addrlen;
   size_t                    ai_addrs;
   struct sockaddr*          ai_addr;
   struct rserpool_addrinfo* ai_next;
   uint32_t                  ai_pe_id;
};


/* ###### Handle resolution ############################################## */
unsigned int rsp_getaddrinfo(const unsigned char*       poolHandle,
                             const size_t               poolHandleSize,
                             struct rserpool_addrinfo** rserpoolAddrInfo,
                             struct TagItem*            tags)
{
   struct PoolHandle                 myPoolHandle;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            poolElementNodes;
   unsigned int                      result;
   char*                             ptr;
   size_t                            i;

   if(gAsapInstance) {
      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);

      poolElementNodes = 1;
      result = asapInstanceHandleResolution(
                  gAsapInstance,
                  &myPoolHandle,
                  (struct ST_CLASS(PoolElementNode)**)&poolElementNode,
                  &poolElementNodes);
      if(result == RSPERR_OKAY) {
         *rserpoolAddrInfo = (struct rserpool_addrinfo*)malloc(sizeof(struct rserpool_addrinfo));
         if(rserpoolAddrInfo != NULL) {
            (*rserpoolAddrInfo)->ai_next     = NULL;
            (*rserpoolAddrInfo)->ai_pe_id    = poolElementNode->Identifier;
            (*rserpoolAddrInfo)->ai_family   = poolElementNode->UserTransport->AddressArray[0].sa.sa_family;
            (*rserpoolAddrInfo)->ai_protocol = poolElementNode->UserTransport->Protocol;
            switch(poolElementNode->UserTransport->Protocol) {
               case IPPROTO_SCTP:
                  (*rserpoolAddrInfo)->ai_socktype = SOCK_STREAM;
                break;
               case IPPROTO_TCP:
                  (*rserpoolAddrInfo)->ai_socktype = SOCK_STREAM;
                break;
               default:
                  (*rserpoolAddrInfo)->ai_socktype = SOCK_DGRAM;
                break;
            }
            (*rserpoolAddrInfo)->ai_addrlen = sizeof(union sockaddr_union);
            (*rserpoolAddrInfo)->ai_addrs   = poolElementNode->UserTransport->Addresses;
            (*rserpoolAddrInfo)->ai_addr    = (struct sockaddr*)malloc((*rserpoolAddrInfo)->ai_addrs * sizeof(union sockaddr_union));
            if((*rserpoolAddrInfo)->ai_addr != NULL) {
               ptr = (char*)(*rserpoolAddrInfo)->ai_addr;
               for(i = 0;i < poolElementNode->UserTransport->Addresses;i++) {
                  memcpy((void*)ptr, (void*)&poolElementNode->UserTransport->AddressArray[i],
                         sizeof(union sockaddr_union));
                  if (poolElementNode->UserTransport->AddressArray[i].sa.sa_family == AF_INET6)
                    (*rserpoolAddrInfo)->ai_family = AF_INET6;
                  switch(poolElementNode->UserTransport->AddressArray[i].sa.sa_family) {
                     case AF_INET:
                        ptr = (char*)((long)ptr + (long)sizeof(struct sockaddr_in));
                      break;
                     case AF_INET6:
                        ptr = (char*)((long)ptr + (long)sizeof(struct sockaddr_in6));
                      break;
                     default:
                        LOG_ERROR
                        fprintf(stdlog, "Bad address type #%d\n",
                                poolElementNode->UserTransport->AddressArray[i].sa.sa_family);
                        LOG_END_FATAL
                      break;
                  }
               }
            }
            else {
               free(*rserpoolAddrInfo);
               *rserpoolAddrInfo = NULL;
            }
         }
         else {
            result = RSPERR_OUT_OF_MEMORY;
         }
      }
   }
   else {
      result = RSPERR_NOT_INITIALIZED;
      LOG_ERROR
      fputs("rsplib is not initialized\n", stdlog);
      LOG_END
   }

   return(result);
}


/* ###### Free endpoint address array #################################### */
void rsp_freeaddrinfo(struct rserpool_addrinfo* rserpoolAddrInfo)
{
   struct rserpool_addrinfo* next;

   while(rserpoolAddrInfo != NULL) {
      next = rserpoolAddrInfo->ai_next;

      if(rserpoolAddrInfo->ai_addr) {
         free(rserpoolAddrInfo->ai_addr);
         rserpoolAddrInfo->ai_addr = NULL;
      }
      free(rserpoolAddrInfo);

      rserpoolAddrInfo = next;
   }
}


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
   uint16_t           rf_type;
   uint16_t           rf_flags;
   uint32_t           rf_length;
   int                rf_state;
   rserpool_session_t rf_session;
};

#define RSERPOOL_FAILOVER_NECESSARY 1
#define RSERPOOL_FAILOVER_COMPLETE  2


struct rserpool_session_change
{
   uint16_t           rsc_type;
   uint16_t           rsc_flags;
   uint32_t           rsc_length;
   int                rsc_state;
   rserpool_session_t rsc_session;
};

#define RSERPOOL_SESSION_ADD    1
#define RSERPOOL_SESSION_REMOVE 2


union rserpool_notification
{
   struct {
      uint16_t rn_type;
      uint16_t rn_flags;
      uint32_t rn_length;
   }                              rn_header;
   struct rserpool_failover       rn_failover;
   struct rserpool_session_change rn_session_change;
};

#define RSERPOOL_SESSION_CHANGE 1
#define RSERPOOL_FAILOVER       2


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

      default:
         fprintf(fd, "Unknown type %d!\n", notification->rn_header.rn_type);
       break;
   }
}


struct rsp_sndrcvinfo
{
   rserpool_session_t rinfo_session;
   uint32_t           rinfo_ppid;
   uint32_t           rinfo_pe_id;
   uint32_t           rinfo_timetolive;
   uint16_t           rinfo_stream;
};


// ????????????
int rsp_mapsocket(int sd, int toSD);
int rsp_unmapsocket(int sd);
int rsp_deregister(int sd);
int rsp_close(int sd);
int rsp_poll(struct pollfd* ufds, unsigned int nfds, int timeout);
int rsp_select(int n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
               struct timeval* timeout);
unsigned int rsp_pe_registration(const unsigned char*      poolHandle,
                                 const size_t              poolHandleSize,
                                 struct rserpool_addrinfo* rserpoolAddrInfo,
                                 struct TagItem*           tags);
unsigned int rsp_pe_deregistration(const unsigned char* poolHandle,
                                   const size_t         poolHandleSize,
                                   const uint32_t       identifier,
                                   struct TagItem*      tags);
unsigned int rsp_pe_failure(const unsigned char* poolHandle,
                            const size_t         poolHandleSize,
                            const uint32_t       identifier,
                            struct TagItem*      tags);
static bool sendCookieEcho(struct RSerPoolSocket* rserpoolSocket,
                           struct Session*        session);
static bool doRegistration(struct RSerPoolSocket* rserpoolSocket);
static void deletePoolElement(struct PoolElement* poolElement);
static bool handleControlChannelAndNotifications(struct RSerPoolSocket* rserpoolSocket);
static void handleControlChannelMessage(struct RSerPoolSocket* rserpoolSocket,
                                        const sctp_assoc_t     assocID,
                                        char*                  buffer,
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
struct ThreadSafety           gRSerPoolSocketSetMutex;
struct IdentifierBitmap*      gRSerPoolSocketAllocationBitmap;









struct rsp_info
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



struct PoolElement
{
   struct PoolHandle                 Handle;
   uint32_t                          Identifier;
   struct ThreadSafety               Mutex;

   struct rsp_loadinfo               LoadInfo;

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

   struct PoolHandle                 Handle;
   PoolElementIdentifierType         ConnectedPE;
   bool                              IsIncoming;
   bool                              IsFailed;
   unsigned long long                ConnectionTimeStamp;

   unsigned int                      PendingNotifications;
   unsigned int                      EventMask;

   void*                             Cookie;
   size_t                            CookieSize;
   void*                             CookieEcho;
   size_t                            CookieEchoSize;

   unsigned long long                ConnectTimeout;
   unsigned long long                HandleResolutionRetryDelay;

   struct TagItem*                   Tags;

   char                              StatusText[128];
};


#define SNT_FAILOVER_NECESSARY ((unsigned int)1 << 0)
#define SNT_FAILOVER_COMPLETE  ((unsigned int)1 << 1)
#define SNT_SESSION_ADD        ((unsigned int)1 << 2)
#define SNT_SESSION_REMOVE     ((unsigned int)1 << 3)

#define SNT_NOTIFICATION_MASK (SNT_FAILOVER_NECESSARY|SNT_FAILOVER_COMPLETE|SNT_SESSION_ADD|SNT_SESSION_REMOVE)


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
   char*                             MessageBuffer;
};

#define RSERPOOL_MESSAGE_BUFFER_SIZE 65536




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


/* ###### Get number of sessions ######################################### */
size_t sessionStorageGetElements(struct SessionStorage* sessionStorage)
{
   return(leafLinkedRedBlackTreeGetElements(&sessionStorage->SessionIDSet));
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
   /* Thread-Safety Notes:
      This function will only be called when the RSerPool socket is existing
      and it has a PoolElement entry. Therefore, locking the
      rseroolSocket->Mutex is not necessary here. It is only necessary to
      ensure that the PE information is not altered by another thread while
      being extracted by doRegistration(). */
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

   /* Delete timer first; this ensures that the doRegistration() function
      will not be called while the PE is going to be deleted! */
   timerDelete(&poolElement->ReregistrationTimer);

   if(poolElement->Identifier != 0x00000000) {
      result = rsp_pe_deregistration((unsigned char*)&poolElement->Handle.Handle,
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
   struct TagItem            tags[16];
   struct rserpool_addrinfo* rspAddrInfo;
   struct rserpool_addrinfo* rspAddrInfo2;
   struct rserpool_addrinfo* next;
   struct sockaddr*          sctpLocalAddressArray = NULL;
   struct sockaddr*          packedAddressArray    = NULL;
   union sockaddr_union*     localAddressArray     = NULL;
   union sockaddr_union      socketName;
   socklen_t                 socketNameLen;
   unsigned int              localAddresses;
   unsigned int              result;
   unsigned int              i;

   CHECK(rserpoolSocket->PoolElement != NULL);

   /* ====== Create rserpool_addrinfo structure ========================== */
   rspAddrInfo = (struct rserpool_addrinfo*)malloc(sizeof(struct rserpool_addrinfo));
   if(rspAddrInfo == NULL) {
      LOG_ERROR
      fputs("Out of memory\n", stdlog);
      LOG_END
      return(false);
   }

   rspAddrInfo->ai_family   = rserpoolSocket->SocketDomain;
   rspAddrInfo->ai_socktype = rserpoolSocket->SocketType;
   rspAddrInfo->ai_protocol = rserpoolSocket->SocketProtocol;
   rspAddrInfo->ai_next     = NULL;
   rspAddrInfo->ai_addr     = NULL;
   rspAddrInfo->ai_addrs    = 0;
   rspAddrInfo->ai_addrlen  = sizeof(union sockaddr_union);
   rspAddrInfo->ai_pe_id    = rserpoolSocket->PoolElement->Identifier;

   /* ====== Get local addresses by sctp_getladds() ====================== */
   rspAddrInfo->ai_addrs = 0;
   if(rserpoolSocket->SocketProtocol == IPPROTO_SCTP) {
      rspAddrInfo->ai_addrs = sctp_getladdrs(rserpoolSocket->Socket, 0, &rspAddrInfo->ai_addr);
      if(rspAddrInfo->ai_addrs <= 0) {
         LOG_WARNING
         fputs("Unable to obtain socket's local addresses using sctp_getladdrs\n", stdlog);
         LOG_END
      }
      else {
         /* --- Check for buggy sctplib/socketapi ----- */
         if( (getPort((struct sockaddr*)rspAddrInfo->ai_addr) == 0)                  ||
             ( (((struct sockaddr*)rspAddrInfo->ai_addr)->sa_family == AF_INET) &&
               (((struct sockaddr_in*)rspAddrInfo->ai_addr)->sin_addr.s_addr == 0) ) ||
             ( (((struct sockaddr*)rspAddrInfo->ai_addr)->sa_family == AF_INET6) &&
                (IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6*)rspAddrInfo->ai_addr)->sin6_addr))) ) {
            LOG_ERROR
            fputs("sctp_getladdrs() replies INADDR_ANY or port 0\n", stdlog);
            for(i = 0;i < rspAddrInfo->ai_addrs;i++) {
               fprintf(stdlog, "Address[%d] = ", i);
               fputaddress((struct sockaddr*)&rspAddrInfo->ai_addr[i], true, stdlog);
               fputs("\n", stdlog);
            }
            LOG_END_FATAL
            rspAddrInfo->ai_addr   = NULL;
            rspAddrInfo->ai_addrs  = 0;
         }
         /* ------------------------------------------- */

        sctpLocalAddressArray = rspAddrInfo->ai_addr;
      }
   }

   /* ====== Get local addresses by gathering interface addresses ======== */
   if(rspAddrInfo->ai_addrs <= 0) {
      /* ====== Get addresses of all local interfaces ==================== */
      localAddresses = gatherLocalAddresses(&localAddressArray);
      if(localAddresses == 0) {
         LOG_ERROR
         fputs("Unable to find out local addresses -> No registration possible\n", stdlog);
         LOG_END
         free(rspAddrInfo);
         return(false);
      }

      /* ====== Get local port number ==================================== */
      socketNameLen = sizeof(socketName);
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
         free(rspAddrInfo);
         return(false);
      }

      /* ====== SCTP: add addresses to rserpool_addrinfo structure ======= */
      if(rserpoolSocket->SocketProtocol == IPPROTO_SCTP) {
         /* SCTP endpoints are described by a list of their
            transport addresses. */
         rspAddrInfo->ai_addrs = localAddresses;
         rspAddrInfo->ai_addr  = pack_sockaddr_union(localAddressArray, localAddresses);
         packedAddressArray    = rspAddrInfo->ai_addr;
      }

      /* ====== TCP/UDP: add own rserpool_addrinfo for each address ====== */
      else {
         rspAddrInfo2 = rspAddrInfo;
         for(i = 0;i < localAddresses;i++) {
            rspAddrInfo2->ai_addrs = 1;
            rspAddrInfo2->ai_addr  = (struct sockaddr*)&localAddressArray[i];
            if(i < localAddresses - 1) {
               next = (struct rserpool_addrinfo*)malloc(sizeof(struct rserpool_addrinfo));
               if(next == NULL) {
                  break;
               }
               *next = *rspAddrInfo2;
               next->ai_next = NULL;
               rspAddrInfo2->ai_next = next;
               rspAddrInfo2          = next;
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
   result = rsp_pe_registration((unsigned char*)&rserpoolSocket->PoolElement->Handle.Handle,
                                rserpoolSocket->PoolElement->Handle.Size,
                                rspAddrInfo, (struct TagItem*)&tags);
   if(result == RSPERR_OKAY) {
      rserpoolSocket->PoolElement->Identifier = rspAddrInfo->ai_pe_id;
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
   if(sctpLocalAddressArray) {
      sctp_freeladdrs(sctpLocalAddressArray);
   }
   if(localAddressArray) {
      free(localAddressArray);
   }
   if(packedAddressArray) {
      free(packedAddressArray);
   }
   while(rspAddrInfo != NULL) {
      next = rspAddrInfo->ai_next;
      free(rspAddrInfo);
      rspAddrInfo = next;
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




/* ###### Register pool element ########################################## */
unsigned int rsp_pe_registration(const unsigned char*      poolHandle,
                                 const size_t              poolHandleSize,
                                 struct rserpool_addrinfo* rserpoolAddrInfo,
                                 struct TagItem*           tags)
{
   struct PoolHandle                myPoolHandle;
   struct ST_CLASS(PoolElementNode) myPoolElementNode;
   struct PoolPolicySettings        myPolicySettings;
   char                             myTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*    myTransportAddressBlock = (struct TransportAddressBlock*)&myTransportAddressBlockBuffer;
   union sockaddr_union*            unpackedAddrs;
   unsigned int                     result;

   if(gAsapInstance) {
      if(rserpoolAddrInfo->ai_pe_id == UNDEFINED_POOL_ELEMENT_IDENTIFIER) {
         rserpoolAddrInfo->ai_pe_id = getPoolElementIdentifier();
      }

      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);

      poolPolicySettingsNew(&myPolicySettings);
      myPolicySettings.PolicyType      = (unsigned int)tagListGetData(tags, TAG_PoolPolicy_Type, PPT_ROUNDROBIN);
      myPolicySettings.Weight          = (unsigned int)tagListGetData(tags, TAG_PoolPolicy_Parameter_Weight, 1);
      myPolicySettings.Load            = (unsigned int)tagListGetData(tags, TAG_PoolPolicy_Parameter_Load, 0);
      myPolicySettings.LoadDegradation = (unsigned int)tagListGetData(tags, TAG_PoolPolicy_Parameter_LoadDegradation, 0);

      unpackedAddrs = unpack_sockaddr(rserpoolAddrInfo->ai_addr, rserpoolAddrInfo->ai_addrs);
      if(unpackedAddrs != NULL) {
         transportAddressBlockNew(myTransportAddressBlock,
                                 rserpoolAddrInfo->ai_protocol,
                                 getPort((struct sockaddr*)rserpoolAddrInfo->ai_addr),
                                 (tagListGetData(tags, TAG_UserTransport_HasControlChannel, 0) != 0) ? TABF_CONTROLCHANNEL : 0,
                                 unpackedAddrs,
                                 rserpoolAddrInfo->ai_addrs);

         ST_CLASS(poolElementNodeNew)(
            &myPoolElementNode,
            rserpoolAddrInfo->ai_pe_id,
            0,
            tagListGetData(tags, TAG_PoolElement_RegistrationLife, 300000000),
            &myPolicySettings,
            myTransportAddressBlock,
            NULL,
            -1, 0);

         LOG_ACTION
         fputs("Trying to register ", stdlog);
         ST_CLASS(poolElementNodePrint)(&myPoolElementNode, stdlog, PENPO_FULL);
         fputs(" to pool ", stdlog);
         poolHandlePrint(&myPoolHandle, stdlog);
         fputs("...\n", stdlog);
         LOG_END

         result = asapInstanceRegister(gAsapInstance, &myPoolHandle, &myPoolElementNode);
         if(result != RSPERR_OKAY) {
            rserpoolAddrInfo->ai_pe_id = UNDEFINED_POOL_ELEMENT_IDENTIFIER;
         }
         free(unpackedAddrs);
      }
      else {
         result = RSPERR_OUT_OF_MEMORY;
      }
   }
   else {
      LOG_ERROR
      fputs("rsplib is not initialized\n", stdlog);
      LOG_END
      result = RSPERR_NOT_INITIALIZED;
   }
   return(result);
}


/* ###### Deregister pool element ######################################## */
unsigned int rsp_pe_deregistration(const unsigned char* poolHandle,
                                   const size_t         poolHandleSize,
                                   const uint32_t       identifier,
                                   struct TagItem*      tags)
{
   struct PoolHandle myPoolHandle;
   unsigned int      result;

   if(gAsapInstance) {
      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);
      result = asapInstanceDeregister(gAsapInstance, &myPoolHandle, identifier);
   }
   else {
      result = RSPERR_NOT_INITIALIZED;
      LOG_ERROR
      fputs("rsplib is not initialized\n", stdlog);
      LOG_END
   }
   return(result);
}


/* ###### Report pool element failure #################################### */
unsigned int rsp_pe_failure(const unsigned char* poolHandle,
                            const size_t         poolHandleSize,
                            const uint32_t       identifier,
                            struct TagItem*      tags)
{
   struct PoolHandle myPoolHandle;
   unsigned int      result;

   if(gAsapInstance) {
      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);
      result = asapInstanceReportFailure(gAsapInstance, &myPoolHandle, identifier);
   }
   else {
      result = RSPERR_NOT_INITIALIZED;
      LOG_ERROR
      fputs("rsplib is not initialized\n", stdlog);
      LOG_END
   }
   return(result);
}




/* ###### Lock mutex ###################################################### */
static void lock(struct Dispatcher* dispatcher, void* userData)
{
   threadSafetyLock(&gThreadSafety);
}


/* ###### Unlock mutex #################################################### */
static void unlock(struct Dispatcher* dispatcher, void* userData)
{
   threadSafetyUnlock(&gThreadSafety);
}


/* ###### rsplib main loop thread ######################################## */
static bool      RsplibThreadStop     = false;
static pthread_t RsplibMainLoopThread = 0;
static void* rsplibMainLoop(void* args)
{
   struct timeval     timeout;
   unsigned long long testTimeStamp;
   fd_set             testfds;
   fd_set             readfds;
   fd_set             writefds;
   fd_set             exceptfds;
   int                result;
   int                n;

   while(!RsplibThreadStop) {
      /* ====== Schedule ================================================= */
      /* pthreads seem to have the property that scheduling is quite
         unfair -> If the main loop only invokes rspSelect(), this
         may block other threads forever => explicitly let other threads
         do their work now, then lock! */
      sched_yield();

      /* ====== Collect data for ext_select() call ======================= */
      timeout.tv_sec  = 0;
      timeout.tv_usec = 100000;
      lock(&gDispatcher, NULL);
      dispatcherGetSelectParameters(&gDispatcher,
                                    &n, &readfds, &writefds, &exceptfds,
                                    &testfds, &testTimeStamp,
                                    &timeout);

      /* ====== Do ext_select() call ===================================== */
      result = ext_select(n + 1, &readfds, &writefds, &exceptfds, &timeout);

      /* ====== Handle results =========================================== */
      dispatcherHandleSelectResult(&gDispatcher, result,
                                   &readfds, &writefds, &exceptfds,
                                   &testfds, testTimeStamp);
      unlock(&gDispatcher, NULL);
   }
   return(NULL);
}


/* ###### Initialize RSerPool API Library ################################ */
int rsp_initialize(struct rsp_info* info, struct TagItem* tags)
{
   static const char* buildDate = __DATE__;
   static const char* buildTime = __TIME__;

   /* ====== Check for problems ========================================== */
   if(gAsapInstance != NULL) {
      LOG_WARNING
      fputs("rsplib is already initialized\n", stdlog);
      LOG_END
      return(0);
   }

   /* ====== Initialize ASAP instance ==================================== */
   threadSafetyInit(&gThreadSafety, "RsplibInstance");
   threadSafetyInit(&gRSerPoolSocketSetMutex, "gRSerPoolSocketSet");
   dispatcherNew(&gDispatcher, lock, unlock, NULL);
   gAsapInstance = asapInstanceNew(&gDispatcher, tags);
   if(gAsapInstance) {
      tagListSetData(tags, TAG_RspLib_GetVersion,   (tagdata_t)RSPLIB_VERSION);
      tagListSetData(tags, TAG_RspLib_GetRevision,  (tagdata_t)RSPLIB_REVISION);
      tagListSetData(tags, TAG_RspLib_GetBuildDate, (tagdata_t)buildDate);
      tagListSetData(tags, TAG_RspLib_GetBuildTime, (tagdata_t)buildTime);
      tagListSetData(tags, TAG_RspLib_IsThreadSafe, (tagdata_t)threadSafetyIsAvailable());

      /* ====== Initialize session storage =============================== */
      leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);

      /* ====== Initialize RSerPool Socket descriptor storage ============ */
      leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);
      leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);
      gRSerPoolSocketAllocationBitmap = identifierBitmapNew(FD_SETSIZE);
      if(gRSerPoolSocketAllocationBitmap != NULL) {
         /* ====== Map stdin, stdout, stderr file descriptors ============ */
         CHECK(rsp_mapsocket(STDOUT_FILENO, STDOUT_FILENO) == STDOUT_FILENO);
         CHECK(rsp_mapsocket(STDIN_FILENO, STDIN_FILENO) == STDIN_FILENO);
         CHECK(rsp_mapsocket(STDERR_FILENO, STDERR_FILENO) == STDERR_FILENO);

         /* ====== Create thread for main loop =========================== */
         RsplibThreadStop = false;
         if(pthread_create(&RsplibMainLoopThread, NULL, &rsplibMainLoop, NULL) == 0) {
            LOG_NOTE
            fputs("rsplib is ready\n", stdlog);
            LOG_END
            return(0);
         }
         else {
            LOG_ERROR
            fputs("Unable to create rsplib main loop thread\n", stdlog);
            LOG_END
         }

         identifierBitmapDelete(gRSerPoolSocketAllocationBitmap);
         gRSerPoolSocketAllocationBitmap = NULL;
      }
      asapInstanceDelete(gAsapInstance);
      gAsapInstance = NULL;
      dispatcherDelete(&gDispatcher);
   }
   return(-1);
}


/* ###### Clean-up RSerPool API Library ################################## */
void rsp_cleanup()
{
   if(gAsapInstance) {
      /* ====== Stopping main loop thread ================================ */
      puts("JOIN...");
      RsplibThreadStop = true;
      pthread_join(RsplibMainLoopThread, NULL);
      puts("JOIN OK");

      /* ====== Clean-up RSerPool Socket descriptor storage ============== */
      CHECK(rsp_unmapsocket(STDOUT_FILENO) == 0);
      CHECK(rsp_unmapsocket(STDIN_FILENO) == 0);
      CHECK(rsp_unmapsocket(STDERR_FILENO) == 0);
      for(int i = 1;i < FD_SETSIZE;i++) {
         if(identifierBitmapAllocateSpecificID(gRSerPoolSocketAllocationBitmap, i) < 0) {
            LOG_WARNING
            fprintf(stdlog, "RSerPool socket %d is still registered -> closing it\n", i);
            LOG_END
            rsp_close(i);
         }
      }
      leafLinkedRedBlackTreeDelete(&gRSerPoolSocketSet);
      identifierBitmapDelete(gRSerPoolSocketAllocationBitmap);
      gRSerPoolSocketAllocationBitmap = NULL;

      /* ====== Clean-up ASAP instance and Dispatcher ==================== */
      asapInstanceDelete(gAsapInstance);
      gAsapInstance = NULL;
      dispatcherDelete(&gDispatcher);
      threadSafetyDestroy(&gRSerPoolSocketSetMutex);
      threadSafetyDestroy(&gThreadSafety);
#ifndef HAVE_KERNEL_SCTP
      /* Finally, give sctplib some time to cleanly shut down associations */
      usleep(250000);
#endif
   }
}


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
      errno = ENOSR;
      return(-1);
   }
   return(rserpoolSocket->Descriptor);
}


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


/* ###### Create RSerPool socket ######################################### */
#define TAG_RspSocket_CustomFD (TAG_USER + 9800)

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
   fd = (int)tagListGetData(tags, TAG_RspSocket_CustomFD, (tagdata_t)-1);
   if(fd < 0) {
      fd = ext_socket(domain, type, protocol);
      printf("CREATE=%d\n",fd);
   } else puts("ACCPETED!!!!");
printf("=> fd=%d    type=%d STREAM=%d\n",fd,type,SOCK_STREAM);
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

   threadSafetyInit(&rserpoolSocket->Mutex, "RSerPoolSocket");
   leafLinkedRedBlackTreeNodeNew(&rserpoolSocket->Node);
   rserpoolSocket->Socket           = fd;
   rserpoolSocket->SocketDomain     = domain;
   rserpoolSocket->SocketType       = type;
   rserpoolSocket->SocketProtocol   = protocol;
   rserpoolSocket->Descriptor       = -1;
   rserpoolSocket->PoolElement      = NULL;
   rserpoolSocket->ConnectedSession = NULL;
   sessionStorageNew(&rserpoolSocket->SessionSet);

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
   threadSafetyLock(&gRSerPoolSocketSetMutex);
   CHECK(leafLinkedRedBlackTreeRemove(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
   identifierBitmapFreeID(gRSerPoolSocketAllocationBitmap, sd);
   rserpoolSocket->Descriptor = -1;
   threadSafetyUnlock(&gRSerPoolSocketSetMutex);

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
   threadSafetyDestroy(&rserpoolSocket->Mutex);
   free(rserpoolSocket);
   return(0);
}


int rsp_forcefailover(int             sd,
                      struct TagItem* tags)
{
   RSerPoolSocket*             rserpoolSocket;
   struct rserpool_addrinfo*   rspAddrInfo;
   struct rserpool_addrinfo*   rspAddrInfo2;
   union sctp_notification     notification;
   struct timeval              timeout;
   ssize_t                     received;
   fd_set                      readfds;
   int                         result;
   int                         flags;
   bool                        success = false;
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
      rsp_pe_failure((unsigned char*)&rserpoolSocket->ConnectedSession->Handle.Handle,
                     rserpoolSocket->ConnectedSession->Handle.Size,
                     rserpoolSocket->ConnectedSession->ConnectedPE, NULL);
      rserpoolSocket->ConnectedSession->ConnectedPE = 0;
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
      result = rsp_getaddrinfo((unsigned char*)&rserpoolSocket->ConnectedSession->Handle.Handle,
                               rserpoolSocket->ConnectedSession->Handle.Size,
                               &rspAddrInfo, NULL);
      if(result == RSPERR_OKAY) {

      // ??????????? WRONG PROTOCOL...

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
               if((received > 0) && (flags & MSG_NOTIFICATION)) {
                  if(notification.sn_header.sn_type == SCTP_ASSOC_CHANGE) {
                     if(notification.sn_assoc_change.sac_state == SCTP_COMM_UP) {
                        LOG_VERBOSE
                        fprintf(stdlog, "Successfully established connection to pool element $%08x (", rspAddrInfo2->ai_pe_id);
                        poolHandlePrint(&rserpoolSocket->ConnectedSession->Handle, stdlog);
                        fprintf(stdlog, "/$%08x on RSerPool socket %u, socket %d, session %u, assoc %u)\n",
                                rspAddrInfo2->ai_pe_id,
                                rserpoolSocket->Descriptor, rserpoolSocket->Socket,
                                rserpoolSocket->ConnectedSession->SessionID,
                                notification.sn_assoc_change.sac_assoc_id);
                        LOG_END
                        success = true;
                        rserpoolSocket->ConnectedSession->IsFailed            = false;
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
                                notification.sn_assoc_change.sac_assoc_id);
                        LOG_END
                        break;
                     }
                  }
               }
               else {
                  LOG_ERROR
                  logerror("Error while trying to wait for COMM_UP notification");
                  LOG_END
                  break;
               }
            }
         }


         /* ====== Free handle resolution result ========================= */
         rsp_freeaddrinfo(rspAddrInfo);

         if(success) {
            if(rserpoolSocket->ConnectedSession->Cookie) {
               sendCookieEcho(rserpoolSocket, rserpoolSocket->ConnectedSession);
            }
/*
            if((!rserpoolSocket->ConnectedSession->IsIncoming) &&
               (rserpoolSocket->ConnectedSession->PoolElement)) {
               sendBusinessCard(rserpoolSocket, rserpoolSocket->ConnectedSession);
            }
*/
         }
         else {
            LOG_ACTION
            fputs("Connection establishment was not possible\n", stdlog);
            LOG_END
         }
      }
      else if(result == RSPERR_NOT_FOUND) {
         LOG_ACTION
         fprintf(stdlog,
                 "Handle resolution did not find new pool element. Waiting %lluus...\n",
                 rserpoolSocket->ConnectedSession->HandleResolutionRetryDelay);
         LOG_END
         usleep((unsigned int)rserpoolSocket->ConnectedSession->HandleResolutionRetryDelay);
      }
      else {
         LOG_WARNING
         fputs("Handle resolution not successful: ", stdlog);
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
   }
   else {
      LOG_WARNING
      fputs("No pool handle for failover\n", stdlog);
      LOG_END
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(success);
}


static bool waitForRead(struct RSerPoolSocket* rserpoolSocket,
                        unsigned long long     timeout)
{
   struct pollfd ufds[1];
   ufds[0].fd     = rserpoolSocket->Descriptor;
   ufds[0].events = POLLIN;
   int result = rsp_poll((struct pollfd*)&ufds, 1, (int)(timeout / 1000));
   if((result > 0) && (ufds[0].revents & POLLIN)) {
      return(true);
   }
   errno = EAGAIN;
   return(false);
}



int rsp_accept(int                sd,
               unsigned long long timeout,
               struct TagItem*    tags)
{
   RSerPoolSocket* rserpoolSocket;
   RSerPoolSocket* newRSerPoolSocket;
   struct Session* session;
   struct TagItem  mytags[16];
   int             result = -1;
   int             newSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   if(waitForRead(rserpoolSocket, timeout)) {
      newSocket = ext_accept(rserpoolSocket->Socket, NULL, 0);
      if(newSocket >= 0) {
         LOG_VERBOSE
         fprintf(stdlog, "Accepted new socket %d on RSerPool socket %u, socket %d\n",
               newSocket, rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
         mytags[0].Tag  = TAG_RspSocket_CustomFD;
         mytags[0].Data = (tagdata_t)newSocket;
         mytags[1].Tag  = TAG_DONE;
         result = rsp_socket(0, SOCK_STREAM, IPPROTO_SCTP, (struct TagItem*)&mytags);
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


int rsp_connect(int                  sd,
                const unsigned char* poolHandle,
                const size_t         poolHandleSize,
                struct TagItem*      tags)
{
   RSerPoolSocket* rserpoolSocket;
   struct Session* session;
   int             result = -1;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   threadSafetyLock(&rserpoolSocket->Mutex);

   /* ====== Check for problems ========================================== */
   if(sessionStorageGetElements(&rserpoolSocket->SessionSet) == 0) {
      /* ====== Create session ============================================== */
      session = addSession(rserpoolSocket, 0, false, poolHandle, poolHandleSize, tags);
      if(session != NULL) {
         rserpoolSocket->ConnectedSession = session;

         /* ====== Connect to a PE ============================================= */
         if(rsp_forcefailover(rserpoolSocket->Descriptor, NULL) >= 0) {
            result = 0;
         }
         else {
            deleteSession(rserpoolSocket, session);
            errno = EIO;
         }

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



/* ###### Register pool element ########################################## */
int rsp_register(int                        sd,
                 const unsigned char*       poolHandle,
                 const size_t               poolHandleSize,
                 const struct rsp_loadinfo* loadinfo,
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
   if(ext_getsockname(rserpoolSocket->Socket, (struct sockaddr*)&socketName, &socketNameLen) < 0) {
      LOG_VERBOSE
      fprintf(stdlog, "RSerPool socket %d, socket %d is not bound -> trying to bind it to the ANY address\n",
              sd, rserpoolSocket->Socket);
      LOG_END
      if(rsp_bind(sd, NULL, 0, NULL) < 0) {
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
      rserpoolSocket->PoolElement->ReregistrationInterval = tagListGetData(tags, TAG_PoolElement_ReregistrationInterval,
                                                               rserpoolSocket->PoolElement->ReregistrationInterval);
      rserpoolSocket->PoolElement->RegistrationLife       = tagListGetData(tags, TAG_PoolElement_RegistrationLife,
                                                               rserpoolSocket->PoolElement->RegistrationLife);

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
      threadSafetyInit(&rserpoolSocket->PoolElement->Mutex, "RspPoolElement");
      poolHandleNew(&rserpoolSocket->PoolElement->Handle, poolHandle, poolHandleSize);
      timerNew(&rserpoolSocket->PoolElement->ReregistrationTimer,
               &gDispatcher,
               reregistrationTimer,
               (void*)rserpoolSocket);

      rserpoolSocket->PoolElement->LoadInfo               = *loadinfo;
      rserpoolSocket->PoolElement->Identifier             = tagListGetData(tags, TAG_PoolElement_Identifier,
                                                               0x00000000);
      rserpoolSocket->PoolElement->ReregistrationInterval = tagListGetData(tags, TAG_PoolElement_ReregistrationInterval,
                                                               60000);
      rserpoolSocket->PoolElement->RegistrationLife       = tagListGetData(tags, TAG_PoolElement_RegistrationLife,
                                                               (rserpoolSocket->PoolElement->ReregistrationInterval * 3) + 3000);
      rserpoolSocket->PoolElement->HasControlChannel      = tagListGetData(tags, TAG_UserTransport_HasControlChannel, false);

      /* ====== Do registration ============================================= */
      if(doRegistration(rserpoolSocket) == false) {
         LOG_ERROR
         fputs("Unable to obtain registration information -> Creating pool element not possible\n", stdlog);
         LOG_END
         deletePoolElement(rserpoolSocket->PoolElement);
         rserpoolSocket->PoolElement = NULL;
         threadSafetyUnlock(&rserpoolSocket->Mutex);
         return(-1);
      }

      /* ====== start reregistration timer ================================== */
      timerStart(&rserpoolSocket->PoolElement->ReregistrationTimer,
                  getMicroTime() + ((unsigned long long)1000 * (unsigned long long)rserpoolSocket->PoolElement->ReregistrationInterval));
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(0);
}


/* ###### Deregister pool element ######################################## */
int rsp_deregister(int sd)
{
   RSerPoolSocket* rserpoolSocket;
   int             result = 0;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   /* ====== Delete pool element ========================================= */
   threadSafetyLock(&rserpoolSocket->Mutex);
   if(rserpoolSocket->PoolElement != NULL) {
      deletePoolElement(rserpoolSocket->PoolElement);
      rserpoolSocket->PoolElement = NULL;
   }
   else {
      result = -1;
      errno  = EBADF;
   }
   threadSafetyUnlock(&rserpoolSocket->Mutex);

   return(result);
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
   threadSafetyLock(&rserpoolSocket->Mutex);

   session = sessionFind(rserpoolSocket, sessionID, 0);
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
                                      (unsigned int)tagListGetData(tags, TAG_RspIO_Flags, MSG_NOSIGNAL),
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


/* ###### Send cookie echo ################################################### */
static bool sendCookieEcho(struct RSerPoolSocket* rserpoolSocket,
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


/* ###### RSerPool socket sendmsg() implementation ####################### */
ssize_t rsp_sendmsg(int                  sd,
                    const void*          data,
                    size_t               dataLength,
                    unsigned int         msg_flags,
                    rserpool_session_t   sessionID,
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
   threadSafetyLock(&rserpoolSocket->Mutex);

   session = sessionFind(rserpoolSocket, sessionID, 0);
   if(session != NULL) {
      if(!session->IsFailed) {
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

            session->PendingNotifications |= SNT_FAILOVER_NECESSARY;
            session->IsFailed              = true;
            result                         = -1;
         }
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "Session %u of RSerPool socket %d, socket %d requires failover\n",
                 session->SessionID,
                 rserpoolSocket->Descriptor, rserpoolSocket->Socket);
         LOG_END
         session->PendingNotifications |= SNT_FAILOVER_NECESSARY;
         result = -1;
         errno = EIO;
      }
   }

   threadSafetyUnlock(&rserpoolSocket->Mutex);
   return(result);
}


#define MSG_RSERPOOL_NOTIFICATION (1 << 0)
#define MSG_RSERPOOL_COOKIE_ECHO  (1 << 1)


static ssize_t getCookieEchoOrNotification(struct RSerPoolSocket* rserpoolSocket,
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
         if(bufferLength < sizeof(rserpool_notification)) {
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
      received = getCookieEchoOrNotification(rserpoolSocket, buffer, bufferLength, msg_flags);
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

      /* ====== Handle notification ====================================== */
      if(flags & MSG_NOTIFICATION) {
         handleNotification(rserpoolSocket,
                            (const char*)rserpoolSocket->MessageBuffer, received);
         received = -1;
      }

      /* ====== Handle ASAP control channel message ====================== */
      else if(rinfo->rinfo_ppid == PPID_ASAP) {
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
         }
         else {
            session = sessionStorageFindSessionByAssocID(&rserpoolSocket->SessionSet, assocID);
            if(session) {
               rinfo->rinfo_session = session->SessionID;
            }
            else {
               LOG_ERROR
               fprintf(stdlog, "Received data on RSerPool socket %d, socket %d via unknown assoc %u\n",
                     rserpoolSocket->Descriptor, rserpoolSocket->Socket, assocID);
               LOG_END
            }
         }

         /* ====== Copy user data ======================================== */
         if((ssize_t)bufferLength < received) {
            LOG_ERROR
            fputs("Buffer is too small to keep full message\n", stdlog);
            LOG_END
            errno = ENOMEM;
            return(-1);
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
         received2 = getCookieEchoOrNotification(rserpoolSocket, buffer, bufferLength, msg_flags);
         if(received2 > 0) {
            received = received2;
         }
      }
   }

   return(received);
}












#define NST_COMM_LOST      1
#define NST_SHUTDOWN_EVENT 2

static void notifySession(struct RSerPoolSocket* rserpoolSocket,
                          sctp_assoc_t           assocID,
                          unsigned int           notificationType)
{
   struct Session*          session;

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
               sendtoplus(rserpoolSocket->Socket, NULL, 0, SCTP_ABORT, NULL, 0, 0,
                          session->AssocID, 0, 0xffffffff, 0);
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


static void handleNotification(struct RSerPoolSocket* rserpoolSocket,
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
                  sendtoplus(rserpoolSocket->Socket, NULL, 0, SCTP_ABORT, NULL, 0, 0,
                           notification->sn_assoc_change.sac_assoc_id, 0, 0xffffffff, 0);
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


static void handleControlChannelMessage(struct RSerPoolSocket* rserpoolSocket,
                                        const sctp_assoc_t     assocID,
                                        char*                  buffer,
                                        size_t                 bufferSize)
{
   struct RSerPoolMessage* message;
   unsigned int            type = 0;
   unsigned int            result;

   struct Session* session = sessionFind(rserpoolSocket, 0, assocID);
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


static bool handleControlChannelAndNotifications(struct RSerPoolSocket* rserpoolSocket)
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

#if 0
static int setFDs(fd_set* fds, int sd, int* notifications)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   int                    n = 0;

   rserpoolSocket = getRSerPoolSocketForDescriptor(sd);
   if(rserpoolSocket != NULL) {
      threadSafetyLock(&rserpoolSocket->Mutex);
      if(rserpoolSocket->Socket >= 0) {
         FD_SET(rserpoolSocket->Socket, fds);
         n = max(n, rserpoolSocket->Socket);
      }

      if(notifications) {
         session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
         while(session != NULL) {
            *notifications |= (session->PendingNotifications & session->EventMask);
            session = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
         }
         *notifications &= SNT_NOTIFICATION_MASK;
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
#endif


/* ###### RSerPool socket poll() implementation ########################## */
int rsp_poll(struct pollfd* ufds, unsigned int nfds, int timeout)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
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
         if(ufds[i].events & POLLIN) {
            session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
            while(session != NULL) {
               if(session->PendingNotifications & session->EventMask) {
                  result++;
                  ufds[i].revents = POLLIN;
               }
               session = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
            }
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
         if(ufds[i].events & POLLIN) {
            session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
            while(session != NULL) {
               if((session->PendingNotifications & session->EventMask) != 0) {
                  ufds[i].revents |= POLLIN;
                  break;
               }
               session = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
            }
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
   if(nfds > FD_SETSIZE) {
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


#if 0
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
   int                    notifications;
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
   notifications = 0;
   if(rserpoolReadFDs) {
      for(i = 0;i < rserpoolN;i++) {
         if(FD_ISSET(i, rserpoolReadFDs)) {
            n = max(n, setFDs(readfds, i, &notifications));
         }
      }
   }
   if(rserpoolWriteFDs) {
      for(i = 0;i < rserpoolN;i++) {
         if(FD_ISSET(i, rserpoolWriteFDs)) {
            n = max(n, setFDs(writefds, i, NULL));
         }
      }
   }
   if(rserpoolExceptFDs) {
      for(i = 0;i < rserpoolN;i++) {
         if(FD_ISSET(i, rserpoolExceptFDs)) {
            n = max(n, setFDs(exceptfds, i, NULL));
         }
      }
   }

   /* ====== Do select() call ============================================= */
   if(notifications == 0) {
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

      /* ====== Handle results of select() call ========================== */
      if(result > 0) {
         result = 0;
         for(i = 0;i < rserpoolN;i++) {
            if( ((rserpoolReadFDs) && FD_ISSET(i, rserpoolReadFDs)) ||
                ((rserpoolWriteFDs) && FD_ISSET(i, rserpoolWriteFDs)) ||
                ((rserpoolExceptFDs) && FD_ISSET(i, rserpoolExceptFDs)) ) {
               rserpoolSocket = getRSerPoolSocketForDescriptor(i);
               if((rserpoolSocket != NULL) && (rserpoolSocket->SessionAllocationBitmap != NULL)) {
                  threadSafetyLock(&rserpoolSocket->Mutex);

                  /* ====== Transfer events ============================== */
                  result +=
                     ((transferFD(rserpoolSocket->Socket, readfds, i, rserpoolReadFDs) +
                       transferFD(rserpoolSocket->Socket, writefds, i, rserpoolWriteFDs) +
                       transferFD(rserpoolSocket->Socket, exceptfds, i, rserpoolExceptFDs)) > 0) ? 1 : 0;

                  /* ======= Check for control channel data ============== */
                  if(FD_ISSET(rserpoolSocket->Socket, readfds)) {
                     LOG_VERBOSE4
                     fprintf(stdlog, "RSerPool socket %d (socket %d) has <read> flag set -> Check, if it has to be handled by rsplib...\n",
                              rserpoolSocket->Descriptor, rserpoolSocket->Socket);
                     LOG_END
                     if(handleControlChannelAndNotifications(rserpoolSocket)) {
                        if(rserpoolReadFDs) {
                           LOG_VERBOSE4
                           fprintf(stdlog, "RSerPool socket %d (socket %d) had <read> event for rsplib only. Clearing <read> flag\n",
                                    rserpoolSocket->Descriptor, rserpoolSocket->Socket);
                           LOG_END
                           FD_CLR(i, rserpoolReadFDs);
                        }
                     }
                  }

                  /* ====== Set <read> flag for RSerPool notifications? == */
                  if(rserpoolReadFDs) {
                     session = sessionStorageGetFirstSession(&rserpoolSocket->SessionSet);
                     while(session != NULL) {
                        if((session->PendingNotifications & session->EventMask) != 0) {
                           FD_SET(i, rserpoolReadFDs);
                           break;
                        }
                        session = sessionStorageGetNextSession(&rserpoolSocket->SessionSet, session);
                     }
                  }

                  threadSafetyUnlock(&rserpoolSocket->Mutex);
               }
            }
         }
       }
   }
   else {
      LOG_VERBOSE
      fputs("There are notifications -> skipping select call\n", stdlog);
      LOG_END
      result = rserpoolN;
   }

   return(result);
}
#endif


/* ###### Set session's CSP status text ################################## */
int rsp_setstatus(int                sd,
                  rserpool_session_t sessionID,
                  const char*        statusText)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct Session*        session;
   int                    result = 0;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   threadSafetyLock(&rserpoolSocket->Mutex);
   session = sessionFind(rserpoolSocket, sessionID, 0);
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
}


/* ###### Get CSP status ################################################# */
size_t getComponentStatus(
          struct ComponentAssociationEntry** caeArray,
          char*                              statusText,
          char*                              componentAddress,
          const int                          registrarSocket,
          const RegistrarIdentifierType      registrarID,
          const int                          registrarSocketProtocol,
          const unsigned long long           registrarConnectionTimeStamp)
{
#if 0
   size_t                            caeArraySize;
   GList*                            list;
   struct SessionDescriptor*         session;
   size_t                            sessions;

   LOG_VERBOSE3
   fputs("Getting Component Status...\n", stdlog);
   LOG_END

   dispatcherLock(&gDispatcher);

   sessions     = g_list_length(gSessionList);
   *caeArray    = componentAssociationEntryArrayNew(1 + sessions);
   caeArraySize = 0;
   if(*caeArray) {
      statusText[0]       = 0x00;
      componentAddress[0] = 0x00;
      if(registrarSocket >= 0) {
         (*caeArray)[caeArraySize].ReceiverID = CID_COMPOUND(CID_GROUP_NAMESERVER, registrarID);
         (*caeArray)[caeArraySize].Duration   = getMicroTime() - registrarConnectionTimeStamp;
         (*caeArray)[caeArraySize].Flags      = 0;
         (*caeArray)[caeArraySize].ProtocolID = registrarSocketProtocol;
         (*caeArray)[caeArraySize].PPID       = PPID_ASAP;
         caeArraySize++;
      }
      componentStatusGetComponentAddress(componentAddress, -1, 0);

      list = g_list_first(gSessionList);
      while(list != NULL) {
         session = (struct SessionDescriptor*)list->data;
         if(!session->Incoming) {
            if(session->Socket >= 0) {
               (*caeArray)[caeArraySize].ReceiverID = CID_COMPOUND(CID_GROUP_POOLELEMENT, session->Identifier);
               (*caeArray)[caeArraySize].Duration   = (session->ConnectionTimeStamp > 0) ? (getMicroTime() - session->ConnectionTimeStamp) : ~0ULL;
               (*caeArray)[caeArraySize].Flags      = 0;
               (*caeArray)[caeArraySize].ProtocolID = session->SocketProtocol;
               (*caeArray)[caeArraySize].PPID       = 0;
               caeArraySize++;
               componentStatusGetComponentAddress(componentAddress, session->Socket, 0);
            }
            if(session->StatusTextText[0] != 0x00) {
               safestrcpy(statusText,
                          session->StatusTextText,
                          CSPH_STATUS_TEXT_SIZE);
            }
         }

         list = g_list_next(list);
      }

      if((statusText[0] == 0x00) || (sessions != 1)) {
         snprintf(statusText, CSPH_STATUS_TEXT_SIZE,
                  "%u Session%s", (unsigned int)sessions, (sessions == 1) ? "" : "s");
      }
   }

   dispatcherUnlock(&gDispatcher);

   return(caeArraySize);
#endif
return(0);
}



#include "breakdetector.h"

#define PPID_PPP 0x29097602

#define PPPT_PING 0x01
#define PPPT_PONG 0x02

struct PingPongCommonHeader
{
   uint8_t  Type;
   uint8_t  Flags;
   uint16_t Length;
};

struct Ping
{
   struct PingPongCommonHeader Header;
   uint64_t                    MessageNo;
   char                        Data[];
};

struct Pong
{
   struct PingPongCommonHeader Header;
   uint64_t                    MessageNo;
   uint64_t                    ReplyNo;
   char                        Data[];
};





#include "t10.cc"

int main(int argc, char** argv)
{
//   TagItem tags[16];
   int sd;
//    fd_set readfds;
   int n;
   struct rsp_loadinfo loadinfo;
   struct rsp_info info;
   struct rsp_sndrcvinfo rinfo;
   bool   server = true;
   bool   thread = false;
   bool   echo   = false;
   bool   fractal = false;

   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-log" ,4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
      else if(!(strcmp(argv[n], "-server"))) {
         server = true;
      }
      else if(!(strcmp(argv[n], "-client"))) {
         server = false;
      }
      else if(!(strcmp(argv[n], "-thread"))) {
         thread = true;
      }
      else if(!(strcmp(argv[n], "-echo"))) {
         echo = true;
      }
      else if(!(strcmp(argv[n], "-fractal"))) {
         fractal = true;
      }
   }
   beginLogging();

   memset(&info, 0, sizeof(info));
   rsp_initialize(&info, NULL);

//    if(pthread_create(&rsplibThread, NULL, &rsplibMainLoop, NULL) != 0) {
//       puts("ERROR: Unable to create rsplib main loop thread!");
//    }


   if(echo) {
      EchoServer echoServer;
      echoServer.poolElement("Echo Server - Version 1.0",
                             "EchoPool", NULL);
      return 0;
   }
   if(fractal) {
      FractalGeneratorServer::FractalGeneratorServerSettings settings;
      settings.TestMode     = false;
      settings.FailureAfter = 20;
      ThreadedServer::poolElement("Fractal Generator Server - Version 1.0",
                                  "FractalGeneratorPool", NULL,
                                  (void*)&settings,
                                  FractalGeneratorServer::fractalGeneratorServerFactory);
      return 0;
   }
   if(!server) {
      sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP, NULL);
      CHECK(sd > 0);
       puts("=========== CLIENT =============");

       int r = rsp_connect(sd, (const unsigned char*)"PingPongPool", 12, NULL);
       if(r < 0) {
          perror("connect error");
          exit(1);
       }


      uint64_t messageNo   = 1;
      uint64_t lastReplyNo = 0;
      unsigned long long s = getMicroTime();
      installBreakDetector();
      while(!breakDetected()) {
         struct pollfd ufds[1];
         ufds[0].fd     = sd;
         ufds[0].events = POLLIN;
         int result = rsp_poll((struct pollfd*)&ufds, 1, 1000);

         if(result > 0) {
            if(ufds[0].revents & POLLIN) {
               printf("READ EVENT FOR %d\n",sd);
               char buffer[65536];
               int  msg_flags = 0;
               ssize_t rv = rsp_recvmsg(sd, (char*)&buffer, sizeof(buffer),
                                        &rinfo, &msg_flags, 0);
               printf("client.rv=%d\n",rv);
               if(rv > 0) {
                  if(!(msg_flags & MSG_RSERPOOL_NOTIFICATION)) {
                     if(rv >= (ssize_t)sizeof(PingPongCommonHeader)) {
                        Pong* pong = (Pong*)&buffer;
                        if(pong->Header.Type == PPPT_PONG) {
                           uint64_t recvMessageNo = ntoh64(pong->MessageNo);
                           uint64_t recvReplyNo   = ntoh64(pong->ReplyNo);
                           printf("Ack: %Ld\n", recvMessageNo);
                           if(recvReplyNo - lastReplyNo != 1) {
                              printf("NUMBER GAP: %Ld\n", (int64)recvReplyNo - (int64)lastReplyNo);
                           }
                           lastReplyNo = recvReplyNo;
                        }
                     } else puts("PPP Message too short!");
                  }
                  else {
                     printf("NOTIFICATION: ");
                     rsp_print_notification((union rserpool_notification*)&buffer, stdout);
                     puts("");
                     union rserpool_notification* n = (union rserpool_notification*)&buffer;
                     if((n->rn_failover.rf_type == RSERPOOL_FAILOVER) &&
                        (n->rn_failover.rf_state == RSERPOOL_FAILOVER_NECESSARY)) {
                        puts("###################### FO NECESSARY...");
                        rsp_forcefailover(sd, NULL);
                        puts("######################################");
                     }
                  }
               }
            }
         }

         if(getMicroTime() - s >= 500000) {
            s = getMicroTime();
            Ping ping;
            ping.Header.Type   = PPPT_PING;
            ping.Header.Flags  = 0x00;
            ping.Header.Length = htons(sizeof(ping));
            ping.MessageNo     = hton64(messageNo);
            int msg_flags = 0;
            ssize_t snd = rsp_sendmsg(sd, (char*)&ping, sizeof(ping), msg_flags,
                                      0,
                                      PPID_PPP,
                                      0,
                                      ~0, 0, NULL);
            printf("snd=%d\n",snd);
            if(snd > 0) {
               printf("Out: %Ld\n", messageNo);
               messageNo++;
            }
         }
      }

   }
   else {
      if(!thread) {
         sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP, NULL);
         CHECK(sd > 0);
         puts("=========== SERVER =============");
         memset(&loadinfo, 0, sizeof(loadinfo));
         loadinfo.rli_policy = PPT_ROUNDROBIN;
         rsp_register(sd, (const unsigned char*)"PingPongPool", 12, &loadinfo, NULL);

         uint64_t replyNo = 1;

         installBreakDetector();
         while(!breakDetected()) {
            struct pollfd ufds[1];
            ufds[0].fd     = sd;
            ufds[0].events = POLLIN;
            int result = rsp_poll((struct pollfd*)&ufds, 1, 1000);

            if(result > 0) {
               if(ufds[0].revents & POLLIN) {
                  puts("READ EVENT!");

                  char buffer[1024];
                  int  msg_flags = 0;
                  ssize_t rv = rsp_recvmsg(sd, (char*)&buffer, sizeof(buffer),
                                          &rinfo, &msg_flags, 0);
   printf("rv=%d   via s%u\n",rv,rinfo.rinfo_session);

                  if(rv > 0) {
                     if(msg_flags & MSG_RSERPOOL_NOTIFICATION) {
                        printf("NOTIFICATION: ");
                        rsp_print_notification((union rserpool_notification*)&buffer, stdout);
                        puts("");
                     }
                     else if(msg_flags & MSG_RSERPOOL_COOKIE_ECHO) {
                        puts("$$$$$$$$$$$$ COOKIE_ECHO $$$$$$$$$$$$$$");
                     }
                     else {
                        if(rv >= (ssize_t)sizeof(PingPongCommonHeader)) {
                           Ping* ping = (Ping*)&buffer;
                           if(ping->Header.Type == PPPT_PING) {
                              if(ntohs(ping->Header.Type) >= (ssize_t)sizeof(struct Ping)) {
                                 Pong pong;
                                 pong.Header.Type   = PPPT_PONG;
                                 pong.Header.Flags  = 0x00;
                                 pong.Header.Length = htons(sizeof(pong));
                                 pong.MessageNo     = ping->MessageNo;
                                 pong.ReplyNo       = hton64(replyNo);
                                 int msg_flags = 0;
                                 ssize_t snd = rsp_sendmsg(sd, (char*)&pong, sizeof(pong), msg_flags,
                                                         rinfo.rinfo_session,
                                                         PPID_PPP,
                                                         0,
                                                         ~0, 0, NULL);
                                 printf("snd=%d\n",snd);
                                 if(snd > 0) {
                                    replyNo++;
                                 }
                                 ssize_t sc = rsp_send_cookie(sd, (unsigned char*)&pong, sizeof(pong),
                                                            rinfo.rinfo_session, 0, NULL);
                                 printf("sc=%d\n",sc);
                              }
                           }
                        } else puts("PPP Message too short!");
                     }
                  }
               }
            }
         }
         puts("DEREG...");
         rsp_deregister(sd);
      }
      else {
         ThreadedServer::poolElement("Ping Pong Server - Version 1.0",
                                     "PingPongPool", NULL,
                                     NULL,
                                     PingPongServer::pingPongServerFactory);
         return 0;
      }
   }

puts("CLOSE...");
   rsp_close(sd);
/*puts("JOIN");
   RsplibThreadStop = true;
   pthread_join(rsplibThread, NULL);*/
puts("CLEANUP...");
   rsp_cleanup();
   finishLogging();
}
