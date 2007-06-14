/*
 *  $Id$
 *
 * RSerPool implementation.
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


#ifdef HAVE_KERNEL_SCTP
#ifndef HAVE_SCTP_CONNECTX
int sctp_connectx(int                    sockfd,
                  const struct sockaddr* addrs,
                  int                    addrcnt);
#endif

#ifndef HAVE_SCTP_SEND
ssize_t sctp_send(int                           sd,
                  const void*                   data,
                  size_t                        len,
                  const struct sctp_sndrcvinfo* sinfo,
                  int                           flags);
#endif

#ifndef HAVE_SCTP_SENDX
ssize_t sctp_sendx(int                           sd,
                   const void*                   data,
                   size_t                        len,
                   const struct sockaddr*        addrs,
                   int                           addrcnt,
                   const struct sctp_sndrcvinfo* sinfo,
                   int                           flags);
#endif
#endif


/**
  * Convert 64-bit value to network byte order.
  *
  * @param value Value in host byte order.
  * @return Value in network byte order.
  */
uint64_t hton64(const uint64_t value);


/**
  * Convert 64-bit value to host byte order.
  *
  * @param value Value in network byte order.
  * @return Value in host byte order.
  */
uint64_t ntoh64(const uint64_t value);


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
size_t getAddressesFromSocket(int sockfd, union sockaddr_union** addressArray);

/**
  * Gather local addresses. The obtained address array will be stored
  * to the variable addressArray. It has to be freed using free().
  *
  * @param addressArray Reference to store address array to.
  * @return Number of obtained addresses or 0 in case of error.
  */
size_t gatherLocalAddresses(union sockaddr_union** addressArray);

/**
  * Delete address array.
  *
  * @param addressArray Address array.
  */
void deleteAddressArray(union sockaddr_union* addressArray);

/**
  * Duplicate address array.
  *
  * @param addressArray Address array.
  * @param addresses Number of addresses.
  * @return Address array or NULL in case of error.
  */
union sockaddr_union* duplicateAddressArray(const union sockaddr_union* addressArray,
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
bool string2address(const char* string, union sockaddr_union* address);

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
uint16_t getPort(const struct sockaddr* address);

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
int getFamily(const struct sockaddr* address);

/**
  * Get relative scope of address. This function supports IPv4 and IPv6
  * addresses. The higher the returned value, the higher the address scope.
  *
  * @param address sockaddr structure.
  * @return Scope.
  */
unsigned int getScope(const struct sockaddr* address);

/**
  * Get one address of highest scope from address array.
  *
  * @param addrs Address array.
  * @param addrcnt Address count.
  * @return Selected address.
  */
const struct sockaddr* getBestScopedAddress(const struct sockaddr* addrs,
                                            int                    addrcnt);


/**
  * Get padding for given data size and alignment.
  *
  * @param size Data size.
  * @param alignment Alignment.
  * @return Number of padding bytes necessary.
  */
size_t getPadding(const size_t size, const size_t alignment);


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
size_t getSocklen(const struct sockaddr* address);


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
bool bindplus(int                   sockfd,
              union sockaddr_union* addressArray,
              const size_t          addresses);

/**
  * Establish a connection.
  *
  * @param sockfd Socket descriptor.
  * @param addressArray Destination address or NULL for connection-oriented socket.
  * @param addresses Length of destination address or 0 if not given.
  * @return 0 in case of success; error code <0 otherwise.
  */
int connectplus(int                   sockfd,
                union sockaddr_union* addressArray,
                const size_t          addresses);

/**
  * Wrapper for sendmsg() with timeout and support for SCTP parameters.
  *
  * @param sockfd Socket descriptor.
  * @param buffer Data to send.
  * @param length Length of data to send.
  * @param flags sendmsg() flags.
  * @param toaddrs Destination addresses or NULL for connection-oriented socket.
  * @param toaddrcnt Number of destination addresses.
  * @param ppid SCTP Payload Protocol Identifier.
  * @param assocID SCTP Association ID or 0 for connection-oriented socket.
  * @param streamID SCTP Stream ID.
  * @param timeToLive SCTP Time To Live.
  * @param sctpFlags SCTP Flags.
  * @param timeout Timeout for sending data.
  * @param Bytes sent or -1 in case of error.
  */
int sendtoplus(int                      sockfd,
               const void*              buffer,
               const size_t             length,
               const int                flags,
               union sockaddr_union*    toaddrs,
               const size_t             toaddrcnt,
               const uint32_t           ppid,
               const sctp_assoc_t       assocID,
               const uint16_t           streamID,
               const uint32_t           timeToLive,
               const uint16_t           sctpFlags,
               const unsigned long long timeout);

/**
  * Wrapper for recvmsg() with timeout and support for SCTP parameters.
  *
  * @param sockfd Socket descriptor.
  * @param buffer Buffer to store read data to.
  * @param length Length of buffer.
  * @param flags Reference to store recvmsg() flags.
  * @param from Reference to store source address to or NULL if not necessary.
  * @param fromlen Reference to store source address length to or NULL if not necessary.
  * @param ppid Reference to store SCTP Payload Protocol Identifier to.
  * @param assocID Reference to store SCTP Association ID to.
  * @param streamID Reference to store SCTP Stream ID to.
  * @param timeToLive Reference to store SCTP Time To Live to.
  * @param timeout Timeout for receiving data.
  * @param Bytes read or -1 in case of error.
  */
int recvfromplus(int                      sockfd,
                 void*                    buffer,
                 size_t                   length,
                 int*                     flags,
                 struct sockaddr*         from,
                 socklen_t*               fromlen,
                 uint32_t*                ppid,
                 sctp_assoc_t*            assocID,
                 uint16_t*                streamID,
                 const unsigned long long timeout);

/**
  * Abort SCTP association.
  *
  * @param sockfd Socket descriptor.
  * @param assocID Association ID.
  * @return 0 in case of success, -1 otherwise.
  */
int sendabort(int sockfd, sctp_assoc_t assocID);

/**
  * Shutdown SCTP association.
  *
  * @param sockfd Socket descriptor.
  * @param assocID Association ID.
  * @return 0 in case of success, -1 otherwise.
  */
int sendshutdown(int sockfd, sctp_assoc_t assocID);

/**
  * Join or leave a multicast group.
  *
  * @param sockfd Socket descriptor.
  * @param address Address of multicast group to join/leave.
  * @param add true to join/false to leave multicast group.
  */
bool joinOrLeaveMulticastGroup(int                         sd,
                               const union sockaddr_union* groupAddress,
                               const bool                  add);

/**
  * Send multicast message over each possible interface.
  *
  * @param sockfd Socket descriptor.
  * @param family Socket family (AF_INET or AF_INET6).
  * @param buffer Data to send.
  * @param length Length of data to send.
  * @param flags sendmsg() flags.
  * @param to Destination address or NULL for connection-oriented socket.
  * @param tolen Length of destination address or 0 if not given.
  */
size_t sendMulticastOverAllInterfaces(int                    sockfd,
                                      int                    family,
                                      const void*            buffer,
                                      const size_t           length,
                                      const int              flags,
                                      const struct sockaddr* to,
                                      socklen_t              tolen);

/**
  * Set address and port reuse on socket on or off.
  *
  * @param sd Socket descriptor.
  * @param on 1 to set reuse on, 0 for off.
  * @return true for success; false otherwise.
  */
bool setReusable(int sd, int on);

/**
  * Create socket and establish connection to a given peer.
  * A connection establishment is tried on all given peer
  * addresses in parallel. The first sucessfully established
  * connection socket descriptor is returned. All other
  * connections are aborted.
  */
int establish(const int             socketDomain,
              const int             socketType,
              const int             socketProtocol,
              union sockaddr_union* addressArray,
              const size_t          addresses,
              const unsigned long long          timeout);


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
  * Translate a sockaddr_union array into a block of sockaddrs.
  * The memory is dynamically allocated using malloc() and has to be
  * freed using free().
  *
  * @param addrArray sockaddr_union array.
  * @param addrs Number of addresses.
  * @return Point to block of sockaddrs.
  */
struct sockaddr* pack_sockaddr_union(const union sockaddr_union* addrArray,
                                     const size_t                addrs);

/**
  * Translate block of sockaddrs into sockaddr_union array.
  * The memory is dynamically allocated using malloc() and has to be
  * freed using free().
  *
  * @param addrArray Block of sockaddrs.
  * @param addrs Number of addresses.
  * @return Point sockaddr_union array.
  */
union sockaddr_union* unpack_sockaddr(struct sockaddr* addrArray,
                                      const size_t     addrs);

/**
  * Wrapper to sctp_getladdrs() using sockaddr_union array instead of
  * packed sockaddr blocks.
  *
  * @param fd Socket FD.
  * @param assocID Association ID.
  * @param addressArray Pointer to store pointer to created address array to.
  * @return Number of addresses stored.
  */
size_t getladdrsplus(const int              fd,
                     const sctp_assoc_t     assocID,
                     union sockaddr_union** addressArray);

/**
  * Wrapper to sctp_getpaddrs() using sockaddr_union array instead of
  * packed sockaddr blocks.
  *
  * @param fd Socket FD.
  * @param assocID Association ID.
  * @param addressArray Pointer to store pointer to created address array to.
  * @return Number of addresses stored.
  */
size_t getpaddrsplus(const int              fd,
                     const sctp_assoc_t     assocID,
                     union sockaddr_union** addressArray);


#ifdef __cplusplus
}
#endif


#endif
