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


/* ###### Upload input file ############################################## */
static bool upload(int                      sd,
                   const char*              inputName,
                   const unsigned long long transmitTimeout)
{
   struct Upload upload;
   size_t        dataLength;
   ssize_t       sent;
   bool          success = true;

   FILE* fh = fopen(inputName, "r");
   if(fh == NULL) {
      fprintf(stderr, "ERROR: Unable to open input file \"%s\"!\n", inputName);
      exit(1);
   }

   for(;;) {
      dataLength = fread((char*)&upload.Data, 1, sizeof(upload.Data), fh);
      if(dataLength >= 0) {
         upload.Header.Type   = SPT_UPLOAD;
         upload.Header.Flags  = 0x00;
         upload.Header.Length = htons(dataLength + sizeof(struct ScriptingCommonHeader));
         sent = rsp_sendmsg(sd, (const char*)&upload, dataLength + sizeof(struct ScriptingCommonHeader), 0,
                            0, htonl(PPID_SP), 0, 0, 0, transmitTimeout);
         if(sent <= 0) {
            success = false;
            printf("Upload error: %s\n", strerror(errno));
            break;
         }
      }
      if(dataLength <= 0) {
         break;
      }
   }
   fclose(fh);

   return(success);
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   const char*                         poolHandle = "ScriptingPool";
   const char*                         inputName  = NULL;
   const char*                         outputName = NULL;
   FILE*                               outputFile = NULL;
   struct pollfd                       ufds;
   char                                buffer[65536];
   struct rsp_info                     info;
   struct rsp_sndrcvinfo               rinfo;
   union rserpool_notification*        notification;
   const struct ScriptingCommonHeader* header;
   const struct Status*                status;
   const struct Download*              download;
   bool                                quiet             =  false;
   unsigned int                        trial             = 1;
   unsigned int                        maxRetry          = 3;
   unsigned long long                  retryDelay        =  1000000;
   unsigned long long                  transmitTimeout   = 60000000;
   unsigned long long                  keepAliveInterval =  5000000;
   unsigned long long                  keepAliveTimeout  =  5000000;
   struct KeepAlive                    keepAlive;
   bool                                keepAliveTransmitted;
   unsigned long long                  lastKeepAlive;
   unsigned long long                  nextTimer;
   unsigned long long                  now;
   ssize_t                             received;
   ssize_t                             sent;
   size_t                              length;
   bool                                success;
   uint32_t                            exitStatus;
   int                                 result;
   int                                 flags;
   int                                 sd;
   int                                 i;

   rsp_initinfo(&info);
   for(i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-input=" ,7))) {
         inputName = (const char*)&argv[i][7];
      }
      else if(!(strncmp(argv[i], "-output=" ,8))) {
         outputName = (const char*)&argv[i][8];
      }
      else if(!(strncmp(argv[i], "-maxretry=" ,10))) {
         maxRetry = atol((const char*)&argv[i][10]);
         if(maxRetry < 1) {
            maxRetry = 1;
         }
      }
      else if(!(strncmp(argv[i], "-retrydelay=" ,12))) {
         retryDelay = 1000ULL * atol((const char*)&argv[i][12]);
         if(retryDelay < 100000) {
            retryDelay = 100000;
         }
      }
      else if(!(strncmp(argv[i], "-transmittimeout=", 17))) {
         transmitTimeout = 1000ULL * atol((const char*)&argv[i][17]);
      }
      else if(!(strncmp(argv[i], "-keepaliveinterval=", 19))) {
         keepAliveInterval = 1000ULL * atol((const char*)&argv[i][19]);
      }
      else if(!(strncmp(argv[i], "-keepalivetimeout=", 18))) {
         keepAliveTimeout = 1000ULL * atol((const char*)&argv[i][18]);
      }
      else if(!(strcmp(argv[i], "-quiet"))) {
         quiet = true;
      }
      else {
         fprintf(stderr, "ERROR: Bad argument %s\n", argv[i]);
         fprintf(stderr, "Usage: %s [-input=Input Name] [-output=Output Name] {-poolhandle=Pool Handle} {-quiet} {-maxretry=Trials} {-retrydelay=milliseconds} {-transmittimeout=milliseconds} {-keepaliveinterval=milliseconds} {-keepalivetimeout=milliseconds}\n", argv[0]);
         exit(1);
      }
   }
   if( (inputName == NULL) || (outputName == NULL) ) {
      fprintf(stderr, "ERROR: Missing input or output file name!\n");
      exit(1);
   }
#ifdef ENABLE_CSP
   info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLUSER, random64());
#endif


   if(!quiet) {
      puts("Scripting Pool User - Version 1.0");
      puts("=================================\n");
      printf("Pool Handle         = %s\n", poolHandle);
      printf("Input Name          = %s\n", inputName);
      printf("Output Name         = %s\n\n", outputName);
      printf("Max Retry           = %u [ms]\n", maxRetry);
      printf("Retry Delay         = %llu [ms]\n", retryDelay / 1000);
      printf("Transmit Timeout    = %llu [ms]\n", transmitTimeout / 1000);
      printf("Keep-Alive Interval = %llu [ms]\n", keepAliveInterval / 1000);
      printf("Keep-Alive Timeout  = %llu [ms]\n", keepAliveTimeout / 1000);
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


   installBreakDetector();
   success = false;
   while( (!breakDetected()) &&
          (rsp_connect(sd, (unsigned char*)poolHandle, strlen(poolHandle), 0) < 0) ) {
      perror("Unable to connect to pool element");
      usleep(2500000);
   }
   while(!breakDetected()) {
      keepAliveTransmitted = false;

      /* ====== Upload input file ======================================== */
      puts("Upload ...");
      if(upload(sd, inputName, transmitTimeout)) {
         puts("Processing ...");

         /* ====== Wait for results and regularly send keep-alives ======= */
         lastKeepAlive = getMicroTime();
         while(!breakDetected()) {
            nextTimer     = lastKeepAlive + (keepAliveTransmitted ? keepAliveTimeout : keepAliveInterval);
            ufds.fd       = sd;
            ufds.events   = POLLIN;
            now           = getMicroTime();
            result = rsp_poll(&ufds, 1,
                              (nextTimer <= now) ? 0 : (int)((nextTimer - now) / 1000));

            /* ====== Handle results ===================================== */
            if(result > 0) {
               if(ufds.revents & POLLIN) {
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
                           break;
                        }
                     }

                     /* ====== Message =================================== */
                     else {
                        header = (const struct ScriptingCommonHeader*)buffer;
                        if( (rinfo.rinfo_ppid == ntohl(PPID_SP)) &&
                            (received >= (ssize_t)sizeof(struct ScriptingCommonHeader)) &&
                            (ntohs(header->Length) == received) ) {
                           switch(header->Type) {

                              /* ====== Download message ================= */
                              case SPT_DOWNLOAD:
                                 download = (const struct Download*)&buffer;
                                 if(!outputFile) {
                                    puts("Download ...");
                                    outputFile = fopen(outputName, "w");
                                    if(outputFile == NULL) {
                                       fprintf(stderr, "ERROR: Unable to create output file \"%s\"!\n", outputName);
                                       goto finish;
                                    }
                                 }
                                 length = received - sizeof(struct ScriptingCommonHeader);
                                 if(length > 0) {
                                    if(fwrite(&download->Data, length, 1, outputFile) != 1) {
                                       fprintf(stderr, "ERROR: Writing to output file failed!\n");
                                       goto finish;
                                    }
                                 }
                                 else {
                                    success = true;
                                    if(!quiet) {
                                       puts("Operation completed!");
                                    }
                                    goto finish;
                                 }
                               break;

                              /* ====== Keep-Alive Ack message =========== */
                              case SPT_KEEPALIVE_ACK:
                                 keepAliveTransmitted = false;
                                 lastKeepAlive        = getMicroTime();
                               break;

                              /* ====== Status message =================== */
                              case SPT_STATUS:
                                 if(received >= (ssize_t)sizeof(struct Status)) {
                                    status     = (const struct Status*)&buffer;
                                    exitStatus = ntohl(status->ExitStatus);
                                    if(exitStatus != 0) {
                                       printf("Got exit status %u from remote side!\n", exitStatus);
                                       trial++;
                                       if(trial < maxRetry) {
                                          printf("Trying again (trial %u of %u) ...\n", trial, maxRetry);
                                          goto failover;
                                       }
                                       fputs("Maximum number of trials reached -> check your input and the results!\n", stderr);
                                       fputs("Trying to download the results file for debugging ...", stderr);
                                    }
                                 }
                                 else {
                                    fputs("Invalid Status message!\n", stderr);
                                 }
                               break;

                              /* ====== Unknown message type ============= */
                              default:
                                 fprintf(stderr, "Received unexpected message type #%u!\n", header->Type);
                               break;

                           }
                        }
                        else {
                           fputs("Received invalid message!\n", stderr);
                        }
                     }
                  }
               }
            }

            /* ====== Handle timers ====================================== */
            now = getMicroTime();
            if(nextTimer <= now) {
               /* ====== KeepAlive transmission ========================== */
               if(!keepAliveTransmitted) {
                  keepAlive.Header.Type   = SPT_KEEPALIVE;
                  keepAlive.Header.Flags  = 0x00;
                  keepAlive.Header.Length = htons(sizeof(keepAlive));
                  sent = rsp_sendmsg(sd, (const char*)&keepAlive, sizeof(keepAlive), 0,
                                     0, htonl(PPID_SP), 0, 0, 0, 0);
                  if(sent <= 0) {
                     printf("Keep-Alive transmission error: %s\n", strerror(errno));
                     break;
                  }
                  keepAliveTransmitted = true;
                  lastKeepAlive        = getMicroTime();
               }

               /* ====== KeepAlive timeout =============================== */
               else {
                  printf("Keep-Alive timeout");
                  break;
               }
            }
         }
      }
failover:
      if(breakDetected()) {
         break;
      }
      puts("FAILOVER ...");
      usleep(2500000);
      if(outputFile) {
         fclose(outputFile);
         outputFile = NULL;
      }
      unlink(outputName);
      rsp_forcefailover(sd, FFF_NONE, 0);
   }

finish:
   if(!quiet) {
      puts("\x1b[0m\nTerminated!");
   }
   if(outputFile) {
      fclose(outputFile);
      outputFile = NULL;
   }
   rsp_close(sd);
   rsp_cleanup();
   rsp_freeinfo(&info);
   return((success == true) ? 0 : 1);
}
