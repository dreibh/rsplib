#include "tdtypes.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "rsplib.h"
#include "ext_socket.h"

int main(int argc, char** argv)
{
   fd_set r;
   fd_set w;
   fd_set e;
   int fd,n,i,result;
   int udp;
   int fd2;
   int a,b;
   char buf[256];
   card64 z=0;
   struct sockaddr_storage adr;
   card64 start,now;
   struct timeval timeout;

   rspInitialize(NULL);

   fd2 = -1;
   fd = ext_socket(AF_INET,SOCK_STREAM,IPPROTO_SCTP);
   if(fd > 0) {
   udp = ext_socket(AF_INET,SOCK_DGRAM,IPPROTO_SCTP);
   if(udp > 0) {
      string2address("0.0.0.0:1234",&adr);
      if(ext_bind(fd,(struct sockaddr*)&adr,sizeof(adr)) == 0) {
      string2address("0.0.0.0:1235",&adr);
      if(ext_bind(udp,(struct sockaddr*)&adr,sizeof(adr)) == 0) {
         puts("READY!");
         ext_listen(fd,5);

         setNonBlocking(fd);
         setNonBlocking(udp);

         start = getMicroTime();
         for(;;) {
            FD_ZERO(&r);
            FD_ZERO(&w);
            FD_ZERO(&e);
            FD_SET(fd,&r);
            FD_SET(fd,&w);
            FD_SET(fd,&e);
            FD_SET(udp,&r);
            FD_SET(udp,&w);
            FD_SET(udp,&e);
            if(fd2 >= 0) {
               FD_SET(fd2,&r);
               FD_SET(fd2,&w);
               FD_SET(fd2,&e);
            }
            timeout.tv_sec = 0;
            timeout.tv_usec = 1;
            result = rspSelect(fd + 1,&r,NULL,NULL,&timeout);
            if(result > 0) {
               printf("Durchläufe: %Ld\n",z);
               if(FD_ISSET(fd,&r)) {
                  fd2 = ext_accept(fd,NULL,0);
                  printf("fd2=%d\n",fd2);
               }
               if((fd2 >= 0) && (FD_ISSET(fd2,&r))) {
                  printf("reading...\n");
                  a = ext_read(fd2,(char*)&buf,sizeof(buf));
                  if(a > 0) {
                     printf("read: %d\n",a);
                  }
                  else if(a == 0) {
                     ext_close(fd2);
                     fd2 = -1;
                     break;
                  }
               }
               if(FD_ISSET(udp,&r)) {
                  printf("reading udp...\n");
                  a = ext_read(udp,(char*)&buf,sizeof(buf));
                  if(a > 0) {
                     printf("read udp: %d\n",a);
                  }
               }
            }
            else {
               z++;
               // now = getMicroTime();
               //printf("%Ld\n",now - start);
            }
         }
         }
      }
      ext_close(udp);
      }
      ext_close(fd);
   }


   return(0);
}
