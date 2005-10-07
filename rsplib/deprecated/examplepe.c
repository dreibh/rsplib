/*
 *  $Id$
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium f�r Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (F�rderkennzeichen 01AK045).
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
#include "netutilities.h"
#include "breakdetector.h"
#include "rsplib.h"
#ifdef ENABLE_CSP
#include "componentstatusprotocol.h"
#endif

#include <ext_socket.h>
#include <pthread.h>
#include <glib.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */


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


/* ###### Send cookie #################################################### */
void sendCookie(struct Client* client)
{
   struct Cookie cookie;
   bool          result;

   strncpy((char*)&cookie.ID, MY_COOKIE_ID, sizeof(MY_COOKIE_ID));
   cookie.Counter = htonl(client->Counter);
   result = rspSessionSendCookie(client->Session,
                                 (unsigned char*)&cookie, sizeof(struct Cookie), NULL);
   if(result) {
      puts("Cookie sent");
   }
   else {
      puts("Sending Cookie failed!");
   }
}


/* ###### Handle cookie echo ############################################# */
static void handleCookieEcho(struct Client* client, void* cookieData, const size_t cookieSize)
{
   struct Cookie* cookie = (struct Cookie*)cookieData;

   if(client->IsNewClient) {
      if( (cookieSize >= sizeof(struct Cookie)) &&
          (!(strncmp(cookie->ID, MY_COOKIE_ID, sizeof(MY_COOKIE_ID)))) ) {
         /* Here should be *at least* checks for a Message Authentication Code
            (MAC), timestamp (out-of-date cookie, e.g. a replay attack) and
            validity and plausibility of the values set from the cookie! */
         client->CounterStart = ntohl(cookie->Counter) + 1;
         client->Counter = client->CounterStart;
         printf("Retrieved Counter from Cookie -> %d\n" , client->Counter);
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


/* ###### Handle service requests from clients ########################### */
static void handleServiceRequest(GList**        clientList,
                                 struct Client* client)
{
   struct TagItem tags[16];
   char           buffer[16385];
   ssize_t        received;

   tags[0].Tag  = TAG_RspIO_MsgIsCookieEcho;
   tags[0].Data = (tagdata_t)0;
   tags[1].Tag  = TAG_RspIO_Timeout;
   tags[1].Data = (tagdata_t)0;
   tags[2].Tag  = TAG_DONE;
   received = rspSessionRead(client->Session, (char*)&buffer, sizeof(buffer) - 1, (struct TagItem*)&tags);
   if(received > 0) {
      if(tags[0].Data == 0) {
         client->IsNewClient = false;
         buffer[received]  = 0x00;
         printf("Echo %u %u> %s\n", client->CounterStart,
                                    client->Counter,
                                    buffer);
         client->Counter++;
         rspSessionWrite(client->Session, (char*)&buffer, received, NULL);

         if((client->Counter % 3) == 0) {
            sendCookie(client);
         }
      }
      else {
         handleCookieEcho(client, (void*)&buffer, received);
      }
   }
   /*
   else if(received == RspRead_Timeout) {
      puts("INTERNAL ERROR: Timeout occurred! This should never happen!");
      exit(1);
   }
   */
   else {
      *clientList = g_list_remove(*clientList, (gpointer)client);
      rspDeleteSession(client->Session, NULL);
      free(client);

      printTimeStamp(stdout);
      printf("Removed client.\n");
   }
}


/* ###### rsplib main loop thread ######################################## */
static bool RsplibThreadStop = false;
static void* rsplibMainLoop(void* args)
{
   struct timeval timeout;
   while(!RsplibThreadStop) {
      timeout.tv_sec  = 0;
      timeout.tv_usec = 250000;
      rspSelect(0, NULL, NULL, NULL, &timeout);
   }
   return(NULL);
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   uint32_t                      identifier = 0;
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
#ifdef ENABLE_CSP
   struct CSPReporter            cspReporter;
   uint64_t                      cspIdentifier;
   unsigned int                  cspReportInterval = 0;
   union sockaddr_union          cspReportAddress;
#endif

   GList*                        clientList                     = NULL;
   unsigned long long            start                          = getMicroTime();
   unsigned long long            stop                           = 0;
   int                           type                           = SOCK_STREAM;
   int                           protocol                       = IPPROTO_SCTP;
   unsigned short                port                           = 0;
   char*                         poolHandle                     = "EchoPool";
   unsigned int                  policyType                     = PPT_ROUNDROBIN;
   unsigned int                  policyParameterWeight          = 1;
   unsigned int                  policyParameterLoad            = 0;
   unsigned int                  policyParameterLoadDegradation = 0;
   int                           result;
   int                           i;
   GList*                        list;

#ifdef ENABLE_CSP
   string2address("127.0.0.1:2960", &cspReportAddress);
#endif
   start = getMicroTime();
   stop  = 0;
   for(i = 1;i < argc;i++) {
      if(!(strcmp(argv[i], "-sctp"))) {
         protocol = IPPROTO_SCTP;
      }
      else if(!(strcmp(argv[i], "-tcp"))) {
         protocol = IPPROTO_TCP;
      }
      else if(!(strncmp(argv[i], "-stop=" ,6))) {
         stop = start + ((unsigned long long)1000000 * (unsigned long long)atol((char*)&argv[i][6]));
      }
      else if(!(strncmp(argv[i], "-port=" ,6))) {
         port = atol((char*)&argv[i][6]);
      }
      else if(!(strncmp(argv[i], "-ph=" ,4))) {
         poolHandle = (char*)&argv[i][4];
      }
      else if(!(strncmp(argv[i], "-identifier=", 12))) {
         identifier = atol((char*)&argv[i][12]);
      }
#ifdef ENABLE_CSP
      else if(!(strncmp(argv[i], "-cspreportinterval=", 19))) {
         cspReportInterval = atol((char*)&argv[i][19]);
      }
      else if(!(strncmp(argv[i], "-cspreportaddress=", 18))) {
         if(!string2address((char*)&argv[i][18], &cspReportAddress)) {
            fprintf(stderr,
                    "ERROR: Bad CSP report address %s! Use format <address:port>.\n",
                    (char*)&argv[i][18]);
            exit(1);
         }
         if(cspReportInterval <= 0) {
            cspReportInterval = 250000;
         }
      }
#else
      else if((!(strncmp(argv[i], "-cspreportinterval=", 19))) ||
              (!(strncmp(argv[i], "-cspreportaddress=", 18)))) {
         fprintf(stderr, "ERROR: CSP support not compiled in! Ignoring argument %s\n", argv[i]);
         exit(1);
      }
#endif
      else if(!(strncmp(argv[i], "-load=" ,6))) {
         policyParameterLoad = atol((char*)&argv[i][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-weight=" ,8))) {
         policyParameterWeight = atol((char*)&argv[i][8]);
         if(policyParameterWeight > 0xffffff) {
            policyParameterWeight = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-load=" ,6))) {
         policyParameterLoad = atol((char*)&argv[i][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-loaddeg=" ,9))) {
         policyParameterLoadDegradation = atol((char*)&argv[i][9]);
         if(policyParameterLoadDegradation > 0xffffff) {
            policyParameterLoadDegradation = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-weight=" ,8))) {
         policyParameterWeight = atol((char*)&argv[i][8]);
         if(policyParameterWeight < 1) {
            policyParameterWeight = 1;
         }
      }
      else if(!(strncmp(argv[i], "-policy=" ,8))) {
         if((!(strcmp((char*)&argv[i][8], "roundrobin"))) || (!(strcmp((char*)&argv[i][8], "rr")))) {
            policyType = PPT_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[i][8], "weightedroundrobin"))) || (!(strcmp((char*)&argv[i][8], "wrr")))) {
            policyType = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[i][8], "leastused"))) || (!(strcmp((char*)&argv[i][8], "lu")))) {
            policyType = PPT_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "leastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "lud")))) {
            policyType = PPT_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedleastused"))) || (!(strcmp((char*)&argv[i][8], "rlu")))) {
            policyType = PPT_RANDOMIZED_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "rlud")))) {
            policyType = PPT_RANDOMIZED_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "priorityleastused"))) || (!(strcmp((char*)&argv[i][8], "plu")))) {
            policyType = PPT_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "priorityleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "plud")))) {
            policyType = PPT_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedpriorityleastused"))) || (!(strcmp((char*)&argv[i][8], "rplu")))) {
            policyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedpriorityleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "rplud")))) {
            policyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "random"))) || (!(strcmp((char*)&argv[i][8], "rand")))) {
            policyType = PPT_RANDOM;
         }
         else if((!(strcmp((char*)&argv[i][8], "weightedrandom"))) || (!(strcmp((char*)&argv[i][8], "wrand")))) {
            policyType = PPT_WEIGHTED_RANDOM;
         }
         else {
            printf("ERROR: Unknown policy type \"%s\"!\n" , (char*)&argv[i][8]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-log" ,4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-registrar=" ,11))) {
         /* Process this later */
      }
      else {
         printf("Bad argument \"%s\"!\n" ,argv[i]);
         printf("Usage: %s {-registrar=Registrar address(es)} {-sctp|-tcp} {-port=local port} {-stop=seconds} {-ph=Pool Handle} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} {-policy=roundrobin|rr|weightedroundrobin|wee|leastused|lu|leastuseddegradation|lud|random|rd|weightedrandom|wrd} {-load=load} {-weight=weight}"
#ifdef ENABLE_CSP
                " {-cspreportaddress=Address} {-cspreportinterval=Microseconds} "
#endif
                "{-identifier=PE Identifier}\n" ,
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
#ifdef ENABLE_CSP
   cspIdentifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, identifier);
   if(cspReportInterval > 0) {
      cspReporterNewForRspLib(&cspReporter, cspIdentifier, &cspReportAddress, cspReportInterval);
   }
#endif

   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-registrar=" ,12))) {
         if(rspAddStaticRegistrar((char*)&argv[i][12]) != RSPERR_OKAY) {
            fprintf(stderr, "ERROR: Bad registrar setting: %s\n", argv[i]);
            exit(1);
         }
      }
   }

   if(pthread_create(&rsplibThread, NULL, &rsplibMainLoop, NULL) != 0) {
      puts("ERROR: Unable to create rsplib main loop thread!");
   }

   tags[0].Tag  = TAG_PoolPolicy_Type;
   tags[0].Data = policyType;
   tags[1].Tag  = TAG_PoolPolicy_Parameter_Load;
   tags[1].Data = policyParameterLoad;
   tags[2].Tag  = TAG_PoolPolicy_Parameter_LoadDegradation;
   tags[2].Data = policyParameterLoadDegradation;
   tags[3].Tag  = TAG_PoolPolicy_Parameter_Weight;
   tags[3].Data = policyParameterWeight;
   tags[4].Tag  = TAG_PoolElement_SocketType;
   tags[4].Data = type;
   tags[5].Tag  = TAG_PoolElement_SocketProtocol;
   tags[5].Data = protocol;
   tags[6].Tag  = TAG_PoolElement_LocalPort;
   tags[6].Data = port;
   tags[7].Tag  = TAG_PoolElement_ReregistrationInterval;
   tags[7].Data = 45000;
   tags[8].Tag  = TAG_PoolElement_RegistrationLife;
   tags[8].Data = (3 * 45000) + 5000;
   tags[9].Tag  = TAG_PoolElement_Identifier;
   tags[9].Data = identifier;
   tags[10].Tag  = TAG_UserTransport_HasControlChannel;
   tags[10].Data = (tagdata_t)true;
   tags[11].Tag  = TAG_END;

   poolElement = rspCreatePoolElement((unsigned char*)poolHandle, strlen(poolHandle), tags);
   if(poolElement != NULL) {
      puts("Session-Based Example Pool Element - Version 2.0");
      puts("------------------------------------------------\n");

#ifndef FAST_BREAK
      installBreakDetector();
#endif

      while(!breakDetected()) {
         /* ====== Collect sources for rspSessionSelect() call =========== */
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

         /* ====== Call rspSessionSelect() =============================== */
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
                                   250000,
                                   (struct TagItem*)&tags);

         /* ====== Handle results of ext_select() ======================== */
         if(result > 0) {
            if(pedStatusArray[0] & RspSelect_Read) {
               client = (struct Client*)malloc(sizeof(struct Client));
               if(client) {
                  client->Session      = NULL;
                  client->Counter      = 0;
                  client->CounterStart = 0;
                  client->IsNewClient    = true;


/*
                  tags[0].Tag  = TAG_TuneSCTP_MinRTO;
                  tags[0].Data = 100;
                  tags[1].Tag  = TAG_TuneSCTP_MaxRTO;
                  tags[1].Data = 500;
                  tags[2].Tag  = TAG_TuneSCTP_InitialRTO;
                  tags[2].Data = 250;
                  tags[3].Tag  = TAG_TuneSCTP_Heartbeat;
                  tags[3].Data = 100;
                  tags[4].Tag  = TAG_TuneSCTP_PathMaxRXT;
                  tags[4].Data = 3;
                  tags[5].Tag  = TAG_TuneSCTP_AssocMaxRXT;
                  tags[5].Data = 12;
                  tags[6].Tag  = TAG_DONE;
*/
                  tags[0].Tag  = TAG_DONE;


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
            for(i = 0;i < (int)sessions;i++) {
               if(sessionStatusArray[i] & RspSelect_Read) {
                  handleServiceRequest(&clientList, clientArray[i]);
               }
            }
            list = g_list_first(clientList);
         }
         if((result < 0) && (errno != EINTR)) {
	    /* FIXME
	    perror("rspSessionSelect() failed");
            */
            break;
         }

         /* ====== Check auto-stop timer ================================= */
         if((stop > 0) && (getMicroTime() >= stop)) {
            puts("Auto-stop time reached -> exiting!");
            break;
         }
      }

      puts("Shutdown...");

      /* ====== Delete sessions and finally pool element ================= */
      list = g_list_first(clientList);
      while(list != NULL) {
         client = (struct Client*)list->data;
         rspDeleteSession(client->Session, NULL);
         clientList = g_list_remove(clientList, (gpointer)client);
         free(client);
         list = g_list_first(clientList);
      }
      rspDeletePoolElement(poolElement, NULL);
   }
   else {
      puts("ERROR: Unable to create pool element!");
      exit(1);
   }

   RsplibThreadStop = true;
   pthread_join(rsplibThread, NULL);
#ifdef ENABLE_CSP
   if(cspReportInterval > 0) {
      cspReporterDelete(&cspReporter);
   }
#endif
   rspCleanUp();
   finishLogging();
   puts("\nTerminated!");
   return(0);
}