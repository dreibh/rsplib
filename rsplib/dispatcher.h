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
 * Copyright (C) 2002-2009 by Thomas Dreibholz
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

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "tdtypes.h"
#include "timer.h"
#include "fdcallback.h"
#include "simpleredblacktree.h"

#include <sys/poll.h>


#ifdef __cplusplus
extern "C" {
#endif


struct Dispatcher
{
   struct SimpleRedBlackTree TimerStorage;
   struct SimpleRedBlackTree FDCallbackStorage;
   bool                      AddRemove;

   void                      (*Lock)(struct Dispatcher* dispatcher, void* userData);
   void                      (*Unlock)(struct Dispatcher* dispatcher, void* userData);
   void*                     LockUserData;
};


/**
  * Constructor.
  *
  * @param lock Lock function (default: dummy function).
  * @param unlock Unlock function (default: dummy function).
  * @param lockUserData User data for lock and unlock function.
  */
void dispatcherNew(struct Dispatcher* dispatcher,
                   void               (*lock)(struct Dispatcher* dispatcher, void* userData),
                   void               (*unlock)(struct Dispatcher* dispatcher, void* userData),
                   void*              lockUserData);

/**
  * Destructor.
  *
  * @param dispatcher Dispatcher.
  */
void dispatcherDelete(struct Dispatcher* dispatcher);

/**
  * Lock dispatcher.
  *
  * @param dispatcher Dispatcher.
  */
void dispatcherLock(struct Dispatcher* dispatcher);

/**
  * Unlock dispatcher.
  *
  * @param dispatcher Dispatcher.
  */
void dispatcherUnlock(struct Dispatcher* dispatcher);

/**
  * Get poll() parameters for user-controlled poll() loop.
  *
  * @param dispatcher Dispatcher.
  * @param ufds pollfd array of at least FD_SETSIZE entries.
  * @param nfds Reference to store number of pollfd entries.
  * @param timeout Reference to store timeout.
  * @param pollTimeStamp Reference to store time stamp of dispatcherGetPollParameters() call.
  */
void dispatcherGetPollParameters(struct Dispatcher*  dispatcher,
                                 struct pollfd*      ufds,
                                 unsigned int*       nfds,
                                 int*                timeout,
                                 unsigned long long* pollTimeStamp);

/**
  * Handle results of poll() call. Important: nfds must be the
  * value obtained from dispatcherGetPollParameters()!
  *
  * @param dispatcher Dispatcher.
  * @param result Result value returned by poll().
  * @param ufds pollfd array.
  * @param nfds Number of pollfd entries.
  * @param timeout timeout.
  * @param pollTimeStamp Time stamp of dispatcherGetPollParameters() call.
  */
void dispatcherHandlePollResult(struct Dispatcher* dispatcher,
                                int                result,
                                struct pollfd*     ufds,
                                unsigned int       nfds,
                                int                timeout,
                                unsigned long long pollTimeStamp);

/**
  * Event loop calling dispatcherGetSelectParameters(), select() and dispatcherHandleSelectResult().
  *
  * @param dispatcher Dispatcher.
  *
  * @see dispatcherGetSelectParameters
  * @see dispatcherHandleSelectResult
  */
void dispatcherEventLoop(struct Dispatcher* dispatcher);


#ifdef __cplusplus
}
#endif

#endif
