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
 * Copyright (C) 2002-2009 by Thomas Dreibholz
 *
 * Acknowledgements:
 * Realized in co-operation between Siemens AG and
 * University of Essen, Institute of Computer Networking Technology.
 * This work was partially funded by the Bundesministerium fuer Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany
 * (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This program is free software: you can redistribute it 1113and/or modify
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
#include "rserpool-internals.h"
#include "loglevel.h"
#include "asapinstance.h"
#include "rserpoolmessage.h"
#include "asapinterthreadmessage.h"
#include "timeutilities.h"
#include "netutilities.h"

#include <ext_socket.h>


static void asapInstanceConfigure(struct ASAPInstance* asapInstance,
                                  struct TagItem*      tags);
static void asapInstanceNotifyMainLoop(struct ASAPInstance* asapInstance);
static void* asapInstanceMainLoop(void* args);
static void asapInstanceHandleRegistrarConnectionEvent(
               struct Dispatcher* dispatcher,
               int                fd,
               unsigned int       eventMask,
               void*              userData);
static void asapInstanceHandleEndpointKeepAlive(
               struct ASAPInstance*    asapInstance,
               struct RSerPoolMessage* message,
               int                     fd);
static void asapInstanceDisconnectFromRegistrar(
               struct ASAPInstance* asapInstance,
               bool                 sendAbort);
static void asapInstanceHandleRegistrarTimeout(struct Dispatcher* dispatcher,
                                               struct Timer*      timer,
                                               void*              userData);


/* ###### Constructor #################################################### */
struct ASAPInstance* asapInstanceNew(struct Dispatcher*          dispatcher,
                                     const bool                  enableAutoConfig,
                                     const union sockaddr_union* registrarAnnounceAddress,
                                     struct TagItem*             tags)
{
   struct ASAPInstance*        asapInstance = NULL;
   struct sctp_event_subscribe sctpEvents;
   int                         autoCloseTimeout;

   if(dispatcher != NULL) {
      asapInstance = (struct ASAPInstance*)malloc(sizeof(struct ASAPInstance));
      if(asapInstance != NULL) {
         /* ====== Initialize ASAP Instance structure ==================== */
         interThreadMessagePortNew(&asapInstance->MainLoopPort);
         asapInstance->StateMachine                 = dispatcher;
         asapInstance->MainLoopPipe[0]              = -1;
         asapInstance->MainLoopPipe[1]              = -1;
         asapInstance->MainLoopThread               = 0;
         asapInstance->MainLoopShutdown             = false;
         asapInstance->LastAITM                     = NULL;
         asapInstance->RegistrarMessageBuffer       = NULL;
         asapInstance->RegistrarHuntMessageBuffer   = NULL;
         asapInstance->RegistrarSet                 = NULL;
         asapInstance->RegistrarConnectionTimeStamp = 0;
         asapInstance->RegistrarHuntSocket          = -1;
         asapInstance->RegistrarSocket              = -1;
         asapInstance->RegistrarIdentifier          = 0;
         asapInstanceConfigure(asapInstance, tags);
         timerNew(&asapInstance->RegistrarTimeoutTimer,
                  asapInstance->StateMachine,
                  asapInstanceHandleRegistrarTimeout,
                  asapInstance);

         /* ====== Initialize PU-side cache and ownership management ===== */
         ST_CLASS(poolHandlespaceManagementNew)(&asapInstance->Cache,
                                                0x00000000,
                                                NULL, NULL, NULL);
         ST_CLASS(poolHandlespaceManagementNew)(&asapInstance->OwnPoolElements,
                                                0x00000000,
                                                NULL, NULL, NULL);

         /* ====== Initialize message buffers ============================ */
         asapInstance->RegistrarMessageBuffer = messageBufferNew(ASAP_BUFFER_SIZE, true);
         if(asapInstance->RegistrarMessageBuffer == NULL) {
            asapInstanceDelete(asapInstance);
            return(NULL);
         }
         asapInstance->RegistrarHuntMessageBuffer = messageBufferNew(ASAP_BUFFER_SIZE, false);
         if(asapInstance->RegistrarHuntMessageBuffer == NULL) {
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         /* ====== Initialize registrar table ============================ */
         asapInstance->RegistrarSet = registrarTableNew(asapInstance->StateMachine,
                                                        enableAutoConfig,
                                                        registrarAnnounceAddress, tags);
         if(asapInstance->RegistrarSet == NULL) {
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         /* ====== Registrar socket ====================================== */
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
                       asapInstanceHandleRegistrarConnectionEvent,
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
         /* sctpEvents.sctp_adaptation_layer_event = 1; */

         if(ext_setsockopt(asapInstance->RegistrarHuntSocket, IPPROTO_SCTP, SCTP_EVENTS, &sctpEvents, sizeof(sctpEvents)) < 0) {
            logerror("setsockopt() for SCTP_EVENTS on registrar hunt socket failed");
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         autoCloseTimeout = 30;
         if(ext_setsockopt(asapInstance->RegistrarHuntSocket, IPPROTO_SCTP, SCTP_AUTOCLOSE, &autoCloseTimeout, sizeof(autoCloseTimeout)) < 0) {
            logerror("setsockopt() for SCTP_AUTOCLOSE on registrar hunt socket failed");
            asapInstanceDelete(asapInstance);
            return(NULL);
         }

         /* ====== Initialize main loop ================================== */
         if(ext_pipe((int*)&asapInstance->MainLoopPipe) < 0) {
            logerror("pipe() failed");
            asapInstanceDelete(asapInstance);
            return(NULL);
         }
         setNonBlocking(asapInstance->MainLoopPipe[0]);
         setNonBlocking(asapInstance->MainLoopPipe[1]);
         /* The ASAP main loop thread cannot be started here, since the
            static registrar entries are still missing. The can be added
            later. After that, asapInstanceStartThread() has to be called. */
      }
   }
   return(asapInstance);
}


/* ###### Constructor #################################################### */
bool asapInstanceStartThread(struct ASAPInstance* asapInstance)
{
   if(pthread_create(&asapInstance->MainLoopThread, NULL, &asapInstanceMainLoop, asapInstance) != 0) {
      logerror("Unable to create ASAP main loop thread");
      return(false);
   }
   return(true);
}


/* ###### Destructor ##################################################### */
void asapInstanceDelete(struct ASAPInstance* asapInstance)
{
   struct ASAPInterThreadMessage* aitm;

   if(asapInstance) {
      if(asapInstance->MainLoopThread != 0) {
         dispatcherLock(asapInstance->StateMachine);
         asapInstance->MainLoopShutdown = true;
         dispatcherUnlock(asapInstance->StateMachine);
         asapInstanceNotifyMainLoop(asapInstance);
         CHECK(pthread_join(asapInstance->MainLoopThread, NULL) == 0);
         asapInstance->MainLoopThread = 0;
      }
      if(asapInstance->MainLoopPipe[0] >= 0) {
         ext_close(asapInstance->MainLoopPipe[0]);
         asapInstance->MainLoopPipe[0] = -1;
      }
      if(asapInstance->MainLoopPipe[1] >= 0) {
         ext_close(asapInstance->MainLoopPipe[1]);
         asapInstance->MainLoopPipe[1] = -1;
      }
      if(asapInstance->RegistrarHuntSocket >= 0) {
         fdCallbackDelete(&asapInstance->RegistrarHuntFDCallback);
         ext_close(asapInstance->RegistrarHuntSocket);
      }
      ST_CLASS(poolHandlespaceManagementDelete)(&asapInstance->OwnPoolElements);
      ST_CLASS(poolHandlespaceManagementDelete)(&asapInstance->Cache);
      if(asapInstance->RegistrarSet) {
         registrarTableDelete(asapInstance->RegistrarSet);
         asapInstance->RegistrarSet = NULL;
      }
      timerDelete(&asapInstance->RegistrarTimeoutTimer);

      /* There may still be AITM messages queued. Remove them first. */
      aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortDequeue(&asapInstance->MainLoopPort);
      while(aitm) {
         asapInterThreadMessageDelete(aitm);
         aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortDequeue(&asapInstance->MainLoopPort);
      }
      interThreadMessagePortDelete(&asapInstance->MainLoopPort);

      if(asapInstance->RegistrarMessageBuffer) {
         messageBufferDelete(asapInstance->RegistrarMessageBuffer);
         asapInstance->RegistrarMessageBuffer = NULL;
      }
      if(asapInstance->RegistrarHuntMessageBuffer) {
         messageBufferDelete(asapInstance->RegistrarHuntMessageBuffer);
         asapInstance->RegistrarHuntMessageBuffer = NULL;
      }

      free(asapInstance);
   }
}


/* ###### Get configuration from file #################################### */
static void asapInstanceConfigure(struct ASAPInstance* asapInstance,
                                  struct TagItem*      tags)
{
   /* ====== ASAP Instance settings ======================================= */
   asapInstance->RegistrarRequestMaxTrials = tagListGetData(tags, TAG_RspLib_RegistrarRequestMaxTrials,
                                                            ASAP_DEFAULT_REGISTRAR_REQUEST_MAXTRIALS);
   asapInstance->RegistrarRequestTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_RegistrarRequestTimeout,
                                                                              ASAP_DEFAULT_REGISTRAR_REQUEST_TIMEOUT);
   asapInstance->RegistrarResponseTimeout = (unsigned long long)tagListGetData(tags, TAG_RspLib_RegistrarResponseTimeout,
                                                                               ASAP_DEFAULT_REGISTRAR_RESPONSE_TIMEOUT);


   /* ====== Show results =================================================== */
   LOG_VERBOSE3
   fputs("New ASAP instance's configuration:\n", stdlog);
   fprintf(stdlog, "registrar.request.timeout     = %lluus\n", asapInstance->RegistrarRequestTimeout);
   fprintf(stdlog, "registrar.response.timeout    = %lluus\n", asapInstance->RegistrarResponseTimeout);
   fprintf(stdlog, "registrar.request.maxtrials   = %u\n",     (unsigned int)asapInstance->RegistrarRequestMaxTrials);
   LOG_END
}


/* ###### Connect to registrar ############################################ */
static bool asapInstanceConnectToRegistrar(struct ASAPInstance* asapInstance,
                                           int                  sd)
{
   RegistrarIdentifierType registrarIdentifier;
#ifdef HAVE_SCTP_DELAYED_SACK
   struct sctp_sack_info   sctpSACKInfo;
#endif

   if(asapInstance->RegistrarSocket < 0) {
      /* ====== Look for registrar, if no FD is given ==================== */
      if(sd < 0) {
         LOG_ACTION
         fputs("Starting registrar hunt...\n", stdlog);
         LOG_END
         if(sd < 0) {
            sd = registrarTableGetRegistrar(asapInstance->RegistrarSet,
                                            asapInstance->RegistrarHuntSocket,
                                            asapInstance->RegistrarHuntMessageBuffer,
                                            &registrarIdentifier);
         }
         dispatcherLock(asapInstance->StateMachine);
         asapInstance->RegistrarIdentifier = registrarIdentifier;
         dispatcherUnlock(asapInstance->StateMachine);
         if(sd < 0) {
            LOG_ACTION
            fputs("Unable to connect to a registrar\n", stdlog);
            LOG_END
            return(false);
         }
      }

      /* ====== Initialize new connection ================================ */
      asapInstance->RegistrarSocket              = sd;
      asapInstance->RegistrarConnectionTimeStamp = getMicroTime();
      fdCallbackNew(&asapInstance->RegistrarFDCallback,
                    asapInstance->StateMachine,
                    asapInstance->RegistrarSocket,
                    FDCE_Read|FDCE_Exception,
                    asapInstanceHandleRegistrarConnectionEvent,
                    (void*)asapInstance);
      asapInstance->LastAITM = NULL; /* Send requests again! */

      LOG_NOTE
      fprintf(stdlog, "Connected to registrar $%08x\n", asapInstance->RegistrarIdentifier);
      LOG_END

#ifdef HAVE_SCTP_DELAYED_SACK
      /* ====== Tune SACK handling ======================================= */
      sctpSACKInfo.sack_assoc_id = 0;
      sctpSACKInfo.sack_delay    = 0;
      sctpSACKInfo.sack_freq     = 1;
      if(ext_setsockopt(asapInstance->RegistrarSocket,
                        IPPROTO_SCTP, SCTP_DELAYED_SACK,
                        &sctpSACKInfo, sizeof(sctpSACKInfo)) < 0) {
         LOG_WARNING
         logerror("Unable to set SCTP_DELAYED_SACK");
         LOG_END
      }
#else
#warning SCTP_DELAYED_SACK/sctp_sack_info is not supported - Unable to tune SACK handling!
#endif
   }
   return(true);
}


/* ###### Disconnect from registrar #################################### */
static void asapInstanceDisconnectFromRegistrar(struct ASAPInstance* asapInstance,
                                                bool                 sendAbort)
{
   if(asapInstance->RegistrarSocket >= 0) {
      dispatcherLock(asapInstance->StateMachine);
      timerStop(&asapInstance->RegistrarTimeoutTimer);
      fdCallbackDelete(&asapInstance->RegistrarFDCallback);
      dispatcherUnlock(asapInstance->StateMachine);
      if(sendAbort) {
         sendabort(asapInstance->RegistrarSocket, 0);
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
   struct ASAPInterThreadMessage* aitm;

   aitm = asapInterThreadMessageNew(request, responseExpected);
   if(aitm == NULL) {
      return(RSPERR_OUT_OF_MEMORY);
   }

// ???   dispatcherLock(asapInstance->StateMachine);
   interThreadMessagePortEnqueue(&asapInstance->MainLoopPort, &aitm->Node, NULL);
// ???   dispatcherUnlock(asapInstance->StateMachine);
   asapInstanceNotifyMainLoop(asapInstance);
   return(RSPERR_OKAY);
}


/* ###### Send request to registrar ###################################### */
static unsigned int asapInstanceBeginIO(struct ASAPInstance*           asapInstance,
                                        struct InterThreadMessagePort* itmPort,
                                        struct RSerPoolMessage*        request)
{
   struct ASAPInterThreadMessage* aitm;

   aitm = asapInterThreadMessageNew(request, true);
   if(aitm == NULL) {
      return(RSPERR_OUT_OF_MEMORY);
   }

// ???   dispatcherLock(asapInstance->StateMachine);
   interThreadMessagePortEnqueue(&asapInstance->MainLoopPort, &aitm->Node, itmPort);
// ???   dispatcherUnlock(asapInstance->StateMachine);
   asapInstanceNotifyMainLoop(asapInstance);

   return(RSPERR_OKAY);
}


/* ###### Wait for response ############################################## */
static unsigned int asapInstanceWaitIO(struct ASAPInstance*           asapInstance,
                                       struct InterThreadMessagePort* itmPort,
                                       struct RSerPoolMessage*        request,
                                       struct RSerPoolMessage**       responsePtr)
{
   struct ASAPInterThreadMessage* aitm;
   unsigned int                   result;

   interThreadMessagePortWait(itmPort);
   aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortDequeue(itmPort);
   CHECK(aitm != NULL);
   *responsePtr = aitm->Response;
   result = aitm->Error;
   free(aitm);
   return(result);
}


/* ###### Send request to registrar and wait for response ############## */
static unsigned int asapInstanceDoIO(struct ASAPInstance*     asapInstance,
                                     struct RSerPoolMessage*  request,
                                     struct RSerPoolMessage** responsePtr)
{
   struct InterThreadMessagePort itmPort;
   unsigned int                  result;

   interThreadMessagePortNew(&itmPort);
   result = asapInstanceBeginIO(asapInstance, &itmPort, request);
   if(result == RSPERR_OKAY) {
      result = asapInstanceWaitIO(asapInstance, &itmPort, request, responsePtr);
   }
   interThreadMessagePortDelete(&itmPort);
   return(result);
}


/* ###### Register pool element ########################################## */
unsigned int asapInstanceRegister(struct ASAPInstance*              asapInstance,
                                  struct PoolHandle*                poolHandle,
                                  struct ST_CLASS(PoolElementNode)* poolElementNode,
                                  const bool                        waitForResponse,
                                  const bool                        daemonMode)
{
   struct RSerPoolMessage*           message;
   struct RSerPoolMessage*           response;
   struct ST_CLASS(PoolElementNode)* oldPoolElementNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct TransportAddressBlock*     newUserTransport;
   unsigned int                      result;

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
         result = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
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
         if(result == RSPERR_OKAY) {
            newPoolElementNode->UserData = (void*)asapInstance;
         }
         else {
            LOG_ERROR
            fprintf(stdlog, "Unable to register pool element $%08x of pool ",
                    poolElementNode->Identifier);
            poolHandlePrint(poolHandle, stdlog);
            fputs(" to OwnPoolElements\n", stdlog);
            LOG_END_FATAL
         }
      }
      dispatcherUnlock(asapInstance->StateMachine);

      /* ====== Send registration ======================================== */
      if(result == RSPERR_OKAY) {
         if(waitForResponse) {
            result = asapInstanceDoIO(asapInstance, message, &response);
            if(result == RSPERR_OKAY) {
               dispatcherLock(asapInstance->StateMachine);

               if( (response->Error == RSPERR_OKAY) && (!(response->Flags & AHF_REGISTRATION_REJECT)) ) {
                  if(response->Identifier != poolElementNode->Identifier) {
                     LOG_ERROR
                     fprintf(stdlog, "Tried to register PE $%08x, got response for PE $%08x\n",
                             poolElementNode->Identifier,
                             response->Identifier);
                     LOG_END
                  }
               }
               else {
                  result = (unsigned int)response->Error;
               }
               dispatcherUnlock(asapInstance->StateMachine);
               
               if(response) {
                  rserpoolMessageDelete(response);
               }
            }
            rserpoolMessageDelete(message);
         }
         else {
            /* Important: message->PoolElementPtr must be a copy of the PE
               data, since this thread may delete the supplied PE data when
               this function returns. */
            newPoolElementNode = (struct ST_CLASS(PoolElementNode)*)malloc(sizeof(struct ST_CLASS(PoolElementNode)));
            if(newPoolElementNode != NULL) {
               newUserTransport = transportAddressBlockDuplicate(message->PoolElementPtr->UserTransport);
               if(newUserTransport != NULL) {
                  ST_CLASS(poolElementNodeNew)(newPoolElementNode,
                                               message->PoolElementPtr->Identifier,
                                               message->PoolElementPtr->HomeRegistrarIdentifier,
                                               message->PoolElementPtr->RegistrationLife,
                                               &message->PoolElementPtr->PolicySettings,
                                               newUserTransport,
                                               NULL,
                                               -1, 0);
                  message->PoolElementPtr           = newPoolElementNode;
                  message->PoolElementPtrAutoDelete = true;
                  result = asapInstanceSendRequest(asapInstance, message, true);
               }
               else {
                  free(newPoolElementNode);
                  rserpoolMessageDelete(message);
                  result = RSPERR_OUT_OF_MEMORY;
               }
            }
            else {
               rserpoolMessageDelete(message);
               result = RSPERR_OUT_OF_MEMORY;
            }
         }
      }
      else {
         LOG_ERROR
         fputs("Incompatible pool element settings for reregistration!Old: \n", stderr);
         ST_CLASS(poolElementNodePrint)(oldPoolElementNode, stdlog, PENPO_FULL);
         fputs("New: \n", stderr);
         ST_CLASS(poolElementNodePrint)(message->PoolElementPtr, stdlog, PENPO_FULL);
         LOG_END
         rserpoolMessageDelete(message);
      }
   }
   else {
      result = RSPERR_OUT_OF_MEMORY;
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
                                    const PoolElementIdentifierType identifier,
                                    const bool                      waitForResponse)
{
   struct RSerPoolMessage* message;
   struct RSerPoolMessage* response;
   unsigned int            result;

   LOG_VERBOSE
   fprintf(stdlog, "Trying to deregister $%08x from pool ",identifier);
   poolHandlePrint(poolHandle, stdlog);
   fputs("\n", stdlog);
   LOG_END

   result = ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
                     &asapInstance->OwnPoolElements,
                     poolHandle,
                     identifier);
   if(result != RSPERR_OKAY) {
      LOG_ERROR
      fprintf(stdlog, "Unable to deregister pool element $%08x of pool ",
              identifier);
      poolHandlePrint(poolHandle, stdlog);
      fputs(" from OwnPoolElements\n", stdlog);
      LOG_END
   }
   else {
      message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
      if(message != NULL) {
         message->Type       = AHT_DEREGISTRATION;
         message->Flags      = 0x00;
         message->Handle     = *poolHandle;
         message->Identifier = identifier;

         if(waitForResponse) {
            result = asapInstanceDoIO(asapInstance, message, &response);
            if((result == RSPERR_OKAY) && (response->Error == RSPERR_OKAY)) {
               if(identifier != response->Identifier) {
                  LOG_ERROR
                  fprintf(stdlog, "Tried to deregister PE $%08x, got response for PE $%08x\n",
                          identifier,
                          message->Identifier);
                  LOG_END_FATAL
               }

               if(response) {
                  rserpoolMessageDelete(response);
               }
            }
            rserpoolMessageDelete(message);
         }
         else {
            result = asapInstanceSendRequest(asapInstance, message, true);
         }
      }
      else {
         result = RSPERR_OUT_OF_MEMORY;
      }
   }

   LOG_VERBOSE
   fputs("Deregistration result is: ", stdlog);
   rserpoolErrorPrint(result, stdlog);
   fputs("\n", stdlog);
   LOG_END
   return(result);
}


/* ###### Do name lookup from cache ###################################### */
static unsigned int asapInstanceHandleResolutionFromCache(
                       struct ASAPInstance*               asapInstance,
                       struct PoolHandle*                 poolHandle,
                       void**                             nodePtrArray,
                       struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                       size_t*                            poolElementNodes,
                       unsigned int                       (*convertFunction)(const struct ST_CLASS(PoolElementNode)* poolElementNode,
                                                                             void*                                   ptr),
                       const bool                         purgeOutOfDateElements)
{
   unsigned int result;
   size_t       i;

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE
   fprintf(stdlog, "Handle Resolution for pool ");
   poolHandlePrint(poolHandle, stdlog);
   fputs(":\n", stdlog);
   ST_CLASS(poolHandlespaceManagementPrint)(&asapInstance->Cache,
                                            stdlog, PENPO_ONLY_POLICY);
   LOG_END

   if(purgeOutOfDateElements) {
      i = ST_CLASS(poolHandlespaceManagementPurgeExpiredPoolElements)(
            &asapInstance->Cache,
            getMicroTime());
      LOG_VERBOSE
      fprintf(stdlog, "Purged %u out-of-date elements\n", (unsigned int)i);
      LOG_END
   }

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
      for(i = 0;i < *poolElementNodes;i++) {
          if(convertFunction(poolElementNodeArray[i], &nodePtrArray[i]) != 0) {
             result = RSPERR_OUT_OF_MEMORY;
          }
      }
      if(result != RSPERR_OKAY) {
         for(i = 0;i < *poolElementNodes;i++) {
            free(nodePtrArray[i]);
            nodePtrArray[i] = 0;
         }
         *poolElementNodes = 0;
      }
   }
   else {
      result = RSPERR_NOT_FOUND;
   }

   dispatcherUnlock(asapInstance->StateMachine);
   return(result);
}


/* ###### Do name lookup ################################################# */
static unsigned int asapInstanceHandleResolutionAtRegistrar(struct ASAPInstance*               asapInstance,
                                                            struct PoolHandle*                 poolHandle,
                                                            void**                             nodePtrArray,
                                                            struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                                                            size_t*                            poolElementNodes,
                                                            unsigned int                       (*convertFunction)(const struct ST_CLASS(PoolElementNode)* poolElementNode,
                                                                                                                  void*                                   ptr),
                                                            const unsigned long long           cacheElementTimeout)
{
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   struct RSerPoolMessage*           message;
   struct RSerPoolMessage*           response;
   unsigned int                      result;
   size_t                            i;

   message = rserpoolMessageNew(NULL, ASAP_BUFFER_SIZE);
   if(message != NULL) {
      message->Type      = AHT_HANDLE_RESOLUTION;
      message->Flags     = 0x00;
      message->Handle    = *poolHandle;
      message->Addresses = ((*poolElementNodes != RSPGETADDRS_MAX) && (cacheElementTimeout > 0)) ? 0 : *poolElementNodes;

      result = asapInstanceDoIO(asapInstance, message, &response);
      if(result == RSPERR_OKAY) {
         if(response->Error == RSPERR_OKAY) {
            LOG_VERBOSE
            fprintf(stdlog, "Got %u elements in handle resolution response\n",
                    (unsigned int)response->PoolElementPtrArraySize);
            LOG_END

            dispatcherLock(asapInstance->StateMachine);

            /* ====== Propagate results into PU-side cache =============== */
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
                  cacheElementTimeout);
            }

            /* ====== Select PEs from cache ============================== */
            result = asapInstanceHandleResolutionFromCache(
                        asapInstance, poolHandle,
                        nodePtrArray,
                        poolElementNodeArray,
                        poolElementNodes, convertFunction, false);

            dispatcherUnlock(asapInstance->StateMachine);
            /* =========================================================== */

         }
         else {
            LOG_VERBOSE2
            fprintf(stdlog, "Handle Resolution at registrar for pool ");
            poolHandlePrint(poolHandle, stdlog);
            fputs(" failed: ", stdlog);
            rserpoolErrorPrint(response->Error, stdlog);
            fputs("\n", stdlog);
            LOG_END
            result = response->Error;
         }
         rserpoolMessageDelete(response);
      }
      else {
         LOG_VERBOSE2
         fprintf(stdlog, "Handle Resolution at registrar for pool ");
         poolHandlePrint(poolHandle, stdlog);
         fputs(" failed: ", stdlog);
         rserpoolErrorPrint(result, stdlog);
         fputs("\n", stdlog);
         LOG_END
      }
      rserpoolMessageDelete(message);
   }
   else {
      result = RSPERR_OUT_OF_MEMORY;
   }

   return(result);
}


/* ###### Do handle resolution for given pool handle ##################### */
#define HRES_POOL_ELEMENT_NODE_ARRAY_SIZE 1024
unsigned int asapInstanceHandleResolution(
                struct ASAPInstance*     asapInstance,
                struct PoolHandle*       poolHandle,
                void**                   nodePtrArray,
                size_t*                  nodePtrs,
                unsigned int             (*convertFunction)(const struct ST_CLASS(PoolElementNode)* poolElementNode,
                                                            void*                                   ptr),
                const unsigned long long cacheElementTimeout)
{
   struct ST_CLASS(PoolElementNode)* poolElementNodeArray[HRES_POOL_ELEMENT_NODE_ARRAY_SIZE];
   const size_t                      originalPoolElementNodes = min(HRES_POOL_ELEMENT_NODE_ARRAY_SIZE, *nodePtrs);
   unsigned int                      result;

   LOG_VERBOSE
   fputs("Trying handle resolution from cache...\n", stdlog);
   LOG_END

   *nodePtrs = originalPoolElementNodes;
   result = asapInstanceHandleResolutionFromCache(
               asapInstance, poolHandle,
               nodePtrArray,
               (struct ST_CLASS(PoolElementNode)**)&poolElementNodeArray,
               nodePtrs, convertFunction, true);
   if(result != RSPERR_OKAY) {
      LOG_VERBOSE
      fputs("No results in cache. Trying handle resolution at registrar...\n", stdlog);
      LOG_END

      /* The amount is now 0 (since handle resolution was not successful).
         Set it to its original value. */
      *nodePtrs = originalPoolElementNodes;

      result = asapInstanceHandleResolutionAtRegistrar(
                  asapInstance, poolHandle,
                  nodePtrArray,
                  (struct ST_CLASS(PoolElementNode)**)&poolElementNodeArray,
                  nodePtrs, convertFunction,
                  cacheElementTimeout);
      if(result != RSPERR_OKAY) {
         LOG_VERBOSE
         fputs("Handle resolution not succesful\n", stdlog);
         LOG_END
      }
   }

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
         fprintf(stdlog, "EndpointKeepAlive from $%08x (assoc %u) instead of home registrar assoc $%08x -> replacing home registrar\n",
               message->RegistrarIdentifier,
               (unsigned int)message->AssocID,
               asapInstance->RegistrarIdentifier);
         LOG_END

         sd = registrarTablePeelOffRegistrarAssocID(asapInstance->RegistrarSet,
                                                    asapInstance->RegistrarHuntSocket,
                                                    asapInstance->RegistrarHuntMessageBuffer,
                                                    message->AssocID);
         if(sd >= 0) {
            asapInstanceDisconnectFromRegistrar(asapInstance, true);
            if(asapInstanceConnectToRegistrar(asapInstance, sd) == true) {
               /* The new registrar ID is already known */
               asapInstance->RegistrarIdentifier = message->RegistrarIdentifier;
               LOG_NOTE
               fprintf(stdlog, "Home registrar is $%08x\n", asapInstance->RegistrarIdentifier);
               LOG_END
            }
         }
         else {
            LOG_ERROR
            logerror("Peel-off for incoming registrar association from new registrar failed");
            LOG_END
         }
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "EndpointKeepAlive from $%08x (assoc %u) instead of home-registrar assoc $%08x. The H bit is not set, therefore ignoring it.\n",
               message->RegistrarIdentifier,
               (unsigned int)message->AssocID,
               asapInstance->RegistrarIdentifier);
         LOG_END
      }
   }
   else {
      if(asapInstance->RegistrarIdentifier == UNDEFINED_REGISTRAR_IDENTIFIER) {
         asapInstance->RegistrarIdentifier = message->RegistrarIdentifier;
         LOG_NOTE
         fprintf(stdlog, "Home registrar is $%08x\n", asapInstance->RegistrarIdentifier);
         LOG_END
      }
   }


   /* ====== Send ENDPOINT_KEEP_ALIVE_ACK ================================ */
   if(message->Identifier) {
      poolElementNode = ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                           &asapInstance->OwnPoolElements,
                           &message->Handle,
                           message->Identifier);
      if(poolElementNode) {
         message->Type       = AHT_ENDPOINT_KEEP_ALIVE_ACK;
         message->Flags      = 0x00;
         message->Identifier = poolElementNode->Identifier;

         LOG_VERBOSE2
         fprintf(stdlog, "Sending KeepAliveAck for pool element $%08x\n",message->Identifier);
         LOG_END
         result = rserpoolMessageSend(IPPROTO_SCTP,
                                    asapInstance->RegistrarSocket,
                                    0, 0, 0, 0,
                                    message);
      }
      else {
         LOG_WARNING
         fprintf(stdlog, "KeepAlive for unknown pool element $%08x -> ignoring it\n",message->Identifier);
         LOG_END
      }
   }
   else {
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
                                         0, 0, 0, 0,
                                         message);

            poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)(poolNode, poolElementNode);
         }

         poolNode = ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(&asapInstance->OwnPoolElements.Handlespace, poolNode);
      }
   }

   rserpoolMessageDelete(message);
}


/* ###### Handle response from registrar ################################# */
static void asapInstanceHandleResponseFromRegistrar(
               struct ASAPInstance*    asapInstance,
               struct RSerPoolMessage* response)
{
   struct ASAPInterThreadMessage* aitm;
   struct ASAPInterThreadMessage* nextAITM;

   aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortDequeue(&asapInstance->MainLoopPort);
   if(aitm != NULL) {
      /* No timeout occurred -> stop timeout timer */
      timerStop(&asapInstance->RegistrarTimeoutTimer);

      if( ((response->Type == AHT_REGISTRATION_RESPONSE)      && (aitm->Request->Type == AHT_REGISTRATION))   ||
          ((response->Type == AHT_DEREGISTRATION_RESPONSE)    && (aitm->Request->Type == AHT_DEREGISTRATION)) ||
          ((response->Type == AHT_HANDLE_RESOLUTION_RESPONSE) && (aitm->Request->Type == AHT_HANDLE_RESOLUTION)) ) {

         LOG_VERBOSE
         fprintf(stdlog, "Successfully got response ($%04x) for request ($%04x) from registrar\n"
                         "RTT %lluus, queuing delay %lluus\n",
                 response->Type, aitm->Request->Type,
                 getMicroTime() - aitm->CreationTimeStamp,
                 aitm->TransmissionTimeStamp - aitm->CreationTimeStamp);
         LOG_END
         
         /* Asynchronous message: print errors here! */
         if(aitm->Node.ReplyPort == NULL) {
            if( (response->Type == AHT_REGISTRATION_RESPONSE) &&
                ((response->Error != RSPERR_OKAY) || (response->Flags & AHF_REGISTRATION_REJECT)) ) {
               LOG_ERROR
               fprintf(stdlog, "Unable to register pool element $%08x of pool ",
                       response->Identifier);
               poolHandlePrint(&response->Handle, stdlog);
               fputs(": ", stdlog);
               rserpoolErrorPrint(response->Error, stdlog);
               fputs("\n", stdlog);
               LOG_END
               
               /* If no addresses are usable, try to re-connect. May be, the
                  host has got some usable addresses then. */
               if( (response->Error == RSPERR_NO_USABLE_USER_ADDRESSES) ||
                   (response->Error == RSPERR_NO_USABLE_ASAP_ADDRESSES) ) {
                  LOG_NOTE
                  fputs("Trying to re-connect to a registrar ...\n", stdlog);   
                  LOG_END
                  asapInstanceDisconnectFromRegistrar(asapInstance, true);
               }
            }
         }
      }
      else {
         if(response->Type == AHT_ERROR) {
            LOG_ERROR
            fprintf(stdlog, "Got ASAP_ERROR ($%04x) for request ($%04x) from registrar: ",
                    response->OperationErrorCode, aitm->Request->Type);
            rserpoolErrorPrint(response->OperationErrorCode, stdlog);
            fputs("\n", stdlog);
            LOG_END
         }
         else {
            LOG_ERROR
            fprintf(stdlog, "Got unexpected response ($%04x) for request ($%04x) from registrar\n",
                    response->Type, aitm->Request->Type);
            LOG_END
         }
         asapInstanceDisconnectFromRegistrar(asapInstance, true);
      }

      aitm->Response = response;
      if(asapInstance->LastAITM == aitm) {
         /* No more responses are expected. */
         asapInstance->LastAITM = NULL;
      }
      if(aitm->Node.ReplyPort) {
          /* Reply to requesting thread, if reply is desired. */
         interThreadMessageReply(&aitm->Node);
      }
      else {
         /* Sender is not interested in result, throw it away. */
         asapInterThreadMessageDelete(aitm);
      }

      /* Schedule next response's timeout */
      dispatcherLock(asapInstance->StateMachine);
      interThreadMessagePortLock(&asapInstance->MainLoopPort);
      if(asapInstance->LastAITM != NULL) {
         nextAITM = (struct ASAPInterThreadMessage*)interThreadMessagePortGetFirstMessage(
                                                       &asapInstance->MainLoopPort);
         CHECK(nextAITM != NULL);
         timerStart(&asapInstance->RegistrarTimeoutTimer,
                    nextAITM->ResponseTimeoutTimeStamp);
      }
      interThreadMessagePortUnlock(&asapInstance->MainLoopPort);
      dispatcherUnlock(asapInstance->StateMachine);
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


/* ###### Handle SCTP notification on registrar socket ################### */
static void handleNotificationOnRegistrarSocket(struct ASAPInstance*           asapInstance,
                                                const union sctp_notification* notification)
{
   if( (notification->sn_header.sn_type == SCTP_ASSOC_CHANGE) &&
       ((notification->sn_assoc_change.sac_state == SCTP_COMM_LOST) ||
        (notification->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP)) ) {
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


/* ###### Handle event on registrar connection ########################### */
static void asapInstanceHandleRegistrarConnectionEvent(
               struct Dispatcher* dispatcher,
               int                fd,
               unsigned int       eventMask,
               void*              userData)
{
   struct ASAPInstance*    asapInstance = (struct ASAPInstance*)userData;
   struct MessageBuffer*   messageBuffer;
   struct RSerPoolMessage* message;
   ssize_t                 received;
   union sockaddr_union    sourceAddress;
   socklen_t               sourceAddressLength;
   uint32_t                ppid;
   sctp_assoc_t            assocID;
   int                     flags;

   dispatcherLock(asapInstance->StateMachine);

   LOG_VERBOSE2
   fputs("Entering Connection Handler...\n", stdlog);
   LOG_END

   if(fd == asapInstance->RegistrarHuntSocket) {
      messageBuffer = asapInstance->RegistrarHuntMessageBuffer;
   }
   else if( (fd == asapInstance->RegistrarSocket) &&
            (eventMask & (FDCE_Read|FDCE_Exception)) ) {
      messageBuffer = asapInstance->RegistrarMessageBuffer;
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "Event for unknown socket %d\n", fd);
      LOG_END_FATAL
      return;   /* Avoids warning about uninitializes message buffer. */
   }

   flags               = 0;
   sourceAddressLength = sizeof(sourceAddress);
   received = messageBufferRead(messageBuffer,
                                fd, &flags,
                                (struct sockaddr*)&sourceAddress, &sourceAddressLength,
                                &ppid, &assocID, NULL,
                                asapInstance->RegistrarResponseTimeout);
   if(received > 0) {
      /* ====== Handle notification ================================ */
      if(flags & MSG_NOTIFICATION) {
         if(fd == asapInstance->RegistrarSocket) {
            handleNotificationOnRegistrarSocket(asapInstance,
                                                (union sctp_notification*)messageBuffer->Buffer);
         }
         else if(fd == asapInstance->RegistrarHuntSocket) {
            registrarTableHandleNotificationOnRegistrarHuntSocket(asapInstance->RegistrarSet,
                                                                  asapInstance->RegistrarHuntSocket,
                                                                  asapInstance->RegistrarHuntMessageBuffer,
                                                                  (union sctp_notification*)messageBuffer->Buffer);
         }
      }

      /* ====== Handle message ===================================== */
      else {
         if(rserpoolPacket2Message(messageBuffer->Buffer,
                                   &sourceAddress,
                                   assocID,
                                   ppid,
                                   received, messageBuffer->BufferSize,
                                   &message) == RSPERR_OKAY) {

            if(message->Type == AHT_ENDPOINT_KEEP_ALIVE) {
               asapInstanceHandleEndpointKeepAlive(asapInstance, message, fd);
            }
            else {
               /* Handle registrar's response */
               asapInstanceHandleResponseFromRegistrar(asapInstance, message);
            }
         }
         else {
            if(fd == asapInstance->RegistrarSocket) {
               LOG_WARNING
               fputs("Disconnecting from registrar due to failure\n", stdlog);
               LOG_END
               asapInstanceDisconnectFromRegistrar(asapInstance, true);
            }
            else {
               LOG_WARNING
               fputs("Got crap on registrar hunt socket -> sending abort\n", stdlog);
               LOG_END
               sendabort(fd, assocID);
            }
         }
      }
   }
   else if(received != MBRead_Partial) {
      LOG_WARNING
      fputs("Disconnecting from registrar due to disconnect\n", stdlog);
      LOG_END
      asapInstanceDisconnectFromRegistrar(asapInstance, true);
   }

   LOG_VERBOSE2
   fputs("Leaving Connection Handler\n", stdlog);
   LOG_END

   dispatcherUnlock(asapInstance->StateMachine);
}


/* ###### Handle registrar request timeout ############################### */
static void asapInstanceHandleRegistrarTimeout(struct Dispatcher* dispatcher,
                                               struct Timer*      timer,
                                               void*              userData)
{
   struct ASAPInstance* asapInstance = (struct ASAPInstance*)userData;

   CHECK(asapInstance->LastAITM != NULL);
   LOG_WARNING
   fputs("Request(s) to registrar timed out!\n", stdlog);
   LOG_END

   asapInstance->LastAITM = NULL;
   asapInstanceDisconnectFromRegistrar(asapInstance, true);
}


/* ###### Handle ASAP inter-thread message ############################### */
static void asapInstanceHandleQueuedAITMs(struct ASAPInstance* asapInstance)
{
   struct ASAPInterThreadMessage* aitm;
   struct ASAPInterThreadMessage* nextAITM;
   unsigned int                   result;

   /* ====== Get next AITM =============================================== */
   interThreadMessagePortLock(&asapInstance->MainLoopPort);
   if(asapInstance->LastAITM) {
      aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortGetNextMessage(
                                                &asapInstance->MainLoopPort,
                                                &asapInstance->LastAITM->Node);
   }
   else {
      aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortGetFirstMessage(&asapInstance->MainLoopPort);
   }

   /* ====== If there is an AITM, connect to registrar if necessary ====== */
   if( (asapInstance->RegistrarSocket < 0) &&
       ( (aitm) || ((ST_CLASS(poolHandlespaceManagementGetPoolElements)(&asapInstance->OwnPoolElements) > 0)) ) ) {
      interThreadMessagePortUnlock(&asapInstance->MainLoopPort);
      asapInstanceConnectToRegistrar(asapInstance, -1);
      interThreadMessagePortLock(&asapInstance->MainLoopPort);
   }

   /* ====== Handle AITMs ================================================ */
   while(aitm != NULL) {
      nextAITM = (struct ASAPInterThreadMessage*)interThreadMessagePortGetNextMessage(&asapInstance->MainLoopPort, &aitm->Node);
      aitm->TransmissionTrials++;

      /* ====== Reply error, when trials have exceeded =================== */
      if(aitm->TransmissionTrials > asapInstance->RegistrarRequestMaxTrials) {
         LOG_WARNING
         fputs("Maximum number of transmission trials reached\n", stdlog);
         LOG_END
         interThreadMessagePortRemoveMessage(&asapInstance->MainLoopPort, &aitm->Node);
         if(aitm->Node.ReplyPort) {
            if(asapInstance->RegistrarSocket < 0) {
               aitm->Error = RSPERR_NO_REGISTRAR;
            }
            else {
               aitm->Error = RSPERR_TIMEOUT;
            }
            interThreadMessageReply(&aitm->Node);
         }
         else {
            asapInterThreadMessageDelete(aitm);
         }
      }

      /* ====== Send to registrar ======================================== */
      else {
         if(asapInstance->RegistrarSocket >= 0) {
            LOG_VERBOSE
            fputs("Sending message to registrar ...\n", stdlog);
            LOG_END
            result = rserpoolMessageSend(IPPROTO_SCTP,
                                         asapInstance->RegistrarSocket,
                                         0, 0, 0,
                                         asapInstance->RegistrarRequestTimeout,
                                         aitm->Request);
            if(result == false) {
               LOG_WARNING
               logerror("Failed to send message to registrar");
               LOG_END
               interThreadMessagePortUnlock(&asapInstance->MainLoopPort);
               asapInstanceDisconnectFromRegistrar(asapInstance, true);
               interThreadMessagePortLock(&asapInstance->MainLoopPort);
               break;
            }
            aitm->TransmissionTimeStamp = getMicroTime();

            if(!aitm->ResponseExpected) {
               /* No response from registrar expected. */
               interThreadMessagePortRemoveMessage(&asapInstance->MainLoopPort, &aitm->Node);
               asapInterThreadMessageDelete(aitm);
            }
            else  {
               asapInstance->LastAITM = aitm;

               /* Schedule timeout */
               aitm->ResponseTimeoutTimeStamp       = getMicroTime() + asapInstance->RegistrarResponseTimeout;
               aitm->ResponseTimeoutNeedsScheduling = true;
               // NOTE:
               // The timer cannot be scheduled here, because a lock for
               // asapInstance->StateMachine is required to do this.
               // asapInstance->StateMachine cannot be locked here, since
               // asapInstance->MainLoopPort is already locked. Trying to
               // lock asapInstance->MainLoopPort would violate the locking
               // order!
            }
         }
      }

      aitm = nextAITM;
   }
   interThreadMessagePortUnlock(&asapInstance->MainLoopPort);


   /* ====== Set timeout ================================================= */
   dispatcherLock(asapInstance->StateMachine);
   interThreadMessagePortLock(&asapInstance->MainLoopPort);
   aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortGetFirstMessage(&asapInstance->MainLoopPort);
   while(aitm != NULL) {
      if( (aitm->ResponseExpected) && (aitm->ResponseTimeoutNeedsScheduling) ) {
         if(!timerIsRunning(&asapInstance->RegistrarTimeoutTimer)) {
            // Now, the AITM's timer can be scheduled, since asapInstance->StateMachine
            // and asapInstance->MainLoopPort are locked.
            timerStart(&asapInstance->RegistrarTimeoutTimer,
                       aitm->ResponseTimeoutTimeStamp);
         }
      }
      aitm->ResponseTimeoutNeedsScheduling = false;         
      aitm = (struct ASAPInterThreadMessage*)interThreadMessagePortGetNextMessage(&asapInstance->MainLoopPort, &aitm->Node);
   }
   interThreadMessagePortUnlock(&asapInstance->MainLoopPort);
   dispatcherUnlock(asapInstance->StateMachine);
}


/* ###### Wake up main loop thread using pipe ############################ */
static void asapInstanceNotifyMainLoop(struct ASAPInstance* asapInstance)
{
   ext_write(asapInstance->MainLoopPipe[1], "!", 1);
}


/* ###### Check whether ASAP instance is shutting down ################### */
static bool asapInstanceIsShuttingDown(struct ASAPInstance* asapInstance)
{
   dispatcherLock(asapInstance->StateMachine);
   const bool isShuttingDown = asapInstance->MainLoopShutdown;
   dispatcherUnlock(asapInstance->StateMachine);
   return(isShuttingDown);
}


/* ###### ASAP Instance main loop thread ################################# */
static void* asapInstanceMainLoop(void* args)
{
   struct ASAPInstance* asapInstance = (struct ASAPInstance*)args;
   char                 buffer[128];
   unsigned long long   pollTimeStamp;
   struct pollfd        ufds[FD_SETSIZE];
   unsigned int         nfds;
   int                  timeout;
   unsigned int         pipeIndex;
   int                  result;

   asapInstanceConnectToRegistrar(asapInstance, -1);

   while(!asapInstanceIsShuttingDown(asapInstance)) {
      /* ====== Collect data for ext_select() call ======================= */
      dispatcherGetPollParameters(asapInstance->StateMachine,
                                  (struct pollfd*)&ufds, &nfds, &timeout,
                                  &pollTimeStamp);
      pipeIndex = nfds;
      ufds[pipeIndex].fd      = asapInstance->MainLoopPipe[0];
      ufds[pipeIndex].events  = POLLIN;
      ufds[pipeIndex].revents = 0;
      if(!interThreadMessagePortIsFirstMessage(&asapInstance->MainLoopPort,
                                               &asapInstance->LastAITM->Node)) {
         /* First message in AITM queue is not LastAITM:
            There are new AITM messages to be handled.
            Do not block if there are no socket events! */
         timeout = 0;
      }

      /* ====== Call ext_poll ============================================ */
      result = ext_poll((struct pollfd*)&ufds, nfds + 1, timeout);

      /* ====== Handle results =========================================== */
      dispatcherHandlePollResult(asapInstance->StateMachine, result,
                                 (struct pollfd*)&ufds, nfds, timeout, pollTimeStamp);
      if(ufds[pipeIndex].revents & POLLIN) {
         CHECK(ext_read(asapInstance->MainLoopPipe[0], (char*)&buffer, sizeof(buffer)) > 0);
      }

      /* ====== Handle inter-thread messages ============================= */
      asapInstanceHandleQueuedAITMs(asapInstance);
   }

   asapInstanceDisconnectFromRegistrar(asapInstance, false);
   return(NULL);
}
