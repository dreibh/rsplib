/*
 *  $Id: asapconnection.h,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: ASAP Connection
 *
 */


#ifndef ASAPCONNECTION_H
#define ASAPCONNECTION_H


#include "tdtypes.h"
#include "pool.h"
#include "poolelement.h"
#include "asapinstance.h"

#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>


#ifdef __cplusplus
extern "C" {
#endif



struct ASAPConnection
{
   struct ASAPInstance* ASAP;

   struct PoolHandle*   Handle;
   struct PoolElement*  CurrentPoolElement;
   int                  CurrentSocket;
   int                  CurrentProtocol;

   int    EventMask;
   void (*Callback)(struct ASAPConnection* asapConnection,
                    int                    eventMask,
                    void*                  userData);
   void*  UserData;
};



/**
  * Do name lookup for given pool handle, select a pool element by policy
  * and establish connection to it.
  *
  * @param asap ASAPInstance.
  * @param poolHandle Pool handle.
  * @param protocol Desired protocol (0 for default).
  * @param asapConnection Reference to store connection data to.
  * @param eventMask Event mask to register the connection's socket for (see Dispatcher class).
  * @param callback Callback function to call for the registered events.
  * @param userData User data for the callback function.
  * @return ASAP_Okay in case of success; error code otherwise.
  */
enum ASAPError asapConnect(struct ASAPInstance*    asap,
                           struct PoolHandle*      poolHandle,
                           const int               protocol,
                           struct ASAPConnection** asapConnection,
                           int                     eventMask,
                           void                    (*callback)(struct ASAPConnection* asapConnection,
                                                               int                    eventMask,
                                                               void*                  userData),
                           void*                   userData);

/**
  * Disconnect from pool element.
  *
  * @param asap ASAPInstance.
  * @param asapConnection ASAPConnection.
  */
void asapDisconnect(struct ASAPInstance*   asap,
                    struct ASAPConnection* asapConnection);

/**
  * Report failure of pool element.
  *
  * @param asap ASAPInstance.
  * @param asapConnection ASAPConnection.
  */
void asapFailure(struct ASAPInstance*   asap,
                 struct ASAPConnection* asapConnection);

/**
  * Do failover: Disconnect, look for a new pool element and establish
  * connection.
  *
  * @param asap ASAPInstance.
  * @param asapConnection ASAPConnection.
  * @return ASAP_Okay in case of success; error code otherwise.
  */
enum ASAPError asapConnectionFailover(struct ASAPConnection* asapConnection);

/**
  * Write user data to pool element. The data to write is given as msghdr
  * structure.
  *
  * @param asapConnection ASAPConnection.
  * @param message msghdr structure.
  * @return Number of bytes written or error code (less than 0).  
  */
ssize_t asapConnectionWrite(struct ASAPConnection* asapConnection,
                            struct msghdr*         message);

/**
  * Read user data from pool element. The buffer to read to is given as msghdr
  * structure. The pool element's pool element handle is stored to
  * message->name if it is not zero and has sufficient space (message->namelen).
  *
  * @param asapConnection ASAPConnection.
  * @param message msghdr structure.
  * @return Number of bytes read or error code (less than 0).
  */
ssize_t asapConnectionRead(struct ASAPConnection* asapConnection,
                           struct msghdr*         message);


#ifdef __cplusplus
}
#endif


#endif
