/*
 *  $Id: simpleexamplepu.c,v 1.3 2004/07/20 15:35:15 dreibh Exp $
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
 * Purpose: Example Pool User
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "rsplib.h"
#include "breakdetector.h"

#include <ext_socket.h>
#include <signal.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
#define FAST_BREAK



static int      DataSocket         = -1;
static uint32_t Identifier         = 0x00000000;
static char*    PoolName           = NULL;
static card64   InBytes            = 0;
static card64   OutBytes           = 0;
static card64   OutCalls           = 0;
static card64   ConnectTimeout     = 3000000;



/* ###### Clean-up ########################################################## */
static void cleanUp()
{
   if(DataSocket >= 0) {
      ext_close(DataSocket);
      DataSocket = -1;
   }
   rspCleanUp();
}


/* ###### Initialize ######################################################## */
static void initAll()
{
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
      cleanUp();
      exit(1);
   }

   /* Ignore SIGPIPE */
   signal(SIGPIPE,SIG_IGN);

#ifndef FAST_BREAK
   installBreakDetector();
#endif
}


/* ###### Do failover to new pool element ################################### */
static void connectToPoolElement()
{
   struct TagItem              tags[16];
   struct EndpointAddressInfo* eai;
   struct EndpointAddressInfo* eai2;
   int                         result;

   /* ====== Close existing connection and report failure =================== */
   if(DataSocket >= 0) {
      rspReportFailure((unsigned char*)PoolName, strlen(PoolName), Identifier, NULL);
      ext_close(DataSocket);
      DataSocket = -1;
      Identifier = 0x00000000;
   }

   /* ====== Do name resolution ============================================= */
   result = rspNameResolution((unsigned char*)PoolName, strlen(PoolName), &eai, NULL);
   if(result != 0) {
      printf("Name resolution failed - %s.\n",rspGetLastErrorDescription());
      return;
   }

   /* ====== Establish connection =========================================== */
   eai2 = eai;
   while(eai2 != NULL) {
      DataSocket = establish(eai2->ai_family,
                             eai2->ai_socktype,
                             eai2->ai_protocol,
                             eai2->ai_addr,
                             eai2->ai_addrs,
                             ConnectTimeout);
      if(DataSocket >= 0) {
         Identifier = eai2->ai_pe_id;
         setNonBlocking(DataSocket);
         printf("Connected to ");
         fputaddress((struct sockaddr*)&eai2->ai_addr[0],true,stdout);
         printf(", Pool Element $%08x\n", eai2->ai_pe_id);

         if(eai2->ai_protocol == IPPROTO_SCTP) {
            tags[0].Tag = TAG_TuneSCTP_MinRTO;
            tags[0].Data = 1000;
            tags[1].Tag = TAG_TuneSCTP_MaxRTO;
            tags[1].Data = 2000;
            tags[2].Tag = TAG_TuneSCTP_InitialRTO;
            tags[2].Data = 500;
            tags[3].Tag = TAG_TuneSCTP_Heartbeat;
            tags[3].Data = 100;
            tags[4].Tag = TAG_TuneSCTP_PathMaxRXT;
            tags[4].Data = 1;
            tags[5].Tag = TAG_TuneSCTP_AssocMaxRXT;
            tags[5].Data = 2;
            tags[6].Tag = TAG_DONE;
            if(!tuneSCTP(DataSocket, 0, (struct TagItem*)&tags)) {
               puts("WARNING: Unable to set SCTP parameters!");
            }
         }
         break;
      }
      else {
         printf("Pool Element $%08x at ", eai2->ai_pe_id);
         fputaddress((struct sockaddr*)&eai2->ai_addr[0],true,stdout);
         puts(" is not answering -> reporting it as failed.");
         rspReportFailure((unsigned char*)PoolName, strlen(PoolName), eai2->ai_pe_id, NULL);
      }
      eai2 = eai2->ai_next;
   }

   /* ====== Free name resolution result ==================================== */
   rspFreeEndpointAddressArray(eai);
}


/* ###### Send buffer to pool element #################################### */
static void sendToPoolElement(char* buffer)
{
   struct msghdr msg;
   struct iovec  io;
   int           result;
   ssize_t       received;

   received = strlen(buffer);
   msg.msg_name       = NULL;
   msg.msg_namelen    = 0;
   msg.msg_control    = NULL;
   msg.msg_controllen = 0;
   msg.msg_iov        = &io;
   msg.msg_iovlen     = 1;
#ifdef MSG_NOSIGNAL
   msg.msg_flags      = MSG_NOSIGNAL;
#else
   msg.msg_flags      = 0;
#endif
   io.iov_base = buffer;
   io.iov_len  = received;
   OutBytes += (card64)received;

   result = ext_sendmsg(DataSocket, &msg, msg.msg_flags);
   if(result < 0) {
      puts("Writing to data socket failed!");
      connectToPoolElement();
   }
}


/* ###### Handle standard input ############################################# */
static void handleStdIn()
{
   char   buffer[8192];
   size_t lineNumberLength;

   snprintf((char*)&buffer,sizeof(buffer),"%09Ld ",++OutCalls);
   lineNumberLength = strlen(buffer);

   setNonBlocking(STDIN_FILENO);
   fgets((char*)&buffer[lineNumberLength],sizeof(buffer) - lineNumberLength,stdin);
   setBlocking(STDIN_FILENO);
   if(buffer[lineNumberLength] == 0x00) {
      sendBreak(true);
   }
   else {
      sendToPoolElement((char*)&buffer);
   }
}


/* ###### Automatic mode #################################################### */
static void handleAutoTimer()
{
   char   buffer[512];
   size_t lineNumberLength;

   snprintf((char*)&buffer, sizeof(buffer),"%09Ld ", ++OutCalls);
   lineNumberLength = strlen(buffer);

   snprintf((char*)&buffer[lineNumberLength],sizeof(buffer) - lineNumberLength,"This is a test!\n");
   /* printf(&buffer[lineNumberLength]); */
   printf(buffer);
   fflush(stdout);
   sendToPoolElement((char*)&buffer);
}


/* ###### Handle server reply ############################################### */
static void handleServerReply()
{
   char          buffer[8193];
   struct msghdr msg;
   struct iovec  io;
   ssize_t       received;
   static card64 counter = 1;

   msg.msg_name       = NULL;
   msg.msg_namelen    = 0;
   msg.msg_control    = NULL;
   msg.msg_controllen = 0;
   msg.msg_iov        = &io;
   msg.msg_iovlen     = 1;
#ifdef MSG_NOSIGNAL
   msg.msg_flags      = MSG_NOSIGNAL|MSG_DONTWAIT;
#else
   msg.msg_flags      = MSG_DONTWAIT;
#endif
   io.iov_base = buffer;
   io.iov_len  = sizeof(buffer) - 1;

   received = ext_recvmsg(DataSocket, &msg,msg.msg_flags);
   if(received > 0) {
      InBytes += received;
      buffer[received] = 0x00;

/*
      printf("[in=%Ld out=%Ld] P%Ld.%Ld> ",
             (card64)InBytes, (card64)OutBytes,
             (card64)Handle, (card64)counter++);
*/
      printf("[in=%Ld",InBytes);
      printf(" out=%Ld] P",OutBytes);
      printf("$%08x.",Identifier);
      printf("%Ld> ",counter++);
      puts(buffer);
      fflush(stdout);
   }
   else {
      puts("Reading from data socket failed!");
      connectToPoolElement();
   }
}



/* ###### Main program ###################################################### */
int main(int argc, char** argv)
{
   int            n = 0;
   int            result;
   struct timeval timeout;
   fd_set         readfdset;
   card64         now;
   card64         selectTimeout = 0;
   card64         autoInterval  = 0;
   card64         nextAutoSend  = 0;

   if(argc < 2) {
      printf("Usage: %s [Pool Name] {-auto=milliseconds} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}\n",argv[0]);
      exit(1);
   }

   PoolName = argv[1];
   if(argc > 2) {
      for(n = 2;n < argc;n++) {
         if(!(strncmp(argv[n],"-log",4))) {
            if(initLogging(argv[n]) == false) {
               exit(1);
            }
         }
         else if(!(strncmp(argv[n],"-auto=",6))) {
            autoInterval = 1000 * (card64)atol((char*)&argv[n][6]);
         }
         else {
            puts("Bad arguments!");
            exit(1);
         }
      }
   }
   if(autoInterval) {
      nextAutoSend = getMicroTime();
   }
   else {
      selectTimeout = 500000;
   }


   beginLogging();
   initAll();

   puts("Example Pool User - Version 2.0");
   puts("-------------------------------\n");

   connectToPoolElement();

   while(!breakDetected()) {
      FD_ZERO(&readfdset);
      if(autoInterval) {
         now = getMicroTime();
         if(now >= nextAutoSend) {
            selectTimeout = 0;
         }
         else {
            selectTimeout = min(nextAutoSend - now,500000);
         }
         if(DataSocket >= 0) {
            FD_SET(DataSocket,&readfdset);
            n = DataSocket;
         }
      }
      else {
         FD_SET(STDIN_FILENO,&readfdset);
         if(DataSocket >= 0) {
            FD_SET(DataSocket,&readfdset);
            n = max(DataSocket,STDIN_FILENO);
         }
         else {
            n = STDIN_FILENO;
         }
      }
      timeout.tv_sec  = selectTimeout / 1000000;
      timeout.tv_usec = selectTimeout % 1000000;
      result = rspSelect(n + 1, &readfdset, NULL, NULL, &timeout);
      if(errno == EINTR) {
         break;
      }

      if(result > 0) {
         if(DataSocket >= 0) {
            if(FD_ISSET(DataSocket,&readfdset)) {
               handleServerReply();
            }
         }
         if(!autoInterval) {
            if(FD_ISSET(STDIN_FILENO,&readfdset)) {
               handleStdIn();
            }
         }
      }
      if((autoInterval) && (getMicroTime() >= nextAutoSend)) {
         handleAutoTimer();
         nextAutoSend += autoInterval;
      }
      if(result < 0) {
         perror("select() failed");
         cleanUp();
         exit(1);
      }
   }

   cleanUp();
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
