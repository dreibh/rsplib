/*
 *  $Id: asapinstance.h,v 1.7 2004/08/25 17:27:33 dreibh Exp $
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
 * Purpose: ASAP Instance
 *
 */

#ifndef ASAPINSTANCE_H
#define ASAPINSTANCE_H

#include "tdtypes.h"
#include "dispatcher.h"
#include "tagitem.h"
#include "rserpoolmessage.h"
#include "messagebuffer.h"
#include "poolnamespacemanagement.h"
#include "servertable.h"


#ifdef __cplusplus
extern "C" {
#endif


struct ASAPInstance
{
   struct Dispatcher*                       StateMachine;
   int                                      NameServerSocket;
   int                                      NameServerSocketProtocol;

   struct ServerTable*                      NameServerTable;
   struct ST_CLASS(PoolNamespaceManagement) Cache;
   struct ST_CLASS(PoolNamespaceManagement) OwnPoolElements;

   char*                                    AsapServerAnnounceConfigFile;
   char*                                    AsapNameServersConfigFile;

   unsigned long long                       CacheElementTimeout;
   unsigned long long                       CacheMaintenanceInterval;

   struct FDCallback                        NameServerFDCallback;
   size_t                                   NameServerRequestMaxTrials;
   unsigned long long                       NameServerRequestTimeout;
   unsigned long long                       NameServerResponseTimeout;

   struct MessageBuffer*                    Buffer;
};


#define ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT               30000000
#define ASAP_DEFAULT_NAMESERVER_ANNOUNCE_ADDRESS "239.0.0.1:3863"
#define ASAP_DEFAULT_NAMESERVER_ANNOUNCE_TIMEOUT         30000000
#define ASAP_DEFAULT_NAMESERVER_CONNECT_MAXTRIALS               1
#define ASAP_DEFAULT_NAMESERVER_CONNECT_TIMEOUT           1500000
#define ASAP_DEFAULT_NAMESERVER_REQUEST_MAXTRIALS               1
#define ASAP_DEFAULT_NAMESERVER_REQUEST_TIMEOUT           1500000
#define ASAP_DEFAULT_NAMESERVER_RESPONSE_TIMEOUT          1500000

#define ASAP_BUFFER_SIZE                                    65536


/**
  * Constructor.
  *
  * @param dispatcher Dispatcher.
  * @param tags TagItems.
  * @return ASAPInstance or NULL in case of error.
  */
struct ASAPInstance* asapInstanceNew(struct Dispatcher* dispatcher,
                                     struct TagItem*    tags);

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
unsigned int asapInstanceRegister(
                struct ASAPInstance*              asapInstance,
                struct PoolHandle*                poolHandle,
                struct ST_CLASS(PoolElementNode)* poolElement);

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
                const PoolElementIdentifierType identifier);

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
unsigned int asapInstanceNameResolution(
                struct ASAPInstance*               asapInstanceInstance,
                struct PoolHandle*                 poolHandle,
                struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                size_t*                            poolElementNodes);


#ifdef __cplusplus
}
#endif


#endif
