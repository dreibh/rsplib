/*
 *  $Id: rsplib.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
#include "utilities.h"
#include "netutilities.h"
#include "threadsafety.h"

#include <ext_socket.h>


#define MAX_ADDRESSES_PER_ENDPOINT 128


struct Dispatcher*          gDispatcher   = NULL;
static struct ASAPInstance* gAsapInstance = NULL;
static enum ASAPError       gLastError    = ASAP_Okay;
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
      gAsapInstance = asapNew(gDispatcher, tags);
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
      asapDelete(gAsapInstance);
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
   return(asapErrorGetDescription(gLastError));
}


/* ###### Register pool element ########################################## */
uint32_t rspRegister(const char*                       poolHandle,
                     const size_t                      poolHandleSize,
                     const struct EndpointAddressInfo* endpointAddressInfo,
                     struct TagItem*                   tags)
{
   struct PoolHandle*                myPoolHandle;
   struct PoolElement*               myPoolElement;
   struct PoolPolicy*                myPolicy;
   PoolElementIdentifier             myIdentifier = UNDEFINED_POOL_ELEMENT_IDENTIFIER;
   struct TransportAddress*          transportAddress;
   const struct EndpointAddressInfo* endpointAddress;
   size_t                            addresses;
   struct sockaddr_storage           addressArray[MAX_ADDRESSES_PER_ENDPOINT];
   char*                             ptr;
   int                               error = 0;
   size_t                            i;

   gLastError = ASAP_Okay;
   if(gAsapInstance) {
      myPoolHandle = poolHandleNew(poolHandle,poolHandleSize);
      if(myPoolHandle) {
         myIdentifier = endpointAddressInfo->ai_pe_id;
         if(myIdentifier == UNDEFINED_POOL_ELEMENT_IDENTIFIER) {
            myIdentifier = getPoolElementIdentifier();
         }
         myPolicy = poolPolicyNew((uint8_t)tagListGetData(tags,TAG_PoolPolicy_Type,TAGDATA_PoolPolicy_Type_RoundRobin));
         if(myPolicy) {
            myPolicy->Weight = (unsigned int)tagListGetData(tags,TAG_PoolPolicy_Parameter_Weight,1);
            myPolicy->Load   = (unsigned int)tagListGetData(tags,TAG_PoolPolicy_Parameter_Load,0);
            myPoolElement = poolElementNew(myIdentifier, myPolicy,
                                           tagListGetData(tags, TAG_UserTransport_HasControlChannel, false));
            if(myPoolElement != NULL) {
               endpointAddress = endpointAddressInfo;
               while(endpointAddress != NULL) {
                  addresses = endpointAddress->ai_addrs;
                  if(addresses > MAX_ADDRESSES_PER_ENDPOINT) {
                     error = EINVAL;
                     LOG_ERROR
                     fprintf(stdlog,"Too many addresses: %d\n",(int)addresses);
                     LOG_END
                     break;
                  }
                  if(endpointAddress->ai_addrlen > sizeof(struct sockaddr_storage)) {
                     error = EINVAL;
                     LOG_ERROR
                     fprintf(stdlog,"Bad address length: %d\n",(int)endpointAddress->ai_addrlen);
                     LOG_END
                     break;
                  }

                  ptr = (char*)endpointAddress->ai_addr;
                  for(i = 0;i < addresses;i++) {
                     memcpy((void*)&addressArray[i],
                            (void*)ptr,
                            endpointAddress->ai_addrlen);
                     ptr = (char*)((long)ptr + (long)endpointAddress->ai_addrlen);
                  }

                  transportAddress = transportAddressNew(endpointAddress->ai_protocol,
                                                         getPort((struct sockaddr*)endpointAddress->ai_addr),
                                                         (struct sockaddr_storage*)&addressArray,
                                                         addresses);
                  if(transportAddress == NULL) {
                     error = ENOMEM;
                     break;
                  }

                  poolElementAddTransportAddress(myPoolElement,transportAddress);

                  endpointAddress = endpointAddress->ai_next;
               }


               if(error == 0) {
                  LOG_ACTION
                  fputs("Trying to register ",stdlog);
                  poolElementPrint(myPoolElement,stdlog);
                  LOG_END

                  gLastError = asapRegister(gAsapInstance,myPoolHandle,myPoolElement);
                  if(gLastError != ASAP_Okay) {
                     switch(gLastError) {
                        default:
                           error = EIO;
                         break;
                     }
                  }
               }

               poolElementDelete(myPoolElement);
            }
            else {
               error = ENOMEM;
            }

            poolPolicyDelete(myPolicy);
         }
         else {
            error = ENOMEM;
         }

         poolHandleDelete(myPoolHandle);
      }
      else {
         error = ENOMEM;
      }
   }
   else {
      error = EPERM;
      LOG_ERROR
      fputs("rsplib is not initialized\n",stdlog);
      LOG_END
   }

   if(error) {
      errno = error;
      return(0);
   }
   return(myIdentifier);
}


/* ###### Deregister pool element ######################################## */
int rspDeregister(const char*     poolHandle,
                  const size_t    poolHandleSize,
                  const uint32_t  identifier,
                  struct TagItem* tags)
{
   struct PoolHandle* myPoolHandle;
   int                error;

   error = 0;
   gLastError = ASAP_Okay;
   if(gAsapInstance) {
      myPoolHandle = poolHandleNew(poolHandle,poolHandleSize);
      if(myPoolHandle) {
         gLastError = asapDeregister(gAsapInstance,myPoolHandle,identifier);
         if(gLastError != ASAP_Okay) {
            switch(gLastError) {
               default:
                  error = EIO;
                break;
            }
         }
         poolHandleDelete(myPoolHandle);
      }
      else {
         error = ENOMEM;
      }
   }
   else {
      error = EPERM;
      LOG_ERROR
      fputs("rsplib is not initialized\n",stdlog);
      LOG_END
   }

   errno = error;
   return(error);
}


/* ###### Name resolution ################################################ */
int rspNameResolution(const char*                  poolHandle,
                      const size_t                 poolHandleSize,
                      struct EndpointAddressInfo** endpointAddressInfo,
                      struct TagItem*              tags)
{
   struct PoolHandle*          myPoolHandle;
   struct PoolElement*         poolElement;
   struct TransportAddress*    transportAddress;
   struct EndpointAddressInfo* endpointAddress;
   GList*                      list;
   int                         error;
   size_t                      i;
   char*                       ptr;

   error                = 0;
   gLastError           = ASAP_Okay;
   *endpointAddressInfo = NULL;
   if(gAsapInstance) {
      myPoolHandle = poolHandleNew(poolHandle,poolHandleSize);
      if(myPoolHandle) {
         poolElement = asapSelectPoolElement(gAsapInstance,myPoolHandle,&gLastError);
         if(poolElement != NULL) {
            list = g_list_last(poolElement->TransportAddressList);
            while(list != NULL) {
               transportAddress = (struct TransportAddress*)list->data;
               if(transportAddress->Addresses > 0) {
                  endpointAddress = (struct EndpointAddressInfo*)malloc(sizeof(struct EndpointAddressInfo));
                  if(endpointAddress != NULL) {
                     endpointAddress->ai_next     = *endpointAddressInfo;
                     endpointAddress->ai_pe_id    = poolElement->Identifier;
                     endpointAddress->ai_family   = transportAddress->AddressArray[0].sa.sa_family;
                     endpointAddress->ai_protocol = transportAddress->Protocol;
                     switch(transportAddress->Protocol) {
                        case IPPROTO_SCTP:
                        case IPPROTO_TCP:
                           endpointAddress->ai_socktype = SOCK_STREAM;
                         break;
                        default:
                           endpointAddress->ai_socktype = SOCK_DGRAM;
                         break;
                     }
                     endpointAddress->ai_addrlen = sizeof(struct sockaddr_storage);
                     endpointAddress->ai_addrs   = transportAddress->Addresses;
                     endpointAddress->ai_addr    = (struct sockaddr_storage*)malloc(endpointAddress->ai_addrs * sizeof(struct sockaddr_storage));
                     if(endpointAddress->ai_addr != NULL) {
                        ptr = (char*)endpointAddress->ai_addr;
                        for(i = 0;i < transportAddress->Addresses;i++) {
                           memcpy((void*)ptr, (void*)&transportAddress->AddressArray[i], sizeof(union sockaddr_union));
                           ptr = (char*)((long)ptr + (long)sizeof(struct sockaddr_storage));
                        }

                        *endpointAddressInfo = endpointAddress;
                     }
                     else {
                        free(endpointAddress);
                        endpointAddress = NULL;
                     }
                  }
               }

               list = g_list_previous(list);
            }

            poolElementDelete(poolElement);
         }
         else {
            error = ENOENT;
         }
         poolHandleDelete(myPoolHandle);
      }
      else {
         error = ENOMEM;
      }
   }
   else {
      error = EPERM;
      LOG_ERROR
      fputs("rsplib is not initialized\n",stdlog);
      LOG_END
   }

   errno = error;
   return(error);
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
void rspFailure(const char*     poolHandle,
                const size_t    poolHandleSize,
                const uint32_t  identifier,
                struct TagItem* tags)
{
   struct PoolHandle* myPoolHandle;

   if(gAsapInstance) {
       myPoolHandle = poolHandleNew(poolHandle,poolHandleSize);
       if(myPoolHandle) {
          asapFailure(gAsapInstance,myPoolHandle,identifier);
          poolHandleDelete(myPoolHandle);
       }
   }
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


