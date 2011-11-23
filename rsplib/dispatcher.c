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
 * Copyright (C) 2002-2012 by Thomas Dreibholz
 *
 * Acknowledgements:
 * Realized in co-operation between Siemens AG and
 * University of Essen, Institute of Computer Networking Technology.
 * This work was partially funded by the Bundesministerium fuer Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany
 * (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
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

#include "tdtypes.h"
#include "loglevel.h"
#include "dispatcher.h"
#include "netutilities.h"

#include <string.h>
#include <math.h>
#include <netinet/in.h>
#include <ext_socket.h>


static void dispatcherDefaultLock(struct Dispatcher* dispatcher, void* userData);
static void dispatcherDefaultUnlock(struct Dispatcher* dispatcher, void* userData);


/* ###### Constructor #################################################### */
void dispatcherNew(struct Dispatcher* dispatcher,
                   void               (*lock)(struct Dispatcher* dispatcher, void* userData),
                   void               (*unlock)(struct Dispatcher* dispatcher, void* userData),
                   void*              lockUserData)
{
   simpleRedBlackTreeNew(&dispatcher->TimerStorage, NULL, timerComparison);
   simpleRedBlackTreeNew(&dispatcher->FDCallbackStorage, NULL, fdCallbackComparison);

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
   CHECK(simpleRedBlackTreeIsEmpty(&dispatcher->TimerStorage));
   CHECK(simpleRedBlackTreeIsEmpty(&dispatcher->FDCallbackStorage));
   simpleRedBlackTreeDelete(&dispatcher->TimerStorage);
   simpleRedBlackTreeDelete(&dispatcher->FDCallbackStorage);
   dispatcher->Lock         = NULL;
   dispatcher->Unlock       = NULL;
   dispatcher->LockUserData = NULL;
}


/* ###### Lock ########################################################### */
void dispatcherLock(struct Dispatcher* dispatcher)
{
   CHECK(dispatcher);
   dispatcher->Lock(dispatcher, dispatcher->LockUserData);
}


/* ###### Unlock ######################################################### */
void dispatcherUnlock(struct Dispatcher* dispatcher)
{
   CHECK(dispatcher);
   dispatcher->Unlock(dispatcher, dispatcher->LockUserData);
}


/* ###### Default locking function ####################################### */
static void dispatcherDefaultLock(struct Dispatcher* dispatcher, void* userData)
{
}


/* ###### Default unlocking function ##################################### */
static void dispatcherDefaultUnlock(struct Dispatcher* dispatcher, void* userData)
{
}


/* ###### Get poll() parameters ########################################## */
void dispatcherGetPollParameters(struct Dispatcher*  dispatcher,
                                 struct pollfd*      ufds,
                                 unsigned int*       nfds,
                                 int*                timeout,
                                 unsigned long long* pollTimeStamp)
{
   struct SimpleRedBlackTreeNode* node;
   struct FDCallback*             fdCallback;
   struct Timer*                  timer;
   long long                      timeToNextEvent;

   *nfds    = 0;
   *timeout = -1;
   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);

      /*  ====== Create fdset for poll() ================================= */
      *pollTimeStamp = getMicroTime();
      node = simpleRedBlackTreeGetFirst(&dispatcher->FDCallbackStorage);
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
         node = simpleRedBlackTreeGetNext(&dispatcher->FDCallbackStorage, node);
      }

      /*  ====== Get time to next timer event ============================ */
      node = simpleRedBlackTreeGetFirst(&dispatcher->TimerStorage);
      if(node != NULL) {
         timer = (struct Timer*)node;
         timeToNextEvent = max((long long)0, (long long)timer->TimeStamp - (long long)*pollTimeStamp);
         *timeout = (int)ceil((double)timeToNextEvent / 1000.0);
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
   struct SimpleRedBlackTreeNode* node;

   cmpNode.FD = fd;
   node = simpleRedBlackTreeFind(&dispatcher->FDCallbackStorage, &cmpNode.Node);
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
   unsigned long long             now;
   struct SimpleRedBlackTreeNode* node;
   struct Timer*                  timer;
   struct FDCallback*             fdCallback;
   unsigned int                   i;

   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);
      dispatcher->AddRemove = false;

      /* ====== Handle events ============================================ */
      /* We handle the FD callbacks first, because their corresponding FD's
         state has been returned by ext_poll(), since a timer callback
         might modify them (e.g. writing to a socket and reading the
         complete results).
      */
      if(result > 0) {
         LOG_VERBOSE4
         fputs("Handling FD events...\n", stdlog);
         LOG_END
         for(i = 0;i < nfds;i++) {
            if(ufds[i].revents) {
               fdCallback = dispatcherFindFDCallbackForDescriptor(dispatcher, ufds[i].fd);
               if(fdCallback != NULL) {
                  if(fdCallback->SelectTimeStamp <= pollTimeStamp) {
                     if(ufds[i].revents & fdCallback->EventMask) {
                        LOG_VERBOSE4
                        fprintf(stdlog,"Event $%04x (mask $%04x) for socket %d\n",
                              ufds[i].revents, fdCallback->EventMask, fdCallback->FD);
                        LOG_END

                        if(fdCallback->Callback != NULL) {
                           LOG_VERBOSE4
                           fprintf(stdlog,"Executing callback for event $%04x of socket %d\n",
                                 ufds[i].revents, fdCallback->FD);
                           LOG_END
                           dispatcherUnlock(dispatcher);
                           fdCallback->Callback(dispatcher,
                                                fdCallback->FD, ufds[i].revents,
                                                fdCallback->UserData);
                           dispatcherLock(dispatcher);
                           if(dispatcher->AddRemove == true) {
                              break;
                           }
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

      /* ====== Handle timer events ====================================== */
      /* Timers must be handled after the FD callbacks, since
         they might modify the FDs' states (e.g. completely
         reading their buffers, establishing new associations, ...)! */
      LOG_VERBOSE4
      fputs("Handling timer events...\n", stdlog);
      LOG_END
      now  = getMicroTime();
      node = simpleRedBlackTreeGetFirst(&dispatcher->TimerStorage);
      while(node != NULL) {
         timer = (struct Timer*)node;

         if(dispatcher->AddRemove == true) {
            break;
         }
         if(now >= timer->TimeStamp) {
            timer->TimeStamp = 0;
            simpleRedBlackTreeRemove(&dispatcher->TimerStorage,
                                     &timer->Node);
            if(timer->Callback != NULL) {
               dispatcherUnlock(dispatcher);
               timer->Callback(dispatcher, timer, timer->UserData);
               dispatcherLock(dispatcher);
            }
         }
         else {
            break;
         }
         node = simpleRedBlackTreeGetFirst(&dispatcher->TimerStorage);
      }

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
      dispatcherHandlePollResult(dispatcher, result,
                                 (struct pollfd*)&ufds, nfds, timeout,
                                 pollTimeStamp);
   }
}
