/*
 *  $Id: testregistrator.c,v 1.6 2004/11/12 15:56:49 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Example Pool Element
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "breakdetector.h"
#include "rsplib.h"

#include <ext_socket.h>
#include <pthread.h>


#define MAX_POOL_ELEMENTS 1024


/* ###### rsplib main loop thread ########################################### */
static bool RsplibThreadStop = false;
static void* rsplibMainLoop(void* args)
{
   struct timeval timeout;
   while(!RsplibThreadStop) {
      timeout.tv_sec  = 0;
      timeout.tv_usec = 50000;
      rspSelect(0, NULL, NULL, NULL, &timeout);
   }
   return(NULL);
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct PoolElementDescriptor* poolElementArray[MAX_POOL_ELEMENTS];
   struct TagItem                tags[16];
   char                          myPoolHandle[512];
   pthread_t                     rsplibThread;

   card64                        start                          = getMicroTime();
   card64                        stop                           = 0;
   int                           type                           = SOCK_STREAM;
   int                           protocol                       = IPPROTO_SCTP;
   unsigned short                port                           = 0;
   char*                         poolHandle                     = "TestPool";
   unsigned int                  policyType                     = PPT_ROUNDROBIN;
   unsigned int                  policyParameterWeight          = 1;
   unsigned int                  policyParameterLoad            = 0;
   unsigned int                  policyParameterLoadDegradation = 0;
   unsigned int                  newPoolAfter                   = 1000000;
   size_t                        count                          = 10;
   bool                          fastBreak                      = false;
   bool                          noDeregistration               = false;
   size_t                        pools;
   int                           n;
   size_t                        i;


   start = getMicroTime();
   stop  = 0;
   for(n = 1;n < argc;n++) {
      if(!(strcmp(argv[n], "-sctp"))) {
         protocol = IPPROTO_SCTP;
      }
      else if(!(strcmp(argv[n], "-tcp"))) {
         protocol = IPPROTO_TCP;
      }
      else if(!(strncmp(argv[n], "-stop=" ,6))) {
         stop = start + ((card64)1000000 * (card64)atol((char*)&argv[n][6]));
      }
      else if(!(strncmp(argv[n], "-port=" ,6))) {
         port = atol((char*)&argv[n][6]);
      }
      else if(!(strcmp(argv[n], "-fastbreak"))) {
         fastBreak = true;
      }
      else if(!(strcmp(argv[n], "-noderegistration"))) {
         noDeregistration = true;
      }
      else if(!(strncmp(argv[n], "-ph=" ,4))) {
         poolHandle = (char*)&argv[n][4];
      }
      else if(!(strncmp(argv[n], "-load=" ,6))) {
         policyParameterLoad = atol((char*)&argv[n][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-weight=" ,8))) {
         policyParameterWeight = atol((char*)&argv[n][8]);
         if(policyParameterWeight > 0xffffff) {
            policyParameterWeight = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-load=" ,6))) {
         policyParameterLoad = atol((char*)&argv[n][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-loaddeg=" ,9))) {
         policyParameterLoadDegradation = atol((char*)&argv[n][9]);
         if(policyParameterLoadDegradation > 0xffffff) {
            policyParameterLoadDegradation = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-weight=" ,8))) {
         policyParameterWeight = atol((char*)&argv[n][8]);
         if(policyParameterWeight < 1) {
            policyParameterWeight = 1;
         }
      }
      else if(!(strncmp(argv[n], "-count=" ,7))) {
         count = atol((char*)&argv[n][7]);
         if(count < 1) {
            count = 1;
         }
         else if(count > MAX_POOL_ELEMENTS) {
            count = MAX_POOL_ELEMENTS;
         }
      }
      else if(!(strncmp(argv[n], "-newpoolafter=" ,14))) {
         newPoolAfter = atol((char*)&argv[n][14]);
         if(newPoolAfter < 1) {
            newPoolAfter = 1;
         }
      }
      else if(!(strncmp(argv[n], "-policy=" ,8))) {
         if((!(strcmp((char*)&argv[n][8], "roundrobin"))) || (!(strcmp((char*)&argv[n][8], "rr")))) {
            policyType = PPT_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[n][8], "weightedroundrobin"))) || (!(strcmp((char*)&argv[n][8], "wrr")))) {
            policyType = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[n][8], "leastused"))) || (!(strcmp((char*)&argv[n][8], "lu")))) {
            policyType = PPT_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "leastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "lud")))) {
            policyType = PPT_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedleastused"))) || (!(strcmp((char*)&argv[n][8], "rlu")))) {
            policyType = PPT_RANDOMIZED_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "rlud")))) {
            policyType = PPT_RANDOMIZED_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "priorityleastused"))) || (!(strcmp((char*)&argv[n][8], "plu")))) {
            policyType = PPT_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "priorityleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "plud")))) {
            policyType = PPT_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedpriorityleastused"))) || (!(strcmp((char*)&argv[n][8], "rplu")))) {
            policyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedpriorityleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "rplud")))) {
            policyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "random"))) || (!(strcmp((char*)&argv[n][8], "rand")))) {
            policyType = PPT_RANDOM;
         }
         else if((!(strcmp((char*)&argv[n][8], "weightedrandom"))) || (!(strcmp((char*)&argv[n][8], "wrand")))) {
            policyType = PPT_WEIGHTED_RANDOM;
         }
         else {
            printf("ERROR: Unknown policy type \"%s\"!\n" , (char*)&argv[n][8]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[n], "-log" ,4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
      else {
         printf("Bad argument \"%s\"!\n" ,argv[n]);
         printf("Usage: %s {-sctp|-tcp} {-port=local port} {-count=Count} {-newpoolafter=count} {-ph=Pool Handle} {-fastbreak} {-noderegistration} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} {-policy=roundrobin|rr|weightedroundrobin|wee|leastused|lu|leastuseddegradation|lud|random|rd|weightedrandom|wrd} {-load=load} {-weight=weight} \n" ,
                argv[0]);
         exit(1);
      }
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
      finishLogging();
      exit(1);
   }

   if(pthread_create(&rsplibThread, NULL, &rsplibMainLoop, NULL) != 0) {
      puts("ERROR: Unable to create rsplib main loop thread!");
   }


   puts("Test-Registrator - Version 1.0");
   puts("------------------------------\n");


   pools = 0;
   for(i = 0;i < count;i++) {
      tags[0].Tag  = TAG_PoolPolicy_Type;
      tags[0].Data = policyType;
      tags[1].Tag  = TAG_PoolPolicy_Parameter_Load;
      tags[1].Data = policyParameterLoad;
      tags[2].Tag  = TAG_PoolPolicy_Parameter_LoadDegradation;
      tags[2].Data = policyParameterLoadDegradation;
      tags[3].Tag  = TAG_PoolPolicy_Parameter_Weight;
      tags[3].Data = policyParameterWeight;
      tags[4].Tag  = TAG_PoolElement_SocketType;
      tags[4].Data = type;
      tags[5].Tag  = TAG_PoolElement_SocketProtocol;
      tags[5].Data = protocol;
      tags[6].Tag  = TAG_PoolElement_LocalPort;
      tags[6].Data = (port != 0) ? port + i : 0;
      tags[7].Tag  = TAG_PoolElement_ReregistrationInterval;
      tags[7].Data = 45000;
      tags[8].Tag  = TAG_PoolElement_RegistrationLife;
      tags[8].Data = (3 * 45000) + 5000;
      tags[9].Tag  = TAG_UserTransport_HasControlChannel;
      tags[9].Data = (tagdata_t)true;
      tags[10].Tag  = TAG_END;

      printf("Registering PE #%u...\n", (unsigned int)i + 1);
      if(((i % newPoolAfter) == 0) || (i == 0)) {
         snprintf((char*)&myPoolHandle, sizeof(myPoolHandle), "%s-%04u", poolHandle, (unsigned int)++pools);
      }
      poolElementArray[i] = rspCreatePoolElement((unsigned char*)myPoolHandle, strlen(myPoolHandle), tags);
      if(poolElementArray[i] == NULL) {
         puts("ERROR: Registration failed!\n");
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
      for(i = 0;i < count;i++) {
         rspDeletePoolElement(poolElementArray[i], NULL);
      }
   }


   RsplibThreadStop = true;
   pthread_join(rsplibThread, NULL);
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
