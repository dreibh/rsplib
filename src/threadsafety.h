/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2023 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#ifndef THREADSAFETY_H
#define THREADSAFETY_H

#include "tdtypes.h"

#include <pthread.h>


#ifdef __cplusplus
extern "C" {
#endif


struct ThreadSafety
{
   pthread_mutex_t Mutex;
#if defined(__APPLE__)
   pthread_t       MutexOwner;
   unsigned int    MutexRecursionLevel;
#endif
};


/**
  * Create new recursive mutex.
  *
  * @param threadSafety ThreadSafety.
  * @param name Mutex name for debugging purposes or NULL.
  */
void threadSafetyNew(struct ThreadSafety* threadSafety,
                     const char*          name);

/**
  * Delete recursive mutex.
  *
  * @param threadSafety ThreadSafety.
  */
void threadSafetyDelete(struct ThreadSafety* threadSafety);

/**
  * Lock mutex.
  *
  * @param threadSafety ThreadSafety.
  */
void threadSafetyLock(struct ThreadSafety* threadSafety);

/**
  * Unlock mutex.
  *
  * @param threadSafety ThreadSafety.
  */
void threadSafetyUnlock(struct ThreadSafety* threadSafety);


#ifdef __cplusplus
}
#endif

#endif
