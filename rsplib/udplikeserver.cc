/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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
#include "tagitem.h"


// ###### Constructor #######################################################
UDPLikeServer::UDPLikeServer()
{
   RSerPoolSocketDescriptor = -1;
   NextTimerTimeStamp       = 0;
   Load                     = 0;
}


// ###### Destructor ########################################################
UDPLikeServer::~UDPLikeServer()
{
   RSerPoolSocketDescriptor = -1;
}


// ##### Get load ###########################################################
double UDPLikeServer::getLoad() const
{
   return((double)Load / (double)PPV_MAX_LOAD);
}


// ##### Set load ###########################################################
void UDPLikeServer::setLoad(double load)
{
   if((load < 0.0) || (load > 1.0)) {
      fputs("ERROR: Invalid load setting!\n", stderr);
      return;
   }
   Load = (unsigned int)rint(load * (double)PPV_MAX_LOAD);
}


// ###### Start timer #######################################################
void UDPLikeServer::startTimer(unsigned long long timeStamp)
{
   CHECK(timeStamp > 0);
   NextTimerTimeStamp = timeStamp;
}


// ###### Stop timer ########################################################
void UDPLikeServer::stopTimer()
{
   NextTimerTimeStamp = 0;
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
void UDPLikeServer::handleNotification(const union rserpool_notification* notification)
{
   printTimeStamp(stdout);
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
}


// ###### Handle message ####################################################
EventHandlingResult UDPLikeServer::handleMessage(rserpool_session_t sessionID,
                                                 const char*        buffer,
                                                 size_t             bufferSize,
                                                 uint32_t           ppid,
                                                 uint16_t           streamID)
{
   return(EHR_Abort);
}


// ###### Handle timer ######################################################
void UDPLikeServer::handleTimer()
{
}


// ###### Startup ###########################################################
EventHandlingResult UDPLikeServer::initializeService(int sd)
{
   return(EHR_Okay);
}


// ###### Shutdown ##########################################################
void UDPLikeServer::finishService(int sd, EventHandlingResult result)
{
}


// ###### Print parameters ##################################################
void UDPLikeServer::printParameters()
{
}


// ###### Wait for action on RSerPool socket ################################
void UDPLikeServer::waitForAction(unsigned long long timeout)
{
   struct pollfd pollfds[1];
   pollfds[0].fd      = RSerPoolSocketDescriptor;
   pollfds[0].events  = POLLIN;
   pollfds[0].revents = 0;
   rsp_poll((struct pollfd*)&pollfds, 1, (int)(timeout / 1000));
}


// ###### Implementation of a simple UDP-like server ########################
void UDPLikeServer::poolElement(const char*          programTitle,
                                const char*          poolHandle,
                                struct rsp_info*     info,
                                struct rsp_loadinfo* loadinfo,
                                unsigned int         reregInterval,
                                unsigned int         runtimeLimit,
                                struct TagItem*      tags)
{
   if(rsp_initialize(info) < 0) {
      puts("ERROR: Unable to initialize rsplib Library!\n");
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
      Load = loadinfo->rli_load;

      // ====== Print program title =========================================
      puts(programTitle);
      for(size_t i = 0;i < strlen(programTitle);i++) {
         printf("=");
      }
      const char* policyName = rsp_getpolicybytype(loadinfo->rli_policy);
      puts("\n\nGeneral Parameters:");
      printf("   Pool Handle             = %s\n", poolHandle);
      printf("   Reregistration Interval = %u [ms]\n", reregInterval);
      printf("   Runtime Limit           = ");
      if(runtimeLimit > 0) {
         printf("%u [s]\n", runtimeLimit);
      }
      else {
         puts("off");
      }
      puts("   Policy Settings");
      printf("      Policy Type          = %s\n", (policyName != NULL) ? policyName : "?");
      printf("      Load Degradation     = %1.3f [%%]\n", 100.0 * ((double)loadinfo->rli_load_degradation / (double)PPV_MAX_LOAD_DEGRADATION));
      printf("      Load DPF             = %1.3f [%%]\n", 100.0 * ((double)loadinfo->rli_load_dpf / (double)PPV_MAX_LOADDPF));
      printf("      Weight               = %u\n", loadinfo->rli_weight);
      printf("      Weight DPF           = %1.3f [%%]\n", 100.0 * ((double)loadinfo->rli_weight_dpf / (double)PPV_MAX_WEIGHTDPF));
      printParameters();

      // ====== Register PE =================================================
      if(rsp_register_tags(RSerPoolSocketDescriptor,
                           (const unsigned char*)poolHandle, strlen(poolHandle),
                           loadinfo, reregInterval, 0, tags) == 0) {
         uint32_t identifier;
         if(rsp_getsockname(RSerPoolSocketDescriptor, NULL, NULL, &identifier) == 0) {
            puts("Registration");
            printf("   Identifier              = $%08x\n\n", identifier);
         }
         double oldLoad = (unsigned int)rint((double)loadinfo->rli_load / (double)PPV_MAX_LOAD);

         // ====== Startup ==================================================
         const EventHandlingResult initializeResult = initializeService(RSerPoolSocketDescriptor);
         if(initializeResult == EHR_Okay) {

            // ====== Main loop =============================================
            const unsigned long long autoStopTimeStamp =
               (runtimeLimit > 0) ? (getMicroTime() + (1000ULL * runtimeLimit)) : 0;
            installBreakDetector();
            while(!breakDetected()) {
               // ====== Read from socket ===================================
               char                     buffer[65536];
               int                      flags = 0;
               struct rsp_sndrcvinfo rinfo;
               unsigned long long timeout = 500000;
               if(NextTimerTimeStamp > 0) {
                  const unsigned long long now = getMicroTime();
                  if(NextTimerTimeStamp <= now) {
                     timeout = 0;
                  }
                  else {
                     timeout = min(timeout, NextTimerTimeStamp - now);
                  }
               }

               // ====== Wait for action on RSerPool socket =================
               waitForAction(timeout);

               // ====== Read from socket ===================================
               ssize_t received = rsp_recvfullmsg(RSerPoolSocketDescriptor,
                                                  (char*)&buffer, sizeof(buffer),
                                                  &rinfo, &flags, 0);

               // ====== Handle data ========================================
               if(received > 0) {
                  // ====== Handle event ====================================
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

                  // ====== Check for problems ==============================
                  if((eventHandlingResult == EHR_Abort) ||
                     (eventHandlingResult == EHR_Shutdown)) {
                     rsp_sendmsg(RSerPoolSocketDescriptor,
                                 NULL, 0, 0,
                                 rinfo.rinfo_session, 0, 0, 0,
                                 ((eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_EOF), 0);
                  }
               }

               // ====== Handle timer =======================================
               if((NextTimerTimeStamp > 0) &&
                  (getMicroTime() > NextTimerTimeStamp)) {
                  NextTimerTimeStamp = 0;
                  handleTimer();
               }

               // ====== Auto-stop ==========================================
               if((autoStopTimeStamp > 0) &&
                  (getMicroTime() > autoStopTimeStamp)) {
                  puts("Auto-stop reached!");
                  break;
               }

               // ====== Do reregistration on load changes =====================
               if(PPT_IS_ADAPTIVE(loadinfo->rli_policy)) {
                  const double newLoad = getLoad();
                  if(fabs(newLoad - oldLoad) >= 0.01) {
                     oldLoad = newLoad;
                     loadinfo->rli_load = (unsigned int)rint(newLoad * (double)PPV_MAX_LOAD);
                     if(rsp_register_tags(
                           RSerPoolSocketDescriptor,
                           (const unsigned char*)poolHandle, strlen(poolHandle),
                           loadinfo, reregInterval, REGF_DONTWAIT, tags) != 0) {
                        puts("ERROR: Failed to re-register PE with new load setting!");
                     }
                  }
               }
            }
         }

         // ====== Shutdown =================================================
         finishService(RSerPoolSocketDescriptor, initializeResult);

         // ====== Clean up =================================================
         rsp_deregister(RSerPoolSocketDescriptor, 0);
      }
      else {
         printf("ERROR: Failed to register PE to pool %s\n", poolHandle);
      }
      rsp_close(RSerPoolSocketDescriptor);
   }
   else {
      logerror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   puts("\nTerminated!");
}
