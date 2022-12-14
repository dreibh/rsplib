/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2023 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#include <math.h>
#include <iostream>

#include "calcappservice.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "stringutilities.h"
#include "randomizer.h"
#include "loglevel.h"


using namespace std;


// ###### Constructor #######################################################
CalcAppServer::CalcAppServer(const size_t             maxJobs,
                             const char*              objectName,
                             const char*              vectorFileName,
                             const char*              scalarFileName,
                             const double             capacity,
                             const unsigned long long keepAliveTransmissionInterval,
                             const unsigned long long keepAliveTimeoutInterval,
                             const unsigned long long cookieMaxTime,
                             const double             cookieMaxCalculations,
                             const double             cleanShutdownProbability,
                             CalcAppServerStatistics* statistics,
                             const bool               resetStatistics)
{
   FirstJob                      = NULL;
   ObjectName                    = objectName;
   VectorFileName                = vectorFileName;
   ScalarFileName                = scalarFileName;
   VectorFH                      = NULL;
   ScalarFH                      = NULL;
   MaxJobs                       = maxJobs;
   Capacity                      = capacity;
   KeepAliveTimeoutInterval      = keepAliveTimeoutInterval;
   KeepAliveTransmissionInterval = keepAliveTransmissionInterval;
   CookieMaxTime                 = cookieMaxTime;
   CookieMaxCalculations         = cookieMaxCalculations;
   CleanShutdownProbability      = cleanShutdownProbability;
   Jobs                          = 0;
   Stats                         = statistics;
   if(resetStatistics) {
      Stats->StartupTimeStamp      = getMicroTime();
      Stats->TotalUsedCalculations = 0.0;
      Stats->TotalJobsAccepted     = 0;
      Stats->TotalJobsRejected     = 0;
      Stats->VectorLine            = 0;
   }
}


// ###### Destructor ########################################################
CalcAppServer::~CalcAppServer()
{
}


// ###### Print parameters ##################################################
void CalcAppServer::printParameters()
{
   puts("CalcApp Parameters:");
   printf("   Object Name             = %s\n", ObjectName.c_str());
   printf("   Vector File Name        = %s\n", VectorFileName.c_str());
   printf("   Scalar File Name        = %s\n", ScalarFileName.c_str());
   printf("   Max Jobs                = %u\n", (unsigned int)MaxJobs);
   printf("   Capacity                = %1.1f Calculations/s\n", Capacity);
   printf("   Clean Shutdown Prob.    = %1.2f%%\n", 100.0 * CleanShutdownProbability);
   printf("   KA Transmission Interv. = %llums\n", KeepAliveTransmissionInterval / 1000);
   printf("   KA Timeout Interval     = %llums\n", KeepAliveTimeoutInterval / 1000);
   printf("   Cookie Max Time         = %1.3fs\n", CookieMaxTime / 1000000.0);
   printf("   Cookie Max Calculations = %1.0f Calculations\n", CookieMaxCalculations);
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
         job->LastCookieAt                   = job->LastUpdateAt;
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
         delete job;
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
   // ====== Clean shutdown: send cookies to all PUs ========================
   const double r = randomDouble();
   if(r <= CleanShutdownProbability) {
      // Update the jobs' progress to send correct cookies!
      updateCalculations();
      scheduleJobs();

      // Send cookies
      CalcAppServerJob* job = FirstJob;
      while(job != NULL) {
         CalcAppServerJob* next = job->Next;
         sendCookie(job);   // Note: this function may call removeJob()!
         job = next;
      }
   }

   // ====== Remove all jobs ================================================
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
            Stats->TotalUsedCalculations += completedCalculations;
            job->Completed               += completedCalculations;
         }
         else {
            CHECK(job->JobSize - job->Completed >= 0.0);
            Stats->TotalUsedCalculations += job->JobSize - job->Completed;
            job->Completed                = job->JobSize;
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
         const double calculationsToGo     = job->JobSize - job->Completed;
         const unsigned long long timeToGo = (unsigned long long)ceil(calculationsToGo / capacityPerJob);

         // job->JobCompleteTimeStamp = now + timeToGo;
         job->JobCompleteTimeStamp = job->LastUpdateAt + timeToGo;

         // When is the time to send the next cookie?
         const double calculationsSinceLastCookie = job->Completed - job->LastCookieCompleted;
         const double timeSinceLastCookie         = (double)now - (double)job->LastCookieAt;
         const unsigned long long nextByCalculations =
            (unsigned long long)rint(max(0.0, CookieMaxCalculations - calculationsSinceLastCookie) / capacityPerJob);
         const unsigned long long nextByTime =
            (unsigned long long)rint(max(0.0, CookieMaxTime - timeSinceLastCookie));
         const unsigned long long nextCookie = min(nextByCalculations, nextByTime);
         job->CookieTransmissionTimeStamp = now + nextCookie;

         job = job->Next;
      }
   }

   setLoad((double)Jobs / (double)MaxJobs);
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
EventHandlingResult CalcAppServer::initializeService(int sd, const uint32_t peIdentifier)
{
   Identifier = peIdentifier;

   VectorFH = fopen(VectorFileName.c_str(), (Stats->VectorLine > 0) ? "a" : "w");
   if(VectorFH == NULL) {
      cout << " Unable to open output file " << VectorFileName << endl;
      return(EHR_Abort);
   }
   setbuf(VectorFH, NULL);   // Make stream non-buffered!

   ScalarFH = fopen(ScalarFileName.c_str(), "w");
   if(ScalarFH == NULL) {
      cout << " Unable to open output file " << ScalarFileName << endl;
      return(EHR_Abort);
   }
   setbuf(ScalarFH, NULL);   // Make stream non-buffered!
   fprintf(ScalarFH, "run 1 \"scenario\"\n");
   return(EHR_Okay);
}


// ###### Shutdown ##########################################################
void CalcAppServer::finishService(int sd, EventHandlingResult initializeResult)
{
   removeAllJobs();

   if(ScalarFH) {
      unsigned long long       shutdownTimeStamp     = getMicroTime();
      const unsigned long long serviceUptime         = shutdownTimeStamp - Stats->StartupTimeStamp;
      const double             availableCalculations = serviceUptime * Capacity / 1000000.0;
      const double             utilization           = Stats->TotalUsedCalculations / availableCalculations;

      const double             totalUsedCapacity     = Stats->TotalUsedCalculations;
      const double             totalWastedCapacity   = serviceUptime - totalUsedCapacity;

      fprintf(ScalarFH, "scalar \"%s\" \"CalcAppPE Service Uptime\"        %1.6f\n", ObjectName.c_str(), serviceUptime / 1000000.0);
      fprintf(ScalarFH, "scalar \"%s\" \"CalcAppPE Service Capacity\"      %1.0f\n", ObjectName.c_str(), Capacity);
      fprintf(ScalarFH, "scalar \"%s\" \"CalcAppPE Service Max Jobs\"      %u\n",    ObjectName.c_str(), (unsigned int)MaxJobs);

      fprintf(ScalarFH, "scalar \"%s\" \"CalcAppPE Total Used Capacity\"   %1.0f\n", ObjectName.c_str(), totalUsedCapacity);
      fprintf(ScalarFH, "scalar \"%s\" \"CalcAppPE Total Wasted Capacity\" %1.0f\n", ObjectName.c_str(), totalWastedCapacity);

      fprintf(ScalarFH, "scalar \"%s\" \"CalcAppPE Total Jobs Accepted\"   %llu\n",  ObjectName.c_str(), Stats->TotalJobsAccepted);
      fprintf(ScalarFH, "scalar \"%s\" \"CalcAppPE Total Jobs Rejected\"   %llu\n",  ObjectName.c_str(), Stats->TotalJobsRejected);

      fprintf(ScalarFH, "scalar \"%s\" \"CalcAppPE Utilization\"           %1.6f\n", ObjectName.c_str(), utilization);

      fclose(ScalarFH);
      ScalarFH = NULL;
   }
   if(VectorFH) {
      fclose(VectorFH);
      VectorFH = NULL;
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
   Stats->TotalJobsAccepted++;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type   = CALCAPP_ACCEPT;
   message.Flags  = 0x00;
   message.Length = ntohs(sizeof(message));
   message.JobID  = htonl(job->JobID);
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  job->SessionID, htonl(PPID_CALCAPP), 0, 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppAccept");
      removeJob(job);
   }
}


// ###### Send CalcAppReject ################################################
void CalcAppServer::sendCalcAppReject(rserpool_session_t sessionID, uint32_t jobID)
{
   Stats->TotalJobsRejected++;
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type   = CALCAPP_REJECT;
   message.Flags  = 0x00;
   message.Length = ntohs(sizeof(message));
   message.JobID  = htonl(jobID);
   if(rsp_sendmsg(RSerPoolSocketDescriptor,
                  (void*)&message, sizeof(message), 0,
                  sessionID, htonl(PPID_CALCAPP), 0, 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppReject");
   }
}


// ###### Handle JobComplete timer ##########################################
void CalcAppServer::handleJobCompleteTimer(CalcAppServer::CalcAppServerJob* job)
{
   cout << "Job " << job->SessionID << "/" << job->JobID << " is complete!" << endl;
   updateCalculations();
   scheduleJobs();
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
                  job->SessionID, htonl(PPID_CALCAPP), 0, 0, 0, 0) < 0) {
      logerror("Unable to send CalcAppAccept");
   }

   unsigned long long       shutdownTimeStamp     = getMicroTime();
   const unsigned long long serviceUptime         = shutdownTimeStamp - Stats->StartupTimeStamp;
   const double             availableCalculations = serviceUptime * Capacity / 1000000.0;
   const double             utilization           = Stats->TotalUsedCalculations / availableCalculations;
   if(VectorFH) {
      if(Stats->VectorLine == 0) {
         fprintf(VectorFH, "ObjectName PID PoolElementIdentifier CurrentTimeStamp Runtime   AvailableCalculations UsedCalculations Utilization   CurrentJobs MaxJobs\n");
      }
      fprintf(VectorFH,"%06llu %s %u 0x%08x %1.6f   %1.6f   %1.0f %1.0f %1.6f   %u %u\n",
            ++Stats->VectorLine,
            ObjectName.c_str(),
            (unsigned int)getpid(),
            Identifier,
            getMicroTime() / 1000000.0,
            serviceUptime / 1000000.0,
            availableCalculations, Stats->TotalUsedCalculations, utilization,
            (unsigned int)Jobs, (unsigned int)MaxJobs);
      // fflush(VectorFH);
   }
}


// ###### Handle incoming CalcAppKeepAlive ##################################
void CalcAppServer::handleCalcAppKeepAlive(rserpool_session_t    sessionID,
                                           const CalcAppMessage* message,
                                           const size_t          received)
{
   CalcAppServerJob* job = findJob(sessionID, ntohl(message->JobID));
   if(job == NULL) {
      // The CalcAppKeepAlive may be for a job that just finished before. Then,
      // it will not be in the list. In this case, just do nothing.
      return;
   }
   sendCalcAppKeepAliveAck(job);
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
                  job->SessionID, htonl(PPID_CALCAPP), 0, 0, 0, 0) < 0) {
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
                  job->SessionID, htonl(PPID_CALCAPP), 0, 0, 0, 0) < 0) {
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
   if(job == NULL) {
      // The CalcAppKeepAliveAck may be for a job that just finished before. Then,
      // it will not be in the list. In this case, just do nothing.
      return;
   }
   job->KeepAliveTimeoutTimeStamp      = ~0ULL;
   job->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
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
