#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <sys/types.h>
#include <signal.h>

#include "rserpool.h"
#include "netutilities.h"
#include "loglevel.h"

struct TuneSCTPParameters
{
   unsigned int InitialRTO;
   unsigned int MinRTO;
   unsigned int MaxRTO;
   unsigned int AssocMaxRxt;
   unsigned int PathMaxRxt;
   unsigned int HeartbeatInterval;
};


/* ###### Tune SCTP parameters ############################################ */
bool tuneSCTP(const int                  sockfd,
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


   if(ext_setsockopt(sockfd, IPPROTO_SCTP, SCTP_RTOINFO, &rtoinfo, (socklen_t)sizeof(rtoinfo)) < 0) {
      LOG_WARNING
      logerror("setsockopt SCTP_RTOINFO failed");
      LOG_END
      result = false;
   }
   if(ext_setsockopt(sockfd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &peerParams, (socklen_t)sizeof(peerParams)) < 0) {
      LOG_WARNING
      logerror("setsockopt SCTP_PEER_ADDR_PARAMS failed");
      LOG_END
      result = false;
   }
   if(ext_setsockopt(sockfd, IPPROTO_SCTP, SCTP_ASSOCINFO, &assocParams, (socklen_t)sizeof(assocParams)) < 0) {
      LOG_WARNING
      logerror("setsockopt SCTP_ASSOCINFO failed");
      LOG_END
      result = false;
   }

//    LOG_VERBOSE3
   fputs("New configuration:\n", stdlog);
   fprintf(stdlog, " - Initial RTO:       %u ms\n", rtoinfo.srto_initial);
   fprintf(stdlog, " - Min RTO:           %u ms\n", rtoinfo.srto_min);
   fprintf(stdlog, " - Max RTO:           %u ms\n", rtoinfo.srto_max);
   fprintf(stdlog, " - HeartbeatInterval: %u ms\n", peerParams.spp_hbinterval);
   fprintf(stdlog, " - PathMaxRXT:        %u\n",    peerParams.spp_pathmaxrxt);
   fprintf(stdlog, " - AssocMaxRXT:       %u\n",    assocParams.sasoc_asocmaxrxt);
//    LOG_END

   return(result);
}


int main(int argc, char** argv)
{
   sockaddr_union remote;
   int sd;
   char buffer[1024];
   size_t i;

   puts("Start...");
   assert(string2address("[::1]:7777", &remote) == true);
   sd = ext_socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
   if(sd < 0) {
      perror("socket()");
   }
   if(ext_connect(sd, &remote.sa, sizeof(remote)) < 0) {
      perror("connect()");
   }

   struct TuneSCTPParameters tuningParameters;
   tuningParameters.InitialRTO        = 2000;
   tuningParameters.MinRTO            = 1000;
   tuningParameters.MaxRTO            = 3000;
   tuningParameters.AssocMaxRxt       = 12;
   tuningParameters.PathMaxRxt        = 3;
   tuningParameters.HeartbeatInterval = 2000;
   tuneSCTP(sd, 0, &tuningParameters);
   tuneSCTP(sd, 0, &tuningParameters);

   for(i = 0;i < sizeof(buffer);i++) {
      buffer[i] = (char)(i & 0xff);
   }
   for(;;) {
      if(ext_send(sd, &buffer, sizeof(buffer), MSG_NOSIGNAL) < 0) {
         perror("send()");
         break;
      }
      usleep(500000);
   }
   ext_close(sd);
   return 0;
}
