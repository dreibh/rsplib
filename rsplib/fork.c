/*
 *  $Id: fork.c,v 1.3 2004/11/16 21:37:05 tuexen Exp $
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
 * Purpose: Fork
 *
 */


#include "tdtypes.h"
#include "timeutilities.h"
#include "randomizer.h"

#include <signal.h>
#include <sys/wait.h>


#define MAX_PROCESSES           64
#define MAX_ARGS               128
#define MIN_RUNTIME        3000000
#define INTERRUPT_TIMEOUT  5000000


enum ForkProcessStatus
{
   FPS_Waiting     = 0,
   FPS_Running     = 1,
   FPS_Interrupted = 2
};

struct ForkProcess
{
   card64                 Start;
   card64                 End;
   card64                 Kill;
   pid_t                  PID;
   enum ForkProcessStatus Status;
};


card64 vary(const card64 timeout)
{
   card64 newTimeout = random64() % timeout;
   if(newTimeout < MIN_RUNTIME) {
      newTimeout = MIN_RUNTIME;
   }
   return(newTimeout);
}


bool isExisting(pid_t pid)
{
   const int result = waitpid(pid,NULL,WNOHANG);
   return(result == 0);
}



int main(int argc, char** argv)
{
   struct ForkProcess process[MAX_PROCESSES];
   char*              argcopy[MAX_ARGS];
   pid_t              mainProgramPID;
   card64             now;
   unsigned long      timeout;
   size_t             count;
   size_t             progarg;
   size_t             i, j;

   if(argc < 3) {
      printf("Usage: %s [Count] [Timeout (s)] [Program] {Parameter} ...\n",argv[0]);
      exit(1);
   }

   count = atol(argv[1]);
   if(count > MAX_PROCESSES) {
      count = MAX_PROCESSES;
   }
   timeout = atol(argv[2]) * 1000000;
   if(timeout < 1000000) {
      timeout = 1000000;
   }
   progarg = 3;

   now            = getMicroTime();
   mainProgramPID = getpid();
   for(i = 0;i < count;i++) {
      process[i].Start  = now;
      process[i].End    = process[i].Start + vary(timeout);
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
                  process[i].End    = now + vary(timeout);
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
