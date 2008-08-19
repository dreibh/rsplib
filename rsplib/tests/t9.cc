#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>


#define NORMAL_PORT 1234
#define RAW_PORT    0


int main(int argc, char** argv)
{
   sockaddr_in rawLocal;
   memset((void*)&rawLocal, 0, sizeof(rawLocal));
   rawLocal.sin_family = AF_INET;
   rawLocal.sin_port   = htons(RAW_PORT);

   int rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
   if(rawSocket < 0) {
      perror("socket() for RAW socket. Root permission missing?");
      exit(1);
   }

   if(bind(rawSocket, (sockaddr*)&rawLocal, sizeof(rawLocal)) < 0) {
      perror("bind()");
      exit(1);
   }


   sockaddr_in normalLocal;
   memset((void*)&normalLocal, 0, sizeof(normalLocal));
   normalLocal.sin_family = AF_INET;
   normalLocal.sin_port   = htons(NORMAL_PORT);

   int normalSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if(normalSocket < 0) {
      perror("socket()");
      exit(1);
   }

   if(bind(normalSocket, (sockaddr*)&normalLocal, sizeof(normalLocal)) < 0) {
      perror("bind()");
      exit(1);
   }


   for(;;) {
      pollfd pfds[2];
      pfds[0].fd     = normalSocket;
      pfds[0].events = POLLIN;
      pfds[1].fd     = rawSocket;
      pfds[1].events = POLLIN;

      int result = poll((pollfd*)&pfds, 2, -1);
      if(result < 0) {
         perror("poll()");
         exit(1);
      }
      else if(result > 0) {
         char buffer[65536 + 1];
         if(pfds[0].revents & POLLIN) {
            int bytes = recv(normalSocket, (char*)&buffer, sizeof(buffer) - 1, 0);
            if(bytes >= 0) {
               buffer[bytes] = 0x00;
               printf("Received %d bytes on NORMAL socket: %s\n", bytes, buffer);
            }
            else {
               perror("recv()");
               exit(1);
            }
         }
         if(pfds[1].revents & POLLIN) {
            int bytes = recv(rawSocket, (char*)&buffer, sizeof(buffer) - 1, 0);
            if(bytes >= 0) {
               printf("Received %d bytes on RAW socket\n", bytes);
            }
            else {
               perror("recv()");
               exit(1);
            }
         }
      }
   }
}
