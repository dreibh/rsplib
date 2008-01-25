#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip6.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <net/if.h>


void printAddrs(struct sockaddr* addrArray,
                const size_t     addrs)
{
   const sockaddr_in*  in;
   const sockaddr_in6* in6;
   size_t              i;
   char                str[256];
   char                ifnamebuffer[IFNAMSIZ];
   const char*         ifname;

   for(i = 0;i < addrs;i++) {
      switch(addrArray->sa_family) {
         case AF_INET:
            in = (const sockaddr_in*)addrArray;
            if(inet_ntop(AF_INET, &in->sin_addr, (char*)&str, sizeof(str)) != NULL) {
               printf(" - #%u: %s   port=%u\n", i + 1, str, ntohs(in->sin_port));
            }
            else {
               printf("ERROR: Bad IPv4 address!");
            }
            addrArray = (struct sockaddr*)((long)addrArray + (long)sizeof(struct sockaddr_in));
          break;
         case AF_INET6:
            in6 = (const sockaddr_in6*)addrArray;
            if(inet_ntop(AF_INET6, &in6->sin6_addr, (char*)&str, sizeof(str)) != NULL) {
               if(IN6_IS_ADDR_LINKLOCAL(&in6->sin6_addr)) {
                  ifname = if_indextoname(in6->sin6_scope_id, (char*)&ifnamebuffer);
                  if(ifname == NULL) {
                     ifname = "   ERROR: Bad link-local scope ID!";
                  }
                  printf(" - #%u: %s%%%s   port=%u\n", i + 1, str, ifname, ntohs(in6->sin6_port));
               }
               else {
                  printf(" - #%u: %s   port=%u\n", i + 1, str, ntohs(in6->sin6_port));
               }
            }
            else {
               printf("ERROR: Bad IPv6 address!");
            }
            addrArray = (struct sockaddr*)((long)addrArray + (long)sizeof(struct sockaddr_in6));
          break;
         default:
            printf("ERROR: Unknown address type #%d\n",
                    addrArray->sa_family);
          break;
      }
   }
}


int main(int argc, char** argv)
{
   struct sockaddr_in6 in6;
   struct sockaddr*    local;
   int                 sd;
   size_t              addrs;

   sd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(sd < 0) perror("socket()");

   memset((char*)&in6, 0, sizeof(in6));
   in6.sin6_family = AF_INET6;

   if(bind(sd, (struct sockaddr*)&in6, sizeof(in6)) != 0) perror("bind()");

   addrs = sctp_getladdrs(sd, 0, &local);
   if(addrs > 0) {
      printf("Got %u addresses:\n", addrs);
      printAddrs(local, addrs);
      sctp_freeladdrs(local);
   }
   else {
      perror("No addresses!");
   }
}
