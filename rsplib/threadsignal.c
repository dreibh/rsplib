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
#include "threadsignal.h"


/* ###### Constructor #################################################### */
void threadSignalNew(struct ThreadSignal* threadSignal)
{
   /* Important: The mutex is non-recursive! */
   pthread_mutex_init(&threadSignal->Mutex, NULL);
   pthread_cond_init(&threadSignal->Condition, NULL);
};


/* ###### Destructor ##################################################### */
void threadSignalDelete(struct ThreadSignal* threadSignal)
{
   pthread_cond_destroy(&threadSignal->Condition);
   pthread_mutex_destroy(&threadSignal->Mutex);
};


/* ###### Lock ########################################################### */
void threadSignalLock(struct ThreadSignal* threadSignal)
{
   pthread_mutex_lock(&threadSignal->Mutex);
}


/* ###### Unlock ######################################################### */
void threadSignalUnlock(struct ThreadSignal* threadSignal)
{
   pthread_mutex_unlock(&threadSignal->Mutex);
}


/* ###### Send signal #################################################### */
void threadSignalFire(struct ThreadSignal* threadSignal)
{
   pthread_cond_signal(&threadSignal->Condition);
}


/* ###### Wait for signal ################################################ */
void threadSignalWait(struct ThreadSignal* threadSignal)
{
   pthread_cond_wait(&threadSignal->Condition, &threadSignal->Mutex);
}
