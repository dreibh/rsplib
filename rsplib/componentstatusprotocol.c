#include "componentstatusprotocol.h"
#include "timeutilities.h"
#include "netutilities.h"


ssize_t sendStatus(const union sockaddr_union*             statusMonitorAddress,
                   const uint64_t                          senderID,
                   const char*                             statusText,
                   const struct ComponentAssociationEntry* associationArray,
                   const size_t                            associations)
{
   struct ComponentStatusProtocolHeader* csph;
   size_t       i;
   int          sd;
   ssize_t      result;
   const size_t length = sizeof(struct ComponentStatusProtocolHeader) +
                            (associations * sizeof(struct ComponentAssociationEntry));

   result = -1;
   csph   = (struct ComponentStatusProtocolHeader*)malloc(length);
   if(csph) {
      csph->Type            = htons(CSPHT_STATUS);
      csph->Flags           = 0;
      csph->Length          = htonl(length);
      csph->SenderID        = hton64(senderID);
      csph->SenderTimeStamp = hton64(getMicroTime());
      strncpy((char*)&csph->StatusText, statusText, sizeof(csph->StatusText));
      csph->Associations = htonl(associations);
      for(i = 0;i < associations;i++) {
         csph->AssociationArray[i].ReceiverID = hton64(associationArray[i].ReceiverID);
         csph->AssociationArray[i].Flags      = htons(associationArray[i].Flags);
         csph->AssociationArray[i].ProtocolID = htons(associationArray[i].ProtocolID);
         csph->AssociationArray[i].PPID       = htonl(associationArray[i].PPID);
      }

      sd = socket(statusMonitorAddress->sa.sa_family,
                  SOCK_DGRAM,
                  IPPROTO_UDP);
      if(sd >= 0) {
         result = sendto(sd, csph, length, 0,
                         &statusMonitorAddress->sa,
                         getSocklen(&statusMonitorAddress->sa));
if(result < 0) perror("send");
         close(sd);
      }
      else perror("socket");

      free(csph);
   }
   return(result);
}
