#ifndef COMPONENTSTATUSPROTOCOL_H
#define COMPONENTSTATUSPROTOCOL_H

#include <sys/types.h>
#include <inttypes.h>
#include <ext_socket.h>
#include "sockaddrunion.h"
#include "dispatcher.h"
#include "timer.h"


#ifdef __cplusplus
extern "C" {
#endif


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


#define CSPH_STATUS_TEXT_SIZE       128
#define CSPH_COMPONENT_ADDRESS_SIZE 128

struct ComponentStatusProtocolHeader
{
   uint16_t                         Type;
   uint16_t                         Version;
   uint32_t                         Length;

   uint64_t                         SenderID;
   uint64_t                         ReportInterval;
   uint64_t                         SenderTimeStamp;

   char                             StatusText[CSPH_STATUS_TEXT_SIZE];
   char                             ComponentAddress[CSPH_COMPONENT_ADDRESS_SIZE];

   uint32_t                         Associations;
   struct ComponentAssociationEntry AssociationArray[0];
};

#define CSP_VERSION  0x0101
#define CSPHT_STATUS 0x0001


struct ComponentAssociationEntry* componentAssociationEntryArrayNew(const size_t elements);
void componentAssociationEntryArrayDelete(struct ComponentAssociationEntry* associationArray);




struct CSPReporter
{
   struct Dispatcher*   StateMachine;
   uint64_t             CSPIdentifier;
   union sockaddr_union CSPReportAddress;
   unsigned long long   CSPReportInterval;
   struct Timer         CSPReportTimer;

   size_t               (*CSPGetReportFunction)(
                           void*                              userData,
                           struct ComponentAssociationEntry** caeArray,
                           char*                              statusText,
                           char*                              componentAddress);
   void*                CSPGetReportFunctionUserData;
};


void componentStatusGetComponentAddress(char*        componentAddress,
                                        int          sd,
                                        sctp_assoc_t assocID);

void cspReporterNew(struct CSPReporter*         cspReporter,
                    struct Dispatcher*          dispatcher,
                    const uint64_t              cspIdentifier,
                    const union sockaddr_union* cspReportAddress,
                    const unsigned long long    cspReportInterval,
                    size_t                      (*cspGetReportFunction)(
                                                   void*                              userData,
                                                   struct ComponentAssociationEntry** caeArray,
                                                   char*                              statusText,
                                                   char*                              componentAddress),
                    void*                       cspGetReportFunctionUserData);
void cspReporterDelete(struct CSPReporter* cspReporter);
void cspReporterNewForRspLib(struct CSPReporter*         cspReporter,
                             const uint64_t              cspIdentifier,
                             const union sockaddr_union* cspReportAddress,
                             const unsigned long long    cspReportInterval);

#ifdef __cplusplus
}
#endif

#endif
