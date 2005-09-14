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


typedef unsigned int rserpool_session_t;

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
static bool poolElementDoRegistration(struct PoolElement* poolElement);
static void poolElementDelete(struct PoolElement* poolElement);
// ????????????


/* ###### Configure notifications of SCTP socket ############################# */
static bool configureSCTPSocket(int sd, sctp_assoc_t assocID, struct TagItem* tags)
{
   struct sctp_event_subscribe events;

   memset((char*)&events, 0 ,sizeof(events));
   events.sctp_data_io_event = 1;
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
struct LeafLinkedRedBlackTree gSessionSet;
unsigned char                 gRSerPoolSocketAllocationBitmap[FD_SETSIZE / 8];


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

#define GET_RSERPOOL_SESSION(session, rserpoolSocket, sessionID, poolHandle, poolHandleSize) \
   session = sessionFind(rserpoolSocket, sessionID, poolHandle, poolHandleSize); \
   if(session != NULL) { \
      return(-1); \
   }


struct PoolElement
{
   struct PoolHandle                 Handle;
   uint32_t                          Identifier;
   struct ThreadSafety               Mutex;

   int                               SocketDomain;
   int                               SocketType;
   int                               SocketProtocol;
   int                               Socket;

   unsigned int                      PolicyType;
   uint32_t                          PolicyParameterWeight;
   uint32_t                          PolicyParameterLoad;

   struct Timer                      ReregistrationTimer;
   unsigned int                      RegistrationLife;
   unsigned int                      ReregistrationInterval;

   bool                              HasControlChannel;

//   struct LeafLinkedRedBlackTree     SessionSet; //  ??????? WIRD DAS BENÖTIGT? alle Sessions -> RSerPoolSocket
   /* ????
   PoolElement -> Registrierung, etc.
   Socket -> Session-Verwaltung
   */
};


struct Session
{
   struct LeafLinkedRedBlackTreeNode SocketNode;
   struct LeafLinkedRedBlackTreeNode GlobalNode;

   rserpool_session_t                Descriptor;

   struct PoolHandle                 Handle;
   uint32_t                          Identifier;
   struct ThreadSafety               Mutex;

   int                               SocketDomain;
   int                               SocketType;
   int                               SocketProtocol;
   int                               Socket;

   bool                              IsIncoming;
//   struct PoolElement*               PoolElement; ???

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


// static struct Session* getSessionFromPoolElementNode(void* node)
// {
//    return((struct Session*)(long)node - ((long)&((struct Session*)node)->PoolElementNode - (long)node));
// }


static struct Session* getSessionFromSocketNode(void* node)
{
   return((struct Session*)(long)node - ((long)&((struct Session*)node)->SocketNode - (long)node));
}


static struct Session* getSessionFromGlobalNode(void* node)
{
   return((struct Session*)(long)node - ((long)&((struct Session*)node)->GlobalNode - (long)node));
}


static void sessionGlobalPrint(const void* node, FILE* fd)
{
   const struct Session* session = (const struct Session*)getSessionFromGlobalNode((void*)node);

   poolHandlePrint(&session->Handle, fd);
   fprintf(fd, "/%08x ", session->Identifier);
}

static int sessionGlobalComparison(const void* node1, const void* node2)
{
   const struct Session* session1 = (const struct Session*)getSessionFromGlobalNode((void*)node1);
   const struct Session* session2 = (const struct Session*)getSessionFromGlobalNode((void*)node2);

   if(session1->Identifier < session2->Identifier) {
      return(-1);
   }
   else if(session1->Identifier > session2->Identifier) {
      return(1);
   }
   return(poolHandleComparison(&session1->Handle, &session2->Handle));
}

// static void sessionPoolElementPrint(const void* node, FILE* fd)
// {
//    const struct Session* session = (const struct Session*)getSessionFromPoolElementNode((void*)node);
//
//    poolHandlePrint(&session->Handle, fd);
//    fprintf(fd, "/%08x ", session->Identifier);
// }
//
// static int sessionPoolElementComparison(const void* node1, const void* node2)
// {
//    const struct Session* session1 = (const struct Session*)getSessionFromPoolElementNode((void*)node1);
//    const struct Session* session2 = (const struct Session*)getSessionFromPoolElementNode((void*)node2);
//
//    if(session1->Identifier < session2->Identifier) {
//       return(-1);
//    }
//    else if(session1->Identifier > session2->Identifier) {
//       return(1);
//    }
//    return(poolHandleComparison(&session1->Handle, &session2->Handle));
// }


/* ###### Create new session ############################################# */
static struct Session* sessionNew(const int            socketDomain,
                                  const int            socketType,
                                  const int            socketProtocol,
                                  const int            socketDescriptor,
                                  const bool           isIncoming,
                                  struct PoolElement*  poolElement,
                                  const unsigned char* poolHandle,
                                  const size_t         poolHandleSize,
                                  struct TagItem*      tags)
{
   struct Session* session = (struct Session*)malloc(sizeof(struct Session));
   if(session != NULL) {
      threadSafetyInit(&session->Mutex, "Session");

      session->Tags = tagListDuplicate(tags);
      if(session->Tags == NULL) {
         free(session);
         return(NULL);
      }

      if(poolHandleSize > 0) {
         if(poolHandleSize > MAX_POOLHANDLESIZE) {
            LOG_ERROR
            fputs("Pool handle too long\n", stdlog);
            LOG_END_FATAL
         }
         poolHandleNew(&session->Handle, poolHandle, poolHandleSize);
      }
      else {
         session->Handle.Size = 0;
      }

      leafLinkedRedBlackTreeNodeNew(&session->GlobalNode);
      leafLinkedRedBlackTreeNodeNew(&session->SocketNode);
//      leafLinkedRedBlackTreeNodeNew(&session->PoolElementNode);
      session->Descriptor                 = 0;  // ???????????
      session->SocketDomain               = socketDomain;
      session->SocketType                 = socketType;
      session->SocketProtocol             = socketProtocol;
      session->Socket                     = socketDescriptor;
//      session->PoolElement                = poolElement;
      session->IsIncoming                 = isIncoming;
      session->Cookie                     = NULL;
      session->CookieSize                 = 0;
      session->CookieEcho                 = NULL;
      session->CookieEchoSize             = 0;
      session->StatusText[0]              = 0x00;
      session->ConnectionTimeStamp        = 0;
      session->ConnectTimeout             = (unsigned long long)tagListGetData(tags, TAG_RspSession_ConnectTimeout, 5000000);
      session->HandleResolutionRetryDelay = (unsigned long long)tagListGetData(tags, TAG_RspSession_HandleResolutionRetryDelay, 250000);
//    ????   if(session->PoolElement != NULL) {
//          threadSafetyLock(&session->PoolElement->Mutex);
//          CHECK(leafLinkedRedBlackTreeInsert(&session->PoolElement->SessionSet, &session->PoolElementNode) ==
//                   &session->PoolElementNode);
//          threadSafetyUnlock(&session->PoolElement->Mutex);
//       }

      dispatcherLock(&gDispatcher);
      CHECK(leafLinkedRedBlackTreeInsert(&gSessionSet, &session->GlobalNode) == &session->GlobalNode);
      dispatcherUnlock(&gDispatcher);
   }
   return(session);
}


/* ###### Delete session ################################################# */
static void sessionDelete(struct Session* session)
{
   if(session) {
      dispatcherLock(&gDispatcher);
      CHECK(leafLinkedRedBlackTreeRemove(&gSessionSet, &session->GlobalNode) == &session->GlobalNode);
      dispatcherUnlock(&gDispatcher);
//       if(session->PoolElement) {
/*         threadSafetyLock(&session->PoolElement->Mutex);
         CHECK(leafLinkedRedBlackTreeRemove(&session->PoolElement->SessionSet, &session->PoolElementNode) ==
                  &session->PoolElementNode);
         threadSafetyUnlock(&session->PoolElement->Mutex); ???????*/
//          session->PoolElement = NULL;
//       }
      if(session->Socket >= 0) {
         ext_close(session->Socket);
         session->Socket = -1;
      }
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
      threadSafetyDestroy(&session->Mutex);
      free(session);
   }
}





/* ###### Reregistration timer ########################################### */
static void reregistrationTimer(struct Dispatcher* dispatcher,
                                struct Timer*      timer,
                                void*              userData)
{
   struct PoolElement* poolElement = (struct PoolElement*)userData;

   LOG_VERBOSE3
   fputs("Starting reregistration\n", stdlog);
   LOG_END
   threadSafetyLock(&poolElement->Mutex);

   poolElementDoRegistration(poolElement);

   timerStart(&poolElement->ReregistrationTimer,
              getMicroTime() + ((unsigned long long)1000 * (unsigned long long)poolElement->ReregistrationInterval));
   threadSafetyUnlock(&poolElement->Mutex);
   LOG_VERBOSE3
   fputs("Reregistration completed\n", stdlog);
   LOG_END
}


/* ###### Create pool element ############################################ */
static struct PoolElement* poolElementNew(const unsigned char* poolHandle,
                                          const size_t         poolHandleSize,
                                          struct TagItem*      tags)
{
   union sockaddr_union localAddress;

   struct PoolElement* poolElement = (struct PoolElement*)malloc(sizeof(struct PoolElement));
   if(poolElement != NULL) {
      /* ====== Initialize pool element ================================== */
      CHECK(poolHandleSize <= MAX_POOLHANDLESIZE);
      poolHandleNew(&poolElement->Handle, poolHandle, poolHandleSize);

      threadSafetyInit(&poolElement->Mutex, "RspPoolElement");
      timerNew(&poolElement->ReregistrationTimer,
               &gDispatcher,
               reregistrationTimer,
               (void*)poolElement);
/* ???     leafLinkedRedBlackTreeNew(&poolElement->SessionSet,
                                sessionPoolElementPrint,
                                sessionPoolElementComparison);*/
      poolElement->Socket                 = -1;
      poolElement->Identifier             = tagListGetData(tags, TAG_PoolElement_Identifier,
                                               0x00000000);
      poolElement->SocketDomain           = tagListGetData(tags, TAG_PoolElement_SocketDomain,
                                               checkIPv6() ? AF_INET6 : AF_INET);
      poolElement->SocketType             = tagListGetData(tags, TAG_PoolElement_SocketType,
                                               SOCK_STREAM);
      poolElement->SocketProtocol         = tagListGetData(tags, TAG_PoolElement_SocketProtocol,
                                               IPPROTO_SCTP);
      poolElement->ReregistrationInterval = tagListGetData(tags, TAG_PoolElement_ReregistrationInterval,
                                               5000);
      poolElement->RegistrationLife       = tagListGetData(tags, TAG_PoolElement_RegistrationLife,
                                               (poolElement->ReregistrationInterval * 3) + 3000);
      poolElement->PolicyType             = tagListGetData(tags, TAG_PoolPolicy_Type,
                                               PPT_ROUNDROBIN);
      poolElement->PolicyParameterWeight  = tagListGetData(tags, TAG_PoolPolicy_Parameter_Weight,
                                               1);
      poolElement->PolicyParameterLoad    = tagListGetData(tags, TAG_PoolPolicy_Parameter_Load,
                                               0);
      poolElement->HasControlChannel      = tagListGetData(tags, TAG_UserTransport_HasControlChannel, false);

      /* ====== Create socket ============================================ */
      poolElement->Socket = ext_socket(poolElement->SocketDomain,
                               poolElement->SocketType,
                               poolElement->SocketProtocol);
      if(poolElement->Socket <= 0) {
         LOG_ERROR
         logerror("Unable to create socket for new pool element");
         LOG_END
         poolElementDelete(poolElement);
         return(NULL);
      }

      /* ====== Configure SCTP socket ==================================== */
      if(poolElement->SocketProtocol == IPPROTO_SCTP) {
         if(!configureSCTPSocket(poolElement->Socket, 0, tags)) {
            LOG_ERROR
            fprintf(stdlog, "Failed to configure SCTP socket FD %d\n", poolElement->Socket);
            LOG_END
            poolElementDelete(poolElement);
            return(NULL);
         }
      }

      /* ====== Bind socket ============================================== */
      memset((void*)&localAddress, 0, sizeof(localAddress));
      ((struct sockaddr*)&localAddress)->sa_family = poolElement->SocketDomain;
      setPort((struct sockaddr*)&localAddress, tagListGetData(tags, TAG_PoolElement_LocalPort, 0));

      if(bindplus(poolElement->Socket, (union sockaddr_union*)&localAddress, 1) == false) {
         LOG_ERROR
         logerror("Unable to bind socket for new pool element");
         LOG_END
         poolElementDelete(poolElement);
         return(NULL);
      }
      if((poolElement->SocketType == SOCK_STREAM) && (ext_listen(poolElement->Socket, 5) < 0)) {
         LOG_ERROR
         logerror("Unable to set socket for new pool element to listen mode");
         LOG_END
      }


      /* ====== Do registration ========================================== */
      if(poolElementDoRegistration(poolElement) == false) {
         LOG_ERROR
         fputs("Unable to obtain registration information -> Creating pool element not possible\n", stdlog);
         LOG_END
         poolElementDelete(poolElement);
         return(NULL);
      }

      /* Okay -> start reregistration timer */
      timerStart(&poolElement->ReregistrationTimer,
                 getMicroTime() + ((unsigned long long)1000 * (unsigned long long)poolElement->ReregistrationInterval));
   }

   return(poolElement);
}


/* ###### Delete pool element ############################################ */
static void poolElementDelete(struct PoolElement* poolElement)
{
   int result;

/* ???  CHECK(leafLinkedRedBlackTreeIsEmpty(&poolElement->SessionSet));*/
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
   if(poolElement->Socket >= 0) {
      ext_close(poolElement->Socket);
      poolElement->Socket = -1;
   }
   threadSafetyDestroy(&poolElement->Mutex);
   free(poolElement);
}




/* ###### Reregistration ##################################################### */
static bool poolElementDoRegistration(struct PoolElement* poolElement)
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

   /* ====== Create EndpointAddressInfo structure =========================== */
   eai = (struct EndpointAddressInfo*)malloc(sizeof(struct EndpointAddressInfo));
   if(eai == NULL) {
      LOG_ERROR
      fputs("Out of memory\n", stdlog);
      LOG_END
      return(false);
   }
   threadSafetyLock(&poolElement->Mutex);

   eai->ai_family   = poolElement->SocketDomain;
   eai->ai_socktype = poolElement->SocketType;
   eai->ai_protocol = poolElement->SocketProtocol;
   eai->ai_next     = NULL;
   eai->ai_addr     = NULL;
   eai->ai_addrs    = 0;
   eai->ai_addrlen  = sizeof(union sockaddr_union);
   eai->ai_pe_id    = poolElement->Identifier;

   /* ====== Get local addresses for SCTP socket ========================= */
   if(poolElement->SocketProtocol == IPPROTO_SCTP) {
      eai->ai_addrs = getladdrsplus(poolElement->Socket, 0, &eai->ai_addr);
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
      if(ext_getsockname(poolElement->Socket, (struct sockaddr*)&socketName, &socketNameLen) == 0) {
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
      if(poolElement->SocketProtocol == IPPROTO_SCTP) {
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
   tags[0].Data = poolElement->PolicyType;
   tags[1].Tag  = TAG_PoolPolicy_Parameter_Load;
   tags[1].Data = poolElement->PolicyParameterLoad;
   tags[2].Tag  = TAG_PoolPolicy_Parameter_Weight;
   tags[2].Data = poolElement->PolicyParameterWeight;
   tags[3].Tag  = TAG_UserTransport_HasControlChannel;
   tags[3].Data = (tagdata_t)poolElement->HasControlChannel;
   tags[4].Tag  = TAG_END;

   /* ====== Do registration ================================================ */
   result = rspRegister((unsigned char*)&poolElement->Handle.Handle,
                        poolElement->Handle.Size,
                        eai, (struct TagItem*)&tags);
   if(result == RSPERR_OKAY) {
      poolElement->Identifier = eai->ai_pe_id;
      LOG_ACTION
      fprintf(stdlog, "(Re-)Registration successful, ID is $%08x\n", poolElement->Identifier);
      LOG_END

      printf("(Re-)Registration successful, ID is $%08x\n", poolElement->Identifier);
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "(Re-)Registration failed: ");
      rserpoolErrorPrint(result, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }


   /* ====== Clean up ======================================================= */
   threadSafetyUnlock(&poolElement->Mutex);
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

















struct RSerPoolSocket
{
   struct LeafLinkedRedBlackTreeNode Node;

   int                               Descriptor;
   int                               Domain;
   int                               Type;
   int                               Protocol;

   struct PoolElement*               PoolElement;        /* PE mode                      */
   struct LeafLinkedRedBlackTree     SessionSet;         /* UDP-like PU mode and PE mode */
   struct Session*                   ConnectedSession;   /* TCP-like PU mode             */
};



static void rserpoolSocketPrint(const void* node, FILE* fd)
{
   const struct RSerPoolSocket* socket = (const struct RSerPoolSocket*)node;

   fprintf(fd, "%d ", socket->Descriptor);
}


static int rserpoolSocketComparison(const void* node1, const void* node2)
{
   const struct RSerPoolSocket* socket1 = (const struct RSerPoolSocket*)node1;
   const struct RSerPoolSocket* socket2 = (const struct RSerPoolSocket*)node2;

   if(socket1->Descriptor < socket2->Descriptor) {
      return(-1);
   }
   else if(socket1->Descriptor > socket2->Descriptor) {
      return(1);
   }
   return(0);
}


static struct Session* sessionFind(struct RSerPoolSocket* socket,
                                   rserpool_session_t     sessionID,
                                   const unsigned char*   poolHandle,
                                   const size_t           poolHandleSize)
{
   struct Session                     cmpNode;
   struct LeafLinkedRedBlackTreeNode* found;

   if(socket->ConnectedSession) {
      if(sessionID != 0) {
         LOG_WARNING
         fputs("Session ID given for connected RSerPool socket (there is only one session)\n", stdlog);
         LOG_END
      }
      if(poolHandle != NULL) {
         LOG_WARNING
         fputs("Pool handle given for connected RSerPool socket (there is only one session)\n", stdlog);
         LOG_END
      }
      return(socket->ConnectedSession);
   }
   else if(sessionID != 0) {
      cmpNode.Descriptor = sessionID;
      found = leafLinkedRedBlackTreeFind(&socket->SessionSet, &cmpNode.SocketNode);
      if(found) {
         return(getSessionFromSocketNode(found));
      }
      LOG_WARNING
      fprintf(stdlog, "There is no session %u on RSerPool socket %d\n",
              sessionID, socket->Descriptor);
      LOG_END
      errno = EINVAL;
   }
   else {
   puts("???? STOP!!!");
      exit(1);
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









/* ###### Initialize RSerPool API Library ################################ */
unsigned int rsp_initialize()
{
   size_t i;

   rspInitialize(NULL);

   /* ====== Initialize session storage ================================== */
   leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, sessionGlobalPrint, sessionGlobalComparison);

   /* ====== Initialize RSerPool Socket descriptor storage =============== */
   leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);
   for(i = 1;i < (FD_SETSIZE / 8);i++) {
      gRSerPoolSocketAllocationBitmap[i] = 0x00;
   }
   gRSerPoolSocketAllocationBitmap[0] = 0x01;

   return(0);
}


/* ###### Clean-up RSerPool API Library ################################## */
unsigned int rsp_cleanup()
{
   size_t i, j;

   leafLinkedRedBlackTreeDelete(&gRSerPoolSocketSet);


   /* ====== Clean-up RSerPool Socket descriptor storage ================= */
   leafLinkedRedBlackTreeDelete(&gRSerPoolSocketSet);
   gRSerPoolSocketAllocationBitmap[0] &= ~0x01;
   for(i = 0;i < (FD_SETSIZE / 8);i++) {
      for(j = 0;j < 8;j++) {
         if(gRSerPoolSocketAllocationBitmap[i] & (1 << j)) {
            LOG_ERROR
            fprintf(stdlog, "RSerPool socket %d is still registered\n", (i * 8) + j);
            LOG_END
         }
      }
   }

   rspCleanUp();

   return(0);
}


/* ###### Create RSerPool socket ######################################### */
int rsp_socket(int domain, int type, int protocol)
{
   struct RSerPoolSocket* rserpoolSocket;
   size_t                 i, j;

   /* ====== Initialize RSerPool socket entry ============================ */
   rserpoolSocket = (struct RSerPoolSocket*)malloc(sizeof(struct RSerPoolSocket));
   if(rserpoolSocket == NULL) {
      errno = ENOMEM;
      return(-1);
   }

   leafLinkedRedBlackTreeNodeNew(&rserpoolSocket->Node);
   rserpoolSocket->Domain           = domain;
   rserpoolSocket->Type             = type;
   rserpoolSocket->Protocol         = protocol;
   rserpoolSocket->Descriptor       = -1;
   rserpoolSocket->PoolElement      = NULL;
   rserpoolSocket->ConnectedSession = NULL;
   leafLinkedRedBlackTreeNew(&rserpoolSocket->SessionSet, sessionGlobalPrint, sessionGlobalComparison);


   /* ====== Find available RSerPool socket descriptor =================== */
   // ????? dispatcherLock(&gDispatcher);
   for(i = 0;i < (FD_SETSIZE / 8);i++) {
      for(j = 0;j < 8;j++) {
         if(!(gRSerPoolSocketAllocationBitmap[i] & (1 << j))) {
            rserpoolSocket->Descriptor = (8* i) + j;

            /* ====== Add RSerPool socket entry ========================== */
            CHECK(leafLinkedRedBlackTreeInsert(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
            gRSerPoolSocketAllocationBitmap[rserpoolSocket->Descriptor / 8] |= (1 << (rserpoolSocket->Descriptor % 8));

            i = FD_SETSIZE;
            j = 8;
         }
      }
   }
   // ????? dispatcherUnlock(&gDispatcher);

   /* ====== Has there been a problem? =================================== */
   if(rserpoolSocket->Descriptor < 0) {
      free(rserpoolSocket);
      errno = ENOSR;
      return(-1);
   }
   return(rserpoolSocket->Descriptor);
}


/* ###### Delete RSerPool socket ######################################### */
int rsp_close(int sd)
{
   RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd)

   if(rserpoolSocket->PoolElement) {
      rsp_deregister(sd);
   }
   if(!leafLinkedRedBlackTreeIsEmpty(&rserpoolSocket->SessionSet)) {
      LOG_ERROR
      fprintf(stdlog, "RSerPool socket %u still has sessions -> fix your program!", sd);
      LOG_END_FATAL
   }

   /* ====== Delete RSerPool socket entry ================================ */
   // ????? dispatcherLock(&gDispatcher);
   CHECK(leafLinkedRedBlackTreeRemove(&gRSerPoolSocketSet, &rserpoolSocket->Node) == &rserpoolSocket->Node);
   gRSerPoolSocketAllocationBitmap[sd / 8] &= ~(1 << (sd % 8));
   // ????? dispatcherUnlock(&gDispatcher);

   leafLinkedRedBlackTreeDelete(&rserpoolSocket->SessionSet);
   rserpoolSocket->Descriptor = -1;
   free(rserpoolSocket);
   return(0);
}


/* ###### Register pool element ########################################## */
int rsp_register(int                  sd,
                 const unsigned char* poolHandle,
                 const size_t         poolHandleSize,
                 struct TagItem*      tags)
{
   RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   /* ====== Check for problems ========================================== */
   if(rserpoolSocket->PoolElement != NULL) {
      LOG_ERROR
      fprintf(stdlog, "RSerPool socket %d already is a pool element\n", sd);
      LOG_END
      errno = EBADF;
      return(-1);
   }
   if(!leafLinkedRedBlackTreeIsEmpty(&rserpoolSocket->SessionSet)) {
      LOG_ERROR
      fprintf(stdlog, "RSerPool socket %d already has sessions\n", sd);
      LOG_END
      errno = EBADF;
      return(-1);
   }
   if(poolHandleSize > MAX_POOLHANDLESIZE) {
      LOG_ERROR
      fputs("Pool handle too long\n", stdlog);
      LOG_END
      errno = EINVAL;
      return(-1);
   }

   /* ====== Create pool element ========================================= */
   rserpoolSocket->PoolElement = poolElementNew(poolHandle, poolHandleSize, tags);
   if(rserpoolSocket->PoolElement == NULL) {
      return(-1);
   }

   return(0);
}


/* ###### Deregister pool element ######################################## */
int rsp_deregister(int sd)
{
   RSerPoolSocket* rserpoolSocket;
   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   CHECK_RSERPOOL_POOLELEMENT(rserpoolSocket);

   /* ====== Delete pool element ========================================= */
/*  ???  if(!leafLinkedRedBlackTreeIsEmpty(&rserpoolSocket->PoolElement->SessionSet)) {
      LOG_ERROR
      fprintf(stdlog, "RSerPool socket %d's pool element still has sessions\n", sd);
      LOG_END_FATAL
   }*/
   poolElementDelete(rserpoolSocket->PoolElement);
   rserpoolSocket->PoolElement = NULL;

   return(0);
}



/* ###### Send cookie ######################################################## */
int rsp_send_cookie(int                  sd,
                    const unsigned char* cookie,
                    const size_t         cookieSize,
                    const unsigned char* poolHandle,
                    const size_t         poolHandleSize,
                    struct TagItem*      tags)
{
   struct RSerPoolSocket*  rserpoolSocket;
//    struct Session*         session;
//    struct RSerPoolMessage* message;
   bool                    result = false;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
//   GET_RSERPOOL_SESSION(session, rserpoolSocket, poolHandle, poolHandleSize);
/*   session = sessionFind(&rserpoolSocket->SessionSet, poolHandle, poolHandleSize);
   if(session != NULL) {

   }

   message = rserpoolMessageNew(NULL,256 + cookieSize);
   if(message != NULL) {
      message->Type       = AHT_COOKIE;
      message->CookiePtr  = (char*)cookie;
      message->CookieSize = (size_t)cookieSize;
      threadSafetyLock(&session->Mutex);
      LOG_VERBOSE
      fputs("Sending Cookie\n", stdlog);
      LOG_END
      result = rserpoolMessageSend(session->SocketProtocol,
                                   session->Socket,
                                   0,
                                   (unsigned int)tagListGetData(tags, TAG_RspIO_Flags, MSG_NOSIGNAL),
                                   (unsigned long long)tagListGetData(tags, TAG_RspIO_Timeout, (tagdata_t)~0),
                                   message);
      threadSafetyUnlock(&session->Mutex);
      rserpoolMessageDelete(message);
   }
   */
   return(result);
}



ssize_t rsp_sendmsg(int                  sd,
                    const void*          data,
                    size_t               dataLength,
                    unsigned int         rserpoolFlags,
                    rserpool_session_t   sessionID,
                    const unsigned char* poolHandle,
                    size_t               poolHandleSize,
                    uint32_t             sctpPPID,
                    uint16_t             sctpStreamNo,
                    uint32_t             sctpTimeToLive,
                    unsigned long long   timeout,
                    struct TagItem*      tags)
{
   struct RSerPoolSocket*  rserpoolSocket;
   struct Session*         session;
   ssize_t                 result;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);
   GET_RSERPOOL_SESSION(session, rserpoolSocket, sessionID, poolHandle, poolHandleSize);

   result = sendtoplus(session->Socket, data, dataLength,
                       MSG_NOSIGNAL,
                       NULL, 0, 0,
                       sctpStreamNo, sctpPPID, sctpTimeToLive, timeout);
   if((result < 0) && (errno != EAGAIN)) {
      LOG_ACTION
      fprintf(stdlog, "Session failure during send on RSerPool socket %d, session %u. Failover necessary\n",
              rserpoolSocket->Descriptor, session->Descriptor);
      LOG_END
      return(-1);
/*      rspSessionFailover(session);
      return(RspRead_Failover);*/
   }
   tagListSetData(tags, TAG_RspIO_PE_ID, session->Identifier);
   return(result);
}


#define MSG_RSERPOOL_NOTIFICATION (1 << 0)
#define MSG_RSERPOOL_COOKIE_ECHO  (1 << 1)


/* ###### RSerPool socket recvmsg() implementation ####################### */
ssize_t rsp_readmsg(int                  sd,
                    void*                buffer,
                    size_t               bufferLength,
                    unsigned int*        rserpoolFlags,
                    rserpool_session_t*  sessionID,
                    uint32_t*            poolElementID,
                    uint32_t*            sctpPPID,
                    uint16_t*            sctpStreamNo,
                    unsigned long long   timeout,
                    struct TagItem*      tags)
{
   struct RSerPoolSocket*  rserpoolSocket;
   struct Session*         session;

   unsigned long long       startTimeStamp;
   unsigned long long       now;
   long long                readTimeout;
   uint32_t                 ppid;
   sctp_assoc_t             assocID;
   unsigned short           streamID;
   ssize_t                  result;
   int                      flags;
   unsigned int             type;
   size_t                   cookieLength;

   GET_RSERPOOL_SOCKET(rserpoolSocket, sd);

   LOG_VERBOSE3
   fprintf(stdlog, "Trying to read message from RSerPool socket %d, socket %d\n",
           rserpoolSocket->Descriptor, session->Socket);
   LOG_END

   /* ====== Cookie Echo ================================================= */
   if((session->CookieEcho) && (length > 0)) {
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

   /* ====== Notification ================================================ */

   /* ====== Really read from socket ===================================== */
   readFD = getSocketToReadFrom(session);

   now = startTimeStamp = getMicroTime();
   do {
      /* ====== Check for timeout ======================================== */
      readTimeout = (long long)timeout - ((long long)now - (long long)startTimeStamp);
      if(readTimeout < 0) {
         LOG_VERBOSE
         fprintf(stdlog, "Reading from RSerPool socket %d, socket %d timed out\n",
                 rserpoolSocket->Descriptor, readFD);
         LOG_END
         errno = EAGAIN;
         return(0);
      }

      /* ====== Read from socket ========================================= */
      LOG_VERBOSE4
      fprintf(stdlog, "Trying to read from session, socket %d, with timeout %Ldus\n",
              session->Socket, readTimeout);
      LOG_END
      result = recvfromplus(readfd, buffer, bufferLength, &flags,
                            NULL, 0,
                            &sctpPPID, &sctpStreamNo,
                            timeout);

      /* ====== Handle ASAP messages ===================================== */
      if((result > 0) && (sctpPPID == PPID_ASAP)) {
         LOG_VERBOSE2
         fprintf(stdlog, "Completely received message of length %d on socket %d\n", result, session->Socket);
         LOG_END
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
         return(RspRead_ControlRead);
      }

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

      now = getMicroTime();
   } while(result > 0);

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
   }
   return(result);
}


static int setFDs(fd_set* fds, int sd, const char* description)
{
   struct RSerPoolSocket*             socket;
   struct Session*                    session;
   struct LeafLinkedRedBlackTreeNode* node;
   int                                n = 0;

   socket = getRSerPoolSocketForDescriptor(sd);
   if(socket != NULL) {
      if(socket->PoolElement) {
         threadSafetyLock(&socket->PoolElement->Mutex);
         LOG_VERBOSE4
         fprintf(stdlog, "Setting <%s> on socket %d for RSerPool socket %d (PE)\n",
                 description, socket->PoolElement->Socket, socket->Descriptor);
         LOG_END
         CHECK((socket->PoolElement->Socket > 0) && (socket->PoolElement->Socket < FD_SETSIZE));
         FD_SET(socket->PoolElement->Socket, fds);
         n = max(n, socket->PoolElement->Socket);
         threadSafetyUnlock(&socket->PoolElement->Mutex);
      }
      node = leafLinkedRedBlackTreeGetFirst(&socket->SessionSet);
      while(node != NULL) {
         session = getSessionFromSocketNode(node);
         threadSafetyLock(&session->Mutex);
         LOG_VERBOSE4
         fprintf(stdlog, "Setting <%s> on socket %d for RSerPool socket %d, session %u\n",
                 description, session->Socket, socket->Descriptor, session->Descriptor);
         LOG_END
         CHECK((session->Socket > 0) && (session->Socket < FD_SETSIZE));
         FD_SET(session->Socket, fds);
         n = max(n, session->Socket);
         threadSafetyUnlock(&session->Mutex);
      }
   }
   return(n);
}


inline int transferFD(int inSD, const fd_set* inFD, int outSD, fd_set* outFD)
{
   if(FD_ISSET(inSD, inFD)) {
      FD_SET(outSD, outFD);
      return(1);
   }
   FD_CLR(outSD, outFD);
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
   struct RSerPoolSocket*             socket;
   struct Session*                    session;
   struct LeafLinkedRedBlackTreeNode* node;
   struct TagItem                     mytags[16];
   struct timeval                     mytimeout;
   fd_set                             myreadfds, mywritefds, myexceptfds;
   fd_set*                            readfds    = (fd_set*)tagListGetData(tags, TAG_RspSelect_ReadFDs,   (tagdata_t)&myreadfds);
   fd_set*                            writefds   = (fd_set*)tagListGetData(tags, TAG_RspSelect_WriteFDs,  (tagdata_t)&mywritefds);
   fd_set*                            exceptfds  = (fd_set*)tagListGetData(tags, TAG_RspSelect_ExceptFDs, (tagdata_t)&myexceptfds);
   struct timeval*                    to = (struct timeval*)tagListGetData(tags, TAG_RspSelect_Timeout,   (tagdata_t)&mytimeout);
   int                                readResult;
   int                                result;
   int                                i;
   int                                n;

   FD_ZERO(&myreadfds);
   FD_ZERO(&mywritefds);
   FD_ZERO(&myexceptfds);
   mytimeout.tv_sec  = timeout / 1000000;
   mytimeout.tv_usec = timeout % 1000000;

   /* ====== Collect data for select() call =============================== */
   n = tagListGetData(tags, TAG_RspSelect_MaxFD, 0);
   for(i = 0;i < rserpoolN;i++) {
      if(FD_ISSET(i, rserpoolReadFDs)) {
         n = max(n, setFDs(readfds, i, "read"));
      }
      if(FD_ISSET(i, rserpoolWriteFDs)) {
         n = max(n, setFDs(writefds, i, "write"));
      }
      if(FD_ISSET(i, rserpoolExceptFDs)) {
         n = max(n, setFDs(exceptfds, i, "except"));
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
         if( FD_ISSET(i, rserpoolReadFDs) ||
             FD_ISSET(i, rserpoolWriteFDs) ||
             FD_ISSET(i, rserpoolExceptFDs) ) {

            socket = getRSerPoolSocketForDescriptor(i);
            if(socket != NULL) {
               /* ====== Events for pool element socket ===================== */
               if(socket->PoolElement) {
                  threadSafetyLock(&socket->PoolElement->Mutex);

                  /* ====== Transfer events ================================= */
                  result +=
                     ((transferFD(socket->PoolElement->Socket, readfds, i, rserpoolReadFDs) +
                       transferFD(socket->PoolElement->Socket, writefds, i, rserpoolWriteFDs) +
                       transferFD(socket->PoolElement->Socket, exceptfds, i, rserpoolExceptFDs)) > 0) ? 1 : 0;

                  threadSafetyUnlock(&socket->PoolElement->Mutex);
               }

               /* ====== Events for sessions ================================ */
               node = leafLinkedRedBlackTreeGetFirst(&socket->SessionSet);
               while(node != NULL) {
                  session = getSessionFromSocketNode(node);
                  threadSafetyLock(&session->Mutex);

                  /* ====== Transfer events ================================= */
                  result +=
                     ((transferFD(session->Socket, readfds, i, rserpoolReadFDs) +
                       transferFD(session->Socket, writefds, i, rserpoolWriteFDs) +
                       transferFD(session->Socket, exceptfds, i, rserpoolExceptFDs)) > 0) ? 1 : 0;

                  /* ======= Check for control channel data ================= */
                  if((session->Socket >= 0) &&
                     FD_ISSET(session->Socket, readfds)) {
                     LOG_VERBOSE4
                     fprintf(stdlog, "RSerPool socket %d, session %u has <read> flag set -> Checking for ASAP message...\n",
                             socket->Descriptor, session->Descriptor);
                     LOG_END

               /*
               mytags[0].Tag  = TAG_RspIO_MakeFailover;
               mytags[0].Data = 0;
               mytags[1].Tag  = TAG_RspIO_Timeout;
               mytags[1].Data = ~0;
               mytags[2].Tag  = TAG_DONE;
               readResult = rspSessionRead(sessionArray[i], NULL, 0, (struct TagItem*)&mytags);
               if((readResult != RspRead_PartialRead) &&
                  (readResult != RspRead_ControlRead)) {
                  LOG_VERBOSE4
                  fprintf(stdlog, "Session with socket FD %d has real event\n",
                          sessionArray[i]->Socket);
                  LOG_END
                  sessionStatusArray[i] |= RspSelect_Read;
               }
               else {
                  LOG_VERBOSE4
                  fprintf(stdlog, "Session with socket FD %d has control data -> Removing <read> flag\n",
                          sessionArray[i]->Socket);
                  LOG_END
               }*/


                  }

                  threadSafetyUnlock(&session->Mutex);
               }
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



int main(int argc, char** argv)
{
   size_t i;
   int sd;

   initLogging("-loglevel=6");
   beginLogging();
   rsp_initialize();

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

   sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   CHECK(sd > 0);
   rsp_register(sd, (const unsigned char*)"ECHO", 4, NULL);
puts("DEREG...");
   rsp_deregister(sd);
puts("CLOSE...");
   rsp_close(sd);

puts("CLEAN");
   rsp_cleanup();
   finishLogging();
}
