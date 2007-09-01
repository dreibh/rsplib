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
#include <assert.h>


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
   Directory    = NULL;
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
   if(Directory) {
      free(Directory);
      Directory = NULL;
   }
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
   if(Directory) {
      printTimeStamp(stdout);
      printf("Cleaning up directory \"%s\"...\n", Directory);


   }
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
   // ====== Create temporary directory =====================================
   if(UploadFile == NULL) {
      // ====== Get directory and file names ================================
      // Directory = strdup("/tmp/X0");
      Directory = tempnam("/tmp", "rspSS");
      if(Directory == NULL) {
         printTimeStamp(stdout);
         puts("Unable to generate temporary directory name!");
         return(EHR_Abort);
      }
      snprintf((char*)&InputName, sizeof(InputName), "%s/%s", Directory, INPUT_NAME);
      snprintf((char*)&OutputName, sizeof(OutputName), "%s/%s", Directory, OUTPUT_NAME);
      snprintf((char*)&StatusName, sizeof(StatusName), "%s/%s", Directory, STATUS_NAME);

      // ====== Create directory ============================================
      umask(S_IRWXG | S_IRWXO);   // Only access for myself!
      if(mkdir(Directory, S_IRUSR|S_IWUSR|S_IXUSR) != 0) {   // Only for myself!
         printTimeStamp(stdout);
         printf("Unable to create temporary directory \"%s\": %s!\n",
                Directory, strerror(errno));
         return(EHR_Abort);
      }

      // ====== Create input file ===========================================
      umask(S_IXUSR | S_IRWXG | S_IRWXO);   // Only read/write for myself!
      UploadFile = fopen(InputName, "w");
      if(UploadFile == NULL) {
         printTimeStamp(stdout);
         printf("Unable to create input file in directory \"%s\": %s!\n",
                Directory, strerror(errno));
         return(EHR_Abort);
      }
      printf("Starting upload into directory \"%s\"...\n", Directory);
   }

   // ====== Write to input file ============================================
   const Upload* upload = (const Upload*)buffer;
   const size_t length = bufferSize - sizeof(ScriptingCommonHeader);
   if(length > 0) {
      if(fwrite(&upload->Data, length, 1, UploadFile) != 1) {
         printTimeStamp(stdout);
         printf("Write error for input file in directory \"%s\": %s!\n",
                Directory, strerror(errno));
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


// ###### Start working script ##############################################
EventHandlingResult ScriptingServer::performDownload()
{
   Download download;
   size_t   dataLength;
   ssize_t  sent;

   FILE* fh = fopen(OutputName, "r");
   if(fh == NULL) {
      printTimeStamp(stdout);
      printf("There are no results in directory \"%s\"!\n", Directory);
      // Will send empty Download message now!
   }

   printTimeStamp(stdout);
   printf("Starting download of results in directory \"%s\"...\n", Directory);
   for(;;) {
      if(fh != NULL) {
         dataLength = fread((char*)&download.Data, 1, sizeof(download.Data), fh);
      }
      else {
         dataLength = 0;
      }
      if(dataLength >= 0) {
         download.Header.Type   = SPT_DOWNLOAD;
         download.Header.Flags  = 0x00;
         download.Header.Length = htons(dataLength + sizeof(struct ScriptingCommonHeader));
         sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                            (const char*)&download, dataLength + sizeof(struct ScriptingCommonHeader), 0,
                            0, htonl(PPID_SP), 0, 0, 0, 0);
         if(sent <= 0) {
            fclose(fh);
            printTimeStamp(stdout);
            printf("Download data transmission error: %s\n", strerror(errno));
            return(EHR_Abort);
         }
      }
      if(dataLength <= 0) {
         break;
      }
   }
   if(fh) {
      fclose(fh);
   }

   return(EHR_Shutdown);
}


// ###### Start working script ##############################################
EventHandlingResult ScriptingServer::startWorking()
{
   assert(ChildProcess == 0);

   printTimeStamp(stdout);
   printf("Starting work in directory \"%s\"...\n", Directory);
   ChildProcess = fork();
   if(ChildProcess == 0) {
      execlp("/home/dreibh/src/rsplib2/rsplib/scriptingservice.sh",
             "scriptingservice.sh",
             "run", Directory, INPUT_NAME, OUTPUT_NAME, STATUS_NAME, NULL);
      perror("Failed to start script");
      exit(1);
   }

   setTimer(1);
   State = SS_Working;
   return(EHR_Okay);
}


// ###### Check if child process has finished ###############################
bool ScriptingServer::hasFinishedWork() const
{
   int status;

   assert(ChildProcess != 0);
   pid_t childFinished = waitpid(ChildProcess, &status, WNOHANG);
   if(childFinished == ChildProcess) {
      if(WIFEXITED(status) || WIFSIGNALED(status)) {
         return(true);
      }
   }
   return(false);
}


// ###### Check, if work is already complete ################################
EventHandlingResult ScriptingServer::timerEvent(const unsigned long long now)
{
   if(hasFinishedWork()) {
      performDownload();
   }
   setTimer(now + 1000000);
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
                              0, htonl(PPID_SP), 0, 0, 0, 0);

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
      puts("Received invalid message!");
      return(EHR_Abort);
   }
   printTimeStamp(stdout);
   printf("Received unexpected message $%08x in state #%u!\n", header->Type, State);
   return(EHR_Abort);
}
