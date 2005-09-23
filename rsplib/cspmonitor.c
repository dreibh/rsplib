/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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
#include "componentstatusprotocol.h"
#include "leaflinkedredblacktree.h"

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
      case CID_GROUP_NAMESERVER:
         snprintf(buffer, bufferSize,
                  "Registrar  $%08Lx",
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
   struct LeafLinkedRedBlackTreeNode Node;
   uint64_t                          ID;
   uint64_t                          LastReportTimeStamp;
   uint64_t                          ReportInterval;
   char                              Description[128];
   char                              StatusText[128];
   char                              ComponentAddress[128];
   size_t                            Associations;
   struct ComponentAssociationEntry* AssociationArray;
};


/* ###### CSPObject print function ####################################### */
static void cspObjectPrint(const void* cspObjectPtr, FILE* fd)
{
   const struct CSPObject* cspObject = (const struct CSPObject*)cspObjectPtr;
   char                    str[256];
   size_t                  i;

   fprintf(fd,"\x1b[%um", 30 + (unsigned int)CID_GROUP(cspObject->ID) % 8);
   fprintf(fd, "%s [%s]: lr=%5ums, int=%4Ldms, A=%u %s\n",
           cspObject->Description,
           cspObject->ComponentAddress,
           abs(((int64_t)cspObject->LastReportTimeStamp - (int64_t)getMicroTime()) / 1000),
           cspObject->ReportInterval / 1000,
           (unsigned int)cspObject->Associations,
           cspObject->StatusText);
   for(i = 0;i < cspObject->Associations;i++) {
      getDescriptionForID(cspObject->AssociationArray[i].ReceiverID,
                          (char*)&str, sizeof(str));
      fprintf(fd,"   -> %s proto=%4u ppid=$%08x",
              str,
              cspObject->AssociationArray[i].ProtocolID,
              cspObject->AssociationArray[i].PPID);
      if(cspObject->AssociationArray[i].Duration != ~0ULL) {
         fprintf(fd, "  duration=%4llu.%03llus",
                 cspObject->AssociationArray[i].Duration / 1000000,
                 (cspObject->AssociationArray[i].Duration % 1000000) / 1000);
      }
      fputs("\n", fd);
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


struct CSPObject* findCSPObject(struct LeafLinkedRedBlackTree* objectStorage,
                                const uint64_t                 id)
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


void purgeCSPObjects(struct LeafLinkedRedBlackTree* objectStorage)
{
   struct CSPObject* cspObject;
   struct CSPObject* nextCSPObject;

   cspObject = (struct CSPObject*)leafLinkedRedBlackTreeGetFirst(objectStorage);
   while(cspObject != NULL) {
      nextCSPObject = (struct CSPObject*)leafLinkedRedBlackTreeGetNext(objectStorage, &cspObject->Node);
      if(cspObject->LastReportTimeStamp + (10 * cspObject->ReportInterval) < getMicroTime()) {
         CHECK(leafLinkedRedBlackTreeRemove(objectStorage, &cspObject->Node) == &cspObject->Node);
         free(cspObject->AssociationArray);
         free(cspObject);
      }
      cspObject = nextCSPObject;
   }
}


static void handleMessage(int sd, struct LeafLinkedRedBlackTree* objectStorage)
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
                  cspObject->ID               = csph->SenderID;
                  cspObject->AssociationArray = NULL;
               }
            }
            if(cspObject) {
               cspObject->LastReportTimeStamp = getMicroTime();
               cspObject->ReportInterval      = csph->ReportInterval;
               getDescriptionForID(cspObject->ID,
                                   (char*)&cspObject->Description,
                                   sizeof(cspObject->Description));
               memcpy(&cspObject->StatusText,
                        &csph->StatusText,
                        sizeof(cspObject->StatusText));
               cspObject->StatusText[sizeof(cspObject->StatusText) - 1] = 0x00;
               memcpy(&cspObject->ComponentAddress,
                        &csph->ComponentAddress,
                        sizeof(cspObject->ComponentAddress));
               cspObject->ComponentAddress[sizeof(cspObject->ComponentAddress) - 1] = 0x00;
               if(cspObject->AssociationArray) {
                  componentAssociationEntryArrayDelete(cspObject->AssociationArray);
               }
               cspObject->AssociationArray = componentAssociationEntryArrayNew(csph->Associations);
               CHECK(cspObject->AssociationArray);
               memcpy(cspObject->AssociationArray, &csph->AssociationArray, csph->Associations * sizeof(struct ComponentAssociationEntry));
               cspObject->Associations = csph->Associations;
               CHECK(leafLinkedRedBlackTreeInsert(objectStorage,
                                                  &cspObject->Node) == &cspObject->Node);
            }
         }
      }
   }
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct LeafLinkedRedBlackTree objectStorage;
   union sockaddr_union          localAddress;
   struct pollfd                 ufds;
   int                           result;
   int                           reuse;
   int                           sd;
   int                           n;

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

   leafLinkedRedBlackTreeNew(&objectStorage,
                             cspObjectPrint,
                             cspObjectComparison);


   puts("Component Status Monitor - Version 1.0");
   puts("======================================\n");

   installBreakDetector();
   beginLogging();

   while(!breakDetected()) {
      ufds.fd     = sd;
      ufds.events = POLLIN;
      result = ext_poll(&ufds, 1, 500);
      if((result > 0) && (ufds.revents & POLLIN)) {
         handleMessage(sd, &objectStorage);
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
