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
 * Copyright (C) 2002-2020 by Thomas Dreibholz
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

#ifndef CALCAPPSERVICE_H
#define CALCAPPSERVICE_H

#include "rserpool.h"
#include "udplikeserver.h"
#include "calcapppackets.h"

#include <string>


class CalcAppServer : public UDPLikeServer
{
   public:
   struct CalcAppServerStatistics {
      unsigned long long StartupTimeStamp;
      double             TotalUsedCalculations;
      unsigned long long TotalJobsAccepted;
      unsigned long long TotalJobsRejected;
      unsigned long long VectorLine;
   };

   public:
   CalcAppServer(const size_t             maxJobs,
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
                 const bool               resetStatistics);
   virtual ~CalcAppServer();

   protected:
   virtual EventHandlingResult initializeService(int sd, const uint32_t peIdentifier);
   virtual void finishService(int sd, EventHandlingResult result);
   virtual void printParameters();
   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID);
   virtual EventHandlingResult handleCookieEcho(rserpool_session_t sessionID,
                                                const char*        buffer,
                                                size_t             bufferSize);
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
   void removeAllJobs();
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
   void handleCalcAppKeepAlive(rserpool_session_t    sessionID,
                               const CalcAppMessage* message,
                               const size_t          received);
   void sendCalcAppKeepAliveAck(CalcAppServerJob* job);
   void sendCalcAppKeepAlive(CalcAppServerJob* job);
   void handleCalcAppKeepAliveAck(rserpool_session_t    sessionID,
                                  const CalcAppMessage* message,
                                  const size_t          received);
   void handleKeepAliveTransmissionTimer(CalcAppServerJob* job);
   void handleKeepAliveTimeoutTimer(CalcAppServerJob* job);
   void sendCookie(CalcAppServerJob* job);
   void handleCookieTransmissionTimer(CalcAppServerJob* job);


   std::string              ObjectName;
   std::string              VectorFileName;
   std::string              ScalarFileName;
   FILE*                    VectorFH;
   FILE*                    ScalarFH;
   CalcAppServerJob*        FirstJob;
   CalcAppServerStatistics* Stats;

   size_t                   MaxJobs;
   double                   Capacity;
   unsigned long long       KeepAliveTimeoutInterval;
   unsigned long long       KeepAliveTransmissionInterval;
   unsigned long long       CookieMaxTime;
   double                   CookieMaxCalculations;
   double                   CleanShutdownProbability;
   size_t                   Jobs;
   uint32_t                 Identifier;
};

#endif
