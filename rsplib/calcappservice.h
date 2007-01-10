/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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

#ifndef CALCAPPSERVICE_H
#define CALCAPPSERVICE_H

#include "rserpool.h"
#include "udplikeserver.h"
#include "calcapppackets.h"

#include <string>


class CalcAppServer : public UDPLikeServer
{
   public:
   CalcAppServer(const size_t             maxJobs,
                 const char*              objectName,
                 const char*              vectorFileName,
                 const char*              scalarFileName,
                 const double             capacity,
                 const unsigned long long keepAliveTransmissionInterval,
                 const unsigned long long keepAliveTimeoutInterval,
                 const unsigned long long cookieMaxTime,
                 const double             cookieMaxCalculations);
   virtual ~CalcAppServer();

   protected:
   virtual EventHandlingResult initialize();
   virtual void finish(EventHandlingResult result);
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


   std::string        ObjectName;
   std::string        VectorFileName;
   std::string        ScalarFileName;
   FILE*              VectorFH;
   FILE*              ScalarFH;
   unsigned int       VectorLine;

   size_t             MaxJobs;
   double             Capacity;
   unsigned long long KeepAliveTimeoutInterval;
   unsigned long long KeepAliveTransmissionInterval;
   unsigned long long CookieMaxTime;
   double             CookieMaxCalculations;

   double             TotalUsedCalculations;
   unsigned long long TotalJobsAccepted;
   unsigned long long TotalJobsRejected;

   unsigned long long StartupTimeStamp;
   size_t             Jobs;
   CalcAppServerJob*  FirstJob;
};

#endif
