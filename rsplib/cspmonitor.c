/*
 *  $Id: cspmonitor.c,v 1.2 2004/09/16 16:24:43 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germlocal (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) local later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for local discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Example Pool Element
 *
 */

#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "breakdetector.h"
#include "componentstatusprotocol.h"
#include "leaflinkedredblacktree.h"

#include <ext_socket.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */


struct CSPObject
{
   struct LeafLinkedRedBlackTreeNode Node;
   uint64_t                          ID;
   uint64_t                          LastReportTimeStamp;
   uint64_t                          ReportInterval;
   char                              Description[128];
   char                              StatusText[128];
   size_t                            Associations;
   struct ComponentAssociationEntry* AssociationArray;
};


/* ###### CSPObject print function ####################################### */
static void cspObjectPrint(const void* cspObjectPtr, FILE* fd)
{
   const struct CSPObject* cspObject = (const struct CSPObject*)cspObjectPtr;
   size_t i;

   fprintf(fd,"\x1b[%um", 30 + (unsigned int)CID_GROUP(cspObject->ID) % 8);
   fprintf(fd, "%s: lr=%5ums, int=%4Ldms, A=%u, %s\n",
           cspObject->Description,
           abs(((int64_t)cspObject->LastReportTimeStamp - (int64_t)getMicroTime()) / 1000),
           cspObject->ReportInterval / 1000,
           cspObject->Associations,
           cspObject->StatusText);
   for(i = 0;i < cspObject->Associations;i++) {
      fprintf(fd,"   -> $%016Lx proto=%4u ppid=$%08x\n",
              cspObject->AssociationArray[i].ReceiverID,
              cspObject->AssociationArray[i].ProtocolID,
              cspObject->AssociationArray[i].PPID);
   }
   fputs("\x1b[0m\n", fd);
}


/* ###### CSPObject comparision function ################################# */
static int cspObjectComparison(const void* cspObjectPtr1, const void* cspObjectPtr2)
{
   const struct CSPObject* cspObject1 = (const struct CSPObject*)cspObjectPtr1;
   const struct CSPObject* cspObject2 = (const struct CSPObject*)cspObjectPtr2;
   if(cspObject1->ID < cspObject2->ID) {
      return(-1);
   }
   else if(cspObject1->ID > cspObject2->ID) {
      return(1);
   }
   return(0);
}


struct CSPObject* findCSPObject(LeafLinkedRedBlackTree* objectStorage,
                                const uint64_t          id)
{
   struct LeafLinkedRedBlackTreeNode* node;
   struct CSPObject                   cmpCSPObject;
   cmpCSPObject.ID = id;
   node = leafLinkedRedBlackTreeFind(objectStorage, &cmpCSPObject.Node);
   if(node) {
      return((struct CSPObject*)node);
   }
   return(NULL);
}


void purgeCSPObjects(LeafLinkedRedBlackTree* objectStorage)
{
   struct CSPObject* cspObject;
   struct CSPObject* nextCSPObject;

   cspObject = (struct CSPObject*)leafLinkedRedBlackTreeGetFirst(objectStorage);
   while(cspObject != NULL) {
      nextCSPObject = (struct CSPObject*)leafLinkedRedBlackTreeGetNext(objectStorage, &cspObject->Node);
      if(cspObject->LastReportTimeStamp + (3 * cspObject->ReportInterval) < getMicroTime()) {
         leafLinkedRedBlackTreeRemove(objectStorage, &cspObject->Node);
         free(cspObject);
      }
      cspObject = nextCSPObject;
   }
}



static void handleMessage(int sd, LeafLinkedRedBlackTree* objectStorage)
{
   struct ComponentStatusProtocolHeader* csph;
   struct CSPObject*                     cspObject;
   char                                  buffer[65536];
   ssize_t                               received;
   size_t                                i;

   received = ext_recv(sd, (char*)&buffer, sizeof(buffer), 0);
   if(received) {
      if(received >= (ssize_t)sizeof(struct ComponentStatusProtocolHeader)) {
         csph = (struct ComponentStatusProtocolHeader*)&buffer;
         csph->Type            = ntohs(csph->Type);
         csph->Version         = ntohs(csph->Version);
         csph->Length          = ntohl(csph->Length);
         csph->SenderID        = ntoh64(csph->SenderID);
         csph->ReportInterval  = ntoh64(csph->ReportInterval);
         csph->SenderTimeStamp = ntoh64(csph->SenderTimeStamp);
         csph->Associations    = ntohl(csph->Associations);
         if(sizeof(struct ComponentStatusProtocolHeader) + (csph->Associations * sizeof(struct ComponentAssociationEntry)) == (size_t)received) {
            for(i = 0;i < csph->Associations;i++) {
               csph->AssociationArray[i].ReceiverID = ntoh64(csph->AssociationArray[i].ReceiverID);
               csph->AssociationArray[i].Duration   = ntoh64(csph->AssociationArray[i].Duration);
               csph->AssociationArray[i].Flags      = ntohs(csph->AssociationArray[i].Flags);
               csph->AssociationArray[i].ProtocolID = ntohs(csph->AssociationArray[i].ProtocolID);
               csph->AssociationArray[i].PPID       = ntohl(csph->AssociationArray[i].PPID);
            }

            cspObject = findCSPObject(objectStorage, csph->SenderID);
            if(cspObject == NULL) {
               cspObject = (struct CSPObject*)malloc(sizeof(struct CSPObject));
               if(cspObject) {
                  leafLinkedRedBlackTreeNodeNew(&cspObject->Node);
                  cspObject->ID = csph->SenderID;
               }
            }
            if(cspObject) {
               cspObject->LastReportTimeStamp = getMicroTime();
               cspObject->ReportInterval      = csph->ReportInterval;
               switch(CID_GROUP(cspObject->ID)) {
                  case CID_GROUP_NAMESERVER:
                     snprintf((char*)&cspObject->Description,
                              sizeof(cspObject->Description),
                              "Name Server $%08Lx",
                              CID_OBJECT(cspObject->ID));
                   break;
                  case CID_GROUP_POOLELEMENT:
                     snprintf((char*)&cspObject->Description,
                              sizeof(cspObject->Description),
                              "Pool Element $%08Lx",
                              CID_OBJECT(cspObject->ID));
                   break;
                  case CID_GROUP_POOLUSER:
                     snprintf((char*)&cspObject->Description,
                              sizeof(cspObject->Description),
                              "Pool User $%08Lx",
                              CID_OBJECT(cspObject->ID));
                   break;
                  default:
                     snprintf((char*)&cspObject->Description,
                              sizeof(cspObject->Description),
                              "ID $%Lx",
                              CID_OBJECT(cspObject->ID));
                   break;
               }
               memcpy(&cspObject->StatusText,
                        &csph->StatusText,
                        sizeof(cspObject->StatusText));
               cspObject->StatusText[sizeof(cspObject->StatusText) - 1] = 0x00;
               if(cspObject->AssociationArray) {
                  componentAssociationEntryArrayDelete(cspObject->AssociationArray);
               }
               cspObject->AssociationArray = componentAssociationEntryArrayNew(csph->Associations);
               CHECK(cspObject->AssociationArray);
               memcpy(cspObject->AssociationArray, &csph->AssociationArray, csph->Associations * sizeof(struct ComponentAssociationEntry));
               cspObject->Associations = csph->Associations;
               leafLinkedRedBlackTreeInsert(objectStorage,
                                             &cspObject->Node);
            }
         }
      }
   }
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   union sockaddr_union   localAddress;
   int                    n;
   int                    result;
   int                    sd;
   fd_set                 readfdset;
   LeafLinkedRedBlackTree objectStorage;
   struct timeval         timeout;

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

   sd = ext_socket(localAddress.sa.sa_family,
                   SOCK_DGRAM,
                   IPPROTO_UDP);
   if(sd < 0) {
      perror("Unable to create socket");
      exit(1);
   }
   if(bindplus(sd, &localAddress, 1) == false) {
      fputs("ERROR: Unable to bind socket to local address\n", stderr);
      exit(1);
   }

   leafLinkedRedBlackTreeNew(&objectStorage,
                             cspObjectPrint,
                             cspObjectComparison);


   puts("Component Status Monitor - Version 1.0");
   puts("--------------------------------------\n");

   installBreakDetector();
   beginLogging();

   while(!breakDetected()) {
      FD_ZERO(&readfdset);
      FD_SET(sd, &readfdset);

      timeout.tv_sec  = 0;
      timeout.tv_usec = 500000;
      result = ext_select(sd + 1, &readfdset, NULL, NULL, &timeout);

      if(result > 0) {
         if(FD_ISSET(sd, &readfdset)) {
            handleMessage(sd, &objectStorage);
         }
      }
      purgeCSPObjects(&objectStorage);

      printf("\x1b[;H\x1b[2J");
      printTimeStamp(stdout);
      puts("Current Component Status\n");
      leafLinkedRedBlackTreePrint(&objectStorage, stdout);
   }

   ext_close(sd);
   leafLinkedRedBlackTreeDelete(&objectStorage);
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
