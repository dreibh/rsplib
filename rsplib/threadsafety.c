/*
 *  $Id: threadsafety.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Platform-Dependent Thread Safety
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "threadsafety.h"


static card64 gMutexCounter = 0;


void threadSafetyInit(struct ThreadSafety* threadSafety,
                      const char*          name)
{
#ifdef USE_PTHREADS
#ifdef HAS_PTHREADS_RECURSIVE_MUTEX
   pthread_mutexattr_t attr;
   pthread_mutexattr_init(&attr);
   pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
   pthread_mutex_init(&threadSafety->Mutex, &attr);
   pthread_mutexattr_destroy(&attr);
#else
   threadSafety->MutexOwner          = 0;
   threadSafety->MutexRecursionLevel = 0;
   pthread_mutex_init(&threadSafety->Mutex,NULL);
#endif
   snprintf((char*)&threadSafety->Name, sizeof(threadSafety->Name), "%Ld-%s",
            gMutexCounter++, name);
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
      fprintf(stdlog, "Created mutex \"%s\"\n", threadSafety->Name);
      LOG_END
   }
#endif
}


void threadSafetyDestroy(struct ThreadSafety* threadSafety)
{
#ifdef USE_PTHREADS
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
#endif
}


void threadSafetyLock(struct ThreadSafety* threadSafety)
{
#ifdef USE_PTHREADS
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
      fprintf(stdlog, "Locking mutex \"%s\"...\n", threadSafety->Name);
      LOG_END
   }
#ifdef HAS_PTHREADS_RECURSIVE_MUTEX
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
#ifdef HAS_PTHREADS_RECURSIVE_MUTEX
      fprintf(stdlog, "Locked mutex \"%s\"\n", threadSafety->Name);
#else
      fprintf(stdlog, "Locked mutex \"%s\", recursion level %d\n",
              threadSafety->Name, threadSafety->MutexRecursionLevel);
#endif
      LOG_END
   }
#endif
}


void threadSafetyUnlock(struct ThreadSafety* threadSafety)
{
#ifdef USE_PTHREADS
   if(threadSafety != &gLogMutex) {
      LOG_MUTEX
#ifdef HAS_PTHREADS_RECURSIVE_MUTEX
      fprintf(stdlog, "Unlocking mutex \"%s\"...\n", threadSafety->Name);
#else
      fprintf(stdlog, "Unlocking mutex \"%s\", recursion level %d...\n",
              threadSafety->Name, threadSafety->MutexRecursionLevel);
#endif
      LOG_END
   }
#ifdef HAS_PTHREADS_RECURSIVE_MUTEX
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
#endif
}


bool threadSafetyIsAvailable()
{
#ifdef USE_PTHREADS
   return(true);
#endif
   return(false);
}
