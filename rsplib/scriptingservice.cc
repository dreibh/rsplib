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
 * Copyright (C) 2002-2008 by Thomas Dreibholz
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
#include <assert.h>
#include <dirent.h>
#include <sys/types.h>
#include <signal.h>


#define INPUT_NAME  "input.data"
#define OUTPUT_NAME "output.data"
#define STATUS_NAME "status.txt"


// ###### Constructor #######################################################
ScriptingServer::ScriptingServer(
   int                                       rserpoolSocketDescriptor,
   ScriptingServer::ScriptingServerSettings* settings)
   : TCPLikeServer(rserpoolSocketDescriptor)
{
   Settings     = *settings;
   State        = SS_Upload;
   UploadFile   = NULL;
   ChildProcess = 0;
}


// ###### Destructor ########################################################
ScriptingServer::~ScriptingServer()
{
   if(UploadFile) {
      fclose(UploadFile);
      UploadFile = NULL;
   }
}


// ###### Overloaded start() method #########################################
bool ScriptingServer::start()
{
   // As soon as possible tell registrar the new load!
   setLoad(1.0 / ServerList->getMaxThreads());
   return(TDThread::start());
}


// ###### Create a Scripting thread #########################################
TCPLikeServer* ScriptingServer::scriptingServerFactory(int sd, void* userData)
{
   return(new ScriptingServer(sd, (ScriptingServerSettings*)userData));
}


// ###### Clean up session ##################################################
void ScriptingServer::finishSession(EventHandlingResult result)
{
   if(ChildProcess) {
      kill(ChildProcess, SIGKILL);
      waitpid(ChildProcess, NULL, 0);
   }
   ChildProcess = 0;
   if((Directory) && (!Settings.KeepTempDirs)) {
      printTimeStamp(stdout);
      printf("S%04d: Cleaning up directory \"%s\"...\n",
             RSerPoolSocketDescriptor, Directory);

      DIR* dir = opendir(Directory);
      if(dir) {
         dirent* entry = readdir(dir);
         while(entry) {
            char name[1024];
            snprintf((char*)&name, sizeof(name), "%s/%s", Directory, entry->d_name);
            unlink(name);
            entry = readdir(dir);
         }
         closedir(dir);
      }
      if(rmdir(Directory) != 0) {
         printTimeStamp(stdout);
         printf("S%04d: Unable to remove directory \"%s\": %s!\n",
                RSerPoolSocketDescriptor, Directory, strerror(errno));
      }
   }
}


// ###### Print parameters ##################################################
void ScriptingServer::scriptingPrintParameters(const void* userData)
{
   const ScriptingServerSettings* settings = (const ScriptingServerSettings*)userData;

   puts("Scripting Parameters:");
   printf("   Transmit Timeout        = %u [ms]\n", settings->TransmitTimeout);
   printf("   Keep Temp Dirs          = %s\n", (settings->KeepTempDirs == true) ? "yes" : "no");
}


// ###### Handle Upload message #############################################
EventHandlingResult ScriptingServer::handleUploadMessage(const char* buffer,
                                                         size_t      bufferSize)
{
   // ====== Create temporary directory =====================================
   if(UploadFile == NULL) {
      // ====== Create directory and get file names =========================
      safestrcpy((char*)&Directory, "/tmp/rspSS-XXXXXX", sizeof(Directory));
      if(mkdtemp((char*)&Directory) == NULL) {
         printTimeStamp(stdout);
         printf("S%04d: Unable to generate temporary directory!\n",
                RSerPoolSocketDescriptor);
         return(EHR_Abort);
      }
      snprintf((char*)&InputName, sizeof(InputName), "%s/%s", Directory, INPUT_NAME);
      snprintf((char*)&OutputName, sizeof(OutputName), "%s/%s", Directory, OUTPUT_NAME);
      snprintf((char*)&StatusName, sizeof(StatusName), "%s/%s", Directory, STATUS_NAME);

      // ====== Create input file ===========================================
      umask(S_IXUSR | S_IRWXG | S_IRWXO);   // Only read/write for myself!
      UploadFile = fopen(InputName, "w");
      if(UploadFile == NULL) {
         printTimeStamp(stdout);
         printf("S%04d: Unable to create input file in directory \"%s\": %s!\n",
                RSerPoolSocketDescriptor, Directory, strerror(errno));
         return(EHR_Abort);
      }
      printf("S%04d: Starting upload into directory \"%s\"...\n",
             RSerPoolSocketDescriptor, Directory);
   }

   // ====== Write to input file ============================================
   const Upload* upload = (const Upload*)buffer;
   const size_t length = bufferSize - sizeof(ScriptingCommonHeader);
   if(length > 0) {
      if(fwrite(&upload->Data, length, 1, UploadFile) != 1) {
         printTimeStamp(stdout);
         printf("S%04d: Write error for input file in directory \"%s\": %s!\n",
                RSerPoolSocketDescriptor, Directory, strerror(errno));
         return(EHR_Abort);
      }
   }
   else {
      fclose(UploadFile);
      UploadFile = NULL;
      return(startWorking());
   }

   return(EHR_Okay);
}


// ###### Send status report ################################################
EventHandlingResult ScriptingServer::sendStatus(const int exitStatus)
{
   Status status;
   status.Header.Type   = SPT_STATUS;
   status.Header.Flags  = 0x00;
   status.Header.Length = htons(sizeof(status));
   status.ExitStatus    = htonl((uint32_t)exitStatus);
   ssize_t sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                              (const char*)&status, sizeof(status), 0,
                              0, htonl(PPID_SP), 0, 0, 0, Settings.TransmitTimeout);
   if(sent <= 0) {
      printTimeStamp(stdout);
      printf("S%04d: Status transmission error: %s\n",
             RSerPoolSocketDescriptor, strerror(errno));
      return(EHR_Abort);
   }
   return(EHR_Okay);
}


// ###### Start working script ##############################################
EventHandlingResult ScriptingServer::performDownload()
{
   Download download;
   size_t   dataLength;
   ssize_t  sent;

   FILE* fh = fopen(OutputName, "r");
   if(fh != NULL) {
      printTimeStamp(stdout);
      printf("S%04d: Starting download of results in directory \"%s\"...\n",
             RSerPoolSocketDescriptor, Directory);
      for(;;) {
         dataLength = fread((char*)&download.Data, 1, sizeof(download.Data), fh);
         if(dataLength >= 0) {
            download.Header.Type   = SPT_DOWNLOAD;
            download.Header.Flags  = 0x00;
            download.Header.Length = htons(dataLength + sizeof(struct ScriptingCommonHeader));
            sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                               (const char*)&download, dataLength + sizeof(struct ScriptingCommonHeader), 0,
                               0, htonl(PPID_SP), 0, 0, 0, Settings.TransmitTimeout);
            if(sent <= 0) {
               fclose(fh);
               printTimeStamp(stdout);
               printf("S%04d: Download data transmission error: %s\n",
                      RSerPoolSocketDescriptor, strerror(errno));
               return(EHR_Abort);
            }
         }
         if(dataLength <= 0) {
            break;
         }
      }
      fclose(fh);
   } else {
      printTimeStamp(stdout);
      printf("S%04d: There are no results in directory \"%s\"!\n",
             RSerPoolSocketDescriptor, Directory);
      return(EHR_Abort);
   }

   return(EHR_Shutdown);
}


// ###### Start working script ##############################################
EventHandlingResult ScriptingServer::startWorking()
{
   assert(ChildProcess == 0);

   printTimeStamp(stdout);
   printf("S%04d: Starting work in directory \"%s\"...\n",
          RSerPoolSocketDescriptor, Directory);
   ChildProcess = fork();
   if(ChildProcess == 0) {
      execlp("./scriptingcontrol",
             "scriptingcontrol",
             "run", Directory, INPUT_NAME, OUTPUT_NAME, STATUS_NAME, NULL);
      // Try standard location ...
      execlp("scriptingcontrol",
             "scriptingcontrol",
             "run", Directory, INPUT_NAME, OUTPUT_NAME, STATUS_NAME, NULL);
      perror("Failed to start script");
      exit(1);
   }

   setSyncTimer(1);
   State = SS_Working;
   return(EHR_Okay);
}


// ###### Check if child process has finished ###############################
bool ScriptingServer::hasFinishedWork(int& exitStatus) const
{
   int status;

   assert(ChildProcess != 0);
   pid_t childFinished = waitpid(ChildProcess, &status, WNOHANG);
   if(childFinished == ChildProcess) {
      if(WIFEXITED(status) || WIFSIGNALED(status)) {
         exitStatus = WEXITSTATUS(status);
         return(true);
      }
   }
   exitStatus = 0;
   return(false);
}


// ###### Check, if work is already complete ################################
EventHandlingResult ScriptingServer::syncTimerEvent(const unsigned long long now)
{
   int exitStatus;
   if(hasFinishedWork(exitStatus)) {
      if(sendStatus(exitStatus) == EHR_Okay) {
         return(performDownload());
      }
   }
   setSyncTimer(now + 1000000);
   return(EHR_Okay);
}


// ###### Handle KeppAlive message ##########################################
EventHandlingResult ScriptingServer::handleKeepAliveMessage()
{
   KeepAliveAck keepAliveAck;
   keepAliveAck.Header.Type   = SPT_KEEPALIVE_ACK;
   keepAliveAck.Header.Flags  = 0x00;
   keepAliveAck.Header.Length = htons(sizeof(keepAliveAck));
   keepAliveAck.Status        = htonl(0);

   ssize_t sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                              (const char*)&keepAliveAck, sizeof(keepAliveAck), 0,
                              0, htonl(PPID_SP), 0, 0, 0, Settings.TransmitTimeout);

   return( (sent == sizeof(keepAliveAck)) ? EHR_Okay : EHR_Abort );
}


// ###### Handle message ####################################################
EventHandlingResult ScriptingServer::handleMessage(const char* buffer,
                                                   size_t      bufferSize,
                                                   uint32_t    ppid,
                                                   uint16_t    streamID)
{
   const ScriptingCommonHeader* header = (const ScriptingCommonHeader*)buffer;
   if( (ppid == ntohl(PPID_SP)) &&
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
      printf("S%04d: Received invalid message!\n", RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   printTimeStamp(stdout);
   printf("S%04d: Received unexpected message $%08x in state #%u!\n",
          RSerPoolSocketDescriptor, header->Type, State);
   return(EHR_Abort);
}
