/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#include "transportaddressblock.h"
#include "stringutilities.h"

#include <ext_socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/* ###### Convert address to string ###################################### */
int address2string(const struct sockaddr* address,
                   char*                  buffer,
                   const size_t           length,
                   const int              port)
{
   struct sockaddr_in*  ipv4address;
#ifdef HAVE_IPV6
   struct sockaddr_in6* ipv6address;
   char                 str[128];
#endif
#ifdef HAVE_TEST
   struct sockaddr_testaddr* testaddress;
#endif

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
#ifdef HAVE_IPV6
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
#endif
#ifdef HAVE_TEST
      case AF_TEST:
         testaddress = (struct sockaddr_testaddr*)address;
         snprintf(buffer, length, "%u:%u-(x=%1.2f/y=%1.2f/z=%1.2f)",
                  testaddress->ta_addr,
                  testaddress->ta_port,
                  testaddress->ta_pos_x,
                  testaddress->ta_pos_y,
                  testaddress->ta_pos_z);
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


/* ###### Initialize ##################################################### */
void transportAddressBlockNew(struct TransportAddressBlock*  transportAddressBlock,
                              const int                      protocol,
                              const uint16_t                 port,
                              const uint16_t                 flags,
                              const struct sockaddr_storage* addressArray,
                              const size_t                   addresses)
{
   size_t i;
   transportAddressBlock->Flags     = flags;
   transportAddressBlock->Port      = port;
   transportAddressBlock->Protocol  = protocol;
   transportAddressBlock->Addresses = addresses;

   for(i = 0;i < addresses;i++) {
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

   char   addressString[64];
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
   char buffer[256];
   transportAddressBlockGetDescription(transportAddressBlock,
                                       (char*)&buffer, sizeof(buffer));
   fputs(buffer, fd);
}


/* ###### Duplicate TransportAddressBlock ################################ */
struct TransportAddressBlock* transportAddressBlockDuplicate(const struct TransportAddressBlock* transportAddressBlock)
{
   const size_t                  size      = transportAddressBlockGetSize(transportAddressBlock->Addresses);
   struct TransportAddressBlock* duplicate = (struct TransportAddressBlock*)malloc(size);
   if(duplicate) {
      memcpy(duplicate, transportAddressBlock, size);
   }
   return(duplicate);
}
