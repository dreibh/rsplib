/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2012 by Thomas Dreibholz
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

#include "config.h"
#include "transportaddressblock.h"
#include "stringutilities.h"
#include "debug.h"

#include <ext_socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifndef HAVE_TEST
#include "netutilities.h"
#else
/* ###### Convert address to string ###################################### */
static int address2string(const struct sockaddr* address,
                          char*                  buffer,
                          const size_t           length,
                          const int              port)
{
   struct sockaddr_in*       ipv4address;
   struct sockaddr_in6*      ipv6address;
#ifdef HAVE_TEST
   struct sockaddr_testaddr* testaddress;
#endif
   char                      str[128];

   switch(address->sa_family) {
      case AF_INET:
         ipv4address = (struct sockaddr_in*)address;
         if(port) {
            snprintf(buffer, length,
                     "%s:%u", inet_ntoa(ipv4address->sin_addr), ntohs(ipv4address->sin_port));
         }
         else {
            snprintf(buffer, length, "%s", inet_ntoa(ipv4address->sin_addr));
         }
         return(1);
       break;
      case AF_INET6:
         ipv6address = (struct sockaddr_in6*)address;
         ipv6address->sin6_scope_id = 0;
         if(inet_ntop(AF_INET6,&ipv6address->sin6_addr, str, sizeof(str)) != NULL) {
            if(port) {
               snprintf(buffer, length,
                        "[%s]:%u", str, ntohs(ipv6address->sin6_port));
            }
            else {
               snprintf(buffer, length, "%s", str);
            }
            return(1);
         }
       break;
#ifdef HAVE_TEST
      case AF_TEST:
         testaddress = (struct sockaddr_testaddr*)address;
         snprintf(buffer, length, "%u:%u",
                  testaddress->ta_addr,
                  testaddress->ta_port);
         return(1);
       break;
#endif
      case AF_UNSPEC:
         safestrcpy(buffer,"(unspecified)",length);
         return(1);
       break;
   }

   snprintf(buffer,length,"(unsupported address family #%d)",address->sa_family);
   return(0);
}
#endif


/* ###### Initialize ##################################################### */
void transportAddressBlockNew(struct TransportAddressBlock* transportAddressBlock,
                              const int                     protocol,
                              const uint16_t                port,
                              const uint16_t                flags,
                              const union sockaddr_union*   addressArray,
                              const size_t                  addresses,
                              const size_t                  maxAddresses)
{
   size_t i;
   transportAddressBlock->Next      = NULL;
   transportAddressBlock->Flags     = flags;
   transportAddressBlock->Port      = port;
   transportAddressBlock->Protocol  = protocol;
   transportAddressBlock->Addresses = min(maxAddresses, addresses);

   for(i = 0;i < transportAddressBlock->Addresses;i++) {
      memcpy((void*)&transportAddressBlock->AddressArray[i],
             (void*)&addressArray[i],
             sizeof(union sockaddr_union));
      switch(((struct sockaddr*)&addressArray[i])->sa_family) {
         case AF_INET:
            ((struct sockaddr_in*)&transportAddressBlock->AddressArray[i])->sin_port = htons(port);
          break;
         case AF_INET6:
            ((struct sockaddr_in6*)&transportAddressBlock->AddressArray[i])->sin6_port = htons(port);
          break;
#ifdef HAVE_TEST
         case AF_TEST:
            ((struct sockaddr_testaddr*)&transportAddressBlock->AddressArray[i])->ta_port = port;
          break;
#endif
         default:
            fprintf(stderr,"Unsupported address family #%d\n",((struct sockaddr*)&addressArray[i])->sa_family);
          break;
      }
   }
}


/* ###### Invalidate ##################################################### */
void transportAddressBlockDelete(struct TransportAddressBlock* transportAddressBlock)
{
   transportAddressBlock->Flags     = 0;
   transportAddressBlock->Port      = 0;
   transportAddressBlock->Protocol  = 0;
   transportAddressBlock->Addresses = 0;
}


/* ###### Get textual description ######################################## */
void transportAddressBlockGetDescription(
        const struct TransportAddressBlock* transportAddressBlock,
        char*                               buffer,
        const size_t                        bufferSize)
{

   char   addressString[96];
   char   protocolString[32];
   size_t i;

   if(transportAddressBlock != NULL) {
      safestrcpy(buffer, "{", bufferSize);
      for(i = 0;i < transportAddressBlock->Addresses;i++) {
         if(i > 0) {
            safestrcat(buffer, ", ", bufferSize);
         }
         if(address2string((struct sockaddr*)&transportAddressBlock->AddressArray[i],
                           (char*)&addressString, sizeof(addressString), 0)) {
            safestrcat(buffer, addressString, bufferSize);
         }
         else {
            safestrcat(buffer, "(invalid)", bufferSize);
         }
      }
      safestrcat(buffer, "} ", bufferSize);
      switch(transportAddressBlock->Protocol) {
         case IPPROTO_SCTP:
            strcpy((char*)&protocolString,"SCTP");
          break;
         case IPPROTO_TCP:
            strcpy((char*)&protocolString,"TCP");
          break;
         case IPPROTO_UDP:
            strcpy((char*)&protocolString,"UDP");
          break;
         default:
            snprintf((char*)&protocolString, sizeof(protocolString),
                     "Protocol $%04x",transportAddressBlock->Protocol);
          break;
      }
      snprintf((char*)&addressString, sizeof(addressString),
               "%u/%s%s", transportAddressBlock->Port,
                          protocolString,
                          ((transportAddressBlock->Flags & TABF_CONTROLCHANNEL) ?
                             "+CtrlCh" : ""));
      safestrcat(buffer, addressString, bufferSize);
   }
   else {
      safestrcpy(buffer, "(null)", bufferSize);
   }
}


/* ###### Print TransportAddressBlock #################################### */
void transportAddressBlockPrint(
        const struct TransportAddressBlock* transportAddressBlock,
        FILE*                               fd)
{
   char buffer[512];
   transportAddressBlockGetDescription(transportAddressBlock,
                                       (char*)&buffer, sizeof(buffer));
   fputs(buffer, fd);
}


/* ###### Duplicate TransportAddressBlock ################################ */
struct TransportAddressBlock* transportAddressBlockDuplicate(const struct TransportAddressBlock* transportAddressBlock)
{
   struct TransportAddressBlock* duplicate;
   size_t                        size;

   if(transportAddressBlock) {
      size      = transportAddressBlockGetSize(transportAddressBlock->Addresses);
      duplicate = (struct TransportAddressBlock*)malloc(size);
      if(duplicate) {
         memcpy(duplicate, transportAddressBlock, size);
         return(duplicate);
      }
   }
   return(NULL);
}


#ifdef HAVE_TEST
int addresscmp(const struct sockaddr* address1, const struct sockaddr* address2, const bool port)
{
   const struct sockaddr_testaddr* test1 = (const struct sockaddr_testaddr*)address1;
   const struct sockaddr_testaddr* test2 = (const struct sockaddr_testaddr*)address2;
   CHECK(test1->ta_family == AF_TEST);
   CHECK(test2->ta_family == AF_TEST);
   if(test1->ta_addr < test2->ta_addr) {
      return(-1);
   }
   else if(test1->ta_addr > test2->ta_addr) {
      return(1);
   }
   if(port) {
      if(test1->ta_port < test2->ta_port) {
         return(-1);
      }
      else if(test1->ta_port > test2->ta_port) {
         return(1);
      }
   }
   return(0);
}
#else
#include "netutilities.h"
#endif


/* ###### Compare TransportAddressBlocks ################################# */
int transportAddressBlockComparison(const void* transportAddressBlockPtr1,
                                    const void* transportAddressBlockPtr2)
{
   const struct TransportAddressBlock* transportAddressBlock1 = (const struct TransportAddressBlock*)transportAddressBlockPtr1;
   const struct TransportAddressBlock* transportAddressBlock2 = (const struct TransportAddressBlock*)transportAddressBlockPtr2;
   int                                 result;
   size_t                              i;

   if((transportAddressBlock1 == NULL) &&
      (transportAddressBlock2 != NULL)) {
      return(-1);
   }
   else if((transportAddressBlock1 != NULL) &&
      (transportAddressBlock2 == NULL)) {
      return(1);
   }
   if(transportAddressBlock1->Port < transportAddressBlock2->Port) {
      return(-1);
   }
   else if(transportAddressBlock1->Port > transportAddressBlock2->Port) {
      return(1);
   }
   if(transportAddressBlock1->Flags < transportAddressBlock2->Flags) {
      return(-1);
   }
   else if(transportAddressBlock1->Flags > transportAddressBlock2->Flags) {
      return(1);
   }
   if(transportAddressBlock1->Addresses < transportAddressBlock2->Addresses) {
      return(-1);
   }
   else if(transportAddressBlock1->Addresses > transportAddressBlock2->Addresses) {
      return(1);
   }
   for(i = 0;i < transportAddressBlock1->Addresses;i++) {
      result = addresscmp((const struct sockaddr*)&transportAddressBlock1->AddressArray[i],
                          (const struct sockaddr*)&transportAddressBlock2->AddressArray[i],
                          false);
      if(result != 0) {
         return(result);
      }
   }
   return(0);
}


/* ###### Overlap comparison of TransportAddressBlocks ################### */
int transportAddressBlockOverlapComparison(const void* transportAddressBlockPtr1,
                                           const void* transportAddressBlockPtr2)
{
   const struct TransportAddressBlock* transportAddressBlock1 = (const struct TransportAddressBlock*)transportAddressBlockPtr1;
   const struct TransportAddressBlock* transportAddressBlock2 = (const struct TransportAddressBlock*)transportAddressBlockPtr2;
   int                                 result;
   size_t                              i, j;

   if((transportAddressBlock1 == NULL) &&
      (transportAddressBlock2 != NULL)) {
      return(-1);
   }
   else if((transportAddressBlock1 != NULL) &&
      (transportAddressBlock2 == NULL)) {
      return(1);
   }
   if(transportAddressBlock1->Port < transportAddressBlock2->Port) {
      return(-1);
   }
   else if(transportAddressBlock1->Port > transportAddressBlock2->Port) {
      return(1);
   }
   if(transportAddressBlock1->Flags < transportAddressBlock2->Flags) {
      return(-1);
   }
   else if(transportAddressBlock1->Flags > transportAddressBlock2->Flags) {
      return(1);
   }

   for(i = 0;i < transportAddressBlock1->Addresses;i++) {
      for(j = 0;j < transportAddressBlock2->Addresses;j++) {
         result = addresscmp((const struct sockaddr*)&transportAddressBlock1->AddressArray[i],
                             (const struct sockaddr*)&transportAddressBlock2->AddressArray[j],
                             false);
         if(result == 0) {
            return(0);
         }
      }
   }

   if(transportAddressBlock1->Addresses < transportAddressBlock2->Addresses) {
      return(-1);
   }
   else if(transportAddressBlock1->Addresses > transportAddressBlock2->Addresses) {
      return(1);
   }
   for(i = 0;i < transportAddressBlock1->Addresses;i++) {
      result = addresscmp((const struct sockaddr*)&transportAddressBlock1->AddressArray[i],
                          (const struct sockaddr*)&transportAddressBlock2->AddressArray[i],
                          false);
      if(result != 0) {
         return(result);
      }
   }
   return(0);
}


#ifndef HAVE_TEST
/* ###### Get addresses from SCTP socket ################################# */
#define MAX_ADDRESSES 128
size_t transportAddressBlockGetAddressesFromSCTPSocket(
          struct TransportAddressBlock* sctpAddress,
          int                           sockFD,
          sctp_assoc_t                  assocID,
          const size_t                  maxAddresses,
          const bool                    local)
{
   union sockaddr_union  sctpAddressArray[MAX_ADDRESSES];
   union sockaddr_union* endpointAddressArray;
   size_t                sctpAddresses;

   if(local) {
      sctpAddresses = getladdrsplus(sockFD, assocID, (union sockaddr_union**)&endpointAddressArray);
   }
   else {
      sctpAddresses = getpaddrsplus(sockFD, assocID, (union sockaddr_union**)&endpointAddressArray);
   }
   if(sctpAddresses > 0) {
      if(sctpAddresses > maxAddresses) {
         sctpAddresses = maxAddresses;
      }
      if(sctpAddresses > MAX_ADDRESSES) {
         sctpAddresses = MAX_ADDRESSES;
      }
      memcpy(&sctpAddressArray, endpointAddressArray, sctpAddresses * sizeof(union sockaddr_union));
      free(endpointAddressArray);

      transportAddressBlockNew(sctpAddress,
                               IPPROTO_SCTP,
                               getPort((struct sockaddr*)&sctpAddressArray[0]),
                               0,
                               (union sockaddr_union*)&sctpAddressArray,
                               sctpAddresses,maxAddresses);
   }
   return(sctpAddresses);
}
#endif


#ifndef HAVE_TEST
/* ###### Filter address array ########################################### */
size_t transportAddressBlockFilter(
          const struct TransportAddressBlock* originalAddressBlock,
          const struct TransportAddressBlock* associationAddressBlock,
          struct TransportAddressBlock*       filteredAddressBlock,
          const size_t                        maxAddresses,
          const bool                          filterPort,
          const unsigned int                  minScope)
{
   bool   selectionArray[MAX_ADDRESSES];
   size_t selected = 0;
   size_t i, j;

   CHECK(maxAddresses <= MAX_ADDRESSES);
   for(i = 0;i < originalAddressBlock->Addresses;i++) {
      selectionArray[i] = false;
      if(getScope((const struct sockaddr*)&originalAddressBlock->AddressArray[i]) >= minScope) {
         if(associationAddressBlock != NULL) {
            for(j = 0;j < associationAddressBlock->Addresses;j++) {
               if(addresscmp(&originalAddressBlock->AddressArray[i].sa,
                             &associationAddressBlock->AddressArray[j].sa,
                             filterPort) == 0) {
                  selectionArray[i] = true;
                  selected++;
                  break;
               }
            }
         }
         else {
            selectionArray[i] = true;
            selected++;
         }
      }
   }

   if(selected > 0) {
      filteredAddressBlock->Next      = NULL;
      filteredAddressBlock->Protocol  = originalAddressBlock->Protocol;
      filteredAddressBlock->Port      = originalAddressBlock->Port;
      filteredAddressBlock->Flags     = originalAddressBlock->Flags;
      filteredAddressBlock->Addresses = selected;
      j = 0;
      for(i = 0;i < originalAddressBlock->Addresses;i++) {
         if(selectionArray[i]) {
            memcpy(&filteredAddressBlock->AddressArray[j],
                   (const struct sockaddr*)&originalAddressBlock->AddressArray[i],
                   sizeof(filteredAddressBlock->AddressArray[j]));
            j++;
         }
      }
   }

   return(selected);
}
#endif
