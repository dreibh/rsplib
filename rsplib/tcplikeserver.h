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
 * Copyright (C) 2004-2014 by Thomas Dreibholz
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

#ifndef TCPLIKESERVER_H
#define TCPLIKESERVER_H

#include "rserpool-internals.h"
#include "cpprspserver.h"
#include "thread.h"


struct TagItem;
class TCPLikeServer;

class TCPLikeServerList : public TDMutex
{
   public:
   TCPLikeServerList(size_t maxThreads, int systemNotificationPipe);
   ~TCPLikeServerList();
   bool add(TCPLikeServer* thread);
   void remove(TCPLikeServer* thread);
   size_t handleRemovalsAndTimers();
   void removeAll();

   double getTotalLoad();
   size_t getThreads();
   inline size_t getMaxThreads() const {
      return(MaxThreads);
   }
   inline bool hasCapacity() const {
      return(Threads < MaxThreads);
   }

   private:
   friend class TCPLikeServer;
   struct ThreadListEntry {
      ThreadListEntry* Next;
      TCPLikeServer*   Object;
   };
   ThreadListEntry* ThreadList;
   size_t           Threads;
   size_t           MaxThreads;
   int              SystemNotificationPipe;
   unsigned int     LoadSum;
};


class TCPLikeServer : public TDThread
{
   friend class TCPLikeServerList;

   public:
   TCPLikeServer(int rserpoolSocketDescriptor);
   ~TCPLikeServer();

   inline bool hasFinished() {
      lock();
      const bool finished = Finished;
      unlock();
      return(finished);
   }
   inline bool isShuttingDown() {
      lock();
      const bool shutdown = Shutdown;
      unlock();
      return(shutdown);
   }
   inline void setSyncTimer(const unsigned long long timeStamp) {
      lock();
      SyncTimerTimeStamp = timeStamp;
      unlock();
   }
   inline void setAsyncTimer(const unsigned long long timeStamp) {
      lock();
      AsyncTimerTimeStamp = timeStamp;
      unlock();
   }
   inline TCPLikeServerList* getServerList() const {
      return(ServerList);
   }
   inline void setServerList(TCPLikeServerList* serverList) {
      ServerList = serverList;
   }
   void shutdown();

   double getLoad() const;
   void setLoad(double load);

   static void poolElement(const char*          programTitle,
                           const char*          poolHandle,
                           struct rsp_info*     info,
                           struct rsp_loadinfo* loadinfo,
                           size_t               maxThreads,
                           TCPLikeServer*       (*threadFactory)(int sd, void* userData, uint32_t peIdentifier),
                           void                 (*printParameters)(const void* userData),
                           void                 (*rejectNewSession)(int sd),
                           bool                 (*initializeService)(void* userData),
                           void                 (*finishService)(void* userData),
                           double               (*loadUpdateHook)(const double load),
                           void*                userData,
                           const sockaddr*      localAddressSet = NULL,
                           const size_t         localAddresses  = 0,
                           unsigned int         reregInterval   = 30000,
                           unsigned int         runtimeLimit    = 0,
                           const bool           quiet           = false,
                           const bool           daemonMode      = false,
                           struct TagItem*      tags            = NULL);

   protected:
   int                RSerPoolSocketDescriptor;
   TCPLikeServerList* ServerList;

   protected:
   virtual EventHandlingResult initializeSession();
   virtual void finishSession(EventHandlingResult result);
   virtual EventHandlingResult syncTimerEvent(const unsigned long long now);
   virtual void asyncTimerEvent(const unsigned long long now);
   virtual EventHandlingResult handleMessage(const char* buffer,
                                             size_t      bufferSize,
                                             uint32_t    ppid,
                                             uint16_t    streamID);
   virtual EventHandlingResult handleCookieEcho(const char* buffer, size_t bufferSize);
   virtual EventHandlingResult handleNotification(const union rserpool_notification* notification);

   private:
   void run();

   unsigned int       Load;
   bool               IsNewSession;
   bool               Shutdown;
   bool               Finished;
   unsigned long long SyncTimerTimeStamp;
   unsigned long long AsyncTimerTimeStamp;
};


#endif
