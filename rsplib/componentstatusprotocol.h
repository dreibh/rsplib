#ifndef COMPONENTSTATUSPROTOCOL_H
#define COMPONENTSTATUSPROTOCOL_H


#include "sockaddrunion.h"


struct ComponentAssociationEntry
{
   uint64_t ReceiverID;
   uint16_t Flags;
   uint16_t ProtocolID;
   uint32_t PPID;
};


struct ComponentStatusProtocolHeader
{
   uint16_t                         Type;
   uint16_t                         Flags;
   uint32_t                         Length;

   uint64_t                         SenderID;
   uint64_t                         SenderTimeStamp;
   char                             StatusText[128];

   uint32_t                         Associations;
   struct ComponentAssociationEntry AssociationArray[];
};

#define CSPHT_STATUS 0x0001


ssize_t sendStatus(const union sockaddr_union*             statusMonitorAddress,
                   const uint64_t                          senderID,
                   const char*                             statusText,
                   const struct ComponentAssociationEntry* associationArray,
                   const size_t                            associations);


#endif
