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
   void removeAll();
   size_t getSessions();
   unsigned long long initSelectArray(SessionDescriptor** sdArray,
                                      unsigned int*       statusArray,
                                      const unsigned int  status);
   void handleTimers();
   void handleEvents(SessionDescriptor* session,
                     const unsigned int sessionEvents);

   private:
   unsigned long long KeepAliveTimeoutInterval;
   unsigned long long KeepAliveTransmissionInterval;

   struct SessionSetEntry {
      SessionSetEntry*   Next;
      SessionDescriptor* Session;

      bool               HasJob;
      unsigned int       JobID;
      double             JobSize;
      double             Completed;
      unsigned int       Cookieintervall;
      unsigned long long LastUpdate;

      unsigned long long KeepAliveTransmissionTimeStamp;
      unsigned long long KeepAliveTimeoutTimeStamp;
      unsigned long long JobCompleteTimeStamp;
      unsigned long long KeepAliveTimeoutInterval;

   };

   SessionSetEntry* find(SessionDescriptor* session);

   void sendCalcAppAccept(SessionSetEntry* sessionListEntry);
   void sendCalcAppReject(SessionSetEntry* sessionListEntry);
   void Cookie(SessionSetEntry* sessionListEntry);
   void sendCalcAppComplete(SessionSetEntry* sessionListEntry);
   void sendCalcAppKeepAlive(SessionSetEntry* sessionListEntry);
   void sendCalcAppKeepAliveAck(SessionSetEntry* sessionListEntry);
   void sendCalcAppAbort(SessionSetEntry* sessionListEntry);

   void handleJobCompleteTimer(SessionSetEntry* sessionListEntry);
   void handleKeepAliveTransmissionTimer(SessionSetEntry* sessionListEntry);
   void handleKeepAliveTimeoutTimer(SessionSetEntry* sessionListEntry);
   void handleCalcAppRequest(SessionSetEntry* sessionListEntry,
                             CalcAppMessage*  message,
                             const size_t     size);
   void handleCalcAppKeepAliveAck(SessionSetEntry* sessionListEntry,
                             CalcAppMessage*  message,
                             const size_t     size);
   void handleCookieEcho(SessionSetEntry* sessionListEntry,
                         CalcAppMessage*  message,
                         const size_t     size);
   void handleCommand(SessionSetEntry* sessionListEntry,
                      CalcAppMessage*  message,
                      const size_t     size);
   void handleTimer(SessionSetEntry* sessionListEntry);

   size_t getActiveSessions();
   void updateCalculations();
   void scheduleJobs();


   SessionSetEntry* FirstSession;
   size_t           Sessions;
   double           Capacity;
};

SessionSet::SessionSet()
{
   FirstSession                  = NULL;
   Sessions                      = 0;
   Capacity                      = 1000000.0;
   KeepAliveTimeoutInterval      = 2000000;
   KeepAliveTransmissionInterval = 2000000;
}

SessionSet::~SessionSet()
{
   removeAll();
}

void SessionSet::removeAll()
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      removeSession(sessionSetEntry->Session);
      sessionSetEntry = FirstSession;
   }
   CHECK(Sessions == 0);
}

bool SessionSet::addSession(SessionDescriptor* session)
{
   bool result = false;

   updateCalculations();
   SessionSetEntry* sessionSetEntry = new SessionSetEntry;
   if(sessionSetEntry) {
      sessionSetEntry->Next       = FirstSession;
      sessionSetEntry->Session    = session;
      sessionSetEntry->HasJob     = false;
      sessionSetEntry->JobID      = 0;
      sessionSetEntry->JobSize    = 0;
      sessionSetEntry->Completed  = 0;
      sessionSetEntry->LastUpdate = 0;

      sessionSetEntry->KeepAliveTransmissionTimeStamp = ~0ULL;
      sessionSetEntry->KeepAliveTimeoutTimeStamp      = ~0ULL;
      sessionSetEntry->JobCompleteTimeStamp           = ~0ULL;

      FirstSession                = sessionSetEntry;
      Sessions++;
      result = true;
   }
   scheduleJobs();

   return(result);
}

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

size_t SessionSet::getSessions()
{
   return(Sessions);
}

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

      sessionSetEntry = sessionSetEntry->Next;
   }

   return(nextTimerTimeStamp);
}


void SessionSet::handleCalcAppRequest(SessionSetEntry* sessionListEntry,
                                      CalcAppMessage*  message,
                                      const size_t     received)
{
   if(sessionListEntry->HasJob) {
      cerr << "ERROR: Session already has a job!" << endl;
      removeSession(sessionListEntry->Session);
      return;
   }

   sessionListEntry->HasJob     = true;
   sessionListEntry->JobID      = message->JobID;
   sessionListEntry->JobSize    = message->JobSize;
   sessionListEntry->Completed  = 0;
   sessionListEntry->LastUpdate = getMicroTime();

   cout << "Job " << sessionListEntry->JobID << " with size "
        << sessionListEntry->JobSize << " accepted" << endl;

   sessionListEntry->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
   sessionListEntry->KeepAliveTimeoutTimeStamp      = ~0ULL;

   updateCalculations();
   scheduleJobs();

   sendCalcAppAccept(sessionListEntry);
}


void SessionSet::handleCookieEcho(SessionSetEntry* sessionListEntry,
                                  CalcAppMessage*  message,
                                  const size_t     size)
{
   puts("COOKIE ECHO!");
}


void SessionSet::sendCalcAppAccept(SessionSetEntry* sessionListEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type  = htonl(CALCAPP_ACCEPT);
   message.JobID = htonl(sessionListEntry->JobID);
   if(rspSessionWrite(sessionListEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppAccept" << endl;
      removeSession(sessionListEntry->Session);
   }
}


void SessionSet::sendCalcAppReject(SessionSetEntry* sessionListEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type = htonl(CALCAPP_REJECT);
   removeSession(sessionListEntry->Session);
   printf("Your Request was denied");
      if(rspSessionWrite(sessionListEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppReject" << endl;
      removeSession(sessionListEntry->Session);
   }
}


void SessionSet::sendCalcAppKeepAlive(SessionSetEntry* sessionListEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type      = htonl(CALCAPP_KEEPALIVE);
   message.JobID     = htonl(sessionListEntry->JobID);
   if(rspSessionWrite(sessionListEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppKeepAlive" << endl;
      removeSession(sessionListEntry->Session);
   }
}


void SessionSet::handleJobCompleteTimer(SessionSetEntry* sessionListEntry)
{
   CHECK(sessionListEntry->HasJob);
   cout << "Job " << sessionListEntry->JobID << " is complete!" << endl;
   sendCalcAppComplete(sessionListEntry);
   removeSession(sessionListEntry->Session);
}


void SessionSet::handleKeepAliveTransmissionTimer(SessionSetEntry* sessionListEntry)
{
   CHECK(sessionListEntry->HasJob);
   sessionListEntry->KeepAliveTransmissionTimeStamp = ~0ULL;
   sessionListEntry->KeepAliveTimeoutTimeStamp      = getMicroTime() + KeepAliveTimeoutInterval;
puts("Sende KA...");
   sendCalcAppKeepAlive(sessionListEntry);
}


void SessionSet::handleKeepAliveTimeoutTimer(SessionSetEntry* sessionListEntry)
{
   CHECK(sessionListEntry->HasJob);
   sessionListEntry->KeepAliveTimeoutTimeStamp = ~0ULL;
   cout << "No keep-alive for job " << sessionListEntry->JobID
        << " -> removing session" << endl;
   removeSession(sessionListEntry->Session);
}


void SessionSet::handleCalcAppKeepAliveAck(SessionSetEntry* sessionListEntry,
                                           CalcAppMessage*  message,
                                           const size_t     size)
{
   if(sessionListEntry->HasJob) {
      cerr << "ERROR: Session already has a job!" << endl;
      removeSession(sessionListEntry->Session);
      return;
   }
   if(sessionListEntry->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppKeepAliveAck for wrong job!" << endl;
      removeSession(sessionListEntry->Session);
      return;
   }
puts("KA empfangen");

   sessionListEntry->KeepAliveTimeoutTimeStamp      = ~0ULL;
   sessionListEntry->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
}


void SessionSet::handleTimer(SessionSetEntry* sessionListEntry)
{
   unsigned long long now = getMicroTime();

   if(sessionListEntry->JobCompleteTimeStamp <= now) {
      handleJobCompleteTimer(sessionListEntry);
      sessionListEntry->JobCompleteTimeStamp = ~0ULL;
   }
   if(sessionListEntry->KeepAliveTransmissionTimeStamp <= now) {
      handleKeepAliveTransmissionTimer(sessionListEntry);
      sessionListEntry->KeepAliveTransmissionTimeStamp = ~0ULL;
   }
   if(sessionListEntry->KeepAliveTimeoutTimeStamp <= now) {
      handleKeepAliveTimeoutTimer(sessionListEntry);
      sessionListEntry->KeepAliveTimeoutTimeStamp = ~0ULL;
   }
}


void SessionSet::handleEvents(SessionDescriptor* session,
                              const unsigned int sessionEvents)
{
   char           buffer[4096];
   struct TagItem tags[16];
   ssize_t        received;

   SessionSetEntry* sessionSetEntry = find(session);
   CHECK(sessionSetEntry != NULL);


   tags[0].Tag  = TAG_RspIO_MsgIsCookieEcho;
   tags[0].Data = 0;
   tags[1].Tag  = TAG_RspIO_Timeout;
   tags[1].Data = 0;
   tags[2].Tag  = TAG_DONE;
   received = rspSessionRead(sessionSetEntry->Session, (char*)&buffer, sizeof(buffer), (struct TagItem*)&tags);
   if(received > 0) {
      printf("recv=%d\n",received);
      if(tags[0].Data != 0) {
         handleCookieEcho(sessionSetEntry, (struct CalcAppMessage*)&buffer, received);
      }
      else {
         handleCommand(sessionSetEntry, (struct CalcAppMessage*)&buffer, received);
      }
   }
}


/*
void SessionSet::Cookie(SessionSetEntry* sessionListEntry)
{
   if (HasJob==true && getMicrotime()%Cookieintervall==0)
   {
       message.JobID     = htonl(sessionListEntry->JobID);
        message.JobSize   = sessionListEntry->JobSize;
       sendCalcAppComplete(SessionSetEntry* sessionListEntry);
   }
}
*/



void SessionSet::sendCalcAppComplete(SessionSetEntry* sessionListEntry)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type      = htonl(CALCAPP_COMPLETE);
   message.JobID     = htonl(sessionListEntry->JobID);
   message.JobSize   = sessionListEntry->JobSize;
   if(sessionListEntry->HasJob) {
      cerr << "ERROR: Session already has a job!" << endl;
      removeSession(sessionListEntry->Session);
      return;
   }   message.Completed = sessionListEntry->Completed;
   if(rspSessionWrite(sessionListEntry->Session,
                      (void*)&message,
                      sizeof(message),
                      NULL) <= 0) {
      cerr << "ERROR: Unable to send CalcAppComplete" << endl;
      removeSession(sessionListEntry->Session);
   }
}


void SessionSet::handleTimers()
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      handleTimer(sessionSetEntry);
      sessionSetEntry = sessionSetEntry->Next;
   }
}


void SessionSet::handleCommand(SessionSetEntry* sessionListEntry,
                               CalcAppMessage*  message,
                               const size_t     received)
{
   if(received >= sizeof(CalcAppMessage)) {
      printf("Type = %x\n",ntohl(message->Type));
      switch(ntohl(message->Type)) {
         case CALCAPP_REQUEST:
            handleCalcAppRequest(sessionListEntry, message, received);
          break;
         default:
           cerr << "ERROR: Unexpected message type " << ntohl(message->Type) << endl;
           removeSession(sessionListEntry->Session);
          break;
      }
   }
}


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


void SessionSet::updateCalculations()
{
   const unsigned long long now     = getMicroTime();
   SessionSetEntry* sessionSetEntry = FirstSession;
   const size_t     activeSessions  = getActiveSessions();

   if(activeSessions > 0) {
      const double capacityPerJob = Capacity / (double)activeSessions;

      while(sessionSetEntry != NULL) {
         if(sessionSetEntry->HasJob) {
            const unsigned long long elapsed   = now - sessionSetEntry->LastUpdate;
            const double completedCalculations = elapsed / capacityPerJob;
            if(sessionSetEntry->Completed + completedCalculations < sessionSetEntry->JobSize) {
               sessionSetEntry->Completed += completedCalculations;
            }
            else {
               sessionSetEntry->Completed = sessionSetEntry->JobSize;
            }
         }
         sessionSetEntry->LastUpdate = now;
         sessionSetEntry = sessionSetEntry->Next;
      }
   }
}


void SessionSet::scheduleJobs()
{
   const unsigned long long now     = getMicroTime();
   SessionSetEntry* sessionSetEntry = FirstSession;
   const size_t     activeSessions  = getActiveSessions();

   if(activeSessions > 0) {
      const double capacityPerJob = Capacity / (double)activeSessions;
      while(sessionSetEntry != NULL) {
         if(sessionSetEntry->HasJob) {
            const double calculationsToGo         = sessionSetEntry->JobSize - sessionSetEntry->Completed;
            const unsigned long long timeToGo     = (unsigned long long)ceil(1000000.0 * (calculationsToGo / capacityPerJob));

printf("%Ld  cpj=%f\n",timeToGo,    capacityPerJob);
            sessionSetEntry->JobCompleteTimeStamp = now + timeToGo;
         }
         sessionSetEntry = sessionSetEntry->Next;
      }
   }
}



int main(int argc, char** argv)
{
   uint32_t                      identifier        = 0;
   unsigned int                  reregInterval     = 5000;
   struct PoolElementDescriptor* poolElement;
   struct TagItem                tags[16];
   struct PoolElementDescriptor* pedArray[1];
   unsigned int                  pedStatusArray[FD_SETSIZE];
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
printf("NEXT: %Ld\n",timeout);
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

