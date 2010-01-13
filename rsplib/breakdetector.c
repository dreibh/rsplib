/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2004-2010 by Thomas Dreibholz
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

#include "tdtypes.h"
#include "breakdetector.h"
#include "timeutilities.h"

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>



/* Kill after timeout: Send kill signal, if Ctrl-C is pressed again
   after more than KILL_TIMEOUT microseconds */
#define KILL_AFTER_TIMEOUT
#define KILL_TIMEOUT 2000000



/* ###### Global variables ############################################### */
static bool   DetectedBreak = false;
static bool   PrintedBreak  = false;
static int    BreakSignal   = 0;
static bool   Quiet         = false;
static pid_t  MainThreadPID = 0;
#ifdef KILL_AFTER_TIMEOUT
static bool               PrintedKill   = false;
static unsigned long long LastDetection = (unsigned long long)-1;
#endif


/* ###### Handler for SIGINT ############################################# */
void breakDetector(int signum)
{
   BreakSignal   = signum;
   DetectedBreak = true;

#ifdef KILL_AFTER_TIMEOUT
   if(!PrintedKill) {
      const unsigned long long now = getMicroTime();
      if(LastDetection == (unsigned long long)-1) {
         LastDetection = now;
      }
      else if(now - LastDetection >= 2000000) {
         PrintedKill = true;
         fprintf(stderr, "\x1b[0m\n*** Kill ***\n\n");
         kill(MainThreadPID, SIGKILL);
      }
   }
#endif
}


/* ###### Install break detector ######################################### */
void installBreakDetector()
{
   BreakSignal   = 0;
   DetectedBreak = false;
   PrintedBreak  = false;
   Quiet         = false;
   MainThreadPID = getpid();
#ifdef KILL_AFTER_TIMEOUT
   PrintedKill   = false;
   LastDetection = (unsigned long long)-1;
#endif
   signal(SIGINT,  &breakDetector);
   signal(SIGTERM, &breakDetector);
}


/* ###### Unnstall break detector ######################################## */
void uninstallBreakDetector()
{
   signal(SIGINT,  SIG_DFL);
   signal(SIGTERM, SIG_DFL);
#ifdef KILL_AFTER_TIMEOUT
   PrintedKill   = false;
   LastDetection = (unsigned long long)-1;
#endif
   /* No reset here!
      DetectedBreak = false;
      PrintedBreak  = false; */
   Quiet         = false;
}


/* ###### Check, if break has been detected ############################## */
bool breakDetected()
{
   if((DetectedBreak) && (!PrintedBreak)) {
      if(!Quiet) {
         fprintf(stderr, "\x1b[0m\n*** Break ***    Signal #%d\n\n",  BreakSignal);
      }
      PrintedBreak = getMicroTime();
   }
   return(DetectedBreak);
}


/* ###### Send break to main thread ###################################### */
void sendBreak(const bool quiet)
{
   Quiet = quiet;
   kill(MainThreadPID, SIGINT);
}
