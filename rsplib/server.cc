/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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
#include "loglevel.h"
#include "breakdetector.h"
#include "tagitem.h"
#include "rsputilities.h"
#include "netutilities.h"
#include "standardservices.h"
#include "fractalgeneratorservice.h"
#include "calcappservice.h"


#define SERVICE_ECHO     1
#define SERVICE_DISCARD  2
#define SERVICE_DAYTIME  3
#define SERVICE_CHARGEN  4
#define SERVICE_PINGPONG 5
#define SERVICE_FRACTAL  6
#define SERVICE_CALCAPP  7


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info      info;
   struct rsp_loadinfo  loadInfo;
   union sockaddr_union asapAnnounceAddress;
   double               degradation;
   unsigned int         identifier;
   unsigned int         reregInterval = 30000;
   unsigned int         runtimeLimit  = 0;
   unsigned int         service       = SERVICE_ECHO;
   const char*          poolHandle    = NULL;
   struct TagItem       tags[8];

   /* ====== Read parameters ============================================= */
   memset(&info, 0, sizeof(info));
   tags[0].Tag  = TAG_PoolElement_Identifier;
   tags[0].Data = 0;
   tags[1].Tag  = TAG_DONE;
#ifdef ENABLE_CSP
   info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, 0);
#endif
   memset(&loadInfo, 0, sizeof(loadInfo));
   loadInfo.rli_policy = PPT_ROUNDROBIN;
   for(int i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-log" ,4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
#ifdef ENABLE_CSP
      if(!(strncmp(argv[i], "-csp" ,4))) {
         if(initComponentStatusReporter(&info, argv[i]) == false) {
            exit(1);
         }
      }
#endif
      else if(!(strncmp(argv[i], "-identifier=", 12))) {
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
      else if(!(strncmp(argv[i], "-registrar=", 11))) {
         if(addStaticRegistrar(&info, (char*)&argv[i][11]) < 0) {
            fprintf(stderr, "ERROR: Bad registrar setting %s\n", argv[i]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-asapannounce=", 14))) {
         if(string2address((char*)&argv[i][14], &asapAnnounceAddress) == false) {
            fprintf(stderr, "ERROR: Bad ASAP announce setting %s\n", argv[i]);
            exit(1);
         }
         info.ri_registrar_announce = (struct sockaddr*)&asapAnnounceAddress;
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-rereginterval=" ,15))) {
         reregInterval = atol((char*)&argv[i][15]);
         if(reregInterval < 250) {
            reregInterval = 250;
         }
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
            loadInfo.rli_load_degradation = (unsigned int)rint(degradation * (double)0xffffff);
            if(loadInfo.rli_load_degradation < 0) {
               fputs("ERROR: Bad LUD degradation value!\n", stderr);
               exit(1);
            }
            else if(loadInfo.rli_load_degradation > 0xffffff) {
               fputs("ERROR: Bad LUD degradation value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_policy = PPT_LEASTUSED_DEGRADATION;
         }
         else if(sscanf((const char*)&argv[i][8], "PriorityLeastUsed:%lf", &degradation) == 1) {
            loadInfo.rli_load_degradation = (unsigned int)rint(degradation * (double)0xffffff);
            if(loadInfo.rli_load_degradation < 0) {
               fputs("ERROR: Bad PLU degradation value!\n", stderr);
               exit(1);
            }
            else if(loadInfo.rli_load_degradation > 0xffffff) {
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
            loadInfo.rli_load_dpf = (unsigned int)rint(dpf * (double)0xffffffff);
            loadInfo.rli_policy = PPT_LEASTUSED_DPF;
         }
         else if(sscanf((const char*)&argv[i][8], "LeastUsedDegradationDPF:%lf:%lf", &degradation, &dpf) == 2) {
            if((dpf < 0.0) || (dpf > 1.0)) {
               fputs("ERROR: Bad LU-DPF DPF value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_load_dpf         = (unsigned int)rint(dpf * (double)0xffffffff);
            loadInfo.rli_load_degradation = (unsigned int)rint(degradation * (double)0xffffff);
            if(loadInfo.rli_load_degradation < 0) {
               fputs("ERROR: Bad LU-DPF degradation value!\n", stderr);
               exit(1);
            }
            else if(loadInfo.rli_load_degradation > 0xffffff) {
               fputs("ERROR: Bad LU-DPF degradation value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_policy = PPT_LEASTUSED_DEGRADATION_DPF;
         }
         else if(sscanf((const char*)&argv[i][8], "WeightedRoundRobin:%u", &loadInfo.rli_weight) == 1) {
            loadInfo.rli_policy = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if(sscanf((const char*)&argv[i][8], "WeightedRandom:%u", &loadInfo.rli_weight) == 1) {
            loadInfo.rli_policy = PPT_WEIGHTED_RANDOM;
         }
         else if(sscanf((const char*)&argv[i][8], "WeightedRandomDPF:%u:%lf", &loadInfo.rli_weight, &dpf) == 2) {
            if((dpf < 0.0) || (dpf > 1.0)) {
               fputs("ERROR: Bad WRAND-DPF DPF value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_weight_dpf = (unsigned int)rint(dpf * (double)0xffffffff);
            loadInfo.rli_policy = PPT_WEIGHTED_RANDOM_DPF;
         }
         else {
            fprintf(stderr, "ERROR: Bad policy setting <%s>!\n", argv[i]);
            exit(1);
         }
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
   }


   /* ====== Print startup message ======================================= */
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


   /* ====== Start requested service ===================================== */
   if(service == SERVICE_ECHO) {
      EchoServer echoServer;
      echoServer.poolElement("Echo Server - Version 1.0",
                             (poolHandle != NULL) ? poolHandle : "EchoPool",
                             &info, &loadInfo,
                             reregInterval, runtimeLimit,
                             (struct TagItem*)&tags);
   }
   else if(service == SERVICE_DISCARD) {
      DiscardServer discardServer;
      discardServer.poolElement("Discard Server - Version 1.0",
                                (poolHandle != NULL) ? poolHandle : "DiscardPool",
                                &info, &loadInfo,
                                reregInterval, runtimeLimit,
                                (struct TagItem*)&tags);
   }
   else if(service == SERVICE_DAYTIME) {
      DaytimeServer daytimeServer;
      daytimeServer.poolElement("Daytime Server - Version 1.0",
                                (poolHandle != NULL) ? poolHandle : "DaytimePool",
                                &info, &loadInfo,
                                reregInterval, runtimeLimit,
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
                                  NULL, NULL,
                                  reregInterval, runtimeLimit,
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
                                 NULL,
                                 (void*)&settings,
                                 reregInterval, runtimeLimit,
                                 (struct TagItem*)&tags);
   }
   else if(service == SERVICE_FRACTAL) {
      size_t maxThreads = 4;
      FractalGeneratorServer::FractalGeneratorServerSettings settings;
      settings.TestMode     = false;
      settings.FailureAfter = 0;
      for(int i = 1;i < argc;i++) {
         if(!(strcmp(argv[i], "-fgptestmode"))) {
            settings.TestMode = true;
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
                                 (void*)&settings,
                                 reregInterval, runtimeLimit,
                                 (struct TagItem*)&tags);
   }
   else if(service == SERVICE_CALCAPP) {
      const char*        objectName                    = "scenario.calcapppoolelement[0]";
      const char*        vectorFileName                = "calcapppoolelement.vec";
      const char*        scalarFileName                = "calcapppoolelement.sca";
      double             capacity                      = 1000000.0;
      unsigned long long keepAliveTransmissionInterval = 2500000;
      unsigned long long keepAliveTimeoutInterval      = 2500000;
      unsigned long long cookieMaxTime                 = 5000000;
      double             cookieMaxCalculations         = 5000000.0;
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
            maxJobs = atol((char*)&argv[i][12]);
            if(maxJobs < 1) {
               maxJobs = 1;
            }
         }
         else if(!(strncmp(argv[i], "-capkeepalivetransmissioninterval=" ,34))) {
            keepAliveTransmissionInterval = atol((char*)&argv[i][34]);
            if(keepAliveTransmissionInterval < 100000) {
               keepAliveTransmissionInterval = 100000;
            }
         }
         else if(!(strncmp(argv[i], "-capkeepalivetimeoutinterval=" ,29))) {
            keepAliveTimeoutInterval = atol((char*)&argv[i][29]);
            if(keepAliveTimeoutInterval < 100000) {
               keepAliveTimeoutInterval = 100000;
            }
         }
         else if(!(strncmp(argv[i], "-capcapacity=" ,13))) {
            capacity = atof((char*)&argv[i][13]);
            if(capacity < 1.0) {
               capacity = 1.0;
            }
         }
         else if(!(strncmp(argv[i], "-capcookiemaxcalculations=" ,26))) {
            cookieMaxCalculations = atof((char*)&argv[i][26]);
            if(cookieMaxCalculations < 1.0) {
               cookieMaxCalculations = 1.0;
            }
         }
         else if(!(strncmp(argv[i], "-capcookiemaxtime=" ,18))) {
            cookieMaxTime = atol((char*)&argv[i][18]);
            if(cookieMaxTime < 100000) {
               cookieMaxTime = 100000;
            }
         }
      }

      CalcAppServer calcAppServer(maxJobs, objectName, vectorFileName, scalarFileName,
                                  capacity,
                                  keepAliveTransmissionInterval, keepAliveTimeoutInterval,
                                  cookieMaxTime, cookieMaxCalculations);
      calcAppServer.poolElement("CalcApp Server - Version 1.0",
                                (poolHandle != NULL) ? poolHandle : "CalcAppPool",
                                &info, &loadInfo,
                                reregInterval, runtimeLimit,
                                (struct TagItem*)&tags);
   }

   return(0);
}
