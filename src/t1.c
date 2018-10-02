#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>

#include "rserpool.h"
#include "netutilities.h"
#include "loglevel.h"
#include "timeutilities.h"


// struct TuneSCTPParameters
// {
//    unsigned int InitialRTO;
//    unsigned int MinRTO;
//    unsigned int MaxRTO;
//    unsigned int AssocMaxRxt;
//    unsigned int PathMaxRxt;
//    unsigned int HeartbeatInterval;
// };


/* ###### Tune SCTP parameters ############################################ */
bool myTuneSCTP(const int                  sockfd,
                sctp_assoc_t               assocID,
                struct TuneSCTPParameters* parameters)
{
   struct sctp_rtoinfo     rtoinfo;
   struct sctp_paddrparams peerParams;
   struct sctp_assocparams assocParams;
   socklen_t               size;
   bool                    result = true;

   LOG_VERBOSE3
   fprintf(stdlog, "Tuning SCTP parameters of assoc %u:\n", sockfd);
   LOG_END

   size = (socklen_t)sizeof(rtoinfo);
   rtoinfo.srto_assoc_id = assocID;
   if(ext_getsockopt(sockfd, IPPROTO_SCTP, SCTP_RTOINFO, &rtoinfo, &size) < 0) {
//       LOG_WARNING
      logerror("getsockopt SCTP_RTOINFO failed -> skipping RTO configuration");
//       LOG_END
      return(false);
   }
   size = (socklen_t)sizeof(peerParams);
   peerParams.spp_assoc_id = assocID;
   memset(&peerParams.spp_address, 0, sizeof(peerParams.spp_address));   // ANY address
   if(ext_getsockopt(sockfd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &peerParams, &size) < 0) {
//       LOG_WARNING
      logerror("getsockopt SCTP_PEER_ADDR_PARAMS failed -> skipping path configuration");
//       LOG_END
      return(false);
   }
   size = (socklen_t)sizeof(assocParams);
   assocParams.sasoc_assoc_id = assocID;
   if(ext_getsockopt(sockfd, IPPROTO_SCTP, SCTP_ASSOCINFO, &assocParams, &size) < 0) {
//       LOG_WARNING
      logerror("getsockopt SCTP_ASSOCINFO failed -> skipping assoc configuration");
//       LOG_END
      return(false);
   }

//    LOG_VERBOSE3
   fputs("Old configuration:\n", stdlog);
   fprintf(stdlog, " - Initial RTO:       %u ms\n", rtoinfo.srto_initial);
   fprintf(stdlog, " - Min RTO:           %u ms\n", rtoinfo.srto_min);
   fprintf(stdlog, " - Max RTO:           %u ms\n", rtoinfo.srto_max);
   fprintf(stdlog, " - HeartbeatInterval: %u ms\n", peerParams.spp_hbinterval);
   fprintf(stdlog, " - PathMaxRXT:        %u\n",    peerParams.spp_pathmaxrxt);
   fprintf(stdlog, " - AssocMaxRXT:       %u\n",    assocParams.sasoc_asocmaxrxt);
//    LOG_END

   if(parameters->InitialRTO != 0) {
      rtoinfo.srto_initial = parameters->InitialRTO;
   }
   if(parameters->MinRTO != 0) {
      rtoinfo.srto_min = parameters->MinRTO;
   }
   if(parameters->MaxRTO != 0) {
      rtoinfo.srto_max = parameters->MaxRTO;
   }
   peerParams.spp_hbinterval = parameters->HeartbeatInterval;
   peerParams.spp_flags      = 0;
   if(parameters->HeartbeatInterval != 0) {
      peerParams.spp_flags |= SPP_HB_ENABLE;
   }
   if(parameters->PathMaxRxt != 0) {
      peerParams.spp_pathmaxrxt = parameters->PathMaxRxt;
   }
   if(parameters->AssocMaxRxt != 0) {
      assocParams.sasoc_asocmaxrxt = parameters->AssocMaxRxt;
   }

   if(ext_setsockopt(sockfd, IPPROTO_SCTP, SCTP_ASSOCINFO, &assocParams, (socklen_t)sizeof(assocParams)) < 0) {
//       LOG_WARNING
      logerror("setsockopt SCTP_ASSOCINFO failed");
//       LOG_END
      result = false;
   }

   if(ext_setsockopt(sockfd, IPPROTO_SCTP, SCTP_RTOINFO, &rtoinfo, (socklen_t)sizeof(rtoinfo)) < 0) {
//       LOG_WARNING
      logerror("setsockopt SCTP_RTOINFO failed");
//       LOG_END
      result = false;
   }
   printf("PM1=%d\n",  peerParams.spp_pathmaxrxt );
   peerParams.spp_assoc_id = assocID;
   memset(&peerParams.spp_address, 0, sizeof(peerParams.spp_address));   // ANY address
   if(ext_setsockopt(sockfd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &peerParams, sizeof(peerParams)) < 0) {
//       LOG_WARNING
      logerror("setsockopt SCTP_PEER_ADDR_PARAMS failed");
//       LOG_END
      result = false;
   }
   printf("PM2=%d\n",  peerParams.spp_pathmaxrxt );

   if(result) {
   //    LOG_VERBOSE3
      fputs("New configuration:\n", stdlog);
      fprintf(stdlog, " - Initial RTO:       %u ms\n", rtoinfo.srto_initial);
      fprintf(stdlog, " - Min RTO:           %u ms\n", rtoinfo.srto_min);
      fprintf(stdlog, " - Max RTO:           %u ms\n", rtoinfo.srto_max);
      fprintf(stdlog, " - HeartbeatInterval: %u ms\n", peerParams.spp_hbinterval);
      fprintf(stdlog, " - PathMaxRXT:        %u\n",    peerParams.spp_pathmaxrxt);
      fprintf(stdlog, " - AssocMaxRXT:       %u\n",    assocParams.sasoc_asocmaxrxt);
   //    LOG_END
   }

   return(result);
}


int main(int argc, char** argv)
{
   sockaddr_union remote;
   int sd;
   char buffer[1024];
   size_t i;
   unsigned long long startTime;

   puts("Start...");
   assert(string2address("[132.252.151.80]:7777", &remote) == true);
   sd = ext_socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
   if(sd < 0) {
      perror("socket()");
      exit(1);
   }

   struct TuneSCTPParameters tuningParameters;
   tuningParameters.InitialRTO        = 1000;
   tuningParameters.MinRTO            =  500;
   tuningParameters.MaxRTO            = 1500;
   tuningParameters.AssocMaxRxt       = 4;
   tuningParameters.PathMaxRxt        = 1;
   tuningParameters.HeartbeatInterval = 5000;
   myTuneSCTP(sd, 0, &tuningParameters);
   myTuneSCTP(sd, 0, &tuningParameters);

   struct sctp_event_subscribe event;
   memset(&event, 1, sizeof(event));
   if (setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) < 0) {
      perror("set event failed");
      exit(1);
   }

   if(ext_connect(sd, &remote.sa, sizeof(remote)) < 0) {
      perror("connect()");
      exit(1);
   }

   for(i = 0;i < sizeof(buffer);i++) {
      buffer[i] = (char)(i & 0xff);
   }
   i = 0;
   startTime = getMicroTime();
   for(;;) {
      if(i == 0) {
         struct sctp_setprim sp;
         sp.ssp_assoc_id = 0;
         assert(string2address("[2001:638:501:4ef2:21b:21ff:fe2a:449]:7777", (sockaddr_union*)&sp.ssp_addr) == true);
         if(ext_setsockopt(sd, IPPROTO_SCTP, SCTP_PRIMARY_ADDR, &sp, (socklen_t)sizeof(sp)) < 0) {
            perror("setsockopt SCTP_PRIMARY_ADDR failed");
         }
         puts("SET_PRIMARY");
      }


      if(ext_send(sd, &buffer, sizeof(buffer), MSG_NOSIGNAL) < 0) {
         perror("send()");
         break;
      }
      i++;
      fputs(".", stdout);
      fflush(stdout);
      usleep(500000);


      int flags = MSG_NOSIGNAL|MSG_DONTWAIT;
      int r = sctp_recvmsg(sd, &buffer, sizeof(buffer), NULL, NULL, NULL, &flags);
      while(r > 0) {
         printf("\nt=%1.6f\n", (getMicroTime() - startTime) / 1000000.0);
         if(flags & MSG_NOTIFICATION) {
            const char* state;
            char addrbuf[INET6_ADDRSTRLEN];
            const char* ap;
            const struct sctp_assoc_change *sac;
            const struct sctp_send_failed *ssf;
            const struct sctp_paddr_change *spc;
            const struct sctp_remote_error *sre;
            const union sctp_notification* snp = (const union sctp_notification*)&buffer;
            switch (snp->sn_header.sn_type) {
               case SCTP_ASSOC_CHANGE:
                     sac = &snp->sn_assoc_change;
                     printf("^^^ assoc_change: state=%hu, error=%hu, instr=%hu "
                        "outstr=%hu\n", sac->sac_state, sac->sac_error,
                        sac->sac_inbound_streams, sac->sac_outbound_streams);
                     break;
               case SCTP_SEND_FAILED:
                     ssf = &snp->sn_send_failed;
                     printf("^^^ sendfailed: len=%hu err=%d\n", ssf->ssf_length,
                        ssf->ssf_error);
                     break;

               case SCTP_PEER_ADDR_CHANGE:
                     spc = &snp->sn_paddr_change;
                     if (spc->spc_aaddr.ss_family == AF_INET) {
                        const sockaddr_in* sin = (const struct sockaddr_in *)&spc->spc_aaddr;
                        ap = inet_ntop(AF_INET, &sin->sin_addr,
                                       addrbuf, INET6_ADDRSTRLEN);
                     } else {
                        const sockaddr_in6* sin6 = (const struct sockaddr_in6 *)&spc->spc_aaddr;
                        ap = inet_ntop(AF_INET6, &sin6->sin6_addr,
                                       addrbuf, INET6_ADDRSTRLEN);
                     }
                     switch(spc->spc_state) {
                        case SCTP_ADDR_AVAILABLE:
                           state = "AVAILABLE";
                         break;
                        case SCTP_ADDR_UNREACHABLE:
                           state = "UNREACHABLE";
                         break;
                        case SCTP_ADDR_REMOVED:
                           state = "REMOVED";
                         break;
                        case SCTP_ADDR_ADDED:
                           state = "ADDED";
                         break;
                        case SCTP_ADDR_MADE_PRIM:
                           state = "MADE_PRIM";
                         break;
                        case SCTP_ADDR_CONFIRMED:
                           state = "CONFIRMED";
                         break;
                        default:
                           state = "???";
                         break;
                     }
                     printf("^^^ intf_change: %s state=%d %s, error=%d\n", ap,
                           spc->spc_state, state, spc->spc_error);
                     {
   struct sctp_paddrparams peerParams;
   socklen_t               size;
   size = (socklen_t)sizeof(peerParams);
   peerParams.spp_address = spc->spc_aaddr;
   if(ext_getsockopt(sd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &peerParams, &size) < 0) {
      perror("getsockopt SCTP_PEER_ADDR_PARAMS failed");
      return(false);
   }
   printf("====> spp_pathmaxrxt=%d spp_pathmtu=%d hb=%d    ST=%d\n", peerParams.spp_pathmaxrxt,peerParams.spp_pathmtu,peerParams.spp_hbinterval);
                     }
                     break;
               case SCTP_REMOTE_ERROR:
                     sre = &snp->sn_remote_error;
                     printf("^^^ remote_error: err=%hu len=%hu\n",
                        ntohs(sre->sre_error), ntohs(sre->sre_length));
                     break;
               case SCTP_SHUTDOWN_EVENT:
                     printf("^^^ shutdown event\n");
                     break;
               default:
                     printf("unknown type: %hu\n", snp->sn_header.sn_type);
                     break;
            }
         }

         flags = MSG_NOSIGNAL|MSG_DONTWAIT;
         r = sctp_recvmsg(sd, &buffer, sizeof(buffer), NULL, NULL, NULL, &flags);
      }
   }
   ext_close(sd);
   return 0;
}
