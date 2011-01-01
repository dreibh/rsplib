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
 * Copyright (C) 2002-2011 by Thomas Dreibholz
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
   rserpoolSocket = (struct RSerPoolSocket*)simpleRedBlackTreeFind(&gRSerPoolSocketSet,
                                                                   &cmpSocket.Node);
   threadSafetyUnlock(&gRSerPoolSocketSetMutex);
   if(rserpoolSocket == NULL) {
      LOG_ERROR
      fprintf(stdlog, "Bad RSerPool socket descriptor %d\n", sd);
      LOG_END_FATAL
   }
   return(rserpoolSocket);
}


/* ###### Wait until there is something to read ########################## */
bool waitForRead(struct RSerPoolSocket* rserpoolSocket,
                 int                    timeout)
{
   struct pollfd ufds[1];
   ufds[0].fd     = rserpoolSocket->Descriptor;
   ufds[0].events = POLLIN;
   int result = rsp_poll((struct pollfd*)&ufds, 1, timeout);
   if((result > 0) && (ufds[0].revents & POLLIN)) {
      return(true);
   }
   errno = EAGAIN;
   return(false);
}


/* ###### Delete pool element ############################################ */
void deletePoolElement(struct PoolElement* poolElement,
                       int                 flags,
                       struct TagItem*     tags)
{
   int result;

   /* Delete timer first; this ensures that the doRegistration() function
      will not be called while the PE is going to be deleted! */
   timerDelete(&poolElement->ReregistrationTimer);

   threadSafetyLock(&poolElement->Mutex);
   if(poolElement->Identifier != 0x00000000) {
      result = rsp_pe_deregistration_tags((unsigned char*)&poolElement->Handle.Handle,
                                          poolElement->Handle.Size,
                                          poolElement->Identifier,
                                          flags, tags);
      if(result != RSPERR_OKAY) {
         LOG_ERROR
         fprintf(stdlog, "Deregistration failed: ");
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
   }

   threadSafetyUnlock(&poolElement->Mutex);
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
      rserpoolSocket->Mutex is not necessary here. It is only necessary to
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
   struct rsp_addrinfo*  rspAddrInfo;
   struct rsp_addrinfo*  rspAddrInfo2;
   struct rsp_addrinfo*  next;
   size_t                oldSctpLocalAddresses;
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
         if( (getPort(&sctpLocalAddressArray[0].sa) == 0)           ||
             ( ((sctpLocalAddressArray[0].sa.sa_family == AF_INET) &&
                (sctpLocalAddressArray[0].in.sin_addr.s_addr == 0)) ||
             ( ((sctpLocalAddressArray[0].sa.sa_family == AF_INET6) &&
                (IN6_IS_ADDR_UNSPECIFIED(&sctpLocalAddressArray[0].in6.sin6_addr)))) ) ) {
            LOG_ERROR
            fputs("getladdrsplus() replied INADDR_ANY or port 0\n", stdlog);
            LOG_END_FATAL
         }
         /* --------------------------------------------- */

         /* ====== Filter out link-local addresses ======================= */
         oldSctpLocalAddresses = sctpLocalAddresses;
         sctpLocalAddresses = filterAddressesByScope(sctpLocalAddressArray, sctpLocalAddresses,
                                                     AS_UNICAST_SITELOCAL);
         if(sctpLocalAddresses == 0) {
            LOG_ERROR
            fputs("No addresses of at least site-local scope found -> No registration possible. Addresses:\n", stdlog);
            for(i = 0; i < oldSctpLocalAddresses;i++) {
               fputs(" - ", stdlog);
               fputaddress(&sctpLocalAddressArray[i].sa, true, stdlog);
               fputs("\n", stdlog);
            }
            LOG_END
            return(false);
         }

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

      /* ====== Filter out link-local addresses ========================= */
      localAddresses = filterAddressesByScope(localAddressArray, localAddresses,
                                              AS_UNICAST_SITELOCAL);
      if(localAddresses == 0) {
         LOG_ERROR
         fputs("No addresses of at least site-local scope available -> No registration possible\n", stdlog);
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
   int flags = 0;
   if(waitForRegistrationResult == false) {
      flags |= REGF_DONTWAIT;
   }
   if(rserpoolSocket->PoolElement->HasControlChannel) {
      flags |= REGF_CONTROLCHANNEL;
   }
   if(rserpoolSocket->PoolElement->InDaemonMode) {
      flags |= REGF_DAEMONMODE;
   }

   /* ====== Do registration ================================================ */
   result = rsp_pe_registration(
               (unsigned char*)&rserpoolSocket->PoolElement->Handle.Handle,
               rserpoolSocket->PoolElement->Handle.Size,
               rspAddrInfo,
               &rserpoolSocket->PoolElement->LoadInfo,
               rserpoolSocket->PoolElement->RegistrationLife,
               flags);
   /* Keep identifier chosen by rsp_pe_registration()! */
   rserpoolSocket->PoolElement->Identifier = rspAddrInfo->ai_pe_id;
   if(result == RSPERR_OKAY) {
      LOG_VERBOSE2
      fprintf(stdlog, "(Re-)Registration successful, ID is $%08x\n",
              rserpoolSocket->PoolElement->Identifier);
      LOG_END
/*
      printf("(Re-)Registration successful, ID is $%08x\n",
             rserpoolSocket->PoolElement->Identifier);
*/
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
