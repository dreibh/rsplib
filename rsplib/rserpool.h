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

#ifndef RSERPOOL_H
#define RSERPOOL_H

#include <stdint.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/poll.h>

#include "rserpool-policytypes.h"
#include "rserpool-csp.h"


#ifdef __cplusplus
extern "C" {
#endif


#define RSPLIB_VERSION  2
#define RSPLIB_REVISION 3


/*
   ##########################################################################
   #### RSerPool DEFINITIONS                                             ####
   ##########################################################################
*/

#define ASAP_PORT 3863
#define ENRP_PORT 9901

#define ASAP_ANNOUNCE_MULTICAST_ADDRESS "239.0.0.50:3863"
#define ENRP_ANNOUNCE_MULTICAST_ADDRESS "239.0.0.51:9901"


/*
   ##########################################################################
   #### LIBRARY INITIALIZATION/CLEAN-UP                                  ####
   ##########################################################################
*/

struct rsp_registrar_info
{
   struct rsp_registrar_info* rri_next;
   struct sockaddr*           rri_addr;
   size_t                     rri_addrs;
};

struct rsp_info
{
   unsigned int               ri_version;
   unsigned int               ri_revision;
   const char*                ri_build_date;
   const char*                ri_build_time;
   struct rsp_registrar_info* ri_registrars;
   int                        ri_disable_autoconfig;
   struct sockaddr*           ri_registrar_announce;

   unsigned int               ri_registrar_announce_timeout;
   unsigned int               ri_registrar_connect_timeout;
   unsigned int               ri_registrar_connect_max_trials;
   unsigned int               ri_registrar_request_timeout;
   unsigned int               ri_registrar_response_timeout;
   unsigned int               ri_registrar_request_max_trials;
   
   uint64_t                   ri_csp_identifier;
   struct sockaddr*           ri_csp_server;
   unsigned int               ri_csp_interval;
};

struct rsp_loadinfo
{
   uint32_t rli_policy;
   uint32_t rli_weight;
   uint32_t rli_weight_dpf;
   uint32_t rli_load;
   uint32_t rli_load_degradation;
   uint32_t rli_load_dpf;
};

#define REGF_DONTWAIT         (1 << 0)   /* Do not wait for Registration Response   */
#define REGF_CONTROLCHANNEL   (1 << 1)   /* Pool Element has Control Channel        */
#define REGF_DAEMONMODE       (1 << 2)   /* Daemon mode: don not stop on errors     */

#define DEREGF_DONTWAIT       (1 << 0)   /* Do not wait for Deregistration Response */


/**
  * Initialize rsp_info structure with defaults.
  *
  * @param info rsp_info structure with parameters.
  */
void rsp_initinfo(struct rsp_info* info);

/**
  * Free rsp_info structure contents allocated upon rsp_initarg
  * calls.
  *
  * @param info rsp_info structure with parameters.
  *
  * @see rsp_initarg
  */
void rsp_freeinfo(struct rsp_info* info);

/**
  * Handle command line argument and put results into rsp_info structure
  * (if it is an rsplib parameter).
  *
  * @param info rsp_info structure with parameters.
  * @return 1 in case of success, 0 if the parameter is unknown.
  *
  * @see rsp_initarg
  */
int rsp_initarg(struct rsp_info* info, const char* arg);


/**
  * Initialize the RSerPool library. This function must be called before any
  * other RSerPool library function can be used.
  *
  * @param info rsp_info structure with parameters (NULL for defaults).
  * @param 0 in case of success; -1 in case of an error.
  */
int rsp_initialize(struct rsp_info* info);

/**
  * Clean-up the RSerPool library.
  */
void rsp_cleanup();


/*
   ##########################################################################
   #### BASIC MODE API                                                   ####
   ##########################################################################
*/

struct rsp_addrinfo {
   int                  ai_family;
   int                  ai_socktype;
   int                  ai_protocol;
   size_t               ai_addrlen;
   size_t               ai_addrs;
   struct sockaddr*     ai_addr;
   struct rsp_addrinfo* ai_next;
   uint32_t             ai_pe_id;
};

unsigned int rsp_pe_registration(const unsigned char*       poolHandle,
                                 const size_t               poolHandleSize,
                                 struct rsp_addrinfo*       rspAddrInfo,
                                 const struct rsp_loadinfo* rspLoadInfo,
                                 unsigned int               registrationLife,
                                 const int                  flags);
unsigned int rsp_pe_deregistration(const unsigned char* poolHandle,
                                   const size_t         poolHandleSize,
                                   const uint32_t       identifier,
                                   const int            flags);
unsigned int rsp_pe_failure(const unsigned char* poolHandle,
                            const size_t         poolHandleSize,
                            const uint32_t       identifier);


#define RSPGETADDRS_MIN     (size_t)1
#define RSPGETADDRS_MAX     (size_t)0xffffffff
#define RSPGETADDRS_DEFAULT (size_t)0

/**
  * Perform handle resolution.
  *
  * @param poolHandle Pool handle.
  * @param poolHandleSize Pool handle size.
  * @param rserpoolAddrInfo Pointer to variable to store pointer to first rsp_addrinfo to.
  * @param items Desired number of PE entries to obtain.
  * @param staleCacheValue Stale cache value in milliseconds.
  * @return Number of PE entries obtained in case of success; -1 in case of an error.
  *
  * @see rsp_freeaddrinfo
  */
int rsp_getaddrinfo(const unsigned char*  poolHandle,
                    const size_t          poolHandleSize,
                    struct rsp_addrinfo** rserpoolAddrInfo,
                    const size_t          items,
                    const unsigned int    staleCacheValue);

/**
  * Free rsp_addrinfo structure.
  *
  * @param rserpoolAddrInfo Pointer to rsp_addrinfo to be freed.
  */
void rsp_freeaddrinfo(struct rsp_addrinfo* rserpoolAddrInfo);



/*
   ##########################################################################
   #### ENHANCED MODE API                                                ####
   ##########################################################################
*/


typedef int rserpool_session_t;

#define SESSION_SETSIZE 16384


#define MSG_RSERPOOL_NOTIFICATION (1 << 0)
#define MSG_RSERPOOL_COOKIE_ECHO  (1 << 1)


struct rsp_sndrcvinfo
{
   rserpool_session_t rinfo_session;
   uint32_t           rinfo_ppid;
   uint32_t           rinfo_pe_id;
   uint32_t           rinfo_timetolive;
   uint16_t           rinfo_stream;
};


struct rserpool_failover
{
   uint16_t           rf_type;
   uint16_t           rf_flags;
   uint32_t           rf_length;
   unsigned int       rf_state;
   rserpool_session_t rf_session;
   unsigned char      rf_has_cookie;
};

#define RSERPOOL_FAILOVER_NECESSARY 1
#define RSERPOOL_FAILOVER_COMPLETE  2


struct rserpool_session_change
{
   uint16_t           rsc_type;
   uint16_t           rsc_flags;
   uint32_t           rsc_length;
   unsigned int       rsc_state;
   rserpool_session_t rsc_session;
};

#define RSERPOOL_SESSION_ADD    1
#define RSERPOOL_SESSION_REMOVE 2


struct rserpool_shutdown_event
{
   uint16_t           rse_type;
   uint16_t           rse_flags;
   uint32_t           rse_length;
   rserpool_session_t rse_session;
};


union rserpool_notification
{
   struct {
      uint16_t rn_type;
      uint16_t rn_flags;
      uint32_t rn_length;
   }                              rn_header;
   struct rserpool_failover       rn_failover;
   struct rserpool_session_change rn_session_change;
   struct rserpool_shutdown_event rn_shutdown_event;
};

#define RSERPOOL_SESSION_CHANGE 1
#define RSERPOOL_FAILOVER       2
#define RSERPOOL_SHUTDOWN_EVENT 3


/* #######################################################################
   #### RSerPool Socket Functions                                     ####
   ####################################################################### */

/**
  * Creation of a RSerPool socket.
  *
  * @param domain Socket domain (e.g. AF_INET, AF_INET6, AF_UNSPEC).
  * @param type Socket type (e.g. SOCK_DGRAM, SOCK_STREAM, SOCK_SEQPACKET).
  * @param protocol Socket protocol (usually IPPROTO_SCTP).
  * @return Socket descriptor or -1 in case of an error.
  */
int rsp_socket(int domain, int type, int protocol);

/**
  * Bind RSerPool socket to address(es).
  *
  * @param sd Socket descriptor.
  * @param addrs Pointer to packed address array.
  * @param addrcnt Number of addresses in address array.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_bind(int sd, struct sockaddr* addrs, int addrcnt);

/**
  * Put RSerPool socket (PE only) into listening mode.
  *
  * @param sd Socket descriptor.
  * @param backlog Backlog.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_listen(int sd, int backlog);

/**
  * Get PE's pool handle and identifier.
  *
  * @param sd Socket descriptor.
  * @param poolHandle Buffer to store pool handle.
  * @param poolHandleSize Pointer to variable containing maximum pool handle size. In case of success, the actual pool handle size will be stored there.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_getsockname(int            sd,
                    unsigned char* poolHandle,
                    size_t*        poolHandleSize,
                    uint32_t*      identifier);

/**
  * Get peer PE's pool handle and identifier.
  *
  * @param sd Socket descriptor.
  * @param poolHandle Buffer to store pool handle.
  * @param poolHandleSize Pointer to variable containing maximum pool handle size. In case of success, the actual pool handle size will be stored there.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_getpeername(int            sd,
                    unsigned char* poolHandle,
                    size_t*        poolHandleSize,
                    uint32_t*      identifier);

/**
  *
  *
  * @param
  * @param
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_();

/**
  * Close RSerPool socket.
  *
  * @param sd Socket descriptor.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_close(int sd);

/**
  * Poll function for RSerPool sockets.
  *
  * @param ufds Poll array.
  * @param nfds Number of FDs in poll array.
  * @param timeout Timeout in milliseconds; -1 for infinite.
  * @return Number of FDs with events.
  */
int rsp_poll(struct pollfd* ufds, unsigned int nfds, int timeout);

/**
  * Select function for RSerPool sockets.
  *
  * @param n Maximum FD number plus 1.
  * @param readfds Read FD set.
  * @param writefds Write FD set.
  * @param exceptfds Exception FD set.
  * @param timeout Timeout in form of timeval structure (NULL for infinite).
  * @return Number of FDs with events.
  */
int rsp_select(int             n,
               fd_set*         readfds,
               fd_set*         writefds,
               fd_set*         exceptfds,
               struct timeval* timeout);

/**
  * Get RSerPool socket option.
  *
  * @param sd RSerPool socket descriptor.
  * @param level Level number.
  * @param optname Option number.
  * @param optval Buffer to store option value to.
  * @param optlen Pointer to option length. After returning, the value is set to the actual option length.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_getsockopt(int sd, int level, int optname, void* optval, socklen_t* optlen);

/**
  * Set RSerPool socket option.
  *
  * @param sd RSerPool socket descriptor.
  * @param level Level number.
  * @param optname Option number.
  * @param optval Option value.
  * @param optlen Length of the option value.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_setsockopt(int sd, int level, int optname, const void* optval, socklen_t optlen);


/* #######################################################################
   #### Pool Element Functions                                        ####
   ####################################################################### */

/**
  * Register a pool element. Upon registration, the RSerPool socket becomes a Pool Element
  * socket.
  *
  * @param sd RSerPool socket descriptor.
  * @param poolHandle Pool handle.
  * @param poolHandleSize Size of the pool handle in bytes.
  * @param loadinfo rsp_loadinfo structure describing the pool policy (NULL for default, i.e. Round Robin).
  * @param reregistrationInterval Reregistration interval in milliseconds.
  * @param flags Flags (Set REGF_DONTWAIT in order to avoid blocking until reception of Registration Response).
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_register(int                        sd,
                 const unsigned char*       poolHandle,
                 const size_t               poolHandleSize,
                 const struct rsp_loadinfo* loadinfo,
                 unsigned int               reregistrationInterval,
                 int                        flags);


/**
  * Deregister a pool element.
  *
  * @param sd RSerPool socket descriptor.
  * @param flags Flags.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_deregister(int sd,
                   int flags);

/**
  * Accept incoming session on RSerPool Pool Element socket.
  *
  * @param sd RSerPool socket descriptor.
  * @param timeout Timeout in milliseconds; -1 for infinite.
  * @return RSerPool socket descriptor for the new session in case of success; -1 in case of an error.
  */
int rsp_accept(int sd,
               int timeout);


/* #######################################################################
   #### Pool User Functions                                           ####
   ####################################################################### */

/**
  * Connect a RSerPool socket to a pool. Upon connection establishment, the RSerPool Socket becomes
  * a description for the new session.
  *
  * @param sd RSerPool socket descriptor.
  * @param poolHandle Pool handle.
  * @param poolHandleSize Size of the pool handle in bytes.
  * @param staleCacheValue Stale cache value in milliseconds.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_connect(int                  sd,
                const unsigned char* poolHandle,
                const size_t         poolHandleSize,
                const unsigned int   staleCacheValue);

/**
  * Check, if a RSerPool socket has received a cookie from its PE.
  *
  * @param sd RSerPool socket descriptor.
  * @return 1, if a cookie has been received; 0 otherwise.
  */
int rsp_has_cookie(int sd);


#define FFF_NONE        0                 /* Normal failover, no report */
#define FFF_UNREACHABLE (1 << 0)          /* Report unreachable PE      */

/**
  * For a failover to a new Pool Element.
  *
  * @param sd RSerPool socket descriptor.
  * @param flags Flags.
  * @param staleCacheValue Stale cache value in milliseconds.
  * @return 0 in case of success; -1 in case of an error.
  */
int rsp_forcefailover(int                sd,
                      const unsigned int flags,
                      const unsigned int staleCacheValue);


/* #######################################################################
   #### Session Input/Output Functions                                ####
   ####################################################################### */

/**
  * Send data to a RSerPool socket (session).
  *
  * @param sd RSerPool socket descriptor.
  * @param data Data to send.
  * @param dataLength Length of the data in bytes.
  * @param msg_flags Message flags.
  * @param sessionID Session ID (for one-to-many style RSerPool socket only).
  * @param sctpPPID SCTP payload protocol identifier.
  * @param sctpStreamID SCTP stream ID.
  * @param sctpTimeToLive SCTP time to live (ignored, if Pr-SCTP is not available).
  * @param sctpFlags SCTP flags.
  * @param timeout Timeout in milliseconds; -1 for infinite.
  * @return length of the sent data in case of success; -1 in case of an error.
  */
ssize_t rsp_sendmsg(int                sd,
                    const void*        data,
                    size_t             dataLength,
                    unsigned int       msg_flags,
                    rserpool_session_t sessionID,
                    uint32_t           sctpPPID,
                    uint16_t           sctpStreamID,
                    uint32_t           sctpTimeToLive,
                    uint16_t           sctpFlags,
                    int                timeout);

/**
  * Send cookie via control channel.
  *
  * @param sd RSerPool socket descriptor.
  * @param data Cookie data to send.
  * @param dataLength Length of the cookie data in bytes.
  * @param sessionID Session ID (for one-to-many style RSerPool socket only).
  * @param timeout Timeout in milliseconds; -1 for infinite.
  * @return length of the sent cookie data in case of success; -1 in case of an error.
  */
ssize_t rsp_send_cookie(int                  sd,
                        const unsigned char* cookie,
                        const size_t         cookieSize,
                        rserpool_session_t   sessionID,
                        int                  timeout);

/**
  * Receive data from a RSerPool socket (session). This function may return partial
  * messages, check for MSG_EOR in msg_flags!
  *
  * @param sd RSerPool socket descriptor.
  * @param buffer Buffer to write the received data into.
  * @param bufferLength Buffer size.
  * @param rinfo rsp_sndrcvinfo structure containing information about the received data (in particular the session ID for one-to-may style RSerPool sockets).
  * @param msg_flags Pointer to message flags. In case of RSerPool notification, this function will set MSG_RSERPOOL_NOTIFICATION; for a received cookie echo, MSG_RSERPOOL_COOKIE_ECHO will be set.
  * @param timeout Timeout in milliseconds; -1 for infinite.
  * @return length of the sent data in case of success; -1 in case of an error.
  */
ssize_t rsp_recvmsg(int                    sd,
                    void*                  buffer,
                    size_t                 bufferLength,
                    struct rsp_sndrcvinfo* rinfo,
                    int*                   msg_flags,
                    int                    timeout);

/**
  * Receive full message from a RSerPool socket (session), i.e. until MSG_EOR. Note, that
  * the message may still be partial when the buffer is too small!
  *
  * @param sd RSerPool socket descriptor.
  * @param buffer Buffer to write the received data into.
  * @param bufferLength Buffer size.
  * @param rinfo rsp_sndrcvinfo structure containing information about the received data (in particular the session ID for one-to-may style RSerPool sockets).
  * @param msg_flags Pointer to message flags. In case of RSerPool notification, this function will set MSG_RSERPOOL_NOTIFICATION; for a received cookie echo, MSG_RSERPOOL_COOKIE_ECHO will be set.
  * @param timeout Timeout in milliseconds; -1 for infinite.
  * @return length of the sent data in case of success; -1 in case of an error.
  */
ssize_t rsp_recvfullmsg(int                    sd,
                        void*                  buffer,
                        size_t                 bufferLength,
                        struct rsp_sndrcvinfo* rinfo,
                        int*                   msg_flags,
                        int                    timeout);

/**
  * Wrapper for rsp_recvmsg().
  *
  * @param fd RSerPool socket descriptor.
  * @param buffer Buffer to write the received data into.
  * @param bufferLength Buffer size.
  * @return length of the sent data in case of success; -1 in case of an error.
  *
  * @see rsp_recvmsg
  */
ssize_t rsp_read(int fd, void* buffer, size_t bufferLength);

/**
  * Wrapper for rsp_recvmsg().
  *
  * @param sd RSerPool socket descriptor.
  * @param buffer Buffer to write the received data into.
  * @param bufferLength Buffer size.
  * @param flags Message flags.
  * @return length of the sent data in case of success; -1 in case of an error.
  *
  * @see rsp_recvmsg
  */
ssize_t rsp_recv(int sd, void* buffer, size_t bufferLength, int flags);

/**
  * Wrapper for rsp_sendmsg().
  *
  * @param fd RSerPool socket descriptor.
  * @param data Data to send.
  * @param dataLength Length of the data in bytes.
  * @return length of the sent data in case of success; -1 in case of an error.
  *
  * @see rsp_sendmsg
  */
ssize_t rsp_write(int fd, const char* data, size_t dataLength);

/**
  * Wrapper for rsp_sendmsg().
  *
  * @param sd RSerPool socket descriptor.
  * @param data Data to send.
  * @param dataLength Length of the data in bytes.
  * @param flags Message flags.
  * @return length of the sent data in case of success; -1 in case of an error.
  *
  * @see rsp_sendmsg
  */
ssize_t rsp_send(int sd, const void* data, size_t dataLength, int flags);


/* #######################################################################
   #### Miscellaneous Functions                                       ####
   ####################################################################### */

/**
  * Map system socket to RSerPool socket.
  *
  * @param sd System socket descriptor.
  * @param toSD Desired RSerPool socket descriptor (-1 for automatic allocation).
  * @return RSerPool socket descriptor or -1 in case of error.
  */
int rsp_mapsocket(int sd, int toSD);

/**
  * Unmap system socket from RSerPool socket.
  *
  * @param sd System socket descriptor.
  * @return 0 in case of success; -1 in case of error.
  */
int rsp_unmapsocket(int sd);

/**
  * Print contents of RSerPool notification.
  *
  * @param notification RSerPool notification.
  * @param fd File descriptor to print notification to (e.g. stdout).
  */
void rsp_print_notification(const union rserpool_notification* notification, FILE* fd);

/**
  * Translate policy ID into textual representation.
  *
  * @param policyType Policy type.
  * @return Name of the policy or NULL if the type is unknown.
  */
const char* rsp_getpolicybytype(unsigned int policyType);

/**
  * Translate policy name into ID.
  *
  * @param policyName Policy name.
  * @return ID of the policy or PPT_UNDEFINED in case of unknown policy.
  */
unsigned int rsp_getpolicybyname(const char* policyName);

/**
  * Set session's application status text for rsp_csp_getcomponentstatus().
  *
  * @param sd SessionDescriptor.
  * @param statusText Status text.
  */
int rsp_csp_setstatus(int                sd,
                      rserpool_session_t sessionID,
                      const char*        statusText);

#ifdef __cplusplus
}
#endif

#endif
