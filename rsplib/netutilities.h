/*
 *  $Id: netutilities.h,v 1.3 2004/07/18 15:30:43 dreibh Exp $
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
 * Purpose: Network Utilities
 *
 */


#ifndef NETUTILITIES_H
#define NETUTILITIES_H


#include "tdtypes.h"
#include "tagitem.h"
#include "sockaddrunion.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <glib.h>
#include <ext_socket.h>


#ifdef __cplusplus
extern "C" {
#endif



#ifndef IPV6_JOIN_GROUP
#define IPV6_JOIN_GROUP IPV6_ADD_MEMBERSHIP
#endif
#ifndef IPV6_LEAVE_GROUP
#define IPV6_LEAVE_GROUP IPV6_DROP_MEMBERSHIP
#endif


/**
  * Get local addresses from socket. The obtained address array has
  * to be freed using deleteAddressArray().
  *
  * @param sockfd Socket descriptor.
  * @param addressArray Reference to store obtained address array to.
  * @return Number of addresses.
  *
  * @see deleteAddressArray
  */
size_t getAddressesFromSocket(int sockfd, struct sockaddr_storage** addressArray);

/**
  * Delete address array.
  *
  * @param addressArray Address array.
  */
void deleteAddressArray(struct sockaddr_storage* addressArray);

/**
  * Duplicate address array.
  *
  * @param addressArray Address array.
  * @param addresses Number of addresses.
  * @return Address array or NULL in case of error.
  */
struct sockaddr_storage* duplicateAddressArray(const struct sockaddr_storage* addressArray,
                                               const size_t                   addresses);

/**
  * Convert address to string.
  *
  * @param address Address.
  * @param buffer Buffer to store string to.
  * @param length Length of buffer.
  * @param port true to include port number; false otherwise.
  * @return true for success; false otherwise.
  */
bool address2string(const struct sockaddr* address,
                    char*                  buffer,
                    const size_t           length,
                    const bool             port);

/**
  * Scan address from string.
  *
  * @param string String.
  * @param address Reference to store address to.
  * @return true for success; false otherwise.
  */
bool string2address(const char* string, struct sockaddr_storage* address);

/**
  * Print address.
  *
  * @param address Address.
  * @param port true to include port number; false otherwise.
  * @param fd File to write address to (e.g. stdout, stderr, ...).
  */
void fputaddress(const struct sockaddr* address, const bool port, FILE* fd);

/**
  * Address comparision function.
  *
  * @param a1 Address 1.
  * @param a2 Address 2.
  * @param port true to include port number in comparision; false otherwise.
  * @return Comparision result.
  */
int addresscmp(const struct sockaddr* a1, const struct sockaddr* a2, const bool port);

/**
  * Get port from sockaddr structure.
  *
  * @param address sockaddr structure.
  * @return Port or 0 in case of error.
  */
uint16_t getPort(struct sockaddr* address);

/**
  * Set port in sockaddr structure.
  *
  * @param address sockaddr structure.
  * @param port Port.
  * @return true for success; false otherwise.
  */
bool setPort(struct sockaddr* address, const uint16_t port);

/**
  * Get address family from sockaddr structure.
  *
  * @param address sockaddr structure.
  * @return Address family
  */
int getFamily(struct sockaddr* address);

/**
  * Get relative scope of address. This function supports IPv4 and IPv6
  * addresses. The higher the returned value, the higher the address scope.
  *
  * @param address sockaddr structure.
  * @return Scope.
  */
unsigned int getScope(const struct sockaddr* address);



/**
  * Get padding for given data size and alignment.
  *
  * @param size Data size.
  * @param alignment Alignment.
  * @return Number of padding bytes necessary.
  */
size_t getPadding(const size_t size, const size_t alignment);

/**
  * Convert 24-bit value to host byte-order.
  *
  * @param value Value.
  * @return Value in host byte-order.
  */
uint32_t ntoh24(const uint32_t value);

/**
  * Convert 24-bit value to network byte-order.
  *
  * @param value Value.
  * @return Value in network byte-order.
  */
uint32_t hton24(const uint32_t value);



/**
  * Check, if IPv6 is available.
  *
  * @return true if IPv6 is available; false otherwise.
  */
bool checkIPv6();



/**
  * Set file descriptor to blocking mode.
  *
  * @param fd File descriptor.
  * @return true for success; false otherwise.
  */
bool setBlocking(int fd);

/**
  * Set file descriptor to non-blocking mode.
  *
  * @param fd File descriptor.
  * @return true for success; false otherwise.
  */
bool setNonBlocking(int fd);


/**
  * Get length of sockaddr_* structure for given addres..
  *
  * @param address Address.
  * @return Length of structure.
  */
size_t getSocklen(struct sockaddr* address);


/**
  * Wrapper for bind() and bindx(). This function automatically selects a
  * random port if the given port number is 0. If the number of addresses is
  * 1, bind() will be used; otherwise, bindx() is will be used.
  *
  * @param sockfd Socket descriptor.
  * @param addressArray Address array.
  * @param addresses Number of addresses.
  * @return true for success; false otherwise.
  */
bool bindplus(int                      sockfd,
              struct sockaddr_storage* addressArray,
              const size_t             addresses);

/**
  * Wrapper for sendmsg() with timeout and support for SCTP parameters.
  *
  * @param sockfd Socket descriptor.
  * @param buffer Data to send.
  * @param length Length of data to send.
  * @param flags sendmsg() flags.
  * @param to Destination address or NULL for connection-oriented socket.
  * @param tolen Length of destination address or 0 if not given.
  * @param ppid SCTP Payload Protocol Identifier.
  * @param assocID SCTP Association ID or 0 for connection-oriented socket.
  * @param streamID SCTP Stream ID.
  * @param timeToLive SCTP Time To Live.
  * @param timeout Timeout for sending data.
  * @param Bytes sent or -1 in case of error.
  */
int sendtoplus(int                 sockfd,
               const void*         buffer,
               const size_t        length,
               const int           flags,
               struct sockaddr*    to,
               const socklen_t     tolen,
               const uint32_t      ppid,
               const sctp_assoc_t  assocID,
               const uint16_t      streamID,
               const uint32_t      timeToLive,
               const card64        timeout);

/**
  * Wrapper for recvmsg() with timeout and support for SCTP parameters.
  *
  * @param sockfd Socket descriptor.
  * @param buffer Buffer to store read data to.
  * @param length Length of buffer.
  * @param flags recvmsg() flags.
  * @param from Reference to store source address to or NULL if not necessary.
  * @param fromlen Reference to store source address length to or NULL if not necessary.
  * @param ppid Reference to store SCTP Payload Protocol Identifier to.
  * @param assocID Reference to store SCTP Association ID to.
  * @param streamID Reference to store SCTP Stream ID to.
  * @param timeToLive Reference to store SCTP Time To Live to.
  * @param timeout Timeout for receiving data.
  * @param Bytes read or -1 in case of error.
  */
int recvfromplus(int              sockfd,
                 void*            buffer,
                 size_t           length,
                 int              flags,
                 struct sockaddr* from,
                 socklen_t*       fromlen,
                 uint32_t*        ppid,
                 sctp_assoc_t*    assocID,
                 uint16_t*        streamID,
                 const card64     timeout);

/**
  * Join or leave a multicast group.
  *
  * @param sockfd Socket descriptor.
  * @param address Address of multicast group to join/leave.
  * @param interface Interface name or NULL for default.
  * @param add true to join/false to leave multicast group.
  */
bool multicastGroupMgt(int              sockfd,
                       struct sockaddr* address,
                       const char*      interface,
                       const bool       add);

/**
  * Create socket and establish connection to a given peer.
  * A connection establishment is tried on all given peer
  * addresses in parallel. The first sucessfully established
  * connection socket descriptor is returned. All other
  * connections are aborted.
  */
int establish(const int                socketDomain,
              const int                socketType,
              const int                socketProtocol,
              struct sockaddr_storage* addressArray,
              const size_t             addresses,
              const card64             timeout);


#define TAG_TuneSCTP_MinRTO      (TAG_USER + 15000)
#define TAG_TuneSCTP_MaxRTO      (TAG_USER + 15001)
#define TAG_TuneSCTP_InitialRTO  (TAG_USER + 15002)
#define TAG_TuneSCTP_Heartbeat   (TAG_USER + 15003)
#define TAG_TuneSCTP_PathMaxRXT  (TAG_USER + 15004)
#define TAG_TuneSCTP_AssocMaxRXT (TAG_USER + 15006)
#define TAG_TuneSCTP_LocalRWND   (TAG_USER + 15007)
#define TAG_TuneSCTP_CookieLife  (TAG_USER + 15008)

/**
  * Tune SCTP parameters for given association.
  *
  * @param sockfd Socket.
  * @param assocID Association ID (0 for TCP-like).
  * @param tags Parameters as tag items.
  * @return true in case of success; false otherwise.
  */
bool tuneSCTP(int sockfd, sctp_assoc_t assocID, struct TagItem* tags);


/**
  * Translate a sockaddr_storage array into a block of sockaddrs.
  * The memory is dynamically allocated using malloc() and has to be
  * freed using free().
  *
  * @param addrArray sockaddr_storage array.
  * @param addrs Number of addresses.
  * @return Point to block of sockaddrs.
  */
struct sockaddr* pack_sockaddr_storage(const struct sockaddr_storage* addrArray,
                                       const size_t                   addrs);

/**
  * Translate block of sockaddrs into sockaddr_storage array.
  * The memory is dynamically allocated using malloc() and has to be
  * freed using free().
  *
  * @param addrArray Block of sockaddrs.
  * @param addrs Number of addresses.
  * @return Point sockaddr_storage array.
  */
struct sockaddr_storage* unpack_sockaddr(struct sockaddr* addrArray,
                                         const size_t     addrs);

/**
  * Wrapper to sctp_getladdrs() using sockaddr_storage array instead of
  * packed sockaddr blocks.
  *
  * @param fd Socket FD.
  * @param assocID Association ID.
  * @param addressArray Pointer to store pointer to created address array to.
  * @return Number of addresses stored.
  */
size_t getladdrsplus(const int                 fd,
                     const sctp_assoc_t        assocID,
                     struct sockaddr_storage** addressArray);

/**
  * Wrapper to sctp_getpaddrs() using sockaddr_storage array instead of
  * packed sockaddr blocks.
  *
  * @param fd Socket FD.
  * @param assocID Association ID.
  * @param addressArray Pointer to store pointer to created address array to.
  * @return Number of addresses stored.
  */
size_t getpaddrsplus(const int                 fd,
                     const sctp_assoc_t        assocID,
                     struct sockaddr_storage** addressArray);


#ifdef __cplusplus
}
#endif


#endif
