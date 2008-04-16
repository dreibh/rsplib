#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "netutilities.h"


int main(int argc, char** argv)
{
   if(argc < 5) {
      printf("Usage: %s [sender|receiver] [interface] [group] [port]\n", argv[0]);
      exit(1);
   }

   const int            myInterface = if_nametoindex(argv[2]);
   const char*          myGroup     = argv[3];
   const unsigned short myPort      = atol(argv[4]);

   printf("IF Index = %d\n", myInterface);
   printf("Group    = %s\n", myGroup);
   printf("Port     = %u\n", myPort);

   sockaddr_in6 group;
   memset((void*)&group, 0, sizeof(group));
   group.sin6_family = AF_INET6;
   group.sin6_port   = htons(myPort);
   if(inet_pton(AF_INET6, myGroup, (void *)&group.sin6_addr) == NULL) {
      perror("setsockopt(SO_REUSEADDR)");
      exit(1);
   }

   int sd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
   if(sd < 0) {
      perror("socket()");
      exit(1);
   }


   if(!(strcmp(argv[1], "sender"))) {
      puts("SENDER MODE!");

      sockaddr_in6 local;
      memset((void*)&local, 0, sizeof(local));
      local.sin6_family = AF_INET6;
      local.sin6_port   = 0;

      if(bind(sd, (sockaddr*)&local, sizeof(local)) < 0) {
         perror("bind()");
         exit(1);
      }

      int outif = myInterface;
      if(setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &outif, sizeof(outif)) < 0) {
         perror("setsockopt(IPV6_MULTICAST_IF)");
         exit(1);
      }

      for(int i =0;;i++) {
         char str[256];
         snprintf((char*)&str, sizeof(str), "Message #%06u!", i + 1);

         printf(".");
         fflush(stdout);

         if(sendto(sd, (char*)&str, strlen(str), 0, (sockaddr*)&group, sizeof(group)) < 0) {
            perror("sendto()");
         }
         usleep(500000);
      }
   }
   else {
      puts("RECEIVER MODE!");

      int reusable = 1;
      if(setsockopt(sd,  SOL_SOCKET, SO_REUSEADDR, &reusable, sizeof(reusable)) < 0) {
         perror("setsockopt(SO_REUSEADDR)");
         exit(1);
      }

      sockaddr_in6 local;
      memset((void*)&local, 0, sizeof(local));
      local.sin6_family = AF_INET6;
      local.sin6_port   = htons(myPort);

      if(bind(sd, (sockaddr*)&local, sizeof(local)) < 0) {
         perror("bind()");
         exit(1);
      }

      struct ipv6_mreq mreq6;
      memcpy((char*)&mreq6.ipv6mr_multiaddr,
             (char*)&group.sin6_addr,
             sizeof(group.sin6_addr));
      mreq6.ipv6mr_interface = myInterface;
      if(setsockopt(sd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)) < 0) {
         perror("setsockopt(IPV6_JOIN_GROUP)");
         exit(1);
      }

      for(;;) {
         char buffer[16384];
         int bytes = recv(sd, (char*)&buffer, sizeof(buffer) - 1, 0);
         if(bytes >= 0) {
            buffer[bytes] = 0x00;
            printf("Get %d bytes: %s\n", bytes, buffer);
         }
         else {
            perror("recv()");
            exit(1);
         }
      }
   }
}
