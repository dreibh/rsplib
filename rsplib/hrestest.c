/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
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
#include "breakdetector.h"
#include "timeutilities.h"


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info      info;
   struct rsp_addrinfo* rspAddrInfo;
   char*                poolHandle = "EchoPool";
   unsigned int         pause      = 0;
   int                  i;
   int                  result;
   FILE*                statsFile     = stdout;
   unsigned int         statsInterval = 1000;
   unsigned long long   statsLine     = 1;
   unsigned long long   startTimeStamp;
   unsigned long long   updateTimeStamp;
   unsigned long long   requestTimeStamp;
   unsigned long long   responseTimeStamp;
   unsigned long long   successCount = 0;
   unsigned long long   failureCount = 0;
   bool                 quiet        = false;

   rsp_initinfo(&info);
   for(i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-pause=" ,7))) {
         pause = atol((char*)&argv[i][7]);
      }
      else if(!(strcmp(argv[i], "-quiet"))) {
         quiet = true;
      }
      else if(!(strncmp(argv[i], "-statsfile=", 11))) {
         if(statsFile != stdout) {
            fclose(statsFile);
         }
         statsFile = fopen((const char*)&argv[i][11], "w");
         if(statsFile == NULL) {
            fprintf(stderr, "ERROR: Unable to create statistics file \"%s\"!\n",
                    (char*)&argv[i][11]);
         }
      }
      else if(!(strncmp(argv[i], "-statsinterval=", 15))) {
         statsInterval = atol((const char*)&argv[i][15]);
         if(statsInterval < 10) {
            statsInterval = 10;
         }
         else if(statsInterval > 3600000) {
            statsInterval = 36000000;
         }
      }
      else {
         fprintf(stderr, "Usage: %s {-poolhandle=pool handle} {-pause=microsecs} {-statsfile=file} {-statsinterval=millisecs}\n", argv[i]);
         exit(1);
      }
   }


   if(!quiet) {
      puts("RSerPool Handle Resolution Tester - Version 1.0");
      puts("===============================================\n");
      printf("Pool Handle         = %s\n", poolHandle);
      printf("Pause               = %uus\n", pause);
      printf("Statistics Interval = %ums\n", statsInterval);
      puts(""); 
   }


   if(rsp_initialize(&info) < 0) {
      fputs("ERROR: Unable to initialize rsplib\n", stderr);
      exit(1);
   }

   fputs("AbsTime RelTime Success Failure\n", statsFile);


   installBreakDetector();
   startTimeStamp  = getMicroTime();
   updateTimeStamp = 0;
   while(!breakDetected()) {
      requestTimeStamp = getMicroTime();
      result = rsp_getaddrinfo((const unsigned char*)poolHandle, strlen(poolHandle), &rspAddrInfo);
      responseTimeStamp = getMicroTime();

      if(result == 0) {
         successCount++;
         rsp_freeaddrinfo(rspAddrInfo);
      }
      else {
         failureCount++;
      }

      if(responseTimeStamp - updateTimeStamp >= 1000ULL * statsInterval) {
         fprintf(statsFile,
                 "%06llu   %1.6f %1.6f   %llu %llu\n",
                 statsLine++,
                 responseTimeStamp / 1000000.0,
                 (responseTimeStamp - startTimeStamp) / 1000000.0,
                 successCount, failureCount);
         fflush(statsFile);
         updateTimeStamp = responseTimeStamp;
      }
      if(pause > 0) {
         usleep(pause);
      }
   }

   puts("\nTerminated!");
   rsp_freeinfo(&info);
   if(statsFile != stdout) {
      fclose(statsFile);
   }
   return 0;
}
