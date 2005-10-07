/*
 *  $Id$
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium f�r Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (F�rderkennzeichen 01AK045).
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
 * Purpose: RSerPool Library
 *
 */


#ifndef RSPLIB_H
#define RSPLIB_H

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <sys/types.h>

#include "tdtypes.h"
#include "tagitem.h"
#include "sockaddrunion.h"
#include "rsplib-tags.h"
#include "poolpolicysettings.h"
#include "rserpoolerror.h"
#include "componentstatusprotocol.h"


#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
#define CPP_DEFAULT(x) = x
#else
#define CPP_DEFAULT(x)
#endif


#define RSPLIB_VERSION  0
#define RSPLIB_REVISION 8001


#define RspSelect_Read   (1 << 0)
#define RspSelect_Write  (1 << 1)
#define RspSelect_Except (1 << 2)


struct EndpointAddressInfo {
   int                         ai_family;
   int                         ai_socktype;
   int                         ai_protocol;
   size_t                      ai_addrlen;
   size_t                      ai_addrs;
   union sockaddr_union*       ai_addr;
   struct EndpointAddressInfo* ai_next;
   uint32_t                    ai_pe_id;
};



/**
  * Initialize rsplib.
  *
  * @param tags Additional parameters.
  * @return 0 in case of success, <>0 in case of error.
  */
unsigned int rspInitialize(struct TagItem* tags CPP_DEFAULT(NULL));

/**
  * Add static registrar entry.
  *
  * @addressString Comma-separated list of transport addresses.
  * @return Error code.
  */
unsigned int rspAddStaticRegistrar(const char* addressString);

/**
  * Clean up rsplib.
  */
void rspCleanUp();

/**
  * Get textual description of error code.
  *
  * @param errorCode Error code.
  * @return Textual description.
  */
const char* rspGetErrorDescription(const unsigned int errorCode);


/**
  * Register pool element. In case of success, the pool element's identifier
  * is returned. Otherwise, 0 is returned and the exact error can be obtained
  * using rspGetLastError() and rspGetLastErrorDescription().
  *
  * @param poolHandle Pool handle.
  * @param poolHandleSize Size of pool handle.
  * @param endpointAddressInfo Pool element's address information.
  * @param tags Additional parameters.
  * @return Success.
  *
  * @see rspGetLastError
  * @see rspGetLastErrorDescription
  */
unsigned int rspRegister(const unsigned char*        poolHandle,
                         const size_t                poolHandleSize,
                         struct EndpointAddressInfo* endpointAddressInfo,
                         struct TagItem*             tags CPP_DEFAULT(NULL));

/**
  * Deregister pool element. In case of success, 0 is returned. Otherwise,
  * a nonzero value is returned and the exact error can be obtained
  * using rspGetLastError() and rspGetLastErrorDescription().
  *
  * @param poolHandle Pool handle.
  * @param poolHandleSize Size of pool handle.
  * @param identifier Pool element identifier.
  * @param tags Additional parameters.
  * @return Success.
  */
unsigned int rspDeregister(const unsigned char* poolHandle,
                           const size_t         poolHandleSize,
                           const uint32_t       identifier,
                           struct TagItem*      tags CPP_DEFAULT(NULL));

/**
  * Do handle resolution for given pool handle and select one pool element
  * by policy. The resolved address structure is stored to a variable
  * given as reference parameter. This structure must be freed after usage
  * using rspFreeEndpointAddressArray().
  * In case of success, 0 is returned. Otherwise,
  * a nonzero value is returned and the exact error can be obtained
  * using rspGetLastError() and rspGetLastErrorDescription().
  *
  * @param poolHandle Pool handle.
  * @param poolHandleSize Size of pool handle.
  * @param endpointAddressInfo Pool element's address information.
  * @param tags Additional parameters.
  * @return Success.
  *
  * @rspFreeEndpointAddressArray
  * @see rspGetLastError
  * @see rspGetLastErrorDescription
  */
unsigned int rspHandleResolution(const unsigned char*         poolHandle,
                                 const size_t                 poolHandleSize,
                                 struct EndpointAddressInfo** endpointAddressInfo,
                                 struct TagItem*              tags CPP_DEFAULT(NULL));

/**
  * Free endpoint address structure created by rspHandleResolution().
  *
  * @param endpointAddressInfo Endpoint address structure to be freed.
  *
  * @see rspHandleResolution
  */
void rspFreeEndpointAddressArray(struct EndpointAddressInfo* endpointAddressInfo);

/**
  * Report pool element failure to rsplib.
  *
  * @param poolHandle Pool handle.
  * @param poolHandleSize Size of pool handle.
  * @param identifier Pool element identifier.
  * @param tags Additional parameters.
  * @return Success.
  */
unsigned int rspReportFailure(const unsigned char* poolHandle,
                              const size_t         poolHandleSize,
                              const uint32_t       identifier,
                              struct TagItem*      tags CPP_DEFAULT(NULL));

/**
  * Wrapper to the system's select() function. This function handles
  * ASAP-internal management functions like keep alives. Use
  * this function instead of select().
  *
  * @param n Highest FD number + 1.
  * @param readfds FD set for read events.
  * @param writefds FD set for write events.
  * @param exceptfds FD set for exception events.
  * @param timeout Timeout.
  * @return Number of events or -1 in case of error.
  */
int rspSelect(int             n         CPP_DEFAULT(0),
              fd_set*         readfds   CPP_DEFAULT(NULL),
              fd_set*         writefds  CPP_DEFAULT(NULL),
              fd_set*         exceptfds CPP_DEFAULT(NULL),
              struct timeval* timeout   CPP_DEFAULT(NULL));


struct PoolElementDescriptor;
struct SessionDescriptor;


/**
  * Create pool element.
  *
  * @param poolHandle Pool handle.
  * @param poolHandleSize Pool handle size.
  * @param tags TagItem array.
  * @return PoolElementDescriptor or NULL in case of error.
  */
struct PoolElementDescriptor* rspCreatePoolElement(const unsigned char* poolHandle,
                                                   const size_t         poolHandleSize,
                                                   struct TagItem*      tags CPP_DEFAULT(NULL));

/**
  * Delete pool element.
  *
  * @param poolElement PoolElementDescriptor.
  * @param tags TagItem array.
  */
void rspDeletePoolElement(struct PoolElementDescriptor* ped,
                          struct TagItem*               tags CPP_DEFAULT(NULL));

/**
  * Accept session.
  *
  * @param poolElement PoolElementDescriptor.
  * @param tags TagItem array.
  * @return SessionDescriptor or NULL in case of error.
  */
struct SessionDescriptor* rspAcceptSession(struct PoolElementDescriptor* ped,
                                           struct TagItem*               tags CPP_DEFAULT(NULL));

/**
  * Create session.
  *
  * @param poolHandle Pool handle.
  * @param poolHandleSize Pool handle size.
  * @param ped PoolElementDescriptor (PU is also PE here).
  * @param tags TagItem array.
  * @return SessionDescriptor or NULL in case of error.
  */
struct SessionDescriptor* rspCreateSession(const unsigned char*          poolHandle,
                                           const size_t                  poolHandleSize,
                                           struct PoolElementDescriptor* ped  CPP_DEFAULT(NULL),
                                           struct TagItem*               tags CPP_DEFAULT(NULL));

/**
  * Delete session.
  *
  * @param sd SessionDescriptor.
  * @param tags TagItem array.
  */
void rspDeleteSession(struct SessionDescriptor* sd,
                      struct TagItem*           tags CPP_DEFAULT(NULL));

/**
  * Read from session.
  *
  * @param sd SessionDescriptor.
  * @param buffer Buffer.
  * @param length Buffer length.
  * @param tags TagItem array.
  * @return Bytes read or error code (< 0).
  */
ssize_t rspSessionRead(struct SessionDescriptor* sd,
                       void*                     buffer,
                       const size_t              length,
                       struct TagItem*           tags CPP_DEFAULT(NULL));

/**
  * Write to session.
  *
  * @param sd SessionDescriptor.
  * @param buffer Buffer.
  * @param length Buffer length.
  * @param tags TagItem array.
  * @return Bytes written or error code (< 0).
  */
ssize_t rspSessionWrite(struct SessionDescriptor* sd,
                        const void*               buffer,
                        const size_t              length,
                        struct TagItem*           tags CPP_DEFAULT(NULL));

/**
  * Check, if session has cookie.
  *
  * @param sd SessionDescriptor.
  * @return true, if cookie is available; false otherwise.
  */
bool rspSessionHasCookie(struct SessionDescriptor* session);

/**
  * Force session failover.
  *
  * @param sd SessionDescriptor.
  * @return true in case of success; false otherwise.
  */
bool rspSessionFailover(struct SessionDescriptor* session);

/**
  * Send cookie.
  *
  * @param sd SessionDescriptor.
  * @param cookie Cookie.
  * @param cookieSize Cookie size,
  * @param tags TagItem array.
  * @return true for success or false in case of error.
  */
bool rspSessionSendCookie(struct SessionDescriptor* session,
                          const unsigned char*      cookie,
                          const size_t              cookieSize,
                          struct TagItem*           tags CPP_DEFAULT(NULL));

/**
  * select() equivalent for sessions and pool elements.
  *
  * @param sessionArray SessionDescriptor array.
  * @param sessionStatusArray Session status array.
  * @param sessions Number of sessions.
  * @param poolElementArray PoolElementDescriptor array.
  * @param poolElementStatusArray PoolElement status array.
  * @param poolElements Number of poolElements.
  * @param timeout Maximum timeout in microseconds.
  * @param tags TagItem array.
  * @return Number of events.
  */
int rspSessionSelect(struct SessionDescriptor**     sessionArray,
                     unsigned int*                  sessionStatusArray,
                     const size_t                   sessions,
                     struct PoolElementDescriptor** pedArray,
                     unsigned int*                  pedStatusArray,
                     const size_t                   peds,
                     const unsigned long long       timeout,
                     struct TagItem*                tags CPP_DEFAULT(NULL));


/**
  * Set session's application status text for rspGetComponentStatus().
  *
  * @param sd SessionDescriptor.
  * @param statusText Status text.
  */
void rspSessionSetStatusText(struct SessionDescriptor* sd,
                             const char*               statusText);


#ifdef __cplusplus
}
#endif

#endif