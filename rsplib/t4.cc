#include "tdtypes.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "rsplib.h"
#include "ext_socket.h"


int main(int argc, char** argv)
{
   union sockaddr_union a1;
   union sockaddr_union a2;
   int yes = 1;
   int sd;

   string2address("0.0.0.0:1234",&a1);
   string2address("127.0.0.1:1234",&a2);

   yes = 1;

   sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if(sd < 0) {
      perror("socket()");
      exit(1);
   }

   if(argc < 2) {
      yes = 1;
      if(setsockopt(sd,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(yes)) < 0) {
         perror("setsockopt()");
      }
      if(setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
         perror("setsockopt()");
      }

      if(bind(sd, &a1.sa, sizeof(a1)) < 0) {
         perror("bind()");
      }

      puts("RECEIVER MODE");
      char buffer[2048];
      ssize_t r = recv(sd, (char*)&buffer, sizeof(buffer), 0);
      while(r > 0) {
         printf("=> r=%d\n", r);
         r = recv(sd, (char*)&buffer, sizeof(buffer), 0);
      }
   }
   else {
      puts("SENDER MODE");
      const char* text = "Test";
      ssize_t s = sendto(sd, &text, strlen(text), 0, &a2.sa, sizeof(a2));
      printf("s=%d\n", s);
   }

   close(sd);
}
