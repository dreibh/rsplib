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
   virtual EventHandlingResult handleNotification(const union rserpool_notification* notification);
   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID);
};


#if 0
class CharGenServer : public TCPLikeServer
{
   public:
   struct CharGenServerSettings
   {
      size_t FailureAfter;
   };

   CharGenServer(int rserpoolSocketDescriptor);
   ~CharGenServer();

   static TCPLikeServer* charGenServerFactory(int sd, void* userData);

   protected:
   EventHandlingResult handleMessage(const char* buffer,
                                     size_t      bufferSize,
                                     uint32_t    ppid,
                                     uint16_t    streamID);
};
#endif


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

   static TCPLikeServer* pingPongServerFactory(int sd, void* userData);

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


#include "fractalgeneratorpackets.h"

class FractalGeneratorServer : public TCPLikeServer
{
   public:
   struct FractalGeneratorServerSettings
   {
      size_t FailureAfter;
      bool  TestMode;
   };

   FractalGeneratorServer(int                             rserpoolSocketDescriptor,
                          FractalGeneratorServerSettings* settings);
   ~FractalGeneratorServer();

   static TCPLikeServer* fractalGeneratorServerFactory(int sd, void* userData);

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
