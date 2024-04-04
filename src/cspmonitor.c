/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2024 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#include "tdtypes.h"
#include "debug.h"
#include "timeutilities.h"
#include "stringutilities.h"
#include "netutilities.h"
#include "breakdetector.h"
#include "componentstatusreporter.h"
#include "simpleredblacktree.h"

#include "rserpoolmessage.h"
#include "calcapppackets.h"
#include "fractalgeneratorpackets.h"
#include "pingpongpackets.h"
#include "scriptingpackets.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <ext_socket.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */



/* ###### Get description for ID ######################################### */
static void getDescriptionForID(const uint64_t id,
                                char*          buffer,
                                const size_t   bufferSize)
{
   switch(CID_GROUP(id)) {
      case CID_GROUP_REGISTRAR:
         snprintf(buffer, bufferSize,
                  "PR $%08Lx",
                  CID_OBJECT(id));
         break;
      case CID_GROUP_POOLELEMENT:
         snprintf(buffer, bufferSize,
                  "PE $%08Lx",
                  CID_OBJECT(id));
         break;
      case CID_GROUP_POOLUSER:
         snprintf(buffer, bufferSize,
                  "PU $%014Lx",
                  CID_OBJECT(id));
         break;
      default:
         snprintf(buffer, bufferSize,
                  "ID $%14Lx",
                  CID_OBJECT(id));
         break;
   }
}


struct ValueItem {
   uint32_t    Value;
   const char* String;
};

const struct ValueItem PPIDTable[] = {
   { 0x00000000,   "Data"      },
   { PPID_ENRP,    "ENRP"      },
   { PPID_ASAP,    "ASAP"      },
   { PPID_CALCAPP, "CalcApp"   },
   { PPID_FGP,     "FractGen"  },
   { PPID_SP,      "Scripting" },
   { PPID_PPP,     "PingPong"  }
};


/* ###### Get description for protocol ################################### */
static void getDescriptionForProtocol(const uint16_t protocolID,
                                      const uint32_t ppid,
                                      char*          buffer,
                                      const size_t   bufferSize)
{
   char ppidString[32];
   char protocolString[32];

   const char* ppidValue = NULL;
   for(unsigned int i = 0; i < sizeof(PPIDTable) / sizeof(struct ValueItem); i++) {
      if(PPIDTable[i].Value == ppid) {
         ppidValue = PPIDTable[i].String;
         break;
      }
   }
   if(ppidValue != NULL) {
      safestrcpy((char*)&ppidString, ppidValue, sizeof(ppidString));
   }
   else {
      snprintf((char*)&ppidString, sizeof(ppidString), "$%08x", ppid);
   }

   switch(protocolID) {
      case IPPROTO_SCTP:
         safestrcpy((char*)&protocolString, "SCTP", sizeof(protocolString));
       break;
      case IPPROTO_TCP:
         safestrcpy((char*)&protocolString, "TCP", sizeof(protocolString));
       break;
      case IPPROTO_UDP:
         safestrcpy((char*)&protocolString, "UDP", sizeof(protocolString));
       break;
      default:
         snprintf((char*)&protocolString, sizeof(protocolString), "$%04x", protocolID);
       break;
   }

   snprintf(buffer, bufferSize, "%s%s%s",
            ppidString,
            (ppidString[0] != 0x00) ? "/" : "",
            protocolString);
}


struct CSPObject
{
   struct SimpleRedBlackTreeNode StorageNode;
   struct SimpleRedBlackTreeNode DisplayNode;
   uint64_t                      Identifier;
   uint64_t                      LastReportTimeStamp;
   uint64_t                      ReportInterval;
   uint64_t                      SenderTimeStamp;
   char                          Description[128];
   char                          Status[128];
   char                          Location[128];
   double                        Workload;
   size_t                        Associations;
   struct ComponentAssociation*  AssociationArray;
};


static bool         useCompactMode         = true;
static size_t       currentObjectLabelSize = 0;
static unsigned int totalPRs               = 0;
static unsigned int totalPEs               = 0;
static unsigned int totalPUs               = 0;
static unsigned int currentPRs             = 0;
static unsigned int currentPEs             = 0;
static unsigned int currentPUs             = 0;
static unsigned int maxPRs                 = 6;
static unsigned int maxPEs                 = 42;
static unsigned int maxPUs                 = 24;
static unsigned int maxLocationSize        = 72;
static size_t       maxObjectLabelSize     = 0;


/* ###### Get CSPObject from given Display Node ########################## */
static const struct CSPObject* getCSPObjectFromDisplayNode(const void* displayNodePtr)
{
   const struct CSPObject* dummy = (const struct CSPObject*)displayNodePtr;
   long n = (long)displayNodePtr - ((long)&dummy->DisplayNode - (long)dummy);
   return((const struct CSPObject*)n);
}


/* ###### CSPObject print function ####################################### */
static void cspObjectDisplayPrint(const void* cspObjectPtr, FILE* fd)
{
   const struct CSPObject* cspObject = getCSPObjectFromDisplayNode(cspObjectPtr);
   char                    idString[256];
   char                    protocolString[64];
   char                    workloadString[32];
   char                    locationString[sizeof(cspObject->Location)];
   char                    uptimeString[32];
   char                    objectLabelString[384];
   char                    space[256];
   size_t                  objectLabelSize;
   unsigned int            h, m, s;
   size_t                  i;
   int                     color;

   color = 0;
   /* ====== Check component number limit to display ===================== */
   if(CID_GROUP(cspObject->Identifier) == CID_GROUP_REGISTRAR) {
      color = 31;
      currentPRs++;
      if(currentPRs > maxPRs) {
         if(currentPRs == maxPRs + 1) {
            fprintf(fd, "\n\x1b[%u;47m(%u further PRs have been hidden)\x1b[0m",
                    color, totalPRs -maxPRs);
         }
         return;
      }
   }
   else if(CID_GROUP(cspObject->Identifier) == CID_GROUP_POOLELEMENT) {
      color = 34;
      currentPEs++;
      if(currentPEs > maxPEs) {
         if(currentPEs == maxPEs + 1) {
            fprintf(fd, "\n\x1b[%u;47m(%u further PEs have been hidden)\x1b[0m",
                    color, totalPEs -maxPEs);
         }
         return;
      }
   }
   else if(CID_GROUP(cspObject->Identifier) == CID_GROUP_POOLUSER) {
      color = 32;
      currentPUs++;
      if(currentPUs > maxPUs) {
         if(currentPUs == maxPUs + 1) {
            fprintf(fd, "\n\x1b[%u;47m(%u further PUs have been hidden)\x1b[0m",
                    color, totalPUs -maxPUs);
         }
         return;
      }
   }

   /* ====== Get workload string ========================================= */
   if(cspObject->Workload >= 0.00) {
      if(cspObject->Workload < 0.90) {
         snprintf((char*)&workloadString, sizeof(workloadString), " L=%3u%%",
                  (unsigned int)rint(100.0 * cspObject->Workload));
      }
      else {
         snprintf((char*)&workloadString, sizeof(workloadString), " L=\x1b[7m%3u%%\x1b[27m",
                  (unsigned int)rint(100.0 * cspObject->Workload));
      }
   }
   else {
      workloadString[0] = 0x00;
   }

   /* ====== Get uptime string =========================================== */
   h = (unsigned int)(cspObject->SenderTimeStamp / (3600ULL * 1000000ULL));
   if(h < 100) {
      m = (unsigned int)((cspObject->SenderTimeStamp / (60ULL * 1000000ULL)) % 60);
      s = (unsigned int)((cspObject->SenderTimeStamp / 1000000ULL) % 60);
      snprintf((char*)&uptimeString, sizeof(uptimeString), "%02u:%02u:%02u", h, m, s);
   }
   else {
      if((h / 24) < 15000) {
         snprintf((char*)&uptimeString, sizeof(uptimeString), "%3ud %02uh", h / 24, h % 24);
      }
      else {
         safestrcpy((char*)&uptimeString, "??:??:??", sizeof(uptimeString));
      }
   }

   /* ====== Get object label string ===================================== */
   maxLocationSize = min(maxLocationSize, sizeof(cspObject->Location) - 1);
   memcpy((char*)&locationString, cspObject->Location, sizeof(cspObject->Location));
   i = strlen(locationString);
   if(i > maxLocationSize) {
      if(maxLocationSize >= 3) {
         locationString[maxLocationSize - 1] = '.';
         locationString[maxLocationSize - 2] = '.';
         locationString[maxLocationSize - 3] = '.';
      }
      locationString[maxLocationSize] = 0x00;
   }
   snprintf((char*)&objectLabelString, sizeof(objectLabelString),
            "\n%s [%s]",
            cspObject->Description,
            locationString);
   objectLabelSize    = strlen(objectLabelString);
   maxObjectLabelSize = max(maxObjectLabelSize, objectLabelSize);
   if(currentObjectLabelSize > objectLabelSize) {
      for(i = 0;i < min(sizeof(space) - 1, currentObjectLabelSize - objectLabelSize); i++) {
         space[i] = ' ';
      }
      space[i] = 0x00;
   }
   else {
      space[0] = 0x00;
   }

   fprintf(fd, "\x1b[%u;1m%s\x1b[0m\x1b[%um %s",
           color,
           objectLabelString,
           color,
           space);
   fprintf(fd, "U=%s l=%1.1fs A=%u%s \"%s\"\x1b[0K",
           uptimeString,
           (double)llabs(((int64_t)cspObject->LastReportTimeStamp - (int64_t)getMicroTime()) / 1000LL) / 1000.0,
           (unsigned int)cspObject->Associations,
           workloadString,
           cspObject->Status);
   if(!useCompactMode) {
      for(i = 0;i < cspObject->Associations;i++) {
         getDescriptionForID(cspObject->AssociationArray[i].ReceiverID,
                             (char*)&idString, sizeof(idString));
         getDescriptionForProtocol(cspObject->AssociationArray[i].ProtocolID,
                                   cspObject->AssociationArray[i].PPID,
                                   (char*)&protocolString, sizeof(protocolString));
         fprintf(fd, "\n   -> %s %-16s", idString, protocolString);
         if(cspObject->AssociationArray[i].Duration != ~0ULL) {
            h = (unsigned int)(cspObject->AssociationArray[i].Duration / (3600ULL * 1000000ULL));
            m = (unsigned int)((cspObject->AssociationArray[i].Duration / (60ULL * 1000000ULL)) % 60);
            s = (unsigned int)((cspObject->AssociationArray[i].Duration / 1000000ULL) % 60);
            fprintf(fd, "  duration=%u:%02u:%02u", h, m, s);
         }
         fputs("\x1b[0K", fd);
      }
   }
   fputs("\x1b[0m\x1b[0K", fd);
}


/* ###### CSPObject comparison function ################################## */
static int cspObjectDisplayComparison(const void* cspObjectPtr1, const void* cspObjectPtr2)
{
   const struct CSPObject* cspObject1 = getCSPObjectFromDisplayNode(cspObjectPtr1);
   const struct CSPObject* cspObject2 = getCSPObjectFromDisplayNode(cspObjectPtr2);

   if(CID_GROUP(cspObject1->Identifier) < CID_GROUP(cspObject2->Identifier)) {
      return(-1);
   }
   else if(CID_GROUP(cspObject1->Identifier) > CID_GROUP(cspObject2->Identifier)) {
      return(1);
   }

   const int result = strcmp((const char*)&cspObject1->Location,
                             (const char*)&cspObject2->Location);
   if(result != 0) {
      return(result);
   }

   if(cspObject1->Identifier < cspObject2->Identifier) {
      return(-1);
   }
   else if(cspObject1->Identifier > cspObject2->Identifier) {
      return(1);
   }
   return(0);
}


/* ###### CSPObject comparison function ################################## */
static int cspObjectStorageComparison(const void* cspObjectPtr1, const void* cspObjectPtr2)
{
   const struct CSPObject* cspObject1 = (const struct CSPObject*)cspObjectPtr1;
   const struct CSPObject* cspObject2 = (const struct CSPObject*)cspObjectPtr2;
   if(cspObject1->Identifier < cspObject2->Identifier) {
      return(-1);
   }
   else if(cspObject1->Identifier > cspObject2->Identifier) {
      return(1);
   }
   return(0);
}


/* ###### Find CSPObject in storage ###################################### */
struct CSPObject* findCSPObject(struct SimpleRedBlackTree* objectStorage,
                                const uint64_t             id)
{
   struct SimpleRedBlackTreeNode* node;
   struct CSPObject               cmpCSPObject;
   cmpCSPObject.Identifier = id;
   node = simpleRedBlackTreeFind(objectStorage, &cmpCSPObject.StorageNode);
   if(node) {
      return((struct CSPObject*)node);
   }
   return(NULL);
}


/* ###### Purge out-of-date CSPObject in storage ######################### */
void purgeCSPObjects(struct SimpleRedBlackTree* objectStorage,
                     struct SimpleRedBlackTree* objectDisplay,
                     const unsigned long long purgeInterval)
{
   struct CSPObject* cspObject;
   struct CSPObject* nextCSPObject;

   cspObject = (struct CSPObject*)simpleRedBlackTreeGetFirst(objectStorage);
   while(cspObject != NULL) {
      nextCSPObject = (struct CSPObject*)simpleRedBlackTreeGetNext(objectStorage, &cspObject->StorageNode);
      if(cspObject->LastReportTimeStamp + min(purgeInterval, (10 * cspObject->ReportInterval)) < getMicroTime()) {
         CHECK(simpleRedBlackTreeRemove(objectStorage, &cspObject->StorageNode) == &cspObject->StorageNode);
         switch(CID_GROUP(cspObject->Identifier)) {
            case CID_GROUP_REGISTRAR:
              assert(totalPRs > 0); totalPRs--;
             break;
            case CID_GROUP_POOLELEMENT:
              assert(totalPEs > 0); totalPEs--;
             break;
            case CID_GROUP_POOLUSER:
              assert(totalPUs > 0); totalPUs--;
             break;
         }
         simpleRedBlackTreeVerify(objectStorage);
         CHECK(simpleRedBlackTreeRemove(objectDisplay, &cspObject->DisplayNode) == &cspObject->DisplayNode);
         simpleRedBlackTreeVerify(objectDisplay);
         simpleRedBlackTreeNodeDelete(&cspObject->StorageNode);
         simpleRedBlackTreeNodeDelete(&cspObject->DisplayNode);
         free(cspObject->AssociationArray);
         free(cspObject);
      }
      cspObject = nextCSPObject;
   }
}


/* ###### Handle incoming CSP message #################################### */
static void handleMessage(int                        sd,
                          struct SimpleRedBlackTree* objectStorage,
                          struct SimpleRedBlackTree* objectDisplay)
{
   struct ComponentStatusReport* cspReport;
   struct CSPObject*             cspObject;
   char                          buffer[65536];
   ssize_t                       received;
   size_t                        i;

   received = ext_recv(sd, (char*)&buffer, sizeof(buffer), 0);
   if(received) {
      cspReport = (struct ComponentStatusReport*)&buffer;
      if( (received >= (ssize_t)sizeof(struct ComponentStatusReport)) &&
          (cspReport->Header.Type == CSPT_REPORT) &&
          (ntohl(cspReport->Header.Version) == CSP_VERSION) ) {
         cspReport->Header.Version         = ntohl(cspReport->Header.Version);
         cspReport->Header.Length          = ntohs(cspReport->Header.Length);
         cspReport->Header.SenderID        = ntoh64(cspReport->Header.SenderID);
         cspReport->Header.SenderTimeStamp = ntoh64(cspReport->Header.SenderTimeStamp);
         cspReport->ReportInterval         = ntohl(cspReport->ReportInterval);
         cspReport->Workload               = ntohs(cspReport->Workload);
         cspReport->Associations           = ntohs(cspReport->Associations);
         if(sizeof(struct ComponentStatusReport) + (cspReport->Associations * sizeof(struct ComponentAssociation)) == (size_t)received) {
            for(i = 0;i < cspReport->Associations;i++) {
               cspReport->AssociationArray[i].ReceiverID = ntoh64(cspReport->AssociationArray[i].ReceiverID);
               cspReport->AssociationArray[i].Duration   = ntoh64(cspReport->AssociationArray[i].Duration);
               cspReport->AssociationArray[i].Flags      = ntohs(cspReport->AssociationArray[i].Flags);
               cspReport->AssociationArray[i].ProtocolID = ntohs(cspReport->AssociationArray[i].ProtocolID);
               cspReport->AssociationArray[i].PPID       = ntohl(cspReport->AssociationArray[i].PPID);
            }

            cspObject = findCSPObject(objectStorage, cspReport->Header.SenderID);
            if(cspObject == NULL) {
               cspObject = (struct CSPObject*)malloc(sizeof(struct CSPObject));
               if(cspObject) {
                  simpleRedBlackTreeNodeNew(&cspObject->StorageNode);
                  simpleRedBlackTreeNodeNew(&cspObject->DisplayNode);
                  cspObject->Identifier       = cspReport->Header.SenderID;
                  cspObject->AssociationArray = NULL;
                  switch(CID_GROUP(cspObject->Identifier)) {
                     case CID_GROUP_REGISTRAR:
                       totalPRs++;
                      break;
                     case CID_GROUP_POOLELEMENT:
                       totalPEs++;
                      break;
                     case CID_GROUP_POOLUSER:
                       totalPUs++;
                      break;
                  }
               }
            }
            if(cspObject) {
               if(simpleRedBlackTreeNodeIsLinked(&cspObject->DisplayNode)) {
                  /* The position within the objectDisplay storage may change,
                     due to updated locationArray! */
                  CHECK(simpleRedBlackTreeRemove(objectDisplay, &cspObject->DisplayNode) == &cspObject->DisplayNode);
                  simpleRedBlackTreeVerify(objectDisplay);
               }

               if(cspReport->Header.Flags != 0x00) {
                  cspObject->LastReportTimeStamp = 0;
               }
               else {
                  cspObject->LastReportTimeStamp = getMicroTime();
                  cspObject->SenderTimeStamp     = cspReport->Header.SenderTimeStamp;
                  cspObject->ReportInterval      = cspReport->ReportInterval;
                  cspObject->Workload            = CSR_GET_WORKLOAD(cspReport->Workload);
                  getDescriptionForID(cspObject->Identifier,
                                      (char*)&cspObject->Description,
                                      sizeof(cspObject->Description));
                  memcpy(&cspObject->Status,
                         &cspReport->Status,
                         sizeof(cspObject->Status));
                  cspObject->Status[sizeof(cspObject->Status) - 1] = 0x00;
                  memcpy(&cspObject->Location,
                         &cspReport->Location,
                         sizeof(cspObject->Location));

                  /* DisplayNode MUST be re-inserted, since the location may change and
                     the location is part of the sorting key! */
                  /*
                  for(int i=0;i < 6;i++) {
                    cspObject->Location[i] = 'A' + (random() % 26);
                  }
                  */

                  cspObject->Location[sizeof(cspObject->Location) - 1] = 0x00;
               }
               if(cspObject->AssociationArray) {
                  deleteComponentAssociationArray(cspObject->AssociationArray);
               }
               cspObject->AssociationArray = createComponentAssociationArray(cspReport->Associations);
               CHECK(cspObject->AssociationArray);
               memcpy(cspObject->AssociationArray, &cspReport->AssociationArray, cspReport->Associations * sizeof(struct ComponentAssociation));
               cspObject->Associations = cspReport->Associations;
               CHECK(simpleRedBlackTreeInsert(objectStorage,
                                              &cspObject->StorageNode) == &cspObject->StorageNode);
               simpleRedBlackTreeVerify(objectStorage);
               CHECK(simpleRedBlackTreeInsert(objectDisplay,
                                              &cspObject->DisplayNode) == &cspObject->DisplayNode);
               simpleRedBlackTreeVerify(objectDisplay);
            }
         }
      }
   }
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   union sockaddr_union           localAddress;
   struct pollfd                  ufds;
   struct SimpleRedBlackTree      objectStorage;
   struct SimpleRedBlackTree      objectDisplay;
   struct SimpleRedBlackTreeNode* node;
   unsigned long long             now;
   unsigned long long             updateInterval = 1000000;
   unsigned long long             purgeInterval  = 30000000;
   unsigned long long             lastUpdate     = 0;
   size_t                         lastElements   = ~0;
   size_t                         elements;
   int                            result;
   int                            sd;
   int                            n;

   if(checkIPv6()) {
      string2address("[::]:0", &localAddress);
      setPort(&localAddress.sa, 2960);
   }
   else {
      string2address("0.0.0.0:0", &localAddress);
      setPort(&localAddress.sa, 2960);
   }
   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-localaddress=", 14))) {
         if(string2address((char*)&argv[n][14], &localAddress) == false) {
            fprintf(stderr, "ERROR: Bad local address <%s>\n", (char*)&argv[n][14]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[n], "-updateinterval=", 16))) {
         updateInterval = 1000 * atol((char*)&argv[n][16]);
         if(updateInterval < 100000) {
            updateInterval = 100000;
         }
      }
      else if(!(strncmp(argv[n], "-purgeinterval=", 15))) {
         purgeInterval = 1000 * atol((const char*)&argv[n][15]);
         if(purgeInterval < 1000000) {
            purgeInterval = 1000000;
         }
      }
      else if(!(strncmp(argv[n], "-maxpr=", 7))) {
         maxPRs = atoi((const char*)&argv[n][7]);
      }
      else if(!(strncmp(argv[n], "-maxpe=", 7))) {
         maxPEs = atoi((const char*)&argv[n][7]);
      }
      else if(!(strncmp(argv[n], "-maxpu=", 7))) {
         maxPUs = atoi((const char*)&argv[n][7]);
      }
      else if(!(strncmp(argv[n], "-maxlocationsize=", 17))) {
         maxLocationSize = atoi((const char*)&argv[n][17]);
      }
      else if(!(strcmp(argv[n], "-compact"))) {
         useCompactMode = true;
      }
      else if(!(strcmp(argv[n], "-full"))) {
         useCompactMode = false;
      }
      else {
         printf("Bad argument \"%s\"!\n" ,argv[n]);
         fprintf(stderr, "Usage: %s {-localaddress=address:port} {-updateinterval=milliseconds} {-purgeinterval=milliseconds} {-compact|-full} {-maxpr=PRs} {-maxpe=PEs} {-maxpu=PUs} {-maxlocationsize=characters}\n", argv[0]);
         exit(1);
      }
   }

   sd = ext_socket(localAddress.sa.sa_family, SOCK_DGRAM, IPPROTO_UDP);
   if(sd < 0) {
      perror("Unable to create socket");
      exit(1);
   }
   if(!setReusable(sd, 1)) {
      perror("setReusable() failed");
   }
   setIPv6Only(sd, 0);
   if(bindplus(sd, &localAddress, 1) == false) {
      fputs("ERROR: Unable to bind socket to local address\n", stderr);
      exit(1);
   }

   simpleRedBlackTreeNew(&objectStorage, NULL,                  cspObjectStorageComparison);
   simpleRedBlackTreeNew(&objectDisplay, cspObjectDisplayPrint, cspObjectDisplayComparison);

   puts("Component Status Monitor - Version 1.0");
   puts("======================================\n");

   installBreakDetector();
   printf("\x1b[;H\x1b[2J");

   /* The first update should be in 1 second ... */
   lastUpdate = getMicroTime() + 1000000 - updateInterval;

   while(!breakDetected()) {
      ufds.fd          = sd;
      ufds.events      = POLLIN;
      now              = getMicroTime();
      while(now - lastUpdate < updateInterval) {
         now = getMicroTime();
         result = ext_poll(&ufds, 1,
                           ((lastUpdate + updateInterval) > now) ?
                              (int)((lastUpdate + updateInterval - now) / 1000) : 0);
         if((result > 0) && (ufds.revents & POLLIN)) {
            handleMessage(sd, &objectStorage, &objectDisplay);
         }
         else if((result < 0) && (errno == EINTR)) {
            goto finished;
         }
      }
      purgeCSPObjects(&objectStorage, &objectDisplay, purgeInterval);

      elements = simpleRedBlackTreeGetElements(&objectStorage);

      if( (elements != lastElements) || (elements > 0) ) {
         printf("\x1b[;H");
         printTimeStamp(stdout);
         printf("Current Component Status -- \x1b[31;1m%u PRs\x1b[0m, \x1b[34;1m%u PEs\x1b[0m, \x1b[32;1m%u PUs\x1b[0m\x1b[0K\n\x1b[0K\n\x1b[0K\x1b[;H\n",
                totalPRs, totalPEs, totalPUs);
         maxObjectLabelSize = 0;
         currentPRs         = 0;
         currentPEs         = 0;
         currentPUs         = 0;

         node = simpleRedBlackTreeGetFirst(&objectDisplay);
         while(node != NULL) {
            cspObjectDisplayPrint(node, stdout);
            node = simpleRedBlackTreeGetNext(&objectDisplay, node);
         }

         currentObjectLabelSize = maxObjectLabelSize;
         printf("\x1b[0J");
         fflush(stdout);
      }

      lastElements = elements;
      lastUpdate   = now;
   }

finished:
   ext_close(sd);
   simpleRedBlackTreeDelete(&objectStorage);
   puts("\nTerminated!");
   return(0);
}
