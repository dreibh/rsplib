#include "tdtypes.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "ext_socket.h"

#include <net/if.h>
#include <sys/ioctl.h>


int main(int argc, char** argv)
{
   char          buffer[4096];
   struct ifconf ifc;

   int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


   ifc.ifc_len = sizeof(buffer);
   ifc.ifc_buf = buffer;
   if(ioctl(sd, SIOCGIFCONF, (char*)&ifc) >= 0) {
      struct ifreq* ifr;

      // loop over all interfaces
      ifr = ifc.ifc_req;
      for(size_t i = 0;i < ifc.ifc_len / sizeof(struct ifreq);i++, ifr++) {
         printf("#%d = %s\n", i, ifr->ifr_name);

         if(ioctl(sd, SIOCGIFFLAGS, (char*)ifr) >= 0) {
            if( (ifr->ifr_flags & IFF_UP) &&
                (ifr->ifr_flags & IFF_MULTICAST) ) {

               puts("ok!");
            }
         }
         else {
            printf("Skipping interface %s\n", ifr->ifr_name);
         }
      }
   }
   else {
      perror("ioctl SIOCGIFCONF failed - unable to obtain network interfaces");
   }

   return(0);
}
