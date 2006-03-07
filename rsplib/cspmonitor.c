/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "breakdetector.h"
#include "componentstatusreporter.h"
#include "simpleredblacktree.h"

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
                  "Registrar    $%08Lx",
                  CID_OBJECT(id));
         break;
      case CID_GROUP_POOLELEMENT:
         snprintf(buffer, bufferSize,
                  "Pool Element $%08Lx",
                  CID_OBJECT(id));
         break;
      case CID_GROUP_POOLUSER:
         snprintf(buffer, bufferSize,
                  "Pool User    $%08Lx",
                  CID_OBJECT(id));
         break;
      default:
         snprintf(buffer, bufferSize,
                  "ID $%Lx",
                  CID_OBJECT(id));
         break;
   }
}



struct CSPObject
{
   struct SimpleRedBlackTreeNode Node;
   uint64_t                      Identifier;
   uint64_t                      LastReportTimeStamp;
   uint64_t                      ReportInterval;
   char                          Description[128];
   char                          Status[128];
   char                          Location[128];
   double                        Workload;
   size_t                        Associations;
   struct ComponentAssociation*  AssociationArray;
};


/* ###### CSPObject print function ####################################### */
static void cspObjectPrint(const void* cspObjectPtr, FILE* fd)
{
   const struct CSPObject* cspObject = (const struct CSPObject*)cspObjectPtr;
   char                    str[256];
   char                    workload[16];
   size_t                  i;
   int                     color;

   if(cspObject->Workload >= 0.0) {
      snprintf((char*)&workload, sizeof(workload), ", L=%u%%",
               (unsigned int)rint(100.0 * cspObject->Workload));
   }
   else {
      workload[0] = 0x00;
   }

   color = 31 + (unsigned int)(CID_GROUP(cspObject->Identifier) % 8);
   fprintf(fd, "\x1b[%u;47m%s [%s]:\x1b[0m\x1b[%um lr=%5ums, int=%4Ldms, A=%u%s \"%s\"\x1b[0K\n",
           color,
           cspObject->Description,
           cspObject->Location,
           color,
           abs(((int64_t)cspObject->LastReportTimeStamp - (int64_t)getMicroTime()) / 1000),
           cspObject->ReportInterval / 1000,
           (unsigned int)cspObject->Associations,
           workload,
           cspObject->Status);
   for(i = 0;i < cspObject->Associations;i++) {
      getDescriptionForID(cspObject->AssociationArray[i].ReceiverID,
                          (char*)&str, sizeof(str));
      switch(cspObject->AssociationArray[i].ProtocolID) {
         case IPPROTO_SCTP:
            fprintf(fd,"   -> %s SCTP ppid=$%08x",
                  str,
                  cspObject->AssociationArray[i].PPID);
          break;
         case IPPROTO_TCP:
            fprintf(fd,"   -> %s TCP", str);
          break;
         case IPPROTO_UDP:
            fprintf(fd,"   -> %s UDP", str);
          break;
         default:
            fprintf(fd,"   -> %s proto=%d ppid=$%08x",
                  str,
                  cspObject->AssociationArray[i].ProtocolID,
                  cspObject->AssociationArray[i].PPID);
          break;
      }
      if(cspObject->AssociationArray[i].Duration != ~0ULL) {
         fprintf(fd, "  duration=%4llu.%03llus",
                 cspObject->AssociationArray[i].Duration / 1000000,
                 (cspObject->AssociationArray[i].Duration % 1000000) / 1000);
      }
      fputs("\x1b[0K\n", fd);
   }
   fputs("\x1b[0m\x1b[0K", fd);
}


/* ###### CSPObject comparision function ################################# */
static int cspObjectComparison(const void* cspObjectPtr1, const void* cspObjectPtr2)
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
                                const uint64_t                 id)
{
   struct SimpleRedBlackTreeNode* node;
   struct CSPObject               cmpCSPObject;
   cmpCSPObject.Identifier = id;
   node = simpleRedBlackTreeFind(objectStorage, &cmpCSPObject.Node);
   if(node) {
      return((struct CSPObject*)node);
   }
   return(NULL);
}


/* ###### Purge out-of-date CSPObject in storage ######################### */
void purgeCSPObjects(struct SimpleRedBlackTree* objectStorage)
{
   struct CSPObject* cspObject;
   struct CSPObject* nextCSPObject;

   cspObject = (struct CSPObject*)simpleRedBlackTreeGetFirst(objectStorage);
   while(cspObject != NULL) {
      nextCSPObject = (struct CSPObject*)simpleRedBlackTreeGetNext(objectStorage, &cspObject->Node);
      if(cspObject->LastReportTimeStamp + (10 * cspObject->ReportInterval) < getMicroTime()) {
         CHECK(simpleRedBlackTreeRemove(objectStorage, &cspObject->Node) == &cspObject->Node);
         free(cspObject->AssociationArray);
         free(cspObject);
      }
      cspObject = nextCSPObject;
   }
}


/* ###### Handle incoming CSP message #################################### */
static void handleMessage(int sd, struct SimpleRedBlackTree* objectStorage)
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
                  simpleRedBlackTreeNodeNew(&cspObject->Node);
                  cspObject->Identifier       = cspReport->Header.SenderID;
                  cspObject->AssociationArray = NULL;
               }
            }
            if(cspObject) {
               cspObject->LastReportTimeStamp = getMicroTime();
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
               cspObject->Location[sizeof(cspObject->Location) - 1] = 0x00;
               if(cspObject->AssociationArray) {
                  deleteComponentAssociationArray(cspObject->AssociationArray);
               }
               cspObject->AssociationArray = createComponentAssociationArray(cspReport->Associations);
               CHECK(cspObject->AssociationArray);
               memcpy(cspObject->AssociationArray, &cspReport->AssociationArray, cspReport->Associations * sizeof(struct ComponentAssociation));
               cspObject->Associations = cspReport->Associations;
               CHECK(simpleRedBlackTreeInsert(objectStorage,
                                                  &cspObject->Node) == &cspObject->Node);
            }
         }
      }
   }
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct SimpleRedBlackTree objectStorage;
   union sockaddr_union      localAddress;
   struct pollfd             ufds;
   unsigned long long        lastUpdate;
   int                       result;
   int                       reuse;
   int                       sd;
   int                       n;

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
      else if(!(strncmp(argv[n], "-log" ,4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
      else {
         printf("Bad argument \"%s\"!\n" ,argv[n]);
         exit(1);
      }
   }

   sd = ext_socket(localAddress.sa.sa_family, SOCK_DGRAM, IPPROTO_UDP);
   if(sd < 0) {
      perror("Unable to create socket");
      exit(1);
   }
   reuse = 1;
   if(ext_setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
      perror("setsockopt() with SO_REUSEADDR failed");
   }
   if(bindplus(sd, &localAddress, 1) == false) {
      fputs("ERROR: Unable to bind socket to local address\n", stderr);
      exit(1);
   }

   simpleRedBlackTreeNew(&objectStorage,
                             cspObjectPrint,
                             cspObjectComparison);


   puts("Component Status Monitor - Version 1.0");
   puts("======================================\n");

   installBreakDetector();
   beginLogging();

   while(!breakDetected()) {
      ufds.fd     = sd;
      ufds.events = POLLIN;
      lastUpdate = getMicroTime();
      while(getMicroTime() - lastUpdate < 350000) {
         result = ext_poll(&ufds, 1, 175);
         if((result > 0) && (ufds.revents & POLLIN)) {
            handleMessage(sd, &objectStorage);
         }
      }
      purgeCSPObjects(&objectStorage);

      //printf("\x1b[;H\x1b[2J");
      printf("\x1b[;H");
      printTimeStamp(stdout);
      puts("Current Component Status\x1b[0K\n\x1b[0K");
      simpleRedBlackTreePrint(&objectStorage, stdout);
      printf("\x1b[0J");
      fflush(stdout);
   }

   ext_close(sd);
   simpleRedBlackTreeDelete(&objectStorage);
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
