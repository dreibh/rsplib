/*
 *  $Id: localaddresses.c,v 1.2 2004/07/29 15:10:33 dreibh Exp $
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
 * Purpose: Obtaining all Local Addresses
 *
 */


#include "tdtypes.h"
#include "localaddresses.h"


#include <netinet/in_systm.h>
#include <netinet/ip.h>

#ifdef HAVE_IPV6
#ifdef LINUX_IPV6
#include <netinet/ip6.h>
#else
/* include files for IPv6 header structs */
#endif
#endif
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>



#define LINUX_PROC_IPV6_FILE "/proc/net/if_inet6"



/* ###### Filter address ################################################# */
static bool filterAddress(union sockaddr_union* address,
                          unsigned int          flags)
{
   switch (address->sa.sa_family) {
      case AF_INET :
         if((IN_MULTICAST(ntohl(address->in.sin_addr.s_addr))         && (flags & ASF_HideMulticast)) ||
            (IN_EXPERIMENTAL(ntohl(address->in.sin_addr.s_addr))      && (flags & ASF_HideReserved))  ||
            (IN_BADCLASS(ntohl(address->in.sin_addr.s_addr))          && (flags & ASF_HideReserved))  ||
            ((INADDR_BROADCAST == ntohl(address->in.sin_addr.s_addr)) && (flags & ASF_HideBroadcast)) ||
            ((INADDR_LOOPBACK == ntohl(address->in.sin_addr.s_addr))  && (flags & ASF_HideLoopback))  ||
            ((INADDR_LOOPBACK != ntohl(address->in.sin_addr.s_addr))  && (flags & ASF_HideAllExceptLoopback))) {
            return(false);
         }
       break;
#ifdef HAVE_IPV6
      case AF_INET6 :
        if((!IN6_IS_ADDR_LOOPBACK(&(address->in6.sin6_addr))  && (flags & ASF_HideAllExceptLoopback))  ||
           (IN6_IS_ADDR_LOOPBACK(&(address->in6.sin6_addr))   && (flags & ASF_HideLoopback))           ||
           (!IN6_IS_ADDR_LINKLOCAL(&(address->in6.sin6_addr)) && (flags & ASF_HideAllExceptLinkLocal)) ||
           (!IN6_IS_ADDR_SITELOCAL(&(address->in6.sin6_addr)) && (flags & ASF_HideAllExceptSiteLocal)) ||
           (IN6_IS_ADDR_LINKLOCAL(&(address->in6.sin6_addr))  && (flags & ASF_HideLinkLocal))          ||
           (IN6_IS_ADDR_SITELOCAL(&(address->in6.sin6_addr))  && (flags & ASF_HideSiteLocal))          ||
           (IN6_IS_ADDR_MULTICAST(&(address->in6.sin6_addr))  && (flags & ASF_HideMulticast))          ||
           (IN6_IS_ADDR_UNSPECIFIED(&(address->in6.sin6_addr)))) {
            return(false);
        }
       break;
#endif
      default:
         return(false);
       break;
   }
   return(true);
}


/* ###### Gather local addresses ######################################### */
bool gatherLocalAddresses(union sockaddr_union** addressArray,
                          size_t*                addresses,
                          const unsigned int     flags)
{
   union sockaddr_union* localAddresses = NULL;
   struct sockaddr*         toUse;
   struct ifreq             local;
   struct ifreq*            ifrequest;
   struct ifreq*            nextif;
   struct ifconf            cf;
   char                     buffer[8192];
#if defined (LINUX)
   int                      addedNets;
   char                     addrBuffer[256];
   char                     addrBuffer2[64];
   FILE*                    v6list;
#endif
   size_t pos           = 0;
   size_t copySize      = 0;
   size_t numAllocAddrs = 0;
   size_t numIfAddrs    = 0;
   size_t i;
   size_t j;
   size_t dup;
   int    fd;


   *addresses = 0;

   fd = socket(AF_INET,SOCK_DGRAM,0);
   if(fd < 0) {
      return(false);
   }

   /* Now gather the master address information */
   cf.ifc_buf = buffer;
   cf.ifc_len = sizeof(buffer);
   if(ioctl(fd, SIOCGIFCONF, (char *)&cf) == -1) {
       return(FALSE);
   }

#if defined (LINUX)
   numAllocAddrs = cf.ifc_len / sizeof(struct ifreq);
   ifrequest     = cf.ifc_req;
#else
   for (pos = 0; pos < cf.ifc_len; ) {
      ifrequest = (struct ifreq *)&buffer[pos];
      pos += (ifrequest->ifr_addr.sa_len + sizeof(ifrequest->ifr_name));
      if(ifrequest->ifr_addr.sa_len == 0) {
         /* If the interface has no address then you must
            skip at a minium a sockaddr structure */
         pos += sizeof(struct sockaddr);
      }
      numAllocAddrs++;
   }
#endif
   numIfAddrs = numAllocAddrs;

#if defined  (LINUX)
   addedNets = 0;
   v6list = fopen(LINUX_PROC_IPV6_FILE,"r");
   if (v6list != NULL) {
       while(fgets(addrBuffer,sizeof(addrBuffer),v6list) != NULL) {
           addedNets++;
       }
       fclose(v6list);
   }
   numAllocAddrs += addedNets;
#endif
   /* now allocate the appropriate memory */
   localAddresses = (union sockaddr_union*)calloc(numAllocAddrs,sizeof(union sockaddr_union));

   if(localAddresses == NULL) {
      close(fd);
      return(false);
   }

    pos = 0;
   /* Now we go through and pull each one */

#if defined (LINUX)
   v6list = fopen(LINUX_PROC_IPV6_FILE,"r");
   if(v6list != NULL) {
      struct sockaddr_in6 sin6;
      memset((char *)&sin6,0,sizeof(sin6));
      sin6.sin6_family = AF_INET6;

      while(fgets(addrBuffer,sizeof(addrBuffer),v6list) != NULL) {
         memset(addrBuffer2,0,sizeof(addrBuffer2));
         strncpy(addrBuffer2,addrBuffer,4);
         addrBuffer2[4] = ':';
         strncpy(&addrBuffer2[5],&addrBuffer[4],4);
         addrBuffer2[9] = ':';
         strncpy(&addrBuffer2[10],&addrBuffer[8],4);
         addrBuffer2[14] = ':';
         strncpy(&addrBuffer2[15],&addrBuffer[12],4);
         addrBuffer2[19] = ':';
         strncpy(&addrBuffer2[20],&addrBuffer[16],4);
         addrBuffer2[24] = ':';
         strncpy(&addrBuffer2[25],&addrBuffer[20],4);
         addrBuffer2[29] = ':';
         strncpy(&addrBuffer2[30],&addrBuffer[24],4);
         addrBuffer2[34] = ':';
         strncpy(&addrBuffer2[35],&addrBuffer[28],4);

         if(inet_pton(AF_INET6,addrBuffer2,(void *)&sin6.sin6_addr)) {
            if(filterAddress((union sockaddr_union*)&sin6,flags) == true) {
               memcpy(&((localAddresses)[*addresses]),&sin6,sizeof(sin6));
               (*addresses)++;
            }
         }
      }
      fclose(v6list);
   }
#endif

   /* set to the start, i.e. buffer[0] */
   ifrequest = (struct ifreq *)&buffer[pos];

   for(i = 0;i < numIfAddrs;i++,ifrequest = nextif) {
#if defined (LINUX)
      nextif = ifrequest + 1;
#else
      /* use the sa_len to calculate where the next one will be */
      pos += (ifrequest->ifr_addr.sa_len + sizeof(ifrequest->ifr_name));

      if (ifrequest->ifr_addr.sa_len == 0) {
         /* if the interface has no address then you must
          * skip at a minium a sockaddr structure
          */
         pos += sizeof(struct sockaddr);
      }
      nextif = (struct ifreq *)&buffer[pos];
#endif
      toUse = &ifrequest->ifr_addr;

      memset(&local, 0, sizeof(local));
      memcpy(local.ifr_name, ifrequest->ifr_name, IFNAMSIZ);
      if(filterAddress((union sockaddr_union*)toUse, flags) == false) {
         continue;
      }

      if(ioctl(fd,SIOCGIFFLAGS,(char*)&local) < 0) {
         continue;
      }
      if(!(local.ifr_flags & IFF_UP)) {
         /* Device is down. */
         continue;
      }

      if(toUse->sa_family == AF_INET) {
         copySize = sizeof(struct sockaddr_in);
      }
      else if (toUse->sa_family == AF_INET6) {
         copySize = sizeof(struct sockaddr_in6);
      }
      else {
         continue;
      }


      /* Check for duplicates */
      if(*addresses) {
         dup = 0;
         /* scan for the dup */
         for(j = 0; j < *addresses; j++) {
            /* family's must match */
            if(((struct sockaddr*)&(localAddresses[j]))->sa_family != toUse->sa_family) {
                continue;
            }

            if(((struct sockaddr*)&(localAddresses[j]))->sa_family == AF_INET) {
               if(((struct sockaddr_in *)(toUse))->sin_addr.s_addr == ((struct sockaddr_in*)&(localAddresses[j]))->sin_addr.s_addr) {
                  /* set the flag and break, it is a dup */
                  dup = 1;
                  break;
               }
#ifdef HAVE_IPV6
            } else {
                if(IN6_ARE_ADDR_EQUAL(&(((struct sockaddr_in6*)(toUse))->sin6_addr),
                                      &(((struct sockaddr_in6*)&(localAddresses[j]))->sin6_addr))) {
                    /* set the flag and break, it is a dup */
                    dup = 1;
                    break;
                }
#endif
            }
         }
         if(dup) {
            /* skip the duplicate name/address we already have it*/
            continue;
         }
      }

      /* copy address and family */
      memcpy(&localAddresses[*addresses],(char *)toUse,copySize);
      ((struct sockaddr*)&(localAddresses[*addresses]))->sa_family = toUse->sa_family;

#if !defined (LINUX)
      ((struct sockaddr*)&(localAddresses[*addresses]))->sa_len = toUse->sa_len;
#endif
      (*addresses)++;
   }

   *addressArray = localAddresses;
   close(fd);

   return(true);
}
