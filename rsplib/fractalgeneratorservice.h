/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2007 by Thomas Dreibholz
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
      size_t FailureAfter;
      bool   TestMode;
   };

   FractalGeneratorServer(int                             rserpoolSocketDescriptor,
                          FractalGeneratorServerSettings* settings);
   ~FractalGeneratorServer();

   static TCPLikeServer* fractalGeneratorServerFactory(int sd, void* userData);
   static void fractalGeneratorPrintParameters(const void* userData);

   protected:
   EventHandlingResult handleCookieEcho(const char* buffer, size_t bufferSize);
   EventHandlingResult handleMessage(const char* buffer,
                                     size_t      bufferSize,
                                     uint32_t    ppid,
                                     uint16_t    streamID);

   private:
   bool sendCookie();
   bool sendData(FGPData* data);
   bool handleParameter(const FGPParameter* parameter,
                        const size_t        size,
                        const bool          insideCookie = false);
   EventHandlingResult calculateImage();

   FractalGeneratorStatus         Status;
   unsigned long long             LastCookieTimeStamp;
   unsigned long long             LastSendTimeStamp;
   FractalGeneratorServerSettings Settings;
};


#endif
