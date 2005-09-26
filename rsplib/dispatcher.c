/*
 *  $Id$
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
#include <netinet/in.h>
#include <ext_socket.h>


/* ###### Constructor #################################################### */
void dispatcherNew(struct Dispatcher* dispatcher,
                   void               (*lock)(struct Dispatcher* dispatcher, void* userData),
                   void               (*unlock)(struct Dispatcher* dispatcher, void* userData),
                   void*              lockUserData)
{
   leafLinkedRedBlackTreeNew(&dispatcher->TimerStorage, NULL, timerComparison);
   leafLinkedRedBlackTreeNew(&dispatcher->FDCallbackStorage, NULL, fdCallbackComparison);

   dispatcher->AddRemove    = false;
   dispatcher->LockUserData = lockUserData;

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
}


/* ###### Destructor ##################################################### */
void dispatcherDelete(struct Dispatcher* dispatcher)
{
   CHECK(leafLinkedRedBlackTreeIsEmpty(&dispatcher->TimerStorage));
   CHECK(leafLinkedRedBlackTreeIsEmpty(&dispatcher->FDCallbackStorage));
   leafLinkedRedBlackTreeDelete(&dispatcher->TimerStorage);
   leafLinkedRedBlackTreeDelete(&dispatcher->FDCallbackStorage);
   dispatcher->Lock         = NULL;
   dispatcher->Unlock       = NULL;
   dispatcher->LockUserData = NULL;
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
   struct LeafLinkedRedBlackTreeNode* node;
   struct Timer*                      timer;
   unsigned long long                 now;

   dispatcherLock(dispatcher);

   node = leafLinkedRedBlackTreeGetFirst(&dispatcher->TimerStorage);
   while(node != NULL) {
      timer = (struct Timer*)node;
      now   = getMicroTime();

      if(now >= timer->TimeStamp) {
         timer->TimeStamp = 0;
         leafLinkedRedBlackTreeRemove(&dispatcher->TimerStorage,
                                      &timer->Node);
         if(timer->Callback != NULL) {
            timer->Callback(dispatcher, timer, timer->UserData);
         }
      }
      else {
         break;
      }
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->TimerStorage);
   }

   dispatcherUnlock(dispatcher);
}


/* ###### Get poll() parameters ########################################## */
void dispatcherGetPollParameters(struct Dispatcher*  dispatcher,
                                 struct pollfd*      ufds,
                                 unsigned int*       nfds,
                                 int*                timeout,
                                 unsigned long long* pollTimeStamp)
{
   struct LeafLinkedRedBlackTreeNode* node;
   struct FDCallback*                 fdCallback;
   struct Timer*                      timer;
   int64                              timeToNextEvent;

   *nfds    = 0;
   *timeout = -1;
   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);

      /*  ====== Create fdset for poll() ================================= */
      *pollTimeStamp = getMicroTime();
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->FDCallbackStorage);
      while(node != NULL) {
         fdCallback = (struct FDCallback*)node;
         if(fdCallback->EventMask & (FDCE_Read|FDCE_Write|FDCE_Exception)) {
            fdCallback->SelectTimeStamp = *pollTimeStamp;
            ufds[*nfds].fd     = fdCallback->FD;
            ufds[*nfds].events = fdCallback->EventMask & (FDCE_Read|FDCE_Write|FDCE_Exception);
            (*nfds)++;
         }
         else {
            LOG_WARNING
            fputs("Empty event mask?!\n",stdlog);
            LOG_END
         }
         node = leafLinkedRedBlackTreeGetNext(&dispatcher->FDCallbackStorage, node);
      }

      /*  ====== Get time to next timer event ============================ */
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->TimerStorage);
      if(node != NULL) {
         timer = (struct Timer*)node;
         timeToNextEvent = max((int64)0,(int64)timer->TimeStamp - (int64)*pollTimeStamp);
         *timeout = (int)(timeToNextEvent / 1000);
      }
      else {
         *timeout = -1;
      }

      dispatcherUnlock(dispatcher);
   }
}


/* ###### Find FDCallback for given file descriptor ###################### */
static struct FDCallback* dispatcherFindFDCallbackForDescriptor(struct Dispatcher* dispatcher,
                                                                int                fd)
{
   struct FDCallback                  cmpNode;
   struct LeafLinkedRedBlackTreeNode* node;

   cmpNode.FD = fd;
   node = leafLinkedRedBlackTreeFind(&dispatcher->FDCallbackStorage, &cmpNode.Node);
   if(node != NULL) {
      return((struct FDCallback*)node);
   }
   return(NULL);
}


/* ###### Handle poll() result ########################################### */
void dispatcherHandlePollResult(struct Dispatcher* dispatcher,
                                int                result,
                                struct pollfd*     ufds,
                                unsigned int       nfds,
                                int                timeout,
                                unsigned long long pollTimeStamp)
{
   struct FDCallback* fdCallback;
   unsigned int       i;

   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);

      /* ====== Handle events ============================================ */
      /* We handle the FD callbacks first, because their corresponding FD's
         state has been returned by ext_poll(), since a timer callback
         might modify them (e.g. writing to a socket and reading the
         complete results).
      */
      if(result > 0) {
         LOG_VERBOSE3
         fputs("Handling FD events...\n", stdlog);
         LOG_END
         for(i = 0;i < nfds;i++) {
            if(ufds[i].revents) {
               fdCallback = dispatcherFindFDCallbackForDescriptor(dispatcher, ufds[i].fd);
               if(fdCallback != NULL) {
                  if(fdCallback->SelectTimeStamp <= pollTimeStamp) {
                     if(ufds[i].revents & fdCallback->EventMask) {
                        LOG_VERBOSE3
                        fprintf(stdlog,"Event $%04x (mask $%04x) for socket %d\n",
                              ufds[i].revents, fdCallback->EventMask, fdCallback->FD);
                        LOG_END

                        if(fdCallback->Callback != NULL) {
                           LOG_VERBOSE2
                           fprintf(stdlog,"Executing callback for event $%04x of socket %d\n",
                                 ufds[i].revents, fdCallback->FD);
                           LOG_END
                           fdCallback->Callback(dispatcher,
                                                fdCallback->FD, ufds[i].revents,
                                                fdCallback->UserData);
                        }
                     }
                  }
                  else {
                     LOG_WARNING
                     fprintf(stdlog, "FD callback for FD %d is newer than begin of ext_poll() -> Skipping.\n", fdCallback->FD);
                     LOG_END
                  }
               }
               else {
                  LOG_WARNING
                  fprintf(stdlog,"FD callback for socket %d is gone. Something is going wrong! Have you set nfds correctly?\n", ufds[i].fd);
                  LOG_END
               }
            }
         }
      }

      /* Timers must be handled after the FD callbacks, since
         they might modify the FDs' states (e.g. completely
         reading their buffers, establishing new associations, ...)! */
      LOG_VERBOSE3
      fputs("Handling timer events...\n", stdlog);
      LOG_END
      dispatcherHandleTimerEvents(dispatcher);

      dispatcherUnlock(dispatcher);
   }
}


/* ###### Dispatcher event loop ########################################## */
void dispatcherEventLoop(struct Dispatcher* dispatcher)
{
   unsigned long long   pollTimeStamp;
   struct pollfd        ufds[FD_SETSIZE];
   unsigned int         nfds;
   int                  timeout;
   int                  result;

   if(dispatcher != NULL) {
      dispatcherGetPollParameters(dispatcher,
                                 (struct pollfd*)&ufds, &nfds, &timeout,
                                 &pollTimeStamp);
      result = ext_poll((struct pollfd*)&ufds, nfds, timeout);
      if(result < 0) {
         logerror("poll() failed");
         exit(1);
      }
      dispatcherHandlePollResult(dispatcher, result,
                                 (struct pollfd*)&ufds, nfds, timeout,
                                 pollTimeStamp);
   }
}




/* ??????????????????????????????????????????????????????????????????????? */
/* ###### Get select() parameters ######################################## */
void dispatcherGetSelectParameters(struct Dispatcher*  dispatcher,
                                   int*                n,
                                   fd_set*             readfdset,
                                   fd_set*             writefdset,
                                   fd_set*             exceptfdset,
                                   fd_set*             testfdset,
                                   unsigned long long* testTS,
                                   struct timeval*     timeout)
{
   struct LeafLinkedRedBlackTreeNode* node;
   struct FDCallback*                 fdCallback;
   struct Timer*                      timer;
   unsigned long long                 now;
   int64                              timeToNextEvent;
   int                                fds;

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
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->FDCallbackStorage);
      while(node != NULL) {
         fdCallback = (struct FDCallback*)node;
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
         node = leafLinkedRedBlackTreeGetNext(&dispatcher->FDCallbackStorage, node);
      }

      /*  ====== Get time to next timer event ============================ */
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->TimerStorage);
      if(node != NULL) {
         timer = (struct Timer*)node;
         timeToNextEvent = max((int64)0,(int64)timer->TimeStamp - (int64)now);
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
void dispatcherHandleSelectResult(struct Dispatcher*       dispatcher,
                                  int                      result,
                                  fd_set*                  readfdset,
                                  fd_set*                  writefdset,
                                  fd_set*                  exceptfdset,
                                  fd_set*                  testfdset,
                                  const unsigned long long testTS)
{
   struct LeafLinkedRedBlackTreeNode* node;
   struct FDCallback*                 fdCallback;
   int                                eventMask;

   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);

      /* ====== Handle events ============================================ */
      /* We handle the FD callbacks first, because their corresponding FD's
         state has been returned by ext_select(), since a timer callback
         might modify them (e.g. writing to a socket and reading the
         complete results).
      */
      if(result > 0) {
         LOG_VERBOSE3
         fputs("Handling FD events...\n", stdlog);
         LOG_END
         dispatcher->AddRemove = false;
         node = leafLinkedRedBlackTreeGetFirst(&dispatcher->FDCallbackStorage);
         while(node != NULL) {
            fdCallback = (struct FDCallback*)node;
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
                        node = leafLinkedRedBlackTreeGetFirst(&dispatcher->FDCallbackStorage);
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
            node = leafLinkedRedBlackTreeGetNext(&dispatcher->FDCallbackStorage, node);
         }
      }

      /* Timers must be handled after the FD callbacks, since
         they might modify the FDs' states (e.g. completely
         reading their buffers, establishing new associations, ...)! */
      LOG_VERBOSE3
      fputs("Handling timer events...\n", stdlog);
      LOG_END
      dispatcherHandleTimerEvents(dispatcher);

      dispatcherUnlock(dispatcher);
   }
}
/* ??????????????????????????????????????????????????????????????????????? */
