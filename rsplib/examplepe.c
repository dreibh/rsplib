/*
 *  $Id: examplepe.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Example Pool Element
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "utilities.h"
#include "netutilities.h"
#include "localaddresses.h"
#include "breakdetector.h"
#include "rsplib.h"

#include <ext_socket.h>
#include <pthread.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
#define FAST_BREAK


struct Client
{
   struct SessionDescriptor* Session;
   uint32_t                  CounterStart;
   uint32_t                  Counter;
   bool                      IsNewClient;
};

#define MY_COOKIE_ID "<COOKIE>"

struct Cookie
{
   char     ID[8];
   uint32_t Counter;
};


/* ###### Send cookie ####################################################### */
void sendCookie(struct Client* client)
{
   struct Cookie cookie;
   bool          result;

   strncpy((char*)&cookie.ID, MY_COOKIE_ID, sizeof(MY_COOKIE_ID));
   cookie.Counter = htonl(client->Counter);
   result = rspSessionSendCookie(client->Session,
                                 (char*)&cookie, sizeof(struct Cookie), NULL);
   if(result) {
      puts("Cookie sent");
   }
   else {
      puts("Sending Cookie failed!");
   }
}


static void handleCookieEcho(void* userData, void* cookieData, const size_t cookieSize)
{
   struct Client* client = (struct Client*)userData;
   struct Cookie* cookie = (struct Cookie*)cookieData;

   if(client->IsNewClient) {
      if( (cookieSize >= sizeof(struct Cookie)) &&
          (!(strncmp(cookie->ID, MY_COOKIE_ID, sizeof(MY_COOKIE_ID)))) ) {
         /* Here should be *at least* checks for a Message Authentication Code
            (MAC), timestamp (out-of-date cookie, e.g. a replay attack) and
            validity and plausibility of the values set from the cookie! */
         client->CounterStart = ntohl(cookie->Counter);
         client->Counter = client->CounterStart;
         printf("Retrieved Counter from Cookie -> %d\n", client->Counter);
      }
      else {
         puts("Cookie is invalid!");
      }
      client->IsNewClient = false;
   }
   else {
      puts("CookieEcho is only allowed before first transaction!");
   }
}


/* ###### Handle service requests from clients ############################## */
static void handleServiceRequest(GList**        clientList,
                                 struct Client* client)
{
   struct TagItem tags[16];
   char           buffer[16385];
   ssize_t        received;

   tags[0].Tag  = TAG_RspIO_Timeout;
   tags[0].Data = (tagdata_t)0;
   tags[1].Tag  = TAG_DONE;
   received = rspSessionRead(client->Session, (char*)&buffer, sizeof(buffer) - 1, (struct TagItem*)&tags);
   if(received > 0) {
      client->IsNewClient = false;
      buffer[received]  = 0x00;
      printf("Echo %u %u> %s", client->CounterStart,
                               client->Counter,
                               buffer);
      client->Counter++;
      rspSessionWrite(client->Session, (char*)&buffer, received, NULL);

      if((client->Counter % 3) == 0) {
         sendCookie(client);
      }
   }
   else {
      *clientList = g_list_remove(*clientList, (gpointer)client);
      rspDeleteSession(client->Session, NULL);
      free(client);

      printTimeStamp(stdout);
      printf("Removed client.\n");
   }
}


/* ###### rsplib main loop thread ########################################### */
static bool RsplibThreadStop = false;
static void* rsplibMainLoop(void* args)
{
   struct timeval timeout;
   while(!RsplibThreadStop) {
      timeout.tv_sec  = 0;
      timeout.tv_usec = 1000000;
      rspSelect(0, NULL, NULL, NULL, &timeout);
   }
   return(NULL);
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct PoolElementDescriptor* poolElement;
   struct TagItem                tags[16];
   pthread_t                     rsplibThread;
   struct SessionDescriptor*     newSession;
   struct Client*                client;
   struct Client*                clientArray[FD_SETSIZE];
   struct SessionDescriptor*     sessionArray[FD_SETSIZE];
   unsigned int                  sessionStatusArray[FD_SETSIZE];
   size_t                        sessions;
   struct PoolElementDescriptor* pedArray[1];
   unsigned int                  pedStatusArray[FD_SETSIZE];

   GList*         clientList = NULL;
   card64         start    = getMicroTime();
   card64         stop     = 0;
   int            type     = SOCK_STREAM;
   int            protocol = IPPROTO_SCTP;
   unsigned short port     = 0;
   char*          poolHandle            = "EchoPool";
   unsigned int   policyType            = TAGDATA_PoolPolicy_Type_RoundRobin;
   unsigned int   policyParameterWeight = 1;
   unsigned int   policyParameterLoad   = 0;
   int            n;
   int            result;
   size_t         i;
   GList*         list;


   start = getMicroTime();
   stop  = 0;
   for(n = 1;n < argc;n++) {
      if(!(strcmp(argv[n],"-sctp"))) {
         protocol = IPPROTO_SCTP;
      }
      else if(!(strcmp(argv[n],"-tcp"))) {
         protocol = IPPROTO_TCP;
      }
      else if(!(strncmp(argv[n],"-stop=",6))) {
         stop = start + ((card64)1000000 * (card64)atol((char*)&argv[n][6]));
      }
      else if(!(strncmp(argv[n],"-port=",6))) {
         port = atol((char*)&argv[n][6]);
      }
      else if(!(strncmp(argv[n],"-ph=",4))) {
         poolHandle = (char*)&argv[n][4];
      }
      else if(!(strncmp(argv[n],"-load=",6))) {
         policyParameterLoad = atol((char*)&argv[n][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n],"-weight=",8))) {
         policyParameterWeight = atol((char*)&argv[n][8]);
         if(policyParameterWeight > 0xffffff) {
            policyParameterWeight = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n],"-policy=",8))) {
         if((!(strcmp((char*)&argv[n][8],"roundrobin"))) || (!(strcmp((char*)&argv[n][8],"rr")))) {
            policyType = TAGDATA_PoolPolicy_Type_RoundRobin;
         }
         else if((!(strcmp((char*)&argv[n][8],"weightedroundrobin"))) || (!(strcmp((char*)&argv[n][8],"wrr")))) {
            policyType = TAGDATA_PoolPolicy_Type_WeightedRoundRobin;
         }
         else if((!(strcmp((char*)&argv[n][8],"leastused"))) || (!(strcmp((char*)&argv[n][8],"lu")))) {
            policyType = TAGDATA_PoolPolicy_Type_LeastUsed;
         }
         else if((!(strcmp((char*)&argv[n][8],"leastuseddegradation"))) || (!(strcmp((char*)&argv[n][8],"lud")))) {
            policyType = TAGDATA_PoolPolicy_Type_LeastUsedDegradation;
         }
         else if((!(strcmp((char*)&argv[n][8],"random"))) || (!(strcmp((char*)&argv[n][8],"rd")))) {
            policyType = TAGDATA_PoolPolicy_Type_Random;
         }
         else if((!(strcmp((char*)&argv[n][8],"weightedrandom"))) || (!(strcmp((char*)&argv[n][8],"wrd")))) {
            policyType = TAGDATA_PoolPolicy_Type_WeightedRandom;
         }
         else {
            printf("ERROR: Unknown policy type \"%s\"!\n",(char*)&argv[n][8]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[n],"-log",4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
      else {
         printf("Bad argument \"%s\"!\n",argv[n]);
         printf("Usage: %s {-sctp|-tcp} {-port=local port} {-stop=seconds} {-ph=Pool Handle} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} {-policy=roundrobin|rr|weightedroundrobin|wee|leastused|lu|leastuseddegradation|lud|random|rd|weightedrandom|wrd} {-load=load} {-weight=weight} \n",
                argv[0]);
         exit(1);
      }
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
      finishLogging();
      exit(1);
   }

   if(pthread_create(&rsplibThread, NULL, &rsplibMainLoop, NULL) != 0) {
      puts("ERROR: Unable to create rsplib main loop thread!");
   }

   tags[0].Tag  = TAG_PoolPolicy_Type;
   tags[0].Data = policyType;
   tags[1].Tag  = TAG_PoolPolicy_Parameter_Load;
   tags[1].Data = policyParameterLoad;
   tags[2].Tag  = TAG_PoolPolicy_Parameter_Weight;
   tags[2].Data = policyParameterWeight;
   tags[3].Tag  = TAG_PoolElement_SocketType;
   tags[3].Data = type;
   tags[4].Tag  = TAG_PoolElement_SocketProtocol;
   tags[4].Data = protocol;
   tags[5].Tag  = TAG_PoolElement_LocalPort;
   tags[5].Data = port;
   tags[6].Tag  = TAG_PoolElement_ReregistrationInterval;
   tags[6].Data = 45000;
   tags[7].Tag  = TAG_PoolElement_RegistrationLife;
   tags[7].Data = (3 * 45000) + 5000;
   tags[8].Tag  = TAG_UserTransport_HasControlChannel;
   tags[8].Data = (tagdata_t)true;
   tags[9].Tag  = TAG_END;

   poolElement = rspCreatePoolElement(poolHandle, strlen(poolHandle), tags);
   if(poolElement != NULL) {
      puts("Session-Based Example Pool Element - Version 2.0");
      puts("------------------------------------------------\n");

#ifndef FAST_BREAK
      installBreakDetector();
#endif

      while(!breakDetected()) {
         /* ====== Collect sources for rspSessionSelect() call ============== */
         sessions = 0;
         list = g_list_first(clientList);
         while(list != NULL) {
            clientArray[sessions]        = (struct Client*)list->data;
            sessionArray[sessions]       = clientArray[sessions]->Session;
            sessionStatusArray[sessions] = RspSelect_Read;
            sessions++;
            list = g_list_next(list);
         }
         pedArray[0]       = poolElement;
         pedStatusArray[0] = RspSelect_Read;

         /* ====== Call rspSessionSelect() ================================== */
         /* Wait for activity. Note: rspSessionSelect() is called in the
            background thread. Therefore, it is *not* necessary here! */
         tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
         tags[0].Data = 0;
         tags[1].Tag  = TAG_DONE;
         result = rspSessionSelect((struct SessionDescriptor**)sessionArray,
                                   (unsigned int*)&sessionStatusArray,
                                   sessions,
                                   (struct PoolElementDescriptor**)&pedArray,
                                   (unsigned int*)&pedStatusArray,
                                   1,
                                   500000,
                                   (struct TagItem*)&tags);

         /* ====== Handle results of ext_select() =========================== */
         if(result > 0) {
            if(pedStatusArray[0] & RspSelect_Read) {
               client = (struct Client*)malloc(sizeof(struct Client));
               if(client) {
                  client->Session      = NULL;
                  client->Counter      = 0;
                  client->CounterStart = 0;
                  client->IsNewClient    = true;

                  tags[0].Tag  = TAG_TuneSCTP_MinRTO;
                  tags[0].Data = 1000;
                  tags[1].Tag  = TAG_TuneSCTP_MaxRTO;
                  tags[1].Data = 2000;
                  tags[2].Tag  = TAG_TuneSCTP_InitialRTO;
                  tags[2].Data = 500;
                  tags[3].Tag  = TAG_TuneSCTP_Heartbeat;
                  tags[3].Data = 100;
                  tags[4].Tag  = TAG_TuneSCTP_PathMaxRXT;
                  tags[4].Data = 1;
                  tags[5].Tag  = TAG_TuneSCTP_AssocMaxRXT;
                  tags[5].Data = 2;
                  tags[6].Tag  = TAG_RspSession_ReceivedCookieEchoCallback;
                  tags[6].Data = (tagdata_t)handleCookieEcho;
                  tags[7].Tag  = TAG_RspSession_ReceivedCookieEchoUserData;
                  tags[7].Data = (tagdata_t)client;
                  tags[8].Tag  = TAG_DONE;
                  newSession = rspAcceptSession(pedArray[0], (struct TagItem*)&tags);
                  if(newSession) {
                     client->Session = newSession;
                     clientList = g_list_append(clientList, (gpointer)client);
                     printTimeStamp(stdout);
                     printf("Added client.\n");
                  }
                  else {
                     puts("ERROR: Unable to create new client!");
                     rspDeleteSession(newSession, NULL);
                     free(client);
                  }
               }
            }
            for(i = 0;i < sessions;i++) {
               if(sessionStatusArray[i] & RspSelect_Read) {
                  handleServiceRequest(&clientList, clientArray[i]);
               }
            }
            list = g_list_first(clientList);
         }
         if(result < 0) {
            perror("rspSessionSelect() failed");
            break;
         }

         /* ====== Check auto-stop timer ==================================== */
         if((stop > 0) && (getMicroTime() >= stop)) {
            puts("Auto-stop time reached -> exiting!");
            break;
         }
      }

      rspDeletePoolElement(poolElement, NULL);
   }
   else {
      puts("ERROR: Unable to create pool element!");
      exit(1);
   }

   RsplibThreadStop = true;
   pthread_join(rsplibThread, NULL);
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
