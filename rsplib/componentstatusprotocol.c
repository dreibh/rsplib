#include "componentstatusprotocol.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "loglevel.h"
#include "rsplib.h"


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


/* ###### Report status ################################################## */
static void cspReporterCallback(struct Dispatcher* dispatcher,
                                struct Timer*      timer,
                                void*              userData)
{
   struct CSPReporter*               cspReporter = (struct CSPReporter*)userData;
   struct ComponentAssociationEntry* caeArray    = NULL;
   char                              statusText[CSPH_STATUS_TEXT_SIZE];
   size_t                            caeArraySize;

   LOG_VERBOSE3
   fputs("Creating and sending CSP report...\n", stdlog);
   LOG_END

   statusText[0] = 0x00;
   caeArraySize = cspReporter->CSPGetReportFunction(cspReporter->CSPGetReportFunctionUserData,
                                                    &caeArray,
                                                    (char*)&statusText);

   componentStatusSend(&cspReporter->CSPReportAddress,
                       cspReporter->CSPReportInterval,
                       cspReporter->CSPIdentifier,
                       statusText,
                       caeArray, caeArraySize);

   if(caeArray) {
      componentAssociationEntryArrayDelete(caeArray);
   }

   timerStart(&cspReporter->CSPReportTimer,
              getMicroTime() + cspReporter->CSPReportInterval);

   LOG_VERBOSE3
   fputs("Sending CSP report completed\n", stdlog);
   LOG_END
}


void cspReporterNew(struct CSPReporter*         cspReporter,
                    struct Dispatcher*          dispatcher,
                    const uint64_t              cspIdentifier,
                    const union sockaddr_union* cspReportAddress,
                    const unsigned long long    cspReportInterval,
                    size_t                      (*cspGetReportFunction)(
                                                   void*                              userData,
                                                   struct ComponentAssociationEntry** caeArray,
                                                   char*                              statusText),
                    void*                       cspGetReportFunctionUserData)
{
   cspReporter->StateMachine = dispatcher;
   memcpy(&cspReporter->CSPReportAddress,
          cspReportAddress,
          getSocklen(&cspReportAddress->sa));
   cspReporter->CSPReportInterval            = cspReportInterval;
   cspReporter->CSPIdentifier                = cspIdentifier;
   cspReporter->CSPGetReportFunction         = cspGetReportFunction;
   cspReporter->CSPGetReportFunctionUserData = cspGetReportFunctionUserData;
   timerNew(&cspReporter->CSPReportTimer,
            cspReporter->StateMachine,
            cspReporterCallback,
            cspReporter);
   timerStart(&cspReporter->CSPReportTimer, 0);
}


void cspReporterDelete(struct CSPReporter* cspReporter)
{
   timerDelete(&cspReporter->CSPReportTimer);
   cspReporter->StateMachine                 = NULL;
   cspReporter->CSPGetReportFunction         = NULL;
   cspReporter->CSPGetReportFunctionUserData = NULL;
}


extern struct Dispatcher gDispatcher;


static size_t rsplibGetReportFunction(
                 void*                              userData,
                 struct ComponentAssociationEntry** caeArray,
                 char*                              statusText)
{
   return(rspGetComponentStatus(caeArray, statusText));
}


void cspReporterNewForRspLib(struct CSPReporter*         cspReporter,
                             const uint64_t              cspIdentifier,
                             const union sockaddr_union* cspReportAddress,
                             const unsigned long long    cspReportInterval)
{
   cspReporterNew(cspReporter,
                  &gDispatcher,
                  cspIdentifier,
                  cspReportAddress,
                  cspReportInterval,
                  rsplibGetReportFunction,
                  NULL);
}
