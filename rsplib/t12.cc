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
static int       RsplibPipe[2];
static void handleRegistrarConnectionEvent(struct Dispatcher* dispatcher,
                                           int                fd,
                                           unsigned int       eventMask,
                                           void*              userData);




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
   unsigned int                  Error;
   bool                          ResponseExpected;

};


struct ASAPInstance
{
   struct Dispatcher*                         StateMachine;

   InterThreadMessagePort                     MainLoopPort;
   struct ASAPInterThreadMessage*             LastAITM;

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

static void asapInstanceDisconnectFromRegistrar(
               struct ASAPInstance* asapInstance,
               bool                 sendAbort);


/* ###### Handle SCTP notification on registrar socket ################### */
static void handleNotificationOnRegistrarSocket(struct ASAPInstance*           asapInstance,
                                                const union sctp_notification* notification)
{
   if( (notification->sn_header.sn_type == SCTP_ASSOC_CHANGE) &&
       (notification->sn_assoc_change.sac_state == SCTP_COMM_LOST) ) {
      LOG_WARNING
      fputs("Registrar connection lost\n", stdlog);
      LOG_END
      asapInstanceDisconnectFromRegistrar(asapInstance, true);
   }
   else if(notification->sn_header.sn_type == SCTP_SHUTDOWN_EVENT) {
      LOG_WARNING
      fputs("Registrar connection is shutting down\n", stdlog);
      LOG_END
      asapInstanceDisconnectFromRegistrar(asapInstance, true);
   }
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
         asapInstance->LastAITM     = NULL;
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
      asapInstance->LastAITM = NULL; /* Send requests again! */

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
      asapInstance->LastAITM                     = NULL; /* Send requests again! */

      LOG_ACTION
      fputs("Disconnected from registrar\n", stdlog);
      LOG_END
   }
}




/* ###### Send request without response to registrar ##################### */
static unsigned int asapInstanceSendRequest(struct ASAPInstance*    asapInstance,
                                            struct RSerPoolMessage* request,
                                            const bool              responseExpected)
{
   struct ASAPInterThreadMessage* aitm = (struct ASAPInterThreadMessage*)malloc(sizeof(struct ASAPInterThreadMessage));
   if(aitm == NULL) {
      return(RSPERR_OUT_OF_MEMORY);
   }

   aitm->Request          = request;
   aitm->Response         = NULL;
   aitm->ResponseExpected = responseExpected;
   aitm->Error            = RSPERR_OKAY;
   interThreadMessagePortEnqueue(&asapInstance->MainLoopPort, &aitm->Node, NULL);
   ext_write(RsplibPipe[1], "X", 1);
   return(RSPERR_OKAY);
}


/* ###### Send request to registrar ###################################### */
static unsigned int asapInstanceBeginIO(struct ASAPInstance*           asapInstance,
                                        struct InterThreadMessagePort* itmPort,
                                        struct RSerPoolMessage*        request)
{
   struct ASAPInterThreadMessage* aitm;

   aitm = (struct ASAPInterThreadMessage*)malloc(sizeof(struct ASAPInterThreadMessage));
   if(aitm == NULL) {
      return(RSPERR_OUT_OF_MEMORY);
   }

puts("SendToITM...");
   aitm->Request          = request;
   aitm->Response         = NULL;
   aitm->ResponseExpected = true;
   aitm->Error            = RSPERR_OKAY;
   interThreadMessagePortEnqueue(&asapInstance->MainLoopPort, &aitm->Node, itmPort);
   ext_write(RsplibPipe[1], "X", 1);
   return(RSPERR_OKAY);
}


/* ###### Wait for response ############################################## */
static unsigned int asapInstanceWaitIO(struct ASAPInstance*           asapInstance,
                                       struct InterThreadMessagePort* itmPort,
                                       struct RSerPoolMessage*        request,
                                       struct RSerPoolMessage**       responsePtr,
                                       uint16_t*                      errorPtr)
{
   struct ASAPInterThreadMessage* aitm;
   unsigned int                   result;

puts("WAIT...");
   interThreadMessagePortWait(itmPort);
   aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortDequeue(itmPort);
   CHECK(aitm != NULL);
   *responsePtr = aitm->Response;
   *errorPtr    = aitm->Response->Error;  // ???????????
   result = aitm->Error;
   free(aitm);
   return(result);
}


/* ###### Send request to registrar and wait for response ############## */
static unsigned int asapInstanceDoIO(struct ASAPInstance*     asapInstance,
                                     struct RSerPoolMessage*  request,
                                     struct RSerPoolMessage** responsePtr,
                                     uint16_t*                errorPtr)
{
   struct InterThreadMessagePort itmPort;
   unsigned int                  result;

   interThreadMessagePortNew(&itmPort);
   result = asapInstanceBeginIO(asapInstance, &itmPort, request);
   if(result == RSPERR_OKAY) {
      result = asapInstanceWaitIO(asapInstance, &itmPort, request, responsePtr, errorPtr);
   }
   interThreadMessagePortDelete(&itmPort);
   return(result);
}


/* ###### Register pool element ########################################## */
unsigned int asapInstanceRegister(struct ASAPInstance*              asapInstance,
                                  struct PoolHandle*                poolHandle,
                                  struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct RSerPoolMessage*           message;
   struct RSerPoolMessage*           response;
   struct ST_CLASS(PoolElementNode)* oldPoolElementNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   unsigned int                      result;
   uint16_t                          registrarResult;
   unsigned int                      handlespaceMgtResult;

   LOG_VERBOSE
   fputs("Trying to register to pool ", stdlog);
   poolHandlePrint(poolHandle, stdlog);
   fputs(" pool element ", stdlog);
   ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
   fputs("\n", stdlog);
   LOG_END

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type           = AHT_REGISTRATION;
      message->Flags          = 0x00;
      message->Handle         = *poolHandle;
      message->PoolElementPtr = poolElementNode;

      /* ====== If reregistration, are new settings compatible? ========== */
      dispatcherLock(asapInstance->StateMachine);
      oldPoolElementNode = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                              &asapInstance->OwnPoolElements,
                              poolHandle,
                              message->PoolElementPtr->Identifier);
      if(oldPoolElementNode) {
         result = ST_CLASS(poolNodeCheckPoolElementNodeCompatibility)(
                     oldPoolElementNode->OwnerPoolNode,
                     oldPoolElementNode);
      }
      else {
         result = RSPERR_OKAY;
      }
      dispatcherUnlock(asapInstance->StateMachine);

      /* ====== Send registration ======================================== */
      if(result == RSPERR_OKAY) {
         result = asapInstanceDoIO(asapInstance, message, &response, &registrarResult);
         if(result == RSPERR_OKAY) {
            if(registrarResult == RSPERR_OKAY) {
               dispatcherLock(asapInstance->StateMachine);

               handlespaceMgtResult = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                                       &asapInstance->OwnPoolElements,
                                       poolHandle,
                                       message->PoolElementPtr->HomeRegistrarIdentifier,
                                       message->PoolElementPtr->Identifier,
                                       message->PoolElementPtr->RegistrationLife,
                                       &message->PoolElementPtr->PolicySettings,
                                       message->PoolElementPtr->UserTransport,
                                       NULL,
                                       -1, 0,
                                       getMicroTime(),
                                       &newPoolElementNode);
               if(handlespaceMgtResult == RSPERR_OKAY) {
                  newPoolElementNode->UserData = (void*)asapInstance;
                  if(response->Identifier != poolElementNode->Identifier) {
                     LOG_ERROR
                     fprintf(stdlog, "Tried to register PE $%08x, got response for PE $%08x\n",
                           poolElementNode->Identifier,
                           response->Identifier);
                     LOG_END_FATAL
                  }
               }
               else {
                  LOG_ERROR
                  fprintf(stdlog, "Unable to register pool element $%08x of pool ",
                        poolElementNode->Identifier);
                  poolHandlePrint(poolHandle, stdlog);
                  fputs(" to OwnPoolElements\n", stdlog);
                  LOG_END_FATAL
               }

               dispatcherUnlock(asapInstance->StateMachine);
            }
            else {
               result = (unsigned int)registrarResult;
            }
            if(response) {
               rserpoolMessageDelete(response);
            }
         }
         else {
            LOG_ERROR
            fputs("Incompatible PE settings for reregistration!Old: \n", stderr);
            ST_CLASS(poolElementNodePrint)(oldPoolElementNode, stdlog, PENPO_FULL);
            fputs("New: \n", stderr);
            ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
            LOG_END
         }
      }

      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_NO_RESOURCES;
   }

   LOG_VERBOSE
   fputs("Registration result is: ", stdlog);
   rserpoolErrorPrint(result, stdlog);
   fputs("\n", stdlog);
   LOG_END

   return(result);
}


/* ###### Deregister pool element ######################################## */
unsigned int asapInstanceDeregister(struct ASAPInstance*            asapInstance,
                                    struct PoolHandle*              poolHandle,
                                    const PoolElementIdentifierType identifier)
{
   struct RSerPoolMessage* message;
   struct RSerPoolMessage* response;
   unsigned int            result;
   uint16_t                registrarResult;
   unsigned int            handlespaceMgtResult;

   LOG_VERBOSE
   fprintf(stdlog, "Trying to deregister $%08x from pool ",identifier);
   poolHandlePrint(poolHandle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type       = AHT_DEREGISTRATION;
      message->Flags      = 0x00;
      message->Handle     = *poolHandle;
      message->Identifier = identifier;

      result = asapInstanceDoIO(asapInstance, message, &response, &registrarResult);
      if((result == RSPERR_OKAY) && (registrarResult == RSPERR_OKAY)) {
         if(identifier != response->Identifier) {
            LOG_ERROR
            fprintf(stdlog, "Tried to deregister PE $%08x, got response for PE $%08x\n",
                    identifier,
                    message->Identifier);
            LOG_END_FATAL
         }

         handlespaceMgtResult = ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
                                   &asapInstance->OwnPoolElements,
                                   poolHandle,
                                   identifier);
         if(handlespaceMgtResult != RSPERR_OKAY) {
            LOG_ERROR
            fprintf(stdlog, "Unable to deregister pool element $%08x of pool ",
                    identifier);
            poolHandlePrint(poolHandle, stdlog);
            fputs(" from OwnPoolElements\n", stdlog);
            LOG_END
         }
         if(response) {
            rserpoolMessageDelete(response);
         }
      }

      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_NO_RESOURCES;
   }

   LOG_VERBOSE
   fputs("Deregistration result is: ", stdlog);
   rserpoolErrorPrint(result, stdlog);
   fputs("\n", stdlog);
   LOG_END
   return(result);
}


/* ###### Do name lookup ################################################# */
static unsigned int asapInstanceDoHandleResolution(struct ASAPInstance* asapInstance,
                                                   struct PoolHandle*   poolHandle)
{
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct RSerPoolMessage*           message;
   struct RSerPoolMessage*           response;
   unsigned int                      result;
   uint16_t                          registrarResult;
   size_t                            i;

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type   = AHT_HANDLE_RESOLUTION;
      message->Flags  = 0x00;
      message->Handle = *poolHandle;

      result = asapInstanceDoIO(asapInstance, message, &response, &registrarResult);
      if(result == RSPERR_OKAY) {
         if(registrarResult == RSPERR_OKAY) {
            LOG_VERBOSE
            fprintf(stdlog, "Got %u elements in handle resolution response\n",
                    (unsigned int)response->PoolElementPtrArraySize);
            LOG_END
            for(i = 0;i < response->PoolElementPtrArraySize;i++) {
               LOG_VERBOSE2
               fputs("Adding pool element to cache: ", stdlog);
               ST_CLASS(poolElementNodePrint)(response->PoolElementPtrArray[i], stdlog, PENPO_FULL);
               fputs("\n", stdlog);
               LOG_END
               result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                           &asapInstance->Cache,
                           poolHandle,
                           response->PoolElementPtrArray[i]->HomeRegistrarIdentifier,
                           response->PoolElementPtrArray[i]->Identifier,
                           response->PoolElementPtrArray[i]->RegistrationLife,
                           &response->PoolElementPtrArray[i]->PolicySettings,
                           response->PoolElementPtrArray[i]->UserTransport,
                           NULL,
                           -1, 0,
                           getMicroTime(),
                           &newPoolElementNode);
               if(result != RSPERR_OKAY) {
                  LOG_WARNING
                  fputs("Failed to add pool element to cache: ", stdlog);
                  ST_CLASS(poolElementNodePrint)(response->PoolElementPtrArray[i], stdlog, PENPO_FULL);
                  fputs(": ", stdlog);
                  rserpoolErrorPrint(result, stdlog);
                  fputs("\n", stdlog);
                  LOG_END
               }
               ST_CLASS(poolHandlespaceManagementRestartPoolElementExpiryTimer)(
                  &asapInstance->Cache,
                  newPoolElementNode,
                  asapInstance->CacheElementTimeout);
            }
         }
         else {
            LOG_VERBOSE2
            fprintf(stdlog, "Handle Resolution at registrar for pool ");
            poolHandlePrint(poolHandle, stdlog);
            fputs(" failed: ", stdlog);
            rserpoolErrorPrint(registrarResult, stdlog);
            fputs("\n", stdlog);
            LOG_END
            result = registrarResult;
         }
         rserpoolMessageDelete(response);
      }
      else {
         LOG_VERBOSE2
         fprintf(stdlog, "Handle Resolution at registrar for pool ");
         poolHandlePrint(poolHandle, stdlog);
         fputs(" failed: ", stdlog);
         rserpoolErrorPrint(registrarResult, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_NO_RESOURCES;
   }

   return(result);
}


/* ###### Do name lookup from cache ###################################### */
static unsigned int asapInstanceHandleResolutionFromCache(
                       struct ASAPInstance*               asapInstance,
                       struct PoolHandle*                 poolHandle,
                       struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                       size_t*                            poolElementNodes)
{
   unsigned int result;
   size_t       i;

   LOG_VERBOSE
   fprintf(stdlog, "Handle Resolution for pool ");
   poolHandlePrint(poolHandle, stdlog);
   fputs(":\n", stdlog);
   ST_CLASS(poolHandlespaceManagementPrint)(&asapInstance->Cache,
                                            stdlog, PENPO_ONLY_POLICY);
   LOG_END

   i = ST_CLASS(poolHandlespaceManagementPurgeExpiredPoolElements)(
          &asapInstance->Cache,
          getMicroTime());
   LOG_VERBOSE
   fprintf(stdlog, "Purged %u out-of-date elements\n", (unsigned int)i);
   LOG_END

   if(ST_CLASS(poolHandlespaceManagementHandleResolution)(
         &asapInstance->Cache,
         poolHandle,
         poolElementNodeArray,
         poolElementNodes,
         *poolElementNodes,
         1000000000) == RSPERR_OKAY) {
      LOG_VERBOSE
      fprintf(stdlog, "Got %u items:\n", (unsigned int)*poolElementNodes);
      for(i = 0;i < *poolElementNodes;i++) {
         fprintf(stdlog, "#%u: ", (unsigned int)i + 1);
         ST_CLASS(poolElementNodePrint)(poolElementNodeArray[i],
                                        stdlog, PENPO_FULL);
      }
      fputs("\n", stdlog);
      LOG_END
      result = RSPERR_OKAY;
   }
   else {
      result = RSPERR_NOT_FOUND;
   }

   return(result);
}


/* ###### Do handle resolution for given pool handle ##################### */
unsigned int asapInstanceHandleResolution(
                struct ASAPInstance*               asapInstance,
                struct PoolHandle*                 poolHandle,
                struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                size_t*                            poolElementNodes)
{
   unsigned int result;
   const size_t originalPoolElementNodes = *poolElementNodes;

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE
   fputs("Trying handle resolution from cache...\n", stdlog);
   LOG_END

   result = asapInstanceHandleResolutionFromCache(
               asapInstance, poolHandle,
               poolElementNodeArray,
               poolElementNodes);
   if(result != RSPERR_OKAY) {
      LOG_VERBOSE
      fputs("No results in cache. Trying handle resolution at registrar...\n", stdlog);
      LOG_END

      /* The amount is now 0 (since handle resolution was not successful).
         Set it to its original value. */
      *poolElementNodes = originalPoolElementNodes;

      result = asapInstanceDoHandleResolution(asapInstance, poolHandle);
      if(result == RSPERR_OKAY) {
         result = asapInstanceHandleResolutionFromCache(
                     asapInstance, poolHandle,
                     poolElementNodeArray,
                     poolElementNodes);
      }
      else {
         LOG_VERBOSE
         fputs("Handle resolution not succesful\n", stdlog);
         LOG_END
      }
   }

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
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
   dispatcherUnlock(asapInstance->StateMachine);

   /* ====== Report unreachability ========================================== */
   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type       = AHT_ENDPOINT_UNREACHABLE;
      message->Flags      = 0x00;
      message->Handle     = *poolHandle;
      message->Identifier = identifier;
      result = asapInstanceSendRequest(asapInstance, message, false);
      if(result != RSPERR_OKAY) {
         LOG_VERBOSE2
         fprintf(stdlog, "Failed to send failure report for pool element $%08x of pool ",
               (unsigned int)identifier);
         poolHandlePrint(poolHandle, stdlog);
         fputs("\n", stdlog);
         LOG_END
         rserpoolMessageDelete(message);
      }
   }
   else {
      result = RSPERR_OUT_OF_MEMORY;
   }

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
   *timeout = -1;
   if(dispatcher != NULL) {
      dispatcherLock(dispatcher);

      /*  ====== Create fdset for poll() ================================= */
      *pollTimeStamp = getMicroTime();
      node = leafLinkedRedBlackTreeGetFirst(&dispatcher->FDCallbackStorage);
      while(node != NULL) {
         fdCallback = (struct FDCallback*)node;
         if(fdCallback->EventMask & (FDCE_Read|FDCE_Write|FDCE_Exception)) {
            fdCallback->SelectTimeStamp = *pollTimeStamp;
            ufds[*nfds].fd     = fdCallback->FD;
            ufds[*nfds].events = fdCallback->EventMask & (FDCE_Read|FDCE_Write|FDCE_Exception);
            (*nfds)++;
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
















/* ###### Handle endpoint keepalive ###################################### */
static void asapInstanceHandleEndpointKeepAlive(
               struct ASAPInstance*    asapInstance,
               struct RSerPoolMessage* message,
               int                     fd)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolNode)*        poolNode;
   unsigned int                      result;
   int                               sd;

   LOG_VERBOSE2
   fprintf(stdlog, "EndpointKeepAlive from registrar $%08x via assoc %u for pool ",
           message->RegistrarIdentifier, (unsigned int)message->AssocID);
   poolHandlePrint(&message->Handle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   /* Does message come via association on registrar *hunt* socket
      instead of peeled-off registrar socket? */
   if(fd == asapInstance->RegistrarHuntSocket) {
      if(message->Flags & AHF_ENDPOINT_KEEP_ALIVE_HOME) {
         LOG_NOTE
         fprintf(stdlog, "EndpointKeepAlive from $%08x (assoc %u) instead of home-registrar assoc $%08x -> replacing home registrar\n",
               message->RegistrarIdentifier,
               (unsigned int)message->AssocID,
               asapInstance->RegistrarIdentifier);
         LOG_END

         sd = registrarTablePeelOffRegistrarAssocID(asapInstance->RegistrarSet,
                                                    asapInstance->RegistrarHuntSocket,
                                                    message->AssocID);
         if(sd >= 0) {
            asapInstanceDisconnectFromRegistrar(asapInstance, true);
            if(asapInstanceConnectToRegistrar(asapInstance, sd) == true) {
               /* The new registrar ID is already known */
               asapInstance->RegistrarIdentifier          = message->RegistrarIdentifier;
               asapInstance->RegistrarConnectionTimeStamp = getMicroTime();
            }
         }
         else {
            LOG_ERROR
            logerror("sctp_peeloff() for incoming registrar association from new registrar failed");
            LOG_END
         }
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "EndpointKeepAlive from $%08x (assoc %u) instead of home-registrar assoc $%08x. The H bit is not set, therefore ignoring it.",
               message->RegistrarIdentifier,
               (unsigned int)message->AssocID,
               asapInstance->RegistrarIdentifier);
         LOG_END
      }
   }
   else {
      if(asapInstance->RegistrarIdentifier == UNDEFINED_REGISTRAR_IDENTIFIER) {
         asapInstance->RegistrarIdentifier = message->RegistrarIdentifier;
      }
   }


   /* ====== Send ENDPOINT_KEEP_ALIVE_ACK for each PE ==================== */
   poolNode = ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(&asapInstance->OwnPoolElements.Handlespace);
   while(poolNode != NULL) {
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(poolNode);
      while(poolElementNode != NULL) {

         message->Type       = AHT_ENDPOINT_KEEP_ALIVE_ACK;
         message->Flags      = 0x00;
         message->Identifier = poolElementNode->Identifier;

         LOG_VERBOSE2
         fprintf(stdlog, "Sending KeepAliveAck for pool element $%08x\n",message->Identifier);
         LOG_END
         result = rserpoolMessageSend(IPPROTO_SCTP,
                                      asapInstance->RegistrarSocket,
                                      0,
                                      MSG_NOSIGNAL,
                                      0,
                                      message);

         poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)(poolNode, poolElementNode);
      }

      poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(&asapInstance->OwnPoolElements.Handlespace, poolNode);
   }

   rserpoolMessageDelete(message);
}


/* ###### Handle response from registrar ################################# */
static void asapInstanceHandleResponseFromRegistrar(
               struct ASAPInstance*    asapInstance,
               struct RSerPoolMessage* response)
{
   struct ASAPInterThreadMessage* aitm;

   aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortDequeue(&asapInstance->MainLoopPort);
   if(aitm != NULL) {
      if( ((response->Type == AHT_REGISTRATION_RESPONSE)      && (aitm->Request->Type == AHT_REGISTRATION))   ||
          ((response->Type == AHT_DEREGISTRATION_RESPONSE)    && (aitm->Request->Type == AHT_DEREGISTRATION)) ||
          ((response->Type == AHT_HANDLE_RESOLUTION_RESPONSE) && (aitm->Request->Type == AHT_HANDLE_RESOLUTION)) ) {
         LOG_ACTION
         fprintf(stdlog, "Successfully got response ($%04x) for request ($%04x) from registrar\n",
                 response->Type, aitm->Request->Type);
         LOG_END
         aitm->Response = response;
         if(asapInstance->LastAITM == aitm) {
            puts("was LAST ATIM...");
            asapInstance->LastAITM = NULL;
         }
         interThreadMessageReply(&aitm->Node);
      }
      else {
         LOG_ERROR
         fprintf(stdlog, "Got unexpected response ($%04x) for request ($%04x) from registrar\n",
                 response->Type, aitm->Request->Type);
         LOG_END
         asapInstanceDisconnectFromRegistrar(asapInstance, true);
         rserpoolMessageDelete(response);
      }
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "Got unexpected response ($%04x) from registrar. Disconnecting.\n",
              response->Type);
      LOG_END
      asapInstanceDisconnectFromRegistrar(asapInstance, true);
      rserpoolMessageDelete(response);
   }
}


static void handleRegistrarConnectionEvent(
               struct Dispatcher* dispatcher,
               int                fd,
               unsigned int       eventMask,
               void*              userData)
{
   struct ASAPInstance*    asapInstance = (struct ASAPInstance*)userData;
   struct RSerPoolMessage* message;
   ssize_t                 received;
   union sockaddr_union    sourceAddress;
   socklen_t               sourceAddressLength;
   uint32_t                ppid;
   sctp_assoc_t            assocID;
   int                     flags;
   char                    buffer[65536];

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE2
   fputs("Entering Connection Handler...\n", stdlog);
   LOG_END

   if( ((fd == asapInstance->RegistrarHuntSocket) || (fd == asapInstance->RegistrarSocket)) &&
       (eventMask & (FDCE_Read|FDCE_Exception)) ) {
      flags               = 0;
      sourceAddressLength = sizeof(sourceAddress);
      received = recvfromplus(fd,
                              (char*)&buffer, sizeof(buffer),
                              &flags,
                              (struct sockaddr*)&sourceAddress, &sourceAddressLength,
                              &ppid, &assocID, NULL,
                              asapInstance->RegistrarResponseTimeout);
      if(received > 0) {
         /* ====== Handle notification ================================ */
         if(flags & MSG_NOTIFICATION) {
            if(fd == asapInstance->RegistrarSocket) {
               handleNotificationOnRegistrarSocket(asapInstance,
                                                   (union sctp_notification*)&buffer);
            }
            else if(fd == asapInstance->RegistrarHuntSocket) {
               registrarTableHandleNotificationOnRegistrarHuntSocket(asapInstance->RegistrarSet,
                                                                     asapInstance->RegistrarHuntSocket,
                                                                     (union sctp_notification*)&buffer);
            }
         }

         /* ====== Handle message ===================================== */
         else {
            if(rserpoolPacket2Message((char*)&buffer,
                                       &sourceAddress,
                                       assocID,
                                       ppid,
                                       received, sizeof(buffer),
                                       &message) == RSPERR_OKAY) {

               if(message->Type == AHT_ENDPOINT_KEEP_ALIVE) {
                  asapInstanceHandleEndpointKeepAlive(asapInstance, message, fd);
               }
               else {
                  asapInstanceHandleResponseFromRegistrar(asapInstance, message);
               }
            }
            else {
               LOG_WARNING
               fputs("Disconnecting from registrar due to failure\n", stdlog);
               LOG_END
               asapInstanceDisconnectFromRegistrar(asapInstance, true);
            }
         }
      }
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "Event for unknown socket %d\n", fd);
      LOG_END
   }

   LOG_VERBOSE2
   fputs("Leaving Connection Handler\n", stdlog);
   LOG_END

   dispatcherUnlock(asapInstance->StateMachine);
}


static void handleASAPInterThreadMessages(struct ASAPInstance* asapInstance)
{
   struct ASAPInterThreadMessage* aitm;
   struct ASAPInterThreadMessage* nextAITM;
   unsigned int                   result;

   asapInstanceConnectToRegistrar(asapInstance, -1);
   if(asapInstance->RegistrarSocket >= 0) {
      interThreadMessagePortLock(&asapInstance->MainLoopPort);
      if(asapInstance->LastAITM) {
         aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortGetNextMessage(&asapInstance->MainLoopPort, &asapInstance->LastAITM->Node);
      }
      else {
         aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortGetFirstMessage(&asapInstance->MainLoopPort);
      }
      while(aitm != NULL) {
printf("SEND-TYPE=%x\n",aitm->Request->Type);
         result = rserpoolMessageSend(IPPROTO_SCTP,
                                      asapInstance->RegistrarSocket,
                                      0,
                                      MSG_NOSIGNAL,
                                      asapInstance->RegistrarRequestTimeout,
                                      aitm->Request);
         if(result == false) {
            LOG_WARNING
            logerror("Failed to send message to registrar");
            LOG_END
            asapInstanceDisconnectFromRegistrar(asapInstance, true);
            break;
         }

         nextAITM = (struct ASAPInterThreadMessage*)interThreadMessagePortGetNextMessage(&asapInstance->MainLoopPort, &aitm->Node);
         if(!aitm->ResponseExpected) {
            interThreadMessagePortRemoveMessage(&asapInstance->MainLoopPort, &aitm->Node);
         }
         else  {
            asapInstance->LastAITM = aitm;
         }
         aitm = nextAITM;
      }
      interThreadMessagePortUnlock(&asapInstance->MainLoopPort);
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

static void* rsplibMainLoop(void* args)
{
   struct ASAPInstance*           asapInstance = (struct ASAPInstance*)args;

   char                 buffer[1];
   unsigned long long   pollTimeStamp;
   struct pollfd        ufds[FD_SETSIZE];
   unsigned int         nfds;
   int                  timeout;
   unsigned int         pipeIndex;
   int                  result;

   asapInstanceConnectToRegistrar(asapInstance, -1);

   while(!RsplibThreadStop) {
      /* ====== Schedule ================================================= */
      /* pthreads seem to have the property that scheduling is quite
         unfair -> If the main loop only invokes rspSelect(), this
         may block other threads forever => explicitly let other threads
         do their work now, then lock! */
      sched_yield();

      /* ====== Collect data for ext_select() call ======================= */
      dispatcherGetPollParameters(&gDispatcher,
                                  (struct pollfd*)&ufds, &nfds, &timeout, &pollTimeStamp);
      pipeIndex = nfds;
      ufds[pipeIndex].fd     = RsplibPipe[0];
      ufds[pipeIndex].events = POLLIN;
      nfds++;

      /* ====== Do ext_select() call ===================================== */
      result = ext_poll((struct pollfd*)&ufds, nfds, timeout);

      /* ====== Handle results =========================================== */
      dispatcherHandlePollResult(&gDispatcher, result,
                                 (struct pollfd*)&ufds, nfds, timeout, pollTimeStamp);
      if(ufds[pipeIndex].revents & POLLIN) {
         CHECK(ext_read(RsplibPipe[0], (char*)&buffer, sizeof(buffer)) > 0);
         puts("X");
      }

      /* ====== Handle inter-thread messages ============================= */
      handleASAPInterThreadMessages(asapInstance);
   }

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

   CHECK(ext_pipe((int*)&RsplibPipe) == 0);
   setNonBlocking(RsplibPipe[1]);
   pthread_create(&RsplibMainLoopThread, NULL, &rsplibMainLoop, gAsapInstance);


   puts("START!");
printf("PIPE: %d %d\n",RsplibPipe[0],RsplibPipe[1]);

   installBreakDetector();
   int i = 0x1000000;
   while(!breakDetected()) {
      // usleep(1000000);

      struct PoolHandle ph;
      poolHandleNew(&ph, (unsigned char*)"TestPool", 8);
      printf("REPORT.RESULT=%d\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
      printf("DEREG.RESULT=%d\n",asapInstanceDeregister(gAsapInstance, &ph, 1 + (i++)));
   }


   RsplibThreadStop = true;
puts("wrt...");
   ext_write(RsplibPipe[1], "X", 1);
puts("ok!");
   pthread_join(RsplibMainLoopThread, NULL);

   asapInstanceDelete(gAsapInstance);
   dispatcherDelete(&gDispatcher);
   finishLogging();
   return(0);
}

