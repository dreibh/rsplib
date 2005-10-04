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

#include "rserpool.h"
#include "loglevel.h"
#include "udplikeserver.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "rsputilities.h"
#include "breakdetector.h"
#include "randomizer.h"
#include "statistics.h"
#include "calcapppackets.h"

#include <iostream>


using namespace std;

#if 0
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

   inline double getTotalUsedCalculations() const
   {
    return TotalUsedCalculations;
   }
   private:
   unsigned long long StartupTimeStamp;
   double             TotalUsedCalculations;
   unsigned long long KeepAliveTimeoutInterval;
   unsigned long long KeepAliveTransmissionInterval;
   unsigned long long CookieMaxTime;
   double             CookieMaxCalculations;

   struct SessionSetEntry {
      SessionSetEntry*   Next;
      rserpool_session_t SessionID;

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
   void sendCalcAppReject(SessionSetEntry* sessionSetEntry, unsigned int jobID);
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

FILE*        VectorFH   = NULL;
FILE*        ScalarFH   = NULL;
unsigned int VectorLine = 0;
double       TotalUsedCapacity = 0.0;
double       TotalPossibleCalculations   = 0.0;
double       TotalWastedCapacity    = 0.0;
unsigned int AcceptedJobs      = 0;
unsigned int RejectedJobs      = 0;
unsigned int sessionlimit      = 2;

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
         cout << " Session removed "<< endl;
         cout << "Sessions after removing: " << Sessions << endl;
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
   if (Sessions > sessionlimit)
   {
   cerr << "ERROR: Too many sessions" << endl;
        cout << "Sessions: " << Sessions << endl;
   //Sessions--;
        sendCalcAppReject(sessionSetEntry,ntohl(message->JobID));
        sessionSetEntry->Closing = true;
        return;
   }

   sessionSetEntry->HasJob     = true;
   sessionSetEntry->JobID      = ntohl(message->JobID);
   sessionSetEntry->JobSize    = ntoh64(message->JobSize);
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
   if (Sessions > sessionlimit)
   {
   cerr << "ERROR: Too many sessions" << endl;
        cout << "Sessions: " << Sessions << endl;
   //Sessions--;
        sendCalcAppReject(sessionSetEntry,cookie->JobID);
        sessionSetEntry->Closing = true;
        return;
   }

   sessionSetEntry->HasJob       = true;
   sessionSetEntry->JobID        = ntohl(cookie->JobID);
   sessionSetEntry->JobSize      = ntoh64(cookie->JobSize);
   sessionSetEntry->Completed    = ntoh64(cookie->Completed);
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
   AcceptedJobs++;
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
void SessionSet::sendCalcAppReject(SessionSetEntry* sessionSetEntry, unsigned int jobID)
{

   RejectedJobs++;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type = htonl(CALCAPP_REJECT);
   message.JobID = htonl(jobID);
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
   cookie.JobSize   = hton64((unsigned long long)rint(sessionSetEntry->JobSize));
   cookie.Completed = hton64((unsigned long long)rint(sessionSetEntry->Completed));
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
   cout << "CalcAppCompleted" << endl;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type      = htonl(CALCAPP_COMPLETE);
   message.JobID     = htonl(sessionSetEntry->JobID);
   message.JobSize   = hton64((unsigned long long)rint(sessionSetEntry->JobSize));
   message.Completed = hton64((unsigned long long)rint(sessionSetEntry->Completed));
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
#endif




class CalcAppServer : public UDPLikeServer
{
   public:
   CalcAppServer(const size_t maxJobs,
                 const char*  vectorFileName,
                 const char*  scalarFileName);
   virtual ~CalcAppServer();

   protected:
   virtual EventHandlingResult initialize();
   virtual void finish(EventHandlingResult result);
   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID);
/*   virtual EventHandlingResult handleCookieEcho(rserpool_session_t sessionID,
                                                const char*        buffer,
                                                size_t             bufferSize);*/
   virtual void handleTimer();

   private:
   struct CalcAppServerJob {
      CalcAppServerJob*  Next;
      rserpool_session_t SessionID;
      uint32_t           JobID;

      // ------ Variables ---------------------------------------------------
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

   CalcAppServerJob* addJob(rserpool_session_t sessionID,
                            uint32_t           jobID,
                            double             jobSize,
                            double             completed);
   CalcAppServerJob* findJob(rserpool_session_t sessionID,
                             uint32_t           jobID);
   void removeJob(CalcAppServerJob* job);
   void updateCalculations();
   void scheduleJobs();
   void scheduleNextTimerEvent();

   void handleCalcAppRequest(rserpool_session_t    sessionID,
                             const CalcAppMessage* message,
                             const size_t          received);
   void sendCalcAppAccept(CalcAppServerJob* job);
   void sendCalcAppReject(rserpool_session_t sessionID, uint32_t jobID);
   void handleJobCompleteTimer(CalcAppServerJob* job);
   void sendCalcAppComplete(CalcAppServerJob* job);


   size_t             Jobs;
   CalcAppServerJob*  FirstJob;
   std::string        VectorFileName;
   std::string        ScalarFileName;
   FILE*              VectorFH;
   FILE*              ScalarFH;

   size_t             MaxJobs;
   double             Capacity;
   unsigned long long StartupTimeStamp;
   double             TotalUsedCalculations;
   unsigned long long KeepAliveTimeoutInterval;
   unsigned long long KeepAliveTransmissionInterval;
   unsigned long long CookieMaxTime;
   double             CookieMaxCalculations;
};


// ###### Constructor #######################################################
CalcAppServer::CalcAppServer(const size_t maxJobs,
                             const char*  vectorFileName,
                             const char*  scalarFileName)
{
   Jobs           = 0;
   FirstJob       = NULL;
   VectorFileName = vectorFileName;
   ScalarFileName = scalarFileName;
   VectorFH       = NULL;
   ScalarFH       = NULL;

   MaxJobs                       = maxJobs;
   TotalUsedCalculations         = 0.0;
   Capacity                      = 1000000.0;
   KeepAliveTimeoutInterval      = 2000000;
   KeepAliveTransmissionInterval = 2000000;
   CookieMaxTime                 = 1000000;
   CookieMaxCalculations         = 5000000.0;
   StartupTimeStamp              = getMicroTime();
}


// ###### Destructor ########################################################
CalcAppServer::~CalcAppServer()
{
}


// ###### Add job ###########################################################
CalcAppServer::CalcAppServerJob* CalcAppServer::addJob(rserpool_session_t sessionID,
                                                       uint32_t           jobID,
                                                       double             jobSize,
                                                       double             completed)
{
   if(Jobs < MaxJobs) {
      CalcAppServerJob* job = new CalcAppServerJob;
      if(job) {
         updateCalculations();
         job->Next                           = FirstJob;
         job->SessionID                      = sessionID;
         job->JobID                          = jobID;
         job->JobSize                        = jobSize;
         job->Completed                      = completed;
         job->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
         job->KeepAliveTimeoutTimeStamp      = ~0ULL;
         job->JobCompleteTimeStamp           = ~0ULL;
         job->CookieTransmissionTimeStamp    = ~0ULL;
         job->LastUpdateAt                   = getMicroTime();
         job->LastCookieAt                   = 0;
         job->LastCookieCompleted            = 0.0;
         FirstJob = job;
         Jobs++;

         cout << " Job added (" << Jobs << " are running)" << endl;
         scheduleJobs();
      }
      return(job);
   }
   return(NULL);
}


// ###### Find job ##########################################################
CalcAppServer::CalcAppServerJob* CalcAppServer::findJob(rserpool_session_t sessionID,
                                                        uint32_t           jobID)
{
   CalcAppServerJob* job = FirstJob;
   while(job != NULL) {
      if((job->SessionID == sessionID) && (job->JobID == jobID)) {
         return(job);
      }
      job = job->Next;
   }
   return(NULL);
}


// ###### Remove job ########################################################
void CalcAppServer::removeJob(CalcAppServer::CalcAppServerJob* removedJob)
{
   updateCalculations();
   CalcAppServerJob* job  = FirstJob;
   CalcAppServerJob* prev = NULL;
   while(job != NULL) {
      if(job == removedJob) {
         if(prev == NULL) {
            FirstJob = job->Next;
         }
         else {
            prev->Next = job->Next;
         }
         delete removedJob;
         CHECK(Jobs > 0);
         Jobs--;
         cout << " Job removed (" << Jobs << " still running)" << endl;
         break;
      }
      prev = job;
      job = job->Next;
   }
   scheduleJobs();
}


// ###### Update calculations until now for all sessions ####################
void CalcAppServer::updateCalculations()
{
   const unsigned long long now = getMicroTime();
   CalcAppServerJob*        job = FirstJob;

   if(Jobs > 0) {
      const double capacityPerJob = Capacity / ((double)Jobs * 1000000.0);

      while(job != NULL) {
         const unsigned long long elapsed   = now - job->LastUpdateAt;
         const double completedCalculations = elapsed * capacityPerJob;
         if(job->Completed + completedCalculations < job->JobSize) {
            TotalUsedCalculations += completedCalculations;
            job->Completed += completedCalculations;
         }
         else {
            CHECK(job->JobSize - job->Completed >= 0.0);
            TotalUsedCalculations += job->JobSize - job->Completed;
            job->Completed = job->JobSize;
         }
         job->LastUpdateAt = now;

         job = job->Next;
      }
   }
}


// ###### Schedule requests of all sessions #################################
void CalcAppServer::scheduleJobs()
{
   const unsigned long long now = getMicroTime();
   CalcAppServerJob*        job = FirstJob;

   if(Jobs > 0) {
      const double capacityPerJob = Capacity / ((double)Jobs * 1000000.0);
      while(job != NULL) {
         // When is the current session's job completed?
         const double calculationsToGo         = job->JobSize - job->Completed;
         const unsigned long long timeToGo     = (unsigned long long)ceil(calculationsToGo / capacityPerJob);

         // job->JobCompleteTimeStamp = now + timeToGo;
         job->JobCompleteTimeStamp = job->LastUpdateAt + timeToGo;

         // When is the time to send the next cookie?
         const double calculationsSinceLastCookie = job->Completed - job->LastCookieCompleted;
         const double timeSinceLastCookie         = now - job->LastCookieAt;
         const unsigned long long nextByCalculations =
            (unsigned long long)rint(max(0.0, CookieMaxCalculations - calculationsSinceLastCookie) / capacityPerJob);
         const unsigned long long nextByTime =
            (unsigned long long)rint(max(0.0, CookieMaxTime - timeSinceLastCookie));
         const unsigned long long nextCookie = min(nextByCalculations, nextByTime);
         job->CookieTransmissionTimeStamp = now + nextCookie;

         job = job->Next;
      }
   }
}


// ###### Schedule next timer event #########################################
void CalcAppServer::scheduleNextTimerEvent()
{
   unsigned long long nextTimerTimeStamp = ~0ULL;
   CalcAppServerJob*  job                = FirstJob;
   while(job != NULL) {
      nextTimerTimeStamp = min(nextTimerTimeStamp, job->JobCompleteTimeStamp);
      nextTimerTimeStamp = min(nextTimerTimeStamp, job->KeepAliveTransmissionTimeStamp);
      nextTimerTimeStamp = min(nextTimerTimeStamp, job->KeepAliveTimeoutTimeStamp);
      nextTimerTimeStamp = min(nextTimerTimeStamp, job->CookieTransmissionTimeStamp);
      job = job->Next;
   }
printf("%Lx\n",nextTimerTimeStamp);
   startTimer(nextTimerTimeStamp);
}


// ###### Initialize ########################################################
EventHandlingResult CalcAppServer::initialize()
{
   VectorFH = fopen(VectorFileName.c_str(), "w");
   if(VectorFH == NULL) {
      cout << " Unable to open output file " << VectorFileName << endl;
      return(EHR_Abort);
   }
   fprintf(VectorFH, "Runtime AvailableCalculations UsedCalculations Utilization\n");

   ScalarFH = fopen(ScalarFileName.c_str(), "w");
   if(ScalarFH == NULL) {
      cout << " Unable to open output file " << ScalarFileName << endl;
      return(EHR_Abort);
   }
   fprintf(ScalarFH, "run 1 \"scenario\"\n");
   return(EHR_Okay);
}


// ###### Shutdown ##########################################################
void CalcAppServer::finish(EventHandlingResult initializeResult)
{
/*
   double           Capacity                      = 1000000.0;
   unsigned long long        shutdownTimeStamp             = getMicroTime();
   const unsigned long long     serverRuntime     = shutdownTimeStamp - StartupTimeStamp;
   const double        availableCalculations = serverRuntime * Capacity / 1000000.0;
   const double        utilization           = sessionList.getTotalUsedCalculations() / availableCalculations;
   TotalUsedCapacity = sessionList.getTotalUsedCalculations();
   TotalPossibleCalculations = (serverRuntime/1000000.0)*Capacity;
   TotalWastedCapacity = serverRuntime-TotalUsedCapacity;
   fprintf(VectorFH," %u %1.6llu %1.6f %1.6f %1.6f\n", ++VectorLine, serverRuntime, availableCalculations, Capacity, utilization);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Used Capacity   \" %1.6f \n", objectName, TotalUsedCapacity);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Possible Calculations \" %1.6f \n", objectName, TotalPossibleCalculations);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Wasted Capacity  \" %1.6f \n", objectName, TotalWastedCapacity);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Jobs Accepted   \" %u    \n", objectName, AcceptedJobs);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Jobs Rejected   \" %u    \n", objectName, RejectedJobs);
   fprintf(ScalarFH, "scalar \"%s\" \"Utilization           \" %1.6f \n", objectName, utilization);
*/
   if(VectorFH) {
      fclose(VectorFH);
      VectorFH = NULL;
   }
   if(ScalarFH) {
      fclose(ScalarFH);
      ScalarFH = NULL;
   }
}










// ###### Handle incoming CalcAppRequest ####################################
void CalcAppServer::handleCalcAppRequest(rserpool_session_t    sessionID,
                                         const CalcAppMessage* message,
                                         const size_t          received)
{
   CalcAppServerJob* job = addJob(sessionID, ntohl(message->JobID),
                                  (double)ntoh64(message->JobSize), 0.0);
   if(job) {
      cout << "Job " << job->SessionID << "/" << job->JobID
           << " of size " << job->JobSize << " accepted" << endl;
      sendCalcAppAccept(job);
   }
   else {
      cout << "Job " << job->SessionID << "/" << job->JobID << " rejected!" << endl;
      sendCalcAppReject(sessionID, ntohl(message->JobID));
   }
}


// ###### Send CalcAppAccept ################################################
void CalcAppServer::sendCalcAppAccept(CalcAppServer::CalcAppServerJob* job)
{
//    AcceptedJobs++;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type  = htonl(CALCAPP_ACCEPT);
   message.JobID = htonl(job->JobID);
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  job->SessionID, PPID_CALCAPP, 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppAccept");
      removeJob(job);
   }
}


// ###### Send CalcAppReject ################################################
void CalcAppServer::sendCalcAppReject(rserpool_session_t sessionID, uint32_t jobID)
{
//    RejectedJobs++;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type  = htonl(CALCAPP_ACCEPT);
   message.JobID = htonl(jobID);
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  sessionID, PPID_CALCAPP, 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppReject");
   }
}


// ###### Handle JobComplete timer ##########################################
void CalcAppServer::handleJobCompleteTimer(CalcAppServer::CalcAppServerJob* job)
{
   cout << "Job " << job->SessionID << "/" << job->JobID << " is complete!" << endl;
   sendCalcAppComplete(job);
   removeJob(job);
}


// ###### Send CalcAppComplete ##############################################
void CalcAppServer::sendCalcAppComplete(CalcAppServer::CalcAppServerJob* job)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type      = htonl(CALCAPP_COMPLETE);
   message.JobID     = htonl(job->JobID);
   message.JobSize   = hton64((unsigned long long)rint(job->JobSize));
   message.Completed = hton64((unsigned long long)rint(job->Completed));
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  job->SessionID, PPID_CALCAPP, 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppAccept");
   }
}







// ###### Handle message ####################################################
EventHandlingResult CalcAppServer::handleMessage(rserpool_session_t sessionID,
                                                 const char*        buffer,
                                                 size_t             bufferSize,
                                                 uint32_t           ppid,
                                                 uint16_t           streamID)
{
   if(bufferSize >= sizeof(CalcAppMessage)) {
      const CalcAppMessage* message = (const CalcAppMessage*)buffer;

printf("Type=%d\n",   ntohl(message->Type));
      switch(ntohl(message->Type)) {
         case CALCAPP_REQUEST:
            handleCalcAppRequest(sessionID, message, bufferSize);
          break;
         case CALCAPP_KEEPALIVE:
            //handleCalcAppKeepAlive(message, bufferSize);
          break;
         case CALCAPP_KEEPALIVE_ACK:
            //handleCalcAppKeepAliveAck(message, bufferSize);
          break;
         default:
            cerr << "ERROR: Unexpected message type " << ntohl(message->Type) << endl;
            //sessionSetEntry->Closing = true;
          break;
      }
   }

   scheduleNextTimerEvent();
   return(EHR_Okay);
}


// ###### Handle timer ######################################################
void CalcAppServer::handleTimer()
{
   const unsigned long long now = getMicroTime();

   CalcAppServerJob* job = FirstJob;
   while(job != NULL) {
      // ====== Handle timers ===============================================
      if(job->JobCompleteTimeStamp <= now) {
         job->JobCompleteTimeStamp = ~0ULL;
         handleJobCompleteTimer(job);
         break;
      }
      if(job->CookieTransmissionTimeStamp <= now) {
         job->CookieTransmissionTimeStamp = ~0ULL;
//          handleCookieTransmissionTimer(job);
         break;
      }
      if(job->KeepAliveTransmissionTimeStamp <= now) {
         job->KeepAliveTransmissionTimeStamp = ~0ULL;
//          handleKeepAliveTransmissionTimer(job);
      }
      if(job->KeepAliveTimeoutTimeStamp <= now) {
         job->KeepAliveTimeoutTimeStamp = ~0ULL;
//                   handleKeepAliveTimeoutTimer(job);
      }
      job = job->Next;
   }

   scheduleNextTimerEvent();
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   struct rsp_info info;
   struct TagItem  tags[8];
   unsigned int    reregInterval  = 30000;
   unsigned int    runtimeLimit   = 0;
   const char*     poolHandle     = NULL;
   size_t          maxJobs        = 2;
   char*           objectName     = "scenario.calcapppoolelement[0]";
   char*           vectorFileName = "calcapppoolelement.vec";
   char*           scalarFileName = "calcapppoolelement.sca";

   /* ====== Read parameters ============================================= */
   memset(&info, 0, sizeof(info));
   tags[0].Tag  = TAG_PoolElement_Identifier;
   tags[0].Data = 0;
   tags[1].Tag  = TAG_DONE;
#ifdef ENABLE_CSP
   info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, 0);
#endif
   for(int i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-log" ,4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
#ifdef ENABLE_CSP
      if(!(strncmp(argv[i], "-csp" ,4))) {
         if(initComponentStatusReporter(&info, argv[i]) == false) {
            exit(1);
         }
      }
#endif
      else if(!(strncmp(argv[i], "-identifier=", 12))) {
         tags[0].Data = atol((const char*)&argv[i][12]);
#ifdef ENABLE_CSP
         info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, tags[0].Data);
#endif
      }
      else if(!(strncmp(argv[i], "-registrar=", 11))) {
         if(addStaticRegistrar(&info, (char*)&argv[i][11]) < 0) {
            fprintf(stderr, "ERROR: Bad registrar setting %s\n", argv[i]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-rereginterval=" ,15))) {
         reregInterval = atol((char*)&argv[i][15]);
         if(reregInterval < 250) {
            reregInterval = 250;
         }
      }
      else if(!(strncmp(argv[i], "-runtime=" ,9))) {
         runtimeLimit = atol((const char*)&argv[i][9]);
      }
      else if(!(strncmp(argv[i], "-object=" ,8))) {
         objectName = (char*)&argv[i][8];
      }
      else if(!(strncmp(argv[i], "-vector=" ,8))) {
         vectorFileName = (char*)&argv[i][8];
      }
      else if(!(strncmp(argv[i], "-scalar=" ,8))) {
         scalarFileName = (char*)&argv[i][8];
      }
      else if(!(strncmp(argv[i], "-runtime=" ,9))) {
         runtimeLimit = atol((char*)&argv[i][9]);
      }
      else if(!(strncmp(argv[i], "-maxjobs=" ,9))) {
         maxJobs = atol((char*)&argv[i][9]);
      }
   }


   CalcAppServer calcAppServer(maxJobs, vectorFileName, scalarFileName);
   calcAppServer.poolElement("CalcAppServer - Version 1.0",
                             (poolHandle != NULL) ? poolHandle : "CalcAppPool",
                             &info, NULL,
                             reregInterval, runtimeLimit,
                             (struct TagItem*)&tags);
}
