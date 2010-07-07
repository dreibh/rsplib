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
 * Copyright (C) 2002-2010 by Thomas Dreibholz
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
#include "tagitem.h"
#include "netutilities.h"
#include "stringutilities.h"
#include "randomizer.h"
#include "standardservices.h"
#include "fractalgeneratorservice.h"
#include "calcappservice.h"
#include "scriptingservice.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <signal.h>


#define SERVICE_ECHO      1
#define SERVICE_DISCARD   2
#define SERVICE_DAYTIME   3
#define SERVICE_CHARGEN   4
#define SERVICE_PINGPONG  5
#define SERVICE_FRACTAL   6
#define SERVICE_CALCAPP   7
#define SERVICE_SCRIPTING 8


/* ###### Count the number of CPUs/cores ################################# */
static size_t getNumberOfCPUs()
{
   const size_t numberOfCPUs = sysconf(_SC_NPROCESSORS_ONLN);
   return( (numberOfCPUs > 0) ? numberOfCPUs : 1 );
}


#define MAX_PE_TRANSPORTADDRESSES 64

/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info     info;
   struct rsp_loadinfo loadInfo;
   struct TagItem      tags[8];
   char                localAddressBuffer[sizeof(union sockaddr_union) * MAX_PE_TRANSPORTADDRESSES];
   struct sockaddr*    localAddressSet = (struct sockaddr*)&localAddressBuffer;
   size_t              localAddresses  = 0;
   unsigned int        reregInterval   = 30000;
   unsigned int        runtimeLimit    = 0;
   unsigned int        service         = SERVICE_ECHO;
   const char*         poolHandle      = NULL;
   const char*         daemonPIDFile   = NULL;
   bool                policyChanged   = false;
   bool                quiet           = false;
   double              uptime          = 0.0;
   double              downtime        = 0.0;
   double              degradation;
   unsigned int        identifier;

start:
   /* ====== Read parameters ============================================= */
   rsp_initinfo(&info);
   tags[0].Tag  = TAG_PoolElement_Identifier;
   tags[0].Data = 0;
   tags[1].Tag  = TAG_DONE;
#ifdef ENABLE_CSP
   info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, random32());
#endif
   memset(&loadInfo, 0, sizeof(loadInfo));
   loadInfo.rli_policy = PPT_ROUNDROBIN;
   for(int i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
      }
      if(!(strncmp(argv[i], "-identifier=", 12))) {
         if(sscanf((const char*)&argv[i][12], "0x%x", &identifier) == 0) {
            if(sscanf((const char*)&argv[i][12], "%u", &identifier) == 0) {
               fputs("ERROR: Bad registrar ID given!\n", stderr);
               exit(1);
            }
         }
         tags[0].Data = identifier;
#ifdef ENABLE_CSP
         info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, tags[0].Data);
#endif
      }
      else if(!(strncmp(argv[i], "-local=" ,7))) {
         localAddresses = 0;
         sockaddr* addressPtr    = localAddressSet;
         char*     addressString = (char*)&argv[i][7];
         while(localAddresses < MAX_PE_TRANSPORTADDRESSES) {
            char* idx = strindex(addressString, ',');
            if(idx) {
               *idx = 0x00;
            }
            if(!string2address(addressString, (union sockaddr_union*)addressPtr)) {
               fprintf(stderr, "ERROR: Bad local address %s! Use format <address:port>.\n", addressString);
               exit(1);
            }
            localAddresses++;
            if(idx) {
               addressString = idx;
               addressString++;
            }
            else {
               break;
            }
            addressPtr = (struct sockaddr*)((long)addressPtr + (long)getSocklen(addressPtr));
         }
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
      else if(!(strncmp(argv[i], "-daemonpidfile=", 15))) {
         daemonPIDFile = (const char*)&argv[i][15];
      }
      else if(!(strncmp(argv[i], "-policy=" ,8))) {
         double dpf;

         if(!(strcmp((const char*)&argv[i][8], "RoundRobin"))) {
            loadInfo.rli_policy = PPT_ROUNDROBIN;
         }
         else if(!(strcmp((const char*)&argv[i][8], "Random"))) {
            loadInfo.rli_policy = PPT_RANDOM;
         }
         else if(!(strcmp((const char*)&argv[i][8], "LeastUsed"))) {
            loadInfo.rli_policy = PPT_LEASTUSED;
         }
         else if(!(strcmp((const char*)&argv[i][8], "RandomizedLeastUsed"))) {
            loadInfo.rli_policy = PPT_RANDOMIZED_LEASTUSED;
         }
         else if(sscanf((const char*)&argv[i][8], "LeastUsedDegradation:%lf", &degradation) == 1) {
            loadInfo.rli_load_degradation = (unsigned int)rint(degradation * (double)PPV_MAX_LOAD_DEGRADATION);
            if(loadInfo.rli_load_degradation < 0) {
               fputs("ERROR: Bad LUD degradation value!\n", stderr);
               exit(1);
            }
            else if(loadInfo.rli_load_degradation > PPV_MAX_LOAD_DEGRADATION) {
               fputs("ERROR: Bad LUD degradation value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_policy = PPT_LEASTUSED_DEGRADATION;
         }
         else if(sscanf((const char*)&argv[i][8], "PriorityLeastUsed:%lf", &degradation) == 1) {
            loadInfo.rli_load_degradation = (unsigned int)rint(degradation * (double)PPV_MAX_LOAD_DEGRADATION);
            if(loadInfo.rli_load_degradation < 0) {
               fputs("ERROR: Bad PLU degradation value!\n", stderr);
               exit(1);
            }
            else if(loadInfo.rli_load_degradation > PPV_MAX_LOAD_DEGRADATION) {
               fputs("ERROR: Bad PLU degradation value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_policy = PPT_PRIORITY_LEASTUSED;
         }
         else if(!(strcmp((const char*)&argv[i][8], "RandomizedPriorityLeastUsed"))) {
            loadInfo.rli_policy = PPT_RANDOMIZED_PRIORITY_LEASTUSED;
         }
         else if(sscanf((const char*)&argv[i][8], "LeastUsedDPF:%lf", &dpf) == 1) {
            if((dpf < 0.0) || (dpf > 1.0)) {
               fputs("ERROR: Bad LU-DPF DPF value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_load_dpf = (unsigned int)rint(dpf * (double)PPV_MAX_LOADDPF);
            loadInfo.rli_policy = PPT_LEASTUSED_DPF;
         }
         else if(sscanf((const char*)&argv[i][8], "LeastUsedDegradationDPF:%lf:%lf", &degradation, &dpf) == 2) {
            if((dpf < 0.0) || (dpf > 1.0)) {
               fputs("ERROR: Bad LU-DPF DPF value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_load_dpf         = (unsigned int)rint(dpf * (double)PPV_MAX_LOADDPF);
            loadInfo.rli_load_degradation = (unsigned int)rint(degradation * (double)PPV_MAX_LOAD_DEGRADATION);
            if(loadInfo.rli_load_degradation < 0) {
               fputs("ERROR: Bad LU-DPF degradation value!\n", stderr);
               exit(1);
            }
            else if(loadInfo.rli_load_degradation > PPV_MAX_LOAD_DEGRADATION) {
               fputs("ERROR: Bad LU-DPF degradation value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_policy = PPT_LEASTUSED_DEGRADATION_DPF;
         }
         else if(sscanf((const char*)&argv[i][8], "WeightedRoundRobin:%u", &loadInfo.rli_weight) == 1) {
            loadInfo.rli_policy = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if(sscanf((const char*)&argv[i][8], "Priority:%u", &loadInfo.rli_weight) == 1) {
            loadInfo.rli_policy = PPT_PRIORITY;
         }
         else if(sscanf((const char*)&argv[i][8], "WeightedRandom:%u", &loadInfo.rli_weight) == 1) {
            loadInfo.rli_policy = PPT_WEIGHTED_RANDOM;
         }
         else if(sscanf((const char*)&argv[i][8], "WeightedRandomDPF:%u:%lf", &loadInfo.rli_weight, &dpf) == 2) {
            if((dpf < 0.0) || (dpf > 1.0)) {
               fputs("ERROR: Bad WRAND-DPF DPF value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_weight_dpf = (unsigned int)rint(dpf * (double)PPV_MAX_WEIGHTDPF);
            loadInfo.rli_policy = PPT_WEIGHTED_RANDOM_DPF;
         }
         else {
            fprintf(stderr, "ERROR: Bad policy setting <%s>!\n", argv[i]);
            exit(1);
         }
         policyChanged = true;
      }
      else if(!(strncmp(argv[i], "-runtime=" ,9))) {
         runtimeLimit = atol((const char*)&argv[i][9]);
      }
      else if(!(strcmp(argv[i], "-echo"))) {
         service = SERVICE_ECHO;
      }
      else if(!(strcmp(argv[i], "-discard"))) {
         service = SERVICE_DISCARD;
      }
      else if(!(strcmp(argv[i], "-daytime"))) {
         service = SERVICE_DAYTIME;
      }
      else if(!(strcmp(argv[i], "-chargen"))) {
         service = SERVICE_CHARGEN;
      }
      else if(!(strcmp(argv[i], "-pingpong"))) {
         service = SERVICE_PINGPONG;
      }
      else if(!(strcmp(argv[i], "-fractal"))) {
         service = SERVICE_FRACTAL;
      }
      else if(!(strcmp(argv[i], "-calcapp"))) {
         service = SERVICE_CALCAPP;
      }
      else if(!(strcmp(argv[i], "-scripting"))) {
         service = SERVICE_SCRIPTING;
      }
      else if(!(strcmp(argv[i], "-quiet"))) {
         quiet = true;
      }
      else if(!(strncmp(argv[i], "-uptime=", 8))) {
         uptime = atof((const char*)&argv[i][8]);
      }
      else if(!(strncmp(argv[i], "-downtime=", 10))) {
         downtime = atof((const char*)&argv[i][10]);
      }
   }
   if(uptime > 0.000001) {
      runtimeLimit = (unsigned long long)rint(1000.0 * randomExpDouble(uptime));
   }


   /* ====== Print startup message ======================================= */
   if(!quiet) {
      if(uptime > 0.000001) {
         printf("Uptime   = %1.3f [s]\n", uptime);
         printf("Downtime = %1.3f [s]\n", downtime);
      }
      printf("Starting service ");
      if(service == SERVICE_ECHO) {
         printf("Echo");
      }
      else if(service == SERVICE_DISCARD) {
         printf("Discard");
      }
      else if(service == SERVICE_DAYTIME) {
         printf("Daytime");
      }
      else if(service == SERVICE_CHARGEN) {
         printf("Character Generator");
      }
      puts("...");
   }


   if(daemonPIDFile != NULL) {
      const pid_t childProcess = fork();
      if(childProcess != 0) {
         FILE* fh = fopen(daemonPIDFile, "w");
         if(fh) {
            fprintf(fh, "%d\n", childProcess);
            fclose(fh);
         }
         else {
            kill(childProcess, SIGKILL);
            fprintf(stderr, "ERROR: Unable to create PID file \"%s\"!\n", daemonPIDFile);
         }
         exit(0);
      }
      else {
         signal(SIGHUP, SIG_IGN);
      }
   }


   /* ====== Start requested service ===================================== */
   if(service == SERVICE_ECHO) {
      EchoServer echoServer;
      echoServer.poolElement("Echo Server - Version 1.0",
                             (poolHandle != NULL) ? poolHandle : "EchoPool",
                             &info, &loadInfo,
                             localAddressSet, localAddresses,
                             reregInterval, runtimeLimit, quiet, (daemonPIDFile != NULL),
                             (struct TagItem*)&tags);
   }
   else if(service == SERVICE_DISCARD) {
      DiscardServer discardServer;
      discardServer.poolElement("Discard Server - Version 1.0",
                                (poolHandle != NULL) ? poolHandle : "DiscardPool",
                                &info, &loadInfo,
                                localAddressSet, localAddresses,
                                reregInterval, runtimeLimit, quiet, (daemonPIDFile != NULL),
                                (struct TagItem*)&tags);
   }
   else if(service == SERVICE_DAYTIME) {
      DaytimeServer daytimeServer;
      daytimeServer.poolElement("Daytime Server - Version 1.0",
                                (poolHandle != NULL) ? poolHandle : "DaytimePool",
                                &info, &loadInfo,
                                localAddressSet, localAddresses,
                                reregInterval, runtimeLimit, quiet, (daemonPIDFile != NULL),
                                (struct TagItem*)&tags);
   }
   else if(service == SERVICE_CHARGEN) {
      size_t maxThreads = 4;
      for(int i = 1;i < argc;i++) {
         if(!(strncmp(argv[i], "-chargenmaxthreads=", 19))) {
            maxThreads = atol((const char*)&argv[i][19]);
         }
      }

      TCPLikeServer::poolElement("Character Generator Server - Version 1.0",
                                  (poolHandle != NULL) ? poolHandle : "CharGenPool",
                                  &info, &loadInfo,
                                  maxThreads,
                                  CharGenServer::charGenServerFactory,
                                  NULL, NULL, NULL, NULL, NULL, NULL,
                                  localAddressSet, localAddresses,
                                  reregInterval, runtimeLimit, quiet, (daemonPIDFile != NULL),
                                  (struct TagItem*)&tags);
   }
   else if(service == SERVICE_PINGPONG) {
      size_t maxThreads = 4;
      PingPongServer::PingPongServerSettings settings;
      settings.FailureAfter = 0;
      for(int i = 1;i < argc;i++) {
         if(!(strncmp(argv[i], "-pppfailureafter=", 17))) {
            settings.FailureAfter = atol((const char*)&argv[i][17]);
         }
         else if(!(strncmp(argv[i], "-pppmaxthreads=", 15))) {
            maxThreads = atol((const char*)&argv[i][15]);
         }
      }

      TCPLikeServer::poolElement("Ping Pong Server - Version 1.0",
                                 (poolHandle != NULL) ? poolHandle : "PingPongPool",
                                 &info, &loadInfo,
                                 maxThreads,
                                 PingPongServer::pingPongServerFactory,
                                 NULL, NULL, NULL, NULL, NULL,
                                 (void*)&settings,
                                 localAddressSet, localAddresses,
                                 reregInterval, runtimeLimit, quiet, (daemonPIDFile != NULL),
                                 (struct TagItem*)&tags);
   }
   else if(service == SERVICE_FRACTAL) {
      size_t maxThreads = 4;
      FractalGeneratorServer::FractalGeneratorServerSettings settings;
      settings.TestMode         = false;
      settings.FailureAfter     = 0;
      settings.TransmitTimeout  = 2500;
      settings.DataMaxTime      = 1000000;
      settings.CookieMaxTime    = 1000000;
      settings.CookieMaxPackets = 10;
      for(int i = 1;i < argc;i++) {
         if(!(strcmp(argv[i], "-fgptestmode"))) {
            settings.TestMode = true;
         }
         else if(!(strncmp(argv[i], "-fgptransmittimeout=", 20))) {
            settings.TransmitTimeout = atol((const char*)&argv[i][20]);
         }
         else if(!(strncmp(argv[i], "-fgpdatamaxtime=", 16))) {
            settings.DataMaxTime = 1000ULL * atol((const char*)&argv[i][16]);
            if(settings.DataMaxTime < 100000) {
               settings.DataMaxTime = 100000;
            }
         }
         else if(!(strncmp(argv[i], "-fgpcookiemaxtime=", 18))) {
            settings.CookieMaxTime = 1000ULL * atol((const char*)&argv[i][18]);
            if(settings.CookieMaxTime < 100000) {
               settings.CookieMaxTime = 100000;
            }
         }
         else if(!(strncmp(argv[i], "-fgpcookiemaxpackets=", 21))) {
            settings.CookieMaxPackets = atol((const char*)&argv[i][21]);
            if(settings.CookieMaxPackets < 1) {
               settings.CookieMaxPackets = 1;
            }
         }
         else if(!(strncmp(argv[i], "-fgpmaxthreads=", 15))) {
            maxThreads = atol((const char*)&argv[i][15]);
         }
         else if(!(strncmp(argv[i], "-fgpfailureafter=", 17))) {
            settings.FailureAfter = atol((const char*)&argv[i][17]);
         }
      }

      TCPLikeServer::poolElement("Fractal Generator Server - Version 1.0",
                                 (poolHandle != NULL) ? poolHandle : "FractalGeneratorPool",
                                 &info, &loadInfo,
                                 maxThreads,
                                 FractalGeneratorServer::fractalGeneratorServerFactory,
                                 FractalGeneratorServer::fractalGeneratorPrintParameters,
                                 NULL, NULL, NULL, NULL,
                                 (void*)&settings,
                                 localAddressSet, localAddresses,
                                 reregInterval, runtimeLimit, quiet, (daemonPIDFile != NULL),
                                 (struct TagItem*)&tags);
   }
   else if(service == SERVICE_CALCAPP) {
      static CalcAppServer::CalcAppServerStatistics stats;
      static bool        resetStatistics               = true;
      const char*        objectName                    = "scenario.calcapppoolelement[0]";
      const char*        vectorFileName                = "calcapppoolelement.vec";
      const char*        scalarFileName                = "calcapppoolelement.sca";
      double             capacity                      = 1000000.0;
      unsigned long long keepAliveTransmissionInterval = 2500000;
      unsigned long long keepAliveTimeoutInterval      = 2500000;
      unsigned long long cookieMaxTime                 = 5000000;
      double             cookieMaxCalculations         = 5000000.0;
      double             cleanShutdownProbability      = 1.0;
      size_t             maxJobs                       = 10;
      for(int i = 1;i < argc;i++) {
         if(!(strncmp(argv[i], "-capobject=" ,11))) {
            objectName = (const char*)&argv[i][11];
         }
         else if(!(strncmp(argv[i], "-capvector=" ,11))) {
            vectorFileName = (const char*)&argv[i][11];
         }
         else if(!(strncmp(argv[i], "-capscalar=" ,11))) {
            scalarFileName = (const char*)&argv[i][11];
         }
         else if(!(strncmp(argv[i], "-capmaxjobs=" ,12))) {
            maxJobs = atol((const char*)&argv[i][12]);
            if(maxJobs < 1) {
               maxJobs = 1;
            }
         }
         else if(!(strncmp(argv[i], "-capkeepalivetransmissioninterval=" ,34))) {
            keepAliveTransmissionInterval = 1000ULL * atol((const char*)&argv[i][34]);
            if(keepAliveTransmissionInterval < 100000) {
               keepAliveTransmissionInterval = 100000;
            }
         }
         else if(!(strncmp(argv[i], "-capkeepalivetimeoutinterval=" ,29))) {
            keepAliveTimeoutInterval = 1000ULL * atol((const char*)&argv[i][29]);
            if(keepAliveTimeoutInterval < 100000) {
               keepAliveTimeoutInterval = 100000;
            }
         }
         else if(!(strncmp(argv[i], "-capcapacity=" ,13))) {
            capacity = atof((const char*)&argv[i][13]);
            if(capacity < 1.0) {
               capacity = 1.0;
            }
         }
         else if(!(strncmp(argv[i], "-capcookiemaxcalculations=" ,26))) {
            cookieMaxCalculations = atof((const char*)&argv[i][26]);
            if(cookieMaxCalculations < 1.0) {
               cookieMaxCalculations = 1.0;
            }
         }
         else if(!(strncmp(argv[i], "-capcookiemaxtime=" ,18))) {
            cookieMaxTime = (unsigned long long)rint(1000000.0 * atof((const char*)&argv[i][18]));
            if(cookieMaxTime < 100000) {
               cookieMaxTime = 100000;
            }
         }
         else if(!(strncmp(argv[i], "-capcleanshutdownprobability=" , 29))) {
            cleanShutdownProbability = atof((const char*)&argv[i][29]);
            if(cleanShutdownProbability < 0.0) {
               cleanShutdownProbability = 0.0;
            }
            else if(cleanShutdownProbability > 1.0) {
               cleanShutdownProbability = 1.0;
            }
         }
      }

      CalcAppServer calcAppServer(maxJobs, objectName, vectorFileName, scalarFileName,
                                  capacity,
                                  keepAliveTransmissionInterval, keepAliveTimeoutInterval,
                                  cookieMaxTime, cookieMaxCalculations, cleanShutdownProbability,
                                  &stats, resetStatistics);
      calcAppServer.poolElement("CalcApp Server - Version 1.0",
                                (poolHandle != NULL) ? poolHandle : "CalcAppPool",
                                &info, &loadInfo,
                                localAddressSet, localAddresses,
                                reregInterval, runtimeLimit, quiet, (daemonPIDFile != NULL),
                                (struct TagItem*)&tags);
      resetStatistics = false;
   }
   else if(service == SERVICE_SCRIPTING) {
      size_t maxThreads = getNumberOfCPUs();
      ScriptingServer::ScriptingServerSettings settings;
      settings.KeepTempDirs      = false;
      settings.VerboseMode       = false;
      settings.TransmitTimeout   = 30000;
      settings.KeepAliveInterval = 10000;
      settings.KeepAliveTimeout  = 10000;
      settings.CacheMaxSize      = 100000000;
      settings.CacheMaxEntries   = 10;
      settings.CacheDirectory    = "";
      for(int i = 1;i < argc;i++) {
         if(!(strncmp(argv[i], "-ssmaxthreads=", 14))) {
            maxThreads = atol((const char*)&argv[i][14]);
            if(maxThreads < 1) {
               maxThreads = 1;
            }
         }
         else if(!(strcmp(argv[i], "-sskeeptempdirs"))) {
            settings.KeepTempDirs = true;
         }
         else if(!(strcmp(argv[i], "-ssverbose"))) {
            settings.VerboseMode = true;
         }
         else if(!(strncmp(argv[i], "-sstransmittimeout=", 19))) {
            settings.TransmitTimeout = atol((const char*)&argv[i][19]);
         }
         else if(!(strncmp(argv[i], "-sskeepaliveinterval=", 21))) {
            settings.KeepAliveInterval = atol((const char*)&argv[i][21]);
         }
         else if(!(strncmp(argv[i], "-sskeepalivetimeout=", 20))) {
            settings.KeepAliveTimeout = atol((const char*)&argv[i][20]);
         }
         else if(!(strncmp(argv[i], "-sscachemaxsize=", 16))) {
            settings.CacheMaxSize = 1024ULL * atoll((const char*)&argv[i][16]);
            if(settings.CacheMaxSize < 128*1024) {
               settings.CacheMaxSize = 128*1024;
            }
         }
         else if(!(strncmp(argv[i], "-sscachemaxentries=", 19))) {
            settings.CacheMaxEntries = atol((const char*)&argv[i][19]);
            if(settings.CacheMaxEntries < 1) {
               settings.CacheMaxEntries = 1;
            }
         }
         else if(!(strncmp(argv[i], "-sscachedirectory=", 18))) {
            settings.CacheDirectory = (const char*)&argv[i][18];
         }
      }
      if(!policyChanged) {
         loadInfo.rli_load_degradation = (unsigned int)rint((1.0 / maxThreads) * (double)PPV_MAX_LOAD_DEGRADATION);
         loadInfo.rli_policy = PPT_LEASTUSED_DEGRADATION;
      }

      TCPLikeServer::poolElement("Scripting Server - Version 2.0",
                                 (poolHandle != NULL) ? poolHandle : "ScriptingPool",
                                 &info, &loadInfo,
                                 maxThreads,
                                 ScriptingServer::scriptingServerFactory,
                                 ScriptingServer::scriptingPrintParameters,
                                 ScriptingServer::rejectNewSession,
                                 ScriptingServer::initializeService,
                                 ScriptingServer::finishService,
                                 NULL,
                                 (void*)&settings,
                                 localAddressSet, localAddresses,
                                 reregInterval, runtimeLimit, quiet, (daemonPIDFile != NULL),
                                 (struct TagItem*)&tags);
   }

   rsp_freeinfo(&info);

   // ====== Restart ========================================================
   if((uptime > 0.000001) && (!breakDetected())) {
      const double t = randomExpDouble(downtime);
      printf("\nWaiting for %1.3fs ...\n", t);
      usleep((unsigned long long)rint(t * 1000000.0));
      if(!breakDetected()) {
         goto start;
      }
   }
   return(0);
}
