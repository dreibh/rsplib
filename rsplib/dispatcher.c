/*
 *  $Id: dispatcher.c,v 1.2 2004/07/18 15:30:42 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Dispatcher
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "dispatcher.h"

#include <ext_socket.h>



struct FDCallback
{
   struct Dispatcher* Master;
   int                FD;
   int                EventMask;

   void               (*Callback)(struct Dispatcher* dispatcher,
                                  int                fd,
                                  int                eventMask,
                                  void*              userData);
   card64             SelectTimeStamp;
   void*              UserData;
};



/* ###### Constructor #################################################### */
struct Dispatcher* dispatcherNew(void (*lock)(struct Dispatcher* dispatcher, void* userData),
                                 void (*unlock)(struct Dispatcher* dispatcher, void* userData),
                                 void* lockUserData)
{
   struct Dispatcher* dispatcher = (struct Dispatcher*)malloc(sizeof(struct Dispatcher));
   if(dispatcher != NULL) {
      if(lock != NULL) {
         dispatcher->Lock = lock;
      }
      else {
         dispatcher->Lock = dispatcherDefaultLock;
      }
      if(unlock != NULL) {
         dispatcher->Unlock = unlock;
      }
      else {
         dispatcher->Unlock = dispatcherDefaultUnlock;
      }
      dispatcher->LockUserData = lockUserData;
      dispatcher->TimerList    = NULL;
      dispatcher->SocketList   = NULL;
   }
   return(dispatcher);
}


/* ###### Destructor ##################################################### */
void dispatcherDelete(struct Dispatcher* dispatcher)
{
   if(dispatcher != NULL) {
      free(dispatcher);
   }
}


/* ###### Lock ########################################################### */
void dispatcherLock(struct Dispatcher* dispatcher)
{
   if(dispatcher != NULL) {
      dispatcher->Lock(dispatcher, dispatcher->LockUserData);
   }
}


/* ###### Unlock ######################################################### */
void dispatcherUnlock(struct Dispatcher* dispatcher)
{
   if(dispatcher != NULL) {
      dispatcher->Unlock(dispatcher, dispatcher->LockUserData);
   }
}


/* ###### Default locking function ####################################### */
void dispatcherDefaultLock(struct Dispatcher* dispatcher, void* userData)
{
}


/* ###### Default unlocking function ##################################### */
void dispatcherDefaultUnlock(struct Dispatcher* dispatcher, void* userData)
{
}


/* ###### Handle timer events ############################################ */
static void dispatcherHandleTimerEvents(struct Dispatcher* dispatcher)
{
   GList*        timerList;
   struct Timer* timer;
   card64        now;
   dispatcherLock(dispatcher);

   dispatcher->AddRemove = false;
   timerList = g_list_first(dispatcher->TimerList);
   while(timerList != NULL) {
      timer = (struct Timer*)timerList->data;
      now   = getMicroTime();

      if(now >= timer->Time) {
         timer->Time = 0;
         dispatcher->TimerList = g_list_remove(dispatcher->TimerList,timer);
         timerList = g_list_first(dispatcher->TimerList);

         if(timer->Callback != NULL) {
            timer->Callback(dispatcher,timer,timer->UserData);
            if(dispatcher->AddRemove == true) {
               dispatcher->AddRemove = false;
               timerList = g_list_first(dispatcher->TimerList);
            }
         }
      }
      else {
         break;
      }
   }

   dispatcherUnlock(dispatcher);
}


/* ###### Get select() parameters ######################################## */
void dispatcherGetSelectParameters(struct Dispatcher* dispatcher,
                                   int*               n,
                                   fd_set*            readfdset,
                                   fd_set*            writefdset,
                                   fd_set*            exceptfdset,
                                   fd_set*            testfdset,
                                   card64*            testTS,
                                   struct timeval*    timeout)
{
   GList*             glist;
   struct FDCallback* fdCallback;
   struct Timer*      timer;
   card64             now;
   int64              timeToNextEvent;
   int                fds;

   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);
      now = getMicroTime();
      *testTS = now;

      /*  ====== Create FD sets for select() ============================= */
      FD_ZERO(readfdset);
      FD_ZERO(writefdset);
      FD_ZERO(exceptfdset);
      FD_ZERO(testfdset);
      fds = 0;
      glist = g_list_first(dispatcher->SocketList);
      while(glist != NULL) {
         fdCallback = (struct FDCallback*)glist->data;
         if(fdCallback->EventMask & (FDCE_Read|FDCE_Write|FDCE_Exception)) {
            FD_SET(fdCallback->FD, testfdset);
            fdCallback->SelectTimeStamp = now;
            fds = max(fds,fdCallback->FD);
            if(fdCallback->EventMask & FDCE_Read) {
               FD_SET(fdCallback->FD,readfdset);
            }
            if(fdCallback->EventMask & FDCE_Write) {
               FD_SET(fdCallback->FD,writefdset);
            }
            if(fdCallback->EventMask & FDCE_Exception) {
               FD_SET(fdCallback->FD,exceptfdset);
            }
         }
         else {
            LOG_WARNING
            fputs("Empty event mask?!\n",stdlog);
            LOG_END
         }
         glist = g_list_next(glist);
      }

      /*  ====== Get time to next timer event ============================ */
      glist = g_list_first(dispatcher->TimerList);
      if(glist != NULL) {
         timer = (struct Timer*)glist->data;
         timeToNextEvent = max((int64)0,(int64)timer->Time - (int64)now);
      }
      else {
         timeToNextEvent = 10000000;
      }
      if(timeToNextEvent > 10000000) {
         timeToNextEvent = 10000000;
      }

      dispatcherUnlock(dispatcher);


      /* ====== Wait ===================================================== */
      timeout->tv_sec  = (timeToNextEvent / 1000000);
      timeout->tv_usec = (timeToNextEvent % 1000000);
      *n = fds + 1;
   }
   else {
      *n = 0;
   }
}


/* ###### Handle select() result ######################################### */
void dispatcherHandleSelectResult(struct Dispatcher* dispatcher,
                                  int                result,
                                  fd_set*            readfdset,
                                  fd_set*            writefdset,
                                  fd_set*            exceptfdset,
                                  fd_set*            testfdset,
                                  const card64       testTS)
{
   GList*             glist;
   struct FDCallback* fdCallback;
   int                eventMask;

   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);

      /* ====== Handle events ============================================ */
      dispatcherHandleTimerEvents(dispatcher);
      if(result > 0) {
         dispatcher->AddRemove = false;
         glist = g_list_first(dispatcher->SocketList);
         while(glist != NULL) {
            fdCallback = (struct FDCallback*)glist->data;
            if(fdCallback->SelectTimeStamp <= testTS) {
               eventMask  = 0;
               if(FD_ISSET(fdCallback->FD,readfdset)) {
                  eventMask |= FDCE_Read;
                  FD_CLR(fdCallback->FD,readfdset);
               }
               if(FD_ISSET(fdCallback->FD,writefdset)) {
                  eventMask |= FDCE_Write;
                  FD_CLR(fdCallback->FD,writefdset);
               }
               if(FD_ISSET(fdCallback->FD,exceptfdset)) {
                  eventMask |= FDCE_Exception;
                  FD_CLR(fdCallback->FD,exceptfdset);
               }

               if(eventMask & fdCallback->EventMask) {
                  LOG_VERBOSE3
                  fprintf(stdlog,"Event $%04x (mask $%04x) for socket %d\n",
                          eventMask, fdCallback->EventMask, fdCallback->FD);
                  LOG_END

                  if(fdCallback->Callback != NULL) {
                     LOG_VERBOSE2
                     fprintf(stdlog,"Executing callback for event $%04x of socket %d\n",
                             eventMask, fdCallback->FD);
                     LOG_END

                     fdCallback->Callback(dispatcher,
                                          fdCallback->FD, eventMask,
                                          fdCallback->UserData);
                     if(dispatcher->AddRemove == true) {
                        dispatcher->AddRemove = false;
                        glist = g_list_first(dispatcher->SocketList);
                        continue;
                     }
                  }
               }
            }
            else {
               LOG_VERBOSE4
               fprintf(stdlog, "FD callback for FD %d is newer than begin of ext_select() -> Skipping.\n", fdCallback->FD);
               LOG_END
            }
            glist = g_list_next(glist);
         }
      }

      dispatcherUnlock(dispatcher);
   }
}


/* ###### Dispatcher event loop ########################################## */
void dispatcherEventLoop(struct Dispatcher* dispatcher)
{
   int            n;
   int            result;
   struct timeval timeout;
   fd_set         readfdset;
   fd_set         writefdset;
   fd_set         exceptfdset;
   fd_set         testfdset;
   card64         testTS;

   if(dispatcher != NULL) {
      dispatcherGetSelectParameters(dispatcher, &n, &readfdset, &writefdset, &exceptfdset, &testfdset, &testTS, &timeout);

      result = ext_select(n, &readfdset, &writefdset, &exceptfdset, &timeout);
      if(result < 0) {
         logerror("select() failed");
         exit(1);
      }

      dispatcherHandleSelectResult(dispatcher, result, &readfdset, &writefdset, &exceptfdset, &testfdset, testTS);
   }
}


/* ###### File descriptor callback comparision function ################## */
static gint fdCallbackCompareFunc(gconstpointer a,
                                  gconstpointer b)
{
   struct FDCallback* t1 = (struct FDCallback*)a;
   struct FDCallback* t2 = (struct FDCallback*)b;
   if(t1->FD < t2->FD) {
      return(-1);
   }
   else if(t1->FD > t2->FD) {
      return(1);
   }
   return(0);
}


/* ###### Add file descriptor callback ################################### */
bool dispatcherAddFDCallback(struct Dispatcher* dispatcher,
                             int                fd,
                             int                eventMask,
                             void               (*callback)(struct Dispatcher* dispatcher,
                                                            int                fd,
                                                            int                eventMask,
                                                            void*              userData),
                             void*              userData)
{
   struct FDCallback* fdCallback;
   if(dispatcher != NULL) {
      if((fd < 0) || (fd >= FD_SETSIZE)) {
         LOG_ERROR
         fprintf(stderr, "Invalid FD %d specified\n", fd);
         LOG_END_FATAL
         return(false);
      }
      fdCallback = (struct FDCallback*)malloc(sizeof(struct FDCallback));
      if(fdCallback != NULL) {
         fdCallback->Master    = dispatcher;
         fdCallback->FD        = fd;
         fdCallback->EventMask = eventMask;
         fdCallback->Callback  = callback;
         fdCallback->UserData  = userData;
         fdCallback->SelectTimeStamp = getMicroTime();
         dispatcherLock(dispatcher);
         dispatcher->SocketList = g_list_insert_sorted(dispatcher->SocketList,
                                                      (gpointer)fdCallback,
                                                      fdCallbackCompareFunc);
         dispatcher->AddRemove = true;
         dispatcherUnlock(dispatcher);
         return(true);
      }
   }
   return(false);
}


/* ###### Update file descriptor callback ################################ */
void dispatcherUpdateFDCallback(struct Dispatcher* dispatcher,
                                int                fd,
                                int                eventMask)
{
   GList*             found;
   struct FDCallback* fdCallback;
   struct FDCallback  example;

   example.FD = fd;

   dispatcherLock(dispatcher);
   found = g_list_find_custom(dispatcher->SocketList,(gpointer)&example,fdCallbackCompareFunc);
   if(found != NULL) {
      fdCallback = (struct FDCallback*)found->data;
      fdCallback->EventMask = eventMask;
   }
   dispatcherUnlock(dispatcher);
}


/* ###### Remove file descriptor callback ################################ */
void dispatcherRemoveFDCallback(struct Dispatcher* dispatcher,
                                int                fd)
{
   GList*           found;
   struct FDCallback* fdCallback;
   struct FDCallback  example;

   example.FD = fd;

   dispatcherLock(dispatcher);
   found = g_list_find_custom(dispatcher->SocketList,(gpointer)&example,fdCallbackCompareFunc);
   if(found != NULL) {
      fdCallback = (struct FDCallback*)found->data;
      dispatcher->SocketList = g_list_remove(dispatcher->SocketList,fdCallback);
      dispatcher->AddRemove  = true;
      free(fdCallback);
   }
   dispatcherUnlock(dispatcher);
}
