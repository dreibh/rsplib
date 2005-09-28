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


// ###### Constructor #######################################################
TCPLikeServer::TCPLikeServer(int rserpoolSocketDescriptor)
{
   RSerPoolSocketDescriptor = rserpoolSocketDescriptor;
   ServerList   = NULL;
   IsNewSession = true;
   Shutdown     = false;
   Finished     = false;
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


// ###### Shutdown thread ###################################################
void TCPLikeServer::shutdown()
{
   if(!Finished) {
      Shutdown = true;
   }
}


// ###### Handle cookie #####################################################
EventHandlingResult TCPLikeServer::handleCookieEcho(const char* buffer, size_t bufferSize)
{
   printTimeStamp(stdout);
   puts("COOKIE ECHO");
   return(EHR_Okay);
}


// ###### Handle notification ###############################################
EventHandlingResult TCPLikeServer::handleNotification(const union rserpool_notification* notification)
{
   printTimeStamp(stdout);
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
   return(EHR_Okay);
}


// ###### TCPLike server main loop #########################################
void TCPLikeServer::run()
{
   char                  buffer[65536];
   struct rsp_sndrcvinfo rinfo;
   ssize_t               received;
   int                   flags;

   while(!Shutdown) {
      flags     = 0;
      received = rsp_recvmsg(RSerPoolSocketDescriptor,
                             (char*)&buffer, sizeof(buffer),
                             &rinfo, &flags, 1000000);
      if(received > 0) {
         // ====== Handle event =============================================
         EventHandlingResult eventHandlingResult;
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

         // ====== Check for problems =======================================
         if((eventHandlingResult == EHR_Abort) ||
            (eventHandlingResult == EHR_Shutdown)) {
            rsp_sendmsg(RSerPoolSocketDescriptor,
                        NULL, 0, (eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_EOF,
                        0, 0, 0, 0, 0);
            break;
         }
      }
      else if(received == 0) {
         break;
      }
   }

   Finished = true;
}


// ###### Implementation of a simple threaded server ########################
void TCPLikeServer::poolElement(const char*          programTitle,
                                const char*          poolHandle,
                                struct rsp_info*     info,
                                struct rsp_loadinfo* loadinfo,
                                void*                userData,
                                TCPLikeServer*      (*threadFactory)(int sd, void* userData))
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
         loadinfo->rli_policy = PPT_ROUNDROBIN;
      }

      // ====== Print program title =========================================
      puts(programTitle);
      for(size_t i = 0;i < strlen(programTitle);i++) {
         printf("=");
      }
      puts("\n");

      // ====== Register PE =================================================
      if(rsp_register(rserpoolSocket,
                      (const unsigned char*)poolHandle, strlen(poolHandle),
                      loadinfo, 30000) == 0) {

         // ====== Main loop ===================================================
         TCPLikeServerList serverSet;
         installBreakDetector();
         while(!breakDetected()) {
            // ====== Clean-up session list ====================================
            serverSet.removeFinished();

            // ====== Wait for incoming sessions ===============================
            int newRSerPoolSocket = rsp_accept(rserpoolSocket, 100000);
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
         }

         // ====== Clean up ====================================================
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
}


// ###### Destructor ########################################################
TCPLikeServerList::~TCPLikeServerList()
{
   removeAll();
}


// ###### Get number of threads #############################################
size_t TCPLikeServerList::getThreads()
{
   size_t count;
   lock();
   count = Threads;
   unlock();
   return(count);
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
