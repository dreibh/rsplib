/*
 * mutex.h: PThread mutex class
 * Copyright (C) 2003 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>


class TDMutex
{
   public:
   TDMutex();
   virtual ~TDMutex();

   inline void lock() {
      pthread_mutex_lock(&MyTDMutex);
   }
   inline void unlock() {
      pthread_mutex_unlock(&MyTDMutex);
   }

   private:
   pthread_mutex_t MyTDMutex;
};


#endif
