#include "componentstatusprotocol.h"
#include "timeutilities.h"
#include "netutilities.h"


struct ComponentAssociationEntry* componentAssociationEntryArrayNew(const size_t elements)
{
   struct ComponentAssociationEntry* associationArray =
      (struct ComponentAssociationEntry*)malloc(elements * sizeof(struct ComponentAssociationEntry));
   if(associationArray) {
      memset(associationArray, 0xff, elements * sizeof(struct ComponentAssociationEntry));
   }
   return(associationArray);
}


void componentAssociationEntryArrayDelete(struct ComponentAssociationEntry* associationArray)
{
   free(associationArray);
}


ssize_t componentStatusSend(const union sockaddr_union*             reportAddress,
                            const uint64_t                          reportInterval,
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
      csph->Version         = htons(CSP_VERSION);
      csph->Length          = htonl(length);
      csph->SenderID        = hton64(senderID);
      csph->SenderTimeStamp = hton64(getMicroTime());
      csph->ReportInterval  = hton64(reportInterval);
      strncpy((char*)&csph->StatusText, statusText, sizeof(csph->StatusText));
      csph->Associations = htonl(associations);
      for(i = 0;i < associations;i++) {
         csph->AssociationArray[i].ReceiverID = hton64(associationArray[i].ReceiverID);
         csph->AssociationArray[i].Duration   = hton64(associationArray[i].Duration);
         csph->AssociationArray[i].Flags      = htons(associationArray[i].Flags);
         csph->AssociationArray[i].ProtocolID = htons(associationArray[i].ProtocolID);
         csph->AssociationArray[i].PPID       = htonl(associationArray[i].PPID);
      }

      sd = ext_socket(reportAddress->sa.sa_family,
                      SOCK_DGRAM,
                      IPPROTO_UDP);
      if(sd >= 0) {
         setNonBlocking(sd);
         result = ext_sendto(sd, csph, length, 0,
                             &reportAddress->sa,
                             getSocklen(&reportAddress->sa));
         ext_close(sd);
      }

      free(csph);
   }
   return(result);
}
