/*
 * calcapppoolelement.cc
 *
 * Copyright (C) 2005 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <complex>

#include "rsplib.h"
#include "rsplib-tags.h"
#include "loglevel.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "breakdetector.h"

#include "calcapppackets.h"


using namespace std;


class SessionSet
{
   public:
   SessionSet();
   ~SessionSet();

   bool addSession(SessionDescriptor* session);
   void removeSession(SessionDescriptor* session);
   void removeClosed();
   void removeAll();
   size_t getSessions();
   unsigned long long initSelectArray(SessionDescriptor** sdArray,
                                      unsigned int*       statusArray,
                                      const unsigned int  status);
   void handleTimers();
   void handleEvents(SessionDescriptor* session,
                     const unsigned int sessionEvents);

   private:
   unsigned long long StartupTimeStamp;
   double             TotalUsedCalculations;
   unsigned long long KeepAliveTimeoutInterval;
   unsigned long long KeepAliveTransmissionInterval;
   unsigned long long CookieMaxTime;
   double             CookieMaxCalculations;

   struct SessionSetEntry {
      SessionSetEntry*   Next;
      SessionDescriptor* Session;

      // ------ Variables ---------------------------------------------------
      bool               HasJob;
      bool               Closing;
      unsigned int       JobID;
      double             JobSize;
      double             Completed;
      unsigned long long LastUpdateAt;
      unsigned long long LastCookieAt;
      double             LastCookieCompleted;

      // ------ Timers ------------------------------------------------------
      unsigned long long KeepAliveTransmissionTimeStamp;
      unsigned long long KeepAliveTimeoutTimeStamp;
      unsigned long long JobCompleteTimeStamp;
      unsigned long long CookieTransmissionTimeStamp;
   };

   SessionSetEntry* find(SessionDescriptor* session);

   void sendCalcAppAccept(SessionSetEntry* sessionSetEntry);
   void sendCalcAppReject(SessionSetEntry* sessionSetEntry);
   void Cookie(SessionSetEntry* sessionSetEntry);
   void sendCalcAppComplete(SessionSetEntry* sessionSetEntry);
   void sendCalcAppKeepAlive(SessionSetEntry* sessionSetEntry);
   void sendCalcAppKeepAliveAck(SessionSetEntry* sessionSetEntry);
   void sendCalcAppAbort(SessionSetEntry* sessionSetEntry);

   void handleCookieTransmissionTimer(SessionSetEntry* sessionSetEntry);
   void handleJobCompleteTimer(SessionSetEntry* sessionSetEntry);
   void handleKeepAliveTransmissionTimer(SessionSetEntry* sessionSetEntry);
   void handleKeepAliveTimeoutTimer(SessionSetEntry* sessionSetEntry);
   void handleCalcAppRequest(SessionSetEntry* sessionSetEntry,
                             CalcAppMessage*  message,
                             const size_t     size);
   void handleCalcAppKeepAlive(SessionSetEntry* sessionSetEntry,
                               CalcAppMessage*  message,
                               const size_t     size);
   void handleCalcAppKeepAliveAck(SessionSetEntry* sessionSetEntry,
                                  CalcAppMessage*  message,
                                  const size_t     size);
   void handleCookieEcho(SessionSetEntry* sessionSetEntry,
                         CalcAppCookie*   cookie,
                         const size_t     size);
   void handleMessage(SessionSetEntry* sessionSetEntry,
                      CalcAppMessage*  message,
                      const size_t     size);
   void handleTimer(SessionSetEntry* sessionSetEntry);

   size_t getActiveSessions();
   void updateCalculations();
   void scheduleJobs();


   SessionSetEntry* FirstSession;
   size_t           Sessions;
   double           Capacity;
};


// ###### Constructor #######################################################
SessionSet::SessionSet()
{
   FirstSession                  = NULL;
   Sessions                      = 0;

   StartupTimeStamp              = getMicroTime();
   TotalUsedCalculations         = 0.0;
   Capacity                      = 1000000.0;
   KeepAliveTimeoutInterval      = 2000000;
   KeepAliveTransmissionInterval = 2000000;
   CookieMaxTime                 = 1000000;
   CookieMaxCalculations         = 5000000.0;
}


// ###### Destructor ########################################################
SessionSet::~SessionSet()
{
   removeAll();

   const unsigned long long shutdownTimeStamp = getMicroTime();
   const unsigned long long serverRuntime     = shutdownTimeStamp - StartupTimeStamp;

   const double availableCalculations = serverRuntime * Capacity / 1000000.0;
   const double utilization           = TotalUsedCalculations / availableCalculations;

   printf("Runtime                = %1.2fs\n",  serverRuntime / 1000000.0);
   printf("Available Calculations = %1.1f\n",   availableCalculations);
   printf("Used Calculations      = %1.1f\n",   TotalUsedCalculations);
   printf("Utilization            = %1.3f%%\n", 100.0 * utilization);
}


// ###### Remove all sessions ###############################################
void SessionSet::removeAll()
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      removeSession(sessionSetEntry->Session);
      sessionSetEntry = FirstSession;
   }
   CHECK(Sessions == 0);
}


// ###### Remove closed sessions ############################################
/*
   During handleEvents() and handleTimers(), sessions may *not* be removed
   directly; their structures may still be accessed. Instead, they can be
   marked to be "closing" by setting sessionSetEntry->Closing = true.
   A call of removeClosed() will remove them finally, after event and
   timer handling.
*/
void SessionSet::removeClosed()
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      SessionSetEntry* nextSessionSetEntry = sessionSetEntry->Next;
      if(sessionSetEntry->Closing) {
         removeSession(sessionSetEntry->Session);
      }
      sessionSetEntry = nextSessionSetEntry;
   }
}


// ###### Add session #######################################################
bool SessionSet::addSession(SessionDescriptor* session)
{
   bool result = false;

   updateCalculations();
   SessionSetEntry* sessionSetEntry = new SessionSetEntry;
   if(sessionSetEntry) {
      sessionSetEntry->Next                = FirstSession;
      sessionSetEntry->Session             = session;
      sessionSetEntry->HasJob              = false;
      sessionSetEntry->Closing             = false;
      sessionSetEntry->JobID               = 0;
      sessionSetEntry->JobSize             = 0;
      sessionSetEntry->Completed           = 0;
      sessionSetEntry->LastUpdateAt        = 0;
      sessionSetEntry->LastCookieAt        = 0;
      sessionSetEntry->LastCookieCompleted = 0.0;

      sessionSetEntry->KeepAliveTransmissionTimeStamp = ~0ULL;
      sessionSetEntry->KeepAliveTimeoutTimeStamp      = ~0ULL;
      sessionSetEntry->JobCompleteTimeStamp           = ~0ULL;
      sessionSetEntry->CookieTransmissionTimeStamp    = ~0ULL;

      FirstSession                = sessionSetEntry;
      Sessions++;
      result = true;
   }
   scheduleJobs();

   return(result);
}


// ###### Remove session ####################################################
/*
   Never call this function during handleEvents() and handleTimers()!
   Instead, set sessionSetEntry->Closing = true.and call removeClosed() after
   event and timer handling.
*/
void SessionSet::removeSession(SessionDescriptor* session)
{
   updateCalculations();
   SessionSetEntry* sessionSetEntry = FirstSession;
   SessionSetEntry* prev  = NULL;
   while(sessionSetEntry != NULL) {
      if(sessionSetEntry->Session == session) {
         if(prev == NULL) {
            FirstSession = sessionSetEntry->Next;
         }
         else {
            prev->Next = sessionSetEntry->Next;
         }
         rspDeleteSession(sessionSetEntry->Session);
         sessionSetEntry->Session = NULL;
         delete sessionSetEntry;
         CHECK(Sessions > 0);
         Sessions--;
         break;
      }
      prev  = sessionSetEntry;
      sessionSetEntry = sessionSetEntry->Next;
   }
   scheduleJobs();
}


// ###### Find session ######################################################
SessionSet::SessionSetEntry* SessionSet::find(SessionDescriptor* session)
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      if(sessionSetEntry->Session == session) {
         return(sessionSetEntry);
      }
      sessionSetEntry = sessionSetEntry->Next;
   }
   return(NULL);
}


// ###### Get number of sessions ############################################
size_t SessionSet::getSessions()
{
   return(Sessions);
}


// ###### Initialize session select array ###################################
unsigned long long SessionSet::initSelectArray(SessionDescriptor** sdArray,
                                               unsigned int*       statusArray,
                                               const unsigned int  status)
{
   const size_t       count = getSessions();
   unsigned long long nextTimerTimeStamp = ~0ULL;
   size_t             i;

   SessionSetEntry* sessionSetEntry = FirstSession;
   for(i = 0;i < count;i++) {
      CHECK(sessionSetEntry != NULL);
      sdArray[i]      = sessionSetEntry->Session;
      statusArray[i]  = status;

      nextTimerTimeStamp = min(nextTimerTimeStamp, sessionSetEntry->JobCompleteTimeStamp);
      nextTimerTimeStamp = min(nextTimerTimeStamp, sessionSetEntry->KeepAliveTransmissionTimeStamp);
      nextTimerTimeStamp = min(nextTimerTimeStamp, sessionSetEntry->KeepAliveTimeoutTimeStamp);
      nextTimerTimeStamp = min(nextTimerTimeStamp, sessionSetEntry->CookieTransmissionTimeStamp);

      sessionSetEntry = sessionSetEntry->Next;
   }

   return(nextTimerTimeStamp);
}


// ###### Handle incoming CalcAppRequest ####################################
void SessionSet::handleCalcAppRequest(SessionSetEntry* sessionSetEntry,
                                      CalcAppMessage*  message,
                                      const size_t     received)
{
   if(sessionSetEntry->HasJob) {
      cerr << "ERROR: CalcAppRequest for session that already has a job!" << endl;
      sessionSetEntry->Closing = true;
      return;
   }

   sessionSetEntry->HasJob     = true;
   sessionSetEntry->JobID      = message->JobID;
   sessionSetEntry->JobSize    = message->JobSize;
   sessionSetEntry->Completed  = 0;
   sessionSetEntry->LastUpdateAt = getMicroTime();

   cout << "Job " << sessionSetEntry->JobID << " with size "
        << sessionSetEntry->JobSize << " accepted" << endl;

   sessionSetEntry->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
   sessionSetEntry->KeepAliveTimeoutTimeStamp      = ~0ULL;

   updateCalculations();
   scheduleJobs();

   sendCalcAppAccept(sessionSetEntry);
}


// ###### Handle incoming CookieEcho ########################################
void SessionSet::handleCookieEcho(SessionSetEntry* sessionSetEntry,
                                  CalcAppCookie*   cookie,
                                  const size_t     size)
{
   if(sessionSetEntry->HasJob) {
      cerr << "ERROR: CookieEcho for session that already has a job!" << endl;
      sessionSetEntry->Closing = true;
      return;
   }

   sessionSetEntry->HasJob       = true;
   sessionSetEntry->JobID        = ntohl(cookie->JobID);
   sessionSetEntry->JobSize      = cookie->JobSize;
   sessionSetEntry->Completed    = cookie->Completed;
   sessionSetEntry->LastUpdateAt = getMicroTime();

   cout << "Job " << sessionSetEntry->JobID << " with size "
        << sessionSetEntry->JobSize << " ("
        << sessionSetEntry->Completed << " completed) accepted from CookieEcho" << endl;

   sessionSetEntry->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
   sessionSetEntry->KeepAliveTimeoutTimeStamp      = ~0ULL;

   updateCalculations();
   scheduleJobs();

   sendCalcAppAccept(sessionSetEntry);
}


// ###### Handle incoming CalcAppAccept #####################################
void SessionSet::sendCalcAppAccept(SessionSetEntry* sessionSetEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type  = htonl(CALCAPP_ACCEPT);
   message.JobID = htonl(sessionSetEntry->JobID);
   if(rspSessionWrite(sessionSetEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppAccept" << endl;
      sessionSetEntry->Closing = true;
   }
}


// ###### Handle incoming CalcAppReject #####################################
void SessionSet::sendCalcAppReject(SessionSetEntry* sessionSetEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type = htonl(CALCAPP_REJECT);
   if(rspSessionWrite(sessionSetEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppReject" << endl;
      sessionSetEntry->Closing = true;
   }
}


// ###### Handle Cookie Transmission timer ##################################
void SessionSet::handleCookieTransmissionTimer(SessionSetEntry* sessionSetEntry)
{
   updateCalculations();

   struct CalcAppCookie cookie;
   memset(&cookie, 0, sizeof(cookie));
   cookie.JobID     = htonl(sessionSetEntry->JobID);
   cookie.JobSize   = sessionSetEntry->JobSize;
   cookie.Completed = sessionSetEntry->Completed;
   if(rspSessionSendCookie(sessionSetEntry->Session,
                           (unsigned char*)&cookie,
                           sizeof(cookie),
                           NULL) == false) {
      cerr << "ERROR: Unable to send Cookie" << endl;
      sessionSetEntry->Closing = true;
   }

   sessionSetEntry->LastCookieAt        = getMicroTime();
   sessionSetEntry->LastCookieCompleted = sessionSetEntry->Completed;

   scheduleJobs();
}


// ###### Send CalcAppKeepAlive #############################################
void SessionSet::sendCalcAppKeepAlive(SessionSetEntry* sessionSetEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type      = htonl(CALCAPP_KEEPALIVE);
   message.JobID     = htonl(sessionSetEntry->JobID);
   if(rspSessionWrite(sessionSetEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppKeepAlive" << endl;
      sessionSetEntry->Closing = true;
   }
}


// ###### Send CalcAppKeepAlive #############################################
void SessionSet::sendCalcAppKeepAliveAck(SessionSetEntry* sessionSetEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type      = htonl(CALCAPP_KEEPALIVE_ACK);
   message.JobID     = htonl(sessionSetEntry->JobID);
   if(rspSessionWrite(sessionSetEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppKeepAliveAck" << endl;
      sessionSetEntry->Closing = true;
   }
}


// ###### Handle JobComplete timer ##########################################
void SessionSet::handleJobCompleteTimer(SessionSetEntry* sessionSetEntry)
{
   CHECK(sessionSetEntry->HasJob);
   cout << "Job " << sessionSetEntry->JobID << " is complete!" << endl;
   sendCalcAppComplete(sessionSetEntry);
   sessionSetEntry->Closing = true;
}


// ###### Handle KeepAlive Transmission timer ###############################
void SessionSet::handleKeepAliveTransmissionTimer(SessionSetEntry* sessionSetEntry)
{
   CHECK(sessionSetEntry->HasJob);
   sessionSetEntry->KeepAliveTransmissionTimeStamp = ~0ULL;
   sessionSetEntry->KeepAliveTimeoutTimeStamp      = getMicroTime() + KeepAliveTimeoutInterval;
   sendCalcAppKeepAlive(sessionSetEntry);
}


// ###### Handle KeepAlive Timeout timer ####################################
void SessionSet::handleKeepAliveTimeoutTimer(SessionSetEntry* sessionSetEntry)
{
   CHECK(sessionSetEntry->HasJob);
   sessionSetEntry->KeepAliveTimeoutTimeStamp = ~0ULL;
   cout << "No keep-alive for job " << sessionSetEntry->JobID
        << " -> removing session" << endl;
   sessionSetEntry->Closing = true;
}


// ###### Handle incoming CalcAppKeepAlive ##################################
void SessionSet::handleCalcAppKeepAlive(SessionSetEntry* sessionSetEntry,
                                        CalcAppMessage*  message,
                                        const size_t     size)
{
   if(!sessionSetEntry->HasJob) {
      cerr << "ERROR: CalcAppKeepAlive without job!" << endl;
      sessionSetEntry->Closing = true;
      return;
   }
   if(sessionSetEntry->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppKeepAlive for wrong job!" << endl;
      sessionSetEntry->Closing = true;
      return;
   }
   sendCalcAppKeepAliveAck(sessionSetEntry);
}


// ###### Handle incoming CalcAppKeepAliveAck ###############################
void SessionSet::handleCalcAppKeepAliveAck(SessionSetEntry* sessionSetEntry,
                                           CalcAppMessage*  message,
                                           const size_t     size)
{
   if(!sessionSetEntry->HasJob) {
      cerr << "ERROR: CalcAppKeepAliveAck without job!" << endl;
      sessionSetEntry->Closing = true;
      return;
   }
   if(sessionSetEntry->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppKeepAliveAck for wrong job!" << endl;
      sessionSetEntry->Closing = true;
      return;
   }

   sessionSetEntry->KeepAliveTimeoutTimeStamp      = ~0ULL;
   sessionSetEntry->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
}


// ###### Send CalcAppComplete ##############################################
void SessionSet::sendCalcAppComplete(SessionSetEntry* sessionSetEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type      = htonl(CALCAPP_COMPLETE);
   message.JobID     = htonl(sessionSetEntry->JobID);
   message.JobSize   = sessionSetEntry->JobSize;
   message.Completed = sessionSetEntry->Completed;
   if(rspSessionWrite(sessionSetEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppComplete" << endl;
      sessionSetEntry->Closing = true;
   }
}


// ###### Handle timers #####################################################
void SessionSet::handleTimer(SessionSetEntry* sessionSetEntry)
{
   unsigned long long now = getMicroTime();

   if(sessionSetEntry->JobCompleteTimeStamp <= now) {
      handleJobCompleteTimer(sessionSetEntry);
      sessionSetEntry->JobCompleteTimeStamp = ~0ULL;
   }
   if(!sessionSetEntry->Closing) {
      if(sessionSetEntry->CookieTransmissionTimeStamp <= now) {
         handleCookieTransmissionTimer(sessionSetEntry);
      }
      if(!sessionSetEntry->Closing) {
         if(sessionSetEntry->KeepAliveTransmissionTimeStamp <= now) {
            handleKeepAliveTransmissionTimer(sessionSetEntry);
            sessionSetEntry->KeepAliveTransmissionTimeStamp = ~0ULL;
         }
         if(!sessionSetEntry->Closing) {
            if(sessionSetEntry->KeepAliveTimeoutTimeStamp <= now) {
               handleKeepAliveTimeoutTimer(sessionSetEntry);
               sessionSetEntry->KeepAliveTimeoutTimeStamp = ~0ULL;
            }
         }
      }
   }
}


// ###### Handle timers #####################################################
void SessionSet::handleTimers()
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      // Iterate all session entries and
      // invoke handleTimer() for each session.
      handleTimer(sessionSetEntry);
      sessionSetEntry = sessionSetEntry->Next;
   }
}


// ###### Handle incoming message from session ##############################
void SessionSet::handleMessage(SessionSetEntry* sessionSetEntry,
                               CalcAppMessage*  message,
                               const size_t     received)
{
   if(received >= sizeof(CalcAppMessage)) {
      switch(ntohl(message->Type)) {
         case CALCAPP_REQUEST:
            handleCalcAppRequest(sessionSetEntry, message, received);
          break;
         case CALCAPP_KEEPALIVE:
           handleCalcAppKeepAlive(sessionSetEntry, message, received);
          break;
         case CALCAPP_KEEPALIVE_ACK:
           handleCalcAppKeepAliveAck(sessionSetEntry, message, received);
          break;
         default:
           cerr << "ERROR: Unexpected message type " << ntohl(message->Type) << endl;
           sessionSetEntry->Closing = true;
          break;
      }
   }
}


// ###### Handle socket events, i.e. read incoming messages #################
void SessionSet::handleEvents(SessionDescriptor* session,
                              const unsigned int sessionEvents)
{
   char           buffer[4096];
   struct TagItem tags[16];
   ssize_t        received;

   SessionSetEntry* sessionSetEntry = find(session);
   CHECK(sessionSetEntry != NULL);

   if(!sessionSetEntry->Closing) {
      tags[0].Tag  = TAG_RspIO_MsgIsCookieEcho;
      tags[0].Data = 0;
      tags[1].Tag  = TAG_RspIO_Timeout;
      tags[1].Data = 0;
      tags[2].Tag  = TAG_DONE;
      received = rspSessionRead(sessionSetEntry->Session,
                                (char*)&buffer, sizeof(buffer),
                                (struct TagItem*)&tags);
      if(received > 0) {
         if(tags[0].Data != 0) {
            if(received >= (ssize_t)sizeof(CalcAppCookie)) {
               handleCookieEcho(sessionSetEntry, (struct CalcAppCookie*)&buffer, received);
            }
            else {
               cerr << "ERROR: Received cookie is too short!" << endl;
               sessionSetEntry->Closing = true;
            }
         }
         else {
            if(received >= (ssize_t)sizeof(CalcAppMessage)) {
               handleMessage(sessionSetEntry, (struct CalcAppMessage*)&buffer, received);
            }
            else {
               cerr << "ERROR: Received message is too short!" << endl;
               sessionSetEntry->Closing = true;
            }
         }
      }
   }
}


// ###### Get number of active sessions #####################################
size_t SessionSet::getActiveSessions()
{
   size_t           activeSessions  = 0;
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      if(sessionSetEntry->HasJob) {
         activeSessions++;
      }
      sessionSetEntry = sessionSetEntry->Next;
   }
   return(activeSessions);
}


// ###### Update calculations until now for all sessions ####################
void SessionSet::updateCalculations()
{
   const unsigned long long now     = getMicroTime();
   SessionSetEntry* sessionSetEntry = FirstSession;
   const size_t     activeSessions  = getActiveSessions();

   if(activeSessions > 0) {
      const double capacityPerJob = Capacity / ((double)activeSessions * 1000000.0);

      while(sessionSetEntry != NULL) {
         if(sessionSetEntry->HasJob) {
            const unsigned long long elapsed   = now - sessionSetEntry->LastUpdateAt;
            const double completedCalculations = elapsed * capacityPerJob;
            if(sessionSetEntry->Completed + completedCalculations < sessionSetEntry->JobSize) {
               TotalUsedCalculations += completedCalculations;
               sessionSetEntry->Completed += completedCalculations;
            }
            else {
               CHECK(sessionSetEntry->JobSize - sessionSetEntry->Completed >= 0.0);
               TotalUsedCalculations += sessionSetEntry->JobSize - sessionSetEntry->Completed;
               sessionSetEntry->Completed = sessionSetEntry->JobSize;
            }
         }
         sessionSetEntry->LastUpdateAt = now;
         sessionSetEntry = sessionSetEntry->Next;
      }
   }
}


// ###### Schedule requests of all sessions #################################
void SessionSet::scheduleJobs()
{
   const unsigned long long now     = getMicroTime();
   SessionSetEntry* sessionSetEntry = FirstSession;
   const size_t     activeSessions  = getActiveSessions();

   if(activeSessions > 0) {
      const double capacityPerJob = Capacity / ((double)activeSessions * 1000000.0);
      while(sessionSetEntry != NULL) {
         if(sessionSetEntry->HasJob) {
            // When is the current session's job completed?
            const double calculationsToGo         = sessionSetEntry->JobSize - sessionSetEntry->Completed;
            const unsigned long long timeToGo     = (unsigned long long)ceil(calculationsToGo / capacityPerJob);

            // sessionSetEntry->JobCompleteTimeStamp = now + timeToGo;
            sessionSetEntry->JobCompleteTimeStamp = sessionSetEntry->LastUpdateAt + timeToGo;

            // When is the time to send the next cookie?
            const double calculationsSinceLastCookie = sessionSetEntry->Completed - sessionSetEntry->LastCookieCompleted;
            const double timeSinceLastCookie         = now - sessionSetEntry->LastCookieAt;
            const unsigned long long nextByCalculations =
               (unsigned long long)rint(max(0.0, CookieMaxCalculations - calculationsSinceLastCookie) / capacityPerJob);
            const unsigned long long nextByTime =
               (unsigned long long)rint(max(0.0, CookieMaxTime - timeSinceLastCookie));
            const unsigned long long nextCookie = min(nextByCalculations, nextByTime);
            sessionSetEntry->CookieTransmissionTimeStamp = now + nextCookie;
         }
         sessionSetEntry = sessionSetEntry->Next;
      }
   }
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   uint32_t                      identifier                     = 0;
   unsigned int                  reregInterval                  = 5000;
   card64                        start                          = getMicroTime();
   card64                        stop                           = 0;
   int                           type                           = SOCK_STREAM;
   int                           protocol                       = IPPROTO_SCTP;
   unsigned short                port                           = 0;
   char*                         poolHandle                     = "CalcAppPool";
   unsigned int                  policyType                     = PPT_RANDOM;
   unsigned int                  policyParameterWeight          = 1;
   unsigned int                  policyParameterLoad            = 0;
   unsigned int                  policyParameterLoadDegradation = 0;
   struct PoolElementDescriptor* poolElement;
   struct TagItem                tags[16];
   struct PoolElementDescriptor* pedArray[1];
   unsigned int                  pedStatusArray[FD_SETSIZE];
   int                           i;
   int                           result;

   start = getMicroTime();
   stop  = 0;
   for(i = 1;i < argc;i++) {
      if(!(strcmp(argv[i], "-sctp"))) {
         protocol = IPPROTO_SCTP;
      }
      else if(!(strncmp(argv[i], "-stop=" ,6))) {
         stop = start + ((card64)1000000 * (card64)atol((char*)&argv[i][6]));
      }
      else if(!(strncmp(argv[i], "-port=" ,6))) {
         port = atol((char*)&argv[i][6]);
      }
      else if(!(strncmp(argv[i], "-ph=" ,4))) {
         poolHandle = (char*)&argv[i][4];
      }
      else if(!(strncmp(argv[i], "-rereginterval=" ,15))) {
         reregInterval = atol((char*)&argv[i][15]);
         if(reregInterval < 250) {
            reregInterval = 250;
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
         cerr << "Bad argument \"" << argv[i] << "\"!"  << endl;
         cerr << "Usage: " << argv[0]
              << " {-registrar=Registrar address(es)} {-ph=Pool Handle} {-sctp} {-port=local port} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} "
              << endl;
         exit(1);
      }
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      cerr << "ERROR: Unable to initialize rsplib!" << endl;
      finishLogging();
      exit(1);
   }

   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-registrar=" ,11))) {
         if(rspAddStaticRegistrar((char*)&argv[i][11]) != RSPERR_OKAY) {
            cerr << "ERROR: Bad registrar setting: "
                 << argv[i] << "!" << endl;
            exit(1);
         }
      }
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
   tags[7].Data = reregInterval;
   tags[8].Tag  = TAG_PoolElement_RegistrationLife;
   tags[8].Data = (3 * 45000) + 5000;
   tags[9].Tag  = TAG_PoolElement_Identifier;
   tags[9].Data = identifier;
   tags[10].Tag  = TAG_UserTransport_HasControlChannel;
   tags[10].Data = (tagdata_t)true;
   tags[11].Tag  = TAG_END;

   poolElement = rspCreatePoolElement((unsigned char*)poolHandle, strlen(poolHandle), tags);
   if(poolElement != NULL) {
      cout << "CalcApp Pool Element - Version 1.0" << endl
           << "----------------------------------" << endl << endl;

#ifndef FAST_BREAK
      installBreakDetector();
#endif

      SessionSet sessionList;
      while(!breakDetected()) {
         /* ====== Call rspSessionSelect() =============================== */
         tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
         tags[0].Data = 1;  // !!!!!!
         tags[1].Tag  = TAG_DONE;
         pedArray[0]       = poolElement;
         pedStatusArray[0] = RspSelect_Read;
         const size_t sessions = sessionList.getSessions();
         struct SessionDescriptor* sessionDescriptorArray[sessions];
         unsigned int              sessionStatusArray[sessions];
         unsigned long long nextTimer = sessionList.initSelectArray(
                                           (struct SessionDescriptor**)&sessionDescriptorArray,
                                           (unsigned int*)&sessionStatusArray,
                                           RspSelect_Read);
         unsigned long long now       = getMicroTime();
         nextTimer = min(now + 500000, nextTimer);
         unsigned long long timeout = (nextTimer >= now) ? (nextTimer - now) : 1;
//printf("NEXT: %Ld\n",timeout);
         result = rspSessionSelect((struct SessionDescriptor**)&sessionDescriptorArray,
                                   (unsigned int*)&sessionStatusArray,
                                   sessions,
                                   (struct PoolElementDescriptor**)&pedArray,
                                   (unsigned int*)&pedStatusArray,
                                   1,
                                   timeout,
                                   (struct TagItem*)&tags);
         sessionList.handleTimers();

         /* ====== Handle results of ext_select() =========================== */
         if(result > 0) {
            if(pedStatusArray[0] & RspSelect_Read) {
               /*
               tags[0].Tag  = TAG_TuneSCTP_MinRTO;
               tags[0].Data = 200;
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
               tags[6].Tag  = TAG_UserTransport_HasControlChannel;
               tags[6].Data = 1;
               tags[7].Tag  = TAG_DONE;
               */
               tags[0].Tag  = TAG_DONE;
               struct SessionDescriptor* session = rspAcceptSession(pedArray[0], (struct TagItem*)&tags);
               if(session) {
                  if(!sessionList.addSession(session)) {
                     rspDeleteSession(session);
                  }
               }
               else {
                  cerr << "ERROR: Unable to accept new session!" << endl;
               }
            }
            for(size_t i = 0;i < sessions;i++) {
               if(sessionStatusArray[i] & RspSelect_Read) {
                  sessionList.handleEvents(sessionDescriptorArray[i],
                                           sessionStatusArray[i]);
               }
            }
         }

         if(result < 0) {
            if(errno != EINTR) {
               perror("rspSessionSelect() failed");
            }
            break;
         }

         sessionList.removeClosed();

         /* ====== Check auto-stop timer ==================================== */
         if((stop > 0) && (getMicroTime() >= stop)) {
            cerr << "Auto-stop time reached -> exiting!" << endl;
            break;
         }
      }

      cout << "Closing sessions..." << endl;
      sessionList.removeAll();
      cout << "Removing Pool Element..." << endl;
      rspDeletePoolElement(poolElement, NULL);
   }
   else {
      cerr << "ERROR: Unable to create pool element!" << endl;
      exit(1);
   }

   rspCleanUp();
   finishLogging();
   cout << endl << "Terminated!" << endl;
   return(0);
}

