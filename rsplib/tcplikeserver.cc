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

#include "tcplikeserver.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "loglevel.h"
#include "breakdetector.h"
#include "tagitem.h"

#include <math.h>
#include <string.h>


// ###### Constructor #######################################################
TCPLikeServer::TCPLikeServer(int rserpoolSocketDescriptor)
{
   RSerPoolSocketDescriptor = rserpoolSocketDescriptor;
   ServerList               = NULL;
   IsNewSession             = true;
   Shutdown                 = false;
   Finished                 = false;
   Load                     = 0;
   SyncTimerTimeStamp       = 0;
   AsyncTimerTimeStamp      = 0;
   printTimeStamp(stdout);
   printf("New thread for RSerPool socket %d.\n", RSerPoolSocketDescriptor);
}


// ###### Destructor ########################################################
TCPLikeServer::~TCPLikeServer()
{
   CHECK(ServerList == NULL);
   printTimeStamp(stdout);
   printf("Thread for RSerPool socket %d has been stopped.\n", RSerPoolSocketDescriptor);
   if(RSerPoolSocketDescriptor >= 0) {
      rsp_close(RSerPoolSocketDescriptor);
      RSerPoolSocketDescriptor = -1;
   }
}


// ###### Startup of thread #################################################
EventHandlingResult TCPLikeServer::initializeSession()
{
   return(EHR_Okay);
}


// ###### Finish of thread ##################################################
void TCPLikeServer::finishSession(EventHandlingResult result)
{
}


// ###### Shutdown thread ###################################################
void TCPLikeServer::shutdown()
{
   Shutdown = true;
}


// ##### Get load ###########################################################
double TCPLikeServer::getLoad() const
{
   return((double)Load / (double)PPV_MAX_LOAD);
}


// ##### Set load ###########################################################
void TCPLikeServer::setLoad(double load)
{
   CHECK(ServerList != NULL);
   CHECK(ServerList->LoadSum >= Load);
   if((load < 0.0) || (load > 1.0)) {
      fputs("ERROR: Invalid load setting!\n", stderr);
      return;
   }
   const unsigned int newLoad = (unsigned int)floor(load * (double)PPV_MAX_LOAD);
   if((long long)ServerList->LoadSum - (long long)Load + (long long)newLoad > PPV_MAX_LOAD) {
      fputs("ERROR: Something is wrong with load settings. Total load would exceed 100%!\n", stderr);
      return;
   }

   ServerList->lock();
   const double oldTotalLoad = ServerList->getTotalLoad();

   ServerList->LoadSum -= Load;
   Load = newLoad;
   ServerList->LoadSum += Load;

   const double newTotalLoad = ServerList->getTotalLoad();
   ServerList->unlock();

   if(oldTotalLoad != newTotalLoad) {
      ext_write(ServerList->SystemNotificationPipe, "!", 1);
   }
}


// ###### Handle cookie #####################################################
EventHandlingResult TCPLikeServer::handleCookieEcho(const char* buffer,
                                                    size_t      bufferSize)
{
   printTimeStamp(stdout);
   puts("COOKIE ECHO");
   return(EHR_Okay);
}


// ###### Handle notification ###############################################
EventHandlingResult TCPLikeServer::handleNotification(
                       const union rserpool_notification* notification)
{
   printTimeStamp(stdout);
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
   return(EHR_Okay);
}


// ###### Handle synchronous timer ##########################################
EventHandlingResult TCPLikeServer::syncTimerEvent(const unsigned long long now)
{
   printTimeStamp(stdout);
   puts("SyncTimer");
   return(EHR_Okay);
}


// ###### Handle asynchronous timer #########################################
void TCPLikeServer::asyncTimerEvent(const unsigned long long now)
{
   printTimeStamp(stdout);
   puts("AsyncTimer");
}


// ###### Handle message ####################################################
EventHandlingResult TCPLikeServer::handleMessage(const char* buffer,
                                                 size_t      bufferSize,
                                                 uint32_t    ppid,
                                                 uint16_t    streamID)
{
   return(EHR_Abort);
}


// ###### TCPLike server main loop #########################################
void TCPLikeServer::run()
{
   char                  buffer[65536];
   struct rsp_sndrcvinfo rinfo;
   ssize_t               received;
   int                   flags;
   EventHandlingResult   eventHandlingResult;
   unsigned long long    now;

   eventHandlingResult = initializeSession();
   if(eventHandlingResult == EHR_Okay) {
      while(!Shutdown) {
         flags    = 0;
         unsigned long long nextTimerEvent;
         if(SyncTimerTimeStamp > 0) {
            now = getMicroTime();
            nextTimerEvent = (SyncTimerTimeStamp <= now) ? 0 : SyncTimerTimeStamp - now;
         }
         else {
            nextTimerEvent = 5000000;
         }
         now      = getMicroTime();
         received = rsp_recvfullmsg(RSerPoolSocketDescriptor,
                                    (char*)&buffer, sizeof(buffer),
                                    &rinfo, &flags, (int)(nextTimerEvent / 1000));
         if(received > 0) {
            /*
            for(int i = 0;i < received;i++) {
               printf("%02x ", ((unsigned char*)&buffer)[i]);
            }
            puts("");
            fflush(stdout);
            */

            // ====== Handle event ==========================================
            if(flags & MSG_RSERPOOL_COOKIE_ECHO) {
               if(IsNewSession) {
                  IsNewSession = false;
                  eventHandlingResult = handleCookieEcho((char*)&buffer, received);
               }
               else {
                  printTimeStamp(stdout);
                  puts("Dropped unexpected ASAP COOKIE_ECHO!");
                  eventHandlingResult = EHR_Abort;
               }
            }
            else if(flags & MSG_RSERPOOL_NOTIFICATION) {
               eventHandlingResult = handleNotification((const union rserpool_notification*)&buffer);
            }
            else {
               IsNewSession = false;
               eventHandlingResult = handleMessage((char*)&buffer, received,
                                                   rinfo.rinfo_ppid, rinfo.rinfo_stream);
            }

            // ====== Check for problems ====================================
            if(eventHandlingResult != EHR_Okay) {
               break;
            }
         }
         else if(received == 0) {
            break;
         }

         // ====== Handle timer event =======================================
         if((SyncTimerTimeStamp > 0) && (eventHandlingResult == EHR_Okay)) {
            now = getMicroTime();
            if(SyncTimerTimeStamp <= now) {
               eventHandlingResult = syncTimerEvent(now);
               if(eventHandlingResult != EHR_Okay) {
                  break;
               }
            }
         }
      }
   }

   // Since it can take some time to clean up the session (e.g. removing
   // temporary files for Scripting Service), we finish the session *before*
   // actually closing the association. Then, the PU waits until being finished
   // before actually trying to distribute the next session.
   finishSession(eventHandlingResult);
   if((eventHandlingResult == EHR_Abort) ||
      (eventHandlingResult == EHR_Shutdown)) {
printf("Abort: sd=%d  ehr=%d\n", RSerPoolSocketDescriptor, eventHandlingResult);
      rsp_sendmsg(RSerPoolSocketDescriptor,
                  NULL, 0, 0,
                  0, 0, 0, 0,
                  (eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_EOF,
                  0);
   }
   Finished = true;
}


// ###### Implementation of a simple threaded server ########################
void TCPLikeServer::poolElement(const char*          programTitle,
                                const char*          poolHandle,
                                struct rsp_info*     info,
                                struct rsp_loadinfo* loadinfo,
                                size_t               maxThreads,
                                TCPLikeServer*       (*threadFactory)(int sd, void* userData),
                                void                 (*printParameters)(const void* userData),
                                bool                 (*initializeService)(void* userData),
                                void                 (*finishService)(void* userData),
                                double               (*loadUpdateHook)(const double load),
                                void*                userData,
                                unsigned int         reregInterval,
                                unsigned int         runtimeLimit,
                                const bool           quiet,
                                struct TagItem*      tags)
{
   if(rsp_initialize(info) < 0) {
      fputs("ERROR: Unable to initialize rsplib Library!\n", stderr);
      return;
   }

   int rserpoolSocket = rsp_socket(0, SOCK_STREAM, IPPROTO_SCTP);
   if(rserpoolSocket >= 0) {
      // ====== Initialize notification pipe ================================
      int systemNotificationPipe[2];
      int rspNotificationPipe[2];
      if(ext_pipe((int*)&systemNotificationPipe) == 0) {
         rspNotificationPipe[0] = rsp_mapsocket(systemNotificationPipe[0], -1);
         rspNotificationPipe[1] = rsp_mapsocket(systemNotificationPipe[1], -1);
         if( (setNonBlocking(systemNotificationPipe[0])) && (rspNotificationPipe[0] > 0) &&
             (setNonBlocking(systemNotificationPipe[1])) && (rspNotificationPipe[1] > 0) ) {

            // ====== Initialize PE settings ================================
            struct rsp_loadinfo dummyLoadinfo;
            if(loadinfo == NULL) {
               memset(&dummyLoadinfo, 0, sizeof(dummyLoadinfo));
               loadinfo = &dummyLoadinfo;
               loadinfo->rli_policy = PPT_ROUNDROBIN;
            }

            // ====== Server startup ========================================
            if((initializeService == NULL) || (initializeService(userData))) {
               double oldLoad = 0.0;
               if(PPT_IS_ADAPTIVE(loadinfo->rli_policy)) {
                  if(loadUpdateHook) {
                     double newLoad = loadUpdateHook(0.0);
                     CHECK((newLoad >= 0.0) && (newLoad <= 1.0));
                     loadinfo->rli_load = (unsigned int)rint(newLoad * (double)PPV_MAX_LOAD);
                     oldLoad = newLoad;
                  }
               }

               // ====== Print program title ================================
               if(!quiet) {
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
                  printf("   Max Threads             = %u\n", (unsigned int)maxThreads);
                  puts("   Policy Settings");
                  printf("      Policy Type          = %s\n", (policyName != NULL) ? policyName : "?");
                  printf("      Load Degradation     = %1.3f [%%]\n", 100.0 * ((double)loadinfo->rli_load_degradation / (double)PPV_MAX_LOAD_DEGRADATION));
                  printf("      Load DPF             = %1.3f [%%]\n", 100.0 * ((double)loadinfo->rli_load_dpf / (double)PPV_MAX_LOADDPF));
                  printf("      Weight               = %u\n", loadinfo->rli_weight);
                  printf("      Weight DPF           = %1.3f [%%]\n", 100.0 * ((double)loadinfo->rli_weight_dpf / (double)PPV_MAX_WEIGHTDPF));
                  if(printParameters) {
                     printParameters(userData);
                  }
               }

               // ====== Register PE ========================================
               if(rsp_register_tags(rserpoolSocket,
                                    (const unsigned char*)poolHandle, strlen(poolHandle),
                                    loadinfo, reregInterval, 0, tags) == 0) {
                  if(!quiet) {
                     uint32_t identifier;
                     if(rsp_getsockname(rserpoolSocket, NULL, NULL, &identifier) == 0) {
                        puts("Registration");
                        printf("   Identifier              = $%08x\n\n", identifier);
                     }
                  }

                  // ====== Main loop =======================================
                  TCPLikeServerList        serverSet(maxThreads, systemNotificationPipe[1]);
                  const unsigned long long autoStopTimeStamp =
                     (runtimeLimit > 0) ? (getMicroTime() + (1000ULL * runtimeLimit)) : 0;
                  installBreakDetector();
                  while(!breakDetected()) {
                     // ====== Wait for actions =============================
                     struct pollfd pollfds[2];
                     pollfds[0].fd      = rserpoolSocket;
                     pollfds[0].events  = POLLIN;
                     pollfds[0].revents = 0;
                     pollfds[1].fd      = rspNotificationPipe[0];
                     pollfds[1].events  = POLLIN;
                     pollfds[1].revents = 0;
                     const int result = rsp_poll((struct pollfd*)&pollfds, 2, 2500);

                     // ====== Clean-up session list ========================
                     // Possible, some sessions have finished working during
                     // waiting time. Therefore, cleaning them up is useful
                     // before accepting new sessions.
                     serverSet.handleRemovalsAndTimers();

                     // ====== Wait for incoming sessions ===================
                     if(result > 0) {
                        if(pollfds[0].revents & POLLIN) {
                            int newRSerPoolSocket = rsp_accept(rserpoolSocket, 0);
                            if(newRSerPoolSocket >= 0) {
                               TCPLikeServer* serviceThread = threadFactory(newRSerPoolSocket, userData);
                               if(serviceThread) {
                                  if(serverSet.add(serviceThread)) {
                                     serviceThread->start();
                                  }
                                  else {
                                     puts("Rejected new session");
                                     delete serviceThread;
                                  }
                               }
                               else {
                                  puts("Unable to create new service thread");
                                  rsp_close(newRSerPoolSocket);
                               }
                            }
                        }

                        // ====== Do reregistration on load changes =========
                        if(PPT_IS_ADAPTIVE(loadinfo->rli_policy)) {
                           double newLoad = serverSet.getTotalLoad();
                           if(loadUpdateHook) {
                              newLoad = loadUpdateHook(newLoad);
                              CHECK((newLoad >= 0.0) && (newLoad <= 1.0));
                           }
                           if(fabs(newLoad - oldLoad) >= 0.01) {
                              // printf("NEW LOAD = %1.6f\n", newLoad);
                              oldLoad = newLoad;
                              loadinfo->rli_load = (unsigned int)rint(newLoad * (double)PPV_MAX_LOAD);
                              rsp_register_tags(rserpoolSocket,
                                                (const unsigned char*)poolHandle, strlen(poolHandle),
                                                loadinfo, reregInterval, REGF_DONTWAIT,
                                                tags);
                           }
                        }

                        // ====== Clear notification pipe ===================
                        if(pollfds[1].revents & POLLIN) {
                           char buffer[1024];
                           ext_read(systemNotificationPipe[0], (char*)&buffer, sizeof(buffer));
                        }

                     }

                     // ====== Auto-stop ====================================
                     if((autoStopTimeStamp > 0) &&
                        (getMicroTime() > autoStopTimeStamp)) {
                        puts("Auto-stop reached!");
                        break;
                     }
                  }

                  // ====== Clean up ========================================
                  serverSet.removeAll();
                  rsp_deregister(rserpoolSocket, 0);
               }
               if(finishService != NULL) {
                  finishService(userData);
               }
            }
            else {
               fprintf(stderr, "ERROR: Failed to register PE to pool %s\n", poolHandle);
            }
            if(rspNotificationPipe[0] > 0) {
               rsp_unmapsocket(rspNotificationPipe[0]);
            }
            if(rspNotificationPipe[1] > 0) {
               rsp_unmapsocket(rspNotificationPipe[1]);
            }
         }
         else {
            perror("Unable to map notification pipe FDs");
         }
         ext_close(systemNotificationPipe[0]);
         ext_close(systemNotificationPipe[1]);
      }
      else {
         perror("Unable to create notification pipe");
      }
      rsp_close(rserpoolSocket);
   }
   else {
      perror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   puts("\nTerminated!");
}




// ###### Constructor #######################################################
TCPLikeServerList::TCPLikeServerList(size_t maxThreads, int systemNotificationPipe)
{
   ThreadList             = NULL;
   LoadSum                = 0;
   Threads                = 0;
   MaxThreads             = maxThreads;
   SystemNotificationPipe = systemNotificationPipe;
}


// ###### Destructor ########################################################
TCPLikeServerList::~TCPLikeServerList()
{
   removeAll();
}


// ###### Get number of threads #############################################
size_t TCPLikeServerList::getThreads()
{
   size_t threads;
   lock();
   threads = Threads;
   unlock();
   return(threads);
}


// ###### Remove finished sessions ##########################################
void TCPLikeServerList::handleRemovalsAndTimers()
{
   lock();
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      ThreadListEntry* next = entry->Next;
      if(entry->Object->hasFinished()) {
printf("FIN!!!: %p\n", entry->Object);
         remove(entry->Object);
      }
      else {
printf("!fin: %p\n", entry->Object);
         entry->Object->lock();
         if(entry->Object->AsyncTimerTimeStamp > 0) {
            const unsigned long long now = getMicroTime();
            if(entry->Object->AsyncTimerTimeStamp <= now) {
               entry->Object->asyncTimerEvent(now);
            }
         }
         entry->Object->unlock();
      }
      entry = next;
   }
   unlock();
}


// ###### Remove all sessions ###############################################
void TCPLikeServerList::removeAll()
{
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      remove(entry->Object);
      entry = ThreadList;
   }
}


// ###### Add session #######################################################
bool TCPLikeServerList::add(TCPLikeServer* thread)
{
   if(Threads < MaxThreads) {
      ThreadListEntry* entry = new ThreadListEntry;
      if(entry) {
         lock();
         entry->Next   = ThreadList;
         entry->Object = thread;
         ThreadList    = entry;

         thread->setServerList(this);
         Threads++;
         unlock();
         return(true);
      }
   }
   return(false);
}


// ###### Remove session ####################################################
void TCPLikeServerList::remove(TCPLikeServer* thread)
{
   // ====== Tell thread to shut down =======================================
   thread->shutdown();
   thread->waitForFinish();
   thread->setLoad(0.0);

   // ====== Remove thread ==================================================
   lock();
   ThreadListEntry* entry = ThreadList;
   ThreadListEntry* prev  = NULL;
   while(entry != NULL) {
      if(entry->Object == thread) {
         if(prev == NULL) {
            ThreadList = entry->Next;
         }
         else {
            prev->Next = entry->Next;
         }

         thread->setServerList(NULL);
         Threads--;

         delete entry->Object;
         entry->Object = NULL;
         delete entry;
         break;
      }
      prev  = entry;
      entry = entry->Next;
   }
   unlock();
}


// ###### Get total load ####################################################
double TCPLikeServerList::getTotalLoad()
{
   size_t             threads;
   unsigned long long loadSum;

   lock();
   threads = Threads;
   loadSum = LoadSum;
   unlock();

   if(threads > 0) {
      return(LoadSum / (double)PPV_MAX_LOAD);
   }
   return(0.0);
}
