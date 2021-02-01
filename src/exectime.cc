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
 * Copyright (C) 2002-2021 by Thomas Dreibholz
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

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <thread.h>
#include <sys/time.h>

#include "timeutilities.h"


using namespace std;


class ExecThread : public TDThread
{
   public:
   ExecThread();
   virtual ~ExecThread();
   void initialize(const size_t id, const char* command, const char* arg);

   static int compareExecThreads(const void* et1, const void* et2);

   protected:
   virtual void run();

   public:
   size_t             ID;
   const char*        Command;
   const char*        Argument;
   char               CmdLine[1024];
   int                Result;
   int                ErrorCode;
   unsigned long long Runtime;
};


// ###### Constructor #######################################################
ExecThread::ExecThread()
{
   Command   = NULL;
   Argument  = NULL;
   Runtime   = ~0ULL;
   Result    = -1;
   ErrorCode = EINVAL;
   ID        = 0;
}


// ###### Destructor ########################################################
ExecThread::~ExecThread()
{
}


// ###### Initialize thread #################################################
void ExecThread::initialize(const size_t id, const char* command, const char* arg)
{
   Command  = command;
   Argument = arg;
   ID       = id;
   snprintf((char*)&CmdLine, sizeof(CmdLine), "%s %s", Command, Argument);
   cerr << CmdLine << endl;
}


// ###### Thread implementation #############################################
void ExecThread::run()
{
   const unsigned long long startTimeStamp = getMicroTime();
   Result    = system(CmdLine);
   ErrorCode = errno;
   const unsigned long long endTimeStamp = getMicroTime();
   if((ErrorCode == 0) && (Result == 0)) {
      Runtime = endTimeStamp - startTimeStamp;
   }
}


int ExecThread::compareExecThreads(const void* ptr1, const void* ptr2)
{
   const ExecThread* et1 = *((const ExecThread**)ptr1);
   const ExecThread* et2 = *((const ExecThread**)ptr2);

   if(et1->Runtime < et2->Runtime) {
      return(-1);
   }
   else if(et1->Runtime > et2->Runtime) {
      return(1);
   }
   return(0);
}


// ###### Main program ######################################################
int main(int argc, char** argv)
{
   if(argc < 3) {
      cerr << "Usage: " << argv[0] << " [Command] [Parameter1] {Parameter2} ..."
           << endl;
      exit(1);
   }

   cerr << "Starting ..." << endl;
   ExecThread* threadSet = new ExecThread[argc - 2];
   ExecThread* threadPtrSet[argc - 2];
   size_t      threads = 0;
   for(int i = 2;i < argc;i++) {
      cerr << "   - Thread " << threads + 1 << ": ";
      threadPtrSet[threads] = &threadSet[threads];
      threadSet[threads].initialize(threads + 1, argv[1], argv[i]);
      threadSet[threads].start();
      threads++;
   }

   cerr << endl << "Waiting for the threads to finish ..." << endl;
   for(size_t i = 0;i < threads;i++) {
      threadSet[i].waitForFinish();
   }

   cerr << endl << "Results:" << endl;
   qsort(&threadPtrSet, threads, sizeof(ExecThread*), ExecThread::compareExecThreads);
   for(size_t i = 0;i < threads;i++) {
      char str[1536];
      if(threadPtrSet[i]->Runtime != ~0ULL) {
         snprintf((char*)&str, sizeof(str), "   - %6Lums - Thread %2u: %s",
                  threadPtrSet[i]->Runtime / 1000,
                  (unsigned int)threadPtrSet[i]->ID, threadPtrSet[i]->CmdLine);
         cout << threadPtrSet[i]->Argument << endl;
      }
      else {
         snprintf((char*)&str, sizeof(str), "   - Result %d - Thread %2u: %s",
                  threadPtrSet[i]->Result,
                  (unsigned int)threadPtrSet[i]->ID, threadPtrSet[i]->CmdLine);
      }
      cerr << str << endl;
   }

   cerr << endl << "Done!" << endl;
   delete [] threadSet;
   return(0);
}
