/*
 *  $Id: threadsafety.h,v 1.2 2004/08/26 09:12:16 dreibh Exp $
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

#ifndef THREADSAFETY_H
#define THREADSAFETY_H

#include "tdtypes.h"


#define USE_PTHREADS
#define HAS_PTHREADS_RECURSIVE_MUTEX


#ifdef USE_PTHREADS
#include <pthread.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct ThreadSafety
{
#ifdef USE_PTHREADS
   pthread_mutex_t Mutex;
#ifndef HAS_PTHREADS_RECURSIVE_MUTEX
   pthread_t       MutexOwner;
   unsigned int    MutexRecursionLevel;
#endif
   char            Name[32];
#endif
};


void threadSafetyInit(struct ThreadSafety* threadSafety,
                      const char*          name);

void threadSafetyDestroy(struct ThreadSafety* threadSafety);

void threadSafetyLock(struct ThreadSafety* threadSafety);

void threadSafetyUnlock(struct ThreadSafety* threadSafety);

bool threadSafetyIsAvailable();


#ifdef __cplusplus
}
#endif

#endif
