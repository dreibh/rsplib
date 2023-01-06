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
 * Contact: thomas.dreibholz@gmail.com
 */

#ifndef STANDARDSERVICES_H
#define STANDARDSERVICES_H

#include "rserpool.h"
#include "tcplikeserver.h"
#include "udplikeserver.h"


class EchoServer : public UDPLikeServer
{
   public:
   EchoServer();
   virtual ~EchoServer();

   protected:
   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID);
};


class DiscardServer : public UDPLikeServer
{
   public:
   DiscardServer();
   virtual ~DiscardServer();

   protected:
   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID);
};


class DaytimeServer : public UDPLikeServer
{
   public:
   DaytimeServer();
   virtual ~DaytimeServer();

   protected:
   virtual void handleNotification(const union rserpool_notification* notification);
};


class CharGenServer : public TCPLikeServer
{
   public:
   struct CharGenServerSettings
   {
      size_t FailureAfter;
   };

   CharGenServer(int rserpoolSocketDescriptor);
   ~CharGenServer();

   static TCPLikeServer* charGenServerFactory(int sd, void* userData, const uint32_t peIdentifier);

   protected:
   virtual EventHandlingResult initializeSession();
};


class PingPongServer : public TCPLikeServer
{
   public:
   struct PingPongServerSettings
   {
      size_t FailureAfter;
   };

   PingPongServer(int rserpoolSocketDescriptor,
                  PingPongServer::PingPongServerSettings* settings);
   ~PingPongServer();

   static TCPLikeServer* pingPongServerFactory(int sd, void* userData, const uint32_t peIdentifier);

   protected:
   EventHandlingResult handleCookieEcho(const char* buffer, size_t bufferSize);
   EventHandlingResult handleMessage(const char* buffer,
                                     size_t      bufferSize,
                                     uint32_t    ppid,
                                     uint16_t    streamID);

   private:
   PingPongServerSettings Settings;
   uint64_t               ReplyNo;
   size_t                 Replies;
};

#endif
