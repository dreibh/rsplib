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
 * Copyright (C) 2002-2010 by Thomas Dreibholz
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

#ifndef SCRIPTINGSERVICE_H
#define SCRIPTINGSERVICE_H

#include "rserpool.h"
#include "tcplikeserver.h"
#include "scriptingpackets.h"

#include <unistd.h>
#include <sys/wait.h>


class ScriptingServer : public TCPLikeServer
{
   public:
   struct ScriptingServerSettings
   {
      unsigned int TransmitTimeout;
      unsigned int KeepAliveInterval;
      unsigned int KeepAliveTimeout;
      bool         KeepTempDirs;
      bool         VerboseMode;
   };

   ScriptingServer(int                      rserpoolSocketDescriptor,
                   ScriptingServerSettings* settings);
   ~ScriptingServer();

   static TCPLikeServer* scriptingServerFactory(int sd, void* userData, const uint32_t peIdentifier);
   static void scriptingPrintParameters(const void* userData);
   static void rejectNewSession(int sd);

   protected:
   virtual bool start();
   EventHandlingResult handleMessage(const char* buffer,
                                     size_t      bufferSize,
                                     uint32_t    ppid,
                                     uint16_t    streamID);

   private:
   virtual EventHandlingResult initializeSession();
   virtual void finishSession(EventHandlingResult result);
   virtual EventHandlingResult syncTimerEvent(const unsigned long long now);
   EventHandlingResult startWorking();
   bool hasFinishedWork(int& exitStatus);
   EventHandlingResult sendStatus(const int exitStatus);
   EventHandlingResult performDownload();
   EventHandlingResult handleUploadMessage(const char* buffer,
                                           size_t      bufferSize);
   EventHandlingResult sendKeepAliveMessage();
   EventHandlingResult handleKeepAliveAckMessage();
   EventHandlingResult handleKeepAliveMessage();

   enum ScriptingState {
      SS_Upload   = 1,
      SS_Working  = 2,
      SS_Download = 3
   };

   ScriptingState          State;
   ScriptingServerSettings Settings;
   char                    Directory[128];
   char                    InputName[256];
   char                    OutputName[256];
   char                    StatusName[256];
   FILE*                   UploadFile;
   pid_t                   ChildProcess;
   bool                    WaitingForKeepAliveAck;
   unsigned long long      LastKeepAliveTimeStamp;
   unsigned long long      UploadSize;
   unsigned long long      DownloadSize;
   unsigned long long      UploadStarted;
   unsigned long long      ProcessingStarted;
   unsigned long long      DownloadStarted;
};


#endif
