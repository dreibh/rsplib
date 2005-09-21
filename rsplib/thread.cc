/*
 * thread.cc: PTDThread class
 * Copyright (C) 2003-2005 by Thomas Dreibholz
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

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "thread.h"


// ###### Constructor #######################################################
TDThread::TDThread()
{
   MyThread = 0;
}


// ###### Destructor ########################################################
TDThread::~TDThread()
{
   if(MyThread != 0) {
      waitForFinish();
   }
}


// ###### Start routine to lauch thread's run() function ####################
void* TDThread::startRoutine(void* object)
{
   TDThread* thread = (TDThread*)object;
   thread->run();
   return(NULL);
}


// ###### Start thread ######################################################
bool TDThread::start()
{
   if(MyThread == 0) {
      if(pthread_create(&MyThread, NULL, startRoutine, (void*)this) == 0) {
         return(true);
      }
      MyThread = 0;
      fputs("ERROR: Unable to start new thread!\n", stderr);
   }
   else {
      fputs("ERROR: TDThread already running!\n", stderr);
   }
   return(false);
}


// ###### Wait until thread has been finished ###############################
void TDThread::waitForFinish()
{
   if(MyThread != 0) {
      pthread_join(MyThread, NULL);
      MyThread = 0;
   }
}


// ###### Wait a given amount of microseconds ###############################
void TDThread::delay(const unsigned int us)
{
   usleep(us);
}
