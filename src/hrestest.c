/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2025 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#include "rserpool.h"
#include "breakdetector.h"
#include "timeutilities.h"
#include "randomizer.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info      info;
   struct rsp_addrinfo* rspAddrInfo;
   struct rsp_addrinfo* r;
   const char*          poolHandle                   = "EchoPool";
   unsigned int         pause                        = 0;
   double               reportUnreachableProbability = 0.0;
   int                  i;
   int                  result;
   FILE*                statsFile     = stdout;
   unsigned int         statsInterval = 1000;
   unsigned long long   statsLine     = 1;
   unsigned long long   startTimeStamp;
   unsigned long long   updateTimeStamp;
   unsigned long long   responseTimeStamp;
   unsigned long long   successCount = 0;
   unsigned long long   failureCount = 0;
   unsigned long long   runs         = 0;
   unsigned long long   maxRuns      = 0;
   bool                 quiet        = false;
   bool                 printResults = false;
   size_t               items        = 0;

   rsp_initinfo(&info);
   for(i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
      }
      else if(!(strncmp(argv[i], "-poolhandle=" , 12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-pause=" , 7))) {
         pause = atol((char*)&argv[i][7]);
      }
      else if(!(strncmp(argv[i], "-items=" , 7))) {
         items = atol((char*)&argv[i][7]);
      }
      else if(!(strncmp(argv[i], "-maxruns=" , 9))) {
         maxRuns = atol((char*)&argv[i][9]);
      }
      else if(!(strncmp(argv[i], "-reportunreachableprobability=" , 30))) {
         reportUnreachableProbability = atof((char*)&argv[i][30]);
         if(reportUnreachableProbability > 1.0) {
            reportUnreachableProbability = 1.0;
         }
         else if(reportUnreachableProbability < 0.0) {
            reportUnreachableProbability = 0.0;
         }
      }
      else if(!(strcmp(argv[i], "-quiet"))) {
         quiet = true;
      }
      else if(!(strcmp(argv[i], "-printresults"))) {
         printResults = true;
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
         fprintf(stderr, "Usage: %s {-poolhandle=pool handle} {-items=Items} {-printresults} {-pause=microseconds} {-reportunreachableprobability=probability=probability} {-maxruns=Runs} {-statsfile=file} {-statsinterval=millisecs}\n", argv[0]);
         exit(1);
      }
   }


   if(!quiet) {
      puts("RSerPool Handle Resolution Tester - Version 1.0");
      puts("===============================================\n");
      printf("Pool Handle                    = %s\n",   poolHandle);
      printf("Items                          = %u\n", (unsigned int)items);
      printf("Pause                          = %ums\n", pause);
      printf("Max Runs                       = %llu\n", maxRuns);
      printf("Report Unreachable Probability = %1.2lf%%\n", reportUnreachableProbability * 100.0);
      printf("Statistics Interval            = %ums\n", statsInterval);
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
      result = rsp_getaddrinfo((const unsigned char*)poolHandle, strlen(poolHandle),
                               &rspAddrInfo, items, 0);
      responseTimeStamp = getMicroTime();
      runs++;

      if(result > 0) {
         successCount += (size_t)result;

         if(printResults) {
            printf("#%llu: Selected %d items\n", runs, result);
            r = rspAddrInfo;
            while(r != NULL) {
               printf("   ID $%08x\n", r->ai_pe_id);
               r = r->ai_next;
            }
         }

         if(reportUnreachableProbability > 0.0) {
            r = rspAddrInfo;
            while(r != NULL) {
               if(randomDouble() <= reportUnreachableProbability) {
                  rsp_pe_failure((const unsigned char*)poolHandle, strlen(poolHandle), r->ai_pe_id);
                  if(printResults) {
                     printf("   => Endpoint Unreachable for ID $%08x\n", r->ai_pe_id);
                  }
               }
               r = r->ai_next;
            }
         }

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

      if((maxRuns > 0) && (runs >= maxRuns)) {
         break;
      }

      if(pause > 0) {
         usleep(1000 * pause);
      }
   }

   puts("\nTerminated!");
   rsp_freeinfo(&info);
   if(statsFile != stdout) {
      fclose(statsFile);
   }
   return 0;
}
