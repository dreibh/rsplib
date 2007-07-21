/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
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
#include "timeutilities.h"
#include "randomizer.h"

#include <signal.h>
#include <sys/wait.h>


#define MAX_PROCESSES           64
#define MAX_ARGS               128
#define MIN_RUNTIME         500000
#define INTERRUPT_TIMEOUT  5000000


enum ForkProcessStatus
{
   FPS_Waiting     = 0,
   FPS_Running     = 1,
   FPS_Interrupted = 2
};

struct ForkProcess
{
   unsigned long long                 Start;
   unsigned long long                 End;
   unsigned long long                 Kill;
   pid_t                  PID;
   enum ForkProcessStatus Status;
};


#define DIST_UNIFORM 0
#define DIST_NEGEXP  1


/* ###### Get new random interval ######################################## */
unsigned long long getRandomInterval(const unsigned int       distribution,
                                     const unsigned long long timeout)
{
   unsigned long long newTimeout = 0;

   switch(distribution) {
      case DIST_UNIFORM:
         newTimeout = random64() % timeout;
       break;
      case DIST_NEGEXP:
         newTimeout = (unsigned long long)rint(randomExpDouble((double)timeout));
printf("%llu\n",newTimeout);
       break;
    }

   if(newTimeout < MIN_RUNTIME) {
      newTimeout = MIN_RUNTIME;
   }
   return(newTimeout);
}


/* ###### Check whether process has terminated ########################### */
bool isExisting(pid_t pid)
{
   const int result = waitpid(pid, NULL, WNOHANG);
   return(result == 0);
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   struct ForkProcess process[MAX_PROCESSES];
   char*              argcopy[MAX_ARGS];
   pid_t              mainProgramPID;
   unsigned long long now;
   unsigned long long timeout;
   unsigned int       distribution = DIST_UNIFORM;
   size_t             count;
   size_t             progarg;
   size_t             i, j;

   if(argc < 3) {
      printf("Usage: %s [Count] [{exp|uniform}:Timeout(Microseconds)] [Program] {Parameter} ...\n",argv[0]);
      exit(1);
   }

   count = atol(argv[1]);
   if(count > MAX_PROCESSES) {
      count = MAX_PROCESSES;
   }
   if(sscanf(argv[2], "uniform:%llu", &timeout) == 1) {
      distribution = DIST_UNIFORM;
   }
   else if(sscanf(argv[2], "exp:%llu", &timeout) == 1) {
      distribution = DIST_NEGEXP;
   }
   else {
      fputs("ERROR: Invalid timeout specified!\n", stderr);
      exit(1);
   }
   progarg = 3;

   now            = getMicroTime();
   mainProgramPID = getpid();
   for(i = 0;i < count;i++) {
      process[i].Start  = now;
      process[i].End    = process[i].Start + getRandomInterval(distribution,timeout);
      process[i].Kill   = process[i].End + INTERRUPT_TIMEOUT;
      process[i].PID    = 0;
      process[i].Status = FPS_Waiting;
   }

   for(;;) {
      for(i = 0;i < count;i++) {
         now = getMicroTime();
         switch(process[i].Status) {
            case FPS_Waiting:
               if(process[i].Start <= now) {
                  process[i].Status = FPS_Running;
                  process[i].PID = fork();
                  if(process[i].PID == 0) {
                     for(j = progarg;j < (size_t)min(MAX_ARGS,argc);j++) {
                        argcopy[j - progarg] = argv[j];
                     }
                     argcopy[j - progarg] = NULL;

                     printf("Executing");
                     j = 0;
                     while(argcopy[j] != NULL) {
                        printf(" %s",argcopy[j]);
                        j++;
                     }
                     puts("...");

                     execvp(argv[progarg],argcopy);
                     perror("Unable to start program -> exiting! Reason");
                     kill(mainProgramPID,SIGINT);
                     exit(1);
                  }
                  else {
                     printf("Started process #%d.\n", (int)process[i].PID);
                  }
               }
             break;
            case FPS_Running:
               if(isExisting(process[i].PID)) {
                  if(process[i].End <= now) {
                     printf("Interrupting process #%d.\n", (int)process[i].PID);
                     kill(process[i].PID,SIGINT);
                     process[i].Status = FPS_Interrupted;
                  }
               }
               else {
                  printf("Process #%d is no longer existing.\n", (int)process[i].PID);
                  process[i].Status = FPS_Interrupted;
               }
             break;
            case FPS_Interrupted:
               if(isExisting(process[i].PID)) {
                  if(process[i].Kill <= now) {
                     printf("Killing process #%d.\n", (int)process[i].PID);
                     kill(process[i].PID,SIGKILL);
                  }
               }
               else {
                  printf("Process #%d has terminated.\n", (int)process[i].PID);
                  process[i].Start  = now;
                  process[i].End    = now + getRandomInterval(distribution,timeout);
                  process[i].Kill   = process[i].End + INTERRUPT_TIMEOUT;
                  process[i].PID    = 0;
                  process[i].Status = FPS_Waiting;
               }
             break;
         }
      }
      usleep(100000);
   }
}
