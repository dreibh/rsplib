/*
 *  $Id: timer.h,v 1.2 2004/07/21 14:39:53 dreibh Exp $
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


#ifndef TIMER_H
#define TIMER_H


#include "tdtypes.h"
#include "dispatcher.h"


#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif


struct Timer
{
   struct Dispatcher* Master;
   card64             Time;

   void               (*Callback)(struct Dispatcher* dispatcher,
                                  struct Timer*      timer,
                                  void*              userData);
   void*              UserData;
};



/**
  * Constructor.
  *
  * @param dispatcher Dispatcher.
  * @param callback Callback.
  * @param userData User data for callback.
  */
struct Timer* timerNew(struct Dispatcher* dispatcher,
                       void               (*callback)(struct Dispatcher* dispatcher,
                                                      struct Timer*      timer,
                                                      void*              userData),
                       void*               userData);

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
void timerStart(struct Timer* timer,
                const card64  timeStamp);

/**
  * Restart timer (set new timeout for already running timer).
  *
  * @param timer Timer.
  * @param timeStamp Time stamp.
  */
void timerRestart(struct Timer* timer,
                  const card64  timeStamp);

/**
  * Stop timer.
  *
  * @param timer Timer.
  */
void timerStop(struct Timer* timer);

/**
  * Timer comparision function.
  *
  * @param a Pointer to timer 1.
  * @param a Pointer to timer 2.
  * @return Comparision result.
  */
gint timerCompareFunc(gconstpointer a,
                      gconstpointer b);



#ifdef __cplusplus
}
#endif


#endif
