/*
 * thread.cc: PTDThread class
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

#include <unistd.h>
#include <pthread.h>
#include <iostream>

#include "thread.h"


using namespace std;


TDThread::TDThread()
{
   MyThread = 0;
}

TDThread::~TDThread()
{
   if(MyThread != 0) {
      waitForFinish();
   }
}

void* TDThread::startRoutine(void* object)
{
   TDThread* thread = (TDThread*)object;
   thread->run();
   thread->MyThread = 0;
   return(NULL);
}

void TDThread::start()
{
   if(MyThread == 0) {
      if(pthread_create(&MyThread, NULL, startRoutine, (void*)this) != 0) {
         cerr << "ERROR: Unable to start new thread!" << endl;
         exit(1);
      }
   }
   else {
      cerr << "ERROR: TDThread already running!" << endl;
   }
}

void TDThread::waitForFinish()
{
   if(MyThread != 0) {
      pthread_join(MyThread, NULL);
   }
}

void TDThread::delay(const unsigned int us)
{
   usleep(us);
}
