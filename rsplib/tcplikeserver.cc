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
EventHandlingResult TCPLikeServer::initialize()
{
   return(EHR_Okay);
}


// ###### Finish of thread ##################################################
void TCPLikeServer::finish(EventHandlingResult result)
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
   return((double)Load / (double)0xffffffff);
}


// ##### Set load ###########################################################
void TCPLikeServer::setLoad(double load)
{
   if((load < 0.0) || (load > 1.0)) {
      fputs("ERROR: Invalid load setting!\n", stderr);
      return;
   }

   CHECK(ServerList != NULL);
   CHECK(ServerList->LoadSum >= Load);

   ServerList->lock();
   ServerList->LoadSum -= Load;
   Load = (unsigned int)rint(load * (double)0xffffffff);
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

   eventHandlingResult = initialize();
   if(eventHandlingResult == EHR_Okay) {
      while(!Shutdown) {
         flags     = 0;
         received = rsp_recvmsg(RSerPoolSocketDescriptor,
                              (char*)&buffer, sizeof(buffer),
                              &rinfo, &flags, 1000000);
         if(received > 0) {
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
                  NULL, 0, (eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_EOF,
                  0, 0, 0, 0, 0);
   }

   finish(eventHandlingResult);

   Finished = true;
}


// ###### Implementation of a simple threaded server ########################
void TCPLikeServer::poolElement(const char*          programTitle,
                                const char*          poolHandle,
                                struct rsp_info*     info,
                                struct rsp_loadinfo* loadinfo,
                                TCPLikeServer*       (*threadFactory)(int sd, void* userData),
                                void*                userData,
                                unsigned int         reregInterval,
                                unsigned int         runtimeLimit,
                                struct TagItem*      tags)
{
   beginLogging();
   if(rsp_initialize(info) < 0) {
      puts("ERROR: Unable to initialize rsplib Library!\n");
      finishLogging();
      return;
   }

   int rserpoolSocket = rsp_socket(0, SOCK_STREAM, IPPROTO_SCTP);
   if(rserpoolSocket >= 0) {
      // ====== Initialize PE settings ======================================
      struct rsp_loadinfo dummyLoadinfo;
      if(loadinfo == NULL) {
         memset(&dummyLoadinfo, 0, sizeof(dummyLoadinfo));
         loadinfo = &dummyLoadinfo;
         loadinfo->rli_policy = PPT_LEASTUSED; // ?????
      }

      // ====== Print program title =========================================
      puts(programTitle);
      for(size_t i = 0;i < strlen(programTitle);i++) {
         printf("=");
      }
      puts("\n");

      // ====== Register PE =================================================
      if(rsp_register_tags(rserpoolSocket,
                           (const unsigned char*)poolHandle, strlen(poolHandle),
                           loadinfo, reregInterval, tags) == 0) {

         // ====== Main loop ================================================
         TCPLikeServerList        serverSet;
         double                   oldLoad = 0.0;
         const unsigned long long autoStopTimeStamp =
            (runtimeLimit > 0) ? (getMicroTime() + (1000ULL * runtimeLimit)) : 0;
         installBreakDetector();
         while(!breakDetected()) {
            // ====== Clean-up session list =================================
            serverSet.removeFinished();

            // ====== Wait for incoming sessions ============================
            int newRSerPoolSocket = rsp_accept(rserpoolSocket, 500000);
            if(newRSerPoolSocket >= 0) {
               TCPLikeServer* serviceThread = threadFactory(newRSerPoolSocket, userData);
               if(serviceThread) {
                  serverSet.add(serviceThread);
                  serviceThread->start();
               }
               else {
                  rsp_close(newRSerPoolSocket);
               }
            }

            // ====== Do reregistration on load changes =====================
            if((loadinfo->rli_policy == PPT_LEASTUSED) ||
               (loadinfo->rli_policy == PPT_LEASTUSED_DEGRADATION) ||
               (loadinfo->rli_policy == PPT_PRIORITY_LEASTUSED) ||
               (loadinfo->rli_policy == PPT_PRIORITY_LEASTUSED_DEGRADATION) ||
               (loadinfo->rli_policy == PPT_RANDOMIZED_LEASTUSED) ||
               (loadinfo->rli_policy == PPT_RANDOMIZED_LEASTUSED_DEGRADATION) ||
               (loadinfo->rli_policy == PPT_RANDOMIZED_PRIORITY_LEASTUSED) ||
               (loadinfo->rli_policy == PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION)) {
               const double newLoad = serverSet.getTotalLoad();
               if(fabs(newLoad - oldLoad) >= 0.01) {
                  oldLoad = newLoad;
                  struct TagItem mytags[4];
                  loadinfo->rli_load = (unsigned int)rint(newLoad * (double)0xffffff);
                  mytags[0].Tag  = TAG_RspPERegistration_WaitForResult;
                  mytags[0].Data = 0;
                  mytags[1].Tag  = TAG_MORE;
                  mytags[1].Data = (tagdata_t)tags;
                  rsp_register_tags(rserpoolSocket,
                                    (const unsigned char*)poolHandle, strlen(poolHandle),
                                    loadinfo, 30000, (TagItem*)&mytags);
               }
            }

            // ====== Auto-stop =============================================
            if((autoStopTimeStamp > 0) &&
               (getMicroTime() > autoStopTimeStamp)) {
               puts("Auto-stop reached!");
               break;
            }
         }

         // ====== Clean up =================================================
         serverSet.removeAll();
         rsp_deregister(rserpoolSocket);
      }
      else {
         printf("Failed to register PE to pool %s\n", poolHandle);
      }
      rsp_close(rserpoolSocket);
   }
   else {
      logerror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   finishLogging();
   puts("\nTerminated!");
}




// ###### Constructor #######################################################
TCPLikeServerList::TCPLikeServerList()
{
   ThreadList = NULL;
   Threads    = 0;
   LoadSum    = 0;
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
void TCPLikeServerList::add(TCPLikeServer* thread)
{
   ThreadListEntry* entry = new ThreadListEntry;
   if(entry) {
      lock();
      entry->Next   = ThreadList;
      entry->Object = thread;
      ThreadList    = entry;

      thread->setServerList(this);
      Threads++;
      unlock();
   }
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
         return;
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
      return(loadSum / (threads * (double)0xffffffff));
   }
   return(0.0);
}
