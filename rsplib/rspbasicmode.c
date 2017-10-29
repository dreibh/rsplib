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
#include "rserpool-internals.h"
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

#include <stdio.h>


struct ASAPInstance*       gAsapInstance = NULL;
struct Dispatcher          gDispatcher;
static struct ThreadSafety gThreadSafety;
#ifdef ENABLE_CSP
struct CSPReporter*        gCSPReporter = NULL;
#endif

struct SimpleRedBlackTree  gRSerPoolSocketSet;
struct ThreadSafety        gRSerPoolSocketSetMutex;
struct IdentifierBitmap*   gRSerPoolSocketAllocationBitmap;


static void lock(struct Dispatcher* dispatcher, void* userData);
static void unlock(struct Dispatcher* dispatcher, void* userData);
static bool addStaticRegistrars(struct RegistrarTable* registrarTable,
                                struct rsp_info*       info);
#ifdef ENABLE_CSP
static size_t getComponentStatus(void*                         userData,
                                 uint64_t*                     identifier,
                                 struct ComponentAssociation** caeArray,
                                 char*                         statusText,
                                 char*                         componentAddress,
                                 double*                       workload);
extern size_t getSessionStatus(struct ComponentAssociation** caeArray,
                               uint64_t*                     identifier,
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
   struct rsp_info emptyinfo;
   struct TagItem  tagList[16];
   size_t          i;

   beginLogging();

   if(info == NULL) {
      memset(&emptyinfo, 0, sizeof(emptyinfo));
      info = &emptyinfo;
   }

   /* ====== Check for problems ========================================== */
   if(gAsapInstance != NULL) {
      LOG_WARNING
      fputs("rsplib is already initialized\n", stdlog);
      LOG_END
      return(0);
   }

   /* ====== Handle tuning parameters ==================================== */
   i = 0;
   if(info->ri_registrar_announce_timeout > 0) {
      tagList[i].Tag  = TAG_RspLib_RegistrarAnnounceTimeout;
      tagList[i].Data = (tagdata_t)1000 * (tagdata_t)info->ri_registrar_announce_timeout;
      i++;
   }
   if(info->ri_registrar_connect_timeout > 0) {
      tagList[i].Tag  = TAG_RspLib_RegistrarConnectTimeout;
      tagList[i].Data = (tagdata_t)1000 * (tagdata_t)info->ri_registrar_connect_timeout;
      i++;
   }
   if(info->ri_registrar_connect_max_trials > 0) {
      tagList[i].Tag  = TAG_RspLib_RegistrarConnectMaxTrials;
      tagList[i].Data = (tagdata_t)info->ri_registrar_connect_max_trials;
      i++;
   }
   if(info->ri_registrar_request_timeout > 0) {
      tagList[i].Tag  = TAG_RspLib_RegistrarRequestTimeout;
      tagList[i].Data = (tagdata_t)1000 * (tagdata_t)info->ri_registrar_request_timeout;
      i++;
   }
   if(info->ri_registrar_response_timeout > 0) {
      tagList[i].Tag  = TAG_RspLib_RegistrarResponseTimeout;
      tagList[i].Data = (tagdata_t)1000 * (tagdata_t)info->ri_registrar_response_timeout;
      i++;
   }
   if(info->ri_registrar_request_max_trials > 0) {
      tagList[i].Tag  = TAG_RspLib_RegistrarRequestMaxTrials;
      tagList[i].Data = (tagdata_t)info->ri_registrar_request_max_trials;
      i++;
   }
   tagList[i].Tag = TAG_DONE;

   /* ====== Initialize ASAP instance ==================================== */
   threadSafetyNew(&gThreadSafety, "RsplibInstance");
   threadSafetyNew(&gRSerPoolSocketSetMutex, "gRSerPoolSocketSet");
   dispatcherNew(&gDispatcher, lock, unlock, NULL);
   gAsapInstance = asapInstanceNew(&gDispatcher,
                                   (info->ri_disable_autoconfig == 0),
                                   (union sockaddr_union*)info->ri_registrar_announce,
                                   (struct TagItem*)&tagList);
   if(gAsapInstance) {
      if(info) {
         info->ri_version    = RSPLIB_VERSION;
         info->ri_revision   = RSPLIB_REVISION;
         info->ri_build_date = NULL;
         info->ri_build_time = NULL;
      }

      /* ====== Initialize session storage =============================== */
      simpleRedBlackTreeNew(&gRSerPoolSocketSet,
                            rserpoolSocketPrint,
                            rserpoolSocketComparison);

      /* ====== Initialize RSerPool Socket descriptor storage ============ */
      gRSerPoolSocketAllocationBitmap = identifierBitmapNew(FD_SETSIZE);
      if(gRSerPoolSocketAllocationBitmap != NULL) {
         /* ====== Map stdin, stdout, stderr file descriptors ============ */
         CHECK(rsp_mapsocket(STDOUT_FILENO, STDOUT_FILENO) == STDOUT_FILENO);
         CHECK(rsp_mapsocket(STDIN_FILENO,  STDIN_FILENO)  == STDIN_FILENO);
         CHECK(rsp_mapsocket(STDERR_FILENO, STDERR_FILENO) == STDERR_FILENO);

         /* ====== Add static registrars ================================= */
         if( (info == NULL) ||
             (addStaticRegistrars(gAsapInstance->RegistrarSet, info)) ) {

#ifdef ENABLE_CSP
            /* ====== Initialize Component Status Reporter =============== */
            if((info) &&
               (info->ri_csp_interval > 0) && (info->ri_csp_server != NULL)) {
               gCSPReporter = (struct CSPReporter*)malloc(sizeof(struct CSPReporter));
               if(gCSPReporter) {
                  cspReporterNew(gCSPReporter, &gDispatcher,
                                 info->ri_csp_identifier,
                                 info->ri_csp_server,
                                 1000UL * info->ri_csp_interval,
                                 getComponentStatus,
                                 NULL);
               }
            }
#endif

            /* ====== Start the main loop thread ========================= */
            if(asapInstanceStartThread(gAsapInstance)) {
               LOG_NOTE
               fputs("rsplib is ready\n", stdlog);
               LOG_END
               return(0);
            }
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
      /* ====== Clean-up RSerPool Socket descriptor storage ============== */
      CHECK(rsp_unmapsocket(STDOUT_FILENO) == 0);
      CHECK(rsp_unmapsocket(STDIN_FILENO) == 0);
      CHECK(rsp_unmapsocket(STDERR_FILENO) == 0);
      for(i = 1;i < (int)FD_SETSIZE;i++) {
         if(identifierBitmapAllocateSpecificID(gRSerPoolSocketAllocationBitmap, i) < 0) {
            LOG_WARNING
            fprintf(stdlog, "RSerPool socket %d is still registered -> closing it\n", i);
            LOG_END
            rsp_close(i);
         }
      }

      /* ====== Clean-up ASAP Instance, CSP Reported and Dispatcher ====== */
      asapInstanceDelete(gAsapInstance);
      gAsapInstance = NULL;

      /* ====== Remove Component Status Reporter ========================= */
#ifdef ENABLE_CSP
      if(gCSPReporter) {
         cspReporterDelete(gCSPReporter);
         free(gCSPReporter);
         gCSPReporter = NULL;
      }
#endif

      /* ====== Remove Dispatcher ======================================== */
      dispatcherDelete(&gDispatcher);
      threadSafetyDelete(&gRSerPoolSocketSetMutex);
      threadSafetyDelete(&gThreadSafety);

      /* ====== Remove socket set ======================================== */
      simpleRedBlackTreeDelete(&gRSerPoolSocketSet);
      identifierBitmapDelete(gRSerPoolSocketAllocationBitmap);
      gRSerPoolSocketAllocationBitmap = NULL;

   }
   finishLogging();
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
                                  registrarInfo->rri_addrs, MAX_PE_TRANSPORTADDRESSES);
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


/* ###### Convert pool element node to rsp_addrinfo structure ############ */
static int poolElementNodeToAddrInfo(const struct ST_CLASS(PoolElementNode)* poolElementNode,
                                     struct rsp_addrinfo**                   rspAddrInfo)
{
   int    result = REAI_MEMORY;
   char*  ptr;
   size_t i;

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
                  result = REAI_FAMILY;
                break;
            }
         }
      }
      else {
         free(*rspAddrInfo);
         *rspAddrInfo = NULL;
         result = REAI_MEMORY;
      }
   }
   return(result);
}


// ###### Conversion function from PoolElementNode to rsp_addrinfo ####### */
static unsigned int convertPoolElementNode(
                       const struct ST_CLASS(PoolElementNode)* poolElementNode,
                       void*                                   ptr)
{
   return(poolElementNodeToAddrInfo(poolElementNode, (struct rsp_addrinfo**)ptr));
}


/* ###### Handle resolution ############################################## */
int rsp_getaddrinfo_tags(const unsigned char*  poolHandle,
                         const size_t          poolHandleSize,
                         struct rsp_addrinfo** rspAddrInfo,
                         const size_t          items,
                         const unsigned int    staleCacheValue,
                         struct TagItem*       tags)
{
   struct PoolHandle myPoolHandle;
   void*             addrInfoArray[MAX_MAX_HANDLE_RESOLUTION_ITEMS];
   size_t            addrInfos;
   unsigned int      hresResult;
   int               result;
   size_t            n;

   *rspAddrInfo = NULL;
   if(gAsapInstance) {
      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);
      addrInfos = max(1, min((size_t)items, MAX_MAX_HANDLE_RESOLUTION_ITEMS));

      hresResult = asapInstanceHandleResolution(
                      gAsapInstance,
                      &myPoolHandle,
                      (void**)&addrInfoArray,
                      &addrInfos,
                      convertPoolElementNode,
                      1000ULL * staleCacheValue);
      if(hresResult == RSPERR_OKAY) {
         if(addrInfos > 0) {
            for(n = 0;n < addrInfos - 1;n++) {
               ((struct rsp_addrinfo*)addrInfoArray[n])->ai_next =
                  (struct rsp_addrinfo*)addrInfoArray[n + 1];
            }
            *rspAddrInfo = (struct rsp_addrinfo*)addrInfoArray[0];
         }
         return(addrInfos);
      }
      else {
         if(hresResult == RSPERR_NOT_FOUND) {
            result = REAI_NONAME;
         }
         else {
            result = REAI_SYSTEM;
         }
      }
   }
   else {
      LOG_ERROR
      fputs("rsplib is not initialized\n", stdlog);
      LOG_END
      result = REAI_SYSTEM;
   }

   return(result);
}


/* ###### Handle resolution ############################################## */
int rsp_getaddrinfo(const unsigned char*  poolHandle,
                    const size_t          poolHandleSize,
                    struct rsp_addrinfo** rspAddrInfo,
                    const size_t          items,
                    const unsigned int    staleCacheValue)
{
   return(rsp_getaddrinfo_tags(poolHandle, poolHandleSize, rspAddrInfo, items, staleCacheValue, NULL));
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
unsigned int rsp_pe_registration_tags(const unsigned char*       poolHandle,
                                      const size_t               poolHandleSize,
                                      struct rsp_addrinfo*       rspAddrInfo,
                                      const struct rsp_loadinfo* rspLoadInfo,
                                      const unsigned int         registrationLife,
                                      const int                  flags,
                                      struct TagItem*            tags)
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
      myPolicySettings.WeightDPF       = rspLoadInfo->rli_weight_dpf;
      myPolicySettings.Load            = rspLoadInfo->rli_load;
      myPolicySettings.LoadDegradation = rspLoadInfo->rli_load_degradation;
      myPolicySettings.LoadDPF         = rspLoadInfo->rli_load_dpf;

      unpackedAddrs = unpack_sockaddr(rspAddrInfo->ai_addr, rspAddrInfo->ai_addrs);
      if(unpackedAddrs != NULL) {
         transportAddressBlockNew(myTransportAddressBlock,
                                  rspAddrInfo->ai_protocol,
                                  getPort((struct sockaddr*)rspAddrInfo->ai_addr),
                                  (flags & REGF_CONTROLCHANNEL) ? TABF_CONTROLCHANNEL : 0,
                                  unpackedAddrs,
                                  rspAddrInfo->ai_addrs, MAX_PE_TRANSPORTADDRESSES);
         ST_CLASS(poolElementNodeNew)(
            &myPoolElementNode,
            rspAddrInfo->ai_pe_id,
            gAsapInstance->RegistrarIdentifier,
            registrationLife,
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
                                       !(flags & REGF_DONTWAIT), (flags & REGF_DAEMONMODE));
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
unsigned int rsp_pe_registration(const unsigned char*       poolHandle,
                                 const size_t               poolHandleSize,
                                 struct rsp_addrinfo*       rspAddrInfo,
                                 const struct rsp_loadinfo* rspLoadInfo,
                                 const unsigned int         registrationLife,
                                 const int                  flags)
{
   return(rsp_pe_registration_tags(poolHandle, poolHandleSize,
                                   rspAddrInfo, rspLoadInfo, registrationLife,
                                   flags, NULL));
}


/* ###### Deregister pool element ######################################## */
unsigned int rsp_pe_deregistration_tags(const unsigned char* poolHandle,
                                        const size_t         poolHandleSize,
                                        const uint32_t       identifier,
                                        const int            flags,
                                        struct TagItem*      tags)
{
   struct PoolHandle myPoolHandle;
   unsigned int      result;

   if(gAsapInstance) {
      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);
      result = asapInstanceDeregister(gAsapInstance, &myPoolHandle, identifier,
                                      !(flags & DEREGF_DONTWAIT));
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
                                   const uint32_t       identifier,
                                   const int            flags)
{
   return(rsp_pe_deregistration_tags(poolHandle, poolHandleSize, identifier,
                                     flags, NULL));
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


/* ###### Get policy name by type number ################################# */
const char* rsp_getpolicybytype(unsigned int policyType)
{
   const struct ST_CLASS(PoolPolicy)* poolPolicy = ST_CLASS(poolPolicyGetPoolPolicyByType)(policyType);
   if(poolPolicy) {
      return(poolPolicy->Name);
   }
   return(NULL);
}


/* ###### Get policy type by name ######################################## */
unsigned int rsp_getpolicybyname(const char* policyName)
{
   const struct ST_CLASS(PoolPolicy)* poolPolicy = ST_CLASS(poolPolicyGetPoolPolicyByName)(policyName);
   if(poolPolicy) {
      return(poolPolicy->Type);
   }
   return(PPT_UNDEFINED);
}


#ifdef ENABLE_CSP
/* ###### Get PE/PU status ############################################### */
static size_t getComponentStatus(void*                         userData,
                                 uint64_t*                     identifier,
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
