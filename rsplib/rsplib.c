/*
 *  $Id: rsplib.c,v 1.4 2004/07/20 15:35:15 dreibh Exp $
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
 * Purpose: RSerPool Library
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "rsplib.h"
#include "asapinstance.h"
#include "netutilities.h"
#include "threadsafety.h"

#include <ext_socket.h>


#define MAX_ADDRESSES_PER_ENDPOINT 128


struct Dispatcher*          gDispatcher   = NULL;
static struct ASAPInstance* gAsapInstance = NULL;
static unsigned int         gLastError    = RSPERR_OKAY;
static struct ThreadSafety  gThreadSafety;


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


/* ###### Initialize library ############################################# */
int rspInitialize(struct TagItem* tags)
{
   static const char* buildDate = __DATE__;
   static const char* buildTime = __TIME__;

   threadSafetyInit(&gThreadSafety, "RsplibInstance");
   gDispatcher = dispatcherNew(lock, unlock, NULL);
   if(gDispatcher) {
      gAsapInstance = asapInstanceNew(gDispatcher, tags);
      if(gAsapInstance) {
         tagListSetData(tags, TAG_RspLib_GetVersion,   (tagdata_t)RSPLIB_VERSION);
         tagListSetData(tags, TAG_RspLib_GetRevision,  (tagdata_t)RSPLIB_REVISION);
         tagListSetData(tags, TAG_RspLib_GetBuildDate, (tagdata_t)buildDate);
         tagListSetData(tags, TAG_RspLib_GetBuildTime, (tagdata_t)buildTime);
         tagListSetData(tags, TAG_RspLib_IsThreadSafe, (tagdata_t)threadSafetyIsAvailable());
         return(0);
      }
      else {
         dispatcherDelete(gDispatcher);
         gDispatcher = NULL;
      }
   }

   return(ENOMEM);
}


/* ###### Clean-up library ############################################### */
void rspCleanUp()
{
   if(gAsapInstance) {
      asapInstanceDelete(gAsapInstance);
      dispatcherDelete(gDispatcher);
      threadSafetyDestroy(&gThreadSafety);
      gAsapInstance = NULL;
      gDispatcher   = NULL;

      /* Give sctplib some time to cleanly shut down all associations */
      usleep(250000);
   }
}


/* ###### Get last error ################################################# */
unsigned int rspGetLastError()
{
   return((unsigned int)gLastError);
}


/* ###### Get last error description ##################################### */
const char* rspGetLastErrorDescription()
{
   return(rserpoolErrorGetDescription(gLastError));
}


/* ###### Register pool element ########################################## */
unsigned int rspRegister(const unsigned char*        poolHandle,
                         const size_t                poolHandleSize,
                         struct EndpointAddressInfo* endpointAddressInfo,
                         struct TagItem*             tags)
{
   struct PoolHandle                myPoolHandle;
   struct ST_CLASS(PoolElementNode) myPoolElementNode;
   struct PoolPolicySettings        myPolicySettings;
   char                             myTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*    myTransportAddressBlock = (struct TransportAddressBlock*)&myTransportAddressBlockBuffer;
   unsigned int                     result;

   if(gAsapInstance) {
      if(endpointAddressInfo->ai_pe_id == UNDEFINED_POOL_ELEMENT_IDENTIFIER) {
         endpointAddressInfo->ai_pe_id = getPoolElementIdentifier();
      }

      poolHandleNew(&myPoolHandle, poolHandle, poolHandleSize);

      poolPolicySettingsNew(&myPolicySettings);
      myPolicySettings.PolicyType      = (unsigned int)tagListGetData(tags, TAG_PoolPolicy_Type, PPT_ROUNDROBIN);
      myPolicySettings.Weight          = (unsigned int)tagListGetData(tags, TAG_PoolPolicy_Parameter_Weight, 1);
      myPolicySettings.Load            = (unsigned int)tagListGetData(tags, TAG_PoolPolicy_Parameter_Load, 0);
      myPolicySettings.LoadDegradation = (unsigned int)tagListGetData(tags, TAG_PoolPolicy_Parameter_LoadDegradation, 0);

      transportAddressBlockNew(myTransportAddressBlock,
                               endpointAddressInfo->ai_protocol,
                               getPort((struct sockaddr*)endpointAddressInfo->ai_addr),
                               (tagListGetData(tags, TAG_UserTransport_HasControlChannel, 0) != 0) ? TABF_CONTROLCHANNEL : 0,
                               endpointAddressInfo->ai_addr,
                               endpointAddressInfo->ai_addrs);

      ST_CLASS(poolElementNodeNew)(
         &myPoolElementNode,
         endpointAddressInfo->ai_pe_id,
         0,
         tagListGetData(tags, TAG_PoolElement_RegistrationLife, 300000000),
         &myPolicySettings,
         myTransportAddressBlock);

      LOG_ACTION
      fputs("Trying to register ", stdlog);
      ST_CLASS(poolElementNodePrint)(&myPoolElementNode, stdlog, PENPO_FULL);
      fputs(" to pool ", stdlog);
      poolHandlePrint(&myPoolHandle, stdlog);
      fputs("...\n", stdlog);
      LOG_END

      result = asapInstanceRegister(gAsapInstance, &myPoolHandle, &myPoolElementNode);
      if(result != RSPERR_OKAY) {
         endpointAddressInfo->ai_pe_id = UNDEFINED_POOL_ELEMENT_IDENTIFIER;
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
unsigned int rspDeregister(const unsigned char* poolHandle,
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


/* ###### Name resolution ################################################ */
unsigned int rspNameResolution(const unsigned char*         poolHandle,
                               const size_t                 poolHandleSize,
                               struct EndpointAddressInfo** endpointAddressInfo,
                               struct TagItem*              tags)
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
      result = asapInstanceNameResolution(
                  gAsapInstance,
                  &myPoolHandle,
                  (struct ST_CLASS(PoolElementNode)**)&poolElementNode,
                  &poolElementNodes);
      if(result == RSPERR_OKAY) {
         *endpointAddressInfo = (struct EndpointAddressInfo*)malloc(sizeof(struct EndpointAddressInfo));
         if(endpointAddressInfo != NULL) {
            (*endpointAddressInfo)->ai_next     = NULL;
            (*endpointAddressInfo)->ai_pe_id    = poolElementNode->Identifier;
            (*endpointAddressInfo)->ai_family   = poolElementNode->AddressBlock->AddressArray[0].sa.sa_family;
            (*endpointAddressInfo)->ai_protocol = poolElementNode->AddressBlock->Protocol;
            switch(poolElementNode->AddressBlock->Protocol) {
               case IPPROTO_SCTP:
                  (*endpointAddressInfo)->ai_socktype = SOCK_STREAM;
                break;
               case IPPROTO_TCP:
                  (*endpointAddressInfo)->ai_socktype = SOCK_STREAM;
                break;
               default:
                  (*endpointAddressInfo)->ai_socktype = SOCK_DGRAM;
                break;
            }
            (*endpointAddressInfo)->ai_addrlen = sizeof(struct sockaddr_storage);
            (*endpointAddressInfo)->ai_addrs   = poolElementNode->AddressBlock->Addresses;
            (*endpointAddressInfo)->ai_addr    = (struct sockaddr_storage*)malloc((*endpointAddressInfo)->ai_addrs * sizeof(struct sockaddr_storage));
            if((*endpointAddressInfo)->ai_addr != NULL) {
               ptr = (char*)(*endpointAddressInfo)->ai_addr;
               for(i = 0;i < poolElementNode->AddressBlock->Addresses;i++) {
                  memcpy((void*)ptr, (void*)&poolElementNode->AddressBlock->AddressArray[i],
                         sizeof(union sockaddr_union));
                  ptr = (char*)((long)ptr + (long)sizeof(struct sockaddr_storage));
               }
            }
            else {
               free(*endpointAddressInfo);
               *endpointAddressInfo = NULL;
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
void rspFreeEndpointAddressArray(struct EndpointAddressInfo* endpointAddressInfo)
{
   struct EndpointAddressInfo* next;

   while(endpointAddressInfo != NULL) {
      next = endpointAddressInfo->ai_next;

      if(endpointAddressInfo->ai_addr) {
         free(endpointAddressInfo->ai_addr);
         endpointAddressInfo->ai_addr = NULL;
      }
      free(endpointAddressInfo);

      endpointAddressInfo = next;
   }
}


/* ###### Report pool element failure #################################### */
unsigned int rspReportFailure(const unsigned char* poolHandle,
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


/* ###### select() wrapper ############################################### */
int rspSelect(int             n,
              fd_set*         readfds,
              fd_set*         writefds,
              fd_set*         exceptfds,
              struct timeval* timeout)
{
   struct timeval mytimeout;
   fd_set         myreadfds;
   fd_set         mywritefds;
   fd_set         myexceptfds;
   fd_set         testfds;
   card64         testTS;
   int            myn;
   int            i;
   card64         asapTimeout;
   card64         userTimeout;
   card64         newTimeout;
   int            result;

   if(gDispatcher) {
      /* ====== Collect data for ext_select() call ======================= */
      lock(gDispatcher, NULL);
      if(timeout == NULL) {
         userTimeout = (card64)~0;
         mytimeout.tv_sec  = ~0;
         mytimeout.tv_usec = 0;
      }
      else {
         userTimeout = ((card64)timeout->tv_sec * 1000000) + (card64)timeout->tv_usec;
      }
      dispatcherGetSelectParameters(gDispatcher, &myn, &myreadfds, &mywritefds, &myexceptfds, &testfds, &testTS, &mytimeout);
      asapTimeout = ((card64)mytimeout.tv_sec * 1000000) + (card64)mytimeout.tv_usec;
      newTimeout  = min(userTimeout, asapTimeout);
      mytimeout.tv_sec  = newTimeout / 1000000;
      mytimeout.tv_usec = newTimeout % 1000000;

      if(readfds) {
         for(i = 0;i < n;i++) {
            if(FD_ISSET(i,readfds)) {
               FD_SET(i,&myreadfds);
            }
         }
      }
      if(writefds) {
         for(i = 0;i < n;i++) {
            if(FD_ISSET(i,writefds)) {
               FD_SET(i,&mywritefds);
            }
         }
      }
      if(exceptfds) {
         for(i = 0;i < n;i++) {
            if(FD_ISSET(i,exceptfds)) {
               FD_SET(i,&myexceptfds);
            }
         }
      }
      myn = max(myn,n);

      /* ====== Print collected data ===================================== */
      LOG_VERBOSE5
      fprintf(stdlog, "Calling ext_select() with timeout %Ld [µs]...\n", newTimeout);
      for(i = 0;i < myn;i++) {
         if(FD_ISSET(i, &myreadfds) || FD_ISSET(i, &mywritefds) || FD_ISSET(i, &myexceptfds)) {
            fprintf(stdlog, "  Registered FD %d for %s%s%s\n",
                    i,
                    FD_ISSET(i, &myreadfds)   ? "<read> "  : "",
                    FD_ISSET(i, &mywritefds)  ? "<write> " : "",
                    FD_ISSET(i, &myexceptfds) ? "<except>" : "");
         }
      }
      fprintf(stdlog, "n=%d myn=%d\n", n, myn);
      LOG_END

      /* ====== Do ext_select() call ===================================== */
      unlock(gDispatcher, NULL);
sched_yield();
      result = ext_select(myn, &myreadfds, &mywritefds, &myexceptfds, &mytimeout);
      lock(gDispatcher, NULL);

      /* ====== Print results ============================================= */
      LOG_VERBOSE5
      fprintf(stdlog, "ext_select() result is %d\n", result);
      for(i = 0;i < myn;i++) {
         if(FD_ISSET(i, &myreadfds) || FD_ISSET(i, &mywritefds) || FD_ISSET(i, &myexceptfds)) {
            fprintf(stdlog, "  Events for FD %d: %s%s%s\n",
                    i,
                    FD_ISSET(i, &myreadfds)   ? "<read> "  : "",
                    FD_ISSET(i, &mywritefds)  ? "<write> " : "",
                    FD_ISSET(i, &myexceptfds) ? "<except>" : "");
         }
      }
      LOG_END

      /* ====== Handle results ============================================ */
      dispatcherHandleSelectResult(gDispatcher, result, &myreadfds, &mywritefds, &myexceptfds, &testfds, testTS);

      /* ====== Prepare results for user ================================== */
      result = 0;
      if(readfds) {
         FD_ZERO(readfds);
         for(i = 0;i < n;i++) {
            if(FD_ISSET(i,&myreadfds)) {
               FD_SET(i,readfds);
               result++;
            }
         }
      }
      if(writefds) {
         FD_ZERO(writefds);
         for(i = 0;i < n;i++) {
            if(FD_ISSET(i,&mywritefds)) {
               FD_SET(i,writefds);
               result++;
            }
         }
      }
      if(exceptfds) {
         FD_ZERO(exceptfds);
         for(i = 0;i < n;i++) {
            if(FD_ISSET(i,&myexceptfds)) {
               FD_SET(i,exceptfds);
               result++;
            }
         }
      }
      unlock(gDispatcher, NULL);
   }
   else {
      result = ext_select(n, readfds, writefds, exceptfds, timeout);
   }

   return(result);
}


