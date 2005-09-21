#include "tdtypes.h"
#include "loglevel.h"
#include "rserpoolmessage.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "rsplib-tags.h"
#include "threadsafety.h"
#include "breakdetector.h"

#include <ext_socket.h>


#include "tdtypes.h"
#include "dispatcher.h"
#include "tagitem.h"
#include "rserpoolmessage.h"
#include "poolhandlespacemanagement.h"
#include "registrartable.h"




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




struct ASAPInterThreadMessage
{
   struct InterThreadMessageNode Node;
   struct RSerPoolMessage*       Request;
   struct RSerPoolMessage*       Response;
};


struct ASAPInstance
{
   struct Dispatcher*                         StateMachine;

   InterThreadMessagePort                     MainLoopPort;
   int                                        RegistrarHuntSocket;
   int                                        RegistrarSocket;
   RegistrarIdentifierType                    RegistrarIdentifier;
   unsigned long long                         RegistrarConnectionTimeStamp;

   struct RegistrarTable*                     RegistrarSet;
   struct ST_CLASS(PoolHandlespaceManagement) Cache;
   struct ST_CLASS(PoolHandlespaceManagement) OwnPoolElements;

   unsigned long long                         CacheElementTimeout;

   struct FDCallback                          RegistrarHuntFDCallback;
   struct FDCallback                          RegistrarFDCallback;

   size_t                                     RegistrarRequestMaxTrials;
   unsigned long long                         RegistrarRequestTimeout;
   unsigned long long                         RegistrarResponseTimeout;
};


#define ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT               5000000
#define ASAP_DEFAULT_REGISTRAR_ANNOUNCE_ADDRESS "239.0.0.1:3863"
#define ASAP_DEFAULT_REGISTRAR_ANNOUNCE_TIMEOUT          5000000
#define ASAP_DEFAULT_REGISTRAR_CONNECT_MAXTRIALS               3
#define ASAP_DEFAULT_REGISTRAR_CONNECT_TIMEOUT           1500000
#define ASAP_DEFAULT_REGISTRAR_REQUEST_MAXTRIALS               1
#define ASAP_DEFAULT_REGISTRAR_REQUEST_TIMEOUT           1500000
#define ASAP_DEFAULT_REGISTRAR_RESPONSE_TIMEOUT          1500000

#define ASAP_BUFFER_SIZE 65536


static void handleRegistrarConnectionEvent(
               struct Dispatcher* dispatcher,
               int                fd,
               unsigned int       eventMask,
               void*              userData)
{
   puts("SOCK!");
}


/* ###### Get configuration from file #################################### */
static void asapInstanceConfigure(struct ASAPInstance* asapInstance, struct TagItem* tags)
{
   /* ====== ASAP Instance settings ======================================= */
   asapInstance->RegistrarRequestMaxTrials = tagListGetData(tags, TAG_RspLib_RegistrarRequestMaxTrials,
                                                            ASAP_DEFAULT_REGISTRAR_REQUEST_MAXTRIALS);
   asapInstance->RegistrarRequestTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_RegistrarRequestTimeout,
                                                                              ASAP_DEFAULT_REGISTRAR_REQUEST_TIMEOUT);
   asapInstance->RegistrarResponseTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_RegistrarResponseTimeout,
                                                                               ASAP_DEFAULT_REGISTRAR_RESPONSE_TIMEOUT);
   asapInstance->CacheElementTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_CacheElementTimeout,
                                                                          ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT);


   /* ====== Show results =================================================== */
   LOG_VERBOSE3
   fputs("New ASAP instance's configuration:\n", stdlog);
   fprintf(stdlog, "registrar.request.maxtrials   = %u\n",     (unsigned int)asapInstance->RegistrarRequestMaxTrials);
   fprintf(stdlog, "registrar.request.timeout     = %lluus\n", asapInstance->RegistrarRequestTimeout);
   fprintf(stdlog, "registrar.response.timeout    = %lluus\n", asapInstance->RegistrarResponseTimeout);
   fprintf(stdlog, "cache.elementtimeout          = %lluus\n", asapInstance->CacheElementTimeout);
   LOG_END
}


/* ###### Destructor ##################################################### */
void asapInstanceDelete(struct ASAPInstance* asapInstance)
{
   if(asapInstance) {
      if(asapInstance->RegistrarHuntSocket >= 0) {
         fdCallbackDelete(&asapInstance->RegistrarHuntFDCallback);
         ext_close(asapInstance->RegistrarHuntSocket);
      }
      ST_CLASS(poolHandlespaceManagementClear)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolHandlespaceManagementDelete)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolHandlespaceManagementClear)(&asapInstance->Cache);
      ST_CLASS(poolHandlespaceManagementDelete)(&asapInstance->Cache);
      if(asapInstance->RegistrarSet) {
         registrarTableDelete(asapInstance->RegistrarSet);
         asapInstance->RegistrarSet = NULL;
      }
      interThreadMessagePortDelete(&asapInstance->MainLoopPort);
      free(asapInstance);
   }
}


/* ###### Constructor #################################################### */
struct ASAPInstance* asapInstanceNew(struct Dispatcher* dispatcher,
                                     struct TagItem*    tags)
{
   struct ASAPInstance*        asapInstance = NULL;
   struct sctp_event_subscribe sctpEvents;
   int                         autoCloseTimeout;

   if(dispatcher != NULL) {
      asapInstance = (struct ASAPInstance*)malloc(sizeof(struct ASAPInstance));
      if(asapInstance != NULL) {
         interThreadMessagePortNew(&asapInstance->MainLoopPort);
         asapInstance->StateMachine = dispatcher;

         asapInstanceConfigure(asapInstance, tags);

         asapInstance->RegistrarConnectionTimeStamp = 0;
         asapInstance->RegistrarHuntSocket          = -1;
         asapInstance->RegistrarSocket              = -1;
         asapInstance->RegistrarIdentifier          = 0;
         ST_CLASS(poolHandlespaceManagementNew)(&asapInstance->Cache,
                                                0x00000000,
                                                NULL, NULL, NULL);
         ST_CLASS(poolHandlespaceManagementNew)(&asapInstance->OwnPoolElements,
                                                0x00000000,
                                                NULL, NULL, NULL);
         asapInstance->RegistrarSet = registrarTableNew(asapInstance->StateMachine, tags);
         if(asapInstance->RegistrarSet == NULL) {
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         /* ====== Registrar socket ========================================== */
         asapInstance->RegistrarHuntSocket = ext_socket(checkIPv6() ? AF_INET6 : AF_INET,
                                                        SOCK_SEQPACKET,
                                                        IPPROTO_SCTP);
         if(asapInstance->RegistrarHuntSocket < 0) {
            LOG_ERROR
            logerror("Creating registrar hunt socket failed");
            LOG_END
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         fdCallbackNew(&asapInstance->RegistrarHuntFDCallback,
                       asapInstance->StateMachine,
                       asapInstance->RegistrarHuntSocket,
                       FDCE_Read|FDCE_Exception,
                       handleRegistrarConnectionEvent,
                       (void*)asapInstance);

         if(bindplus(asapInstance->RegistrarHuntSocket, NULL, 0) == false) {
            LOG_ERROR
            logerror("Binding registrar hunt socket failed");
            LOG_END
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         setNonBlocking(asapInstance->RegistrarHuntSocket);

         if(ext_listen(asapInstance->RegistrarHuntSocket, 10) < 0) {
            LOG_ERROR
            logerror("Unable to turn registrar hunt socket into listening mode");
            LOG_END
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         memset(&sctpEvents, 0, sizeof(sctpEvents));
         sctpEvents.sctp_data_io_event          = 1;
         sctpEvents.sctp_association_event      = 1;
         sctpEvents.sctp_address_event          = 1;
         sctpEvents.sctp_send_failure_event     = 1;
         sctpEvents.sctp_peer_error_event       = 1;
         sctpEvents.sctp_shutdown_event         = 1;
         sctpEvents.sctp_partial_delivery_event = 1;
         sctpEvents.sctp_adaption_layer_event   = 1;

         if(ext_setsockopt(asapInstance->RegistrarHuntSocket, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
            logerror("setsockopt() for SCTP_EVENTS on registrar hunt socket failed");
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         autoCloseTimeout = 30;
         if(ext_setsockopt(asapInstance->RegistrarHuntSocket, IPPROTO_SCTP, SCTP_AUTOCLOSE, &autoCloseTimeout, sizeof(autoCloseTimeout)) < 0) {
            logerror("setsockopt() for SCTP_AUTOCLOSE on registrar hunt socket failed");
            exit(1);
         }
      }
   }
   return(asapInstance);
}

/* ###### Connect to registrar ############################################ */
static bool asapInstanceConnectToRegistrar(struct ASAPInstance* asapInstance,
                                           int                  sd)
{
   if(asapInstance->RegistrarSocket < 0) {
      if(sd < 0) {
         LOG_ACTION
         fputs("Starting registrar hunt...\n", stdlog);
         LOG_END
         if(sd < 0) {
            sd = registrarTableGetRegistrar(asapInstance->RegistrarSet,
                                            asapInstance->RegistrarHuntSocket,
                                            &asapInstance->RegistrarIdentifier);
         }
         if(sd < 0) {
            LOG_ACTION
            fputs("Unable to connect to a registrar\n", stdlog);
            LOG_END
            return(false);
         }
      }

      asapInstance->RegistrarSocket = sd;
      fdCallbackNew(&asapInstance->RegistrarFDCallback,
                    asapInstance->StateMachine,
                    asapInstance->RegistrarSocket,
                    FDCE_Read|FDCE_Exception,
                    handleRegistrarConnectionEvent,
                    (void*)asapInstance);

      LOG_NOTE
      fprintf(stdlog, "Connected to registrar $%08x\n", asapInstance->RegistrarIdentifier);
      LOG_END
   }
   return(true);
}


/* ###### Disconnect from registrar #################################### */
static void asapInstanceDisconnectFromRegistrar(struct ASAPInstance* asapInstance,
                                                bool                 sendAbort)
{
   if(asapInstance->RegistrarSocket >= 0) {
      fdCallbackDelete(&asapInstance->RegistrarFDCallback);
      if(sendAbort) {
         /* Abort association to current registrar */
         sendtoplus(asapInstance->RegistrarSocket, NULL, 0, SCTP_ABORT,
                    NULL, 0,
                    0, 0, 0, 0xffffffff, 0);
      }
      ext_close(asapInstance->RegistrarSocket);
      asapInstance->RegistrarSocket              = -1;
      asapInstance->RegistrarConnectionTimeStamp = 0;
      asapInstance->RegistrarIdentifier          = UNDEFINED_REGISTRAR_IDENTIFIER;

      LOG_ACTION
      fputs("Disconnected from registrar\n", stdlog);
      LOG_END
   }
}


/* ###### Send request to registrar #################################### */
static unsigned int asapInstanceSendRequest(struct ASAPInstance*     asapInstance,
                                            struct RSerPoolMessage*  request)
{
   struct ASAPInterThreadMessage* aitm = (struct ASAPInterThreadMessage*)malloc(sizeof(struct ASAPInterThreadMessage));
   if(aitm == NULL) {
      return(RSPERR_OUT_OF_MEMORY);
   }

   aitm->Request  = request;
   aitm->Response = NULL;
   interThreadMessagePortEnqueue(&asapInstance->MainLoopPort, &aitm->Node, NULL);
   return(RSPERR_OKAY);
}


/* ###### Report pool element failure ####################################### */
unsigned int asapInstanceReportFailure(struct ASAPInstance*            asapInstance,
                                       struct PoolHandle*              poolHandle,
                                       const PoolElementIdentifierType identifier)
{
   struct RSerPoolMessage*           message;
   struct ST_CLASS(PoolElementNode)* found;
   unsigned int                      result;

   LOG_VERBOSE
   fprintf(stdlog, "Failure reported for pool element $%08x of pool ",
           (unsigned int)identifier);
   poolHandlePrint(poolHandle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   /* ====== Remove pool element from cache ================================= */
   dispatcherLock(asapInstance->StateMachine);

   found = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
              &asapInstance->Cache,
              poolHandle,
              identifier);
   if(found != NULL) {
      result = ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
                          &asapInstance->Cache,
                          poolHandle,
                          identifier);
      CHECK(result == RSPERR_OKAY);
   }
   else {
      LOG_VERBOSE
      fputs("Pool element does not exist in cache\n", stdlog);
      LOG_END
   }

   /* ====== Report unreachability ========================================== */
   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type       = AHT_ENDPOINT_UNREACHABLE;
      message->Flags      = 0x00;
      message->Handle     = *poolHandle;
      message->Identifier = identifier;
      result = asapInstanceSendRequest(asapInstance, message);
      if(result != RSPERR_OKAY) {
         LOG_VERBOSE2
         fprintf(stdlog, "Failed to send failure report for pool element $%08x of pool ",
               (unsigned int)identifier);
         poolHandlePrint(poolHandle, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_OUT_OF_MEMORY;
   }

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}











/* ###### Get poll() parameters ########################################## */
void dispatcherGetPollParameters(struct Dispatcher*  dispatcher,
                                 struct pollfd*      ufds,
                                 unsigned int*       nfds,
                                 int*                timeout,
                                 unsigned long long* pollTimeStamp)
{
   struct LeafLinkedRedBlackTreeNode* node;
   struct FDCallback*                 fdCallback;
   struct Timer*                      timer;
   int64                              timeToNextEvent;

   *nfds    = 0;
   *timeout = 60000;
   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);

      /*  ====== Create F sets for poll() ================================ */
      *pollTimeStamp = getMicroTime();
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->FDCallbackStorage);
      while(node != NULL) {
         fdCallback = (struct FDCallback*)node;
         if(fdCallback->EventMask & (FDCE_Read|FDCE_Write|FDCE_Exception)) {
            fdCallback->SelectTimeStamp = *pollTimeStamp;
            ufds[*nfds].fd     = fdCallback->FD;
            ufds[*nfds].events = fdCallback->EventMask & (FDCE_Read|FDCE_Write|FDCE_Exception);
            *nfds++;
         }
         else {
            LOG_WARNING
            fputs("Empty event mask?!\n",stdlog);
            LOG_END
         }
         node = leafLinkedRedBlackTreeGetNext(&dispatcher->FDCallbackStorage, node);
      }

      /*  ====== Get time to next timer event ============================ */
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->TimerStorage);
      if(node != NULL) {
         timer = (struct Timer*)node;
         timeToNextEvent = max((int64)0,(int64)timer->TimeStamp - (int64)*pollTimeStamp);
         *timeout = (int)(timeToNextEvent / 1000);
      }
      else {
         *timeout = -1;
      }

      dispatcherUnlock(dispatcher);
   }
}


/* ###### Find FDCallback for given file descriptor ###################### */
struct FDCallback* dispatcherFindFDCallbackForDescriptor(struct Dispatcher* dispatcher,
                                                         int                fd)
{
   struct FDCallback                  cmpNode;
   struct LeafLinkedRedBlackTreeNode* node;

   cmpNode.FD = fd;
   node = leafLinkedRedBlackTreeFind(&dispatcher->FDCallbackStorage, &cmpNode.Node);
   if(node != NULL) {
      return((struct FDCallback*)node);
   }
   return(NULL);
}


/* ###### Handle timer events ############################################ */
static void dispatcherHandleTimerEvents(struct Dispatcher* dispatcher)
{
   struct LeafLinkedRedBlackTreeNode* node;
   struct Timer*                      timer;
   unsigned long long                 now;

   dispatcherLock(dispatcher);

   node = leafLinkedRedBlackTreeGetFirst(&dispatcher->TimerStorage);
   while(node != NULL) {
      timer = (struct Timer*)node;
      now   = getMicroTime();

      if(now >= timer->TimeStamp) {
         timer->TimeStamp = 0;
         leafLinkedRedBlackTreeRemove(&dispatcher->TimerStorage,
                                      &timer->Node);
         if(timer->Callback != NULL) {
            timer->Callback(dispatcher, timer, timer->UserData);
         }
      }
      else {
         break;
      }
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->TimerStorage);
   }

   dispatcherUnlock(dispatcher);
}


/* ###### Handle poll() result ########################################### */
void dispatcherHandlePollResult(struct Dispatcher* dispatcher,
                                int                result,
                                struct pollfd*     ufds,
                                unsigned int       nfds,
                                int                timeout,
                                unsigned long long pollTimeStamp)
{
   struct FDCallback* fdCallback;
   unsigned int       i;

   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);

      /* ====== Handle events ============================================ */
      /* We handle the FD callbacks first, because their corresponding FD's
         state has been returned by ext_poll(), since a timer callback
         might modify them (e.g. writing to a socket and reading the
         complete results).
      */
      if(result > 0) {
         LOG_VERBOSE3
         fputs("Handling FD events...\n", stdlog);
         LOG_END
         for(i = 0;i < nfds;i++) {
            if(ufds[i].revents) {
               fdCallback = dispatcherFindFDCallbackForDescriptor(dispatcher, ufds[i].fd);
               if((fdCallback) &&
                  (fdCallback->SelectTimeStamp <= pollTimeStamp) &&
                  (ufds[i].revents & fdCallback->EventMask)) {
                  LOG_VERBOSE3
                  fprintf(stdlog,"Event $%04x (mask $%04x) for socket %d\n",
                          ufds[i].revents, fdCallback->EventMask, fdCallback->FD);
                  LOG_END

                  if(fdCallback->Callback != NULL) {
                     LOG_VERBOSE2
                     fprintf(stdlog,"Executing callback for event $%04x of socket %d\n",
                             ufds[i].revents, fdCallback->FD);
                     LOG_END
                     fdCallback->Callback(dispatcher,
                                          fdCallback->FD, ufds[i].revents,
                                          fdCallback->UserData);
                  }
               }
            }
            else {
               LOG_VERBOSE4
               fprintf(stdlog, "FD callback for FD %d is newer than begin of ext_select() -> Skipping.\n", fdCallback->FD);
               LOG_END
            }
         }
      }

      /* Timers must be handled after the FD callbacks, since
         they might modify the FDs' states (e.g. completely
         reading their buffers, establishing new associations, ...)! */
      LOG_VERBOSE3
      fputs("Handling timer events...\n", stdlog);
      LOG_END
      dispatcherHandleTimerEvents(dispatcher);

      dispatcherUnlock(dispatcher);
   }
}








Dispatcher     gDispatcher;
ThreadSafety   gThreadSafety;


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

static void* rsplibMainLoop(void* args)
{
   struct ASAPInstance* asapInstance = (struct ASAPInstance*)args;
   char buffer[1024];
   struct timeval     timeout;
   unsigned long long pollTimeStamp;
   fd_set             testfds;
   fd_set             readfds;
   fd_set             writefds;
   fd_set             exceptfds;
   int                result;
   int                n;

   asapInstanceConnectToRegistrar(asapInstance, -1);

   while(!RsplibThreadStop) {
      /* ====== Schedule ================================================= */
      /* pthreads seem to have the property that scheduling is quite
         unfair -> If the main loop only invokes rspSelect(), this
         may block other threads forever => explicitly let other threads
         do their work now, then lock! */
      sched_yield();

      /* ====== Collect data for ext_select() call ======================= */
      timeout.tv_sec  = 30;
      timeout.tv_usec = 0;
      dispatcherGetSelectParameters(&gDispatcher,
                                    &n, &readfds, &writefds, &exceptfds,
                                    &testfds, &pollTimeStamp,
                                    &timeout);
      FD_SET(RsplibPipe[0], &readfds);

      /* ====== Do ext_select() call ===================================== */
      result = ext_select(n + 1, &readfds, &writefds, &exceptfds, &timeout);

      /* ====== Handle results =========================================== */
      dispatcherHandleSelectResult(&gDispatcher, result,
                                   &readfds, &writefds, &exceptfds,
                                   &testfds, pollTimeStamp);

      if(FD_ISSET(RsplibPipe[0], &readfds)) {
         // read(ufds[0].fd, (char*)&buffer, sizeof(buffer));
         read(RsplibPipe[0], (char*)&buffer, sizeof(buffer));
         puts("X");
      }
      puts(".");
   }

#if 0
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
            puts("X");
         }
      }

      struct InterThreadMessage* message = interThreadMessagePortDequeue(&RsplibITMPort);
      while(message != NULL) {
         printf("t1=%d\n", message->t1);
         message->t2 = 2*message->t1;
         if(message->ReplyPort) {
            puts("rep...");
            interThreadMessageReply(message);
         }
         message = interThreadMessagePortDequeue(&RsplibITMPort);
      }


   puts(".");

   }
#endif

   puts("ENDE!");
   asapInstanceDisconnectFromRegistrar(asapInstance, false);
   puts("---T.ENDE---");
   return(NULL);
}



int main(int argc, char** argv)
{
   for(int n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-log" ,4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
   }
   beginLogging();

   threadSafetyInit(&gThreadSafety, "gRSerPoolSocketSet");
   dispatcherNew(&gDispatcher, lock, unlock, NULL);
   ASAPInstance* gAsapInstance = asapInstanceNew(&gDispatcher,NULL);
   CHECK(gAsapInstance);

   CHECK(pipe((int*)&RsplibPipe) == 0);
   setNonBlocking(RsplibPipe[1]);
   pthread_create(&RsplibMainLoopThread, NULL, &rsplibMainLoop, gAsapInstance);


   puts("START!");

   installBreakDetector();
   while(!breakDetected()) {
      usleep(1000000);
      struct PoolHandle ph;
      poolHandleNew(&ph, (unsigned char*)"TestPool", 8);
      printf("REPORT.RESULT=%d\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
   }


   RsplibThreadStop = true;
puts("wrt...");
   write(RsplibPipe[1], "X", 1);
puts("ok!");
   pthread_join(RsplibMainLoopThread, NULL);

   asapInstanceDelete(gAsapInstance);
   dispatcherDelete(&gDispatcher);
   finishLogging();
   return(0);
}

