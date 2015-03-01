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
 * Copyright (C) 2002-2015 by Thomas Dreibholz
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

#include "udplikeserver.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "loglevel.h"
#include "breakdetector.h"
#include "tagitem.h"

#include <math.h>
#include <string.h>
#include <ext_socket.h>


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
      fputs("ERROR: Invalid load setting!\n", stdlog);
      fflush(stdlog);
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
   printTimeStamp(stdlog);
   fprintf(stdlog, "COOKIE ECHO for session %u\n", sessionID);
   fflush(stdlog);
   return(EHR_Okay);
}


// ###### Handle notification ###############################################
void UDPLikeServer::handleNotification(const union rserpool_notification* notification)
{
   printTimeStamp(stdlog);
   fprintf(stdlog, "NOTIFICATION: ");
   rsp_print_notification(notification, stdlog);
   fputs("\n", stdlog);
   fflush(stdlog);
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
EventHandlingResult UDPLikeServer::initializeService(int sd, const uint32_t peIdentifier)
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
bool UDPLikeServer::waitForAction(unsigned long long timeout)
{
   struct pollfd pollfds[1];
   pollfds[0].fd      = RSerPoolSocketDescriptor;
   pollfds[0].events  = POLLIN;
   pollfds[0].revents = 0;
   if(rsp_poll((struct pollfd*)&pollfds, 1, (int)(timeout / 1000)) > 0) {
      return(pollfds[0].revents & POLLIN);
   }
   return(false);
}


// ###### Implementation of a simple UDP-like server ########################
void UDPLikeServer::poolElement(const char*          programTitle,
                                const char*          poolHandle,
                                struct rsp_info*     info,
                                struct rsp_loadinfo* loadinfo,
                                const sockaddr*      localAddressSet,
                                const size_t         localAddresses,
                                unsigned int         reregInterval,
                                unsigned int         runtimeLimit,
                                const bool           quiet,
                                const bool           daemonMode, 
                                struct TagItem*      tags)
{
   if(rsp_initialize(info) < 0) {
      puts("ERROR: Unable to initialize rsplib Library!\n");
      return;
   }

   RSerPoolSocketDescriptor = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(RSerPoolSocketDescriptor >= 0) {
      if(rsp_bind(RSerPoolSocketDescriptor, localAddressSet, localAddresses) == 0) {
         if(rsp_listen(RSerPoolSocketDescriptor, 10) == 0) {
            // ====== Initialize PE settings ======================================
            struct rsp_loadinfo dummyLoadinfo;
            if(loadinfo == NULL) {
               memset(&dummyLoadinfo, 0, sizeof(dummyLoadinfo));
               loadinfo = &dummyLoadinfo;
               loadinfo->rli_policy = PPT_ROUNDROBIN;
            }
            loadinfo->rli_load = (unsigned int)rint(getLoad() * (double)PPV_MAX_LOAD);
            Load = loadinfo->rli_load;

            // ====== Print program title =========================================
            if(!quiet) {
               puts(programTitle);
               const size_t programTitleSize = strlen(programTitle);
               for(size_t i = 0;i < programTitleSize;i++) {
                  printf("=");
               }
               const char* policyName = rsp_getpolicybytype(loadinfo->rli_policy);
               puts("\n\nGeneral Parameters:");
               printf("   Pool Handle             = %s\n", poolHandle);
               printf("   Reregistration Interval = %1.3fs\n", reregInterval / 1000.0);
               printf("   Local Addresses         = { ");
               if(localAddresses == 0) {
                  printf("all");
               }
               else {
                  const sockaddr* addressPtr = localAddressSet;
                  for(size_t i = 0;i < localAddresses;i++) {
                     if(i > 0) {
                        printf(", ");
                     }
                     fputaddress(addressPtr, (i == 0), stdout);
                     addressPtr = (const sockaddr*)((long)addressPtr + (long)getSocklen(addressPtr));
                  }
               }
               printf(" }\n");
               printf("   Runtime Limit           = ");
               if(runtimeLimit > 0) {
                  printf("%1.3f [s]\n", runtimeLimit / 1000.0);
               }
               else {
                  puts("off");
               }
               puts("   Policy Settings");
               printf("      Policy Type          = %s\n", (policyName != NULL) ? policyName : "?");
               printf("      Load Degradation     = %1.3f%%\n", 100.0 * ((double)loadinfo->rli_load_degradation / (double)PPV_MAX_LOAD_DEGRADATION));
               printf("      Load DPF             = %1.3f%%\n", 100.0 * ((double)loadinfo->rli_load_dpf / (double)PPV_MAX_LOADDPF));
               printf("      Weight               = %u\n", loadinfo->rli_weight);
               printf("      Weight DPF           = %1.3f%%\n", 100.0 * ((double)loadinfo->rli_weight_dpf / (double)PPV_MAX_WEIGHTDPF));
               printParameters();
            }

            // ====== Register PE =================================================
            if(rsp_register_tags(RSerPoolSocketDescriptor,
                                 (const unsigned char*)poolHandle, strlen(poolHandle),
                                 loadinfo, reregInterval,
                                 daemonMode ? REGF_DAEMONMODE : 0,
                                 tags) == 0) {
               uint32_t identifier = 0;
               if(rsp_getsockname(RSerPoolSocketDescriptor, NULL, NULL, &identifier) == 0) {
                  if(!quiet) {
                     puts("Registration:");
                     printf("   Identifier              = $%08x\n\n", identifier);
                  }
               }
               double oldLoad = (unsigned int)rint((double)loadinfo->rli_load / (double)PPV_MAX_LOAD);

               // ====== Startup ==================================================
               const EventHandlingResult initializeResult = initializeService(RSerPoolSocketDescriptor, identifier);
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
                     if(waitForAction(timeout)) {

                        // ====== Read from socket ================================
                        ssize_t received = rsp_recvfullmsg(RSerPoolSocketDescriptor,
                                                           (char*)&buffer, sizeof(buffer),
                                                           &rinfo, &flags, 0);

                        // ====== Handle data =====================================
                        if(received > 0) {
                           // ====== Handle event =================================
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

                           // ====== Check for problems ===========================
                           if((eventHandlingResult == EHR_Abort) ||
                              (eventHandlingResult == EHR_Shutdown)) {
#ifdef SOLARIS
                              rsp_sendmsg(RSerPoolSocketDescriptor,
                                          NULL, 0,
                                          ((eventHandlingResult == EHR_Abort) ? MSG_ABORT : MSG_EOF),
                                          rinfo.rinfo_session, 0, 0, 0,
                                          0, 0);
#else
                              rsp_sendmsg(RSerPoolSocketDescriptor,
                                          NULL, 0, 0,
                                          rinfo.rinfo_session, 0, 0, 0,
                                          ((eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_EOF), 0);
#endif
                           }
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

                     // ====== Do reregistration on load changes ==================
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
                  uninstallBreakDetector();
               }

               // ====== Shutdown =================================================
               finishService(RSerPoolSocketDescriptor, initializeResult);

               // ====== Clean up =================================================
               rsp_deregister(RSerPoolSocketDescriptor, 0);
            }
            else {
               fprintf(stdlog, "ERROR: Failed to register PE to pool %s\n", poolHandle);
            }
         }
         else {
            logerror("Unable to put RSerPool socket into listening mode");
         }
      }
      else {
         logerror("Unable to bind RSerPool socket");
      }
      rsp_close(RSerPoolSocketDescriptor);
   }
   else {
      logerror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   fputs("\nTerminated!\n", stdlog);
}
