/*
 * IPv4 Subnet Calculator
 * Copyright (C) 2005-2006 by Thomas Dreibholz
 *
 * $Id: subnetcalc.cc 964 2006-02-27 10:31:57Z dreibh $
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


// ###### Convert string to IPv4 address ####################################
bool getAddress(const char* str, uint32_t& address)
{
   in_addr iaddr;

   if(inet_aton(str, &iaddr) == 0) {
      return(false);
   }
   address = ntohl(iaddr.s_addr);
   return(true);
}


// ###### Print IPv4 address ################################################
void putAddress(uint32_t address)
{
   char    str[256];
   in_addr iaddr;

   iaddr.s_addr = htonl(address);
   inet_ntop(AF_INET, (char*)&iaddr, (char*)&str, sizeof(str));
   printf(str);
}


// ###### Calculate prefix length ###########################################
size_t prefixLength(uint32_t netmask)
{
   for(int i = 31;i >= 0;i--) {
      if(!(netmask & (1 << (uint32_t)i))) {
         return(31 - i);
      }
   }
   return(32);
}


// ###### Is given netmask valid? ###########################################
bool verifyNetmask(uint32_t netmask)
{
   bool host = true;

   for(int i = 0;i <= 31;i++) {
      if(netmask & (1 << (uint32_t)i)) {
         host = false;
      }
      else {
         if(host == false) {
            return(false);
         }
      }
   }
   return(true);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   size_t  prefix;
   size_t  hosts;
   uint32_t network;
   uint32_t address;
   uint32_t netmask;
   uint32_t broadcast;
   uint32_t wildcard;
   uint32_t host1;
   uint32_t host2;

   if(argc != 3) {
      printf("Usage: %s [Address] [Netmask]\n", argv[0]);
      exit(1);
   }

   if(!getAddress(argv[1], address)) {
      printf("ERROR: Bad address %s!\n", argv[1]);
      exit(1);
   }
   if(!getAddress(argv[2], netmask)) {
      printf("ERROR: Bad netmask %s!\n", argv[2]);
      exit(1);
   }
   if(!verifyNetmask(netmask)) {
      printf("ERROR: Invalid netmask "); putAddress(netmask); puts("!");
      exit(1);
   }

   prefix    = prefixLength(netmask);
   network   = address & netmask;
   broadcast = network | (~netmask);
   if(prefix < 31) {
      hosts     = (1 << (32 - prefix)) - 2;
      host1     = network + 1;
      host2     = broadcast - 1;
   }
   else if(prefix == 31) {
      hosts = 2;
      host1 = netmask;
      host2 = broadcast;
   }
   else {
      hosts = 1;
      host1 = broadcast;
      host2 = broadcast;
   }
   wildcard  = ~netmask;

   printf("Network       = "); putAddress(network); printf(" / %u\n", (unsigned int)prefix);
   printf("Netmask       = "); putAddress(netmask); puts("");
   printf("Broadcast     = "); putAddress(broadcast); puts("");
   printf("Wildcard Mask = "); putAddress(wildcard); puts("");
   printf("Max. Hosts    = %u\n", (unsigned int)hosts);
   printf("Host Range    = { ");
   putAddress(host1); printf(" ... "); putAddress(host2);
   puts(" }");
}
