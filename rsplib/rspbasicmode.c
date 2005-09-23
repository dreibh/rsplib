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
#include "rserpoolsocket.h"
#include "dispatcher.h"
#include "identifierbitmap.h"
#include "netutilities.h"
#include "threadsafety.h"
#include "rserpoolmessage.h"
#include "asapinstance.h"
#include "componentstatusprotocol.h"
#include "leaflinkedredblacktree.h"
#include "loglevel.h"
#include "debug.h"


struct ASAPInstance*          gAsapInstance = NULL;
struct Dispatcher             gDispatcher;
static struct ThreadSafety    gThreadSafety;

struct LeafLinkedRedBlackTree gRSerPoolSocketSet;
struct ThreadSafety           gRSerPoolSocketSetMutex;
struct IdentifierBitmap*      gRSerPoolSocketAllocationBitmap;


static void lock(struct Dispatcher* dispatcher, void* userData);
static void unlock(struct Dispatcher* dispatcher, void* userData);
extern size_t rsp_csp_getsessionstatus(struct ComponentAssociationEntry** caeArray,
                                       char*                              statusText,
                                       char*                              componentAddress,
                                       const int                          registrarSocket,
                                       const RegistrarIdentifierType      registrarID,
                                       const unsigned long long           registrarConnectionTimeStamp);


/* ###### Initialize RSerPool API Library ################################ */
int rsp_initialize(struct rserpool_info* info, struct TagItem* tags)
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
   threadSafetyNew(&gThreadSafety, "RsplibInstance");
   threadSafetyNew(&gRSerPoolSocketSetMutex, "gRSerPoolSocketSet");
   dispatcherNew(&gDispatcher, lock, unlock, NULL);
   gAsapInstance = asapInstanceNew(&gDispatcher, tags);
   if(gAsapInstance) {
      if(info) {
         info->ri_version    = RSPLIB_VERSION;
         info->ri_revision   = RSPLIB_REVISION;
         info->ri_build_date = buildDate;
         info->ri_build_time = buildTime;
      }

      /* ====== Initialize session storage =============================== */
      leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet,
                                rserpoolSocketPrint,
                                rserpoolSocketComparison);

      /* ====== Initialize RSerPool Socket descriptor storage ============ */
      leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);
      leafLinkedRedBlackTreeNew(&gRSerPoolSocketSet, rserpoolSocketPrint, rserpoolSocketComparison);
      gRSerPoolSocketAllocationBitmap = identifierBitmapNew(FD_SETSIZE);
      if(gRSerPoolSocketAllocationBitmap != NULL) {
         /* ====== Map stdin, stdout, stderr file descriptors ============ */
         CHECK(rsp_mapsocket(STDOUT_FILENO, STDOUT_FILENO) == STDOUT_FILENO);
         CHECK(rsp_mapsocket(STDIN_FILENO, STDIN_FILENO) == STDIN_FILENO);
         CHECK(rsp_mapsocket(STDERR_FILENO, STDERR_FILENO) == STDERR_FILENO);

         LOG_NOTE
         fputs("rsplib is ready\n", stdlog);
         LOG_END
         return(0);
      }
      asapInstanceDelete(gAsapInstance);
      gAsapInstance = NULL;
      dispatcherDelete(&gDispatcher);
   }
   return(-1);
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


/* ###### Clean-up RSerPool API Library ################################## */
void rsp_cleanup()
{
   int i;

   if(gAsapInstance) {
      /* ====== Clean-up RSerPool Socket descriptor storage ============== */
      CHECK(rsp_unmapsocket(STDOUT_FILENO) == 0);
      CHECK(rsp_unmapsocket(STDIN_FILENO) == 0);
      CHECK(rsp_unmapsocket(STDERR_FILENO) == 0);
      for(i = 1;i < FD_SETSIZE;i++) {
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
      threadSafetyDelete(&gRSerPoolSocketSetMutex);
      threadSafetyDelete(&gThreadSafety);
#ifndef HAVE_KERNEL_SCTP
      /* Finally, give sctplib some time to cleanly shut down associations */
      usleep(250000);
#endif
   }
}


/* ###### Handle resolution ############################################## */
int rsp_getaddrinfo(const unsigned char*       poolHandle,
                    const size_t               poolHandleSize,
                    struct rserpool_addrinfo** rserpoolAddrInfo,
                    struct TagItem*            tags)
{
   struct PoolHandle                 myPoolHandle;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            poolElementNodes;
   unsigned int                      hresResult;
   int                               result;
   char*                             ptr;
   size_t                            i;

   if(gAsapInstance) {
      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);

      poolElementNodes = 1;
      hresResult = asapInstanceHandleResolution(
                      gAsapInstance,
                      &myPoolHandle,
                      (struct ST_CLASS(PoolElementNode)**)&poolElementNode,
                      &poolElementNodes);
      if(hresResult == RSPERR_OKAY) {
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
               result = 0;
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
                        result = EAI_ADDRFAMILY;
                      break;
                  }
               }
            }
            else {
               free(*rserpoolAddrInfo);
               *rserpoolAddrInfo = NULL;
               result = EAI_MEMORY;
            }
         }
         else {
            result = EAI_MEMORY;
         }
      }
      else {
         if(hresResult == RSPERR_NOT_FOUND) {
            result = EAI_NONAME;
         }
         else {
            result = EAI_SYSTEM;
         }
      }
   }
   else {
      LOG_ERROR
      fputs("rsplib is not initialized\n", stdlog);
      LOG_END
      result = EAI_SYSTEM;
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

         result = asapInstanceRegister(gAsapInstance, &myPoolHandle, &myPoolElementNode, true);
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
      result = asapInstanceDeregister(gAsapInstance, &myPoolHandle, identifier, true);
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


/* ###### Get PE/PU status ############################################### */
size_t rsp_csp_getcomponentstatus(struct ComponentAssociationEntry** caeArray,
                                  char*                              statusText,
                                  char*                              componentAddress)
{
   return(rsp_csp_getsessionstatus(caeArray,
                                   statusText,
                                   componentAddress,
                                   gAsapInstance->RegistrarSocket,
                                   gAsapInstance->RegistrarIdentifier,
                                   gAsapInstance->RegistrarConnectionTimeStamp));
}
