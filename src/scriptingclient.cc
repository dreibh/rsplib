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
 * Copyright (C) 2002-2025 by Thomas Dreibholz
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

#include "rserpool.h"
#include "scriptingpackets.h"
#include "breakdetector.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "randomizer.h"
#include "netutilities.h"
#include "sha1.h"
#include "memfile.h"
#ifdef ENABLE_CSP
#include "componentstatuspackets.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>


#define SSCR_OKAY        0
#define SSCR_COMPLETE    1
#define SSCR_FAILOVER    2

#define SSCS_WAIT_SERVER_READY     1
#define SSCS_WAIT_ENVIRONMENT      2
#define SSCS_WAIT_START_PROCESSING 3
#define SSCS_PROCESSING            4
#define SSCS_DOWNLOAD              5


static uint8_t            EnvironmentHash[SE_HASH_SIZE];
static unsigned long long EnvironmentSize   = 0;
static const char*        EnvironmentName   = NULL;
static const char*        InputName         = NULL;
static const char*        OutputName        = NULL;
static FILE*              OutputFile        = NULL;
static bool               Quiet             = false;
static unsigned int       MaxRetry          = 3;
static unsigned long long TransmitTimeout   = 59500000;
static unsigned long long KeepAliveInterval = 10000000;
static unsigned long long KeepAliveTimeout  = 10000000;
static unsigned long long RetryDelay        = 10000000;
static const char*        RunID             = NULL;
static unsigned int       State             = SSCS_WAIT_SERVER_READY;
static unsigned int       Trial             = 1;
static uint32_t           ExitStatus        = 0;
static bool               HasAssignedPE     = false;
static char               InfoString[SR_MAX_INFOSIZE + 1] = "";
static bool               KeepAliveTransmitted;
static unsigned long long LastKeepAlive;


// ###### Randomize waiting time ############################################
static unsigned long long randomizeWaitingTime(const unsigned long long interval)
{
   const double originalInterval = (double)interval;
   const double variation    = 0.250 * originalInterval;
   const double nextInterval = originalInterval - (variation / 2.0) +
                                  variation * ((double)rand() / (double)RAND_MAX);
   return((unsigned long long)nextInterval);
}


// ###### Write new log line ################################################
static void newLogLine(FILE* fh)
{
   printTimeStamp(fh);
   if(RunID) {
      if(InfoString[0] != 0x00) {
         fprintf(fh, "[%s@%s] ", RunID, InfoString);
      }
      else {
         fprintf(fh, "[%s] ", RunID);
      }
   }
}


// ###### Upload input file #################################################
static unsigned int performUpload(int sd, const char* name)
{
   MemFile* memFile = openMemFile(name);
   if(memFile == NULL) {
      printTimeStamp(stderr);
      fprintf(stderr, "ERROR: Unable to open and memory-map file \"%s\"!\n", name);
      exit(1);
   }

   newLogLine(stdout);
   printf("Uploading %llu bytes ...\n", (unsigned long long)memFile->Length);
   fflush(stdout);

   size_t totalBytesSent = 0;
   for(;;) {
      Upload upload;

      size_t dataLength = memFile->Length - totalBytesSent;
      if(dataLength > sizeof(upload.Data)) {
         dataLength = sizeof(upload.Data);
      }

      // NOTE: end-of-file will be indicated by empty SPT_UPLOAD message!
      upload.Header.Type   = SPT_UPLOAD;
      upload.Header.Flags  = 0x00;
      upload.Header.Length = htons(dataLength + sizeof(struct ScriptingCommonHeader));
      memcpy((void*)&upload.Data, &memFile->Address[totalBytesSent], dataLength);
      const ssize_t sent = rsp_sendmsg(sd, (const char*)&upload, dataLength + sizeof(struct ScriptingCommonHeader), 0,
                                       0, htonl(PPID_SP), 0, 0, 0, (int)(TransmitTimeout / 1000));
      if(sent != (ssize_t)dataLength + (ssize_t)sizeof(struct ScriptingCommonHeader)) {
         closeMemFile(memFile);
         newLogLine(stdout);
         printf("Upload error: %s\n", strerror(errno));
         fflush(stdout);
         return(SSCR_FAILOVER);
      }

      if(dataLength <= 0) {
         break;
      }

      totalBytesSent += dataLength;
   }
   closeMemFile(memFile);

   newLogLine(stdout);
   puts("Upload complete.");
   fflush(stdout);
   return(SSCR_OKAY);
}


// ###### Handle Status message ################################################
static unsigned int handleStatusMessage(const struct Status* status,
                                        const size_t         length)
{
   if(length >= sizeof(struct Status)) {
      ExitStatus = ntohl(status->Status);
      if(ExitStatus != 0) {
         newLogLine(stdout);
         printf("Got exit status %u from remote side!\n", ExitStatus);
         fflush(stdout);
         Trial++;
         if(Trial < MaxRetry) {
            newLogLine(stdout);
            printf("Trying again (Trial %u of %u) ...\n", Trial, MaxRetry);
            fflush(stdout);
            return(SSCR_FAILOVER);
         }
         newLogLine(stderr);
         fputs("Maximum number of trials reached -> check your input and the results!\n", stderr);
         fputs("Trying to download the results file for debugging ...", stderr);
         /* Final trial has failed: continue with download of the error output! */
      }
      return(SSCR_OKAY);
   }
   else {
      newLogLine(stdout);
      puts("Invalid Status message!");
      fflush(stdout);
      return(SSCR_FAILOVER);
   }
}


// ###### Send KeepAlive message ############################################
static unsigned int sendKeepAliveMessage(int sd)
{
   struct KeepAlive keepAlive;
   ssize_t          sent;

   keepAlive.Header.Type   = SPT_KEEPALIVE;
   keepAlive.Header.Flags  = 0x00;
   keepAlive.Header.Length = htons(sizeof(keepAlive));
   sent = rsp_sendmsg(sd, (const char*)&keepAlive, sizeof(keepAlive), 0,
                      0, htonl(PPID_SP), 0, 0, 0, 0);
   if(sent != sizeof(keepAlive)) {
      if((State != SSCS_WAIT_SERVER_READY) &&
         (State != SSCS_WAIT_ENVIRONMENT) &&
         (State != SSCS_WAIT_START_PROCESSING)) {
         newLogLine(stdout);
         printf("Keep-Alive transmission error in state #%u: %s\n", State, strerror(errno));
         fflush(stdout);
      }
      else {
         newLogLine(stdout);
         puts("Trying again ...");
         fflush(stdout);
      }
      return(SSCR_FAILOVER);
   }
   KeepAliveTransmitted = true;
   LastKeepAlive        = getMicroTime();
   return(SSCR_OKAY);
}


// ###### Handle KeepAliveAck message #######################################
static unsigned int handleKeepAliveAckMessage()
{
   KeepAliveTransmitted = false;
   LastKeepAlive        = getMicroTime();
   return(SSCR_OKAY);
}


// ###### Send KeepAliveAck message #########################################
static unsigned int sendKeepAliveAckMessage(int sd)
{
   struct KeepAlive keepAliveAck;
   ssize_t          sent;

   keepAliveAck.Header.Type   = SPT_KEEPALIVE_ACK;
   keepAliveAck.Header.Flags  = 0x00;
   keepAliveAck.Header.Length = htons(sizeof(keepAliveAck));
   sent = rsp_sendmsg(sd, (const char*)&keepAliveAck, sizeof(keepAliveAck), 0,
                      0, htonl(PPID_SP), 0, 0, 0, 0);
   return( (sent == sizeof(keepAliveAck)) ? SSCR_OKAY : SSCR_FAILOVER );
}


// ###### Handle Ready message ##############################################
static void handleNotReadyMessage(const int              sd,
                                  const struct NotReady* notReady,
                                  const size_t           length)
{
   if(length >= sizeof(struct NotReady) - SR_MAX_INFOSIZE) {
      /* ====== Get info string ========================================== */
      size_t i;
      for(i = 0;i < length - (sizeof(struct NotReady) - SR_MAX_INFOSIZE);i++) {
         if(i == SR_MAX_INFOSIZE) {
            break;
         }
         InfoString[i] = isprint(notReady->Info[i]) ? notReady->Info[i] : '.';
      }
      InfoString[i] = 0x00;
   }
   else {
      InfoString[0] = 0x00;
   }
   if(!Quiet) {
      newLogLine(stdout);
      printf("Server %s is not ready yet => trying another one.\n", InfoString);
      fflush(stdout);
   }
}


// ###### Handle Ready message ##############################################
static unsigned int handleReadyMessage(const int           sd,
                                       const struct Ready* ready,
                                       const size_t        length)
{
   if(length < sizeof(struct Ready) - SR_MAX_INFOSIZE) {
      newLogLine(stdout);
      puts("Received invalid Ready message!");
      fflush(stdout);
      return(SSCR_FAILOVER);
   }

   /* ====== Get info string ============================================= */
   size_t i;
   for(i = 0;i < length - (sizeof(struct Ready) - SR_MAX_INFOSIZE);i++) {
      if(i == SR_MAX_INFOSIZE) {
         break;
      }
      InfoString[i] = isprint(ready->Info[i]) ? ready->Info[i] : '.';
   }
   InfoString[i] = 0x00;
   HasAssignedPE = true;

   /* ====== Proceed with uploading the work package ===================== */
   newLogLine(stdout);
   printf("Server %s is ready!\n", InfoString);
   fflush(stdout);

   /* ====== Request environment ========================================= */
   if(EnvironmentName != NULL) {
      State = SSCS_WAIT_ENVIRONMENT;
      struct Environment environment;
      environment.Header.Type   = SPT_ENVIRONMENT;
      environment.Header.Flags  = 0x00;
      environment.Header.Length = htons(sizeof(environment));
      memcpy(&environment.Hash, EnvironmentHash, sizeof(EnvironmentHash));
      ssize_t sent = rsp_sendmsg(sd, (const char*)&environment, sizeof(environment), 0,
                                 0, htonl(PPID_SP), 0, 0, 0, (int)(TransmitTimeout / 1000));
      if(sent != (ssize_t)sizeof(environment)) {
         newLogLine(stdout);
         printf("Environment query error: %s\n", strerror(errno));
         fflush(stdout);
         return(SSCR_FAILOVER);
      }
   }
   else {
      State = SSCS_WAIT_START_PROCESSING;
   }

   /* ====== Continue with input upload ================================== */
   return(performUpload(sd, InputName));
}


// ###### Handle Ready message ##############################################
static unsigned int handleEnvironmentMessage(const int                 sd,
                                             const struct Environment* environment,
                                             const size_t              length)
{
   if(length < sizeof(struct Environment)) {
      newLogLine(stdout);
      puts("Received invalid Environment message!");
      fflush(stdout);
      return(SSCR_FAILOVER);
   }

   /* ====== Proceed with uploading the work package ===================== */
   State = SSCS_WAIT_START_PROCESSING;
   newLogLine(stdout);
   if(environment->Header.Flags & SEF_UPLOAD_NEEDED) {
      if(EnvironmentName != NULL) {
         printf("Server %s needs environment\n", InfoString);
         fflush(stdout);
         return(performUpload(sd, EnvironmentName));
      }
      else {
         printf("Server %s has requested environment, but there is no environment?!\n", InfoString);
         fflush(stdout);
         return(SSCR_FAILOVER);
      }
   }
   else {
      printf("Server %s has cached environment -> skipping upload\n", InfoString);
      fflush(stdout);
      return(SSCR_OKAY);
   }
}

// ###### Handle Status message #############################################
static unsigned int serverStartsProcessing(const int            sd,
                                           const struct Status* status,
                                           const size_t         length)
{
   if(length < sizeof(struct Status)) {
      newLogLine(stdout);
      puts("Received invalid Status message!");
      fflush(stdout);
      return(SSCR_FAILOVER);
   }

   if(ntohl(status->Status) == 0) {
      State = SSCS_PROCESSING;
      newLogLine(stdout);
      printf("Server %s is processing ...\n", InfoString);
      fflush(stdout);
   }
   else {
      State = SSCR_FAILOVER;
      newLogLine(stdout);
      printf("Server did not accept upload; status was %u\n", ntohl(status->Status));
      fflush(stdout);
   }
   return(SSCR_OKAY);
}


// ###### Handle Download message ###########################################
static unsigned int handleDownloadMessage(const struct Download* download,
                                          const size_t           length)
{
   /* ====== Create file, if necessary =================================== */
   if(!OutputFile) {
      newLogLine(stdout);
      puts("Downloading ...");
      fflush(stdout);
      OutputFile = fopen(OutputName, "w");
      if(OutputFile == NULL) {
         newLogLine(stderr);
         fprintf(stderr, "ERROR: Unable to create output file \"%s\"!\n", OutputName);
         exit(1);
      }
   }

   /* ====== Write data ================================================== */
   size_t dataLength = length - sizeof(struct ScriptingCommonHeader);
   if(dataLength > 0) {
      if(fwrite(&download->Data, dataLength, 1, OutputFile) != 1) {
         newLogLine(stderr);
         fprintf(stderr, "ERROR: Writing to output file failed!\n");
         fclose(OutputFile);
         exit(1);
      }
   }
   else {
      fclose(OutputFile);
      OutputFile = NULL;
      if(!Quiet) {
         newLogLine(stdout);
         puts("Download completed!");
         fflush(stdout);
      }
      return(SSCR_COMPLETE);
   }

   return(SSCR_OKAY);
}


// ###### Handle message ####################################################
static unsigned int handleMessage(int                                 sd,
                                  const struct ScriptingCommonHeader* header,
                                  const size_t                        length,
                                  const uint32_t                      ppid)
{
   /* printf("Message %u in state %u ...\n", header->Type, State); */

   /* ====== Check message header ======================================== */
/*
   if(ppid != PPID_SP) {
      newLogLine(stdout);
      printf("Received message has wrong PPID $%08x!\n", ppid);
      fflush(stdout);
      return(SSCR_FAILOVER);
   }
*/
   if(length < sizeof(struct ScriptingCommonHeader)) {
      newLogLine(stdout);
      printf("Received message of %u bytes does not even contain header!\n",
             (unsigned int)length);
      fflush(stdout);
      return(SSCR_FAILOVER);
   }
   if(ntohs(header->Length) != length) {
      newLogLine(stdout);
      printf("Received message has %u bytes but %u bytes are expected!\n",
             (unsigned int)length, ntohs(header->Length));
      fflush(stdout);
      return(SSCR_FAILOVER);
   }

   /* ====== Handle message ============================================== */
   switch(State) {

      /* ====== Wait for Ready message to start upload =================== */
      case SSCS_WAIT_SERVER_READY:
         switch(header->Type) {
            case SPT_READY:
               return(handleReadyMessage(sd, (const struct Ready*)header, length));
            case SPT_NOTREADY:
               handleNotReadyMessage(sd, (const struct NotReady*)header, length);
               return(SSCR_FAILOVER);
         }
        break;

      /* ====== Wait for Ready message to start upload =================== */
      case SSCS_WAIT_ENVIRONMENT:
         switch(header->Type) {
            case SPT_ENVIRONMENT:
               return(handleEnvironmentMessage(sd, (const struct Environment*)header, length));
         }
        break;

      /* ====== Wait for Ready message to start upload =================== */
      case SSCS_WAIT_START_PROCESSING:
         switch(header->Type) {
            case SPT_STATUS:
               return(serverStartsProcessing(sd, (const struct Status*)header, length));
         }
        break;

      /* ====== Processing =============================================== */
      case SSCS_PROCESSING:
         switch(header->Type) {
            case SPT_KEEPALIVE_ACK:
               return(handleKeepAliveAckMessage());
            case SPT_KEEPALIVE:
               return(sendKeepAliveAckMessage(sd));
            case SPT_STATUS:
               State = SSCS_DOWNLOAD;
               return(handleStatusMessage((const struct Status*)header, length));
         }
       break;

      /* ====== Processing =============================================== */
      case SSCS_DOWNLOAD:
         switch(header->Type) {
            case SPT_DOWNLOAD:
               return(handleDownloadMessage((const struct Download*)header, length));
            case SPT_KEEPALIVE_ACK:
               return(handleKeepAliveAckMessage());
         }
       break;
   }


   /* ====== Unexpected message ========================================== */
   newLogLine(stdout);
   printf("Unexpected message $%02x in state #%u (length=%u, ppid=$%08x)\n",
          header->Type, State, (unsigned int)length, ppid);
   fflush(stdout);
   return(SSCR_FAILOVER);
}


// ###### Main program ######################################################
int main(int argc, char** argv)
{
   const char*                  poolHandle = "ScriptingPool";
   struct pollfd                ufds;
   char                         buffer[65536];
   struct rsp_info              info;
   struct rsp_sndrcvinfo        rinfo;
   union rserpool_notification* notification;
   unsigned long long           nextTimer;
   unsigned long long           now;
   ssize_t                      received;
   unsigned int                 result;
   int                          events;
   int                          flags;
   int                          sd;
   int                          i;

   rsp_initinfo(&info);
   for(i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-environment=" ,13))) {
         EnvironmentName = (const char*)&argv[i][13];
         if(EnvironmentName[0] == 0x00) {
            EnvironmentName = NULL;
         }
      }
      else if(!(strncmp(argv[i], "-input=" ,7))) {
         InputName = (const char*)&argv[i][7];
      }
      else if(!(strncmp(argv[i], "-output=" ,8))) {
         OutputName = (const char*)&argv[i][8];
      }
      else if(!(strncmp(argv[i], "-runid=" ,7))) {
         RunID = (const char*)&argv[i][7];
      }
      else if(!(strncmp(argv[i], "-maxretry=" ,10))) {
         MaxRetry = atol((const char*)&argv[i][10]);
         if(MaxRetry < 1) {
            MaxRetry = 1;
         }
      }
      else if(!(strncmp(argv[i], "-retrydelay=" ,12))) {
         RetryDelay = 1000ULL * atol((const char*)&argv[i][12]);
         if(RetryDelay < 100000) {
            RetryDelay = 100000;
         }
      }
      else if(!(strncmp(argv[i], "-transmittimeout=", 17))) {
         TransmitTimeout = 1000ULL * atol((const char*)&argv[i][17]);
      }
      else if(!(strncmp(argv[i], "-keepaliveinterval=", 19))) {
         KeepAliveInterval = 1000ULL * atol((const char*)&argv[i][19]);
      }
      else if(!(strncmp(argv[i], "-keepalivetimeout=", 18))) {
         KeepAliveTimeout = 1000ULL * atol((const char*)&argv[i][18]);
      }
      else if(!(strcmp(argv[i], "-quiet"))) {
         Quiet = true;
      }
      else {
         fprintf(stderr, "ERROR: Bad argument %s\n", argv[i]);
         fprintf(stderr, "Usage: %s [-environment=Environment Name] [-input=Input Name] [-output=Output Name] {-poolhandle=Pool Handle} {-quiet} {-maxretry=Trials} {-retrydelay=milliseconds} {-transmittimeout=milliseconds} {-keepaliveinterval=milliseconds} {-keepalivetimeout=milliseconds}\n", argv[0]);
         exit(1);
      }
   }
   if( (InputName == NULL) || (OutputName == NULL) ) {
      fprintf(stderr, "ERROR: Missing input or output file name!\n");
      exit(1);
   }
#ifdef ENABLE_CSP
   info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLUSER, random64());
#endif

   if(EnvironmentName) {
      EnvironmentSize = sha1_computeHashOfFile(EnvironmentName, (uint8_t*)&EnvironmentHash);
      if(EnvironmentSize == 0) {
         fprintf(stderr, "ERROR: Unable to compute hash of environment file %s\n",
                 EnvironmentName);
         exit(1);
      }
   }


   if(!Quiet) {
      puts("Scripting Pool User - Version 2.0");
      puts("=================================\n");
      printf("Pool Handle         = %s\n", poolHandle);
      printf("Environment Name    = ");
      if(EnvironmentName) {
         printf("%s (", EnvironmentName);
         for(i = 0; i < (int)sizeof(EnvironmentHash); i++) {
            printf("%02x", (unsigned int)EnvironmentHash[i]);
         }
         printf(", %llu KiB)\n", EnvironmentSize / 1024);
      }
      else {
         puts("(none)");
      }
      printf("Input Name          = %s\n", InputName);
      printf("Output Name         = %s\n\n", OutputName);
      printf("Max Retry           = %u\n", MaxRetry);
      printf("Retry Delay         = %llu [ms]\n", RetryDelay / 1000);
      printf("Transmit Timeout    = %llu [ms]\n", TransmitTimeout / 1000);
      printf("Keep-Alive Interval = %llu [ms]\n", KeepAliveInterval / 1000);
      printf("Keep-Alive Timeout  = %llu [ms]\n\n", KeepAliveTimeout / 1000);
   }


   if(rsp_initialize(&info) < 0) {
      newLogLine(stderr);
      fputs("ERROR: Unable to initialize rsplib\n", stderr);
      exit(1);
   }

   sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(sd < 0) {
      newLogLine(stderr);
      perror("Unable to create RSerPool socket");
      exit(1);
   }

   newLogLine(stdout);
   printf("Connecting to pool %s ...\n", poolHandle);
   fflush(stdout);

   if(rsp_connect(sd, (unsigned char*)poolHandle, strlen(poolHandle), 0) < 0) {
      newLogLine(stderr);
      perror("Unable to connect to pool element");
      exit(1);
   }

   installBreakDetector();
   result = SSCR_OKAY;
   KeepAliveTransmitted = false;
   LastKeepAlive        = getMicroTime();
   while( (!breakDetected()) && (result != SSCR_COMPLETE) ) {
      result = SSCR_OKAY;

      /* ====== Wait for results and regularly send keep-alives ======= */
      nextTimer   = LastKeepAlive + (KeepAliveTransmitted ? KeepAliveTimeout : KeepAliveInterval);
      ufds.fd     = sd;
      ufds.events = POLLIN;
      now         = getMicroTime();
      events = rsp_poll(&ufds, 1,
                        (nextTimer <= now) ? 0 : (int)ceil((double)(nextTimer - now) / 1000.0));

      /* ====== Handle results ===================================== */
      if( (events > 0) && (ufds.revents & POLLIN) ) {
         flags = 0;
         received = rsp_recvmsg(sd, (char*)&buffer, sizeof(buffer),
                                &rinfo, &flags, 0);
         if(received > 0) {

            /* ====== Notification ================================= */
            if(flags & MSG_RSERPOOL_NOTIFICATION) {
               notification = (union rserpool_notification*)&buffer;
               if(HasAssignedPE == true) {   /* Skip printing if no PE has accepted the request yet. */
                  newLogLine(stdout);
                  printf("\x1b[39;47mNotification: ");
                  rsp_print_notification(notification, stdout);
                  puts("\x1b[0m");
                  fflush(stdout);
               }
               if((notification->rn_header.rn_type == RSERPOOL_FAILOVER) &&
                  (notification->rn_failover.rf_state == RSERPOOL_FAILOVER_NECESSARY)) {
                  result = SSCR_FAILOVER;
               }
            }

            /* ====== Message =================================== */
            else {
               result = handleMessage(sd,
                                      (const struct ScriptingCommonHeader*)&buffer,
                                      (size_t)received, ntohl(rinfo.rinfo_ppid));
            }
         }
      }

      /* ====== Handle timers ====================================== */
      if( (result == SSCR_OKAY) && (nextTimer <= getMicroTime()) ) {
         /* ====== KeepAlive transmission ========================== */
         if(!KeepAliveTransmitted) {
            result = sendKeepAliveMessage(sd);
         }

         /* ====== KeepAlive timeout =============================== */
         else {
            newLogLine(stdout);
            puts("Keep-Alive timeout");
            fflush(stdout);
            result = SSCR_FAILOVER;
         }
      }

      /* ====== Failover ================================================= */
      if(result == SSCR_FAILOVER) {
         if(OutputFile) {
            fclose(OutputFile);
            OutputFile = NULL;
         }
         unlink(OutputName);
         if(HasAssignedPE == true) {   /* Skip printing if no PE has accepted the request yet. */
            newLogLine(stdout);
            puts("FAILOVER ...");
            fflush(stdout);
            HasAssignedPE = false;   /* All work is lost -> start from scratch ... */
         }
         InfoString[0] = 0x00;
         nextTimer = getMicroTime() + randomizeWaitingTime(RetryDelay);
         do {
            usleep(500000);
         } while( (getMicroTime() < nextTimer) && (!breakDetected()) );
         State = SSCS_WAIT_SERVER_READY;
         rsp_forcefailover(sd, FFF_NONE, 0);
         KeepAliveTransmitted = false;
         LastKeepAlive        = getMicroTime();
      }
   }

   if(OutputFile) {
      fclose(OutputFile);
      OutputFile = NULL;
   }
   if(!Quiet) {
      puts("\x1b[0m\nTerminated!");
   }
   rsp_close(sd);
   rsp_cleanup();
   rsp_freeinfo(&info);
   return(((result == SSCR_COMPLETE) && (ExitStatus == 0)) ? 0 : 1);
}
