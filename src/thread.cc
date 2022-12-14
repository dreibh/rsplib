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
 * Contact: dreibh@iem.uni-due.de
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "thread.h"


// ###### Constructor #######################################################
TDThread::TDThread()
{
   MyThread = 0;
   lock();
   Stopping = false;
   unlock();
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
      lock();
      Stopping = false;
      unlock();
      if(pthread_create(&MyThread, NULL, startRoutine, (void*)this) == 0) {
         return(true);
      }
      MyThread = 0;
      fputs("ERROR: Unable to start new thread!\n", stderr);
   }
   else {
      fputs("ERROR: Thread already running!\n", stderr);
   }
   return(false);
}


// ###### Stop thread #######################################################
void TDThread::stop()
{
   lock();
   Stopping = true;
   unlock();
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
