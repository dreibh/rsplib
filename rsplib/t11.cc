#include "tdtypes.h"
#include "loglevel.h"
#include "rserpoolmessage.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "rserpool.h"
#include "threadsafety.h"
#include "breakdetector.h"

#include <ext_socket.h>


#include "tdtypes.h"
#include "dispatcher.h"
#include "tagitem.h"
#include "rserpoolmessage.h"
#include "poolhandlespacemanagement.h"
#include "registrartable.h"
#include "threadsignal.h"







#if 0
struct ThreadSignal
{
   pthread_mutex_t Mutex;       /* Non-recursive mutex! */
   pthread_cond_t  Condition;
};

void threadSignalNew(struct ThreadSignal* threadSignal)
{
   /* Important: The mutex is non-recursive! */
   pthread_mutex_init(&threadSignal->Mutex, NULL);
   pthread_cond_init(&threadSignal->Condition, NULL);
};

void threadSignalDelete(struct ThreadSignal* threadSignal)
{
   pthread_cond_destroy(&threadSignal->Condition);
   pthread_mutex_destroy(&threadSignal->Mutex);
};

void threadSignalLock(struct ThreadSignal* threadSignal)
{
   pthread_mutex_lock(&threadSignal->Mutex);
}

void threadSignalUnlock(struct ThreadSignal* threadSignal)
{
   pthread_mutex_unlock(&threadSignal->Mutex);
}

void threadSignalFire(struct ThreadSignal* threadSignal)
{
   pthread_cond_signal(&threadSignal->Condition);
}

void threadSignalWait(struct ThreadSignal* threadSignal)
{
   pthread_cond_wait(&threadSignal->Condition, &threadSignal->Mutex);
}
#endif



Dispatcher   gDispatcher;
ThreadSafety gThreadSafety;





struct InterThreadMessagePort;

struct InterThreadMessageNode
{
   struct DoubleLinkedRingListNode Node;
   struct InterThreadMessagePort*  ReplyPort;
   int t1,t2;  // ?????????????????????????????????????????????????
};

struct InterThreadMessagePort
{
   struct DoubleLinkedRingList Queue;
   struct ThreadSignal         Signal;
};

void interThreadMessagePortNew(struct InterThreadMessagePort* itmPort)
{
   threadSignalNew(&itmPort->Signal);
   doubleLinkedRingListNew(&itmPort->Queue);
}

void interThreadMessagePortDelete(struct InterThreadMessagePort* itmPort)
{
   CHECK(itmPort->Queue.Node.Next == itmPort->Queue.Head);
   doubleLinkedRingListDelete(&itmPort->Queue);
   threadSignalDelete(&itmPort->Signal);
}

void interThreadMessagePortLock(struct InterThreadMessagePort* itmPort)
{
    threadSignalLock(&itmPort->Signal);
}

void interThreadMessagePortUnlock(struct InterThreadMessagePort* itmPort)
{
   threadSignalUnlock(&itmPort->Signal);
}

struct InterThreadMessageNode* interThreadMessagePortGetFirstMessage(struct InterThreadMessagePort* itmPort)
{
   struct DoubleLinkedRingListNode* node;
   node = itmPort->Queue.Node.Next;
   if(node != itmPort->Queue.Head) {
      return((struct InterThreadMessageNode*)node);
   }
   return(NULL);
}

struct InterThreadMessageNode* interThreadMessagePortGetNextMessage(struct InterThreadMessagePort* itmPort,
                                                                    struct InterThreadMessageNode*     message)
{
   struct DoubleLinkedRingListNode* node;
   node = message->Node.Next;
   if(node != itmPort->Queue.Head) {
      return((struct InterThreadMessageNode*)node);
   }
   return(NULL);
}

void interThreadMessagePortRemoveMessage(struct InterThreadMessagePort* itmPort,
                                         struct InterThreadMessageNode* message)
{
   doubleLinkedRingListRemNode(&message->Node);
   doubleLinkedRingListNodeDelete(&message->Node);
}

void interThreadMessagePortEnqueue(struct InterThreadMessagePort* itmPort,
                                   struct InterThreadMessageNode* message,
                                   struct InterThreadMessagePort* replyPort)
{
   doubleLinkedRingListNodeNew(&message->Node);
   message->ReplyPort = replyPort;

   threadSignalLock(&itmPort->Signal);
   doubleLinkedRingListAddTail(&itmPort->Queue, &message->Node);
   threadSignalFire(&itmPort->Signal);
   threadSignalUnlock(&itmPort->Signal);
}

struct InterThreadMessageNode* interThreadMessagePortDequeue(struct InterThreadMessagePort* itmPort)
{
   struct InterThreadMessageNode* message;
   threadSignalLock(&itmPort->Signal);
   message = interThreadMessagePortGetFirstMessage(itmPort);
   if(message) {
      doubleLinkedRingListRemNode(&message->Node);
      doubleLinkedRingListNodeDelete(&message->Node);
   }
   threadSignalUnlock(&itmPort->Signal);
   return(message);
}

void interThreadMessagePortWait(struct InterThreadMessagePort* itmPort)
{
   threadSignalLock(&itmPort->Signal);
   while(interThreadMessagePortGetFirstMessage(itmPort) == NULL) {
      threadSignalWait(&itmPort->Signal);
   }
   threadSignalUnlock(&itmPort->Signal);
}

void interThreadMessageReply(struct InterThreadMessageNode* message)
{
   CHECK(message->ReplyPort != NULL);
   interThreadMessagePortEnqueue(message->ReplyPort, message, NULL);
}




/* ###### Lock mutex ###################################################### */
static void lock(struct Dispatcher* dispatcher, void* userData)
{
   threadSafetyLock(&gThreadSafety);
}


/* ###### Unlock mutex #################################################### */
static void unlock(struct Dispatcher* dispatcher, void* userData)
{
   threadSafetyUnlock(&gThreadSafety);
}



static bool      RsplibThreadStop     = false;
static pthread_t RsplibMainLoopThread = 0;
static int       RsplibPipe[2];
static struct InterThreadMessagePort RsplibITMPort;

static void* rsplibMainLoop(void* args)
{
   struct pollfd ufds[10];
   char          buffer[128];
   int           result;

   puts("START!");
   while(!RsplibThreadStop) {
      ufds[0].fd     = RsplibPipe[0];
      ufds[0].events = POLLIN|POLLPRI;
printf("POLL: fd=%d\n",ufds[0].fd);
      result = poll((struct pollfd*)&ufds, 1, 10000);
printf("pres=%d\n",result);
      if(result > 0) {
         if(ufds[0].revents & POLLIN) {
            read(ufds[0].fd, (char*)&buffer, sizeof(buffer));
         }
      }

      puts("§§§§§§§§§§§§§§§ HANDLE §§§§§§§§§§§§§§§");
      struct InterThreadMessageNode* message = interThreadMessagePortDequeue(&RsplibITMPort);
      while(message != NULL) {
         printf("t1=%d\n", message->t1);
         usleep(250000);
         message->t2 = 2*message->t1;
         if(message->ReplyPort) {
            puts("rep...");
            interThreadMessageReply(message);
            puts("rep.. OK!!!!!!!!!!!!!!!!");
         }
         message = interThreadMessagePortDequeue(&RsplibITMPort);
      }
      puts("§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§");
   }
   puts("ENDE!");
   return(NULL);
}



int main(int argc, char** argv)
{
   struct InterThreadMessagePort myitmp;

   threadSafetyNew(&gThreadSafety, "gRSerPoolSocketSet");
   dispatcherNew(&gDispatcher, lock, unlock, NULL);
   interThreadMessagePortNew(&myitmp);
   interThreadMessagePortNew(&RsplibITMPort);

   CHECK(pipe((int*)&RsplibPipe) == 0);
   setNonBlocking(RsplibPipe[1]);


   pthread_create(&RsplibMainLoopThread, NULL, &rsplibMainLoop, NULL);


   int i=0;
   installBreakDetector();
   while(!breakDetected()) {
      usleep(1000000);

      InterThreadMessageNode* i1 = (InterThreadMessageNode*)malloc(sizeof(InterThreadMessageNode));
      InterThreadMessageNode* i2 = (InterThreadMessageNode*)malloc(sizeof(InterThreadMessageNode));
      i1->t1=i++;
      i2->t1=i++;
      puts("############ SEND........");
      interThreadMessagePortEnqueue(&RsplibITMPort, i1, &myitmp);
      interThreadMessagePortEnqueue(&RsplibITMPort, i2, &myitmp);

      write(RsplibPipe[1], "X", 1);

      puts("########### RECV.........");
      interThreadMessagePortWait(&myitmp);
      struct InterThreadMessageNode* message = interThreadMessagePortDequeue(&myitmp);
      CHECK(message != NULL);
      while(message != NULL) {
         printf("REPLY: t1=%d t2=%d\n",message->t1,message->t2);
         free(message);
         message = interThreadMessagePortDequeue(&myitmp);
      }
      puts("#########################");
   }

   RsplibThreadStop = true;
   write(RsplibPipe[1], "X", 1);
   pthread_join(RsplibMainLoopThread, NULL);

   struct InterThreadMessageNode* message = interThreadMessagePortDequeue(&myitmp);
   while(message != NULL) {
      free(message);
      message = interThreadMessagePortDequeue(&myitmp);
   }

   interThreadMessagePortDelete(&myitmp);
   interThreadMessagePortDelete(&RsplibITMPort);
   dispatcherDelete(&gDispatcher);
   return(0);
}

