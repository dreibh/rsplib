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


unsigned long long KeepAliveTransmissionInterval = 5000000;
unsigned long long KeepAliveTimeoutInterval      = 5000000;


enum ProcessStatus {
   PS_Init       = 0,
   PS_Processing = 1,
   PS_Failover   = 2,
   PS_Failed     = 3,
   PS_Completed  = 4
};

struct Process {
   SessionDescriptor* Session;
   unsigned int       JobID;
   double             JobSize;
   ProcessStatus      Status;

   // ------ Timers ------------------------------------------------------
   unsigned long long KeepAliveTransmissionTimeStamp;
   unsigned long long KeepAliveTimeoutTimeStamp;
};


// ###### Send CalcAppRequest message #######################################
void sendCalcAppRequest(struct Process* process)
{
   CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type    = htonl(CALCAPP_REQUEST);
   message.JobID   = ++process->JobID;
   message.JobSize = process->JobSize;

   ssize_t result = rspSessionWrite(process->Session,
                                    (void*)&message, sizeof(message), NULL);
   if(result <= 0) {
      process->Status = PS_Failed;
   }
}


// ###### Send CalcAppKeepAlive message #####################################
void sendCalcAppKeepAlive(struct Process* process)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type  = htonl(CALCAPP_KEEPALIVE);
   message.JobID = htonl(process->JobID);

   ssize_t result = rspSessionWrite(process->Session,
                                    (void*)&message, sizeof(message), NULL);
   if(result <= 0) {
      cerr << "WARNING: Unable to send CalcAppKeepAlive" << endl;
      process->Status = PS_Failover;
   }
}


// ###### Send CalcAppKeepAliveAck message ##################################
void sendCalcAppKeepAliveAck(struct Process* process)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type  = htonl(CALCAPP_KEEPALIVE_ACK);
   message.JobID = htonl(process->JobID);

   ssize_t result = rspSessionWrite(process->Session,
                                    (void*)&message, sizeof(message), NULL);
   if(result <= 0) {
      cerr << "WARNING: Unable to send CalcAppKeepAliveAck" << endl;
      process->Status = PS_Failover;
   }
}


// ###### Handle incoming CalcAppAccept message #############################
void handleCalcAppAccept(struct Process* process,
                         CalcAppMessage* message,
                         const size_t    size)
{
   if(process->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppAccept for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }

   cout << "Job " << process->JobID << " accepted" << endl;

   process->Status                         = PS_Processing;
   process->KeepAliveTimeoutTimeStamp      = ~0ULL;
   process->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
}


// ###### Handle incoming CalcAppReject message #############################
void handleCalcAppReject(struct Process* process,
                         CalcAppMessage* message,
                         const size_t    size)
{
   if(process->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppReject for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }

   cout << "Job " << process->JobID << " rejected" << endl;
   process->Status = PS_Failover;
   rspSessionFailover(process->Session);
}


// ###### Handle incoming CalcAppKeepAlive message ##########################
void handleCalcAppKeepAlive(struct Process* process,
                            CalcAppMessage* message,
                            const size_t    size)
{
   if(process->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppKeepAlive for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }
   sendCalcAppKeepAliveAck(process);
}


// ###### Handle incoming CalcAppKeepAliveAck message #######################
void handleCalcAppKeepAliveAck(struct Process* process,
                               CalcAppMessage* message,
                               const size_t    size)
{
   if(process->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppKeepAliveAck for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }

   process->Status                         = PS_Processing;
   process->KeepAliveTimeoutTimeStamp      = ~0ULL;
   process->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
}


// ###### Handle incoming CalcAppCompleted message ##########################
void handleCalcAppCompleted(struct Process* process,
                            CalcAppMessage* message,
                            const size_t    size)
{
   if(process->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppCompleted for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }

   cout << "Job " << process->JobID << " completed" << endl;
   process->Status = PS_Completed;
}


// ###### Handle KeepAlive Transmission timer ###############################
void handleKeepAliveTransmissionTimer(struct Process* process)
{
   process->KeepAliveTransmissionTimeStamp = ~0ULL;
   process->KeepAliveTimeoutTimeStamp      = getMicroTime() + KeepAliveTimeoutInterval;
   sendCalcAppKeepAlive(process);
}


// ###### Handle KeepAlive Timeout timer ####################################
bool handleKeepAliveTimeoutTimer(struct Process* process)
{
   cout << "Timeout -> Failover" << endl;

   rspSessionFailover(process->Session);

   process->Status                         = PS_Processing;
   process->KeepAliveTimeoutTimeStamp      = ~0ULL;
   process->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;

   return(true);
}


// ###### Handle failover ###################################################
void handleFailover(Process* process)
{
   puts("===== FAILOVER ======");
   process->Status = PS_Processing;
}


// ###### Handle session events #############################################
void handleEvents(Process*           process,
                  const unsigned int sessionEvents)
{
   struct TagItem tags[16];
   char           buffer[4096];
   ssize_t        received;

   tags[0].Tag  = TAG_RspIO_Timeout;
   tags[0].Data = 0;
   tags[1].Tag  = TAG_DONE;
   received = rspSessionRead(process->Session, (char*)&buffer, sizeof(buffer),
                             (struct TagItem*)&tags);
   if(received > 0) {

      if(received >= (ssize_t)sizeof(CalcAppMessage)) {
         CalcAppMessage* response = (CalcAppMessage*)&buffer;
         switch(ntohl(response->Type)) {
            case CALCAPP_ACCEPT:
               handleCalcAppAccept(process, response, sizeof(response));
             break;
            case CALCAPP_REJECT:
               handleCalcAppReject(process, response, sizeof(response));
             break;
            case CALCAPP_KEEPALIVE:
               handleCalcAppKeepAlive(process, response, sizeof(response));
             break;
            case CALCAPP_KEEPALIVE_ACK:
               handleCalcAppKeepAliveAck(process, response, sizeof(response));
             break;
            case CALCAPP_COMPLETE:
               handleCalcAppCompleted(process, response, sizeof(response));
             break;
            default:
               cerr << "ERROR: Unknown message type " << ntohl(response->Type) << endl;
             break;
         }
      }
   }
}


// ###### Handle timers #####################################################
void handleTimer(Process* process)
{
   unsigned long long now = getMicroTime();

   if(process->KeepAliveTransmissionTimeStamp <= now) {
      handleKeepAliveTransmissionTimer(process);
      process->KeepAliveTransmissionTimeStamp = ~0ULL;
   }

//   if(!process->Closing) {
      if(process->KeepAliveTimeoutTimeStamp <= now) {
         handleKeepAliveTimeoutTimer(process);
         process->KeepAliveTimeoutTimeStamp = ~0ULL;
      }
//   }

}


// ###### Run one job #######################################################
void runJob(const char* poolHandle, const double jobSize)
{
   struct TagItem tags[16];
   uint32_t       jobID = 0;

   tags[0].Tag  = TAG_RspIO_MakeFailover;
   tags[0].Data = 1;
   tags[1].Tag  = TAG_DONE;

   Process process;
   process.Status                         = PS_Init;
   process.JobID                          = ++jobID;
   process.JobSize                        = jobSize;
   process.KeepAliveTransmissionTimeStamp = ~0ULL;
   process.KeepAliveTimeoutTimeStamp      = ~0ULL;
   process.Session = rspCreateSession((unsigned char*)poolHandle, strlen(poolHandle),
                                      NULL, (struct TagItem*)&tags);
   if(process.Session != NULL) {

      sendCalcAppRequest(&process);

      while((process.Status != PS_Failed) &&
            (process.Status != PS_Completed)) {

         /* ====== Call rspSessionSelect() =============================== */
         tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
         tags[0].Data = 1;
         tags[1].Tag  = TAG_DONE;
         struct SessionDescriptor* sessionDescriptorArray[1];
         unsigned int              sessionStatusArray[1];
         sessionDescriptorArray[0] = process.Session;
         sessionStatusArray[0]     = RspSelect_Read;
         unsigned long long now    = getMicroTime();
         unsigned long long nextTimer = now + 500000;
         nextTimer = min(nextTimer, process.KeepAliveTransmissionTimeStamp);
         nextTimer = min(nextTimer, process.KeepAliveTimeoutTimeStamp);
         unsigned long long timeout = (nextTimer >= now) ? (nextTimer - now) : 1;
         int result = rspSessionSelect((struct SessionDescriptor**)&sessionDescriptorArray,
                                       (unsigned int*)&sessionStatusArray,
                                       1,
                                       NULL, NULL, 0,
                                       timeout,
                                       (struct TagItem*)&tags);
         handleTimer(&process);

         /* ====== Handle results of ext_select() =========================== */
         if((result > 0) && (sessionStatusArray[0] & RspSelect_Read)) {
            handleEvents(&process, sessionStatusArray[0]);
         }

         if(result < 0) {
            if(errno != EINTR) {
               perror("rspSessionSelect() failed");
            }
            break;
         }

         if(process.Status == PS_Failover) {
            handleFailover(&process);
         }
      }

      rspDeleteSession(process.Session, NULL);
   }
}


// ###### Main program ######################################################
int main(int argc, char** argv)
{
   char*          poolHandle = "CalcAppPool";
   struct TagItem tags[16];
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


   cout << "CalcApp Pool User - Version 1.0" << endl;
   cout << "-------------------------------" << endl;


   runJob(poolHandle, 10000000.0);


   finishLogging();
   rspCleanUp();
   puts("\nTerminated!");
   return(0);
}
