/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include "udplikeserver.h"
#include "timeutilities.h"
#include "loglevel.h"
#include "breakdetector.h"


// ###### Constructor #######################################################
UDPLikeServer::UDPLikeServer()
{
   RSerPoolSocketDescriptor = -1;
}


// ###### Destructor ########################################################
UDPLikeServer::~UDPLikeServer()
{
}


// ###### Handle cookie #####################################################
EventHandlingResult UDPLikeServer::handleCookieEcho(rserpool_session_t sessionID,
                                                    const char*        buffer,
                                                    size_t             bufferSize)
{
   printTimeStamp(stdout);
   printf("COOKIE ECHO for session %u\n", sessionID);
   return(EHR_Okay);
}


// ###### Handle notification ###############################################
EventHandlingResult UDPLikeServer::handleNotification(
                       const union rserpool_notification* notification)
{
   printTimeStamp(stdout);
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
   return(EHR_Okay);
}


// ###### Implementation of a simple UDP-like server ########################
void UDPLikeServer::poolElement(const char*          programTitle,
                                const char*          poolHandle,
                                struct rsp_info*     info,
                                struct rsp_loadinfo* loadinfo,
                                struct TagItem*      tags)
{
   beginLogging();
   if(rsp_initialize(info) < 0) {
      puts("ERROR: Unable to initialize rsplib Library!\n");
      finishLogging();
      return;
   }

   RSerPoolSocketDescriptor = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(RSerPoolSocketDescriptor >= 0) {
      // ====== Initialize PE settings ======================================
      struct rsp_loadinfo dummyLoadinfo;
      if(loadinfo == NULL) {
         memset(&dummyLoadinfo, 0, sizeof(dummyLoadinfo));
         loadinfo = &dummyLoadinfo;
         loadinfo->rli_policy = PPT_ROUNDROBIN;
      }

      // ====== Print program title =========================================
      puts(programTitle);
      for(size_t i = 0;i < strlen(programTitle);i++) {
         printf("=");
      }
      printf("\n\nPool Handle = %s\n\n", poolHandle);

      // ====== Register PE =================================================
      if(rsp_register(RSerPoolSocketDescriptor,
                      (const unsigned char*)poolHandle, strlen(poolHandle),
                      loadinfo, 30000) == 0) {

         // ====== Main loop ================================================
         installBreakDetector();
         while(!breakDetected()) {
            // ====== Read from socket ======================================
            char                  buffer[65536];
            int                   flags = 0;
            struct rsp_sndrcvinfo rinfo;
            ssize_t received = rsp_recvmsg(RSerPoolSocketDescriptor, (char*)&buffer, sizeof(buffer),
                                           &rinfo, &flags, 500000);

            // ====== Handle data ===========================================
            if(received > 0) {
               // ====== Handle event =======================================
               EventHandlingResult eventHandlingResult;
               if(flags & MSG_RSERPOOL_NOTIFICATION) {
                  handleNotification((const union rserpool_notification*)&buffer);
                  /*
                     We cannot shutdown or abort, since the session ID is not
                     inserted into rinfo!
                     Should we dissect the notification for the ID here?
                  */
                  eventHandlingResult = EHR_Okay;
               }
               else if(flags & MSG_RSERPOOL_COOKIE_ECHO) {
                  eventHandlingResult = handleCookieEcho(rinfo.rinfo_session,
                                                         (char*)&buffer, received);
               }
               else {
                  eventHandlingResult = handleMessage(rinfo.rinfo_session,
                                                      (char*)&buffer, received,
                                                      rinfo.rinfo_ppid,
                                                      rinfo.rinfo_stream);
               }

               // ====== Check for problems =================================
               if((eventHandlingResult == EHR_Abort) ||
                  (eventHandlingResult == EHR_Shutdown)) {
                  rsp_sendmsg(RSerPoolSocketDescriptor,
                              NULL, 0, ((eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_EOF),
                              rinfo.rinfo_session, 0, 0, 0, 0);
               }
            }
         }

         // ====== Clean up =================================================
         rsp_deregister(RSerPoolSocketDescriptor);
      }
      else {
         printf("Failed to register PE to pool %s\n", poolHandle);
      }
      rsp_close(RSerPoolSocketDescriptor);
   }
   else {
      logerror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   finishLogging();
   puts("\nTerminated!");
}
