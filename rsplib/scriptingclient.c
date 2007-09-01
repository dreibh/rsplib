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
bool upload(int sd, const char* inputName)
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
                            0, htonl(PPID_SP), 0, 0, 0, 0);
printf("sent: %d\n", sent);
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
   const char*                  poolHandle   = "ScriptingPool";
   const char*                  inputName    = NULL;
   const char*                  outputName   = NULL;
   union rserpool_notification* notification;
   struct rsp_info              info;
   struct rsp_sndrcvinfo        rinfo;
//    char                         buffer[65536 + 4];
//    char                         str[128];
//    struct pollfd                ufds;
//    struct Ping*                 ping;
//    const struct Pong*           pong;
//    unsigned long long           now;
//    unsigned long long           nextPing;
//    uint64_t                     messageNo;
//    uint64_t                     lastReplyNo;
//    uint64_t                     recvMessageNo;
//    uint64_t                     recvReplyNo;
//    unsigned long long           lastPing;
//    ssize_t                      received;
//    ssize_t                      sent;
//    int                          result;
//    int                          flags;
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
         inputName = (const char*)&argv[i][7];
      }
      else if(!(strncmp(argv[i], "-output=" ,8))) {
         outputName = (const char*)&argv[i][8];
      }
      else {
         fprintf(stderr, "ERROR: Bad argument %s\n", argv[i]);
         fprintf(stderr, "Usage: %s [-input=Input Name] [-output=Output Name] {-poolhandle=Pool Handle}\n", argv[0]);
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


   puts("Scripting Pool User - Version 1.0");
   puts("=================================\n");
   printf("Pool Handle   = %s\n", poolHandle);
   printf("Input Name    = %s\n", inputName);
   printf("Output Name   = %s\n\n", outputName);


   if(rsp_initialize(&info) < 0) {
      fputs("ERROR: Unable to initialize rsplib\n", stderr);
      exit(1);
   }

   sd = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(sd < 0) {
      perror("Unable to create RSerPool socket");
      exit(1);
   }
   if(rsp_connect(sd, (unsigned char*)poolHandle, strlen(poolHandle)) < 0) {
      perror("Unable to connect to pool element");
      exit(1);
   }

   upload(sd, inputName);

//    messageNo =   1;
//    lastReplyNo = 0;
//    lastPing    = 0;
//    installBreakDetector();
//    while(!breakDetected()) {
//       ufds.fd     = sd;
//       ufds.events = POLLIN;
//       nextPing = lastPing + pingInterval;
//       now      = getMicroTime();
//       result = rsp_poll(&ufds, 1,
//                         (nextPing <= now) ? 0 : (int)((nextPing - now) / 1000));
//
//       /* ###### Handle Pong message ###################################### */
//       if(result > 0) {
//          if(ufds.revents & POLLIN) {
//             flags = 0;
//             received = rsp_recvmsg(sd, (char*)&buffer, sizeof(buffer),
//                                    &rinfo, &flags, 0);
//             if(received > 0) {
//                if(flags & MSG_RSERPOOL_NOTIFICATION) {
//                   notification = (union rserpool_notification*)&buffer;
//                   printf("\x1b[39;47mNotification: ");
//                   rsp_print_notification(notification, stdout);
//                   puts("\x1b[0m");
//                   if((notification->rn_header.rn_type == RSERPOOL_FAILOVER) &&
//                      (notification->rn_failover.rf_state == RSERPOOL_FAILOVER_NECESSARY)) {
//                      puts("FAILOVER...");
//                      rsp_forcefailover(sd);
//                   }
//                }
//                else {
//                   pong = (const struct Pong*)&buffer;
//                   if((pong->Header.Type == PPPT_PONG) &&
//                      (ntohs(pong->Header.Length) >= (ssize_t)sizeof(struct Pong))) {
//                      recvMessageNo = ntoh64(pong->MessageNo);
//                      recvReplyNo   = ntoh64(pong->ReplyNo);
//
//                      printf("\x1b[34m");
//                      printTimeStamp(stdout);
//                      printf("Ack #%llu from PE $%08x (reply #%llu)\x1b[0m\n",
//                             (unsigned long long)recvMessageNo, rinfo.rinfo_pe_id, (unsigned long long)recvReplyNo);
//
//                      if(recvReplyNo - lastReplyNo != 1) {
//                         printTimeStamp(stdout);
//                         printf("*** Detected gap of %lld! ***\n",
//                                (unsigned long long)recvReplyNo - (unsigned long long)lastReplyNo);
//                      }
//                      lastReplyNo = recvReplyNo;
//                   }
//                }
//             }
//          }
//       }
//
//       /* ###### Send Ping message ###################################### */
//       if(getMicroTime() - lastPing >= pingInterval) {
//          lastPing = getMicroTime();
//
//          snprintf((char*)&str,sizeof(str),
//                   "Nachricht: %llu, Zeitstempel: %llu",
//                   (unsigned long long)messageNo, (unsigned long long)lastPing);
//          size_t dataLength = strlen(str);
//
//          ping = (struct Ping*)&buffer;
//          ping->Header.Type   = PPPT_PING;
//          ping->Header.Flags  = 0x00;
//          ping->Header.Length = htons(sizeof(struct Ping) + dataLength);
//          ping->MessageNo     = hton64(messageNo);
//          memcpy(&ping->Data, str, dataLength);
//
//          sent = rsp_sendmsg(sd, (char*)ping, sizeof(struct Ping) + dataLength, 0,
//                             0, htonl(PPID_PPP), 0, 0, 0, 0);
//          if(sent > 0) {
//             printf("Message #%Ld sent\n", (unsigned long long)messageNo);
//             messageNo++;
//          }
//       }
//
//    }

usleep(5000000);

   puts("\x1b[0m\nTerminated!");
   rsp_close(sd);
   rsp_cleanup();
   rsp_freeinfo(&info);
   return 0;
}
