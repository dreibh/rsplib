/*
 *  $Id: examplepu.c,v 1.10 2004/11/10 22:07:34 dreibh Exp $
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
#include "breakdetector.h"
#include "rsplib.h"
#ifdef ENABLE_CSP
#include "componentstatusprotocol.h"
#endif


#include <ext_socket.h>
#include <signal.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */


struct SessionDescriptor* Session;
card64                    OutCalls = 0;
card64                    InBytes  = 0;
card64                    OutBytes = 0;


/* ###### Handle standard input ############################################# */
bool handleFailover(void* userData)
{
   puts("FAILOVER IS NECESSARY!");
   return(true);
}


/* ###### Handle standard input ############################################# */
static void handleStdIn()
{
   char   buffer[8192];
   size_t lineNumberLength;
   size_t toSend;

   snprintf((char*)&buffer,sizeof(buffer),"%09Ld ",++OutCalls);
   lineNumberLength = strlen(buffer);

   setNonBlocking(STDIN_FILENO);
   fgets((char*)&buffer[lineNumberLength],sizeof(buffer) - lineNumberLength,stdin);
   setBlocking(STDIN_FILENO);

   if(buffer[lineNumberLength] == 0x00) {
      sendBreak(true);
   }
   toSend = strlen(buffer);
   if(rspSessionWrite(Session, (char*)&buffer, toSend, NULL) <= 0) {
      puts("Writing to pool element failed!");
   }
   else {
      OutBytes += toSend;
   }
}


/* ###### Automatic mode #################################################### */
static void handleAutoTimer()
{
   char   buffer[512];
   size_t lineNumberLength;
   size_t toSend;

   snprintf((char*)&buffer,sizeof(buffer),"%09Ld ",++OutCalls);
   lineNumberLength = strlen(buffer);

   snprintf((char*)&buffer[lineNumberLength], sizeof(buffer) - lineNumberLength,
            "This is a test %09Ld!\n", OutCalls);
   printf(&buffer[lineNumberLength]);
   fflush(stdout);

   toSend = strlen(buffer);
   if(rspSessionWrite(Session, (char*)&buffer, toSend, NULL) <= 0) {
      puts("Writing to pool element failed!");
   }
   else {
      OutBytes += toSend;
   }
}


/* ###### Handle server reply ############################################### */
static void handleServerReply()
{
   struct TagItem tags[16];
   char           buffer[8193];
   ssize_t        received;
   static card64  counter = 1;

   tags[0].Tag = TAG_RspIO_PE_ID;
   tags[1].Tag = TAG_DONE;

   received = rspSessionRead(Session, (char*)&buffer, sizeof(buffer), (struct TagItem*)&tags);
   if(received > 0) {
      InBytes += received;
      buffer[received] = 0x00;

      printf("##### [in=%Ld", InBytes);
      printf(" out=%Ld] P", OutBytes);
      printf("$%08x.", (int)tags[0].Data);
      printf("%Ld rcv=%d> ", counter++, received);
      puts(buffer);
      fflush(stdout);
   }
   else if(errno != EAGAIN) {
      puts("rspSessionRead failed!");
   }
   else {
      puts("Message/Partial -- DAS SOLLTE NICHT ANGEZEIGT WERDEN!");
   }
}



/* ###### Main program ###################################################### */
int main(int argc, char** argv)
{
#ifdef ENABLE_CSP
   struct CSPReporter        cspReporter;
   uint64_t                  cspIdentifier     = 0;
   unsigned int              cspReportInterval = 0;
   union sockaddr_union      cspReportAddress;
#endif
   struct TagItem            tags[16];
   struct SessionDescriptor* sessionArray[1];
   unsigned int              statusArray[1];
   fd_set                    readfdset;
   char*                     poolHandle    = "EchoPool";
   card64                    autoInterval  = 0;
   card64                    selectTimeout = 0;
   card64                    nextAutoSend  = 250000;
   card64                    now;
   int                       result;
   int                       i, n;

#ifdef ENABLE_CSP
   string2address("127.0.0.1:2960", &cspReportAddress);
#endif
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i],"-ph=",4))) {
         poolHandle = (char*)&argv[i][4];
      }
      else if(!(strncmp(argv[i],"-auto=",6))) {
         autoInterval = 1000 * (card64)atol((char*)&argv[i][6]);
      }
#ifdef ENABLE_CSP
      else if(!(strncmp(argv[i], "-identifier=", 12))) {
         cspIdentifier = CID_COMPOUND(CID_GROUP_POOLUSER, atol((char*)&argv[i][12]));
      }
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
#endif
      else if(!(strncmp(argv[i],"-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-nameserver=" ,12))) {
         /* Process this later */
      }
      else {
         puts("Bad arguments!");
         printf("Usage: %s {-nameserver=Nameserver address(es)} {-ph=Pool handle} {-auto=milliseconds} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}"
#ifdef ENABLE_CSP
                " {-cspreportaddress=Address} {-cspreportinterval=Microseconds} {-identifier=PU Identifier (used for CSP only)}"
#endif
                "\n",argv[0]);
         exit(1);
      }
   }
   if(autoInterval) {
      nextAutoSend = getMicroTime();
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
      exit(1);
   }
#ifdef ENABLE_CSP
   if((cspReportInterval > 0) && (cspIdentifier != 0)) {
      cspReporterNewForRspLib(&cspReporter, cspIdentifier, &cspReportAddress, cspReportInterval);
   }
#endif
#ifndef FAST_BREAK
   installBreakDetector();
#endif

   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-nameserver=" ,12))) {
         if(rspAddStaticNameServer((char*)&argv[n][12]) != RSPERR_OKAY) {
            fprintf(stderr, "ERROR: Bad name server setting: %s\n", argv[n]);
            exit(1);
         }
      }
   }

   tags[0].Tag = TAG_TuneSCTP_MinRTO;
   tags[0].Data = 100;
   tags[1].Tag = TAG_TuneSCTP_MaxRTO;
   tags[1].Data = 500;
   tags[2].Tag = TAG_TuneSCTP_InitialRTO;
   tags[2].Data = 250;
   tags[3].Tag = TAG_TuneSCTP_Heartbeat;
   tags[3].Data = 100;
   tags[4].Tag = TAG_TuneSCTP_PathMaxRXT;
   tags[4].Data = 3;
   tags[5].Tag = TAG_TuneSCTP_AssocMaxRXT;
   tags[5].Data = 9;
   tags[6].Tag  = TAG_RspSession_FailoverCallback;
   tags[6].Data = (tagdata_t)handleFailover;
   tags[7].Tag  = TAG_RspSession_FailoverUserData;
   tags[7].Data = (tagdata_t)NULL;
   tags[8].Tag = TAG_DONE;

   Session = rspCreateSession((unsigned char*)poolHandle, strlen(poolHandle),
                              NULL, (struct TagItem*)&tags);
   if(Session != NULL) {
      puts("Example Pool User - Version 2.0");
      puts("-------------------------------\n");

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
            n = 0;
         }
         else {
            FD_SET(STDIN_FILENO, &readfdset);
            n = STDIN_FILENO;
         }

         /* Very important! Allow rspSessionSelect() to call
            the rsplib event loop. */
         tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
         tags[0].Data = 1;

         tags[1].Tag  = TAG_RspSelect_ReadFDs;
         tags[1].Data = (tagdata_t)&readfdset;
         tags[2].Tag  = TAG_RspSelect_MaxFD;
         tags[2].Data = n + 1;
         tags[3].Tag  = TAG_DONE;

         sessionArray[0] = Session;
         statusArray[0]  = RspSelect_Read;

         result = rspSessionSelect((struct SessionDescriptor**)&sessionArray,
                                   (unsigned int*)&statusArray,
                                   1,
                                   NULL, NULL, 0,
                                   selectTimeout,
                                   (struct TagItem*)&tags);
         if(result > 0) {
            if(statusArray[0] & RspSelect_Read) {
               handleServerReply();
            }
            if(!autoInterval) {
               if(FD_ISSET(STDIN_FILENO, &readfdset)) {
                  handleStdIn();
               }
            }
         }

         if((autoInterval) && (getMicroTime() >= nextAutoSend)) {
            handleAutoTimer();
            nextAutoSend += autoInterval;
         }
         if(result < 0) {
            perror("rspSelect() failed");
            break;
         }
      }

      rspDeleteSession(Session, NULL);
   }

   finishLogging();
#ifdef ENABLE_CSP
   if(cspReportInterval > 0) {
      cspReporterDelete(&cspReporter);
   }
#endif
   rspCleanUp();
   puts("\nTerminated!");
   return(0);
}
