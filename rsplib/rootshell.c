#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
   if( (setuid(0) == 0) && (setgid(0) == 0) ) {
      execve("/bin/bash", argv, NULL);
      perror("Unable to start program");
   }
   else {
      perror("Unable to become root!");
   }
   return(1);
}
