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

#include "scriptingservice.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "stringutilities.h"

#include <sys/stat.h>


// ###### Constructor #######################################################
ScriptingServer::ScriptingServer(
   int                                       rserpoolSocketDescriptor,
   ScriptingServer::ScriptingServerSettings* settings)
   : TCPLikeServer(rserpoolSocketDescriptor)
{
   Settings     = *settings;
   State        = SS_Upload;
   Directory    = NULL;
   UploadFile   = NULL;
   DownloadFile = NULL;
}


// ###### Destructor ########################################################
ScriptingServer::~ScriptingServer()
{
   if(UploadFile) {
      fclose(UploadFile);
      UploadFile = NULL;
   }
   if(DownloadFile) {
      fclose(DownloadFile);
      DownloadFile = NULL;
   }
   if(Directory) {
      free(Directory);
      Directory = NULL;
   }
}


// ###### Create a Scripting thread ##################################
TCPLikeServer* ScriptingServer::scriptingServerFactory(int sd, void* userData)
{
   return(new ScriptingServer(sd, (ScriptingServerSettings*)userData));
}


// ###### Print parameters ##################################################
void ScriptingServer::scriptingPrintParameters(const void* userData)
{
   const ScriptingServerSettings* settings = (const ScriptingServerSettings*)userData;

   puts("Scripting Parameters:");
/*   printf("   Cookie Max Time         = %u [ms]\n", (unsigned int)(settings->CookieMaxTime / 1000ULL));
   printf("   Cookie Max Packets      = %u [Packets]\n", (unsigned int)settings->CookieMaxPackets);
   printf("   Transmit Timeout        = %u [ms]\n", (unsigned int)settings->TransmitTimeout);
   printf("   Failure After           = %u [Packets]\n", (unsigned int)settings->FailureAfter);
   printf("   Test Mode               = %s\n", (settings->TestMode == true) ? "on" : "off");*/
}


// ###### Handle Upload message #############################################
EventHandlingResult ScriptingServer::handleUploadMessage(const char* buffer,
                                                         size_t      bufferSize)
{
   if(UploadFile == NULL) {
      Directory = tempnam("/tmp", "rspSS");
      if(Directory == NULL) {
         printTimeStamp(stdout);
         puts("Unable to generate temporary directory name!");
         return(EHR_Abort);
      }
      if(mkdir(Directory, 700) != 0) {
         printTimeStamp(stdout);
         printf("Unable to create temporary directory \"%s\": %s!\n",
                Directory, strerror(errno));
         return(EHR_Abort);
      }
      if(chdir(Directory) != 0) {
         printTimeStamp(stdout);
         printf("Unable to change to temporary directory \"%s\": %s!\n",
                Directory, strerror(errno));
         return(EHR_Abort);
      }
      umask(600);

      UploadFile = fopen("input.dat", "w");
      if(UploadFile == NULL) {
         printTimeStamp(stdout);
         printf("Unable to create input file in directory \"%s\": %s!\n",
                Directory, strerror(errno));
         return(EHR_Abort);
      }
printf("W=<%s>\n",Directory);
   }

   const Upload* upload = (const Upload*)buffer;

   const size_t length = bufferSize - sizeof(ScriptingCommonHeader);
   if(fwrite(&upload->Data, length, 1, UploadFile) != 1) {
      printTimeStamp(stdout);
      printf("Write error for input file in directory \"%s\": %s!\n",
             Directory, strerror(errno));
      return(EHR_Abort);
   }

   return(EHR_Okay);
}


// ###### Handle KeppAlive message ##########################################
EventHandlingResult ScriptingServer::handleKeepAliveMessage()
{
   return(EHR_Okay);
}


// ###### Handle message ####################################################
EventHandlingResult ScriptingServer::handleMessage(const char* buffer,
                                                   size_t      bufferSize,
                                                   uint32_t    ppid,
                                                   uint16_t    streamID)
{
   const ScriptingCommonHeader* header = (const ScriptingCommonHeader*)buffer;
   if( (ppid == PPID_SP) &&
       (bufferSize >= sizeof(ScriptingCommonHeader)) &&
       (ntohs(header->Length) == bufferSize) ) {
      switch(State) {
         case SS_Upload:
            if(header->Type == SPT_UPLOAD) {
               return(handleUploadMessage(buffer, bufferSize));
            }
          break;
         case SS_Working:
            if(header->Type == SPT_KEEPALIVE) {
               return(handleKeepAliveMessage());
            }
          break;
         default:
             // All messages here are unexpected!
          break;
      }
   }
   else {
      printTimeStamp(stdout);
      puts("Received invalid message!");
      return(EHR_Abort);
   }
   printTimeStamp(stdout);
   printf("Received unexpected message $%08x in state #%u!\n", header->Type, State);
   return(EHR_Abort);
}
