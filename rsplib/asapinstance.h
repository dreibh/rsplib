/*
 *  $Id: asapinstance.h,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
#include "asapmessage.h"
#include "messagebuffer.h"
#include "poolnamespace.h"
#include "asaperror.h"
#include "servertable.h"

#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



struct ASAPInstance
{
   struct Dispatcher*    StateMachine;
   int                   NameServerSocket;
   int                   NameServerSocketProtocol;

   struct ServerTable*   NameServerTable;
   struct ASAPCache*     Cache;

   char*                 AsapServerAnnounceConfigFile;
   char*                 AsapNameServersConfigFile;

   card64                CacheElementTimeout;
   card64                CacheMaintenanceInterval;

   cardinal              NameServerRequestMaxTrials;
   card64                NameServerRequestTimeout;
   card64                NameServerResponseTimeout;

   struct MessageBuffer* Buffer;
};


#define ASAP_DEFAULT_NAMESERVER_ANNOUNCE_ADDRESS_IPv4         "239.0.0.1:3863"
#define ASAP_DEFAULT_NAMESERVER_ANNOUNCE_ADDRESS_IPv6         "[fff5::1]:3863"
#define ASAP_DEFAULT_NAMESERVER_ANNOUNCE_MAINTENANCE_INTERVAL  1000000
#define ASAP_DEFAULT_NAMESERVER_ANNOUNCE_TIMEOUT              30000000
#define ASAP_DEFAULT_NAMESERVER_CONNECT_MAXTRIALS                    3
#define ASAP_DEFAULT_NAMESERVER_CONNECT_TIMEOUT                7500000
#define ASAP_DEFAULT_NAMESERVER_REQUEST_MAXTRIALS                    3
#define ASAP_DEFAULT_NAMESERVER_REQUEST_TIMEOUT                5000000
#define ASAP_DEFAULT_NAMESERVER_RESPONSE_TIMEOUT               4500000

#define ASAP_DEFAULT_CACHE_MAINTENANCE_INTERVAL                1000000
#define ASAP_DEFAULT_CACHE_ELEMENT_TIMEOUT                    30000000

#define ASAP_BUFFER_SIZE                                         65536


/**
  * Constructor.
  *
  * @param dispatcher Dispatcher.
  * @param tags TagItems.
  * @return ASAPInstance or NULL in case of error.
  */
struct ASAPInstance* asapNew(struct Dispatcher* dispatcher,
                             struct TagItem*    tags);

/**
  * Destructor.
  *
  * @param asap ASAPInstance.
  */
void asapDelete(struct ASAPInstance* asap);

/**
  * Register pool element.
  *
  * @param asap ASAPInstance.
  * @param poolHandle Pool handle.
  * @param poolElement Pool Element.
  * @return ASAP_Okay in case of success; error code otherwise.
  */
enum ASAPError asapRegister(struct ASAPInstance* asap,
                            struct PoolHandle*   poolHandle,
                            struct PoolElement*  poolElement);

/**
  * Deregister pool element.
  *
  * @param asap ASAPInstance.
  * @param poolHandle Pool handle.
  * @param identifier Pool element identifier.
  * @return ASAP_Okay in case of success; error code otherwise.
  */
enum ASAPError asapDeregister(struct ASAPInstance*        asap,
                              struct PoolHandle*          poolHandle,
                              const PoolElementIdentifier identifier);

/**
  * Report failure of pool element.
  *
  * @param asap ASAPInstance.
  * @param poolHandle Pool handle.
  * @param identifier Pool element identifier.
  * @return ASAP_Okay in case of success; error code otherwise.
  */
enum ASAPError asapFailure(struct ASAPInstance*        asap,
                           struct PoolHandle*          poolHandle,
                           const PoolElementIdentifier identifier);

/**
  * Query name server for pool handle and select one pool element
  * by policy.
  *
  * @param asap ASAPInstance.
  * @param poolHandle Pool handle.
  * @param error Reference to store error code to.
  * @return Pool element or NULL in case of error.
  */
struct PoolElement* asapSelectPoolElement(struct ASAPInstance* asap,
                                          struct PoolHandle*   poolHandle,
                                          enum ASAPError*      error);

/**
  * Do name resolution of given pool handle. The resulting pool pointer
  * will be stored to the variable poolPtr.
  *
  * @param asap ASAPInstance.
  * @param poolHandle Pool handle.
  * @param poolPtr Reference to store reply to.
  * @return ASAP_Okay in case of success; error code otherwise.
  */
enum ASAPError asapNameResolution(struct ASAPInstance* asap,
                                  struct PoolHandle*   poolHandle,
                                  struct Pool**        poolPtr);


#ifdef __cplusplus
}
#endif


#endif
