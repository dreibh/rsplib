#include "dispatcher.h"
#include "fdcallback.h"
#include "timer.h"
#include "timeutilities.h"
#include "loglevel.h"


void fdCallback(struct Dispatcher* dispatcher,
                int                fd,
                unsigned int       eventMask,
                void*              userData)
{
   printf("File: fd=%d mask=$%04x\n", fd, eventMask);

   char buffer[1024];
   printf("read from stdin: %d\n", read(fd, (char*)&buffer, sizeof(buffer)));
}


void timerCallback(struct Dispatcher* dispatcher,
                   struct Timer*      timer,
                   void*              userData)
{
   printf("Timer: ud=%d\n", userData);
   timerRestart(timer, getMicroTime() + (1000000 * (unsigned int)userData));
}


int main(int argc, char** argv)
{
   Dispatcher d;
   Timer t1;
   Timer t2;
   Timer t3;
   FDCallback fdc1;

   initLogging("-loglevel=9");

   dispatcherNew(&d,NULL,NULL,NULL);
   fdCallbackNew(&fdc1, &d, STDIN_FILENO, FDCE_Read, fdCallback, (void*)1);
   timerNew(&t1, &d, timerCallback, (void*)1);
   timerNew(&t2, &d, timerCallback, (void*)2);
   timerNew(&t3, &d, timerCallback, (void*)3);
   timerStart(&t1, getMicroTime() + 1000000);
   timerStart(&t2, getMicroTime() + 2000000);
   timerStart(&t3, getMicroTime() + 3000000);

   for(;;) {
      puts(".");
      dispatcherEventLoop(&d);
   }

   dispatcherDelete(&d);

   return(0);
}
