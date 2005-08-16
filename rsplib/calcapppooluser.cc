/*
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium fr Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (F�derkennzeichen 01AK045).
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
 * Purpose: CalcApp Pool User
 *
 */

#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "breakdetector.h"
#include "rsplib.h"
#include "calcapppackets.h"

#include <ext_socket.h>
#include <signal.h>

#include <iostream>


using namespace std;


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */

unsigned int JobID = 0;


void sendCalcAppKeepAliveAck(struct SessionDescriptor* session)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type      = htonl(CALCAPP_KEEPALIVE_ACK);
   message.JobID     = htonl(JobID);
   if(rspSessionWrite(session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppKeepAlive" << endl;
      exit(1);
   }
}


void handleCalcAppKeepAlive(struct SessionDescriptor* session,
                            CalcAppMessage*           message,
                            const size_t              size)
{
   if(JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppKeepAlive for wrong job!" << endl;
      exit(1);
   }
puts("KEEP-ALIVE");
   sendCalcAppKeepAliveAck(session);
}


void runJob(const char* poolHandle, const double jobSize)
{
   struct SessionDescriptor* session;
   char                      buffer[4096];
   struct TagItem            tags[16];
   ssize_t                   received;

   session = rspCreateSession((unsigned char*)poolHandle, strlen(poolHandle),
                              NULL, (struct TagItem*)&tags);
   if(session != NULL) {
      CalcAppMessage message;
      memset(&message, 0, sizeof(message));
      message.Type    = htonl(CALCAPP_REQUEST);
      message.JobID   = ++JobID;
      message.JobSize = jobSize;
      if(rspSessionWrite(session, (void*)&message, sizeof(message), NULL) > 0) {

         bool finished = false;
         while(!finished) {
            tags[0].Tag  = TAG_RspIO_MsgIsCookieEcho;
            tags[0].Data = 0;
            tags[1].Tag  = TAG_RspIO_Timeout;
            tags[1].Data = 30000000;
            tags[2].Tag  = TAG_DONE;
            received = rspSessionRead(session, (char*)&buffer, sizeof(buffer), (struct TagItem*)&tags);
            if(received > 0) {
               printf("recv=%d\n",received);

               if(received >= (ssize_t)sizeof(CalcAppMessage)) {
                  CalcAppMessage* response = (CalcAppMessage*)&buffer;
                  switch(ntohl(response->Type)) {
                     case CALCAPP_ACCEPT:
                        cout << "Job " << JobID << " accepted" << endl;
                     break;
                     case CALCAPP_COMPLETE:
                        cout << "Job " << JobID << " completed" << endl;
                        finished = true;
                      break;
                     case CALCAPP_KEEPALIVE:
                        handleCalcAppKeepAlive(session, response, sizeof(response));
                      break;
                     default:
                        cerr << "ERROR: Unknown message type " << ntohl(response->Type) << endl;
                      break;
                  }
               }
            }
            else {
               cerr << "ERROR: Unable to start job " << JobID << endl;
               finished = true;
            }
         }

      }

      rspDeleteSession(session, NULL);
   }
}


/* ###### Main program ###################################################### */
int main(int argc, char** argv)
{
   struct TagItem tags[16];
   char*          poolHandle    = "CalcAppPool";
   int            i;
   int            n;

   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i],"-ph=",4))) {
         poolHandle = (char*)&argv[i][4];
      }
      else if(!(strncmp(argv[i],"-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-registrar=" ,11))) {
         /* Process this later */
      }
      else {
         puts("Bad arguments!");
         printf("Usage: %s {-registrar=Registrar address(es)} {-ph=Pool handle} {-auto=milliseconds} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}\n", argv[0]);
         exit(1);
      }
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
      exit(1);
   }
#ifndef FAST_BREAK
   installBreakDetector();
#endif

   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-registrar=" ,11))) {
         if(rspAddStaticRegistrar((char*)&argv[n][11]) != RSPERR_OKAY) {
            fprintf(stderr, "ERROR: Bad registrar setting: %s\n", argv[n]);
            exit(1);
         }
      }
   }


   tags[0].Tag = TAG_DONE;
/*
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
*/


   cout << "CalcApp Pool User - Version 2.0" << endl;
   cout << "-------------------------------" << endl;


   runJob(poolHandle, 10000000.0);


   finishLogging();
   rspCleanUp();
   puts("\nTerminated!");
   return(0);
}
