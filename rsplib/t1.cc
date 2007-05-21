#include <ext_socket.h>
#include "netutilities.h"

/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   if(argc < 2) exit(1);

   sockaddr_union dst;
   if(string2address(argv[1], &dst) == false) exit(1);

   puts("Start ...");


   int sd = ext_socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
   if(sd<0) { perror("socket"); exit(1); }

   puts("NonBlock...");
   setNonBlocking(sd);

   struct sctp_event_subscribe sctpEvents;
   memset(&sctpEvents, 0, sizeof(sctpEvents));
   sctpEvents.sctp_data_io_event          = 1;
   sctpEvents.sctp_association_event      = 1;
   sctpEvents.sctp_address_event          = 1;
   sctpEvents.sctp_send_failure_event     = 1;
   sctpEvents.sctp_peer_error_event       = 1;
   sctpEvents.sctp_shutdown_event         = 1;
   sctpEvents.sctp_partial_delivery_event = 1;
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

   printf("Write...\n");
   int w = ext_send(sd, "TEST", 4, MSG_NOSIGNAL);
printf("w=%d\n", w);
   if(w < 0) { perror("send"); exit(1); }

   printf("Read...\n");
   r = ext_recv(sd, (char*)&buffer, sizeof(buffer), 0);
   printf("read=%d\n", r);

   ext_close(sd);
}
