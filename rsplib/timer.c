/*
 *  $Id: timer.c,v 1.3 2004/07/21 14:39:53 dreibh Exp $
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
struct Timer* timerNew(struct Dispatcher* dispatcher,
                       void               (*callback)(struct Dispatcher* dispatcher,
                                                      struct Timer*      timer,
                                                      void*              userData),
                       void*               userData)
{
   struct Timer* timer = NULL;
   if(dispatcher != NULL) {
      timer = (struct Timer*)malloc(sizeof(struct Timer));
      if(timer != NULL) {
         timer->Master   = dispatcher;
         timer->Time     = 0;
         timer->Callback = callback;
         timer->UserData = userData;
      }
   }
   return(timer);
}


/* ###### Destructor ##################################################### */
void timerDelete(struct Timer* timer)
{
   if(timer != NULL) {
      timerStop(timer);
      free(timer);
   }
}


/* ###### Start timer #################################################### */
void timerStart(struct Timer* timer,
                const card64  timeStamp)
{
   if(timer != NULL) {
      dispatcherLock(timer->Master);
      if(timer->Time == 0) {
         timer->Time = timeStamp;
         timer->Master->TimerList = g_list_insert_sorted(timer->Master->TimerList,
                                                         (gpointer)timer,
                                                         timerCompareFunc);
      }
      else {
         LOG_ERROR
         fputs("Timer already started!\n",stdlog);
         LOG_END
      }
      dispatcherUnlock(timer->Master);
   }
}


/* ###### Restart timer ################################################## */
void timerRestart(struct Timer* timer,
                  const card64  timeStamp)
{
   timerStop(timer);
   timerStart(timer, timeStamp);
}


/* ###### Stop timer ##################################################### */
void timerStop(struct Timer* timer)
{
   if(timer != NULL) {
      dispatcherLock(timer->Master);
      timer->Time = 0;
      timer->Master->TimerList = g_list_remove(timer->Master->TimerList, (gpointer)timer);
      dispatcherUnlock(timer->Master);
   }
}


/* ###### Timer comparision function ##################################### */
gint timerCompareFunc(gconstpointer a,
                      gconstpointer b)
{
   const struct Timer* t1 = (const struct Timer*)a;
   const struct Timer* t2 = (const struct Timer*)b;
   if(t1->Time < t2->Time) {
      return(-1);
   }
   else if(t1->Time > t2->Time) {
      return(1);
   }
   return(0);
}
