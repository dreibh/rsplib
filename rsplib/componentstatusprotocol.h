#ifndef COMPONENTSTATUSPROTOCOL_H
#define COMPONENTSTATUSPROTOCOL_H

#include "sockaddrunion.h"

#define CID_GROUP(id)  (((uint64_t)id >> 56) & (0xffffLL))
#define CID_OBJECT(id) ((uint64_t)id & 0xffffffffffffffLL)

#define CID_GROUP_NAMESERVER  0x0001
#define CID_GROUP_POOLELEMENT 0x0002
#define CID_GROUP_POOLUSER    0x0003

#define CID_COMPOUND(group, object)  ((((uint64_t)group) << 56) | (uint64_t)object)


struct ComponentAssociationEntry
{
   uint64_t ReceiverID;
   uint64_t Duration;
   uint16_t Flags;
   uint16_t ProtocolID;
   uint32_t PPID;
};


struct ComponentStatusProtocolHeader
{
   uint16_t                         Type;
   uint16_t                         Version;
   uint32_t                         Length;

   uint64_t                         SenderID;
   uint64_t                         ReportInterval;
   uint64_t                         SenderTimeStamp;
   char                             StatusText[128];

   uint32_t                         Associations;
   struct ComponentAssociationEntry AssociationArray[];
};

#define CSP_VERSION  0x0100
#define CSPHT_STATUS 0x0001


struct ComponentAssociationEntry* componentAssociationEntryArrayNew(const size_t elements);
void componentAssociationEntryArrayDelete(struct ComponentAssociationEntry* associationArray);

ssize_t componentStatusSend(const union sockaddr_union*             reportAddress,
                            const uint64_t                          reportInterval,
                            const uint64_t                          senderID,
                            const char*                             statusText,
                            const struct ComponentAssociationEntry* associationArray,
                            const size_t                            associations);


#endif
