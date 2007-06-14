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

#include "tcplikeserver.h"
#include "timeutilities.h"
#include "loglevel.h"
#include "breakdetector.h"
#include "tagitem.h"


// ###### Constructor #######################################################
TCPLikeServer::TCPLikeServer(int rserpoolSocketDescriptor)
{
   RSerPoolSocketDescriptor = rserpoolSocketDescriptor;
   ServerList   = NULL;
   IsNewSession = true;
   Shutdown     = false;
   Finished     = false;
   Load         = 0;
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
   if(!Finished) {
      Shutdown = true;
   }
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
   ServerList->LoadSum -= Load;
   Load = newLoad;
   ServerList->LoadSum += Load;
   ServerList->unlock();
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

   eventHandlingResult = initializeSession();
   if(eventHandlingResult == EHR_Okay) {
      while(!Shutdown) {
         flags     = 0;
         received = rsp_recvfullmsg(RSerPoolSocketDescriptor,
                                    (char*)&buffer, sizeof(buffer),
                                    &rinfo, &flags, 5000);
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
      }
   }

   if((eventHandlingResult == EHR_Abort) ||
      (eventHandlingResult == EHR_Shutdown)) {
      rsp_sendmsg(RSerPoolSocketDescriptor,
                  NULL, 0, 0,
                  0, 0, 0, 0,
                  (eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_EOF,
                  0);
   }

   finishSession(eventHandlingResult);

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
                                struct TagItem*      tags)
{
   if(rsp_initialize(info) < 0) {
      puts("ERROR: Unable to initialize rsplib Library!\n");
      return;
   }

   int rserpoolSocket = rsp_socket(0, SOCK_STREAM, IPPROTO_SCTP);
   if(rserpoolSocket >= 0) {
      // ====== Initialize PE settings ======================================
      struct rsp_loadinfo dummyLoadinfo;
      if(loadinfo == NULL) {
         memset(&dummyLoadinfo, 0, sizeof(dummyLoadinfo));
         loadinfo = &dummyLoadinfo;
         loadinfo->rli_policy = PPT_ROUNDROBIN;
      }

      // ====== Server startup ==============================================
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

         // ====== Print program title ======================================
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
         if(printParameters) {
            printParameters(userData);
         }

         // ====== Register PE ==============================================
         if(rsp_register_tags(rserpoolSocket,
                              (const unsigned char*)poolHandle, strlen(poolHandle),
                              loadinfo, reregInterval, 0, tags) == 0) {
            uint32_t identifier;
            if(rsp_getsockname(rserpoolSocket, NULL, NULL, &identifier) == 0) {
               puts("Registration");
               printf("   Identifier              = $%08x\n\n", identifier);
            }

            // ====== Main loop =============================================
            TCPLikeServerList        serverSet(maxThreads);
            const unsigned long long autoStopTimeStamp =
               (runtimeLimit > 0) ? (getMicroTime() + (1000ULL * runtimeLimit)) : 0;
            installBreakDetector();
            while(!breakDetected()) {
               // ====== Clean-up session list ==============================
               serverSet.removeFinished();

               // ====== Wait for incoming sessions =========================
               int newRSerPoolSocket = rsp_accept(rserpoolSocket, 25);
               if(newRSerPoolSocket >= 0) {
                  TCPLikeServer* serviceThread = threadFactory(newRSerPoolSocket, userData);
                  if((serviceThread) && (serverSet.add(serviceThread))) {
                     serviceThread->start();
                  }
                  else {
                     puts("Rejected new session");
                     rsp_close(newRSerPoolSocket);
                  }
               }

               // ====== Do reregistration on load changes ==================
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

               // ====== Auto-stop ==========================================
               if((autoStopTimeStamp > 0) &&
                  (getMicroTime() > autoStopTimeStamp)) {
                  puts("Auto-stop reached!");
                  break;
               }
            }

            // ====== Clean up ==============================================
            serverSet.removeAll();
            rsp_deregister(rserpoolSocket, DEREGF_DONTWAIT);
         }
         if(finishService != NULL) {
            finishService(userData);
         }
      }
      else {
         printf("ERROR: Failed to register PE to pool %s\n", poolHandle);
      }
      rsp_close(rserpoolSocket);
   }
   else {
      logerror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   puts("\nTerminated!");
}




// ###### Constructor #######################################################
TCPLikeServerList::TCPLikeServerList(size_t maxThreads)
{
   ThreadList = NULL;
   LoadSum    = 0;
   Threads    = 0;
   MaxThreads = maxThreads;
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
void TCPLikeServerList::removeFinished()
{
   lock();
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      ThreadListEntry* next = entry->Next;
      if(entry->Object->hasFinished()) {
         remove(entry->Object);
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
