/*
 *  $Id: timer.c,v 1.6 2004/08/26 09:12:16 dreibh Exp $
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
 * Purpose: Timer
 *
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
   leafLinkedRedBlackTreeNodeNew(&timer->Node);
   timer->Master    = dispatcher;
   timer->TimeStamp = 0;
   timer->Callback  = callback;
   timer->UserData  = userData;
}


/* ###### Destructor ##################################################### */
void timerDelete(struct Timer* timer)
{
   timerStop(timer);
   leafLinkedRedBlackTreeNodeDelete(&timer->Node);
   timer->Master    = NULL;
   timer->TimeStamp = 0;
   timer->Callback  = NULL;
   timer->UserData  = NULL;
}


/* ###### Start timer #################################################### */
void timerStart(struct Timer*            timer,
                const unsigned long long timeStamp)
{
   struct LeafLinkedRedBlackTreeNode* result;

   CHECK(!leafLinkedRedBlackTreeNodeIsLinked(&timer->Node));
   timer->TimeStamp = timeStamp;

   dispatcherLock(timer->Master);
   result = leafLinkedRedBlackTreeInsert(&timer->Master->TimerStorage,
                                         &timer->Node);
   CHECK(result == &timer->Node);
   dispatcherUnlock(timer->Master);
}


/* ###### Restart timer ################################################## */
void timerRestart(struct Timer*            timer,
                  const unsigned long long timeStamp)
{
   timerStop(timer);
   timerStart(timer, timeStamp);
}


/* ###### Stop timer ##################################################### */
void timerStop(struct Timer* timer)
{
   struct LeafLinkedRedBlackTreeNode* result;
   if(leafLinkedRedBlackTreeNodeIsLinked(&timer->Node)) {
      dispatcherLock(timer->Master);
      result = leafLinkedRedBlackTreeRemove(&timer->Master->TimerStorage,
                                            &timer->Node);
      CHECK(result == &timer->Node);
      dispatcherUnlock(timer->Master);
      timer->TimeStamp = 0;
   }
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
   return(0);
}
