/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include "tdtypes.h"
#include "loglevel.h"
#include "threadsafety.h"


static unsigned long long gMutexCounter = 0;


/* ###### Constructor #################################################### */
void threadSafetyNew(struct ThreadSafety* threadSafety,
                     const char*          name)
{
#ifndef __APPLE__
   pthread_mutexattr_t attr;
   pthread_mutexattr_init(&attr);
   pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
   pthread_mutex_init(&threadSafety->Mutex, &attr);
   pthread_mutexattr_destroy(&attr);
#else
   threadSafety->MutexOwner          = 0;
   threadSafety->MutexRecursionLevel = 0;
   pthread_mutex_init(&threadSafety->Mutex, NULL);
#endif
   snprintf((char*)&threadSafety->Name, sizeof(threadSafety->Name), "%llu-%s",
            gMutexCounter++, name);
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
      fprintf(stdlog, "Created mutex \"%s\"\n", threadSafety->Name);
      LOG_END
   }
}


/* ###### Destructor ##################################################### */
void threadSafetyDelete(struct ThreadSafety* threadSafety)
{
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
      fprintf(stdlog, "Destroying mutex \"%s\"...\n", threadSafety->Name);
      LOG_END
   }
   pthread_mutex_destroy(&threadSafety->Mutex);
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
      fprintf(stdlog, "Destroyed mutex \"%s\"\n", threadSafety->Name);
      LOG_END
   }
}


/* ###### Lock ########################################################### */
void threadSafetyLock(struct ThreadSafety* threadSafety)
{
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
      fprintf(stdlog, "Locking mutex \"%s\"...\n", threadSafety->Name);
      LOG_END
   }
#ifndef __APPLE__
   pthread_mutex_lock(&threadSafety->Mutex);
#else
   if(!pthread_equal(threadSafety->MutexOwner, pthread_self())) {
      pthread_mutex_lock(&threadSafety->Mutex);
      threadSafety->MutexOwner = pthread_self();
   }
   threadSafety->MutexRecursionLevel++;
#endif
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
#ifndef __APPLE__
      fprintf(stdlog, "Locked mutex \"%s\"\n", threadSafety->Name);
#else
      fprintf(stdlog, "Locked mutex \"%s\", recursion level %d\n",
              threadSafety->Name, threadSafety->MutexRecursionLevel);
#endif
      LOG_END
   }
}


/* ###### Unlock ######################################################### */
void threadSafetyUnlock(struct ThreadSafety* threadSafety)
{
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
#ifndef __APPLE__
      fprintf(stdlog, "Unlocking mutex \"%s\"...\n", threadSafety->Name);
#else
      fprintf(stdlog, "Unlocking mutex \"%s\", recursion level %d...\n",
              threadSafety->Name, threadSafety->MutexRecursionLevel);
#endif
      LOG_END
   }
#ifndef __APPLE__
   pthread_mutex_unlock(&threadSafety->Mutex);
#else
   if(threadSafety->MutexRecursionLevel == 0) {
      LOG_ERROR
      fputs("Mutex is already unlocked!\n",stdlog);
      LOG_END
      exit(1);
   }
   if(pthread_equal(threadSafety->MutexOwner, pthread_self())) {
      threadSafety->MutexRecursionLevel--;
      if(threadSafety->MutexRecursionLevel == 0) {
         threadSafety->MutexOwner = 0;
         pthread_mutex_unlock(&threadSafety->Mutex);
      }
   }
   else {
      LOG_ERROR
      fputs("Mutex is not owned!\n",stdlog);
      LOG_END
   }
#endif
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
      fprintf(stdlog, "Unlocked mutex \"%s\"\n", threadSafety->Name);
      LOG_END
   }
}
