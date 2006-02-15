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
#include "standardservices.h"


#define SERVICE_ECHO     1
#define SERVICE_DISCARD  2
#define SERVICE_DAYTIME  3
#define SERVICE_CHARGEN  4

#define SERVICE_PINGPONG 5
#define SERVICE_FRACTAL  6


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info     info;
   struct rsp_loadinfo loadInfo;
   unsigned int        reregInterval = 30000;
   unsigned int        runtimeLimit  = 0;
   unsigned int        service       = SERVICE_ECHO;
   const char*         poolHandle    = NULL;
   struct TagItem      tags[8];

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
         tags[0].Data = atol((const char*)&argv[i][12]);
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
         else if(sscanf((const char*)&argv[i][8], "LeastUsedDegradation:%u", &loadInfo.rli_load_degradation) == 1) {
            if(loadInfo.rli_load_degradation > 0xffffff) {
               fputs("ERROR: Bad LUD degradation value!\n", stderr);
               exit(1);
            }
            loadInfo.rli_policy = PPT_LEASTUSED_DEGRADATION;
         }
         else if(sscanf((const char*)&argv[i][8], "PriorityLeastUsed:%u", &loadInfo.rli_load_degradation) == 1) {
            loadInfo.rli_policy = PPT_PRIORITY_LEASTUSED;
         }
         else if(sscanf((const char*)&argv[i][8], "RandomizedPriorityLeastUsed:%u", &loadInfo.rli_load_degradation) == 1) {
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
                                  NULL,
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
                                 (void*)&settings,
                                 reregInterval, runtimeLimit,
                                 (struct TagItem*)&tags);
   }

   return(0);
}
