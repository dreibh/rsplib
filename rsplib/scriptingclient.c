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

#include "rserpool.h"
#include "scriptingpackets.h"
#include "breakdetector.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "randomizer.h"
#include "netutilities.h"
#ifdef ENABLE_CSP
#include "componentstatuspackets.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define SSCR_OKAY        0
#define SSCR_COMPLETE    1
#define SSCR_FAILOVER    2

#define SSCS_WAIT_READY  1
#define SSCS_PROCESSING  2
#define SSCS_DOWNLOAD    3


static unsigned int       Status            = SSCS_WAIT_READY;
static const char*        InputName         = NULL;
static const char*        OutputName        = NULL;
static FILE*              OutputFile        = NULL;
static bool               Quiet             =  false;
static unsigned int       MaxRetry          = 3;
static unsigned long long TransmitTimeout   = 60000000;
static unsigned long long KeepAliveInterval =  5000000;
static unsigned long long KeepAliveTimeout  =  5000000;
static unsigned long long RetryDelay        = 10000000;

static unsigned int       Trial             = 1;
static uint32_t           ExitStatus        = 0;
static bool               KeepAliveTransmitted;
static unsigned long long LastKeepAlive;



/* ###### Upload input file ############################################## */
static unsigned int performUpload(int sd)
{
   struct Upload upload;
   size_t        dataLength;
   ssize_t       sent;

   FILE* fh = fopen(InputName, "r");
   if(fh == NULL) {
      fprintf(stderr, "ERROR: Unable to open input file \"%s\"!\n", InputName);
      exit(1);
   }

   puts("Uploading ...");
   for(;;) {
      dataLength = fread((char*)&upload.Data, 1, sizeof(upload.Data), fh);
      if(dataLength >= 0) {
         upload.Header.Type   = SPT_UPLOAD;
         upload.Header.Flags  = 0x00;
         upload.Header.Length = htons(dataLength + sizeof(struct ScriptingCommonHeader));
         sent = rsp_sendmsg(sd, (const char*)&upload, dataLength + sizeof(struct ScriptingCommonHeader), 0,
                            0, htonl(PPID_SP), 0, 0, 0, TransmitTimeout);
         if(sent <= 0) {
            printf("Upload error: %s\n", strerror(errno));
            return(SSCR_FAILOVER);
         }
         if(dataLength == 0) {
            break;
         }
      }
      else {
         fprintf(stderr, "ERROR: Reading failed in \"%s\"!\n", InputName);
         exit(1);
      }
   }
   fclose(fh);

   puts("Upload complete.");
   return(SSCR_OKAY);
}


/* ###### Handle Status message ############################################# */
static unsigned int handleStatus(const struct Status* status,
                                 const size_t         length)
{
   if(length >= sizeof(struct Status)) {
      ExitStatus = ntohl(status->Status);
      if(ExitStatus != 0) {
         printf("Got exit status %u from remote side!\n", ExitStatus);
         Trial++;
         if(Trial < MaxRetry) {
            printf("Trying again (Trial %u of %u) ...\n", Trial, MaxRetry);
            return(SSCR_FAILOVER);
         }
         fputs("Maximum number of Trials reached -> check your input and the results!\n", stderr);
         fputs("Trying to download the results file for debugging ...", stderr);
      }
      return(SSCR_OKAY);
   }
   else {
      fputs("Invalid Status message!\n", stderr);
      return(SSCR_FAILOVER);
   }
}


/* ###### Send KeepAlive message ######################################### */
static unsigned int sendKeepAlive(int sd)
{
   struct KeepAlive keepAlive;
   ssize_t          sent;

   keepAlive.Header.Type   = SPT_KEEPALIVE;
   keepAlive.Header.Flags  = 0x00;
   keepAlive.Header.Length = htons(sizeof(keepAlive));
   sent = rsp_sendmsg(sd, (const char*)&keepAlive, sizeof(keepAlive), 0,
                      0, htonl(PPID_SP), 0, 0, 0, 0);
   if(sent <= 0) {
      printf("Keep-Alive transmission error: %s\n", strerror(errno));
      return(SSCR_FAILOVER);
   }
   KeepAliveTransmitted = true;
   LastKeepAlive        = getMicroTime();
   return(SSCR_OKAY);
}


/* ###### Handle Download message ######################################## */
static unsigned int handleDownload(const struct Download* download,
                                   const size_t           length)
{
   size_t dataLength;

   /* ====== Create file, if necessary =================================== */
   if(!OutputFile) {
      puts("Downloading ...");
      OutputFile = fopen(OutputName, "w");
      if(OutputFile == NULL) {
         fprintf(stderr, "ERROR: Unable to create output file \"%s\"!\n", OutputName);
         exit(1);
      }
   }

   /* ====== Write data ================================================== */
   dataLength = length - sizeof(struct ScriptingCommonHeader);
   if(dataLength > 0) {
      if(fwrite(&download->Data, dataLength, 1, OutputFile) != 1) {
         fprintf(stderr, "ERROR: Writing to output file failed!\n");
         fclose(OutputFile);
         exit(1);
      }
   }
   else {
      fclose(OutputFile);
      OutputFile = NULL;
      if(!Quiet) {
         puts("Download completed!");
      }
      return(SSCR_COMPLETE);
   }

   return(SSCR_OKAY);
}


/* ###### Handle message ################################################# */
static unsigned int handleMessage(int                                 sd,
                                  const struct ScriptingCommonHeader* header,
                                  const size_t                        length,
                                  const uint32_t                      ppid)
{
   /* ====== Check message header ======================================== */
   if( (ppid != PPID_SP) ||
       (length < sizeof(struct ScriptingCommonHeader)) ) {
      fputs("Received invalid message!\n", stderr);
      return(SSCR_FAILOVER);
   }


   /* ====== Handle message ============================================== */
   switch(Status) {

      /* ====== Wait for Ready message =================================== */
      case SSCS_WAIT_READY:
         switch(header->Type) {
            case SPT_READY:
               Status = SSCS_PROCESSING;
               puts("Server is ready");
               return(performUpload(sd));
             break;
         }
        break;

      /* ====== Processing =============================================== */
      case SSCS_PROCESSING:
         switch(header->Type) {
            case SPT_KEEPALIVE_ACK:
               KeepAliveTransmitted = false;
               LastKeepAlive        = getMicroTime();
               return(SSCR_OKAY);
             break;
            case SPT_STATUS:
               Status = SSCS_DOWNLOAD;
               return(handleStatus((const struct Status*)header, length));
             break;
         }
       break;

      /* ====== Processing =============================================== */
      case SSCS_DOWNLOAD:
         switch(header->Type) {
            case SPT_DOWNLOAD:
               return(handleDownload((const struct Download*)header, length));
             break;
            case SPT_KEEPALIVE_ACK:
               KeepAliveTransmitted = false;
               LastKeepAlive        = getMicroTime();
               return(SSCR_OKAY);
             break;
         }
       break;
   }


   /* ====== Unexpected message ========================================== */
   fprintf(stderr, "Unexpected message $%02x in status #%u\n", header->Type, Status);
   return(SSCR_FAILOVER);
}



/* ###### Main program ################################################### */
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
      else if(!(strncmp(argv[i], "-input=" ,7))) {
         InputName = (const char*)&argv[i][7];
      }
      else if(!(strncmp(argv[i], "-output=" ,8))) {
         OutputName = (const char*)&argv[i][8];
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
         fprintf(stderr, "Usage: %s [-input=Input Name] [-output=Output Name] {-poolhandle=Pool Handle} {-quiet} {-maxretry=Trials} {-retrydelay=milliseconds} {-transmittimeout=milliseconds} {-keepaliveinterval=milliseconds} {-keepalivetimeout=milliseconds}\n", argv[0]);
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


   if(!Quiet) {
      puts("Scripting Pool User - Version 1.0");
      puts("=================================\n");
      printf("Pool Handle         = %s\n", poolHandle);
      printf("Input Name          = %s\n", InputName);
      printf("Output Name         = %s\n\n", OutputName);
      printf("Max Retry           = %u [ms]\n", MaxRetry);
      printf("Retry Delay         = %llu [ms]\n", RetryDelay / 1000);
      printf("Transmit Timeout    = %llu [ms]\n", TransmitTimeout / 1000);
      printf("Keep-Alive Interval = %llu [ms]\n", KeepAliveInterval / 1000);
      printf("Keep-Alive Timeout  = %llu [ms]\n\n", KeepAliveTimeout / 1000);
   }


   if(rsp_initialize(&info) < 0) {
      fputs("ERROR: Unable to initialize rsplib\n", stderr);
      exit(1);
   }

   sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(sd < 0) {
      perror("Unable to create RSerPool socket");
      exit(1);
   }

   if(rsp_connect(sd, (unsigned char*)poolHandle, strlen(poolHandle), 0) < 0) {
      perror("Unable to connect to pool element");
      exit(1);
   }

   installBreakDetector();
   result = SSCR_OKAY;
   while( (!breakDetected()) && (result != SSCR_COMPLETE) ) {
      KeepAliveTransmitted = false;
      LastKeepAlive        = getMicroTime();
      result               = SSCR_OKAY;

      /* ====== Wait for results and regularly send keep-alives ======= */
      nextTimer   = LastKeepAlive + (KeepAliveTransmitted ? KeepAliveTimeout : KeepAliveInterval);
      ufds.fd     = sd;
      ufds.events = POLLIN;
      now         = getMicroTime();
      events = rsp_poll(&ufds, 1,
                        (nextTimer <= now) ? 0 : (int)((nextTimer - now) / 1000));

      /* ====== Handle results ===================================== */
      if( (events > 0) && (ufds.revents & POLLIN) ) {
         flags = 0;
         received = rsp_recvmsg(sd, (char*)&buffer, sizeof(buffer),
                                &rinfo, &flags, 0);
         if(received > 0) {

            /* ====== Notification ================================= */
            if(flags & MSG_RSERPOOL_NOTIFICATION) {
               notification = (union rserpool_notification*)&buffer;
               printf("\x1b[39;47mNotification: ");
               rsp_print_notification(notification, stdout);
               puts("\x1b[0m");
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
      if(nextTimer <= getMicroTime()) {
         /* ====== KeepAlive transmission ========================== */
         if(!KeepAliveTransmitted) {
            result = sendKeepAlive(sd);
         }

         /* ====== KeepAlive timeout =============================== */
         else {
            puts("Keep-Alive timeout");
            result = SSCR_FAILOVER;
         }
      }

      /* ====== Failover ================================================= */
      if(result == SSCR_FAILOVER) {
         if(OutputFile) {
            fclose(OutputFile);
            OutputFile = NULL;
         }
         puts("FAILOVER ...");
         nextTimer = getMicroTime() + RetryDelay;
         do {
            usleep(500000);
         } while( (getMicroTime() < nextTimer) && (!breakDetected()) );
         unlink(OutputName);
         Status = SSCS_WAIT_READY;
         rsp_forcefailover(sd, FFF_NONE, 0);
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
   return((result == SSCR_COMPLETE) ? 0 : 1);
}
