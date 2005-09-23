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
