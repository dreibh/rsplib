/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2018 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#include "rserpool.h"
#include "breakdetector.h"
#include "randomizer.h"
#include "netutilities.h"
#ifdef ENABLE_CSP
#include "componentstatuspackets.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info              info;
   struct rsp_sndrcvinfo        rinfo;
   union rserpool_notification* notification;
   char                         buffer[65536 + 4];
   const char*                  poolHandle = "EchoPool";
   struct pollfd                ufds[2];
   ssize_t                      received;
   ssize_t                      sent;
   size_t                       failovers = 0;
   int                          result;
   int                          flags;
   int                          sd;
   int                          i;

   rsp_initinfo(&info);
   for(i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else {
         fprintf(stderr, "ERROR: Bad argument %s\n", argv[i]);
         exit(1);
      }
   }
#ifdef ENABLE_CSP
   if(CID_OBJECT(info.ri_csp_identifier) == 0ULL) {
      info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLUSER, random64());
   }
#endif


   puts("RSerPool Terminal - Version 1.0");
   puts("===============================\n");
   printf("Pool Handle = %s\n\n", poolHandle);


   if(rsp_initialize(&info) < 0) {
      fputs("ERROR: Unable to initialize rsplib\n", stderr);
      exit(1);
   }

   sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(sd < 0) {
      perror("Unable to create RSerPool socket");
      exit(1);
   }
   if(rsp_connect(sd, (unsigned char*)poolHandle, strlen(poolHandle), 0) < 0) {
      perror("Unable to connect to pool element");
      exit(1);
   }

   installBreakDetector();
   while(!breakDetected()) {
      ufds[0].fd     = STDIN_FILENO;
      ufds[0].events = POLLIN;
      ufds[1].fd     = sd;
      ufds[1].events = POLLIN;
      result = rsp_poll((struct pollfd*)&ufds, 2, -1);
      if(result > 0) {
         if(ufds[0].revents & POLLIN) {
            received = rsp_read(STDIN_FILENO, (char*)&buffer, sizeof(buffer));
            if(received > 0) {
               sent = rsp_write(sd, (char*)&buffer, received);
               if(sent < received) {
                  perror("rsp_write() failed");
               }
            }
            else {
               break;
            }
         }
         if(ufds[1].revents & POLLIN) {
            flags = 0;
            received = rsp_recvmsg(sd, (char*)&buffer, sizeof(buffer),
                                   &rinfo, &flags, 0);
            if(received > 0) {
               if(flags & MSG_RSERPOOL_NOTIFICATION) {
                  notification = (union rserpool_notification*)&buffer;
                  printf("\x1b[39;47mNotification: ");
                  rsp_print_notification(notification, stdout);
                  puts("\x1b[0m");
                  if((notification->rn_header.rn_type == RSERPOOL_FAILOVER) &&
                     (notification->rn_failover.rf_state == RSERPOOL_FAILOVER_NECESSARY)) {
                     failovers++;
                     printf("FAILOVER #%u ...\n", (unsigned int)failovers);
/*static int yyy=0;yyy++;
if(yyy>=2) exit(1);*/
                     /* usleep(200000); */
                     rsp_forcefailover(sd, FFF_NONE, 0);
                  }
               }
               else {
                  /* ====== Replace non-printable characters ============= */
                  buffer[received] = 0x00;
                  for(i = received;i >= 0;i--) {
                     if(buffer[i] < ' ') {
                        buffer[i] = 0x00;
                     }
                     else {
                        break;
                     }
                  }
                  for(   ;i >= 0;i--) {
                     if((unsigned char)buffer[i] < 30) {
                        buffer[i] = '.';
                     }
                  }

                  printf("\x1b[34mfrom PE $%08x> %s\x1b[0m\n",
                         rinfo.rinfo_pe_id, buffer);
                  fflush(stdout);
               }
            }
            else {
               perror("rsp_read() failed");
            }
         }
      }
   }

   puts("\nTerminated!");
   rsp_close(sd);
   rsp_cleanup();
   rsp_freeinfo(&info);
   return 0;
}
