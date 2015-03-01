/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2004-2015 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#include "componentstatusreporter.h"
#include "rserpool.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "loglevel.h"
#include "netutilities.h"
#include "stringutilities.h"
#include "randomizer.h"

#include <math.h>
#include <sys/utsname.h>


extern struct Dispatcher gDispatcher;


static void cspReporterCallback(struct Dispatcher* dispatcher,
                                struct Timer*      timer,
                                void*              userData);
static void sendCSPReport(struct CSPReporter* cspReporter, const bool final);


/* ###### Create new ComponentAssociation array ########################## */
struct ComponentAssociation* createComponentAssociationArray(const size_t elements)
{
   struct ComponentAssociation* associationArray =
      (struct ComponentAssociation*)malloc(elements * sizeof(struct ComponentAssociation));
   if(associationArray) {
      memset(associationArray, 0xff, elements * sizeof(struct ComponentAssociation));
   }
   return(associationArray);
}


/* ###### Delete ComponentAssociation array ############################## */
void deleteComponentAssociationArray(struct ComponentAssociation* associationArray)
{
   free(associationArray);
}


/* ###### Fill in component location string ############################## */
void getComponentLocation(char*        componentLocation,
                          int          sd,
                          sctp_assoc_t assocID)
{
   char                  str[CSPR_LOCATION_SIZE];
   struct utsname        hostInfo;
   union sockaddr_union* addressArray;
   int                   addresses;
   int                   copiedAddresses;
   int                   i, j;
   unsigned int          minScope;
   char*                 s;

   componentLocation[0] = 0x00;

   /* ====== Get hostname ================================================ */
   if(uname(&hostInfo) == 0) {
      safestrcat(componentLocation, hostInfo.nodename, CSPR_LOCATION_SIZE);
   }

   /* ====== Get address(es) ============================================= */
   addresses = 0;
   if(sd >= 0) {
      addresses = getladdrsplus(sd, assocID, &addressArray);
   }
   if( (sd < 1) ||
       ( (addresses == 1) && ( ((addressArray[0].sa.sa_family == AF_INET) &&
                                (addressArray[0].in.sin_addr.s_addr == INADDR_ANY)) ||
                               ((addressArray[0].sa.sa_family == AF_INET6) &&
                                (IN6_IS_ADDR_UNSPECIFIED(&(addressArray[0].in6.sin6_addr)))) ) ) ) {
      if(addresses > 0) {
         free(addressArray);
      }
      addresses = gatherLocalAddresses(&addressArray);
   }
   
   if(addresses > 0) {
      copiedAddresses = 0;
      minScope        = AS_UNICAST_GLOBAL;
      for(j = 0; j <= 1; j++) {
         for(i = 0;i < addresses;i++) {
            if(getScope((const struct sockaddr*)&addressArray[i]) >= minScope) {
               if(address2string((const struct sockaddr*)&addressArray[i],
                                 (char*)&str, sizeof(str),
                                 ((copiedAddresses == 0) && (sd >= 0)) ? true : false)) {
                  if(componentLocation[0] != 0x00) {
                     safestrcat(componentLocation,
                                (copiedAddresses > 0) ? ", " : ": ", CSPR_LOCATION_SIZE);
                  }
                  if(strncmp(str, "::ffff:", 7) == 0) {
                     safestrcat(componentLocation, (const char*)&str[7], CSPR_LOCATION_SIZE);
                  }
                  else if(strncmp(str, "[::ffff:", 8) == 0) {
                     s = index(str, ']');
                     while(*s != 0x00) {
                        *s = s[1];
                        s++;
                     }
                     safestrcat(componentLocation, (const char*)&str[8], CSPR_LOCATION_SIZE);
                  }
                  else {
                     safestrcat(componentLocation, str, CSPR_LOCATION_SIZE);
                  }
                  copiedAddresses++;
               }
            }
         }
         if(copiedAddresses > 0) {
            /* At least one address of current minScope has been found ... */
            break;
         }
         minScope = AS_UNICAST_SITELOCAL;
      }
      free(addressArray);
   }
   if(componentLocation[0] == 0x00) {
      snprintf(componentLocation, CSPR_LOCATION_SIZE, "(local only)");
   }
}


/* ###### Constructor #################################################### */
void cspReporterNew(struct CSPReporter*    cspReporter,
                    struct Dispatcher*     dispatcher,
                    const uint64_t         cspIdentifier,
                    const struct sockaddr* cspReportAddress,
                    const unsigned int     cspReportInterval,
                    size_t                 (*cspGetReportFunction)(
                                              void*                         userData,
                                              uint64_t*                     identifier,
                                              struct ComponentAssociation** caeArray,
                                              char*                         statusText,
                                              char*                         componentAddress,
                                              double*                       workload),
                    void*                  cspGetReportFunctionUserData)
{
   cspReporter->StateMachine = dispatcher;
   memcpy(&cspReporter->CSPReportAddress,
          cspReportAddress,
          getSocklen(cspReportAddress));
   cspReporter->CSPReportInterval            = cspReportInterval;
   cspReporter->CSPIdentifier                = cspIdentifier;
   cspReporter->CSPGetReportFunction         = cspGetReportFunction;
   cspReporter->CSPGetReportFunctionUserData = cspGetReportFunctionUserData;
   cspReporter->StatusTextOverride           = NULL;
   timerNew(&cspReporter->CSPReportTimer,
            cspReporter->StateMachine,
            cspReporterCallback,
            cspReporter);
   timerStart(&cspReporter->CSPReportTimer, 0);
}


/* ###### Destructor ##################################################### */
void cspReporterDelete(struct CSPReporter* cspReporter)
{
   timerDelete(&cspReporter->CSPReportTimer);
   sendCSPReport(cspReporter, true);
   cspReporter->StateMachine                 = NULL;
   cspReporter->CSPGetReportFunction         = NULL;
   cspReporter->CSPGetReportFunctionUserData = NULL;
}


/* ###### Send CSP Status message ######################################## */
static ssize_t componentStatusSend(const union sockaddr_union*        reportAddress,
                                   const uint64_t                     reportInterval,
                                   const uint64_t                     senderID,
                                   const char*                        statusText,
                                   const char*                        componentLocation,
                                   const double                       workload,
                                   const struct ComponentAssociation* associationArray,
                                   const size_t                       associations,
                                   const uint8_t                      flags)
{
   static unsigned long long     startupTime = 0;;
   struct ComponentStatusReport* cspReport;
   size_t                        i;
   int                           sd;
   ssize_t                       result;
   const size_t                  length = sizeof(struct ComponentStatusReport) +
                                             (associations * sizeof(struct ComponentAssociation));

   result = -1;
   cspReport   = (struct ComponentStatusReport*)malloc(length);
   if(cspReport) {
      if(startupTime == 0) {
         startupTime = getMicroTime();
      }
      cspReport->Header.Type            = CSPT_REPORT;
      cspReport->Header.Flags           = flags;
      cspReport->Header.Version         = htonl(CSP_VERSION);
      cspReport->Header.Length          = htonl(length);
      cspReport->Header.SenderID        = hton64(senderID);
      cspReport->Header.SenderTimeStamp = hton64(getMicroTime() - startupTime);
      cspReport->ReportInterval         = htonl(reportInterval);
      cspReport->Workload               = htons(CSR_SET_WORKLOAD(workload));
      strncpy((char*)&cspReport->Status, statusText, sizeof(cspReport->Status));
      strncpy((char*)&cspReport->Location, componentLocation, sizeof(cspReport->Location));
      cspReport->Associations = htons(associations);
      for(i = 0;i < associations;i++) {
         cspReport->AssociationArray[i].ReceiverID = hton64(associationArray[i].ReceiverID);
         cspReport->AssociationArray[i].Duration   = hton64(associationArray[i].Duration);
         cspReport->AssociationArray[i].Flags      = htons(associationArray[i].Flags);
         cspReport->AssociationArray[i].ProtocolID = htons(associationArray[i].ProtocolID);
         cspReport->AssociationArray[i].PPID       = htonl(associationArray[i].PPID);
      }

      sd = ext_socket(reportAddress->sa.sa_family,
                      SOCK_DGRAM,
                      IPPROTO_UDP);
      if(sd >= 0) {
         setNonBlocking(sd);
         result = ext_sendto(sd, cspReport, length, 0,
                             &reportAddress->sa,
                             getSocklen(&reportAddress->sa));
         ext_close(sd);
      }

      free(cspReport);
   }
   return(result);
}


/* ###### Report status ################################################## */
static void sendCSPReport(struct CSPReporter* cspReporter, const bool final)
{
   char                         statusText[CSPR_STATUS_SIZE];
   char                         componentLocation[CSPR_LOCATION_SIZE];
   struct ComponentAssociation* caeArray     = NULL;
   size_t                       caeArraySize = 0;
   double                       workload;

   LOG_VERBOSE4
   fputs("Creating and sending CSP report...\n", stdlog);
   LOG_END

   statusText[0] = 0x00;
   if(final == false) {
      caeArraySize = cspReporter->CSPGetReportFunction(cspReporter->CSPGetReportFunctionUserData,
                                                       &cspReporter->CSPIdentifier,
                                                       &caeArray,
                                                       (char*)&statusText,
                                                       (char*)&componentLocation,
                                                       &workload);
   }
   else {
      statusText[0]        = 0x00;
      componentLocation[0] = 0x00;
      workload             = 0.0;
   }
   if(CID_OBJECT(cspReporter->CSPIdentifier) != 0ULL) {
      componentStatusSend(&cspReporter->CSPReportAddress,
                          cspReporter->CSPReportInterval,
                          cspReporter->CSPIdentifier,
                          (cspReporter->StatusTextOverride != NULL) ? cspReporter->StatusTextOverride : statusText,
                          componentLocation,
                          workload,
                          caeArray, caeArraySize,
                          (final == true) ? CSPF_FINAL : 0x00);
   }
   if(caeArray) {
      deleteComponentAssociationArray(caeArray);
   }

   LOG_VERBOSE4
   fputs("Sending CSP report completed\n", stdlog);
   LOG_END
}


/* ###### Timer callback to report status ################################ */
static void cspReporterCallback(struct Dispatcher* dispatcher,
                                struct Timer*      timer,
                                void*              userData)
{
   struct CSPReporter* cspReporter = (struct CSPReporter*)userData;

   sendCSPReport(cspReporter, false);
   timerStart(&cspReporter->CSPReportTimer,
           getMicroTime() + cspReporter->CSPReportInterval);
}


/* ###### Report status during registrar search ########################## */
void cspReporterHandleTimerDuringRegistrarSearch(struct CSPReporter* cspReporter)
{
   cspReporter->StatusTextOverride = "Looking for Registrar ...";
   timerStop(&cspReporter->CSPReportTimer);
   cspReporterCallback(cspReporter->StateMachine, &cspReporter->CSPReportTimer, cspReporter);
   cspReporter->StatusTextOverride = NULL;
}
