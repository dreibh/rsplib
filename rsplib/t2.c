#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>


int main(int argc, char** argv)
{
   struct sockaddr_in dst;
   struct sctp_event_subscribe sctpEvents;
   union sctp_notification notification;
   struct sctp_sndrcvinfo sri;
   char buffer[1024];
   int w,r;
   int sd;
   int newSD;
   fd_set rs;
   fd_set ws;


   if(argc < 3) { puts("Bad args"); exit(1); }

   if(inet_aton(argv[1], &dst.sin_addr) == 0) {
      puts("Bad address"); exit(1);
   }
   dst.sin_port = ntohs(atol(argv[2]));
   dst.sin_family = AF_INET;


   puts("Socket ...");
   sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(sd<0) { perror("socket"); exit(1); }

   puts("Events ...");
   memset(&sctpEvents, 0, sizeof(sctpEvents));
   sctpEvents.sctp_association_event      = 1;
   sctpEvents.sctp_address_event          = 1;
   sctpEvents.sctp_send_failure_event     = 1;
   sctpEvents.sctp_peer_error_event       = 1;
   sctpEvents.sctp_shutdown_event         = 1;
   if(setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
      perror("setsockopt() for SCTP_EVENTS failed");
      exit(1);
   }


   puts("Connect ...");
   int c = connect(sd, (struct sockaddr*)&dst, sizeof(dst));
   if((c < 0) && (errno != EINPROGRESS)) { perror("connect"); exit(1); }


   puts("Select ...");
   FD_ZERO(&rs);
   FD_SET(sd, &rs);
   int result = select(sd + 1, &rs, NULL, NULL, NULL);
   printf("result=%d\n", result);


   printf("Read Notification ...\n");
   r = recv(sd, (char*)&notification, sizeof(notification), 0);
   if(r <= 0) { perror("read notification"); exit(1); }

   if( (notification.sn_header.sn_type != SCTP_ASSOC_CHANGE) ||
       (notification.sn_assoc_change.sac_state != SCTP_COMM_UP) ) {
      puts("Wrong notification.");
      exit(1);
   }

   sctp_assoc_t assoc = notification.sn_assoc_change.sac_assoc_id;
   printf("New association: %d\n",assoc);


   printf("Write ...\n");
   memset(&sri, 0, sizeof(sri));
   sri.sinfo_assoc_id = assoc;
   sri.sinfo_stream   = 0;
   sri.sinfo_ppid     = htonl(0x1234);
   w = sctp_send(sd, "TEST", 4, &sri, MSG_NOSIGNAL|MSG_EOR);
   if(w < 0) { perror("send"); exit(1); }


   printf("Read ...\n");
   r = recv(sd, (char*)&buffer, sizeof(buffer), 0);
   printf("read=%d\n", r);


/*
   int newSD = sctp_peeloff(sd, n->sn_assoc_change.sac_assoc_id);
   if(newSD >= 0) {
      printf("PEELOFF: %d\n", newSD);

      puts("poll...");
      fd_set ws;
      FD_ZERO(&ws);
      FD_SET(newSD, &ws);
      int result = select(newSD + 1, NULL, &ws, NULL, NULL);
      printf("result=%d\n", result);

      w = send(newSD, "YYYY", 4, MSG_NOSIGNAL|MSG_EOR);
      printf("write2=%d\n", w);

      printf("Read...\n");
      setBlocking(newSD);
      r = recv(newSD, (char*)&buffer, sizeof(buffer), 0);
      printf("read=%d\n", r);

      close(newSD);
   }
*/
   close(sd);
   return(0);
}
