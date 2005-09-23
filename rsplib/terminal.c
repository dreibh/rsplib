/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id: t7.cc 797 2005-09-23 16:23:38Z dreibh $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include "rserpool.h"
#include "loglevel.h"
#include "breakdetector.h"


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info info;
   char            buffer[65536 + 4];
   char*           poolHandle = "EchoPool";
   struct pollfd   ufds[2];
   ssize_t         received;
   ssize_t         sent;
   int             result;
   int             sd;
   int             n;

   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-log" ,4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[n], "-poolhandle" ,11))) {
         poolHandle = (char*)&argv[n][11];
      }
   }


   puts("RSerPool Terminal - Version 1.0");
   puts("===============================\n");
   printf("Pool Handle = %s\n", poolHandle);
   puts("");


   beginLogging();

   memset(&info, 0, sizeof(info));
   rsp_initialize(&info, NULL);

   sd = rsp_socket(AF_INET, SOCK_DGRAM, IPPROTO_SCTP);
   if(sd < 0) {
      logerror("Unable to create RSerPool socket");
      exit(1);
   }
   if(rsp_connect(sd, (unsigned char*)poolHandle, strlen(poolHandle), NULL) < 0) {
      logerror("Unable to connect to pool element");
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
printf("sent=%d\n",sent);
               if(sent < received) {
                  puts("Write Failure -> Failover!");
                  rsp_forcefailover(sd, NULL);
               }
            }
            else {
               break;
            }
         }
         if(ufds[1].revents & POLLIN) {
            received = rsp_recv(sd, (char*)&buffer, sizeof(buffer), 0);
printf("recv=%d\n", received);
            if(received > 0) {
               printf("\x1b[34m");
               write(STDIN_FILENO, buffer, received);
               printf("\x1b[0m");
            }
            else {
               puts("Read Failure -> Failover!");
               rsp_forcefailover(sd, NULL);
            }
         }
      }
   }

   puts("\nTerminated!");
   rsp_close(sd);
   rsp_cleanup();
   finishLogging();
   return 0;
}
