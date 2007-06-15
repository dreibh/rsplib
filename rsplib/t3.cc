#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip6.h>
#include <netinet/sctp.h>


int main(int argc, char** argv)
{
   struct sockaddr_in6 in6;
   struct sockaddr*    local;
   int                 sd;

   sd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(sd < 0) perror("socket()");

   memset((char*)&in6, 0, sizeof(in6));
   in6.sin6_family = AF_INET6;

   if(bind(sd, (struct sockaddr*)&in6, sizeof(in6)) != 0) perror("bind()");

   printf("Addresses=%d\n", sctp_getladdrs(sd, 0, &local));
}
