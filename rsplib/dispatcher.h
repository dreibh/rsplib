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

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "tdtypes.h"
#include "timer.h"
#include "fdcallback.h"
#include "leaflinkedredblacktree.h"

#include <sys/poll.h>


#ifdef __cplusplus
extern "C" {
#endif


struct Dispatcher
{
   struct LeafLinkedRedBlackTree TimerStorage;
   struct LeafLinkedRedBlackTree FDCallbackStorage;
   bool                          AddRemove;

   void                          (*Lock)(struct Dispatcher* dispatcher, void* userData);
   void                          (*Unlock)(struct Dispatcher* dispatcher, void* userData);
   void*                         LockUserData;
};


/**
  * Constructor.
  *
  * @param lock Lock function (default: dispatcherDefaultLock).
  * @param unlock Unlock function (default: dispatcherDefaultUnlock).
  * @param lockUserData User data for lock and unlock function.
  *
  * @see dispatcherDefaultLock
  * @see dispatcherDefaultUnlock
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
  * Default dispatcher lock function. This is a dummy function and does nothing!
  */
void dispatcherDefaultLock(struct Dispatcher* dispatcher, void* userData);

/**
  * Default dispatcher unlock function. This is a dummy function and does nothing!
  */
void dispatcherDefaultUnlock(struct Dispatcher* dispatcher, void* userData);

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
  * Handle results of poll() call.
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



/* ??????????????????????????????????????????????????????????????????????? */
void dispatcherGetSelectParameters(struct Dispatcher*  dispatcher,
                                   int*                n,
                                   fd_set*             readfdset,
                                   fd_set*             writefdset,
                                   fd_set*             exceptfdset,
                                   fd_set*             testfdset,
                                   unsigned long long* testTS,
                                   struct timeval*     timeout);
void dispatcherHandleSelectResult(struct Dispatcher*       dispatcher,
                                  int                      result,
                                  fd_set*                  readfdset,
                                  fd_set*                  writefdset,
                                  fd_set*                  exceptfdset,
                                  fd_set*                  testfdset,
                                  const unsigned long long testTS);
/* ??????????????????????????????????????????????????????????????????????? */


#ifdef __cplusplus
}
#endif

#endif
