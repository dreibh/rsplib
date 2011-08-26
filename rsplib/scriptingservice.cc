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
 * Copyright (C) 2002-2011 by Thomas Dreibholz
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
#include "loglevel.h"

#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>


#define ENVIRONMENT_NAME  "environment.data"
#define INPUT_NAME        "input.data"
#define OUTPUT_NAME       "output.data"
#define STATUS_NAME       "status.txt"


EnvironmentCache ScriptingServer::Cache;


// ###### Constructor #######################################################
ScriptingServer::ScriptingServer(
   int                                       rserpoolSocketDescriptor,
   ScriptingServer::ScriptingServerSettings* settings)
   : TCPLikeServer(rserpoolSocketDescriptor)
{
   Settings               = *settings;
   State                  = SS_Upload;
   UploadFile             = NULL;
   ChildProcess           = 0;
   Directory[0]           = 0x00;
   LastKeepAliveTimeStamp = 0;
   WaitingForKeepAliveAck = false;
   WaitingForEnvironment  = false;
   NeedsEnvironment       = false;
   GotEnvironment         = false;

   UploadSize             = 0;
   DownloadSize           = 0;
   UploadStarted          = 0;
   ProcessingStarted      = 0;
   DownloadStarted        = 0;
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
TCPLikeServer* ScriptingServer::scriptingServerFactory(
                  int sd, void* userData, const uint32_t peIdentifier)
{
   return(new ScriptingServer(sd, (ScriptingServerSettings*)userData));
}


// ###### Initialize service ################################################
bool ScriptingServer::initializeService(void* userData)
{
   const ScriptingServerSettings* settings = (const ScriptingServerSettings*)userData;
   if(Cache.initializeCache(settings->CacheDirectory.c_str(),
                            settings->CacheMaxSize, settings->CacheMaxEntries,
                            stdlog) == false) {
      fputs("ERROR: Unable to initialize environment cache!\n", stderr);
      return(false);
   }
   return(true);
}


// ###### Finish service ####################################################
void ScriptingServer::finishService(void* userData)
{
   // const ScriptingServerSettings* settings = (const ScriptingServerSettings*)userData;
   Cache.cleanUp();
}


// ###### Print parameters ##################################################
void ScriptingServer::scriptingPrintParameters(const void* userData)
{
   const ScriptingServerSettings* settings = (const ScriptingServerSettings*)userData;

   puts("Scripting Parameters:");
   printf("   Keep Temp Dirs          = %s\n", (settings->KeepTempDirs == true) ? "yes" : "no");
   printf("   Verbose Mode            = %s\n", (settings->VerboseMode == true) ? "yes" : "no");
   printf("   Transmit Timeout        = %u [ms]\n", settings->TransmitTimeout);
   printf("   Keep-Alive Interval     = %u [ms]\n", settings->KeepAliveInterval);
   printf("   Keep-Alive Timeout      = %u [ms]\n", settings->KeepAliveTimeout);
   printf("   Cache Max Size          = %llu [KiB]\n", settings->CacheMaxSize / 1024);
   printf("   Cache Max Entries       = %u\n", settings->CacheMaxEntries);
   printf("   Cache Directory         = %s\n", settings->CacheDirectory.c_str());
}


// ###### Reject new session when server is fully loaded ####################
void ScriptingServer::rejectNewSession(int sd)
{
   char           buffer[sizeof(NotReady) + SR_MAX_INFOSIZE];
   struct utsname systemInfo;

   if(uname(&systemInfo) != 0) {
      systemInfo.nodename[0] = 0x00;
   }

   NotReady* notReady = (NotReady*)&buffer;
   memset((char*)&notReady->Info, 0x00, SR_MAX_INFOSIZE);
   snprintf((char*)&notReady->Info, SR_MAX_INFOSIZE, "%s", systemInfo.nodename);
   const ssize_t notReadyLength = sizeof(NotReady) + strlen(notReady->Info);
   notReady->Header.Type   = SPT_NOTREADY;
   notReady->Header.Flags  = 0x00;
   notReady->Header.Length = htons(notReadyLength);
   notReady->Reason        = htonl(SSNR_FULLY_LOADED);

   rsp_sendmsg(sd, (const char*)notReady, notReadyLength, 0,
                   0, htonl(PPID_SP), 0, 0, 0, 0);
}


// ###### Initialize session ################################################
EventHandlingResult ScriptingServer::initializeSession()
{
   // ====== Create directory and get file names ============================
   const char* tempDirectory = getenv("TMPDIR");
   if(tempDirectory == NULL) {
      safestrcpy((char*)&Directory, "/tmp", sizeof(Directory));
   }
   else {
      safestrcpy((char*)&Directory, tempDirectory, sizeof(Directory));
   }
   safestrcat((char*)&Directory, "/rspSS-Session-XXXXXX", sizeof(Directory));
   if(mkdtemp((char*)&Directory) == NULL) {
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Unable to generate temporary directory!\n",
              RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   snprintf((char*)&EnvironmentName, sizeof(EnvironmentName), "%s/%s", Directory, ENVIRONMENT_NAME);
   snprintf((char*)&InputName,  sizeof(InputName),  "%s/%s", Directory, INPUT_NAME);
   snprintf((char*)&OutputName, sizeof(OutputName), "%s/%s", Directory, OUTPUT_NAME);
   snprintf((char*)&StatusName, sizeof(StatusName), "%s/%s", Directory, STATUS_NAME);


   // ====== Send Ready message =============================================
   char           buffer[sizeof(Ready) + SR_MAX_INFOSIZE];
   struct utsname systemInfo;
   if(uname(&systemInfo) != 0) {
      systemInfo.nodename[0] = 0x00;
   }

   Ready* ready = (Ready*)&buffer;
   memset((char*)&ready->Info, 0x00, SR_MAX_INFOSIZE);
   snprintf((char*)&ready->Info, SR_MAX_INFOSIZE, "%s", systemInfo.nodename);
   const ssize_t readyLength = sizeof(Ready) + strlen(ready->Info);
   ready->Header.Type   = SPT_READY;
   ready->Header.Flags  = 0x00;
   ready->Header.Length = htons(readyLength);

   setSyncTimer(getMicroTime() + (1000ULL * Settings.TransmitTimeout));   // Timeout for first upload message


   const ssize_t sent  = rsp_sendmsg(RSerPoolSocketDescriptor,
                                     (const char*)ready, readyLength, 0,
                                     0, htonl(PPID_SP), 0, 0, 0, Settings.TransmitTimeout);
   return((sent == readyLength) ? EHR_Okay : EHR_Abort);
}


// ###### Clean up session ##################################################
void ScriptingServer::finishSession(EventHandlingResult result)
{
   if(Directory[0] != 0x00) {   // If there is no upload, there is no directory to clean up
      // ====== Redirect output into logfile ================================
      const int stdlogFD = fileno(stdlog);
      dup2(stdlogFD, STDOUT_FILENO);
      dup2(stdlogFD, STDERR_FILENO);

      // ====== Run script ==================================================
      char sscmd[128];
      char callcmd[384];
      int  success;

      snprintf((char*)&sscmd, sizeof(sscmd), "scriptingcontrol cleanup %s %d %s",
               Directory, (int)ChildProcess,
               (Settings.KeepTempDirs == true) ? "keeptempdirs" : "");
      snprintf((char*)&callcmd, sizeof(callcmd), "if [ -e ./scriptingcontrol ] ; then ./%s ; else %s ; fi", sscmd, sscmd);

      success = system(callcmd);
      if(success != 0) {
         printTimeStamp(stdlog);
         fprintf(stdlog, "S%04d: ERROR: Unable to clean up directory \"%s\": %s!\n",
                 RSerPoolSocketDescriptor, Directory, strerror(errno));
      }

      if(ChildProcess) {
         kill(ChildProcess, SIGKILL);   // Just to be really sure ...
         waitpid(ChildProcess, NULL, 0);
         ChildProcess = 0;
      }
   }
}


// ###### Handle Upload message #############################################
EventHandlingResult ScriptingServer::handleUploadMessage(const Upload* upload)

{
   // ====== Create input file ==============================================
   if(UploadFile == NULL) {
      if(WaitingForEnvironment == true) {
         sha1_init(&EnvironmentHashContext);
         UploadFile = fopen(EnvironmentName, "w");
      }
      else {
         UploadFile = fopen(InputName, "w");
      }

      if(UploadFile == NULL) {
         printTimeStamp(stdlog);
         fprintf(stdlog, "S%04d: Unable to create input file \"%s\" in directory \"%s\": %s!\n",
                 RSerPoolSocketDescriptor,
                 (WaitingForEnvironment == true) ? EnvironmentName : InputName,
                 Directory, strerror(errno));
         return(EHR_Abort);
      }
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Starting upload of %s into directory \"%s\" ...\n",
              RSerPoolSocketDescriptor,
              (WaitingForEnvironment == true) ? "environment" : "work package",
              Directory);
      UploadSize    = 0;
      UploadStarted = getMicroTime();
   }


   // ====== Write data to input file =======================================
   const size_t length = ntohs(upload->Header.Length) - sizeof(upload->Header);
   if(length > 0) {
      if(Settings.VerboseMode) {
         fputs(".", stdlog);
         fflush(stdlog);
      }
      if(fwrite(&upload->Data, length, 1, UploadFile) != 1) {
         printTimeStamp(stdlog);
         fprintf(stdlog, "S%04d: Write error for input file in directory \"%s\": %s!\n",
                 RSerPoolSocketDescriptor, Directory, strerror(errno));
         return(EHR_Abort);
      }
      if(WaitingForEnvironment == true) {
         sha1_update(&EnvironmentHashContext, (uint8_t*)&upload->Data, length);
      }
      UploadSize += (unsigned long long)length;
      setSyncTimer(getMicroTime() + (1000ULL * Settings.TransmitTimeout));   // Timeout for next upload message
      return(EHR_Okay);
   }


   // ====== Upload finished ================================================
   else {
      if(Settings.VerboseMode) {
         fputs("\n", stdlog);
      }
      if(fclose(UploadFile) != 0) {
         fprintf(stdlog, "S%04d: Close error for input file in directory \"%s\": %s!\n",
                 RSerPoolSocketDescriptor, Directory, strerror(errno));
         return(EHR_Abort);
      }
      UploadFile = NULL;

      ProcessingStarted = getMicroTime();

      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Upload completed (%s: %1.0lf KiB in %1.1lf s; %1.1lf KiB/s) => starting work in directory \"%s\" ...\n",
              RSerPoolSocketDescriptor,
              (WaitingForEnvironment == true) ? EnvironmentName : InputName,
              ceil((double)UploadSize / 1024.0),
              (ProcessingStarted - UploadStarted) / 1000000.0,
              (double)((double)UploadSize / 1024.0) / ((ProcessingStarted - UploadStarted) / 1000000.0),
              Directory);


      // ====== Continue upload with environment file =======================
      if((NeedsEnvironment == true) && (WaitingForEnvironment == false)) {
         WaitingForEnvironment = true;   // Now, get the environment
         return(EHR_Okay);
      }

      // ====== Upload of work package and environment completed ============
      else {
         if(WaitingForEnvironment == true) {
            GotEnvironment = true;
            sha1_final(&EnvironmentHashContext, (uint8_t*)&EnvironmentHash);
            printTimeStamp(stdlog);
            fprintf(stdlog, "S%04d: Got environment ",
                    RSerPoolSocketDescriptor);
            for(size_t i = 0; i < sizeof(EnvironmentHash); i++) {
               fprintf(stdlog, "%02x", (unsigned int)EnvironmentHash[i]);
            }
            fputs("\n", stdlog);
            Cache.storeInCache((const uint8_t*)&EnvironmentHash,
                               EnvironmentName,
                               UploadSize);
         }

         // ====== Start to do actual work ==================================
         EventHandlingResult result = sendStatus(0);
         if(result == 0) {
            return(startWorking());
         }
         return(result);
      }
   }
}


// ###### Handle Upload message #############################################
EventHandlingResult ScriptingServer::handleEnvironmentMessage(
                                        const Environment* environmentQuery)
{
   // ====== Check Environment query message ================================
   if(ntohs(environmentQuery->Header.Length) < sizeof(Environment)) {
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Received invalid Environment message\n",
              RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }

   printTimeStamp(stdlog);
   fprintf(stdlog, "S%04d: User requests environment ", RSerPoolSocketDescriptor);
   for(size_t i = 0; i < sizeof(environmentQuery->Hash); i++) {
      fprintf(stdlog, "%02x", (unsigned int)environmentQuery->Hash[i]);
   }
   fputs("\n", stdlog);


   // ====== Look for environment in cache ==================================
   if(Cache.copyFromCache((const uint8_t*)&environmentQuery->Hash,
                          EnvironmentName) == false) {
      NeedsEnvironment = true;
   }
   else {
      GotEnvironment = true;
   }


   // ====== Tell user the result ===========================================
   Environment environmentResponse;
   environmentResponse.Header.Type   = SPT_ENVIRONMENT;
   environmentResponse.Header.Flags  = (NeedsEnvironment == true) ? SEF_UPLOAD_NEEDED : 0;
   environmentResponse.Header.Length = htons(sizeof(environmentResponse));
   memcpy(&environmentResponse.Hash, &environmentQuery->Hash, sizeof(environmentResponse.Hash));
   ssize_t sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                     (const char*)&environmentResponse, sizeof(environmentResponse), 0,
                     0, htonl(PPID_SP), 0, 0, 0, Settings.TransmitTimeout);
   if(sent != (ssize_t)sizeof(environmentResponse)) {
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Environment transmission error: %s\n",
               RSerPoolSocketDescriptor, strerror(errno));
      return(EHR_Abort);
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
   status.Status        = htonl((uint32_t)exitStatus);
   const ssize_t sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                                    (const char*)&status, sizeof(status), 0,
                                    0, htonl(PPID_SP), 0, 0, 0, Settings.TransmitTimeout);
   if(sent != sizeof(status)) {
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Status transmission error: %s\n",
              RSerPoolSocketDescriptor, strerror(errno));
      return(EHR_Abort);
   }
   return(EHR_Okay);
}


// ###### Perform work package download #####################################
EventHandlingResult ScriptingServer::performDownload()
{
   Download download;
   size_t   dataLength;
   ssize_t  sent;

   FILE* fh = fopen(OutputName, "r");
   if(fh != NULL) {
      DownloadStarted = getMicroTime();
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Processing completed (%1.1lf s) => starting download of results in directory \"%s\" ...\n",
              RSerPoolSocketDescriptor,
              (DownloadStarted - ProcessingStarted) / 1000000.0,
              Directory);
      for(;;) {
         dataLength = fread((char*)&download.Data, 1, sizeof(download.Data), fh);
         if(dataLength >= 0) {
            download.Header.Type   = SPT_DOWNLOAD;
            download.Header.Flags  = 0x00;
            download.Header.Length = htons(dataLength + sizeof(struct ScriptingCommonHeader));
            if(Settings.VerboseMode) {
               fputs(".", stdlog);
               fflush(stdlog);
            }
            sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                               (const char*)&download, dataLength + sizeof(struct ScriptingCommonHeader), 0,
                               0, htonl(PPID_SP), 0, 0, 0, Settings.TransmitTimeout);
            if(sent != (ssize_t)(dataLength + sizeof(struct ScriptingCommonHeader))) {
               fclose(fh);
               printTimeStamp(stdlog);
               fprintf(stdlog, "S%04d: Download data transmission error: %s\n",
                       RSerPoolSocketDescriptor, strerror(errno));
               return(EHR_Abort);
            }
            DownloadSize += (unsigned long long)dataLength;
         }
         if(dataLength <= 0) {
            const unsigned long long downloadFinished = getMicroTime();
            printTimeStamp(stdlog);
            fprintf(stdlog, "S%04d: Download completed (%1.0lf KiB in %1.1lf s; %1.1lf KiB/s) => finished work in directory \"%s\".\n",
                  RSerPoolSocketDescriptor,
                  ceil((double)DownloadSize / 1024.0),
                  (downloadFinished - DownloadStarted) / 1000000.0,
                  (double)((double)DownloadSize / 1024.0) / ((downloadFinished - DownloadStarted) / 1000000.0),
                  Directory);
            break;
         }
      }
      if(Settings.VerboseMode) {
         fputs("\n", stdlog);
      }
      fclose(fh);
   } else {
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: There are no results in directory \"%s\"!\n",
              RSerPoolSocketDescriptor, Directory);
      return(EHR_Abort);
   }

   return(EHR_Shutdown);
}


// ###### Start working script ##############################################
EventHandlingResult ScriptingServer::startWorking()
{
   assert(ChildProcess == 0);

   printTimeStamp(stdlog);
   fprintf(stdlog, "S%04d: Starting work in directory \"%s\" ...\n",
           RSerPoolSocketDescriptor, Directory);
   ChildProcess = fork();
   if(ChildProcess == 0) {
      // ====== Redirect output into logfile ================================
      const int stdlogFD = fileno(stdlog);
      dup2(stdlogFD, STDOUT_FILENO);
      dup2(stdlogFD, STDERR_FILENO);

      // ====== Run script ==================================================
      execlp("./scriptingcontrol",
             "scriptingcontrol",
             "run", Directory, INPUT_NAME, OUTPUT_NAME, STATUS_NAME,
             (GotEnvironment == true) ? ENVIRONMENT_NAME : NULL,
             (char*)NULL);
      // Try standard location ...
      execlp("scriptingcontrol",
             "scriptingcontrol",
             "run", Directory, INPUT_NAME, OUTPUT_NAME, STATUS_NAME,
             (GotEnvironment == true) ? ENVIRONMENT_NAME : NULL,
             (char*)NULL);
      perror("Failed to start scriptingcontrol");
      exit(1);
   }

   State = SS_Working;
   setSyncTimer(1);
   return(EHR_Okay);
}


// ###### Check if child process has finished ###############################
bool ScriptingServer::hasFinishedWork(int& exitStatus)
{
   int status;

   assert(ChildProcess != 0);
   pid_t childFinished = waitpid(ChildProcess, &status, WNOHANG);
   if(childFinished == ChildProcess) {
      ChildProcess = 0;
      if(WIFEXITED(status) || WIFSIGNALED(status)) {
         exitStatus = WEXITSTATUS(status);
         return(true);
      }
   }
   exitStatus = 0;
   return(false);
}


// ###### Handle timer events ###############################################
EventHandlingResult ScriptingServer::syncTimerEvent(const unsigned long long now)
{
   EventHandlingResult result = EHR_Okay;

   switch(State) {
     case SS_Upload:
        printTimeStamp(stdlog);
        fprintf(stdlog, "S%04d: Reception timeout during upload to directory \"%s\" ...\n",
                RSerPoolSocketDescriptor, Directory);
        result = EHR_Abort;
      break;

     case SS_Working:
        // ====== Check whether work is complete ============================
        int                 exitStatus;
        if(hasFinishedWork(exitStatus)) {
           result = sendStatus(exitStatus);
           if(result == EHR_Okay) {
              result = performDownload();
           }
        }

        // ====== Keep-Alive handling =======================================
        else {
           // ------ Send KeepAlive -----------------------------------------
           if(WaitingForKeepAliveAck == false) {
              if(now >= LastKeepAliveTimeStamp + (1000ULL * Settings.KeepAliveInterval)) {
                 LastKeepAliveTimeStamp = now;
                 WaitingForKeepAliveAck = true;
                 result = sendKeepAliveMessage();
              }
           }
           // ====== Handle KeepAlive timeout -------------------------------
           else {
              if(now >= LastKeepAliveTimeStamp + (1000ULL * Settings.KeepAliveTimeout)) {
                 printTimeStamp(stdlog);
                 fprintf(stdlog, "S%04d: Keep-alive timeout during processing in directory \"%s\" ...\n",
                         RSerPoolSocketDescriptor, Directory);
                 result = EHR_Abort;
              }
           }
        }
      break;

     default:
      break;
   }
   setSyncTimer(now + 1000000);
   return(result);
}


// ###### Send KeppAlive message ############################################
EventHandlingResult ScriptingServer::sendKeepAliveMessage()
{
   KeepAlive keepAlive;

   keepAlive.Header.Type   = SPT_KEEPALIVE;
   keepAlive.Header.Flags  = 0x00;
   keepAlive.Header.Length = htons(sizeof(keepAlive));

   const ssize_t sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                                    (const char*)&keepAlive, sizeof(keepAlive), 0,
                                    0, htonl(PPID_SP), 0, 0, 0, 0);
   return( (sent == sizeof(keepAlive)) ? EHR_Okay : EHR_Abort );
}


// ###### Handle KeppAliveAck message #######################################
EventHandlingResult ScriptingServer::handleKeepAliveAckMessage()
{
   if(LastKeepAliveTimeStamp == 0) {
     printTimeStamp(stdlog);
     fprintf(stdlog, "S%04d: Received unexpected KeepAliveAck message!\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   WaitingForKeepAliveAck = false;
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

   const ssize_t sent = rsp_sendmsg(RSerPoolSocketDescriptor,
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
   /* ====== Check message header ======================================== */
/*
   if(ntohl(ppid) != PPID_SP) {
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Received message has wrong PPID $%08x!\n",
              RSerPoolSocketDescriptor, ntohl(ppid));
      return(EHR_Abort);
   }
*/
   if(bufferSize < sizeof(struct ScriptingCommonHeader)) {
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Received message of %u bytes does not even contain header!\n",
              RSerPoolSocketDescriptor, (unsigned int)bufferSize);
      return(EHR_Abort);
   }
   const ScriptingCommonHeader* header = (const ScriptingCommonHeader*)buffer;
   if(bufferSize < ntohs(header->Length)) {
      printTimeStamp(stdlog);
      fprintf(stdlog, "S%04d: Received message has %u bytes but %u bytes are expected!\n",
              RSerPoolSocketDescriptor,
              (unsigned int)bufferSize, ntohs(header->Length));
      return(EHR_Abort);
   }

   // ====== Handle message =================================================
   switch(State) {
      case SS_Upload:
         if(header->Type == SPT_UPLOAD) {
            return(handleUploadMessage((const Upload*)buffer));
         }
         else if(header->Type == SPT_ENVIRONMENT) {
            return(handleEnvironmentMessage((const Environment*)buffer));
         }
       break;
      case SS_Working:
         if(header->Type == SPT_KEEPALIVE) {
            return(handleKeepAliveMessage());
         }
         else if(header->Type == SPT_KEEPALIVE_ACK) {
            return(handleKeepAliveAckMessage());
         }
       break;
      default:
          // All messages here are unexpected!
       break;
   }

   // ====== Print unexpected message =======================================
   printTimeStamp(stdlog);
   fprintf(stdlog, "S%04d: Received unexpected message $%02x in state #%u!\n",
           RSerPoolSocketDescriptor, header->Type, State);
   fputs("Dump: ", stdlog);
   unsigned char* ptr = (unsigned char*)buffer;
   for(size_t i = 0;i < bufferSize;i++) {
       fprintf(stdlog, "%02x ", ptr[i]);
   }
   fputs("\n", stdlog);
   return(EHR_Abort);
}
