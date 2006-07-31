#include "tdtypes.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "ext_socket.h"

#include <net/if.h>
#include <sys/ioctl.h>


int main(int argc, char** argv)
{
   char          lastIFName[IFNAMSIZ];
   char          buffer[4096];
   struct ifconf ifc;

   int family = AF_INET;

   int sd = socket(family, SOCK_DGRAM, IPPROTO_UDP);


   ifc.ifc_len = sizeof(buffer);
   ifc.ifc_buf = buffer;
   if(ioctl(sd, SIOCGIFCONF, (char*)&ifc) >= 0) {
      const struct ifreq* ifr;
      const struct ifreq* ifrbase;
      int                 offset;

      // loop over all interfaces
      lastIFName[0] = 0x00;
      ifrbase = ifc.ifc_req;
      for(size_t i =0,offset = 0;offset < ifc.ifc_len;i++) {
         ifr = (const struct ifreq*)(&((char*)ifrbase)[offset]);
#ifdef HAVE_SIN_LEN
         /* The ifreq structure is overwritten by the following ioctl
            calls -> calculate the new offset here! */
         offset += sizeof(ifr->ifr_name) +
                      (ifr->ifr_addr.sa_len > sizeof(struct sockaddr) ?
                         ifr->ifr_addr.sa_len : sizeof(struct sockaddr));
#else
         offset += sizeof(struct ifreq);
#endif

         if(strcmp(lastIFName, ifr->ifr_name)) {
            strcpy((char*)&lastIFName, ifr->ifr_name);
            printf("#%d = %s  o=%d\n", i, ifr->ifr_name,offset);
 
            if(ioctl(sd, SIOCGIFFLAGS, (char*)ifr) >= 0) {
               if( (ifr->ifr_flags & IFF_UP) &&
                   (ifr->ifr_flags & IFF_MULTICAST) ) {
                  puts("ok!");
               }
            }
         }
      }
      
      
      for(size_t i = 0;i < ifc.ifc_len;i++) {
         if(isprint(buffer[i])) printf("%c", buffer[i]); else printf(".");         
      }
      puts("");
   }
   else {
      perror("ioctl SIOCGIFCONF failed - unable to obtain network interfaces");
   }

   return(0);
}
