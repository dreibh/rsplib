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
#include "rsputilities.h"
#ifdef ENABLE_CSP
#include "componentstatusreporter.h"
#include "randomizer.h"
#endif


#define MAX_POOL_ELEMENTS 50000


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info     info;
   struct rsp_loadinfo loadinfo;
   int           poolElementArray[MAX_POOL_ELEMENTS];
   char          myPoolHandle[512];
   char*         poolHandle       = "TestPool";
   size_t        elements         = 10;
   size_t        pools            = 0;
   size_t        poolSize         = 10;
   bool          fastBreak        = false;
   bool          noDeregistration = false;
   size_t        i;


   memset(&info, 0, sizeof(info));
   memset(&loadinfo, 0, sizeof(loadinfo));
   loadinfo.rli_policy           = PPT_ROUNDROBIN;
   loadinfo.rli_weight           = 1;
   loadinfo.rli_load             = 0;
   loadinfo.rli_load_degradation = 0;
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-log" ,4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
#ifdef ENABLE_CSP
      else if(!(strncmp(argv[i], "-csp" ,4))) {
         if(initComponentStatusReporter(&info, argv[i]) == false) {
            exit(1);
         }
      }
#endif
      else if(!(strncmp(argv[i], "-registrar=", 11))) {
         if(addStaticRegistrar(&info, (char*)&argv[i][11]) < 0) {
            fprintf(stderr, "ERROR: Bad registrar setting %s\n", argv[i]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strcmp(argv[i], "-fastbreak"))) {
         fastBreak = true;
      }
      else if(!(strcmp(argv[i], "-noderegistration"))) {
         noDeregistration = true;
      }
      else if(!(strncmp(argv[i], "-ph=" ,4))) {
         poolHandle = (char*)&argv[i][4];
      }
      else if(!(strncmp(argv[i], "-load=" ,6))) {
         loadinfo.rli_load = atol((char*)&argv[i][6]);
         if(loadinfo.rli_load > 0xffffff) {
            loadinfo.rli_load = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-loaddeg=" ,9))) {
         loadinfo.rli_load_degradation = atol((char*)&argv[i][9]);
         if(loadinfo.rli_load_degradation > 0xffffff) {
            loadinfo.rli_load_degradation = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-weight=" ,8))) {
         loadinfo.rli_weight = atol((char*)&argv[i][8]);
         if(loadinfo.rli_weight < 1) {
            loadinfo.rli_weight = 1;
         }
      }
      else if(!(strncmp(argv[i], "-elements=" ,10))) {
         elements = atol((char*)&argv[i][10]);
         if(elements < 1) {
            elements = 1;
         }
         else if(elements > MAX_POOL_ELEMENTS) {
            elements = MAX_POOL_ELEMENTS;
         }
      }
      else if(!(strncmp(argv[i], "-poolsize=" ,10))) {
         poolSize = atol((char*)&argv[i][10]);
         if(poolSize < 1) {
            poolSize = 1;
         }
      }
      else if(!(strncmp(argv[i], "-policy=" ,8))) {
         if((!(strcmp((char*)&argv[i][8], "roundrobin"))) || (!(strcmp((char*)&argv[i][8], "rr")))) {
            loadinfo.rli_policy = PPT_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[i][8], "weightedroundrobin"))) || (!(strcmp((char*)&argv[i][8], "wrr")))) {
            loadinfo.rli_policy = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[i][8], "leastused"))) || (!(strcmp((char*)&argv[i][8], "lu")))) {
            loadinfo.rli_policy = PPT_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "leastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "lud")))) {
            loadinfo.rli_policy = PPT_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedleastused"))) || (!(strcmp((char*)&argv[i][8], "rlu")))) {
            loadinfo.rli_policy = PPT_RANDOMIZED_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "rlud")))) {
            loadinfo.rli_policy = PPT_RANDOMIZED_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "priorityleastused"))) || (!(strcmp((char*)&argv[i][8], "plu")))) {
            loadinfo.rli_policy = PPT_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "priorityleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "plud")))) {
            loadinfo.rli_policy = PPT_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedpriorityleastused"))) || (!(strcmp((char*)&argv[i][8], "rplu")))) {
            loadinfo.rli_policy = PPT_RANDOMIZED_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedpriorityleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "rplud")))) {
            loadinfo.rli_policy = PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "random"))) || (!(strcmp((char*)&argv[i][8], "rand")))) {
            loadinfo.rli_policy = PPT_RANDOM;
         }
         else if((!(strcmp((char*)&argv[i][8], "weightedrandom"))) || (!(strcmp((char*)&argv[i][8], "wrand")))) {
            loadinfo.rli_policy = PPT_WEIGHTED_RANDOM;
         }
         else {
            fprintf(stderr, "ERROR: Unknown policy type \"%s\"!\n" , (char*)&argv[i][8]);
            exit(1);
         }
      }
      else {
         fprintf(stderr, "Bad argument \"%s\"!\n" ,argv[i]);
         fprintf(stderr, "Usage: %s {-elements=total PEs} {-poolsize=PEs} {-poolhandle=pool handle} {-fastbreak} {-noderegistration} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} {-policy=roundrobin|rr|weightedroundrobin|wee|leastused|lu|leastuseddegradation|lud|random|rd|weightedrandom|wrd} {-load=load} {-weight=weight} \n" ,
                argv[0]);
         exit(1);
      }
   }

   beginLogging();
   if(rsp_initialize(&info) < 0) {
      perror("Unable to initialize rsplib!");
      finishLogging();
      exit(1);
   }


   puts("Test-Registrator - Version 1.0");
   puts("==============================\n");
   printf("No Deregistration   = %s\n", (noDeregistration == true) ? "on" : "off");
   printf("Fast Break          = %s\n", (fastBreak == true) ? "on" : "off");
   printf("Total Pool Elements = %u\n", (unsigned int)elements);
   printf("Pool Size           = %u PEs\n\n", (unsigned int)poolSize);


   for(i = 0;i < elements;i++) {
      printf("Registering PE #%u...\n", (unsigned int)i + 1);
      if(((i % poolSize) == 0) || (i == 0)) {
         snprintf((char*)&myPoolHandle, sizeof(myPoolHandle), "%s-%04u", poolHandle, (unsigned int)++pools);
      }
      poolElementArray[i] = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
      if(poolElementArray[i] < 0) {
         perror("Unable to create RSerPool socket");
         exit(1);
      }
      if(rsp_register(poolElementArray[i], poolHandle, strlen(poolHandle),
                      &loadinfo, 60000) != 0) {
         perror("Unable to register pool element");
         exit(1);
      }
   }


   if(!fastBreak) {
      installBreakDetector();
   }
   while(!breakDetected()) {
      usleep(250000);
   }

   if(!noDeregistration) {
      for(i = 0;i < elements;i++) {
         rsp_deregister(poolElementArray[i]);
         rsp_close(poolElementArray[i]);
      }
   }


   rsp_cleanup();
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
