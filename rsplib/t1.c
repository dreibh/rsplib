#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <math.h>
#include <sys/types.h>
#include <signal.h>

#include <rserpool/rserpool.h>
#include <rserpool/rserpool-internals.h>
#include <rserpool/tagitem.h>


int main(int argc, char** argv)
{
   const char*           poolHandle    = "EchoPool";
   unsigned int          reregInterval = 30000;
   struct rsp_info       info;
   struct rsp_loadinfo   loadInfo;
   struct rsp_sndrcvinfo rinfo;
   int                   flags;
   int                   fd;
   int                   i;

   memset(&loadInfo, 0, sizeof(loadInfo));
   loadInfo.rli_policy = PPT_ROUNDROBIN;

   for(i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (const char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-rereginterval=" ,15))) {
         reregInterval = atol((const char*)&argv[i][15]);
         if(reregInterval < 10) {
            reregInterval = 10;
         }
      }
      else {
         fprintf(stderr, "ERROR: Bad argument %s.\n", argv[i]);
         exit(1);
      }
   }


   if(rsp_initialize(&info) < 0) {
      fputs("ERROR: Unable to initialize rsplib Library!\n", stderr);
   }


   fd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(fd >= 0) {
      if(rsp_listen(fd, 10) == 0) {
         if(rsp_register(fd, (const unsigned char*)poolHandle, strlen(poolHandle),
                        (const struct rsp_loadinfo*)&loadInfo, reregInterval, 0) == 0) {
            for(;;) {
               char buffer[65536];
               ssize_t received;
               ssize_t sent;
               flags = 0;
               received = rsp_recvfullmsg(fd, (char*)&buffer, sizeof(buffer),
                                          &rinfo, &flags, -1);
               if(received > 0) {
                  if(flags & MSG_RSERPOOL_NOTIFICATION) {
                     /* RSerPool notification */
                     printf("NOTIFICATION: ");
                     rsp_print_notification((const union rserpool_notification*)&buffer, stdout);
                     puts("");
                  }
                  else if(flags & MSG_RSERPOOL_COOKIE_ECHO) {
                     /* Cookie Echo */
                  }
                  else {
                     /* Regular data message -> echo it back */
                     printf("Echoing %d bytes\n", (int)received);
                     sent = rsp_sendmsg(fd, buffer, received, 0,
                                        rinfo.rinfo_session, rinfo.rinfo_ppid, rinfo.rinfo_stream,
                                        0, 0, 0);
                  }
               }
            }
         }
         else {
            fprintf(stderr, "ERROR: Failed to register PE to pool %s\n", poolHandle);
         }
      }
      rsp_close(fd);
   }
   else {
      fputs("ERROR: Failed to create RSerPool socket\n", stderr);
   }


   rsp_cleanup();
   return 0;
}
