/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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
#include "rserpoolsocket.h"
#include "threadsafety.h"
#include "netutilities.h"
#include "rserpoolmessage.h"
#include "loglevel.h"
#include "debug.h"


extern struct SimpleRedBlackTree gRSerPoolSocketSet;
extern struct ThreadSafety       gRSerPoolSocketSetMutex;


/* ###### Print function ################################################# */
void rserpoolSocketPrint(const void* node, FILE* fd)
{
   const struct RSerPoolSocket* rserpoolSocket = (const struct RSerPoolSocket*)node;

   fprintf(fd, "%d ", rserpoolSocket->Descriptor);
}


/* ###### Comparison function ############################################ */
int rserpoolSocketComparison(const void* node1, const void* node2)
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


/* ###### Get RSerPool socket for given descriptor ####################### */
struct RSerPoolSocket* getRSerPoolSocketForDescriptor(int sd)
{
   struct RSerPoolSocket* rserpoolSocket;
   struct RSerPoolSocket  cmpSocket;

   cmpSocket.Descriptor = sd;
   threadSafetyLock(&gRSerPoolSocketSetMutex);
   rserpoolSocket = (struct RSerPoolSocket*)leafLinkedRedBlackTreeFind(&gRSerPoolSocketSet,
                                                                       &cmpSocket.Node);
   threadSafetyUnlock(&gRSerPoolSocketSetMutex);
   if(rserpoolSocket == NULL) {
      LOG_ERROR
      fprintf(stdlog, "Bad RSerPool socket descriptor %d\n", sd);
      LOG_END
   }
   return(rserpoolSocket);
}


/* ###### Wait until there is something to read ########################## */
bool waitForRead(struct RSerPoolSocket* rserpoolSocket,
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


/* ###### Delete pool element ############################################ */
void deletePoolElement(struct PoolElement* poolElement, struct TagItem* tags)
{
   int result;

   /* Delete timer first; this ensures that the doRegistration() function
      will not be called while the PE is going to be deleted! */
   timerDelete(&poolElement->ReregistrationTimer);

   if(poolElement->Identifier != 0x00000000) {
      result = rsp_pe_deregistration_tags((unsigned char*)&poolElement->Handle.Handle,
                                          poolElement->Handle.Size,
                                          poolElement->Identifier,
                                          tags);
      if(result != RSPERR_OKAY) {
         LOG_ERROR
         fprintf(stdlog, "Deregistration failed: ");
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
   }

   threadSafetyDelete(&poolElement->Mutex);
   free(poolElement);
}


/* ###### Reregistration timer ########################################### */
void reregistrationTimer(struct Dispatcher* dispatcher,
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
   /* Do not wait for result here. This would deadlock, since
      this timer callback is called withing the MainLoop thread! */
   doRegistration(rserpoolSocket, false);
   timerStart(&rserpoolSocket->PoolElement->ReregistrationTimer,
              getMicroTime() + ((unsigned long long)1000 * (unsigned long long)rserpoolSocket->PoolElement->ReregistrationInterval));
   threadSafetyUnlock(&rserpoolSocket->PoolElement->Mutex);

   LOG_VERBOSE3
   fputs("Reregistration completed\n", stdlog);
   LOG_END
}


/* ###### Reregistration ##################################################### */
bool doRegistration(struct RSerPoolSocket* rserpoolSocket,
                    bool                   waitForRegistrationResult)
{
   struct TagItem        tags[16];
   struct rsp_addrinfo*  rspAddrInfo;
   struct rsp_addrinfo*  rspAddrInfo2;
   struct rsp_addrinfo*  next;
   size_t                sctpLocalAddresses;
   union sockaddr_union* sctpLocalAddressArray = NULL;
   union sockaddr_union* localAddressArray     = NULL;
   struct sockaddr*      packedAddressArray    = NULL;
   union sockaddr_union  socketName;
   socklen_t             socketNameLen;
   unsigned int          localAddresses;
   unsigned int          result;
   unsigned int          i;

   CHECK(rserpoolSocket->PoolElement != NULL);

   /* ====== Create rsp_addrinfo structure ========================== */
   rspAddrInfo = (struct rsp_addrinfo*)malloc(sizeof(struct rsp_addrinfo));
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
      sctpLocalAddresses = getladdrsplus(rserpoolSocket->Socket, 0, &sctpLocalAddressArray);
      if(sctpLocalAddresses <= 0) {
         LOG_WARNING
         fputs("Unable to obtain socket's local addresses using sctp_getladdrs\n", stdlog);
         LOG_END
      }
      else {
         /* --- Check for buggy SCTP implementation ----- */
         if( (getPort(&sctpLocalAddressArray[0].sa) == 0)                  ||
             ( ((sctpLocalAddressArray[0].sa.sa_family == AF_INET) &&
                (sctpLocalAddressArray[0].in.sin_addr.s_addr == 0)) ||
             ( ((sctpLocalAddressArray[0].sa.sa_family == AF_INET6) &&
                (IN6_IS_ADDR_UNSPECIFIED(&sctpLocalAddressArray[0].in6.sin6_addr)))) ) ) {
            LOG_ERROR
            fputs("getladdrsplus() replied INADDR_ANY or port 0\n", stdlog);
            for(i = 0;i < rspAddrInfo->ai_addrs;i++) {
               fprintf(stdlog, "Address[%d] = ", i);
               fputaddress((struct sockaddr*)&rspAddrInfo->ai_addr[i], true, stdlog);
               fputs("\n", stdlog);
            }
            LOG_END_FATAL
         }
         /* --------------------------------------------- */

         packedAddressArray = pack_sockaddr_union(sctpLocalAddressArray, sctpLocalAddresses);
         if(packedAddressArray != NULL) {
            rspAddrInfo->ai_addr  = packedAddressArray;
            rspAddrInfo->ai_addrs = sctpLocalAddresses;
         }
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

      /* ====== SCTP: add addresses to rsp_addrinfo structure ======= */
      if(rserpoolSocket->SocketProtocol == IPPROTO_SCTP) {
         /* SCTP endpoints are described by a list of their
            transport addresses. */
         rspAddrInfo->ai_addrs = localAddresses;
         rspAddrInfo->ai_addr  = pack_sockaddr_union(localAddressArray, localAddresses);
         packedAddressArray    = rspAddrInfo->ai_addr;
      }

      /* ====== TCP/UDP: add own rsp_addrinfo for each address ====== */
      else {
         rspAddrInfo2 = rspAddrInfo;
         for(i = 0;i < localAddresses;i++) {
            rspAddrInfo2->ai_addrs = 1;
            rspAddrInfo2->ai_addr  = (struct sockaddr*)&localAddressArray[i];
            if(i < localAddresses - 1) {
               next = (struct rsp_addrinfo*)malloc(sizeof(struct rsp_addrinfo));
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
   tags[0].Tag  = TAG_UserTransport_HasControlChannel;
   tags[0].Data = (tagdata_t)rserpoolSocket->PoolElement->HasControlChannel;
   tags[1].Tag  = TAG_RspPERegistration_WaitForResult;
   tags[1].Data = (tagdata_t)waitForRegistrationResult;
   tags[2].Tag  = TAG_END;

   /* ====== Do registration ================================================ */
   result = rsp_pe_registration_tags(
               (unsigned char*)&rserpoolSocket->PoolElement->Handle.Handle,
               rserpoolSocket->PoolElement->Handle.Size,
               rspAddrInfo,
               &rserpoolSocket->PoolElement->LoadInfo,
               (unsigned int)(rserpoolSocket->PoolElement->RegistrationLife / 1000ULL),
               (struct TagItem*)&tags);
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
      free(sctpLocalAddressArray);
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
