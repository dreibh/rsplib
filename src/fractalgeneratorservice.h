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

#ifndef FRACTALGENERATORSERVICE_H
#define FRACTALGENERATORSERVICE_H

#include "rserpool.h"
#include "tcplikeserver.h"
#include "fractalgeneratorpackets.h"


class FractalGeneratorServer : public TCPLikeServer
{
   public:
   struct FractalGeneratorServerSettings
   {
      unsigned long long DataMaxTime;
      unsigned long long CookieMaxTime;
      size_t             CookieMaxPackets;
      size_t             TransmitTimeout;
      size_t             FailureAfter;
      bool               TestMode;
   };

   FractalGeneratorServer(int                             rserpoolSocketDescriptor,
                          FractalGeneratorServerSettings* settings);
   ~FractalGeneratorServer();

   static TCPLikeServer* fractalGeneratorServerFactory(int sd, void* userData, const uint32_t peIdentifier);
   static void fractalGeneratorPrintParameters(const void* userData);

   protected:
   void asyncTimerEvent(const unsigned long long now);
   EventHandlingResult handleCookieEcho(const char* buffer, size_t bufferSize);
   EventHandlingResult handleMessage(const char* buffer,
                                     size_t      bufferSize,
                                     uint32_t    ppid,
                                     uint16_t    streamID);

   private:
   void scheduleTimer();
   bool sendCookie(const unsigned long long now);
   bool sendData(const unsigned long long now);
   bool handleParameter(const FGPParameter* parameter,
                        const size_t        size,
                        const bool          insideCookie = false);
   EventHandlingResult calculateImage();
   EventHandlingResult advanceX(const unsigned int color);
   void advanceY();

   unsigned long long             LastCookieTimeStamp;
   unsigned long long             LastSendTimeStamp;
   size_t                         DataPacketsAfterLastCookie;

   struct FGPData                 Data;
   size_t                         DataPackets;
   bool                           Alert;

   FractalGeneratorStatus         Status;
   FractalGeneratorServerSettings Settings;
};

#endif
