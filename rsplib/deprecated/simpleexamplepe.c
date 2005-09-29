/*
 *  $Id$
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
#include "netutilities.h"
#include "breakdetector.h"
#include "rserpool.h"

#include <ext_socket.h>
#include <glib.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */

/* Disable RSerPool support */
/* #define NO_RSP */



static int           ListenSocketFamily    = AF_INET6;
static int           ListenSocketType      = SOCK_STREAM;
static int           ListenSocketProtocol  = IPPROTO_SCTP;
static uint16_t      ListenPort            = 0;
static int           ListenSocket          = -1;
static GList*        ClientList            = NULL;


#ifndef NO_RSP
static char*         PoolName                       = NULL;
static uint32_t      PoolElementID                  = 0x00000000;
static unsigned int  PolicyType                     = PPT_ROUNDROBIN;
static uint32_t      PolicyParameterWeight          = 1;
static uint32_t      PolicyParameterLoad            = 0;
static uint32_t      PolicyParameterLoadDegradation = 0;
static unsigned long RegistrationInterval           = 5000000;
static pthread_t     RegistrationThread             = 0;
static bool          RegistrationThreadStop         = false;


/* ###### Registration/Reregistration ####################################### */
static void doRegistration()
{
   struct TagItem            tags[16];
   struct rserpool_addrinfo* eai;
   struct rserpool_addrinfo* eai2;
   struct rserpool_addrinfo* next;
   struct sockaddr*          sctpLocalAddressArray = NULL;
   union sockaddr_union*     localAddressArray     = NULL;
   union sockaddr_union      socketName;
   size_t                    socketNameLen;
   unsigned int              localAddresses;
   unsigned int              i;
   int                       result;

   /* ====== Create rserpool_addrinfo structure =========================== */
   eai = (struct rserpool_addrinfo*)malloc(sizeof(struct rserpool_addrinfo));
   if(eai == NULL) {
      puts("ERROR: Out of memory!");
      return;
   }

   eai->ai_family   = ListenSocketFamily;
   eai->ai_socktype = ListenSocketType;
   eai->ai_protocol = ListenSocketProtocol;
   eai->ai_next     = NULL;
   eai->ai_addr     = NULL;
   eai->ai_addrs    = 0;
   eai->ai_addrlen  = sizeof(union sockaddr_union);
   eai->ai_pe_id    = PoolElementID;

   /* ====== Get local addresses for SCTP socket ============================ */
   if(ListenSocketProtocol == IPPROTO_SCTP) {
      eai->ai_addrs = sctp_getladdrs(ListenSocket, 0, &eai->ai_addr);
      if(eai->ai_addrs <= 0) {
         puts("ERROR: Unable to obtain socket's local addresses!");
      }
      else {
         sctpLocalAddressArray = eai->ai_addr;

         /* --- Check for buggy sctplib ----- */
         if( (getPort((struct sockaddr*)eai->ai_addr) == 0) ||
             ( (((struct sockaddr*)eai->ai_addr)->sa_family == AF_INET) &&
               (((struct sockaddr_in*)eai->ai_addr)->sin_addr.s_addr == 0) ) ) {
            puts("WARNING: getladdrsplus() replies INADDR_ANY or port 0 -> Update sctplib!");
            free(eai->ai_addr);
            eai->ai_addr   = NULL;
            eai->ai_addrs  = 0;
         }
         /* --------------------------------- */
      }
   }


   /* ====== Get local addresses for TCP/UDP socket ========================= */
   if(eai->ai_addrs <= 0) {
      /* Get addresses of all local interfaces */
      localAddresses = gatherLocalAddresses(&localAddressArray);
      if(localAddresses == 0) {
         puts("ERROR: Unable to find out local addresses -> No registration possible!");
         return;
      }

      /* Get local port number */
      socketNameLen = sizeof(socketName);
      if(ext_getsockname(ListenSocket, (struct sockaddr*)&socketName, (socklen_t *)&socketNameLen) == 0) {
         for(i = 0;i < localAddresses;i++) {
            setPort((struct sockaddr*)&localAddressArray[i], getPort((struct sockaddr*)&socketName));
         }
      }
      else {
         perror("Unable to find out local port");
         free(localAddressArray);
         free(eai);
         return;
      }

      /* Add addresses to rserpool_addrinfo structure */
      if(ListenSocketProtocol == IPPROTO_SCTP) {
         /* SCTP endpoints are described by a list of their
            transport addresses. */
         eai->ai_addrs = localAddresses;
         eai->ai_addr  = localAddressArray;
      }
      else {
         /* TCP/UDP endpoints are described by an own rserpool_addrinfo
            for each transport address. */
         eai2 = eai;
         for(i = 0;i < localAddresses;i++) {
            eai2->ai_addrs = 1;
            eai2->ai_addr  = &localAddressArray[i];
            if(i < localAddresses - 1) {
               next = (struct rserpool_addrinfo*)malloc(sizeof(struct rserpool_addrinfo));
               if(next == NULL) {
                  puts("ERROR: Out of memory!");
                  free(localAddressArray);
                  while(eai != NULL) {
                     next = eai->ai_next;
                     free(eai);
                     eai = next;
                  }
                  return;
               }
               *next = *eai2;
               next->ai_next = NULL;
               eai2->ai_next    = next;
               eai2             = next;
            }
         }
      }
   }


   /* ====== Set policy type and parameters ================================= */
   tags[0].Tag  = TAG_PoolPolicy_Type;
   tags[0].Data = PolicyType;
   tags[1].Tag  = TAG_PoolPolicy_Parameter_Load;
   tags[1].Data = PolicyParameterLoad;
   tags[2].Tag  = TAG_PoolPolicy_Parameter_LoadDegradation;
   tags[2].Data = PolicyParameterLoadDegradation;
   tags[3].Tag  = TAG_PoolPolicy_Parameter_Weight;
   tags[3].Data = PolicyParameterWeight;
   tags[4].Tag  = TAG_END;


   /* ====== Do registration ================================================ */
   result = rsp_pe_registration((unsigned char*)PoolName, strlen(PoolName),
                                eai, (struct TagItem*)&tags);
   if(result == 0) {
      PoolElementID = eai->ai_pe_id;
      printf("(Re-)Registration successful, ID is $%08x\n", PoolElementID);
   }
   else {
      perror("(Re-)Registration failed");
   }


   /* ====== Clean up ======================================================= */
   if(sctpLocalAddressArray) {
      sctp_freeladdrs(sctpLocalAddressArray);
   }
   if(localAddressArray) {
      free(localAddressArray);
   }
   while(eai != NULL) {
      next = eai->ai_next;
      free(eai);
      eai = next;
   }
}


/* ###### Deregistration #################################################### */
static void doDeregistration()
{
   if(PoolElementID) {
      rsp_pe_deregistration((unsigned char*)PoolName, strlen(PoolName), PoolElementID, NULL);
   }
}

/* ###### Registration/Reregistration thread ################################ */
static void* registrationMainLoop(void* args)
{
   struct timeval timeout;
   unsigned long long         start;
   unsigned long long         now;

   while(!RegistrationThreadStop) {
      /* ====== Do registration/reregistration ============================== */
      doRegistration();
      usleep(1000000);
   }

   /* ====== Do deregistration ============================================== */
   doDeregistration();
   return(NULL);
}
#endif


/* ###### Clean-up ####################################################### */
static void cleanUp()
{
   GList* list;
   int    fd;

#ifndef NO_RSP
   if(RegistrationThread) {
      RegistrationThreadStop = true;
      pthread_join(RegistrationThread, NULL);
   }
#endif

   list = g_list_first(ClientList);
   while(list != NULL) {
      fd = (int)list->data;
      ClientList = g_list_remove(ClientList, (gpointer)fd);
      ext_close(fd);
      printTimeStamp(stdout);
      printf("Removed client #%d.\n",fd);
      list = g_list_first(ClientList);
   }

   if(ListenSocket >= 0) {
      ext_close(ListenSocket);
      ListenSocket = -1;
   }

#ifndef NO_RSP
   rsp_cleanup();
#endif
}


/* ###### Initialize ##################################################### */
static void initAll()
{
   union sockaddr_union localAddress;

#ifndef FAST_BREAK
   installBreakDetector();
#endif

   ListenSocket = ext_socket(ListenSocketFamily,
                             ListenSocketType,
                             ListenSocketProtocol);
   if(ListenSocket < 0) {
      perror("Unable to create application socket");
      cleanUp();
      exit(1);
   }
   setNonBlocking(ListenSocket);


   memset((void*)&localAddress, 0, sizeof(localAddress));
   ((struct sockaddr*)&localAddress)->sa_family = ListenSocketFamily;
   setPort((struct sockaddr*)&localAddress,ListenPort);

   if(bindplus(ListenSocket, (union sockaddr_union*)&localAddress, 1) == false) {
      perror("Unable to bind application socket");
      cleanUp();
      exit(1);
   }

   if(ListenSocketType != SOCK_DGRAM) {
      if(ext_listen(ListenSocket, 5) < 0) {
         perror("Unable to set application socket to listen mode");
         cleanUp();
         exit(1);
      }
   }

#ifndef NO_RSP
   if(rsp_initialize(NULL, NULL) < 0) {
      logerror("Unable to initialize rsplib!");
      cleanUp();
      exit(1);
   }

   doRegistration();
   if(pthread_create(&RegistrationThread,
                     NULL, &registrationMainLoop, NULL) != 0) {
      puts("ERROR: Unable to create RSerPool registration thread!");
      cleanUp();
      exit(1);
   }
#endif
}


/* ###### Handle socket events ########################################### */
static void socketHandler(int fd)
{
   char                    buffer[16385];
   union sockaddr_union    address;
   socklen_t               addressSize;
   ssize_t                 received;
   int                     sd;
   int                     flags;

   if(ListenSocketType != SOCK_DGRAM) {
      if(fd == ListenSocket) {
         sd = ext_accept(ListenSocket,NULL, 0);
         if(sd >= 0) {
            ClientList = g_list_append(ClientList, (gpointer)sd);
            setNonBlocking(sd);

            printTimeStamp(stdout);
            printf("Added client #%d.\n", sd);
         }
      }
      else {
         if(g_list_find(ClientList, (gpointer)fd) != NULL) {
            flags    = 0;
            received = recvfromplus(fd, (char*)&buffer, sizeof(buffer) - 1, &flags,
                                    NULL, 0,
                                    NULL, NULL, NULL,
                                    0);
            do {
               if(received > 0) {

                  buffer[received] = 0x00;
                  printf("Echo> %s",buffer);

                  ext_send(fd, (char*)&buffer,received, 0);
               }
               else {
                  ClientList = g_list_remove(ClientList, (gpointer)fd);
                  ext_close(fd);

                  printTimeStamp(stdout);
                  printf("Removed client #%d.\n",fd);
                  break;
               }
               flags    = 0;
               received = recvfromplus(fd, (char*)&buffer, sizeof(buffer) - 1, &flags,
                                       NULL, 0,
                                       NULL, NULL, NULL,
                                       0);
            } while(received >= 0);
         }
      }
   }

   else {
      received = ext_recvfrom(fd,
                              (char*)&buffer, sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&address, &addressSize);
      do {
         if(received > 0) {
            buffer[received] = 0x00;
            printf("Echo> %s",buffer);
            received = ext_sendto(fd,
                       (char*)&buffer, received, 0,
                       (struct sockaddr*)&address, addressSize);
         }
         received = ext_recvfrom(fd,
                                 (char*)&buffer, sizeof(buffer) - 1, 0,
                                 (struct sockaddr*)&address, &addressSize);
      } while(received >= 0);
   }
}



/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   int            n;
   int            result;
   struct timeval timeout;
   fd_set         readfdset;
   unsigned long long         start;
   unsigned long long         stop;
   GList*         list;

   start = getMicroTime();
   stop  = 0;
#ifndef NO_RSP
   PoolName = "EchoPool";
#endif
   for(n = 1;n < argc;n++) {
      if(!(strcmp(argv[n], "-sctp-udplike"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_SCTP;
         ListenSocketType     = SOCK_DGRAM;
      }
      else if(!(strcmp(argv[n], "-sctp-tcplike"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_SCTP;
         ListenSocketType     = SOCK_STREAM;
      }
      else if(!(strcmp(argv[n], "-sctp"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_SCTP;
         ListenSocketType     = SOCK_STREAM;
      }
      else if(!(strcmp(argv[n], "-udp"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_UDP;
         ListenSocketType     = SOCK_DGRAM;
      }
      else if(!(strcmp(argv[n], "-tcp"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_TCP;
         ListenSocketType     = SOCK_STREAM;
      }
      else if(!(strncmp(argv[n], "-stop=",6))) {
         stop = start + ((unsigned long long)1000000 * (unsigned long long)atol((char*)&argv[n][6]));
      }
      else if(!(strncmp(argv[n], "-port=",6))) {
         ListenPort = atol((char*)&argv[n][6]);
      }
#ifndef NO_RSP
      else if(!(strncmp(argv[n], "-ph=",4))) {
         PoolName = (char*)&argv[n][4];
      }
      else if(!(strncmp(argv[n], "-load=",6))) {
         PolicyParameterLoad = atol((char*)&argv[n][6]);
         if(PolicyParameterLoad > 0xffffff) {
            PolicyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-loaddeg=",9))) {
         PolicyParameterLoadDegradation = atol((char*)&argv[n][9]);
         if(PolicyParameterLoadDegradation > 0xffffff) {
            PolicyParameterLoadDegradation = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-weight=",8))) {
         PolicyParameterWeight = atol((char*)&argv[n][8]);
         if(PolicyParameterWeight < 1) {
            PolicyParameterWeight = 1;
         }
      }
      else if(!(strncmp(argv[n], "-policy=",8))) {
         if((!(strcmp((char*)&argv[n][8], "roundrobin"))) || (!(strcmp((char*)&argv[n][8], "rr")))) {
            PolicyType = PPT_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[n][8], "weightedroundrobin"))) || (!(strcmp((char*)&argv[n][8], "wrr")))) {
            PolicyType = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[n][8], "leastused"))) || (!(strcmp((char*)&argv[n][8], "lu")))) {
            PolicyType = PPT_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "leastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "lud")))) {
            PolicyType = PPT_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedleastused"))) || (!(strcmp((char*)&argv[n][8], "rlu")))) {
            PolicyType = PPT_RANDOMIZED_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "rlud")))) {
            PolicyType = PPT_RANDOMIZED_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "priorityleastused"))) || (!(strcmp((char*)&argv[n][8], "plu")))) {
            PolicyType = PPT_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "priorityleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "plud")))) {
            PolicyType = PPT_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedpriorityleastused"))) || (!(strcmp((char*)&argv[n][8], "rplu")))) {
            PolicyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedpriorityleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "rplud")))) {
            PolicyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "random"))) || (!(strcmp((char*)&argv[n][8], "rand")))) {
            PolicyType = PPT_RANDOM;
         }
         else if((!(strcmp((char*)&argv[n][8], "weightedrandom"))) || (!(strcmp((char*)&argv[n][8], "wrand")))) {
            PolicyType = PPT_WEIGHTED_RANDOM;
         }
         else {
            printf("ERROR: Unknown policy type \"%s\"!\n", (char*)&argv[n][8]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[n], "-log",4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
#endif
      else {
         printf("Bad argument \"%s\"!\n",argv[n]);
         printf("Usage: %s {-sctp|-sctp-udplike|-sctp-tcplike|-tcp|-udp} {-port=local port} {-stop=seconds}"
#ifndef NO_RSP
                " {-ph=Pool Handle} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} {-policy=roundrobin|rr|weightedroundrobin|wee|leastused|lu|leastuseddegradation|lud|random|rd|weightedrandom|wrand} {-load=load} {-loaddeg=load degradation} {-weight=weight}"
#endif
                "\n",argv[0]);
         exit(1);
      }
   }

   beginLogging();
   initAll();

   puts("Example Pool Element - Version 2.0");
   puts("----------------------------------\n");

   while(!breakDetected()) {
      /* Collect sources for ext_select() call */
      FD_ZERO(&readfdset);
      FD_SET(ListenSocket,&readfdset);
      n = ListenSocket;
      list = g_list_first(ClientList);
      while(list != NULL) {
         FD_SET((int)list->data,&readfdset);
         n = max(n, (int)list->data);
         list = g_list_next(list);
      }

      /* Wait for activity. Note: rspSelect() is called in the
         background thread. Therefore, it is *not* necessary here! */
      timeout.tv_sec  = 0;
      timeout.tv_usec = 500000;
      result = ext_select(n + 1, &readfdset, NULL, NULL, &timeout);
      if(errno == EINTR) {
         continue;
      }

      /* Handle results of ext_select() */
      if(result > 0) {
         if(FD_ISSET(ListenSocket,&readfdset)) {
            socketHandler(ListenSocket);
         }
         list = g_list_first(ClientList);
         while(list != NULL) {
            if(FD_ISSET((int)list->data,&readfdset)) {
               socketHandler((int)list->data);
            }
            list = g_list_next(list);
         }
      }
      if(result < 0) {
         perror("select() failed");
         cleanUp();
         exit(1);
      }

      /* Check auto-stop timer */
      if((stop > 0) && (getMicroTime() >= stop)) {
         puts("Auto-stop time reached -> exiting!");
         break;
      }
   }

   cleanUp();
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
