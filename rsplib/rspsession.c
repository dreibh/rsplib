/*
 *  $Id: rspsession.c,v 1.29 2004/12/03 17:16:03 dreibh Exp $
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
 * Purpose: RSerPool Session
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

#include <ext_socket.h>
#include <glib.h>


#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif


extern struct Dispatcher gDispatcher;


GList* gSessionList = NULL;


struct PoolElementDescriptor
{
   struct ThreadSafety Mutex;

   struct PoolHandle   Handle;
   uint32_t            Identifier;
   int                 SocketDomain;
   int                 SocketType;
   int                 SocketProtocol;
   int                 Socket;

   unsigned int        PolicyType;
   uint32_t            PolicyParameterWeight;
   uint32_t            PolicyParameterLoad;

   GList*              SessionList;
   struct Timer        ReregistrationTimer;

   unsigned int        RegistrationLife;
   unsigned int        ReregistrationInterval;

   bool                HasControlChannel;
};


struct SessionDescriptor
{
   struct ThreadSafety           Mutex;

   struct PoolHandle             Handle;
   uint32_t                      Identifier;
   int                           SocketDomain;
   int                           SocketType;
   int                           SocketProtocol;
   int                           Socket;

   struct PoolElementDescriptor* PoolElement;
   void*                         Cookie;
   size_t                        CookieSize;
   void*                         CookieEcho;
   size_t                        CookieEchoSize;
   bool                          Incoming;

   unsigned long long            ConnectionTimeStamp;
   unsigned long long            ConnectTimeout;
   unsigned long long            NameResolutionRetryDelay;

   struct MessageBuffer*         MessageBuffer;
   struct TagItem*               Tags;

   char                          StatusTextText[128];
};


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


/* ###### Reregistration ##################################################### */
static bool rspPoolElementUpdateRegistration(struct PoolElementDescriptor* ped)
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
   threadSafetyLock(&ped->Mutex);

   eai->ai_family   = ped->SocketDomain;
   eai->ai_socktype = ped->SocketType;
   eai->ai_protocol = ped->SocketProtocol;
   eai->ai_next     = NULL;
   eai->ai_addr     = NULL;
   eai->ai_addrs    = 0;
   eai->ai_addrlen  = sizeof(union sockaddr_union);
   eai->ai_pe_id    = ped->Identifier;

   /* ====== Get local addresses for SCTP socket ============================ */
   if(ped->SocketProtocol == IPPROTO_SCTP) {
      eai->ai_addrs = getladdrsplus(ped->Socket, 0, &eai->ai_addr);
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

   /* ====== Get local addresses for TCP/UDP socket ========================= */
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
      if(ext_getsockname(ped->Socket, (struct sockaddr*)&socketName, &socketNameLen) == 0) {
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
      if(ped->SocketProtocol == IPPROTO_SCTP) {
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
   tags[0].Data = ped->PolicyType;
   tags[1].Tag  = TAG_PoolPolicy_Parameter_Load;
   tags[1].Data = ped->PolicyParameterLoad;
   tags[2].Tag  = TAG_PoolPolicy_Parameter_Weight;
   tags[2].Data = ped->PolicyParameterWeight;
   tags[3].Tag  = TAG_UserTransport_HasControlChannel;
   tags[3].Data = (tagdata_t)ped->HasControlChannel;
   tags[4].Tag  = TAG_END;

   /* ====== Do registration ================================================ */
   result = rspRegister((unsigned char*)&ped->Handle.Handle,
                        ped->Handle.Size,
                        eai, (struct TagItem*)&tags);
   if(result == RSPERR_OKAY) {
      ped->Identifier = eai->ai_pe_id;
      LOG_ACTION
      fprintf(stdlog, "(Re-)Registration successful, ID is $%08x\n", ped->Identifier);
      LOG_END
printf("(Re-)Registration successful, ID is $%08x\n", ped->Identifier);
   }
   else {
      LOG_WARNING
      fprintf(stdlog, "(Re-)Registration failed: ");
      rserpoolErrorPrint(result, stdlog);
      fputs("\n", stdlog);
      LOG_END
   }


   /* ====== Clean up ======================================================= */
   threadSafetyUnlock(&ped->Mutex);
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


/* ###### Reregistration timer ############################################### */
static void reregistrationTimer(struct Dispatcher* dispatcher,
                                struct Timer*      timer,
                                void*              userData)
{
   struct PoolElementDescriptor* ped = (struct PoolElementDescriptor*)userData;

   LOG_VERBOSE3
   fputs("Starting reregistration\n", stdlog);
   LOG_END
   threadSafetyLock(&ped->Mutex);
   rspPoolElementUpdateRegistration(ped);
   timerStart(&ped->ReregistrationTimer,
              getMicroTime() + ((unsigned long long)1000 * (unsigned long long)ped->ReregistrationInterval));
   threadSafetyUnlock(&ped->Mutex);
   LOG_VERBOSE3
   fputs("Reregistration completed\n", stdlog);
   LOG_END
}


/* ###### Delete pool element ###############################################+ */
void rspDeletePoolElement(struct PoolElementDescriptor* ped,
                          struct TagItem*               tags)
{
   GList* list;

   if(ped) {
      list = g_list_first(ped->SessionList);
      if(list == NULL) {
         timerDelete(&ped->ReregistrationTimer);
         if(ped->Identifier != 0x00000000) {
            rspDeregister((unsigned char*)&ped->Handle.Handle,
                          ped->Handle.Size,
                          ped->Identifier, tags);
         }
         if(ped->Socket >= 0) {
            ext_close(ped->Socket);
            ped->Socket = -1;
         }
         threadSafetyDestroy(&ped->Mutex);
         free(ped);
      }
      else {
         LOG_ERROR
         fputs("Pool element to be deleted has still open sessions. Go fix your program!\n", stdlog);
         LOG_END
      }
   }
}


/* ###### Create pool element ################################################ */
struct PoolElementDescriptor* rspCreatePoolElement(const unsigned char* poolHandle,
                                                   const size_t         poolHandleSize,
                                                   struct TagItem*      tags)
{
   union sockaddr_union localAddress;

   struct PoolElementDescriptor* ped = (struct PoolElementDescriptor*)malloc(sizeof(struct PoolElementDescriptor));
   if(ped != NULL) {
      LOG_ACTION
      fputs("Trying to create pool element\n", stdlog);
      LOG_END

      /* ====== Initialize pool element ===================================== */
      if(poolHandleSize > MAX_POOLHANDLESIZE) {
         LOG_ERROR
         fputs("Pool handle too long\n", stdlog);
         LOG_END_FATAL
      }
      poolHandleNew(&ped->Handle, poolHandle, poolHandleSize);

      threadSafetyInit(&ped->Mutex, "RspPoolElement");
      timerNew(&ped->ReregistrationTimer,
               &gDispatcher,
               reregistrationTimer,
               (void*)ped);
      ped->Socket                 = -1;
      ped->Identifier             = tagListGetData(tags, TAG_PoolElement_Identifier,
                                                   0x00000000);
      ped->SocketDomain           = tagListGetData(tags, TAG_PoolElement_SocketDomain,
                                                   checkIPv6() ? AF_INET6 : AF_INET);
      ped->SocketType             = tagListGetData(tags, TAG_PoolElement_SocketType,
                                                   SOCK_STREAM);
      ped->SocketProtocol         = tagListGetData(tags, TAG_PoolElement_SocketProtocol,
                                                   IPPROTO_SCTP);
      ped->ReregistrationInterval = tagListGetData(tags, TAG_PoolElement_ReregistrationInterval,
                                                   5000);
      ped->RegistrationLife       = tagListGetData(tags, TAG_PoolElement_RegistrationLife,
                                                   (ped->ReregistrationInterval * 3) + 3000);
      ped->PolicyType             = tagListGetData(tags, TAG_PoolPolicy_Type,
                                                   PPT_ROUNDROBIN);
      ped->PolicyParameterWeight  = tagListGetData(tags, TAG_PoolPolicy_Parameter_Weight,
                                                   1);
      ped->PolicyParameterLoad    = tagListGetData(tags, TAG_PoolPolicy_Parameter_Load,
                                                   0);
      ped->SessionList            = NULL;
      ped->HasControlChannel      = tagListGetData(tags, TAG_UserTransport_HasControlChannel, false);

      /* ====== Create socket =============================================== */
      ped->Socket = ext_socket(ped->SocketDomain,
                               ped->SocketType,
                               ped->SocketProtocol);
      if(ped->Socket <= 0) {
         LOG_ERROR
         logerror("Unable to create socket for new pool element");
         LOG_END
         rspDeletePoolElement(ped, NULL);
         return(NULL);
      }

      /* ====== Configure SCTP socket ======================================  */
      if(ped->SocketProtocol == IPPROTO_SCTP) {
         if(!configureSCTPSocket(ped->Socket, 0, tags)) {
            LOG_ERROR
            fprintf(stdlog, "Failed to configure SCTP socket FD %d\n", ped->Socket);
            LOG_END
            rspDeletePoolElement(ped, NULL);
            return(NULL);
         }
      }

      /* ====== Bind socket ================================================= */
      memset((void*)&localAddress, 0, sizeof(localAddress));
      ((struct sockaddr*)&localAddress)->sa_family = ped->SocketDomain;
      setPort((struct sockaddr*)&localAddress, tagListGetData(tags, TAG_PoolElement_LocalPort, 0));

      if(bindplus(ped->Socket, (union sockaddr_union*)&localAddress, 1) == false) {
         LOG_ERROR
         logerror("Unable to bind socket for new pool element");
         LOG_END
         rspDeletePoolElement(ped, NULL);
         return(NULL);
      }
      if((ped->SocketType == SOCK_STREAM) && (ext_listen(ped->Socket, 5) < 0)) {
         LOG_WARNING
         logerror("Unable to set socket for new pool element to listen mode");
         LOG_END
      }


      /* ====== Do registration ================================================ */
      if(rspPoolElementUpdateRegistration(ped) == false) {
         LOG_ERROR
         fputs("Unable to obtain registration information -> Creating pool element not possible\n", stdlog);
         LOG_END
         rspDeletePoolElement(ped, NULL);
         return(NULL);
      }

      /* Okay -> start reregistration timer */
      timerStart(&ped->ReregistrationTimer,
                 getMicroTime() + ((unsigned long long)1000 * (unsigned long long)ped->ReregistrationInterval));
   }

   return(ped);
}


/* ###### Create new session ################################################# */
static struct SessionDescriptor* rspSessionNew(
                                    const int                     socketDomain,
                                    const int                     socketType,
                                    const int                     socketProtocol,
                                    const int                     socketDescriptor,
                                    const bool                    incoming,
                                    struct PoolElementDescriptor* ped,
                                    const unsigned char*          poolHandle,
                                    const size_t                  poolHandleSize,
                                    struct TagItem*               tags)
{
   struct SessionDescriptor* session = (struct SessionDescriptor*)malloc(sizeof(struct SessionDescriptor));
   if(session != NULL) {
      threadSafetyInit(&session->Mutex, "RspSession");
      session->MessageBuffer = messageBufferNew(65536);
      if(session->MessageBuffer == NULL) {
         free(session);
         return(NULL);
      }
      session->Tags = tagListDuplicate(tags);  /* ??? FILTER !!! */
      if(session->Tags == NULL) {
         messageBufferDelete(session->MessageBuffer);
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
      session->SocketDomain             = socketDomain;
      session->SocketType               = socketType;
      session->SocketProtocol           = socketProtocol;
      session->Socket                   = socketDescriptor;
      session->PoolElement              = ped;
      session->Incoming                 = incoming;
      session->Cookie                   = NULL;
      session->CookieSize               = 0;
      session->CookieEcho               = NULL;
      session->CookieEchoSize           = 0;
      session->StatusTextText[0]         = 0x00;
      session->ConnectionTimeStamp      = 0;
      session->ConnectTimeout           = (unsigned long long)tagListGetData(tags, TAG_RspSession_ConnectTimeout, 5000000);
      session->NameResolutionRetryDelay = (unsigned long long)tagListGetData(tags, TAG_RspSession_NameResolutionRetryDelay, 250000);
      if(session->PoolElement != NULL) {
         threadSafetyLock(&session->PoolElement->Mutex);
         session->PoolElement->SessionList =
            g_list_append(session->PoolElement->SessionList, session);
         threadSafetyUnlock(&session->PoolElement->Mutex);
      }

      dispatcherLock(&gDispatcher);
      gSessionList = g_list_append(gSessionList, session);
      dispatcherUnlock(&gDispatcher);
   }
   return(session);
}


/* ###### Delete session ##################################################### */
static void rspSessionDelete(struct SessionDescriptor* session)
{
   if(session) {
      dispatcherLock(&gDispatcher);
      gSessionList = g_list_remove(gSessionList, session);
      dispatcherUnlock(&gDispatcher);
      if(session->PoolElement) {
         threadSafetyLock(&session->PoolElement->Mutex);
         session->PoolElement->SessionList = g_list_remove(session->PoolElement->SessionList, session);
         threadSafetyUnlock(&session->PoolElement->Mutex);
         session->PoolElement = NULL;
      }
      if(session->Socket >= 0) {
         ext_close(session->Socket);
         session->Socket = -1;
      }
      if(session->MessageBuffer) {
         messageBufferDelete(session->MessageBuffer);
         session->MessageBuffer = NULL;
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


/* ###### Send cookie ######################################################## */
bool rspSessionSendCookie(struct SessionDescriptor* session,
                          const unsigned char*      cookie,
                          const size_t              cookieSize,
                          struct TagItem*           tags)
{
   struct RSerPoolMessage* message;
   bool                result = false;

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
                                   0, 0,
                                   (unsigned long long)tagListGetData(tags, TAG_RspIO_Timeout, (tagdata_t)~0),
                                   message);
      threadSafetyUnlock(&session->Mutex);
      rserpoolMessageDelete(message);
   }
   return(result);
}


/* ###### Send cookie echo ################################################### */
static bool rspSessionSendCookieEcho(struct SessionDescriptor* session)
{
   struct RSerPoolMessage* message;
   bool                result = false;

   if(session->Cookie) {
      message = rserpoolMessageNew(NULL,256 + session->CookieSize);
      if(message != NULL) {
         message->Type       = AHT_COOKIE_ECHO;
         message->CookiePtr  = session->Cookie;
         message->CookieSize = session->CookieSize;
         threadSafetyLock(&session->Mutex);
         LOG_ACTION
         fputs("Sending Cookie Echo\n", stdlog);
         LOG_END
         result = rserpoolMessageSend(session->SocketProtocol,
                                      session->Socket,
                                      0, 0,
                                      0,
                                      message);
         threadSafetyUnlock(&session->Mutex);
         rserpoolMessageDelete(message);
      }
   }
   return(result);
}


/* ###### Send business card ################################################# */
static void rspSessionSendBusinessCard(struct SessionDescriptor* session)
{
   if(session->PoolElement) {
      threadSafetyLock(&session->PoolElement->Mutex);
      threadSafetyLock(&session->Mutex);
      /* ??? rspSendBusinessCard(session->Socket, ...) */
      threadSafetyUnlock(&session->Mutex);
      threadSafetyUnlock(&session->PoolElement->Mutex);
   }
}


/* ###### Do session failover ################################################*/
static bool rspSessionFailover(struct SessionDescriptor* session)
{
   struct EndpointAddressInfo* eai;
   struct EndpointAddressInfo* eai2;
   int                         result;
   bool                        doFailover;
   FailoverCallbackPtr         callback;

   threadSafetyLock(&session->Mutex);

   if(session->Socket >= 0) {
      /* ====== Close connection ============================================ */
      LOG_ACTION
      fprintf(stdlog, "Closing socket %d\n", session->Socket);
      LOG_END
      ext_close(session->Socket);
      session->Socket              = -1;
      session->ConnectionTimeStamp = 0;

      /* ====== Report failure ============================================== */
      if(!session->Incoming) {
         LOG_ACTION
         fprintf(stdlog, "Reporting failure of pool element $%08x\n", session->Identifier);
         LOG_END
         rspReportFailure((unsigned char*)&session->Handle.Handle,
                          session->Handle.Size,
                          session->Identifier, NULL);
      }

      /* ====== Invoke callback ============================================= */
      callback = (FailoverCallbackPtr)tagListGetData(session->Tags, TAG_RspSession_FailoverCallback, (tagdata_t)NULL);
      if(callback) {
         LOG_VERBOSE3
         fputs("Invoking callback...\n", stdlog);
         LOG_END
         doFailover = callback((void*)tagListGetData(session->Tags, TAG_RspSession_FailoverUserData, (tagdata_t)NULL));
         LOG_VERBOSE3
         fputs("Callback completed\n", stdlog);
         LOG_END
         if(!doFailover) {
            LOG_ACTION
            fputs("User decided to abort failover\n", stdlog);
            LOG_END
            threadSafetyUnlock(&session->Mutex);
            return(false);
         }
      }
      else {
         LOG_VERBOSE2
         fputs("There is no callback installed\n", stdlog);
         LOG_END
      }
   }

   /* ====== Do name resolution ============================================= */
   if(session->Handle.Size > 0) {
      LOG_ACTION
      fputs("Doing name resolution\n", stdlog);
      LOG_END
      result = rspNameResolution((unsigned char*)&session->Handle.Handle,
                                 session->Handle.Size,
                                 &eai, NULL);
      if(result == RSPERR_OKAY) {

         /* ====== Establish connection ======================================== */
         eai2 = eai;
         while(eai2 != NULL) {
            LOG_VERBOSE
            fprintf(stdlog, "Trying connection to pool element $%08x...\n", eai2->ai_pe_id);
            LOG_END
            session->Socket = establish(eai2->ai_family,
                                        eai2->ai_socktype,
                                        eai2->ai_protocol,
                                        eai2->ai_addr,
                                        eai2->ai_addrs,
                                        session->ConnectTimeout);
            if(session->Socket >= 0) {
               session->SocketDomain   = eai2->ai_family;
               session->SocketType     = eai2->ai_socktype;
               session->SocketProtocol = eai2->ai_protocol;
               if((eai2->ai_protocol == IPPROTO_SCTP) &&
                  ((!configureSCTPSocket(session->Socket, 0, session->Tags)) ||
                   (!tuneSCTP(session->Socket, 0, session->Tags)))) {
                  LOG_ERROR
                  fprintf(stdlog, "Failed to configure SCTP socket FD %d -> Closing\n", session->Socket);
                  LOG_END
                  ext_close(session->Socket);
                  session->Socket = -1;
               }
               else {
                  session->ConnectionTimeStamp = getMicroTime();
                  session->Identifier          = eai2->ai_pe_id;
                  setNonBlocking(session->Socket);
                  LOG_ACTION
                  fprintf(stdlog, "Socket %d connected to ", session->Socket);
                  fputaddress((struct sockaddr*)&eai2->ai_addr[0], true, stdlog);
                  fprintf(stdlog, ", Pool Element $%08x\n", session->Identifier);
                  LOG_END
                  break;
               }
            }

            if(session->Socket < 0) {
               LOG_ACTION
               fprintf(stdlog, "Reporting failure of pool element $%08x\n", eai2->ai_pe_id);
               LOG_END
               rspReportFailure((unsigned char*)&session->Handle.Handle,
                                session->Handle.Size,
                                eai2->ai_pe_id, NULL);
            }

            eai2 = eai2->ai_next;
         }

         /* ====== Free name resolution result ================================= */
         rspFreeEndpointAddressArray(eai);

         if(session->Socket >= 0) {
            if(session->Cookie) {
               rspSessionSendCookieEcho(session);
            }
            if((!session->Incoming) && (session->PoolElement)) {
               rspSessionSendBusinessCard(session);
            }
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
                 "Name resolution did not find new pool element. Waiting %lluus...\n",
                 session->NameResolutionRetryDelay);
         LOG_END
         usleep((unsigned int)session->NameResolutionRetryDelay);
      }
      else {
         LOG_WARNING
         fputs("Name resolution not successful: ", stdlog);
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
   }
   else {
      LOG_VERBOSE
      fputs("No pool handle for failover: incoming session with no business card\n", stdlog);
      LOG_END
   }

   threadSafetyUnlock(&session->Mutex);
   return(session->Socket >= 0);
}


/* ###### Accept session #####################################################*/
struct SessionDescriptor* rspAcceptSession(struct PoolElementDescriptor* ped,
                                           struct TagItem*               tags)
{
   struct SessionDescriptor* session = NULL;
   int sd;
   LOG_ACTION
   fprintf(stdlog, "Accepting new association on socket %d\n", ped->Socket);
   LOG_END
   sd = ext_accept(ped->Socket, NULL, 0);
   if(sd >= 0) {
      if((ped->SocketProtocol == IPPROTO_SCTP) &&
         ((!configureSCTPSocket(sd, 0, tags)) ||
         (!tuneSCTP(sd, 0, tags)))) {
         LOG_ERROR
         fprintf(stdlog, "Failed to configure new SCTP association FD %d -> Closing it\n", sd);
         LOG_END
         ext_close(sd);
      }
      else {
         LOG_ACTION
         fprintf(stdlog, "Accepted new association on socket %d => new socket %d\n",
                 ped->Socket, sd);
         LOG_END

         session = rspSessionNew(ped->SocketDomain, ped->SocketType, ped->SocketProtocol,
                                 sd, true,
                                 ped, NULL, 0,
                                 tags);
         if(session == NULL) {
            ext_close(sd);
         }
      }
   }
   else {
      LOG_ERROR
      logerror("Accepting new session failed");
      LOG_END
   }
   return(session);
}


/* ###### Create session ##################################################### */
struct SessionDescriptor* rspCreateSession(const unsigned char*          poolHandle,
                                           const size_t                  poolHandleSize,
                                           struct PoolElementDescriptor* ped,
                                           struct TagItem*               tags)
{
   struct SessionDescriptor* session =
      rspSessionNew(0, 0, 0, -1, false, ped, poolHandle, poolHandleSize, tags);
   if(session == NULL) {
      LOG_ERROR
      fputs("Creating SessionDescriptor object failed!\n", stdlog);
      LOG_END
   }

   rspSessionFailover(session);
   return(session);
}


/* ###### Delete session ##################################################### */
void rspDeleteSession(struct SessionDescriptor* session,
                      struct TagItem*           tags)
{
   rspSessionDelete(session);
}


/* ###### Handle ASAP message (PE<->PE/PU) ###################################*/
static unsigned int handleRSerPoolMessage(struct SessionDescriptor* session,
                                          char*                     buffer,
                                          size_t                    size)
{
   struct RSerPoolMessage* message;
   CookieEchoCallbackPtr   callback;
   unsigned int            type = 0;
   unsigned int            result;

   LOG_VERBOSE
   fputs("Handling ASAP message from control channel...\n", stdlog);
   LOG_END

   threadSafetyLock(&session->Mutex);

   result = rserpoolPacket2Message(buffer, NULL, PPID_ASAP, size, size, &message);
   if(message != NULL) {
      if(result == RSPERR_OKAY) {
         LOG_VERBOSE2
         fprintf(stdlog, "Received ASAP type $%04x from session, socket %d\n",
               message->Type, session->Socket);
         LOG_END
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
               session->Cookie     = message->CookiePtr;
               session->CookieSize = message->CookieSize;
            break;
            case AHT_COOKIE_ECHO:
               if(session->CookieEcho == NULL) {
                  LOG_ACTION
                  fputs("Got cookie echo\n", stdlog);
                  LOG_END
                  message->CookiePtrAutoDelete = false;
                  session->CookieEcho     = message->CookiePtr;
                  session->CookieEchoSize = message->CookieSize;
                  callback = (CookieEchoCallbackPtr)tagListGetData(session->Tags, TAG_RspSession_ReceivedCookieEchoCallback, (tagdata_t)NULL);
                  if(callback) {
                     LOG_VERBOSE3
                     fputs("Invoking callback...\n", stdlog);
                     LOG_END
                     callback((void*)tagListGetData(session->Tags, TAG_RspSession_ReceivedCookieEchoUserData, (tagdata_t)NULL),
                              message->CookiePtr,
                              message->CookieSize);
                     LOG_VERBOSE3
                     fputs("Callback completed\n", stdlog);
                     LOG_END
                  }
                  else {
                     LOG_VERBOSE2
                     fputs("There is no callback installed\n", stdlog);
                     LOG_END
                  }
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

   threadSafetyUnlock(&session->Mutex);

   LOG_VERBOSE
   fputs("Handling ASAP message from control channel completed\n", stdlog);
   LOG_END
   return(type);
}


/* ###### Read from session ##################################################*/
ssize_t rspSessionRead(struct SessionDescriptor* session,
                       void*                     buffer,
                       const size_t              length,
                       struct TagItem*           tags)
{
   const unsigned long long timeout        = (unsigned long long)tagListGetData(tags, TAG_RspIO_Timeout, (tagdata_t)~0);
   unsigned long long       startTimeStamp = getMicroTime();
   unsigned long long       now;
   long long                readTimeout;
   uint32_t                 ppid;
   sctp_assoc_t             assocID;
   unsigned short           streamID;
   ssize_t                  result;
   int                      flags;

   tagListSetData(tags, TAG_RspIO_MsgIsCookie, 0);
   LOG_VERBOSE3
   fprintf(stdlog, "Trying to read message from session, socket %d\n",
           session->Socket);
   LOG_END

   do {
      now = getMicroTime();
      readTimeout = (long long)timeout - (long long)(now - startTimeStamp);
      if(readTimeout < 0) {
         LOG_VERBOSE
         fprintf(stdlog, "Reading from session, socket %d, timed out\n",
                 session->Socket);
         LOG_END
         return(RspRead_Timeout);
      }

      LOG_VERBOSE4
      fprintf(stdlog, "Trying to read from session, socket %d, with timeout %Ldus\n",
              session->Socket, readTimeout);
      LOG_END
      result = messageBufferRead(session->MessageBuffer, session->Socket,
                                 NULL, 0,
                                 PPID_ASAP,
                                 (unsigned long long)readTimeout,
                                 (unsigned long long)readTimeout);
      if(result > 0) {
         LOG_VERBOSE2
         fprintf(stdlog, "Completely received message of length %d on socket %d\n", result, session->Socket);
         LOG_END
         if(handleRSerPoolMessage(session, (char*)&session->MessageBuffer->Buffer, (size_t)result) == AHT_COOKIE_ECHO) {
            if(session->CookieEcho) {
               if(length >= session->CookieEchoSize) {
                  tagListSetData(tags, TAG_RspIO_MsgIsCookie, 1);
                  memcpy(buffer, session->CookieEcho, session->CookieEchoSize);
                  return(session->CookieEchoSize);
               }
               free(session->CookieEcho);
               session->CookieEcho     = NULL;
               session->CookieEchoSize = 0;
            }
         }
      }
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

   if( ( (result <= 0) && (errno != EAGAIN) ) &&
       (tagListGetData(tags, TAG_RspIO_MakeFailover, 1) != 0) ) {
      LOG_ACTION
      fprintf(stdlog, "Session failure during read, socket %d. Failover necessary\n",
              session->Socket);
      LOG_END
      rspSessionFailover(session);
   }
   else {
      tagListSetData(tags, TAG_RspIO_SCTP_AssocID,  (tagdata_t)assocID);
      tagListSetData(tags, TAG_RspIO_SCTP_StreamID, streamID);
      tagListSetData(tags, TAG_RspIO_SCTP_PPID,     ppid);
      tagListSetData(tags, TAG_RspIO_PE_ID,         session->Identifier);
   }
   return(result);
}


/* ###### Write to session ###################################################*/
ssize_t rspSessionWrite(struct SessionDescriptor* session,
                        const void*               buffer,
                        const size_t              length,
                        struct TagItem*           tags)
{
   ssize_t result = sendtoplus(session->Socket, buffer, length,
                               tagListGetData(tags, TAG_RspIO_Flags, MSG_NOSIGNAL),
                               NULL, 0,
                               0,
                               (const sctp_assoc_t)tagListGetData(tags, TAG_RspIO_SCTP_StreamID,   0),
                               tagListGetData(tags, TAG_RspIO_SCTP_PPID,       0),
                               tagListGetData(tags, TAG_RspIO_SCTP_TimeToLive, 0xffffffff),
                               tagListGetData(tags, TAG_RspIO_Timeout, (tagdata_t)~0));
   if((result < 0) && (errno != EAGAIN)) {
      LOG_ACTION
      fprintf(stdlog, "Session failure during write, socket %d. Failover necessary\n",
              session->Socket);
      LOG_END
      rspSessionFailover(session);
   }
   tagListSetData(tags, TAG_RspIO_PE_ID, session->Identifier);
   return(result);
}


/* ###### Session select() implementation ####################################*/
int rspSessionSelect(struct SessionDescriptor**     sessionArray,
                     unsigned int*                  sessionStatusArray,
                     const size_t                   sessions,
                     struct PoolElementDescriptor** pedArray,
                     unsigned int*                  pedStatusArray,
                     const size_t                   peds,
                     const unsigned long long       timeout,
                     struct TagItem*                tags)
{
   struct TagItem mytags[16];
   struct timeval mytimeout;
   fd_set  myreadfds, mywritefds, myexceptfds;
   fd_set* readfds    = (fd_set*)tagListGetData(tags, TAG_RspSelect_ReadFDs,   (tagdata_t)&myreadfds);
   fd_set* writefds   = (fd_set*)tagListGetData(tags, TAG_RspSelect_WriteFDs,  (tagdata_t)&mywritefds);
   fd_set* exceptfds  = (fd_set*)tagListGetData(tags, TAG_RspSelect_ExceptFDs, (tagdata_t)&myexceptfds);
   struct timeval* to = (struct timeval*)tagListGetData(tags, TAG_RspSelect_Timeout, (tagdata_t)&mytimeout);
   int     readResult;
   int     result;
   int     n;
   size_t  i;

   FD_ZERO(&myreadfds);
   FD_ZERO(&mywritefds);
   FD_ZERO(&myexceptfds);
   mytimeout.tv_sec  = timeout / 1000000;
   mytimeout.tv_usec = timeout % 1000000;

   /* ====== Collect data for select() call =============================== */
   n = tagListGetData(tags, TAG_RspSelect_MaxFD, 0);
   for(i = 0;i < sessions;i++) {
      threadSafetyLock(&sessionArray[i]->Mutex);
      if(sessionArray[i]->Socket >= 0) {
         if(sessionStatusArray[i] & RspSelect_Read) {
            FD_SET(sessionArray[i]->Socket, readfds);
            n = max(n, sessionArray[i]->Socket);
         }
         if(sessionStatusArray[i] & RspSelect_Write) {
            FD_SET(sessionArray[i]->Socket, writefds);
            n = max(n, sessionArray[i]->Socket);
         }
         if(sessionStatusArray[i] & RspSelect_Except) {
            FD_SET(sessionArray[i]->Socket, exceptfds);
            n = max(n, sessionArray[i]->Socket);
         }
      }
      else {
         LOG_VERBOSE4
         fprintf(stdlog, "Session %d: No FD\n",(int)i);
         LOG_END
      }
      threadSafetyUnlock(&sessionArray[i]->Mutex);
   }
   for(i = 0;i < peds;i++) {
      threadSafetyLock(&pedArray[i]->Mutex);
      if(pedStatusArray[i] & RspSelect_Read) {
         FD_SET(pedArray[i]->Socket, readfds);
         n = max(n, pedArray[i]->Socket);
      }
      if(pedStatusArray[i] & RspSelect_Write) {
         FD_SET(pedArray[i]->Socket, writefds);
         n = max(n, pedArray[i]->Socket);
      }
      if(pedStatusArray[i] & RspSelect_Except) {
         FD_SET(pedArray[i]->Socket, exceptfds);
         n = max(n, pedArray[i]->Socket);
      }
      threadSafetyUnlock(&pedArray[i]->Mutex);
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
   for(i = 0;i < sessions;i++) {
      threadSafetyLock(&sessionArray[i]->Mutex);
      sessionStatusArray[i] = 0;
      if(sessionArray[i]->Socket >= 0) {
         if(FD_ISSET(sessionArray[i]->Socket, readfds)) {
            LOG_VERBOSE4
            fprintf(stdlog, "Session with socket FD %d has <read> flag set -> Checking for ASAP message...\n",
                    sessionArray[i]->Socket);
            LOG_END
            mytags[0].Tag  = TAG_RspIO_MakeFailover;
            mytags[0].Data = 0;
            mytags[1].Tag  = TAG_DONE;
            readResult = rspSessionRead(sessionArray[i], NULL, 0, (struct TagItem*)&mytags);
            if((readResult != RspRead_MessageRead) && (readResult != RspRead_PartialRead)) {
               LOG_VERBOSE4
               fprintf(stdlog, "Session with socket FD %d has user data\n",
                       sessionArray[i]->Socket);
               LOG_END
               sessionStatusArray[i] |= RspSelect_Read;
            }
            else {
               LOG_VERBOSE4
               fprintf(stdlog, "Session with socket FD %d has control data -> Removing <read> flag\n",
                       sessionArray[i]->Socket);
               LOG_END
            }
         }
         if(FD_ISSET(sessionArray[i]->Socket, writefds)) {
            sessionStatusArray[i] |= RspSelect_Write;
         }
         if(FD_ISSET(sessionArray[i]->Socket, exceptfds)) {
            sessionStatusArray[i] |= RspSelect_Except;
         }
      }
      else {
         LOG_VERBOSE4
         fprintf(stdlog, "Session %d: No FD -> Setting <write>!\n",(int)i);
         LOG_END
         sessionStatusArray[i] |= RspSelect_Write|RspSelect_Except;
      }
      threadSafetyUnlock(&sessionArray[i]->Mutex);
   }
   for(i = 0;i < peds;i++) {
      threadSafetyLock(&pedArray[i]->Mutex);
      pedStatusArray[i] = 0;
      if(FD_ISSET(pedArray[i]->Socket, readfds)) {
         pedStatusArray[i] |= RspSelect_Read;
      }
      if(FD_ISSET(pedArray[i]->Socket, writefds)) {
         pedStatusArray[i] |= RspSelect_Write;
      }
      if(FD_ISSET(pedArray[i]->Socket, exceptfds)) {
         pedStatusArray[i] |= RspSelect_Except;
      }
      threadSafetyUnlock(&pedArray[i]->Mutex);
   }

   return(result);
}


/* ###### Set session's CSP status text ################################## */
void rspSessionSetStatusText(struct SessionDescriptor* session,
                             const char*               statusText)
{
   threadSafetyLock(&session->Mutex);
   safestrcpy((char*)&session->StatusTextText,
              statusText,
              sizeof(session->StatusTextText));
   threadSafetyUnlock(&session->Mutex);
}


/* ###### Get CSP status ################################################# */
size_t rspSessionCreateComponentStatus(
          struct ComponentAssociationEntry** caeArray,
          char*                              statusText,
          const int                          registrarSocket,
          const RegistrarIdentifierType           registrarID,
          const int                          registrarSocketProtocol,
          const unsigned long long           registrarConnectionTimeStamp)
{
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
      statusText[0] = 0x00;
      if(registrarSocket >= 0) {
         (*caeArray)[caeArraySize].ReceiverID = CID_COMPOUND(CID_GROUP_NAMESERVER, registrarID);
         (*caeArray)[caeArraySize].Duration   = getMicroTime() - registrarConnectionTimeStamp;
         (*caeArray)[caeArraySize].Flags      = 0;
         (*caeArray)[caeArraySize].ProtocolID = registrarSocketProtocol;
         (*caeArray)[caeArraySize].PPID       = PPID_ASAP;
         caeArraySize++;
      }

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
}

