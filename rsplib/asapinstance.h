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
 * Copyright (C) 2002-2011 by Thomas Dreibholz
 *
 * Acknowledgements:
 * Realized in co-operation between Siemens AG and
 * University of Essen, Institute of Computer Networking Technology.
 * This work was partially funded by the Bundesministerium fuer Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany
 * (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This program is free software: you can redistribute it and/or modify
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

   struct FDCallback                          RegistrarHuntFDCallback;
   struct FDCallback                          RegistrarFDCallback;
   struct Timer                               RegistrarTimeoutTimer;

   size_t                                     RegistrarRequestMaxTrials;
   unsigned long long                         RegistrarRequestTimeout;
   unsigned long long                         RegistrarResponseTimeout;
};


#define ASAP_DEFAULT_REGISTRAR_ANNOUNCE_ADDRESS ASAP_ANNOUNCE_MULTICAST_ADDRESS

#define ASAP_DEFAULT_REGISTRAR_ANNOUNCE_TIMEOUT          5000000
#define ASAP_DEFAULT_REGISTRAR_CONNECT_MAXTRIALS               1
#define ASAP_DEFAULT_REGISTRAR_CONNECT_TIMEOUT           7500000
#define ASAP_DEFAULT_REGISTRAR_REQUEST_MAXTRIALS               1
#define ASAP_DEFAULT_REGISTRAR_REQUEST_TIMEOUT           3000000
#define ASAP_DEFAULT_REGISTRAR_RESPONSE_TIMEOUT          3000000

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
  * Start ASAP main loop thread.
  *
  * @param asapInstance ASAPInstance.
  */
bool asapInstanceStartThread(struct ASAPInstance* asapInstance);

/**
  * Register pool element.
  *
  * @param asapInstance ASAPInstance.
  * @param poolHandle Pool handle.
  * @param poolElement Pool Element.
  * @param waitForResponse Set to true to wait for registrar's response.
  * @param daemonMode Set to true to use daemon mode.
  * @return RSPERR_OKAY in case of success; error code otherwise.
  */
unsigned int asapInstanceRegister(struct ASAPInstance*              asapInstance,
                                  struct PoolHandle*                poolHandle,
                                  struct ST_CLASS(PoolElementNode)* poolElementNode,
                                  const bool                        waitForResponse,
                                  const bool                        daemonMode);

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
  * Do handle resolution of given pool handle. The result will contain
  * an array of pointers to opaque data converted by the given conversion
  * function from PoolElementNode structures. Conversion or at least copying
  * is necessary, since other ASAP functions could modify the interal
  * PoolElementNodes!
  *
  * @param asapInstance ASAPInstance.
  * @param poolHandle Pool handle.
  * @param nodePtrArray Array to store pointers to converted PoolElementNodes to.
  * @param nodePrts Reference to variable containing maximum amount of pool element nodes to obtain. After function call, this variable contains actual amount of pool element nodes obtained.
  * @param cacheElementTimeout Stale cache value for newly received PE entries.
  * @return RSPERR_OKAY in case of success; error code otherwise.
  */
unsigned int asapInstanceHandleResolution(
                struct ASAPInstance*     asapInstance,
                struct PoolHandle*       poolHandle,
                void**                   nodePtrArray,
                size_t*                  nodePtrs,
                unsigned int             (*convertFunction)(const struct ST_CLASS(PoolElementNode)* poolElementNode,
                                                            void*                                   ptr),
                const unsigned long long cacheElementTimeout);


#ifdef __cplusplus
}
#endif


#endif
