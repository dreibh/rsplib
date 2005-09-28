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
#include "leaflinkedredblacktree.h"
#include "loglevel.h"
#include "debug.h"
#ifdef ENABLE_CSP
#include "componentstatusreporter.h"
#endif


struct ASAPInstance*          gAsapInstance = NULL;
struct Dispatcher             gDispatcher;
static struct ThreadSafety    gThreadSafety;
#ifdef ENABLE_CSP
static struct CSPReporter*    gCSPReporter  = NULL;
#endif

struct LeafLinkedRedBlackTree gRSerPoolSocketSet;
struct ThreadSafety           gRSerPoolSocketSetMutex;
struct IdentifierBitmap*      gRSerPoolSocketAllocationBitmap;


static void lock(struct Dispatcher* dispatcher, void* userData);
static void unlock(struct Dispatcher* dispatcher, void* userData);
static bool addStaticRegistrars(struct RegistrarTable* registrarTable,
                                struct rsp_info*       info);
#ifdef ENABLE_CSP
static size_t getComponentStatus(void*                         userData,
                                 unsigned long long*           identifier,
                                 struct ComponentAssociation** caeArray,
                                 char*                         statusText,
                                 char*                         componentAddress,
                                 double*                       workload);
extern size_t getSessionStatus(struct ComponentAssociation** caeArray,
                               unsigned long long*           identifier,
                               char*                         statusText,
                               char*                         componentAddress,
                               double*                       workload,
                               const int                     registrarSocket,
                               const RegistrarIdentifierType registrarID,
                               const unsigned long long      registrarConnectionTimeStamp);
#endif


/* ###### Initialize RSerPool API Library ################################ */
int rsp_initialize(struct rsp_info* info)
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
   gAsapInstance = asapInstanceNew(&gDispatcher, NULL);
   if(gAsapInstance) {
      if(info) {
         info->ri_version    = RSPLIB_VERSION;
         info->ri_revision   = RSPLIB_REVISION;
         info->ri_build_date = buildDate;
         info->ri_build_time = buildTime;
      }

#ifdef ENABLE_CSP
      /* ====== Initialize Component Status Reporter ===================== */
      if((info) &&
         (info->ri_csp_interval > 0) && (info->ri_csp_server != NULL)) {
         gCSPReporter = (struct CSPReporter*)malloc(sizeof(struct CSPReporter));
         if(gCSPReporter) {
            cspReporterNew(gCSPReporter, &gDispatcher,
                           info->ri_csp_identifier,
                           info->ri_csp_server,
                           info->ri_csp_interval,
                           getComponentStatus,
                           NULL);
         }
      }
#endif

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

         /* ====== Add static registrars ================================= */
         if( (info == NULL) ||
	     (addStaticRegistrars(gAsapInstance->RegistrarSet, info)) ) {
            LOG_NOTE
            fputs("rsplib is ready\n", stdlog);
            LOG_END
            return(0);
         }
         else {
            LOG_ERROR
            fputs("Failed to add static registrars\n", stdlog);
            LOG_END
            return(-1);
         }
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
#ifdef ENABLE_CSP
      /* ====== Remove Component Status Reporter ========================= */
      if(gCSPReporter) {
         cspReporterDelete(gCSPReporter);
         gCSPReporter = NULL;
      }
#endif

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
      /* ??? usleep(250000); */
#endif
   }
}


/* ###### Add list of static registrars ################################## */
static bool addStaticRegistrars(struct RegistrarTable* registrarTable,
                                struct rsp_info*       info)
{
   char                          transportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* transportAddressBlock = (struct TransportAddressBlock*)&transportAddressBlockBuffer;
   union sockaddr_union*         addressArray;
   struct rsp_registrar_info*    registrarInfo;
   int                           result;

   registrarInfo = info->ri_registrars;
   result        = true;
   while((registrarInfo) && (result == true)) {

      addressArray = unpack_sockaddr(registrarInfo->rri_addr, registrarInfo->rri_addrs);
      if(addressArray) {
         transportAddressBlockNew(transportAddressBlock,
                                  IPPROTO_SCTP,
                                  getPort((struct sockaddr*)&addressArray[0]),
                                  0,
                                  addressArray,
                                  registrarInfo->rri_addrs);
         result = (registrarTableAddStaticEntry(registrarTable, transportAddressBlock) == RSPERR_OKAY);
         free(addressArray);
      }
      else {
         LOG_ERROR
         fputs("Unpacking sockaddr array failed\n", stdlog);
         LOG_END
         result = false;
      }

      registrarInfo = registrarInfo->rri_next;
   }
   return(result);
}



/* ###### Handle resolution ############################################## */
int rsp_getaddrinfo_tags(const unsigned char*  poolHandle,
                         const size_t          poolHandleSize,
                         struct rsp_addrinfo** rspAddrInfo,
                         struct TagItem*       tags)
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
         *rspAddrInfo = (struct rsp_addrinfo*)malloc(sizeof(struct rsp_addrinfo));
         if(rspAddrInfo != NULL) {
            (*rspAddrInfo)->ai_next     = NULL;
            (*rspAddrInfo)->ai_pe_id    = poolElementNode->Identifier;
            (*rspAddrInfo)->ai_family   = poolElementNode->UserTransport->AddressArray[0].sa.sa_family;
            (*rspAddrInfo)->ai_protocol = poolElementNode->UserTransport->Protocol;
            switch(poolElementNode->UserTransport->Protocol) {
               case IPPROTO_SCTP:
                  (*rspAddrInfo)->ai_socktype = SOCK_STREAM;
                break;
               case IPPROTO_TCP:
                  (*rspAddrInfo)->ai_socktype = SOCK_STREAM;
                break;
               default:
                  (*rspAddrInfo)->ai_socktype = SOCK_DGRAM;
                break;
            }
            (*rspAddrInfo)->ai_addrlen = sizeof(union sockaddr_union);
            (*rspAddrInfo)->ai_addrs   = poolElementNode->UserTransport->Addresses;
            (*rspAddrInfo)->ai_addr    = (struct sockaddr*)malloc((*rspAddrInfo)->ai_addrs * sizeof(union sockaddr_union));
            if((*rspAddrInfo)->ai_addr != NULL) {
               result = 0;
               ptr = (char*)(*rspAddrInfo)->ai_addr;
               for(i = 0;i < poolElementNode->UserTransport->Addresses;i++) {
                  memcpy((void*)ptr, (void*)&poolElementNode->UserTransport->AddressArray[i],
                         sizeof(union sockaddr_union));
                  if (poolElementNode->UserTransport->AddressArray[i].sa.sa_family == AF_INET6)
                    (*rspAddrInfo)->ai_family = AF_INET6;
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
               free(*rspAddrInfo);
               *rspAddrInfo = NULL;
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


/* ###### Handle resolution ############################################## */
int rsp_getaddrinfo(const unsigned char*  poolHandle,
                    const size_t          poolHandleSize,
                    struct rsp_addrinfo** rspAddrInfo)
{
   return(rsp_getaddrinfo_tags(poolHandle, poolHandleSize, rspAddrInfo, NULL));
}


/* ###### Free endpoint address array #################################### */
void rsp_freeaddrinfo(struct rsp_addrinfo* rspAddrInfo)
{
   struct rsp_addrinfo* next;

   while(rspAddrInfo != NULL) {
      next = rspAddrInfo->ai_next;

      if(rspAddrInfo->ai_addr) {
         free(rspAddrInfo->ai_addr);
         rspAddrInfo->ai_addr = NULL;
      }
      free(rspAddrInfo);

      rspAddrInfo = next;
   }
}


/* ###### Register pool element ########################################## */
unsigned int rsp_pe_registration_tags(const unsigned char* poolHandle,
                                      const size_t         poolHandleSize,
                                      struct rsp_addrinfo* rspAddrInfo,
                                      struct rsp_loadinfo* rspLoadInfo,
                                      unsigned int         registrationLife,
                                      struct TagItem*      tags)
{
   struct PoolHandle                myPoolHandle;
   struct ST_CLASS(PoolElementNode) myPoolElementNode;
   struct PoolPolicySettings        myPolicySettings;
   char                             myTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*    myTransportAddressBlock = (struct TransportAddressBlock*)&myTransportAddressBlockBuffer;
   union sockaddr_union*            unpackedAddrs;
   unsigned int                     result;

   if(gAsapInstance) {
      if(rspAddrInfo->ai_pe_id == UNDEFINED_POOL_ELEMENT_IDENTIFIER) {
         rspAddrInfo->ai_pe_id = getPoolElementIdentifier();
      }

      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);

      poolPolicySettingsNew(&myPolicySettings);
      myPolicySettings.PolicyType      = rspLoadInfo->rli_policy;
      myPolicySettings.Weight          = rspLoadInfo->rli_weight;
      myPolicySettings.Load            = rspLoadInfo->rli_load;
      myPolicySettings.LoadDegradation = rspLoadInfo->rli_load_degradation;

      unpackedAddrs = unpack_sockaddr(rspAddrInfo->ai_addr, rspAddrInfo->ai_addrs);
      if(unpackedAddrs != NULL) {
         transportAddressBlockNew(myTransportAddressBlock,
                                 rspAddrInfo->ai_protocol,
                                 getPort((struct sockaddr*)rspAddrInfo->ai_addr),
                                 (tagListGetData(tags, TAG_UserTransport_HasControlChannel, 0) != 0) ? TABF_CONTROLCHANNEL : 0,
                                 unpackedAddrs,
                                 rspAddrInfo->ai_addrs);

         ST_CLASS(poolElementNodeNew)(
            &myPoolElementNode,
            rspAddrInfo->ai_pe_id,
            0,
            1000ULL * registrationLife,
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

         result = asapInstanceRegister(gAsapInstance, &myPoolHandle, &myPoolElementNode,
                     (bool)tagListGetData(tags, TAG_RspPERegistration_WaitForResult, (tagdata_t)true));
         if(result != RSPERR_OKAY) {
            rspAddrInfo->ai_pe_id = UNDEFINED_POOL_ELEMENT_IDENTIFIER;
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


/* ###### Register pool element ########################################## */
unsigned int rsp_pe_registration(const unsigned char* poolHandle,
                                 const size_t         poolHandleSize,
                                 struct rsp_addrinfo* rspAddrInfo,
                                 struct rsp_loadinfo* rspLoadInfo,
                                 unsigned int         registrationLife)
{
   return(rsp_pe_registration_tags(poolHandle, poolHandleSize,
                                   rspAddrInfo, rspLoadInfo, registrationLife, NULL));
}


/* ###### Deregister pool element ######################################## */
unsigned int rsp_pe_deregistration_tags(const unsigned char* poolHandle,
                                        const size_t         poolHandleSize,
                                        const uint32_t       identifier,
                                        struct TagItem*      tags)
{
   struct PoolHandle myPoolHandle;
   unsigned int      result;

   if(gAsapInstance) {
      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);
      result = asapInstanceDeregister(gAsapInstance, &myPoolHandle, identifier,
                  (bool)tagListGetData(tags, TAG_RspPEDeregistration_WaitForResult, (tagdata_t)true));
   }
   else {
      result = RSPERR_NOT_INITIALIZED;
      LOG_ERROR
      fputs("rsplib is not initialized\n", stdlog);
      LOG_END
   }
   return(result);
}


/* ###### Deregister pool element ######################################## */
unsigned int rsp_pe_deregistration(const unsigned char* poolHandle,
                                   const size_t         poolHandleSize,
                                   const uint32_t       identifier)
{
   return(rsp_pe_deregistration_tags(poolHandle, poolHandleSize, identifier, NULL));
}


/* ###### Report pool element failure #################################### */
unsigned int rsp_pe_failure_tags(const unsigned char* poolHandle,
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


/* ###### Report pool element failure #################################### */
unsigned int rsp_pe_failure(const unsigned char* poolHandle,
                            const size_t         poolHandleSize,
                            const uint32_t       identifier)
{
   return(rsp_pe_failure_tags(poolHandle, poolHandleSize, identifier, NULL));
}


#ifdef ENABLE_CSP
/* ###### Get PE/PU status ############################################### */
static size_t getComponentStatus(void*                         userData,
                                 unsigned long long*           identifier,
                                 struct ComponentAssociation** caeArray,
                                 char*                         statusText,
                                 char*                         componentAddress,
                                 double*                       workload)
{
   return(getSessionStatus(caeArray,
                           identifier,
                           statusText,
                           componentAddress,
                           workload,
                           gAsapInstance->RegistrarSocket,
                           gAsapInstance->RegistrarIdentifier,
                           gAsapInstance->RegistrarConnectionTimeStamp));
}
#endif
