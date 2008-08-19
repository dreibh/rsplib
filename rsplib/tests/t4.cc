#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char** argv)
{
   pid_t childProcess;


   childProcess = fork();
   if(childProcess == 0) {
      execlp("./t4.sh", "t4.sh", "Arg1", "Arg2", NULL);
      perror("Failed to start script");
      return 1;
   }
   else {
      printf("Started child process, PID is %u\n", childProcess);
      for(;;) {
         int status;
         pid_t childFinished = waitpid(childProcess, &status, WNOHANG);
         if(childFinished == childProcess) {
            if(WIFEXITED(status) || WIFSIGNALED(status)) {
               puts("Child process has finished.");
               break;
            }
         }
         puts("Waiting for end of child process. Also finishing now ...");
         usleep(1000000);
      }
   }
   return 0;
}
