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

#include "tdtypes.h"
#include "loglevel.h"
#include "threadsafety.h"

#include <unistd.h>
#include <assert.h>



/* ###### Constructor #################################################### */
void threadSafetyNew(struct ThreadSafety* threadSafety,
                     const char*          name)
{
#if !defined(__APPLE__)
   pthread_mutexattr_t attr;
   pthread_mutexattr_init(&attr);
   pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(&threadSafety->Mutex, &attr);
   pthread_mutexattr_destroy(&attr);
#else
   threadSafety->MutexOwner          = 0;
   threadSafety->MutexRecursionLevel = 0;
   pthread_mutex_init(&threadSafety->Mutex, NULL);
#endif
}


/* ###### Destructor ##################################################### */
void threadSafetyDelete(struct ThreadSafety* threadSafety)
{
   pthread_mutex_destroy(&threadSafety->Mutex);
}


/* ###### Lock ########################################################### */
void threadSafetyLock(struct ThreadSafety* threadSafety)
{
#ifndef __APPLE__
   pthread_mutex_lock(&threadSafety->Mutex);
#else
   if(!pthread_equal(threadSafety->MutexOwner, pthread_self())) {
      pthread_mutex_lock(&threadSafety->Mutex);
      threadSafety->MutexOwner = pthread_self();
   }
   threadSafety->MutexRecursionLevel++;
#endif
}


/* ###### Unlock ######################################################### */
void threadSafetyUnlock(struct ThreadSafety* threadSafety)
{
#ifndef __APPLE__
   pthread_mutex_unlock(&threadSafety->Mutex);
#else
   assert(threadSafety->MutexRecursionLevel > 0);
   assert(pthread_equal(threadSafety->MutexOwner, pthread_self()));
   threadSafety->MutexRecursionLevel--;
   if(threadSafety->MutexRecursionLevel == 0) {
      threadSafety->MutexOwner = 0;
      pthread_mutex_unlock(&threadSafety->Mutex);
   }
#endif
}
