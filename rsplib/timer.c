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

#include "tdtypes.h"
#include "loglevel.h"
#include "timer.h"


/* ###### Constructor #################################################### */
void timerNew(struct Timer*      timer,
              struct Dispatcher* dispatcher,
              void               (*callback)(struct Dispatcher* dispatcher,
                                             struct Timer*      timer,
                                             void*              userData),
              void*              userData)
{
   simpleRedBlackTreeNodeNew(&timer->Node);
   timer->Master    = dispatcher;
   timer->TimeStamp = 0;
   timer->Callback  = callback;
   timer->UserData  = userData;
}


/* ###### Destructor ##################################################### */
void timerDelete(struct Timer* timer)
{
   timerStop(timer);
   simpleRedBlackTreeNodeDelete(&timer->Node);
   timer->Master    = NULL;
   timer->TimeStamp = 0;
   timer->Callback  = NULL;
   timer->UserData  = NULL;
}


/* ###### Start timer #################################################### */
void timerStart(struct Timer*            timer,
                const unsigned long long timeStamp)
{
   struct SimpleRedBlackTreeNode* result;

   CHECK(!simpleRedBlackTreeNodeIsLinked(&timer->Node));
   timer->TimeStamp = timeStamp;

   dispatcherLock(timer->Master);
   result = simpleRedBlackTreeInsert(&timer->Master->TimerStorage,
                                     &timer->Node);
   CHECK(result == &timer->Node);
   timer->Master->AddRemove = true;
   dispatcherUnlock(timer->Master);
}


/* ###### Restart timer ################################################## */
void timerRestart(struct Timer*            timer,
                  const unsigned long long timeStamp)
{
   timerStop(timer);
   timerStart(timer, timeStamp);
}


/* ###### Check, if timer is running ##################################### */
bool timerIsRunning(struct Timer* timer)
{
   return(simpleRedBlackTreeNodeIsLinked(&timer->Node));
}


/* ###### Stop timer ##################################################### */
void timerStop(struct Timer* timer)
{
   struct SimpleRedBlackTreeNode* result;
   dispatcherLock(timer->Master);
   if(simpleRedBlackTreeNodeIsLinked(&timer->Node)) {
      result = simpleRedBlackTreeRemove(&timer->Master->TimerStorage,
                                        &timer->Node);
      CHECK(result == &timer->Node);
      timer->TimeStamp = 0;
      timer->Master->AddRemove = true;
   }
   dispatcherUnlock(timer->Master);
}


/* ###### Timer comparision function ##################################### */
int timerComparison(const void* timerPtr1, const void* timerPtr2)
{
   const struct Timer* timer1 = (const struct Timer*)timerPtr1;
   const struct Timer* timer2 = (const struct Timer*)timerPtr2;
   if(timer1->TimeStamp < timer2->TimeStamp) {
      return(-1);
   }
   else if(timer1->TimeStamp > timer2->TimeStamp) {
      return(1);
   }
   if((long)timer1 < (long)timer2) {
      return(-1);
   }
   if((long)timer1 > (long)timer2) {
      return(1);
   }
   return(0);
}
