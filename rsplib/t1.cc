#include <ext_socket.h>
#include "netutilities.h"

/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   if(argc < 2) exit(1);

   sockaddr_union dst;
   if(string2address(argv[1], &dst) == false) exit(1);

   puts("Start ...");


   int sd = ext_socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(sd<0) { perror("socket"); exit(1); }

   puts("NonBlock...");
   setNonBlocking(sd);

   struct sctp_event_subscribe sctpEvents;
   memset(&sctpEvents, 0, sizeof(sctpEvents));
//    sctpEvents.sctp_data_io_event          = 1;
   sctpEvents.sctp_association_event      = 1;
/*   sctpEvents.sctp_address_event          = 1;
   sctpEvents.sctp_send_failure_event     = 1;
   sctpEvents.sctp_peer_error_event       = 1;
   sctpEvents.sctp_shutdown_event         = 1;
   sctpEvents.sctp_partial_delivery_event = 1;*/
   /* sctpEvents.sctp_adaptation_layer_event = 1; */
   if(ext_setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
      perror("setsockopt() for SCTP_EVENTS failed");
      exit(1);
   }

   puts("Connect...");
   int c = ext_connect(sd, &dst.sa, getSocklen(&dst.sa));
   perror("connect");

   puts("poll...");
   fd_set rs;
   FD_ZERO(&rs);
   FD_SET(sd, &rs);
   int result = ext_select(sd + 1, &rs, NULL, NULL, NULL);
   printf("result=%d\n", result);

   printf("Read NOTIF...\n");
   char buffer[1024];
   int r = ext_recv(sd, (char*)&buffer, sizeof(buffer), 0);
   printf("read=%d\n", r);
//    if(r != sizeof(union sctp_notification)) { puts("NOTF"); exit(1); }

   printf("Read NOTIF2...\n");
   r = ext_recv(sd, (char*)&buffer, sizeof(buffer), 0);
   printf("read=%d\n", r);


   union sctp_notification* n = (union sctp_notification*)&buffer;
   printf("A=%d\n",n->sn_assoc_change.sac_assoc_id);


   printf("Write...\n");
   struct sctp_sndrcvinfo sri;
   memset(&sri, 0, sizeof(sri));
   sri.sinfo_assoc_id = n->sn_assoc_change.sac_assoc_id;
   sri.sinfo_stream   = 0;
   sri.sinfo_ppid     = htonl(0x1234);
//    int w = sctp_send(sd, "TEST", 4, &sri, MSG_NOSIGNAL|MSG_EOR);
   int w = ext_sendto(sd, "TEST", 4, 0, &dst.sa, getSocklen(&dst.sa));
//    int w = sendtoplus(sd, "TEST", 4, MSG_NOSIGNAL|MSG_EOR, NULL, 0, 0x1234, n->sn_assoc_change.sac_assoc_id, 0, 0, 0);
printf("w=%d\n", w);
   if(w < 0) { perror("send"); exit(1); }


   int newSD = sctp_peeloff(sd, n->sn_assoc_change.sac_assoc_id);
   if(newSD >= 0) {
      printf("PEELOFF: %d\n", newSD);

      puts("poll...");
      fd_set ws;
      FD_ZERO(&ws);
      FD_SET(newSD, &ws);
      int result = ext_select(newSD + 1, NULL, &ws, NULL, NULL);
      printf("result=%d\n", result);

      w = ext_send(newSD, "YYYY", 4, MSG_NOSIGNAL|MSG_EOR);
      printf("write2=%d\n", w);

      printf("Read...\n");
      setBlocking(newSD);
      r = ext_recv(newSD, (char*)&buffer, sizeof(buffer), 0);
      printf("read=%d\n", r);

      ext_close(newSD);
   }
   ext_close(sd);
}
