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

#ifndef SCRIPTINGSERVICE_H
#define SCRIPTINGSERVICE_H

#include "rserpool.h"
#include "tcplikeserver.h"
#include "scriptingpackets.h"


class ScriptingServer : public TCPLikeServer
{
   public:
   struct ScriptingServerSettings
   {
   };

   ScriptingServer(int                      rserpoolSocketDescriptor,
                   ScriptingServerSettings* settings);
   ~ScriptingServer();

   static TCPLikeServer* scriptingServerFactory(int sd, void* userData);
   static void scriptingPrintParameters(const void* userData);

   protected:
   EventHandlingResult handleMessage(const char* buffer,
                                     size_t      bufferSize,
                                     uint32_t    ppid,
                                     uint16_t    streamID);

   private:
   EventHandlingResult handleUploadMessage(const char* buffer,
                                           size_t      bufferSize);
   EventHandlingResult handleKeepAliveMessage();

   enum ScriptingState {
      SS_Upload   = 1,
      SS_Working  = 2,
      SS_Download = 3
   };

   ScriptingState          State;
   ScriptingServerSettings Settings;
   char*                   Directory;
   FILE*                   UploadFile;
   FILE*                   DownloadFile;
};


#endif
