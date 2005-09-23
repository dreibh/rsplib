/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id: cspmonitor.c 0 2005-03-02 13:34:16Z dreibh $
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

#ifndef THREADSIGNAL_H
#define THREADSIGNAL_H

#include "tdtypes.h"

#include <pthread.h>


#ifdef __cplusplus
extern "C" {
#endif


struct ThreadSignal
{
   pthread_mutex_t Mutex;       /* Non-recursive mutex! */
   pthread_cond_t  Condition;
};


/**
  * Create new mutex.
  *
  * @param threadSignal ThreadSignal.
  */
void threadSignalNew(struct ThreadSignal* threadSignal,
                     const char*          name);

/**
  * Delete mutex.
  *
  * @param threadSignal ThreadSignal.
  */
void threadSignalDelete(struct ThreadSignal* threadSignal);

/**
  * Lock mutex.
  *
  * @param threadSignal ThreadSignal.
  */
void threadSignalLock(struct ThreadSignal* threadSignal);

/**
  * Unlock mutex.
  *
  * @param threadSignal ThreadSignal.
  */
void threadSignalUnlock(struct ThreadSignal* threadSignal);

/**
  * Fire signal.
  *
  * @param threadSignal ThreadSignal.
  */
void threadSignalFire(struct ThreadSignal* threadSignal);

/**
  * Wait for signal.
  *
  * @param threadSignal ThreadSignal.
  */
void threadSignalWait(struct ThreadSignal* threadSignal);


#ifdef __cplusplus
}
#endif

#endif
