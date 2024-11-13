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
   printTimeStamp(stdlog);
   fprintf(stdlog, "New thread for RSerPool socket %d.\n", RSerPoolSocketDescriptor);
   fflush(stdlog);
}


// ###### Destructor ########################################################
TCPLikeServer::~TCPLikeServer()
{
   CHECK(ServerList == NULL);
   printTimeStamp(stdlog);
   fprintf(stdlog, "Thread for RSerPool socket %d has been stopped.\n", RSerPoolSocketDescriptor);
   fflush(stdlog);
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
   lock();
   Shutdown = true;
   unlock();
}


// ##### Get load ###########################################################
double TCPLikeServer::getLoad() const
{
   return((double)Load / (double)PPV_MAX_LOAD);
}


// ##### Set load ###########################################################
void TCPLikeServer::setLoad(double load)
{
   ServerList->lock();

   CHECK(ServerList != NULL);
   CHECK(ServerList->LoadSum >= Load);
   if((load < 0.0) || (load > 1.0)) {
      fputs("ERROR: Invalid load setting!\n", stdlog);
      fflush(stdlog);
      ServerList->unlock();
      return;
   }
   const unsigned int newLoad = (unsigned int)floor(load * (double)PPV_MAX_LOAD);
   if((long long)ServerList->LoadSum - (long long)Load + (long long)newLoad > PPV_MAX_LOAD) {
      fputs("ERROR: Something is wrong with load settings. Total load would exceed 100%!\n", stdlog);
      fflush(stdlog);
      ServerList->unlock();
      return;
   }

   const double oldTotalLoad = ServerList->getTotalLoad();

   ServerList->LoadSum -= Load;
   lock();
   Load = newLoad;
   unlock();
   ServerList->LoadSum += Load;

   const double newTotalLoad = ServerList->getTotalLoad();

   ServerList->unlock();

   if(oldTotalLoad != newTotalLoad) {
      const ssize_t result = ext_write(ServerList->SystemNotificationPipe, "!", 1);
      if(result <= 0) {
         perror("Writing to system notification pipe failed");
      }
   }
}


// ###### Handle cookie #####################################################
EventHandlingResult TCPLikeServer::handleCookieEcho(const char* buffer,
                                                    size_t      bufferSize)
{
   printTimeStamp(stdlog);
   fputs("COOKIE ECHO\n", stdlog);
   fflush(stdout);
   return(EHR_Okay);
}


// ###### Handle notification ###############################################
EventHandlingResult TCPLikeServer::handleNotification(
                       const union rserpool_notification* notification)
{
   printTimeStamp(stdlog);
   fprintf(stdlog, "NOTIFICATION: ");
   rsp_print_notification(notification, stdlog);
   fputs("\n", stdlog);
   fflush(stdout);
   return(EHR_Okay);
}


// ###### Handle synchronous timer ##########################################
EventHandlingResult TCPLikeServer::syncTimerEvent(const unsigned long long now)
{
   printTimeStamp(stdlog);
   fputs("SyncTimer\n", stdlog);
   fflush(stdout);
   return(EHR_Okay);
}


// ###### Handle asynchronous timer #########################################
void TCPLikeServer::asyncTimerEvent(const unsigned long long now)
{
   printTimeStamp(stdlog);
   fputs("AsyncTimer\n", stdlog);
   fflush(stdout);
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
      while(!isShuttingDown()) {
         flags    = 0;
         unsigned long long nextTimerEvent;
         if(SyncTimerTimeStamp > 0) {
            now = getMicroTime();
            nextTimerEvent = (SyncTimerTimeStamp <= now) ? 0 : SyncTimerTimeStamp - now;
         }
         else {
            nextTimerEvent = 5000000;
         }
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
                  printTimeStamp(stdlog);
                  fputs("Dropped unexpected ASAP COOKIE_ECHO!\n", stdlog);
                  fflush(stdlog);
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
#ifdef SOLARIS
      rsp_sendmsg(RSerPoolSocketDescriptor,
                  NULL, 0,
                  (eventHandlingResult == EHR_Abort) ? MSG_ABORT : MSG_EOF,
                  0, 0, 0, 0, 0,
                  0);
#else
      rsp_sendmsg(RSerPoolSocketDescriptor,
                  NULL, 0, 0,
                  0, 0, 0, 0,
                  (eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_EOF,
                  0);
#endif
   }
   lock();
   Finished = true;
   unlock();
}


// ###### Implementation of a simple threaded server ########################
void TCPLikeServer::poolElement(const char*          programTitle,
                                const char*          poolHandle,
                                struct rsp_info*     info,
                                struct rsp_loadinfo* loadinfo,
                                size_t               maxThreads,
                                TCPLikeServer*       (*threadFactory)(int sd, void* userData, const uint32_t peIdentifier),
                                void                 (*printParameters)(const void* userData),
                                void                 (*rejectNewSession)(int sd),
                                bool                 (*initializeService)(void* userData),
                                void                 (*finishService)(void* userData),
                                double               (*loadUpdateHook)(const double load),
                                void*                userData,
                                const sockaddr*      localAddressSet,
                                const size_t         localAddresses,
                                unsigned int         reregInterval,
                                unsigned int         runtimeLimit,
                                const bool           quiet,
                                const bool           daemonMode,
                                struct TagItem*      tags)
{
   if(rsp_initialize(info) < 0) {
      fputs("ERROR: Unable to initialize rsplib Library!\n", stdlog);
      return;
   }

   int rserpoolSocket = rsp_socket(0, SOCK_STREAM, IPPROTO_SCTP);
   if(rserpoolSocket >= 0) {
      if(rsp_bind(rserpoolSocket, localAddressSet, localAddresses) == 0) {
         if(rsp_listen(rserpoolSocket, (int)maxThreads) == 0) {
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
                        printf("   Max Threads             = %u\n", (unsigned int)maxThreads);
                        puts("   Policy Settings");
                        printf("      Policy Type          = %s\n", (policyName != NULL) ? policyName : "?");
                        printf("      Load Degradation     = %1.3f%%\n", 100.0 * ((double)loadinfo->rli_load_degradation / (double)PPV_MAX_LOAD_DEGRADATION));
                        printf("      Load DPF             = %1.3f%%\n", 100.0 * ((double)loadinfo->rli_load_dpf / (double)PPV_MAX_LOADDPF));
                        printf("      Weight               = %u\n", loadinfo->rli_weight);
                        printf("      Weight DPF           = %1.3f%%\n", 100.0 * ((double)loadinfo->rli_weight_dpf / (double)PPV_MAX_WEIGHTDPF));
                        if(printParameters) {
                           printParameters(userData);
                        }
                     }

                     // ====== Register PE ========================================
                     if(rsp_register_tags(rserpoolSocket,
                                          (const unsigned char*)poolHandle, strlen(poolHandle),
                                          loadinfo, reregInterval,
                                          (daemonMode == true) ? REGF_DAEMONMODE : 0,
                                          tags) == 0) {
                        uint32_t identifier = 0;
                        if(rsp_getsockname(rserpoolSocket, NULL, NULL, &identifier) == 0) {
                           if(!quiet) {
                              puts("Registration:");
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
                           if(serverSet.handleRemovalsAndTimers() > 0) {
                              const int backlog = (int)(serverSet.getMaxThreads() - serverSet.getThreads());
                              if(rsp_listen(rserpoolSocket, 1 + backlog) < 0) {
                                 logerror("Unable to update backlog using rsp_listen()");
                              }
                           }

                           // ====== Wait for incoming sessions ===================
                           if(result > 0) {
                              if(pollfds[0].revents & POLLIN) {
                                  int newRSerPoolSocket = rsp_accept(rserpoolSocket, 0);
                                  if(newRSerPoolSocket >= 0) {
                                     if(serverSet.hasCapacity()) {
                                        TCPLikeServer* serviceThread = threadFactory(newRSerPoolSocket, userData, identifier);
                                        if(serviceThread) {
                                           if(serverSet.add(serviceThread)) {
                                              if(serviceThread->start()) {
                                                 const int backlog = (int)(serverSet.getMaxThreads() - serverSet.getThreads());
                                                 if(rsp_listen(rserpoolSocket, 1 + backlog) < 0) {
                                                    logerror("Unable to update backlog using rsp_listen()");
                                                    fflush(stdlog);
                                                 }
                                              }
                                              else {
                                                 printTimeStamp(stdlog);
                                                 fputs("ERROR: Unable to start up service thread\n", stderr);
                                                 fflush(stdlog);
                                                 serverSet.remove(serviceThread);
                                                 delete serviceThread;
                                              }
                                           }
                                           else {
                                              delete serviceThread;
                                              printTimeStamp(stdlog);
                                              fputs("Unable to add new service thread\n", stdlog);
                                              fflush(stdlog);
                                           }
                                        }
                                        else {
                                           rsp_close(newRSerPoolSocket);
                                           printTimeStamp(stdlog);
                                           fputs("Unable to create new service thread\n", stdlog);
                                           fflush(stdlog);
                                        }
                                     }
                                     else {
                                        if(rejectNewSession) {
                                           rejectNewSession(newRSerPoolSocket);
                                        }
                                        rsp_close(newRSerPoolSocket);
                                        if(!quiet) {
                                           printTimeStamp(stdlog);
                                           fputs("Rejected new session, since server is fully loaded\n", stdlog);
                                           fflush(stdlog);
                                        }
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
                                 const ssize_t result = ext_read(systemNotificationPipe[0], (char*)&buffer, sizeof(buffer));
                                 if(result < 0) {
                                    perror("Reading from system notification pipe failed");
                                 }
                              }

                           }

                           // ====== Auto-stop ====================================
                           if((autoStopTimeStamp > 0) &&
                              (getMicroTime() > autoStopTimeStamp)) {
                              fputs("Auto-stop reached!\n", stdlog);
                              break;
                           }
                        }

                        // ====== Clean up ========================================
                        serverSet.removeAll();
                        rsp_deregister(rserpoolSocket, 0);
                        uninstallBreakDetector();
                     }
                     if(finishService != NULL) {
                        finishService(userData);
                     }
                  }
                  else {
                     fprintf(stdlog, "ERROR: Failed to register PE to pool %s\n", poolHandle);
                  }
                  if(rspNotificationPipe[0] > 0) {
                     rsp_unmapsocket(rspNotificationPipe[0]);
                  }
                  if(rspNotificationPipe[1] > 0) {
                     rsp_unmapsocket(rspNotificationPipe[1]);
                  }
               }
               else {
                  logerror("Unable to map notification pipe FDs");
               }
               ext_close(systemNotificationPipe[0]);
               ext_close(systemNotificationPipe[1]);
            }
            else {
               logerror("Unable to create notification pipe");
            }
         }
         else {
            logerror("Unable to put RSerPool socket into listening mode");
         }
      }
      else {
         logerror("Unable to bind RSerPool socket");
      }
      rsp_close(rserpoolSocket);
   }
   else {
      logerror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   fputs("\nTerminated!\n", stdlog);
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
size_t TCPLikeServerList::handleRemovalsAndTimers()
{
   size_t removed = 0;

   lock();
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      ThreadListEntry* next = entry->Next;
      if(entry->Object->hasFinished()) {
         remove(entry->Object);
         removed++;
      }
      else {
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

   return(removed);
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
   lock();
   const size_t             threads = Threads;
   const unsigned long long loadSum = LoadSum;
   unlock();

   if(threads > 0) {
      return(loadSum / (double)PPV_MAX_LOAD);
   }
   return(0.0);
}
