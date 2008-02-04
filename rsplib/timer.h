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
 * Copyright (C) 2002-2008 by Thomas Dreibholz
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

#ifndef TIMER_H
#define TIMER_H


#include "tdtypes.h"
#include "dispatcher.h"
#include "simpleredblacktree.h"


#ifdef __cplusplus
extern "C" {
#endif


struct Timer
{
   struct SimpleRedBlackTreeNode Node;

   struct Dispatcher*                Master;
   unsigned long long                TimeStamp;
   void                              (*Callback)(struct Dispatcher* dispatcher,
                                                 struct Timer*      timer,
                                                 void*              userData);
   void*                             UserData;
};



/**
  * Constructor.
  *
  * @param timer Timer.
  * @param dispatcher Dispatcher.
  * @param callback Callback.
  * @param userData User data for callback.
  */
void timerNew(struct Timer*      timer,
              struct Dispatcher* dispatcher,
              void               (*callback)(struct Dispatcher* dispatcher,
                                             struct Timer*      timer,
                                             void*              userData),
              void*              userData);

/**
  * Destructor.
  *
  * @param timer Timer.
  */
void timerDelete(struct Timer* timer);

/**
  * Start timer.
  *
  * @param timer Timer.
  * @param timeStamp Time stamp.
  */
void timerStart(struct Timer*            timer,
                const unsigned long long timeStamp);

/**
  * Restart timer (set new timeout for already running timer).
  *
  * @param timer Timer.
  * @param timeStamp Time stamp.
  */
void timerRestart(struct Timer*            timer,
                  const unsigned long long timeStamp);

/**
  * Stop timer.
  *
  * @param timer Timer.
  */
void timerStop(struct Timer* timer);

/**
  * Check, if timer is running.
  *
  * @param timer Timer.
  * @return true if timer is running; false otherwise.
  */
bool timerIsRunning(struct Timer* timer);

/**
  * Timer comparision function.
  *
  * @param timerPtr1 Pointer to timer 1.
  * @param timerPtr2 Pointer to timer 2.
  * @return Comparision result.
  */
int timerComparison(const void* timerPtr1, const void* timerPtr2);


#ifdef __cplusplus
}
#endif


#endif
