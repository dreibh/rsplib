#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <sys/poll.h>

#include "rserpool.h"
#include "ext_socket.h"
#include "registrartable.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "stringutilities.h"
#include "randomizer.h"
#include "statistics.h"
#include "loglevel.h"
#include "debug.h"


#define MAX_PR_TRANSPORTADDRESSES 128

/* ###### Create new static registrar entry in rsp_info ###### */
int rspAddStaticRegistrar(struct rsp_info* info,
                          const char*      addressString)
{
   union sockaddr_union       addressArray[MAX_PR_TRANSPORTADDRESSES];
   struct sockaddr*           packedAddressArray;
   struct rsp_registrar_info* registrarInfo;
   char                       str[1024];
   size_t                     addresses;
   char*                      address;
   char*                      idx;

   safestrcpy((char*)&str, addressString, sizeof(str));
   addresses = 0;
   address = str;
   while(addresses < MAX_PR_TRANSPORTADDRESSES) {
      idx = strindex(address, ',');
      if(idx) {
         *idx = 0x00;
      }
      if(!string2address(address, &addressArray[addresses])) {
         return(-1);
      }
      addresses++;
      if(idx) {
         address = idx;
         address++;
      }
      else {
         break;
      }
   }
   if(addresses < 1) {
      return(-1);
   }

   packedAddressArray = pack_sockaddr_union((union sockaddr_union*)&addressArray, addresses);
   if(packedAddressArray == NULL) {
      return(-1);
   }

   registrarInfo = (struct rsp_registrar_info*)malloc(sizeof(struct rsp_registrar_info));
   if(registrarInfo == NULL) {
      free(packedAddressArray);
      return(-1);
   }

   registrarInfo->rri_next  = info->ri_registrars;
   registrarInfo->rri_addr  = packedAddressArray;
   registrarInfo->rri_addrs = addresses;
   info->ri_registrars = registrarInfo;
   return(0);
}


/* ###### Free static registrar entries of rsp_info ###################### */
void rspFreeStaticRegistrars(struct rsp_info* info)
{
   rsp_registrar_info* registrarInfo;
   while(info->ri_registrars) {
      registrarInfo = info->ri_registrars;
      info->ri_registrars = registrarInfo->rri_next;
      free(registrarInfo->rri_addr);
      free(registrarInfo);
   }
}


int main(int argc, char** argv)
{
   struct rsp_info info;

   memset(&info, 0, sizeof(info));

   for(int i=1;i<argc;i++) {
      if(!(strncmp(argv[i], "-registrar=", 11))) {
         if(rspAddStaticRegistrar(&info, (char*)&argv[i][11]) < 0) {
            fprintf(stderr, "ERROR: Bad registrar setting %s\n", argv[i]);
            exit(1);
         } else puts("ok!");
      }
   }

   initLogging("-loglevel=9");
   rsp_initialize(&info);
   rsp_cleanup();

   rspFreeStaticRegistrars(&info);
   return 0;
}
