/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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
   struct rsp_info info;
   unsigned int    reregInterval = 30000;
   unsigned int    runtimeLimit  = 0;
   unsigned int    service       = SERVICE_ECHO;
   const char*     poolHandle    = NULL;
   struct TagItem  tags[8];

   /* ====== Read parameters ============================================= */
   memset(&info, 0, sizeof(info));
   tags[0].Tag  = TAG_PoolElement_Identifier;
   tags[0].Data = 0;
   tags[1].Tag  = TAG_DONE;
#ifdef ENABLE_CSP
   info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, 0);
#endif
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
                             &info, NULL,
                             reregInterval, runtimeLimit,
                             (struct TagItem*)&tags);
   }
   else if(service == SERVICE_DISCARD) {
      DiscardServer discardServer;
      discardServer.poolElement("Discard Server - Version 1.0",
                                (poolHandle != NULL) ? poolHandle : "DiscardPool",
                                &info, NULL,
                                reregInterval, runtimeLimit,
                                (struct TagItem*)&tags);
   }
   else if(service == SERVICE_DAYTIME) {
      DaytimeServer daytimeServer;
      daytimeServer.poolElement("Daytime Server - Version 1.0",
                                (poolHandle != NULL) ? poolHandle : "DaytimePool",
                                &info, NULL,
                                reregInterval, runtimeLimit,
                                (struct TagItem*)&tags);
   }
   else if(service == SERVICE_CHARGEN) {
      TCPLikeServer::poolElement("Character Generator Server - Version 1.0",
                                  (poolHandle != NULL) ? poolHandle : "CharGenPool",
                                  &info, NULL,
                                  CharGenServer::charGenServerFactory,
                                  NULL,
                                  reregInterval, runtimeLimit,
                                  (struct TagItem*)&tags);
   }
   else if(service == SERVICE_PINGPONG) {
      PingPongServer::PingPongServerSettings settings;
      settings.FailureAfter = 0;
      for(int i = 1;i < argc;i++) {
         if(!(strncmp(argv[i], "-pppfailureafter=", 17))) {
            settings.FailureAfter = atol((const char*)&argv[i][17]);
         }
      }

      TCPLikeServer::poolElement("Ping Pong Server - Version 1.0",
                                 (poolHandle != NULL) ? poolHandle : "PingPongPool",
                                 &info, NULL,
                                 PingPongServer::pingPongServerFactory,
                                 (void*)&settings,
                                 reregInterval, runtimeLimit,
                                 (struct TagItem*)&tags);
   }
   else if(service == SERVICE_FRACTAL) {
      FractalGeneratorServer::FractalGeneratorServerSettings settings;
      settings.TestMode     = false;
      settings.FailureAfter = 0;
      for(int i = 1;i < argc;i++) {
         if(!(strcmp(argv[i], "-fgptestmode"))) {
            settings.TestMode = true;
         }
         else if(!(strncmp(argv[i], "-fgpfailureafter=", 17))) {
            settings.FailureAfter = atol((const char*)&argv[i][17]);
         }
      }

      TCPLikeServer::poolElement("Fractal Generator Server - Version 1.0",
                                 (poolHandle != NULL) ? poolHandle : "FractalGeneratorPool",
                                 &info, NULL,
                                 FractalGeneratorServer::fractalGeneratorServerFactory,
                                 (void*)&settings,
                                 reregInterval, runtimeLimit,
                                 (struct TagItem*)&tags);
   }

   return(0);
}
