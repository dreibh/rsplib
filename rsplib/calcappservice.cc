/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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

#include <iostream>

#include "calcappservice.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "stringutilities.h"
#include "loglevel.h"


using namespace std;


// ###### Constructor #######################################################
CalcAppServer::CalcAppServer(const size_t maxJobs,
                             const char*  objectName,
                             const char*  vectorFileName,
                             const char*  scalarFileName)
{
   Jobs           = 0;
   FirstJob       = NULL;
   ObjectName     = objectName;
   VectorFileName = vectorFileName;
   ScalarFileName = scalarFileName;
   VectorFH       = NULL;
   ScalarFH       = NULL;

   VectorLine = 0;
   MaxJobs                       = maxJobs;
   TotalUsedCalculations         = 0.0;
   Capacity                      = 1000000.0;
   KeepAliveTimeoutInterval      = 2000000;
   KeepAliveTransmissionInterval = 2000000;
   CookieMaxTime                 = 1000000;
   CookieMaxCalculations         = 5000000.0;
   StartupTimeStamp              = getMicroTime();

   AcceptedJobs                  = 0;
   RejectedJobs                  = 0;
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


// ###### Remove all jobs ###################################################
void CalcAppServer::removeAllJobs()
{
   CalcAppServerJob* job = FirstJob;
   while(job != NULL) {
      CalcAppServerJob* next = job->Next;
      sendCookie(job);   // Note, this may all removeJob()!
      job = next;
   }

   while(FirstJob) {
      removeJob(FirstJob);
   }
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
   startTimer(nextTimerTimeStamp);
}


// ###### Initialize ########################################################
EventHandlingResult CalcAppServer::initialize()
{
   VectorFH = fopen(VectorFileName.c_str(), "w");
   fprintf(VectorFH, "Runtime AvailableCalculations UsedCalculations Utilization\n");
   if(VectorFH == NULL) {
      cout << " Unable to open output file " << VectorFileName << endl;
      return(EHR_Abort);
   }

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
   removeAllJobs();

   unsigned long long       shutdownTimeStamp     = getMicroTime();
   const unsigned long long serverRuntime         = shutdownTimeStamp - StartupTimeStamp;
   const double             availableCalculations = serverRuntime * Capacity / 1000000.0;
   const double             utilization           = TotalUsedCalculations / availableCalculations;

   const double totalUsedCapacity         = TotalUsedCalculations;
   const double totalPossibleCalculations = (serverRuntime/1000000.0)*Capacity;
   const double totalWastedCapacity       = serverRuntime - totalUsedCapacity;

   if(ScalarFH) {
      fprintf(ScalarFH, "scalar \"%s\" \"Total Used Capacity \"%1.6f\n", ObjectName.c_str(), totalUsedCapacity);
      fprintf(ScalarFH, "scalar \"%s\" \"Total Possible Calculations \"%1.6f\n", ObjectName.c_str(), totalPossibleCalculations);
      fprintf(ScalarFH, "scalar \"%s\" \"Total Wasted Capacity \"%1.6f\n", ObjectName.c_str(), totalWastedCapacity);
      fprintf(ScalarFH, "scalar \"%s\" \"Total Jobs Accepted \"%u\n", ObjectName.c_str(), AcceptedJobs);
      fprintf(ScalarFH, "scalar \"%s\" \"Total Jobs Rejected \"%u\n", ObjectName.c_str(), RejectedJobs);
      fprintf(ScalarFH, "scalar \"%s\" \"Utilization \"%1.6f\n", ObjectName.c_str(), utilization);
   }
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
      cout << "Job " << sessionID << "/" << ntohl(message->JobID) << " rejected!" << endl;
      sendCalcAppReject(sessionID, ntohl(message->JobID));
   }
}


// ###### Handle incoming CalcAppRequest ####################################
EventHandlingResult CalcAppServer::handleCookieEcho(rserpool_session_t sessionID,
                                                    const char*        buffer,
                                                    size_t             bufferSize)
{
   if(bufferSize >= sizeof(CalcAppCookie)) {
      const CalcAppCookie* cookie = (const CalcAppCookie*)buffer;
      CalcAppServerJob* job = addJob(sessionID, ntohl(cookie->JobID),
                                     (double)ntoh64(cookie->JobSize),
                                     (double)ntoh64(cookie->Completed));
      if(job) {
         cout << "Job " << job->SessionID << "/" << job->JobID
            << " of size " << job->JobSize << ", completed "
            << job->Completed << " accepted from cookie echo" << endl;
         sendCalcAppAccept(job);
      }
      else {
         cout << "Job " << sessionID << "/" << ntohl(cookie->JobID) << " rejected from cookie echo!" << endl;
         sendCalcAppReject(sessionID, ntohl(cookie->JobID));
      }
      return(EHR_Okay);
   }
   else {
      cerr << "ERROR: Received malformed cookie echo!" << endl;
   }
   return(EHR_Abort);
}


// ###### Send CalcAppAccept ################################################
void CalcAppServer::sendCalcAppAccept(CalcAppServer::CalcAppServerJob* job)
{
   AcceptedJobs++;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type   = CALCAPP_ACCEPT;
   message.Flags  = 0x00;
   message.Length = ntohs(sizeof(message));
   message.JobID  = htonl(job->JobID);
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  job->SessionID, htonl(PPID_CALCAPP), 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppAccept");
      removeJob(job);
   }
}


// ###### Send CalcAppReject ################################################
void CalcAppServer::sendCalcAppReject(rserpool_session_t sessionID, uint32_t jobID)
{
   RejectedJobs++;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type   = CALCAPP_REJECT;
   message.Flags  = 0x00;
   message.Length = ntohs(sizeof(message));
   message.JobID  = htonl(jobID);
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  sessionID, htonl(PPID_CALCAPP), 0, 0, 0) < 0) {
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
   message.Type      = CALCAPP_COMPLETE;
   message.Flags     = 0x00;
   message.Length    = ntohs(sizeof(message));
   message.JobID     = htonl(job->JobID);
   message.JobSize   = hton64((unsigned long long)rint(job->JobSize));
   message.Completed = hton64((unsigned long long)rint(job->Completed));
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  job->SessionID, htonl(PPID_CALCAPP), 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppAccept");
   }

   unsigned long long       shutdownTimeStamp     = getMicroTime();
   const unsigned long long serverRuntime         = shutdownTimeStamp - StartupTimeStamp;
   const double             availableCalculations = serverRuntime * Capacity / 1000000.0;
   const double             utilization           = TotalUsedCalculations / availableCalculations;
   fprintf(VectorFH," %u %llu %1.6f %1.6f %1.6f\n",
           ++VectorLine, serverRuntime, availableCalculations,
           TotalUsedCalculations, utilization);
}


// ###### Handle incoming CalcAppKeepAlive ##################################
void CalcAppServer::handleCalcAppKeepAlive(rserpool_session_t    sessionID,
                                           const CalcAppMessage* message,
                                           const size_t          received)
{
   CalcAppServerJob* job = findJob(sessionID, ntohl(message->JobID));
   if(job != NULL) {
      sendCalcAppKeepAliveAck(job);
   }
   else {
      cerr << "ERROR: CalcAppKeepAlive for wrong job!" << endl;
      removeJob(job);
      return;
   }
}


// ###### Send CalcAppKeepAlive #############################################
void CalcAppServer::sendCalcAppKeepAliveAck(CalcAppServer::CalcAppServerJob* job)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type   = CALCAPP_KEEPALIVE_ACK;
   message.Flags  = 0x00;
   message.Length = ntohs(sizeof(message));
   message.JobID  = htonl(job->JobID);
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  job->SessionID, htonl(PPID_CALCAPP), 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppKeepAlive");
      removeJob(job);
      return;
   }
}


// ###### Send CalcAppKeepAlive #############################################
void CalcAppServer::sendCalcAppKeepAlive(CalcAppServer::CalcAppServerJob* job)
{
//    KeepAlives++;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type   = CALCAPP_KEEPALIVE;
   message.Flags  = 0x00;
   message.Length = ntohs(sizeof(message));
   message.JobID  = htonl(job->JobID);
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  job->SessionID, htonl(PPID_CALCAPP), 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppKeepAlive");
      removeJob(job);
      return;
   }
}


// ###### Handle incoming CalcAppKeepAliveAck ###############################
void CalcAppServer::handleCalcAppKeepAliveAck(rserpool_session_t    sessionID,
                                              const CalcAppMessage* message,
                                              const size_t          received)
{
   CalcAppServerJob* job = findJob(sessionID, ntohl(message->JobID));
   if(job != NULL) {
      job->KeepAliveTimeoutTimeStamp      = ~0ULL;
      job->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
   }
   else {
      cerr << "ERROR: CalcAppKeepAliveAck for wrong job!" << endl;
      removeJob(job);
      return;
   }
}


// ###### Handle KeepAlive Transmission timer ###############################
void CalcAppServer::handleKeepAliveTransmissionTimer(CalcAppServer::CalcAppServerJob* job)
{
   job->KeepAliveTransmissionTimeStamp = ~0ULL;
   job->KeepAliveTimeoutTimeStamp      = getMicroTime() + KeepAliveTimeoutInterval;
   sendCalcAppKeepAlive(job);
}


// ###### Handle KeepAlive Timeout timer ####################################
void CalcAppServer::handleKeepAliveTimeoutTimer(CalcAppServer::CalcAppServerJob* job)
{
   job->KeepAliveTimeoutTimeStamp = ~0ULL;
   cout << "No keep-alive for job " << job->JobID << "/" << job->JobID
        << " -> removing session" << endl;
   removeJob(job);
}


// ###### Send cookie #######################################################
void CalcAppServer::sendCookie(CalcAppServer::CalcAppServerJob* job)
{
   struct CalcAppCookie cookie;
   memset(&cookie, 0, sizeof(cookie));
   cookie.JobID     = htonl(job->JobID);
   cookie.JobSize   = hton64((unsigned long long)rint(job->JobSize));
   cookie.Completed = hton64((unsigned long long)rint(job->Completed));
   if(rsp_send_cookie(RSerPoolSocketDescriptor,
                      (const unsigned char*)&cookie, sizeof(cookie),
                      job->SessionID, 0) < 0) {
      logerror("Unable to send CalcAppKeepAlive");
      removeJob(job);
      return;
   }

puts("COOKIE!");

   job->LastCookieAt        = getMicroTime();
   job->LastCookieCompleted = job->Completed;
}


// ###### Handle Cookie Transmission timer ##################################
void CalcAppServer::handleCookieTransmissionTimer(CalcAppServer::CalcAppServerJob* job)
{
   updateCalculations();
   sendCookie(job);
   scheduleJobs();
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

      switch(message->Type) {
         case CALCAPP_REQUEST:
            handleCalcAppRequest(sessionID, message, bufferSize);
          break;
         case CALCAPP_KEEPALIVE:
            handleCalcAppKeepAlive(sessionID, message, bufferSize);
          break;
         case CALCAPP_KEEPALIVE_ACK:
            handleCalcAppKeepAliveAck(sessionID, message, bufferSize);
          break;
         default:
            cerr << "ERROR: Unexpected message type " << message->Type << endl;
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
      CalcAppServerJob* next = job->Next;

      // ====== Handle timers ===============================================
      if(job->JobCompleteTimeStamp <= now) {
         job->JobCompleteTimeStamp = ~0ULL;
         handleJobCompleteTimer(job);
         break;
      }
      if(job->CookieTransmissionTimeStamp <= now) {
         job->CookieTransmissionTimeStamp = ~0ULL;
         handleCookieTransmissionTimer(job);
         break;
      }
      if(job->KeepAliveTransmissionTimeStamp <= now) {
         job->KeepAliveTransmissionTimeStamp = ~0ULL;
          handleKeepAliveTransmissionTimer(job);
         break;
      }
      if(job->KeepAliveTimeoutTimeStamp <= now) {
         job->KeepAliveTimeoutTimeStamp = ~0ULL;
         handleKeepAliveTimeoutTimer(job);
         break;
      }

      job = next;
   }

   scheduleNextTimerEvent();
}
