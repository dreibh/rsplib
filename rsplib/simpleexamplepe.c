/*
 *  $Id: simpleexamplepe.c,v 1.2 2004/07/18 15:30:43 dreibh Exp $
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
#include "localaddresses.h"
#include "breakdetector.h"
#include "rsplib.h"

#include <ext_socket.h>
#include <pthread.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */

/* Disable RSerPool support */
/* #define NO_RSP */



static int           ListenSocketFamily    = AF_INET;
static int           ListenSocketType      = SOCK_STREAM;
static int           ListenSocketProtocol  = IPPROTO_TCP;
static uint16_t      ListenPort            = 0;
static int           ListenSocket          = -1;
static GList*        ClientList            = NULL;


#ifndef NO_RSP
static char*         PoolName               = NULL;
static uint32_t      PoolElementID          = 0x00000000;
static unsigned int  PolicyType             = TAGDATA_PoolPolicy_Type_RoundRobin;
static unsigned int  PolicyParameterWeight  = 1;
static unsigned int  PolicyParameterLoad    = 0;
static unsigned long RegistrationInterval   = 5000000;
static pthread_t     RegistrationThread     = 0;
static bool          RegistrationThreadStop = false;


/* ###### Registration/Reregistration ####################################### */
static void doRegistration()
{
   struct TagItem              tags[16];
   struct EndpointAddressInfo* eai;
   struct EndpointAddressInfo* eai2;
   struct EndpointAddressInfo* next;
   struct sockaddr_storage*    sctpLocalAddressArray = NULL;
   struct sockaddr_storage*    localAddressArray     = NULL;
   struct sockaddr_storage     socketName;
   size_t                      socketNameLen;
   unsigned int                localAddresses;
   unsigned int                i;

   /* ====== Create EndpointAddressInfo structure =========================== */
   eai = (struct EndpointAddressInfo*)malloc(sizeof(struct EndpointAddressInfo));
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
   eai->ai_addrlen  = sizeof(struct sockaddr_storage);
   eai->ai_pe_id    = PoolElementID;

   /* ====== Get local addresses for SCTP socket ============================ */
   if(ListenSocketProtocol == IPPROTO_SCTP) {
      eai->ai_addrs    = getladdrsplus(ListenSocket, 0, &eai->ai_addr);
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
      if(gatherLocalAddresses(&localAddressArray, (size_t *)&localAddresses, ASF_HideMulticast) == false) {
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

      /* Add addresses to EndpointAddressInfo structure */
      if(ListenSocketProtocol == IPPROTO_SCTP) {
         /* SCTP endpoints are described by a list of their
            transport addresses. */
         eai->ai_addrs = localAddresses;
         eai->ai_addr  = localAddressArray;
      }
      else {
         /* TCP/UDP endpoints are described by an own EndpointAddressInfo
            for each transport address. */
         eai2 = eai;
         for(i = 0;i < localAddresses;i++) {
            eai2->ai_addrs = 1;
            eai2->ai_addr  = &localAddressArray[i];
            if(i < localAddresses - 1) {
               next = (struct EndpointAddressInfo*)malloc(sizeof(struct EndpointAddressInfo));
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
   tags[2].Tag  = TAG_PoolPolicy_Parameter_Weight;
   tags[2].Data = PolicyParameterWeight;
   tags[3].Tag  = TAG_END;


   /* ====== Do registration ================================================ */
   PoolElementID = rspRegister((unsigned char*)PoolName, strlen(PoolName),
                               eai, (struct TagItem*)&tags);
   if(PoolElementID == 0x00000000) {
      printf("WARNING: (Re-)Registration failed: ");
      puts(rspGetLastErrorDescription());
   }
   else {
      printf("(Re-)Registration successful - ID=$%08x\n", PoolElementID);
   }


   /* ====== Clean up ======================================================= */
   if(sctpLocalAddressArray) {
      free(sctpLocalAddressArray);
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
      rspDeregister((unsigned char*)PoolName, strlen(PoolName), PoolElementID, NULL);
   }
}


/* ###### Registration/Reregistration thread ################################ */
static void* registrationMainLoop(void* args)
{
   struct timeval timeout;
   card64         start;
   card64         now;

   for(;;) {
      /* ====== Do registration/reregistration ============================== */
      doRegistration();

      /* ====== Sleep and allow rsplib to handle internal events ============ */
      start = getMicroTime();
      do {
         if(RegistrationThreadStop) {
            goto finish;
         }
         pthread_testcancel();
         timeout.tv_sec  = 0;
         timeout.tv_usec = 1000000;
         rspSelect(0, NULL, NULL, NULL, &timeout);
         now = getMicroTime();
      } while(now < start + RegistrationInterval);
   }

   /* ====== Do deregistration ============================================== */
finish:
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
      ClientList = g_list_remove(ClientList,(gpointer)fd);
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
   rspCleanUp();
#endif
}


/* ###### Initialize ##################################################### */
static void initAll()
{
   struct sockaddr_storage localAddress;

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


   memset((void*)&localAddress,0,sizeof(localAddress));
   ((struct sockaddr*)&localAddress)->sa_family = ListenSocketFamily;
   setPort((struct sockaddr*)&localAddress,ListenPort);

   if(bindplus(ListenSocket,(struct sockaddr_storage*)&localAddress,1) == false) {
      perror("Unable to bind application socket");
      cleanUp();
      exit(1);
   }

   if(ListenSocketType != SOCK_DGRAM) {
      if(ext_listen(ListenSocket,5) < 0) {
         perror("Unable to set application socket to listen mode");
         cleanUp();
         exit(1);
      }
   }

#ifndef NO_RSP
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
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
   char                       buffer[16385];
   struct sockaddr_storage    address;
   socklen_t                  addressSize;
   ssize_t                    received;
   int                        sd;

   if(ListenSocketType != SOCK_DGRAM) {
      if(fd == ListenSocket) {
         sd = ext_accept(ListenSocket,NULL,0);
         if(sd >= 0) {
            ClientList = g_list_append(ClientList,(gpointer)sd);
            setNonBlocking(sd);

            printTimeStamp(stdout);
            printf("Added client #%d.\n",sd);
         }
      }
      else {
         if(g_list_find(ClientList,(gpointer)fd) != NULL) {
            received = recvfromplus(fd, (char*)&buffer, sizeof(buffer) - 1, 0,
                                    NULL, 0,
                                    NULL, NULL, NULL,
                                    0);
            do {
               if(received > 0) {

                  buffer[received] = 0x00;
                  printf("Echo> %s",buffer);

                  ext_send(fd,(char*)&buffer,received,0);
               }
               else {
                  ClientList = g_list_remove(ClientList,(gpointer)fd);
                  ext_close(fd);

                  printTimeStamp(stdout);
                  printf("Removed client #%d.\n",fd);
                  break;
               }
               received = recvfromplus(fd, (char*)&buffer, sizeof(buffer) - 1, 0,
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
   card64         start;
   card64         stop;
   GList*         list;

   start = getMicroTime();
   stop  = 0;
#ifndef NO_RSP
   PoolName = "EchoPool";
#endif
   for(n = 1;n < argc;n++) {
      if(!(strcmp(argv[n],"-sctp-udplike"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_SCTP;
         ListenSocketType     = SOCK_DGRAM;
      }
      else if(!(strcmp(argv[n],"-sctp-tcplike"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_SCTP;
         ListenSocketType     = SOCK_STREAM;
      }
      else if(!(strcmp(argv[n],"-sctp"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_SCTP;
         ListenSocketType     = SOCK_STREAM;
      }
      else if(!(strcmp(argv[n],"-udp"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_UDP;
         ListenSocketType     = SOCK_DGRAM;
      }
      else if(!(strcmp(argv[n],"-tcp"))) {
         ListenSocketFamily   = AF_INET6;
         ListenSocketProtocol = IPPROTO_TCP;
         ListenSocketType     = SOCK_STREAM;
      }
      else if(!(strncmp(argv[n],"-stop=",6))) {
         stop = start + ((card64)1000000 * (card64)atol((char*)&argv[n][6]));
      }
      else if(!(strncmp(argv[n],"-port=",6))) {
         ListenPort = atol((char*)&argv[n][6]);
      }
#ifndef NO_RSP
      else if(!(strncmp(argv[n],"-ph=",4))) {
         PoolName = (char*)&argv[n][4];
      }
      else if(!(strncmp(argv[n],"-load=",6))) {
         PolicyParameterLoad = atol((char*)&argv[n][6]);
         if(PolicyParameterLoad > 0xffffff) {
            PolicyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n],"-weight=",8))) {
         PolicyParameterWeight = atol((char*)&argv[n][8]);
         if(PolicyParameterWeight > 0xffffff) {
            PolicyParameterWeight = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n],"-policy=",8))) {
         if((!(strcmp((char*)&argv[n][8],"roundrobin"))) || (!(strcmp((char*)&argv[n][8],"rr")))) {
            PolicyType = TAGDATA_PoolPolicy_Type_RoundRobin;
         }
         else if((!(strcmp((char*)&argv[n][8],"weightedroundrobin"))) || (!(strcmp((char*)&argv[n][8],"wrr")))) {
            PolicyType = TAGDATA_PoolPolicy_Type_WeightedRoundRobin;
         }
         else if((!(strcmp((char*)&argv[n][8],"leastused"))) || (!(strcmp((char*)&argv[n][8],"lu")))) {
            PolicyType = TAGDATA_PoolPolicy_Type_LeastUsed;
         }
         else if((!(strcmp((char*)&argv[n][8],"leastuseddegradation"))) || (!(strcmp((char*)&argv[n][8],"lud")))) {
            PolicyType = TAGDATA_PoolPolicy_Type_LeastUsedDegradation;
         }
         else if((!(strcmp((char*)&argv[n][8],"random"))) || (!(strcmp((char*)&argv[n][8],"rd")))) {
            PolicyType = TAGDATA_PoolPolicy_Type_Random;
         }
         else if((!(strcmp((char*)&argv[n][8],"weightedrandom"))) || (!(strcmp((char*)&argv[n][8],"wrd")))) {
            PolicyType = TAGDATA_PoolPolicy_Type_WeightedRandom;
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
#endif
      else {
         printf("Bad argument \"%s\"!\n",argv[n]);
         printf("Usage: %s {-sctp|-sctp-udplike|-sctp-tcplike|-tcp|-udp} {-port=local port} {-stop=seconds}"
#ifndef NO_RSP
                " {-ph=Pool Handle} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} {-policy=roundrobin|rr|weightedroundrobin|wee|leastused|lu|leastuseddegradation|lud|random|rd|weightedrandom|wrd} {-load=load} {-weight=weight}"
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
         n = max(n,(int)list->data);
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
