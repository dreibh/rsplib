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
 * Copyright (C) 2002-2012 by Thomas Dreibholz
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
#include "timeutilities.h"
#include "randomizer.h"
#include "netutilities.h"
#include "loglevel.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct rsp_info       info;
   struct rsp_loadinfo   loadInfo;
   uint16_t              port           = 0;
   uint32_t              identifier     = 0;
   const char*           attackType     = "registration";
   unsigned long long    attackInterval = 1000000;
   const char*           poolHandle     = "EchoPool";
   bool                  dontwait       = false;
   double                degradation;
   double                reportUnreachableProbability = 0.0;


   rsp_initinfo(&info);
   memset((char*)&loadInfo, 0, sizeof(loadInfo));
   loadInfo.rli_policy = PPT_ROUNDROBIN;
   for(int i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
      }
      else if(!(strncmp(argv[i],"-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-type=", 6))) {
         attackType = (char*)&argv[i][6];
      }
      else if(!(strncmp(argv[i], "-interval=", 10))) {
         attackInterval = (unsigned long long)rint(atof((char*)&argv[i][10]) * 1000000.0);
      }
      else if(!(strncmp(argv[i], "-loaddegoverride=", 17))) {
         loadInfo.rli_load_degradation = (unsigned int)rint(atof((const char*)&argv[i][17]) * (double)PPV_MAX_LOAD_DEGRADATION);
      }
      else if(!(strncmp(argv[i], "-poolhandle=" , 12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-port=" , 6))) {
         port = atol((const char*)&argv[i][6]);
      }
      else if(!(strcmp(argv[i], "-dontwait"))) {
         dontwait = true;
      }
      else if(!(strncmp(argv[i], "-identifier=", 12))) {
         if(sscanf((const char*)&argv[i][12], "0x%x", &identifier) == 0) {
            if(sscanf((const char*)&argv[i][12], "%u", &identifier) == 0) {
               fputs("ERROR: Bad registrar ID given!\n", stderr);
               exit(1);
            }
         }
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
      }
      else {
         fprintf(stderr, "ERROR: Bad parameter <%s>!\n", argv[i]);
         fprintf(stderr, "Usage: %s {-type=registration|handleresolution} {-interval=Seconds} {-port=Port} {-identifier=Identifier} {-policy=Policy} {-loaddegoverride=Load Degradation} {-reportunreachableprobability=Probability}\n", argv[0]);
         exit(1);
      }
   }


   puts("RSerPool Attack Tester - Version 1.0");
   puts("====================================\n");
   printf("Attack Type            = %s\n", attackType);
   printf("Attack Interval        = %lluus\n", attackInterval);
   printf("Pool Handle            = %s\n", poolHandle);
   printf("Port                   = %u\n", port);
   printf("Identifier             = $%08x\n", identifier);
   printf("Load Degradation       = $%08x\n", loadInfo.rli_load_degradation);
   printf("Do not wait for result = %s\n", (dontwait == true) ? "yes" : "no");
   printf("Report Unreachable Pr. = %1.2lf%%\n\n", reportUnreachableProbability * 100.0);


   if(rsp_initialize(&info) < 0) {
      fputs("Unable to initialize rsplib\n", stderr);
      exit(1);
   }

   installBreakDetector();
   if(!strcmp(attackType, "registration")) {
      struct rsp_addrinfo   rspAddrInfo;
      size_t                sctpLocalAddresses;
      union sockaddr_union* sctpLocalAddressArray = NULL;
      struct sockaddr*      packedAddressArray    = NULL;

      /* ====== Prepare rspAddrInfo structure ========================= */
      rspAddrInfo.ai_family   = checkIPv6() ? AF_INET6 : AF_INET;
      rspAddrInfo.ai_socktype = SOCK_SEQPACKET;
      rspAddrInfo.ai_protocol = IPPROTO_SCTP;
      rspAddrInfo.ai_next     = NULL;
      rspAddrInfo.ai_addr     = NULL;
      rspAddrInfo.ai_addrs    = 0;
      rspAddrInfo.ai_addrlen  = sizeof(union sockaddr_union);

      /* ====== Get local addresses by sctp_getladds() ================ */
      rspAddrInfo.ai_addrs = 0;
      int sd = ext_socket(rspAddrInfo.ai_family, rspAddrInfo.ai_socktype, rspAddrInfo.ai_protocol);
      if(sd < 1) {
         perror("socket() failed");
         exit(1);
      }
      union sockaddr_union anyAddress;
      string2address(checkIPv6() ? "[::]" : "0.0.0.0", &anyAddress);
      setPort(&anyAddress.sa, port);
      if(ext_bind(sd, (sockaddr*)&anyAddress, sizeof(anyAddress)) < 0) {
         perror("bind() failed");
         exit(1);
      }
      setNonBlocking(sd);
      if(ext_listen(sd, 100) < 0) {
         perror("listen() failed");
         exit(1);
      }
      sctpLocalAddresses = getladdrsplus(sd, 0, &sctpLocalAddressArray);
      if(sctpLocalAddresses <= 0) {
         perror("sctp_getladdrs() failed!");
         exit(1);
      }
      else {
         /* --- Check for buggy SCTP implementation ----- */
         if( (getPort(&sctpLocalAddressArray[0].sa) == 0)                  ||
             ( ((sctpLocalAddressArray[0].sa.sa_family == AF_INET) &&
                (sctpLocalAddressArray[0].in.sin_addr.s_addr == 0)) ||
             ( ((sctpLocalAddressArray[0].sa.sa_family == AF_INET6) &&
                (IN6_IS_ADDR_UNSPECIFIED(&sctpLocalAddressArray[0].in6.sin6_addr)))) ) ) {
            fputs("getladdrsplus() replied INADDR_ANY or port 0\n", stderr);
            exit(1);
         }
         /* --------------------------------------------- */

         packedAddressArray = pack_sockaddr_union(sctpLocalAddressArray, sctpLocalAddresses);
         if(packedAddressArray != NULL) {
            rspAddrInfo.ai_addr  = packedAddressArray;
            rspAddrInfo.ai_addrs = sctpLocalAddresses;
         }
         else {
            fputs("Address problems\n", stderr);
            exit(1);
         }
      }


      /* ====== Registration loop ======================================== */
      while(!breakDetected()) {
         rspAddrInfo.ai_pe_id = identifier;

         /* ====== Do registration ======================================= */
         int result = rsp_pe_registration(
                         (unsigned char*)poolHandle, strlen(poolHandle),
                         &rspAddrInfo,
                         &loadInfo,
                         72000000,
                         dontwait ? REGF_DONTWAIT|REGF_CONTROLCHANNEL : REGF_CONTROLCHANNEL);
         printTimeStamp(stdout);
         if(result == 0) {
            printf("(Re-)Registration successful, ID is $%08x\n", rspAddrInfo.ai_pe_id);
         }
         else {
            printf("(Re-)Registration failed\n");
         }

         usleep(attackInterval);

         char buffer[65536];
         int  received;
         while( (received = ext_recv(sd, (char*)&buffer, sizeof(buffer), 0)) > 0 ) {
            printTimeStamp(stdout);
            printf("Discard message of %d bytes.\n", received);
         }
      }

      /* ====== Clean up ============================================== */
      if(sctpLocalAddressArray) {
         free(sctpLocalAddressArray);
      }
      if(packedAddressArray) {
         free(packedAddressArray);
      }
      close(sd);
   }

   else if(!strcmp(attackType, "handleresolution")) {
      while(!breakDetected()) {
         struct rsp_addrinfo* rspAddrInfo;
         int result = rsp_getaddrinfo((const unsigned char*)poolHandle, strlen(poolHandle),
                                      &rspAddrInfo, 1, 0);

         printTimeStamp(stdout);
         printf("Selected %d items\n", result);
         if(result > 0) {
            if(reportUnreachableProbability > 0.0) {
               struct rsp_addrinfo* r = rspAddrInfo;
               while(r != NULL) {
                  if(randomDouble() <= reportUnreachableProbability) {
                     rsp_pe_failure((const unsigned char*)poolHandle, strlen(poolHandle), r->ai_pe_id);
                     printf("   => Endpoint Unreachable for ID $%08x\n", r->ai_pe_id);
                  }
                  r = r->ai_next;
               }
            }

            rsp_freeaddrinfo(rspAddrInfo);
         }
         usleep(attackInterval);
      }
   }

   else {
      fprintf(stderr, "ERROR: Invalid attack type %s!\n", attackType);
      exit(1);
   }

   rsp_cleanup();
   rsp_freeinfo(&info);
   puts("\nTerminated!");
   return 0;
}
