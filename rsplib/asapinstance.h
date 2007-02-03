/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium fr Bildung und
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 */

#ifndef ASAPINSTANCE_H
#define ASAPINSTANCE_H

#include "tdtypes.h"
#include "dispatcher.h"
#include "tagitem.h"
#include "messagebuffer.h"
#include "rserpoolmessage.h"
#include "poolhandlespacemanagement.h"
#include "registrartable.h"
#include "interthreadmessageport.h"


#ifdef __cplusplus
extern "C" {
#endif


struct ASAPInterThreadMessage;

struct ASAPInstance
{
   struct Dispatcher*                         StateMachine;

   struct InterThreadMessagePort              MainLoopPort;
   int                                        MainLoopPipe[2];
   pthread_t                                  MainLoopThread;
   bool                                       MainLoopShutdown;
   struct ASAPInterThreadMessage*             LastAITM;

   int                                        RegistrarHuntSocket;
   struct MessageBuffer*                      RegistrarHuntMessageBuffer;
   int                                        RegistrarSocket;
   struct MessageBuffer*                      RegistrarMessageBuffer;
   RegistrarIdentifierType                    RegistrarIdentifier;
   unsigned long long                         RegistrarConnectionTimeStamp;

   struct RegistrarTable*                     RegistrarSet;
   struct ST_CLASS(PoolHandlespaceManagement) Cache;
   struct ST_CLASS(PoolHandlespaceManagement) OwnPoolElements;

   unsigned long long                         CacheElementTimeout;

   struct FDCallback                          RegistrarHuntFDCallback;
   struct FDCallback                          RegistrarFDCallback;
   struct Timer                               RegistrarTimeoutTimer;

   size_t                                     RegistrarRequestMaxTrials;
   unsigned long long                         RegistrarRequestTimeout;
   unsigned long long                         RegistrarResponseTimeout;
};


#define ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT                     0
#define ASAP_DEFAULT_REGISTRAR_ANNOUNCE_ADDRESS "239.0.0.1:3863"
#define ASAP_DEFAULT_REGISTRAR_ANNOUNCE_TIMEOUT          5000000
#define ASAP_DEFAULT_REGISTRAR_CONNECT_MAXTRIALS               3
#define ASAP_DEFAULT_REGISTRAR_CONNECT_TIMEOUT           2500000
#define ASAP_DEFAULT_REGISTRAR_REQUEST_MAXTRIALS               1
#define ASAP_DEFAULT_REGISTRAR_REQUEST_TIMEOUT           2500000
#define ASAP_DEFAULT_REGISTRAR_RESPONSE_TIMEOUT          2500000

#define ASAP_BUFFER_SIZE                                   65536


/**
  * Constructor.
  *
  * @param dispatcher Dispatcher.
  * @param enableAutoConfig true to use multicast announces; false otherwise.
  * @param registrarAnnounceAddress Multicast address for PR announces.
  * @param tags TagItems.
  * @return ASAPInstance or NULL in case of error.
  */
struct ASAPInstance* asapInstanceNew(struct Dispatcher*          dispatcher,
                                     const bool                  enableAutoConfig,
                                     const union sockaddr_union* registrarAnnounceAddress,
                                     struct TagItem*             tags);

/**
  * Destructor.
  *
  * @param asapInstance ASAPInstance.
  */
void asapInstanceDelete(struct ASAPInstance* asapInstance);

/**
  * Register pool element.
  *
  * @param asapInstance ASAPInstance.
  * @param poolHandle Pool handle.
  * @param poolElement Pool Element.
  * @return RSPERR_OKAY in case of success; error code otherwise.
  */
unsigned int asapInstanceRegister(struct ASAPInstance*              asapInstance,
                                  struct PoolHandle*                poolHandle,
                                  struct ST_CLASS(PoolElementNode)* poolElementNode,
                                  const bool                        waitForResponse);

/**
  * Deregister pool element.
  *
  * @param asapInstance ASAPInstance.
  * @param poolHandle Pool handle.
  * @param identifier Pool element identifier.
  * @return RSPERR_OKAY in case of success; error code otherwise.
  */
unsigned int asapInstanceDeregister(
                struct ASAPInstance*            asapInstance,
                struct PoolHandle*              poolHandle,
                const PoolElementIdentifierType identifier,
                const bool                      waitForResponse);

/**
  * Report failure of pool element.
  *
  * @param asapInstance ASAPInstance.
  * @param poolHandle Pool handle.
  * @param identifier Pool element identifier.
  * @return RSPERR_OKAY in case of success; error code otherwise.
  */
unsigned int asapInstanceReportFailure(struct ASAPInstance*            asapInstance,
                               struct PoolHandle*              poolHandle,
                               const PoolElementIdentifierType identifier);

/**
  * Do name resolution of given pool handle. The resulting pool pointer
  * will be stored to the variable poolPtr.
  *
  * @param asapInstance ASAPInstance.
  * @param poolHandle Pool handle.
  * @param poolElementNodeArray Array to store pool element node pointers to.
  * @param poolElementNodes Reference to variable containing maximum amount of pool element nodes to obtain. After function call, this variable contains actual amount of pool element nodes obtained.
  * @return RSPERR_OKAY in case of success; error code otherwise.
  */
unsigned int asapInstanceHandleResolution(
                struct ASAPInstance*               asapInstanceInstance,
                struct PoolHandle*                 poolHandle,
                struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                size_t*                            poolElementNodes);


#ifdef __cplusplus
}
#endif


#endif
